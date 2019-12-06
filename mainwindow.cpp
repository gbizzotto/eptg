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

int GetColumn(const QTableWidgetItem *item) { return item->column(); }
int GetColumn(const QModelIndex      &item) { return item .column(); }
QVariant GetData(const QTableWidgetItem *item) { return item->data(Qt::DisplayRole); }
QVariant GetData(const QModelIndex      &item) { return item .data(Qt::DisplayRole); }

template<typename ITEM_TYPE>
std::set<std::string> GetSelectedRowsTitles(const QList<ITEM_TYPE> & selected_items)
{
    std::set<std::string> result;
    for (int i=0 ; i<selected_items.size() ; i++)
        if (GetColumn(selected_items[i]) == 0)
            result.insert(GetData(selected_items[i]).toString().toStdString());
    return result;
}

template<typename C>
QStringList QStringListFromStd(const C & container)
{
    QStringList result;
    for (const std::string & s : container)
        result.append(QString::fromStdString(s));
    return result;
}

template<typename C>
QStringList QStringListFromStd(C && string_collection)
{
    QStringList result;
    for (const std::string & str : string_collection)
        result += QString::fromStdString(str);
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
}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::showEvent(QShowEvent *)
{
    if (ui->menuOpenRecent->actions().size() > 0)
        this->open(ui->menuOpenRecent->actions()[0]->text());
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
        QLineEdit *edit = static_cast<QLineEdit*>(obj);
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
                    if (edit->text().size() > 0)
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

void MainWindow::open(const QString & pathName)
{
    model = eptg::Load(pathName.toStdString());

    ui->fillList->clear();
    for (const auto & p : model->files.collection)
        ui->fillList->addItem(QString::fromStdString(p.first));
    statusCountLabel->setText(QString::number(model->files.size()) + " files");
    if (model->files.size() == 0)
        statusPercentTaggedLabel->setText("");
    else
    {
        double percent_tagged = 0;
        percent_tagged = 100.0 * model->files.count_tagged() / model->files.size();
        QString str = QString::number(percent_tagged);
        str.truncate(4);
        if (str.endsWith(","))
            str.chop(1);
        statusPercentTaggedLabel->setText(str + "% tagged");
    }
    ui->tagsEdit->clear();
    ui->fillPreview->setPixmap(QPixmap());
    if (ui->fillList->count() > 0)
        ui->fillList->setCurrentRow(0);
    this->setWindowTitle("eptgQt - " + pathName);

    // find first untagged file
    ui->searchEdit->setFocus();
    for (int i=0 ; i<ui->fillList->count() ; i++)
    {
        eptg::File * f = model->files.find(ui->fillList->item(i)->data(Qt::DisplayRole).toString().toStdString());
        if (f == nullptr || f->inherited_tags.size() == 0)
        {
            ui->fillList->setCurrentRow(i);
            ui->tagsEdit->setFocus();
            break;
        }
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
    for (const auto & p : model->files.collection)
        for (const std::string & t : p.second.inherited_tags)
        {
            QString tag(t.c_str());
            if (known_tags.find(tag) == known_tags.end())
                known_tags[tag] = 0;
            known_tags[tag]++;
        }
    for (const auto & [key,val] : model->tags.collection)
    {
        if (known_tags.find(QString::fromStdString(key)) == known_tags.end())
            known_tags[QString::fromStdString(key)] = 0;
        for (const std::string & t : val.inherited_tags)
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
    QStringList cols;
    cols << "Name" << "Use count";
    ui->tagList->setHorizontalHeaderLabels(cols);

    if (ui->tabWidget->currentIndex() == 1)
        refresh_tag_list();
}

void MainWindow::refresh_tag_list()
{
    // save selected items
    std::set<std::string> selected_tag_names = GetSelectedRowsTitles(ui->tagList->selectedItems());

    // update list
    ui->tagList->clearContents();
    ui->tagList->setRowCount(int(known_tags.size()));
    int i = 0;
    for (const auto & [tag,count] : known_tags)
    {
        ui->tagList->setItem(i, 0, new QTableWidgetItem(tag));
        QTableWidgetItem * item = new QTableWidgetItem();
        item->setData(Qt::EditRole, count);
        ui->tagList->setItem(i, 1, item);
        i++;
    }
    ui->tagList->sortItems(1, Qt::SortOrder::DescendingOrder);

    // restore selected items
    ui->tagList->setSelectionMode(QAbstractItemView::MultiSelection);
    for (int i=0 ; i<ui->tagList->rowCount() ; i++)
        if (selected_tag_names.find(ui->tagList->item(i,0)->data(Qt::DisplayRole).toString().toStdString()) != selected_tag_names.end())
            ui->tagList->selectRow(i);
    ui->tagList->setSelectionMode(QAbstractItemView::ExtendedSelection);
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
    std::set<std::string> common_tags = model->get_common_tags(model->files.get_all_by_name(titles));

    std::set<std::string> typed_tags;
    for (const QString & tag : ui->tagsEdit->text().split(' '))
    {
        if (tag.size() == 0)
            continue;
        typed_tags.insert(tag.toStdString());
    }
    std::set<std::string> added_tags   = Added(common_tags, typed_tags);
    std::set<std::string> removed_tags = Added(typed_tags, common_tags);

    for (int i=0 ; i<selected_items.size() ; i++)
    {
        int idx = selected_items[i].row();
        auto rel_path = ui->fillList->item(idx)->data(Qt::DisplayRole).toString();
        if (removed_tags.size() > 0)
        {
            eptg::File * f = model->files.find(rel_path.toStdString());
            if (f != nullptr)
                for (const std::string & t : removed_tags)
                {
                    f->erase_tag(t);
                    // update known_tags
                    auto it = known_tags.find(QString::fromStdString(t));
                    if (it != known_tags.end())
                        it->second--;
                }
        }
        if (added_tags.size() > 0)
        {
            eptg::File & f = model->files.insert(rel_path.toStdString(), eptg::File());
            for (const std::string & t : added_tags)
            {
                f.insert_tag(t);
                // update known_tags
                QString qstr_t = QString::fromStdString(t);
                auto it = known_tags.lower_bound(qstr_t);
                if (it != known_tags.end() || it->first != qstr_t)
                    it = known_tags.insert(std::make_pair(qstr_t, 0)).first;
                it->second++;
            }
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
    idx = (idx+1) % ui->fillList->model()->rowCount();
    ui->fillList->setCurrentIndex(ui->fillList->model()->index(idx, 0));

    if (model->files.size() == 0)
        statusPercentTaggedLabel->setText("");
    else
    {
        double percent_tagged = 0;
        percent_tagged = 100.0 * model->files.count_tagged() / model->files.size();
        QString str = QString::number(percent_tagged);
        str.truncate(4);
        if (str.endsWith(","))
            str.chop(1);
        statusPercentTaggedLabel->setText(str + "% tagged");
    }
}

void MainWindow::on_searchEdit_returnPressed()
{
    if ( ! model)
        return;

    ui->fillList->clear();

    std::vector<std::string> rel_paths = model->search(SearchNode(ui->searchEdit->text().toStdString()));
    for (const auto & rel_path : rel_paths)
        ui->fillList->addItem(QString::fromStdString(rel_path));

    statusCountLabel->setText(QString::number(rel_paths.size()) + " / " + QString::number(model->files.size()) + " files");
    ui->fillList->setCurrentIndex(ui->fillList->model()->index(0, 0));

    ui->searchEdit->deselect();
    if (ui->searchEdit->text().size() > 0)
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

    std::set<std::string> selected_tag_names = GetSelectedRowsTitles(selected_items);
    std::set<std::string> common_tags = model->get_common_tags(model->tags.get_all_by_name(selected_tag_names));

    std::set<std::string> typed_tags;
    for (const QString & tag : ui->editTagTags->text().split(' '))
        if (tag.size() > 0)
            typed_tags.insert(tag.toStdString());
    std::set<std::string> added_tags   = Added(common_tags, typed_tags);
    std::set<std::string> removed_tags = Added(typed_tags, common_tags);

    for (const std::string & selected_tag_name : selected_tag_names)
    {
        if (removed_tags.size() > 0)
        {
            eptg::Tag * t = model->tags.find(selected_tag_name);
            if (t != nullptr)
                for (const std::string & st : removed_tags)
                    t->erase_tag(st);
        }
        if (added_tags.size() > 0)
        {
            eptg::Tag & t = model->tags.insert(selected_tag_name, eptg::Tag());
            for (const std::string & st : added_tags)
            {
                // let's check that the tag added does not already inherit (is tagged with) the selected tag
                // if so, it would create circular inheritance, which does not make sense
                // e.g.: if TZM is a ACTIVISM. Can't make ACTIVISM a TZM.
                eptg::Tag * candidate = model->tags.find(st);
                if (candidate != nullptr && candidate == model->tags.find(selected_tag_name))
                    ui->statusbar->showMessage("Can't self-tag.", 10000);
                else if (candidate != nullptr && model->inherits(*candidate, {selected_tag_name}))
                {
                    QString msg;
                    msg << "Can't add tag '" << QString::fromStdString(st) << "' to tag " << selected_tag_name << "' because '" << selected_tag_name
                        << "' or one of its parent tags has already been tagged '" << QString::fromStdString(st) << "'.";
                    ui->statusbar->showMessage(msg, 10000);
                }
                else
                {
                    t.insert_tag(st);
                    // update known_tags
                    QString qstr_t = QString::fromStdString(st);
                    auto it = known_tags.lower_bound(qstr_t);
                    if (it == known_tags.end() || it->first != qstr_t)
                        it = known_tags.insert(std::make_pair(qstr_t, 0)).first;
                }
            }
        }
    }

    refresh_tag_list();

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

    refresh_tag_list();
}

void MainWindow::on_tagList_itemSelectionChanged()
{
    std::set<std::string> tag_names = GetSelectedRowsTitles(ui->tagList->selectedItems());

    if (tag_names.size() == 0)
    {
        ui->editTagTags->setText("");
        return;
    }

    // prepare preview
    QStringList hierarchy;

    for ( std::set<std::string> tags = model->get_common_tags(model->tags.get_all_by_name(tag_names))
        ; tags.size() > 0
        ; tags = model->get_common_tags(model->tags.get_all_by_name(tags)) )
    {
        hierarchy.insert(0, QStringListFromStd(tags).join(", "));
    }
    hierarchy += QString("<b>").append(QStringListFromStd(tag_names).join(", ")).append("</b>");
    for ( std::set<std::string> tags = model->get_descendent_tags(tag_names)
        ; tags.size() > 0
        ; tags = model->get_descendent_tags(tags) )
    {
        hierarchy += QStringListFromStd(tags).join(", ");
    }

    ui->tagTreePreview->setText(hierarchy.join("<br/>↑<br/>"));

    // set tag line into edit
    std::set<const eptg::taggable*> selected_tags = model->tags.get_all_by_name(tag_names);
    QStringList common_tags = QStringListFromStd(model->get_common_tags(selected_tags));
    ui->editTagTags->setText(common_tags.join(" ") + (common_tags.empty()?"":" "));
    ui->editTagTags->setFocus();
}

void MainWindow::on_fillList_itemSelectionChanged()
{
    std::set<std::string> selected_items_text = GetSelectedRowsTitles(ui->fillList->selectionModel()->selectedIndexes());

    // prepare preview
    {
        if (selected_items_text.size() == 1)
        {
            auto full_path = PathAppend(QString::fromStdString(model->path), QString::fromStdString(*selected_items_text.begin()));
            QPixmap image(full_path);
            if (image.width() > ui->fillPreview->width() || image.height() > ui->fillPreview->height())
                ui->fillPreview->setPixmap(image.scaled(ui->fillPreview->width(), ui->fillPreview->height(), Qt::AspectRatioMode::KeepAspectRatio));
            else
                ui->fillPreview->setPixmap(image);
            statusSizeLabel->setText(QString::number(image.width()) + " x " + QString::number(image.height()));
        }
        else if (selected_items_text.size() > 0)
        {
            QPixmap image_result(ui->fillPreview->width(), ui->fillPreview->height());
            image_result.fill(Qt::transparent);
            int cols = int(std::ceil(std::sqrt(selected_items_text.size())));
            int rows = int(std::ceil(selected_items_text.size() * 1.0 / cols));
            int w = image_result.width() / cols;
            int h = image_result.height() / rows;
            auto it = selected_items_text.begin();
            for (int r = 0 ; r<rows ; ++r)
                for (int c = 0 ; c<cols && it!=selected_items_text.end() ; ++c,++it)
                {
                    auto full_path = PathAppend(QString::fromStdString(model->path), QString::fromStdString(*it));
                    QPixmap image(full_path);
                    if (image.width() > w || image.height() > h)
                        image = image.scaled(w-IMG_BORDER, h-IMG_BORDER, Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation);
                    QRectF targetRect(w*c + (w-image.width())/2, h*r + (h-image.height())/2, image.width(), image.height());
                    QRectF sourceRect(0, 0, image.width(), image.height());
                    QPainter painter(&image_result);
                    painter.drawPixmap(targetRect, image, sourceRect);
                }
            statusSizeLabel->setText(QString::number(selected_items_text.size()) + " selected");
            ui->fillPreview->setPixmap(image_result);
        }
    }

    // set tag line into edit
    std::set<const eptg::taggable*> selected_files = model->files.get_all_by_name(selected_items_text);
    QStringList common_tags = QStringListFromStd(model->get_common_tags(selected_files));
    ui->tagsEdit->setText(common_tags.join(" ") + (common_tags.empty()?"":" "));
    ui->tagsEdit->setFocus();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (index == 1)
        refresh_tag_list();
}
