#include <cmath>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QKeyEvent>
#include <QSettings>
#include <QAction>
#include <QModelIndex>
#include <QPainter>
#include <QStringListModel>
#include <QModelIndexList>

QString PathAppend(const QString & path1, const QString & path2)
{
    return QDir::cleanPath(path1 + QDir::separator() + path2);
}

std::set<QString> GetCommonTags(eptg::Model & model, QModelIndexList & selected_items)
{
    std::set<QString> tags;
    // get tags for 1st file as baseline
    {
        auto rel_path = selected_items[0].data().toString();
        for (const auto & t : model.get_file(rel_path.toStdString()).tags)
            tags.insert(QString(t.c_str()));
    }
    // filter out tags that are not in the rest of the files
    for (int i=1 ; i<selected_items.size() ; i++)
    {
        const auto & rel_path = selected_items[i].data().toString();
        const eptg::File & file = model.get_file(rel_path.toStdString());
        for (auto it=tags.begin(),end=tags.end() ; it!=end ; )
        {
            if (file.tags.find(it->toStdString()) == file.tags.end())
                tags.erase(it++);
            else
                it++;
        }
    }
    return tags;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->tagsEdit->installEventFilter(this);
    ui->searchEdit->installEventFilter(this);
    ui->fillList->setModel(new QStringListModel());

    // load settings
    QSettings settings("ttt", "eptg");
    int size = settings.beginReadArray("recents");
    for(int i=0 ; i<size ; i++)
    {
        settings.setArrayIndex(i);
        QString path = settings.value("recent").toString();
        ui->menuOpenRecent->addAction(path)->setData(path);
    }
    settings.endArray();

    connect(ui->menuOpenRecent, SIGNAL(triggered(QAction*)), this, SLOT(on_menuOpenRecent(QAction*)));
    connect(ui->fillList->selectionModel()
           ,SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &))
           ,this
           ,SLOT(fillListSelChanged())
           );
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::fillListSelChanged()
{
    QModelIndexList selected_items = ui->fillList->selectionModel()->selectedIndexes();
    if (selected_items.size() == 0)
    {
        ui->tagsEdit->setText("");
        return;
    }

    // prepare preview
    {
        QPixmap image_result(ui->fillPreview->width(), ui->fillPreview->height());
        image_result.fill(Qt::transparent);
        if (selected_items.size() == 1)
        {
            auto rel_path = selected_items[0].data().toString();
            auto currentText = PathAppend(QString(model->path.c_str()), rel_path);
            QPixmap image(currentText);
            if (image.width() > image_result.width() || image.height() > image_result.height())
                image_result = image.scaled(image_result.width(), image_result.height(), Qt::AspectRatioMode::KeepAspectRatio);
            else
                image_result = image;
        }
        else
        {
            int cols = (int) std::ceil(std::sqrt(selected_items.size()));
            int rows = (int) std::ceil(selected_items.size() * 1.0 / cols);
            int w = image_result.width() / cols;
            int h = image_result.height() / rows;
            for (int r = 0, idx=0 ; r<rows ; r++)
                for (int c = 0 ; c<cols && idx<selected_items.size() ; c++,idx++)
                {
                    auto rel_path = selected_items[idx].data().toString();
                    auto currentText = PathAppend(QString(model->path.c_str()), rel_path);
                    QPixmap image(currentText);
                    if (image.width() > w || image.height() > h)
                        image = image.scaled(w-4, h-4, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);

                    QRectF targetRect(w*c + (w-image.width())/2, h*r + (h-image.height())/2, image.width(), image.height());
                    QRectF sourceRect(0, 0, image.width()-1, image.height()-1);
                    QPainter painter(&image_result);
                    painter.drawPixmap(targetRect, image, sourceRect);
                }
        }
        ui->fillPreview->setPixmap(image_result);
    }

    // prepare tag line
    std::set<QString> tags = GetCommonTags(*model, selected_items);
    QString tag_line;
    for (const auto & t : tags)
        tag_line.append(t).append(" ");

    // set tag line into edit
    ui->tagsEdit->setText(tag_line);
    ui->tagsEdit->setFocus();
}

void MainWindow::on_menuOpenRecent(QAction *action)
{
    QString path = action->data().toString();
    openRecent(path);
}

bool MainWindow::eventFilter(QObject* obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        {
            if (keyEvent->key() == Qt::Key_Up)
            {
                auto do_select = [&](QListView * list, QLineEdit * edit)
                    {
                        if (list->model()->rowCount() == 0)
                            return;
                        auto selected_indices = list->selectionModel()->selectedIndexes();
                        if (selected_indices.size() == 0)
                            list->setCurrentIndex(list->model()->index(list->model()->rowCount()-1, 0));
                        int lowest_index = selected_indices[0].row();
                        for (int i=1 ; i<selected_indices.size() ; i++)
                            if (selected_indices[i].row() < lowest_index)
                                lowest_index = selected_indices[i].row();
                        if (lowest_index > 0)
                            list->setCurrentIndex(list->model()->index(lowest_index-1, 0));
                        edit->setFocus();
                    };
                if (obj == ui->tagsEdit)
                {
                    do_select(ui->fillList, ui->tagsEdit);
                    return true;
                }
                else if (obj == ui->searchEdit)
                {
                    if (ui->searchList->currentRow() <= 0)
                        return true;
                    ui->searchList->setCurrentRow(ui->searchList->currentRow() - 1);
                    ui->searchEdit->setFocus();
                    return true;
                }
            }
            else if(keyEvent->key() == Qt::Key_Down)
            {
                auto do_select = [&](QListView * list, QLineEdit * edit)
                    {
                        if (list->model()->rowCount() == 0)
                            return;
                        auto selected_indices = list->selectionModel()->selectedIndexes();
                        if (selected_indices.size() == 0)
                            list->setCurrentIndex(list->model()->index(0, 0));
                        int highest_index = selected_indices[0].row();
                        for (int i=1 ; i<selected_indices.size() ; i++)
                            if (selected_indices[i].row() > highest_index)
                                highest_index = selected_indices[i].row();
                        if (highest_index < list->model()->rowCount() - 1)
                            list->setCurrentIndex(list->model()->index(highest_index+1, 0));
                        edit->setFocus();
                    };
                if (obj == ui->tagsEdit)
                {
                    do_select(ui->fillList, ui->tagsEdit);
                    return true;
                }
                else if (obj == ui->searchEdit)
                {
                    if (ui->searchList->currentRow()+1 >= ui->searchList->count())
                        return true;
                    ui->searchList->setCurrentRow(ui->searchList->currentRow() + 1);
                    ui->searchEdit->setFocus();
                    return true;
                }
            }
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::on_menuOpenFolder_triggered()
{
    QString pathName = QFileDialog::getExistingDirectory(this, "Select folder", "~");
    if (pathName == "")
        return;

    openRecent(pathName);
}

void MainWindow::openRecent(const QString & pathName)
{
    std::function<void(eptg::Model &, const QString, const QString)> inspect_folder;
    inspect_folder = [this,&inspect_folder](eptg::Model & model, const QString base_dir, const QString rel_dir)
        {
            QDir directory(PathAppend(base_dir, rel_dir));
            QStringList images = directory.entryList(QStringList() << "*.jpg" << "*.JPG",QDir::Files);
            for (auto & i : images)
            {
                const auto & list_model = ui->fillList->model();
                i = PathAppend(rel_dir, i);
                list_model->insertRow(list_model->rowCount());
                QModelIndex index = list_model->index(list_model->rowCount()-1, 0);
                list_model->setData(index, i);
            }

            QStringList folders = directory.entryList(QStringList(), QDir::Dirs);
            for (auto & f : folders)
                if ( ! f.startsWith("."))
                    inspect_folder(model, base_dir, PathAppend(rel_dir, f));
        };

    model = eptg::Load(pathName.toStdString());

    const auto & list_model = ui->fillList->model();
    list_model->removeRows( 0, list_model->rowCount() );
    ui->tagsEdit->clear();
    inspect_folder(*model, pathName, ".");
    QPixmap void_image;
    ui->fillPreview->setPixmap(void_image);
    if (list_model->rowCount() > 0)
        ui->fillList->setCurrentIndex(list_model->index(0, 0));
    this->setWindowTitle("eptgQt - " + pathName);

    // set recent menu
    if (ui->menuOpenRecent->actions().size() == 0)
        ui->menuOpenRecent->addAction(pathName);
    else
    {
        // remove duplicates
        for(int i = ui->menuOpenRecent->actions().size() - 1 ; i >= 0 ; i--)
            if (pathName == ui->menuOpenRecent->actions()[i]->text())
                ui->menuOpenRecent->removeAction(ui->menuOpenRecent->actions()[i]);
        // insert
        QAction * qaction = new QAction(pathName);
        qaction->setData(pathName);
        ui->menuOpenRecent->insertAction(ui->menuOpenRecent->actions()[0], qaction);
        // remove excess recents
        while (ui->menuOpenRecent->actions().size() > 4)
            ui->menuOpenRecent->removeAction(ui->menuOpenRecent->actions()[4]);
    }

    // save recent
    QSettings settings("ttt", "eptg");
    settings.beginWriteArray("recents");
    for(int i = 0 ; i < ui->menuOpenRecent->actions().size() ; i++)
    {
        settings.setArrayIndex(i);
        settings.setValue("recent", ui->menuOpenRecent->actions()[i]->text());
    }
    settings.endArray();
}

template<typename T>
std::set<T> Added(const std::set<T> & before, const std::set<T> & after)
{
    std::set<T> result;
    for (const T & t : after)
        if (before.find(t) == before.end())
            result.insert(t);
    return result;
}

void MainWindow::on_tagsEdit_returnPressed()
{
    if ( ! model)
        return;

    auto selected_items = ui->fillList->selectionModel()->selectedIndexes();
    if (selected_items.size() == 0)
        return;

    std::set<QString> common_tags = GetCommonTags(*model, selected_items);
    std::set<QString> typed_tags;
    for (const QString & tag : ui->tagsEdit->text().split(' '))
        if (tag.size() > 0)
            typed_tags.insert(tag);
    std::set<QString> added_tags   = Added(common_tags, typed_tags);
    std::set<QString> removed_tags = Added(typed_tags, common_tags);

    const auto & list_model = ui->fillList->model();
    for (int i=0 ; i<selected_items.size() ; i++)
    {
        int idx = selected_items[i].row();
        auto rel_path = list_model->data(list_model->index(idx, 0)).toString();
        eptg::File & f = model->get_file(rel_path.toStdString());
        for (const QString & t : removed_tags)
            f.erase_tag(t.toStdString());
        for (const QString & t : added_tags)
            f.insert_tag(t.toStdString());
    }

    eptg::Save(model);

    if (selected_items.size() == 1)
        if (selected_items[0].row()+1 < list_model->rowCount())
            ui->fillList->setCurrentIndex(list_model->index(selected_items[0].row()+1, 0));
}

void MainWindow::on_searchEdit_returnPressed()
{
    if ( ! model)
        return;

    ui->searchList->clear();

    std::set<std::string> tags;
    for (const QString & tag : ui->searchEdit->text().split(' '))
        if (tag.size() > 0)
            tags.insert(tag.toStdString());

    QStringList images;
    for (const auto & f : model->get_files(tags))
        images.append(QString(f.first.c_str()));

    ui->searchList->addItems(images);
}

void MainWindow::on_searchList_currentRowChanged(int currentRow)
{
    if (currentRow == -1)
        return;
    auto rel_path = ui->searchList->item(currentRow)->text();
    auto currentText = PathAppend(QString(model->path.c_str()), rel_path);
    QPixmap image(currentText);
    if (image.width() > 800 || image.height() > 600)
        image = image.scaled(800,600, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
    ui->searchPreview->setPixmap(image);

    ui->searchEdit->setFocus();
}
