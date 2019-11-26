#include <cmath>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "constants.hpp"
#include <QFileDialog>
#include <QKeyEvent>
#include <QSettings>
#include <QAction>
#include <QModelIndex>
#include <QPainter>
#include <QStringListModel>
#include <QModelIndexList>
#include <QKeyEvent>

QString PathAppend(const QString & path1, const QString & path2)
{
    if (path1.size() == 0)
        return path2;
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

    statusCountLabel = new QLabel(this);
    ui->statusbar->addWidget(statusCountLabel);
    statusPercentTaggedLabel = new QLabel(this);
    ui->statusbar->addWidget(statusPercentTaggedLabel);
    statusSizeLabel = new QLabel(this);
    ui->statusbar->addWidget(statusSizeLabel);

    ui->tagsEdit->installEventFilter(this);
    ui->searchEdit->installEventFilter(this);
    list_model.reset(new QStringListModel());
    ui->fillList->setModel(list_model.get());
    statusCountLabel->setText(QString(std::to_string(list_model->rowCount()).c_str()) + " files");


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
            statusSizeLabel->setText(QString(std::to_string(image.width()).c_str()) + " x " + QString(std::to_string(image.height()).c_str()));
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
                        image = image.scaled(w-IMG_BORDER, h-IMG_BORDER, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);

                    QRectF targetRect(w*c + (w-image.width())/2, h*r + (h-image.height())/2, image.width(), image.height());
                    QRectF sourceRect(0, 0, image.width(), image.height());
                    QPainter painter(&image_result);
                    painter.drawPixmap(targetRect, image, sourceRect);
                }
            statusSizeLabel->setText(QString(std::to_string(selected_items.size()).c_str()) + " selected");
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
    //ui->tagsEdit->setFocus();
}

void MainWindow::on_menuOpenRecent(QAction *action)
{
    QString path = action->data().toString();
    open(path);
}

bool MainWindow::eventFilter(QObject* obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        {
            if (obj == ui->tagsEdit || obj == ui->searchEdit)
            {
                if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down)
                {
                    QCoreApplication::postEvent(ui->fillList, new QKeyEvent(*keyEvent));
                    return true;
                }
                else if (keyEvent->key() == Qt::Key_Escape)
                {
                    ui->searchEdit->setText("");
                    on_searchEdit_returnPressed();
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

    open(pathName);
}

std::unique_ptr<QStringListModel> SweepFolder(const QString & pathName)
{
    std::function<void(QStringListModel*, const QString, const QString)> inspect_folder;
    inspect_folder = [&inspect_folder](QStringListModel * list_model, const QString base_dir, const QString rel_dir)
        {
            QDir directory(PathAppend(base_dir, rel_dir));
            QStringList files = directory.entryList(QDir::Files);
            QStringList extensions = {"jpg", "jpeg", "png", "gif"};
            for (QString & f : files)
            {
                for (const QString & ext : extensions)
                    if (f.toLower().endsWith(ext))
                    {
                        f = PathAppend(rel_dir, f);
                        list_model->insertRow(list_model->rowCount());
                        QModelIndex index = list_model->index(list_model->rowCount()-1, 0);
                        list_model->setData(index, f);
                        break;
                    }
            }

            QStringList folders = directory.entryList(QStringList(), QDir::Dirs);
            for (auto & f : folders)
                if ( ! f.startsWith("."))
                    inspect_folder(list_model, base_dir, PathAppend(rel_dir, f));
        };

    std::unique_ptr<QStringListModel> result(new QStringListModel());
    inspect_folder(result.get(), pathName, "");
    return result;
}

void MainWindow::open(const QString & pathName)
{
    model = eptg::Load(pathName.toStdString());
    list_model = SweepFolder(pathName);
    ui->fillList->setModel(list_model.get());
    connect(ui->fillList->selectionModel()
           ,SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &))
           ,this
           ,SLOT(fillListSelChanged())
           );
    statusCountLabel->setText(QString(std::to_string(list_model->rowCount()).c_str()) + " files");
    {
        double percent_tagged = 0;
        if (list_model->rowCount() != 0)
            percent_tagged = 100.0 * model->files.size() / list_model->rowCount();
        std::string str = std::to_string(percent_tagged);
        str.resize(4);
        statusPercentTaggedLabel->setText(QString(str.c_str()) + "% tagged");
    }
    ui->tagsEdit->clear();
    ui->fillPreview->setPixmap(QPixmap());
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
        while (ui->menuOpenRecent->actions().size() > MAX_RECENT)
            ui->menuOpenRecent->removeAction(ui->menuOpenRecent->actions()[MAX_RECENT]);
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

    {
        double percent_tagged = 0;
        if (list_model->rowCount() != 0)
            percent_tagged = 100.0 * model->files.size() / list_model->rowCount();
        std::string str = std::to_string(percent_tagged);
        str.resize(4);
        statusPercentTaggedLabel->setText(QString(str.c_str()) + "% tagged");
    }
}

std::unique_ptr<QStringListModel> Filter(eptg::Model & model, const std::unique_ptr<QStringListModel> & list_model, const std::set<std::string> & tags)
{
    std::unique_ptr<QStringListModel> result(new QStringListModel());

    for (int i=0 ; i<list_model->rowCount() ; i++)
    {
        const QString & rel_path = list_model->data(list_model->index(i,0)).toString();
        const eptg::File & file = model.get_file(rel_path.toStdString());
        for (const std::string & tag : tags)
        {
            if ( ! file.has_tag(tag))
                break;
            result->insertRow(result->rowCount());
            QModelIndex index = result->index(result->rowCount()-1, 0);
            result->setData(index, rel_path);
        }
    }

    return result;
}

void MainWindow::on_searchEdit_returnPressed()
{
    if ( ! model)
        return;

    std::set<std::string> tags;
    for (const QString & tag : ui->searchEdit->text().split(' '))
        if (tag.size() > 0)
            tags.insert(tag.toStdString());

    if (tags.size() > 0)
    {
        filtered_list_model = Filter(*model, list_model, tags);
        ui->fillList->setModel(filtered_list_model.get());
        connect(ui->fillList->selectionModel()
               ,SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &))
               ,this
               ,SLOT(fillListSelChanged())
               );
        statusCountLabel->setText(QString(std::to_string(filtered_list_model->rowCount()).c_str()) + " files");
    }
    else
    {
        ui->fillList->setModel(list_model.get());
        connect(ui->fillList->selectionModel()
               ,SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &))
               ,this
               ,SLOT(fillListSelChanged())
               );
        statusCountLabel->setText(QString(std::to_string(list_model->rowCount()).c_str()) + " files");
    }
    if (ui->fillList->model()->rowCount() > 0)
        ui->fillList->setCurrentIndex(ui->fillList->model()->index(0, 0));
}
