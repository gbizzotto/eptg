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

std::set<std::string> GetSelectedRowsTitles(const QList<QTableWidgetItem*> & selected_items)
{
    std::set<std::string> result;
    for (int i=0 ; i<selected_items.size() ; i++)
        if (selected_items[i]->column() == 0)
            result.insert(selected_items[i]->data(0).toString().toStdString());
    return result;
}

std::set<QString> QStringSetFromStdStrSet(std::set<std::string> && ss)
{
    std::set<QString> result;
    for (const std::string & str : ss)
        result.insert(QString(str.c_str()));
    return result;
}

template<typename T>
QString & operator<<(QString & out, const T & t)
{
    return out.append(t);
}
template<>
QString & operator<<(QString & out, const std::string & t)
{
    return out.append(t.c_str());
}

QString PathAppend(const QString & path1, const QString & path2)
{
    if (path1.size() == 0)
        return path2;
    return QDir::cleanPath(path1 + QDir::separator() + path2);
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
    ui->editTagTags->installEventFilter(this);
    ui->searchEdit->installEventFilter(this);
    list_model.reset(new QStringListModel());
    ui->fillList->setModel(list_model.get());
    statusCountLabel->setText(QString(std::to_string(list_model->rowCount()).c_str()) + " files");

    // load settings
    QSettings settings("ttt", "eptg");
    int recent_count = settings.beginReadArray("recents");
    for(int i=0 ; i<recent_count ; i++)
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
void MainWindow::showEvent(QShowEvent *ev)
{
    if (ui->menuOpenRecent->actions().size() > 0)
        this->open(ui->menuOpenRecent->actions()[0]->text());
}
void MainWindow::fillListSelChanged()
{
    QModelIndexList selected_items = ui->fillList->selectionModel()->selectedIndexes();

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
        else if (selected_items.size() > 0)
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
    std::set<std::string> titles = GetSelectedRowsTitles(selected_items);
    std::set<eptg::taggable*> selected_tags = model->files.get_all_by_name(titles);
    std::set<QString> common_tags = QStringSetFromStdStrSet(model->get_common_tags(selected_files));

    QString tag_line;
    for (const auto & t : common_tags)
        tag_line.append(t).append(" ");

    // set tag line into edit
    ui->tagsEdit->setText(tag_line);
}

void MainWindow::on_menuOpenRecent(QAction *action)
{
    QString path = action->data().toString();
    open(path);
}

bool MainWindow::eventFilter(QObject* obj, QEvent *event)
{
    if (obj == ui->tagsEdit || obj == ui->editTagTags || obj == ui->searchEdit)
    {
        QLineEdit *edit = ((QLineEdit*)obj);
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down)
            {
                if (edit == ui->tagsEdit || edit == ui->searchEdit)
                {
                    saveCurrentFileTags();
                    QCoreApplication::postEvent(ui->fillList, new QKeyEvent(*keyEvent));
                }
                else if (edit == ui->editTagTags)
                {
                    saveCurrentTagTags();
                    QCoreApplication::postEvent(ui->tagList, new QKeyEvent(*keyEvent));
                }
                return true;
            }
            else if (keyEvent->key() == Qt::Key_Escape)
            {
                ui->searchEdit->setText("");
                on_searchEdit_returnPressed();
                ui->searchEdit->setFocus();
                return true;
            }
            else if (keyEvent->key() == Qt::Key_Tab)
            {
                if (edit->selectedText().size() > 0)
                {
                    edit->deselect();
                    edit->setText(edit->text() + " ");
                    edit->setCursorPosition(edit->text().size());
                    return true;
                }
            }
        }
        else if (event->type() == QEvent::KeyRelease)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (edit->cursorPosition() == edit->text().size())
            {
                if (keyEvent->key() != Qt::Key_Space && keyEvent->key() != Qt::Key_Backspace && keyEvent->key() != Qt::Key_Delete)
                {
                    QString text = edit->text().mid(0, edit->selectionStart());
                    QStringList words = text.split(' ');
                    if (words.size() == 0)
                        return true;
                    QString stub = words.last();
                    if (stub.size() == 0)
                        return true;
                    QString candidate = "";
                    int candidate_grade = -1;
                    for (auto it = known_tags.lower_bound(stub) ; it != known_tags.end() && it->first.startsWith(stub) ; ++it)
                        if (it->second > candidate_grade)
                        {
                            candidate = it->first;
                            candidate_grade = it->second;
                        }
                    QString autocompletion = candidate.mid(stub.size());
                    edit->setText(text + autocompletion);
                    edit->setSelection(text.size(), autocompletion.size());
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

    // find first untagged file
    ui->searchEdit->setFocus();
    for (int i=0 ; i<list_model->rowCount() ; i++)
        if ( ! model->files.has(list_model->data(list_model->index(i,0)).toString().toStdString()))
        {
            ui->fillList->setCurrentIndex(list_model->index(i,0));
            ui->tagsEdit->setFocus();
            break;
        }

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

    // known tags for autocompletion
    known_tags.clear();
    for (const auto & [key,val] : model->files.collection)
        for (const std::string & t : val.tags)
        {
            QString tag(t.c_str());
            if (known_tags.find(tag) == known_tags.end())
                known_tags[tag] = 0;
            known_tags[tag]++;
        }
    for (const auto & [key,val] : model->tags.collection)
    {
        if (known_tags.find(QString(key.c_str())) == known_tags.end())
            known_tags[QString(key.c_str())] = 0;
        for (const std::string & t : val.tags)
        {
            QString tag(t.c_str());
            if (known_tags.find(tag) == known_tags.end())
                known_tags[tag] = 0;
        }
    }

    // Tag list
    ui->tagList->setStyleSheet("QTableWidget::item { padding: 100px }");
    ui->tagList->verticalHeader()->setDefaultSectionSize( 0 );
    ui->tagList->setColumnCount(2);
    ui->tagList->setRowCount(known_tags.size());
    QStringList cols;
    cols << "Name" << "Use count";
    ui->tagList->setHorizontalHeaderLabels(cols);
    int i=0;
    for (const auto & [tag,count] : known_tags)
    {
        ui->tagList->setItem(i, 0, new QTableWidgetItem(tag));
        ui->tagList->setItem(i, 1, new QTableWidgetItem(QString("%1").arg(count)));
        i++;
    }
    ui->tagList->sortItems(1, Qt::SortOrder::DescendingOrder);
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

void MainWindow::saveCurrentFileTags()
{
    if ( ! model)
        return;

    auto selected_items = ui->fillList->selectionModel()->selectedIndexes();
    if (selected_items.size() == 0)
        return;

    std::set<std::string> titles = GetSelectedRowsTitles(selected_items);
    std::set<eptg::taggable*> selected_tags = model->files.get_all_by_name(titles);
    std::set<QString> common_tags = QStringSetFromStdStrSet(model->get_common_tags(selected_files));

    std::set<QString> typed_tags;
    for (const QString & tag : ui->tagsEdit->text().split(' '))
        if (tag.size() > 0)
        {
            typed_tags.insert(tag);
            if (known_tags.find(tag) == known_tags.end())
                known_tags[tag] = 0;
            known_tags[tag]++;
        }
    std::set<QString> added_tags   = Added(common_tags, typed_tags);
    std::set<QString> removed_tags = Added(typed_tags, common_tags);

    const auto & list_model = ui->fillList->model();
    for (int i=0 ; i<selected_items.size() ; i++)
    {
        int idx = selected_items[i].row();
        auto rel_path = list_model->data(list_model->index(idx, 0)).toString();
        eptg::File * f = model->files.get(rel_path.toStdString());
        if (f != nullptr)
            for (const QString & t : removed_tags)
                f->erase_tag(t.toStdString());
        if (added_tags.size() > 0)
        {
            f = model->files.add(rel_path.toStdString());
            for (const QString & t : added_tags)
                f->insert_tag(t.toStdString());
        }
    }

    eptg::Save(model);
}

void MainWindow::on_tagsEdit_returnPressed()
{
    saveCurrentFileTags();

    auto selected_items = ui->fillList->selectionModel()->selectedIndexes();
    // get highest selected index
    int idx = 0;
    for (const auto & sel : selected_items)
        if (sel.row() > idx)
            idx = sel.row();
    if (idx+1 < ui->fillList->model()->rowCount())
        ui->fillList->setCurrentIndex(ui->fillList->model()->index(idx+1, 0));

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
        const eptg::File * file = model.files.get(rel_path.toStdString());
        if (file == nullptr)
            continue;
        for (const std::string & tag : tags)
        {
            if (TaggableHasTag(model, *file, tag))
            {
                result->insertRow(result->rowCount());
                QModelIndex index = result->index(result->rowCount()-1, 0);
                result->setData(index, rel_path);
                break;
            }
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

    ui->searchEdit->deselect();
    ui->searchEdit->setText(ui->searchEdit->text() + " ");
    ui->searchEdit->setCursorPosition(ui->searchEdit->text().size());
}


void MainWindow::saveCurrentTagTags()
{
    if ( ! model)
        return;

    auto selected_items = ui->tagList->selectedItems();
    if (selected_items.size() == 0)
        return;

    std::set<std::string> titles = GetSelectedRowsTitles(selected_items);
    std::set<eptg::taggable*> selected_tags = model->tags.get_all_by_name(titles);
    std::set<QString> common_tags = QStringSetFromStdStrSet(model->get_common_tags(selected_files));

    std::set<QString> typed_tags;
    for (const QString & tag : ui->editTagTags->text().split(' '))
        if (tag.size() > 0)
            typed_tags.insert(tag);
    std::set<QString> added_tags   = Added(common_tags, typed_tags);
    std::set<QString> removed_tags = Added(typed_tags, common_tags);

    for (int i=0 ; i<selected_items.size() ; i++)
    {
        if (selected_items[i]->column() != 0)
            continue;
        const auto & name = selected_items[i]->data(0).toString();
        eptg::Tag * t = model->tags.get(name.toStdString());
        if (t != nullptr)
            for (const QString & st : removed_tags)
                t->erase_tag(st.toStdString());
        if (added_tags.size() > 0)
        {
            t = model->tags.add(name.toStdString());
            for (const QString & st : added_tags)
            {
                // let's check that the tag added does not already inherit (is tagged with) the selected tag
                // if so, it would create circular inheritance, which does not make sense
                // e.g.: if TZM is a ACTIVISM. Can't make ACTIVISM a TZM.
                eptg::Tag * candidate = model->tags.get(st.toStdString());
                if (candidate != nullptr && eptg::TaggableHasTag(*model, *candidate, t->id()))
                {
                    QString msg;
                    msg << "Can't add tag '" << st << "' to tag " << t->id() << "' because '" << t->id()
                        << "' or one of its parent tags has already been tagged '" << st << "'.";
                    ui->statusbar->showMessage(msg, 10000);
                }
                else
                    t->insert_tag(st.toStdString());
            }
        }
    }

    eptg::Save(model);
}
void MainWindow::on_editTagTags_returnPressed()
{
    saveCurrentTagTags();

    auto selected_items = ui->tagList->selectedItems();
    // get highest selected index
    int idx = 0;
    for (const auto & sel : selected_items)
        if (sel->row() > idx)
            idx = sel->row();
    if (idx+1 < ui->tagList->rowCount())
        ui->tagList->selectRow(idx+1);
}

void MainWindow::on_tagList_itemSelectionChanged()
{
    auto selected_items = ui->tagList->selectedItems();
    if (selected_items.size() == 0)
    {
        ui->editTagTags->setText("");
        return;
    }

    // prepare preview
    if (model && selected_items.size() > 0)
    {
        std::string selected_item;

        // get first selected item
        for (int i=0 ; i<selected_items.size() ; i++)
            if (selected_items[0]->column() == 0)
            {
                selected_item = selected_items[i]->data(0).toString().toStdString();
                break;
            }

        if (selected_item.size() > 0)
        {
            std::vector<std::vector<std::string>> hierarchy = {{ selected_item }};
            QString hierarchy_to_display;

            // get parents
            while (hierarchy.back().size() > 0)
                hierarchy.push_back(model->get_parent_tags(hierarchy.back()));
            hierarchy.pop_back(); // remove empty list
            hierarchy.erase(hierarchy.begin()); // remove the current tag
            // format parents
            while(hierarchy.size() > 0)
            {
                while(hierarchy.back().size() > 1)
                {
                    hierarchy_to_display << hierarchy.back().front() << ", ";
                    hierarchy.back().erase(hierarchy.back().begin());
                }
                hierarchy_to_display << hierarchy.back().front() << "<br/>↑<br/>";
                hierarchy.pop_back();
            }
            // format current tag
            hierarchy_to_display << "<b>" << selected_item << "</b>";
            // get descendents
            hierarchy = {{ selected_item }};
            while (hierarchy.back().size() > 0)
                hierarchy.push_back(model->get_descendent_tags(hierarchy.back()));
            hierarchy.pop_back(); // remove empty list
            hierarchy.erase(hierarchy.begin()); // remove the current tag
            // format descendents
            while(hierarchy.size() > 0)
            {
                hierarchy_to_display << "<br/>↑<br/>";
                while(hierarchy.front().size() > 1)
                {
                    hierarchy_to_display << hierarchy.front().front() << ", ";
                    hierarchy.front().erase(hierarchy.front().begin());
                }
                hierarchy_to_display << hierarchy.front().front();
                hierarchy.erase(hierarchy.begin());
            }

            ui->tagTreePreview->setText(hierarchy_to_display);
        }
    }

    // prepare tag line
    std::set<std::string> titles = GetSelectedRowsTitles(selected_items);
    std::set<eptg::taggable*> selected_tags = model->tags.get_all_by_name(titles);
    std::set<QString> common_tags = QStringSetFromStdStrSet(model->get_common_tags(selected_tags));
    QString tag_line;
    for (const auto & t : common_tags)
        tag_line.append(t).append(" ");

    // set tag line into edit
    ui->editTagTags->setText(tag_line);
}
