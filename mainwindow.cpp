#include <thread>
#include <atomic>
#include <cmath>
#include <memory>

#include <QFileDialog>
#include <QKeyEvent>
#include <QSettings>
#include <QAction>
#include <QModelIndex>
#include <QPainter>
#include <QStringListModel>
#include <QModelIndexList>
#include <QKeyEvent>
#include <QScrollBar>
#include <QProcess>
#include <QRgb>
#include <QThread>
#include <QTimer>
#include <QStringList>
#include <QToolTip>
#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "MyDialogFindSimilar.h"
#include "MyDialogProcess.h"
#include "MyWizardCopyMove.h"

#include "eptg/constants.hpp"
#include "eptg/helpers.hpp"
#include "eptg/path.hpp"
#include "eptg/string.hpp"

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
	ui->previewCheckBox->setCheckState(Qt::CheckState::PartiallyChecked);

    ui->tagsEdit   ->installEventFilter(this);
    ui->editTagTags->installEventFilter(this);
    ui->searchEdit ->installEventFilter(this);
	ui->pathEdit   ->installEventFilter(this);

    // load settings
    QSettings settings("ttt", "eptg");
    int recent_count = settings.beginReadArray("recents");
    for(int i=0 ; i<recent_count ; i++)
    {
        settings.setArrayIndex(i);
        QString path = settings.value("recent").toString();
        bool has_same = false;
        for(int i = ui->menuOpenRecent->actions().size() - 1 ; i >= 0 ; i--)
            if (path::are_same(path, ui->menuOpenRecent->actions()[i]->text()))
            {
                has_same = true;
                break;
            }
        if ( ! has_same)
            ui->menuOpenRecent->addAction(path)->setData(path);
    }
    settings.endArray();

	autosave_timer.reset(new QTimer(this));
	connect(autosave_timer.get(), SIGNAL(timeout()), this, SLOT(save(false, false)));
	autosave_timer->start(10000); //time specified in ms

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
void MainWindow::closeEvent(QCloseEvent *)
{
    save();
}

void MainWindow::save(bool force, bool save_typed)
{
	if (save_typed)
	{
		save_current_file_tags();
		save_current_tag_tags();
	}
	auto & project = *project_s.GetSynchronizedProxy();
	if (project)
	{
		if ( ! project->save(force))
			QMessageBox::critical(this, "Save error", "Project was not written to disk");
	}
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
        if (keyEvent->key() == Qt::Key_F && keyEvent->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        {
            ui->tabWidget->setCurrentIndex(0);
            ui->searchEdit->setFocus();
        }
    }

    if (obj == ui->pathEdit)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
			if (  keyEvent->key() == Qt::Key_Up     || keyEvent->key() == Qt::Key_Down
			   || keyEvent->key() == Qt::Key_PageUp || keyEvent->key() == Qt::Key_PageDown )
			{
				save_current_file_tags();
				QCoreApplication::postEvent(ui->fillList, new QKeyEvent(*keyEvent));
				QCoreApplication::postEvent(ui->fillList, new QKeyEvent(QEvent::KeyPress, Qt::Key_F2, Qt::KeyboardModifier::NoModifier));
				return true;
			}
			else if (keyEvent->key() == Qt::Key_Escape)
            {
                ui->pathEdit->setSelection(0,0);
                ui->tagsEdit->setFocus();
                return true;
            }
        }
	}
    else if (obj == ui->tagsEdit || obj == ui->editTagTags || obj == ui->searchEdit)
    {
        QLineEdit *edit = static_cast<QLineEdit*>(obj);
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (  keyEvent->key() == Qt::Key_Up     || keyEvent->key() == Qt::Key_Down
               || keyEvent->key() == Qt::Key_PageUp || keyEvent->key() == Qt::Key_PageDown )
            {
                if (edit == ui->tagsEdit || edit == ui->searchEdit)
                {
                    save_current_file_tags();
                    QCoreApplication::postEvent(ui->fillList, new QKeyEvent(*keyEvent));
                }
                else if (edit == ui->editTagTags)
                {
                    save_current_tag_tags();
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
    QString pathName = QFileDialog::getExistingDirectory(this, "Select folder", QDir::homePath());
    if (pathName == "")
        return;

    open(pathName);
}

void MainWindow::open(const QString & pathName)
{
    save();

	try
	{
		auto new_project = std::make_unique<eptg::Project<QString>>(pathName, read_file(path::append(pathName, PROJECT_FILE_NAME)));
		new_project->sweep();
		auto & project = *project_s.GetSynchronizedProxy();
		project.swap(new_project);
	}
	catch (std::runtime_error & err)
	{
		QMessageBox::critical(this, "Load error", "Failed to load project at '" + pathName + "'");
		return;
	}

    adjust_ui_for_project();
}

void MainWindow::add_open_recent(const QString & pathName)
{
	// remove duplicates
	for(int i = ui->menuOpenRecent->actions().size() - 1 ; i >= 0 ; i--)
		if (path::are_same(pathName, ui->menuOpenRecent->actions()[i]->text()))
			ui->menuOpenRecent->removeAction(ui->menuOpenRecent->actions()[i]);
    // set recent menu
    if (ui->menuOpenRecent->actions().size() == 0)
        ui->menuOpenRecent->addAction(pathName);
    else
	{
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

void MainWindow::adjust_ui_for_project()
{
	auto & project = *project_s.GetSynchronizedProxy();
	this->add_open_recent(project->get_path());

    ui->fillList->clear();
	for (const auto & p : project->get_files().collection)
        ui->fillList->addItem(p.first);
	statusCountLabel->setText(QString::number(project->get_files().size()) + " files");
	if (project->get_files().size() == 0)
        statusPercentTaggedLabel->setText("");
    else
    {
        double percent_tagged = 0;
		percent_tagged = 100.0 * project->get_files().count_tagged() / project->get_files().size();
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
	this->setWindowTitle("eptgQt - " + project->get_path());

    ui->tagsEdit->setFocus();

    // known tags for autocompletion
    known_tags.clear();
	for (const auto & p : project->get_files().collection)
        for (const QString & t : p.second.inherited_tags)
        {
            if (known_tags.find(t) == known_tags.end())
                known_tags[t] = 0;
            known_tags[t]++;
        }
	for (const auto & [key,val] : project->get_tags().collection)
    {
        if (known_tags.find(key) == known_tags.end())
            known_tags[key] = 0;
        for (const QString & t : val.inherited_tags)
            if (known_tags.find(t) == known_tags.end())
                known_tags[t] = 0;
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
    ui->tagList->sortItems(1, Qt::SortOrder::DescendingOrder);
}

void MainWindow::refresh_tag_list()
{
    // save selected items
    std::set<QString> selected_tag_names = names_from_list(ui->tagList->selectedItems());

    // update list
    ui->tagList->clearContents();
    ui->tagList->setRowCount(int(known_tags.size()));
    int i = 0;
    for (const auto & [tag,count] : known_tags)
    {
        QTableWidgetItem * item_name = new QTableWidgetItem(tag);
        ui->tagList->setItem(i, 0, item_name);
        QTableWidgetItem * item_count = new QTableWidgetItem();
        item_count->setData(Qt::EditRole, count);
        ui->tagList->setItem(item_name->row(), 1, item_count);
        i++;
    }

    // restore selected items
    ui->tagList->setSelectionMode(QAbstractItemView::MultiSelection);
    for (int i=0 ; i<ui->tagList->rowCount() ; i++)
        if (selected_tag_names.find(ui->tagList->item(i,0)->data(Qt::DisplayRole).toString()) != selected_tag_names.end())
            ui->tagList->selectRow(i);
    ui->tagList->setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void MainWindow::save_current_file_tags()
{
	auto & project = *project_s.GetSynchronizedProxy();
    if ( ! project)
        return;

    auto selected_taggables_names = names_from_list(ui->fillList->selectionModel()->selectedIndexes());
    auto typed_tags = eptg::str::unique_tokens(ui->tagsEdit->text());

    project->set_common_tags<false>(selected_taggables_names, typed_tags,
        [this](const QString & tag, int increment)
        {
            if (increment > 0)
            {
                auto it = known_tags.lower_bound(tag);
                if (it != known_tags.end() && it->first == tag)
                    it->second += increment;
                else
                    it = known_tags.insert({tag, increment}).first;
            }
            else if (increment < 0)
            {
                auto it = known_tags.find(tag);
                if (it != known_tags.end())
                    it->second += increment;
            }
        });
}

void MainWindow::select_next_tag()
{
	auto selected_items = ui->tagList->selectedItems();
	// get highest selected index
	int idx = 0;
	for (const auto & sel : selected_items)
		if (sel->row() > idx)
			idx = sel->row();
	if (idx+1 < ui->tagList->rowCount())
		ui->tagList->selectRow(idx+1);
}

void MainWindow::select_next_file()
{
	// get highest selected index
	auto selected_items = ui->fillList->selectionModel()->selectedIndexes();
	int idx = 0;
	for (const auto & sel : selected_items)
		if (sel.row() > idx)
			idx = sel.row();
	idx = (idx+1) % ui->fillList->model()->rowCount();
	ui->fillList->setCurrentIndex(ui->fillList->model()->index(idx, 0));
}

void MainWindow::on_tagsEdit_returnPressed()
{
	save_current_file_tags();
	select_next_file();

	auto & project = *project_s.GetSynchronizedProxy();
	if (project->get_files().size() == 0)
        statusPercentTaggedLabel->setText("");
    else
    {
        double percent_tagged = 0;
		percent_tagged = 100.0 * project->get_files().count_tagged() / project->get_files().size();
        QString str = QString::number(percent_tagged);
        str.truncate(4);
        if (str.endsWith(","))
            str.chop(1);
        statusPercentTaggedLabel->setText(str + "% tagged");
    }
}

void MainWindow::on_searchEdit_returnPressed()
{
	auto & project = *project_s.GetSynchronizedProxy();
    if ( ! project)
        return;

    ui->fillList->clear();

	std::vector<QString> rel_paths;
	try {
		rel_paths = project->search(SearchNode(ui->searchEdit->text()));
	} catch (std::runtime_error & err) {
		QToolTip::showText(ui->searchEdit->mapToGlobal(QPoint(0, 0)), err.what(), nullptr, QRect(), 3000);
	}

    for (const auto & rel_path : rel_paths)
		ui->fillList->addItem(rel_path);

	statusCountLabel->setText(QString::number(rel_paths.size()) + " / " + QString::number(project->get_files().size()) + " files");
    ui->fillList->setCurrentIndex(ui->fillList->model()->index(0, 0));

    ui->searchEdit->deselect();
    if (ui->searchEdit->text().size() > 0)
        ui->searchEdit->setText(ui->searchEdit->text() + " ");
    ui->searchEdit->setCursorPosition(ui->searchEdit->text().size());
}

void MainWindow::save_current_tag_tags()
{
	auto & project = *project_s.GetSynchronizedProxy();
    if ( ! project)
        return;

    auto selected_taggables_names = names_from_list(ui->tagList->selectedItems());
    auto typed_tags = eptg::str::unique_tokens(ui->editTagTags->text());

    project->set_common_tags<true>(selected_taggables_names, typed_tags,
        [this](const QString & tag, int increment)
        {
            if (increment > 0)
            {
                auto it = known_tags.lower_bound(tag);
                if (it != known_tags.end() && it->first == tag)
                    it->second += increment;
                else
                    it = known_tags.insert({tag, increment}).first;
            }
            else if (increment < 0)
            {
                auto it = known_tags.find(tag);
                if (it != known_tags.end())
                    it->second += increment;
            }
        },
        [this](const QString & selected_tag_name, const QString & st)
        {
            if (selected_tag_name == st)
                QToolTip::showText(ui->editTagTags->mapToGlobal(QPoint(0, 0)), "Can't self-tag.", nullptr, QRect(), 2000);
            else
            {
                QString msg;
                msg << "Can't add tag '" << st << "' to tag " << selected_tag_name << "' because '" << selected_tag_name
                    << "' or one of its parent tags has already been tagged '" << st << "'.";
                QToolTip::showText(ui->editTagTags->mapToGlobal(QPoint(0, 0)), msg, nullptr, QRect(), 5000);
            }
        });

    refresh_tag_list();
}
void MainWindow::on_editTagTags_returnPressed()
{
    save_current_tag_tags();
	select_next_tag();

    refresh_tag_list();
}

void MainWindow::on_tagList_itemSelectionChanged()
{
    std::set<QString> selected_names = names_from_list(ui->tagList->selectedItems());

    if (selected_names.size() == 0)
    {
        ui->editTagTags->setText("");
        return;
    }

	auto & project = *project_s.GetSynchronizedProxy();

    // prepare preview
    QStringList hierarchy;

	for ( std::set<QString> tags = project->get_common_tags(project->get_tags().get_all_by_name(selected_names))
        ; tags.size() > 0
		; tags = project->get_common_tags(project->get_tags().get_all_by_name(tags)) )
    {
        hierarchy.insert(0, accumulate(tags, QStringList()).join(", "));
    }
    hierarchy += QString("<b>").append(accumulate(selected_names, QStringList()).join(", ")).append("</b>");
    for ( std::set<QString> tags = project->get_descendent_tags(selected_names)
        ; tags.size() > 0
        ; tags = project->get_descendent_tags(tags) )
    {
        hierarchy += accumulate(tags, QStringList()).join(", ");
    }

    ui->tagTreePreview->setText(hierarchy.join("<br/>↑<br/>"));

    // set tag line into edit
	std::set<const eptg::taggable<QString>*> selected_tags = project->get_tags().get_all_by_name(selected_names);
    QStringList common_tags = accumulate(project->get_common_tags(selected_tags), QStringList());
    ui->editTagTags->setText(common_tags.join(" ") + (common_tags.empty()?"":" "));
    ui->editTagTags->setFocus();
}

void MainWindow::preview_pictures(const std::set<QString> & selected_items_text)
{
	auto & project = *project_s.GetSynchronizedProxy();

    if (selected_items_text.size() == 0)
	{
        ui->fillPreview->setPixmap(QPixmap());
		statusSizeLabel->setText("");
	}
	else if (ui->previewCheckBox->checkState() == Qt::CheckState::Unchecked)
	{
		ui->fillPreview->setAlignment(Qt::AlignVCenter | Qt::AlignCenter);
		ui->fillPreview->setText("Preview disabled");
		statusSizeLabel->setText(QString::number(selected_items_text.size()) + " selected");
	}
    else if (selected_items_text.size() == 1)
    {
		QSize orig_size;
		QPixmap image;
		int file_size;
		std::tie(image, orig_size, file_size) = make_image(path::append(project->get_path(), *selected_items_text.begin()), ui->fillPreview->size(), ui->fillPreview->size());

		ui->fillPreview->setPixmap(image);
		if (orig_size.isValid())
			statusSizeLabel->setText(QString::number(image.width()) + " x " + QString::number(image.height()) + " px");
		else
			statusSizeLabel->setText(QString::number(file_size) + " bytes");
    }
	else if (selected_items_text.size() > 64 && ui->previewCheckBox->checkState() == Qt::CheckState::PartiallyChecked)
	{
		ui->fillPreview->setAlignment(Qt::AlignVCenter | Qt::AlignCenter);
		ui->fillPreview->setText("Preview disabled:\n"
								 "Too many files selected.");
		statusSizeLabel->setText(QString::number(selected_items_text.size()) + " selected");
	}
	else
	{
		ui->fillPreview->setPixmap(make_preview(project->get_path(), selected_items_text, ui->fillPreview->size()));
		statusSizeLabel->setText(QString::number(selected_items_text.size()) + " selected");
	}
}

void MainWindow::on_fillList_itemSelectionChanged()
{
    std::set<QString> selected_names = names_from_list(ui->fillList->selectionModel()->selectedIndexes());

    // display full path
    if (selected_names.size() == 0)
        ui->pathEdit->setText("");
    else if (selected_names.size() == 1)
    {
        ui->pathEdit->setEnabled(true);
        ui->pathEdit->setText(*selected_names.begin());
        int w = ui->pathEdit->fontMetrics().width(ui->pathEdit->text());
        if (w <= ui->pathEdit->width() - 8)
            ui->pathEdit->setAlignment(Qt::AlignLeft);
        else
            ui->pathEdit->setAlignment(Qt::AlignRight);
    }
    else
    {
        ui->pathEdit->setEnabled(false);
        ui->pathEdit->setAlignment(Qt::AlignLeft);
        ui->pathEdit->setText("Many...");
    }

	this->preview_pictures(selected_names);

    // set tag line into edit
	auto & project = *project_s.GetSynchronizedProxy();
	std::set<const eptg::taggable<QString>*> selected_files = project->get_files().get_all_by_name(selected_names);
    QStringList common_tags = accumulate(project->get_common_tags(selected_files), QStringList());
    ui->tagsEdit->setText(common_tags.join(" ") + (common_tags.empty()?"":" "));
    ui->tagsEdit->setFocus();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    if (index == 1)
    {
        refresh_tag_list();
        ui->tagList->verticalScrollBar()->width();
    }
}

bool MainWindow::open_containing_folder(const QStringList & paths) const
{
    bool success = false;
#if defined(Q_OS_WIN)
    QStringList args;
//    if (!info.isDir())
        args << "/select,";
    args << QDir::toNativeSeparators(path);
    success = QProcess::startDetached("explorer", args)
#elif defined(Q_OS_MAC)
    QStringList args;
    args << "-e";
    args << "tell application \"Finder\"";
    args << "-e";
    args << "activate";
    args << "-e";
    args << "select POSIX file \"" + paths[0] + "\"";
    args << "-e";
    args << "end tell";
    args << "-e";
    args << "return";
    success = QProcess::execute("/usr/bin/osascript", args);
#elif defined(Q_OS_LINUX)
    QProcess browserProc;
    success = QProcess::startDetached(QString("nemo"), paths, QString("."));
    if (!success)
        success = QProcess::startDetached(QString("nautilus"), paths, QString("."));
    const QString error = QString::fromLocal8Bit(browserProc.readAllStandardError());
    success = success && error.isEmpty();
#endif
    return success;
}

void MainWindow::on_menuOpenContainingFolder_triggered()
{
    auto selected_items = ui->fillList->selectionModel()->selectedIndexes();
    if (selected_items.size() == 0)
    {
        QToolTip::showText(ui->tabWidget->mapToGlobal(QPoint(0, 0)), "Nothing to open.", nullptr, QRect(), 2000);
        return;
    }
    std::set<QString> selected_names = names_from_list(selected_items);

//    QStringList qpaths;
//    for (const QString & rel_path : titles)
//        qpaths.append(path::append(project->get_path(), rel_path));

	auto & project = *project_s.GetSynchronizedProxy();
	if (!open_containing_folder(QStringList(path::append(project->get_path(), *selected_names.begin()))))
        QToolTip::showText(QCursor::pos(), "Can't open file browser.", nullptr, QRect(), 2000);
}

void MainWindow::on_fillList_doubleClicked(const QModelIndex &index)
{
	auto & project = *project_s.GetSynchronizedProxy();
	QString path = path::append(project->get_path(), ui->fillList->item(index.row())->text());

    if (!open_containing_folder(QStringList(path)))
        QToolTip::showText(QCursor::pos(), "Can't open file browser.", nullptr, QRect(), 2000);
}

void MainWindow::on_menuQuit_triggered()
{
    close();
    QApplication::quit();
}

void MainWindow::on_actionGoto_first_untagged_triggered()
{
    go_to_first_untagged();
}

void MainWindow::go_to_first_untagged()
{
	auto & project = *project_s.GetSynchronizedProxy();
    for (int i=0 ; i<ui->fillList->count() ; i++)
	{
		const eptg::File<QString> * f = project->get_files().find(ui->fillList->item(i)->data(Qt::DisplayRole).toString());
        if (f == nullptr || f->inherited_tags.size() == 0)
        {
            ui->fillList->setCurrentRow(i);
            ui->tagsEdit->setFocus();
            break;
        }
    }
}

void MainWindow::on_menuFindSimilar_triggered()
{
	auto & project = *project_s.GetSynchronizedProxy();
	std::make_unique<MyDialogFindSimilar>(*project, this)->exec();
}

void MainWindow::show_similar_groups(const std::vector<std::vector<QString>> & groups)
{
    ui->fillList->clear();
    for (auto & group : groups)
    {
        for (auto & str : group)
            ui->fillList->addItem(str);
        ui->fillList->addItem("");
    }
    if (ui->fillList->count() > 0)
        ui->fillList->setCurrentRow(0);
}

void MainWindow::on_menuProcess_triggered()
{
	auto & project = *project_s.GetSynchronizedProxy();
	std::make_unique<MyDialogProcess>(*project, this)->exec();
}

void MainWindow::on_menuCopyFiles_triggered()
{
	auto & project = *project_s.GetSynchronizedProxy();
    std::unique_ptr<MyWizardCopyMove> copy_move_wizard(new MyWizardCopyMove(*project, this, false));
    if (copy_move_wizard->exec())
    {
		auto & project = *project_s.GetSynchronizedProxy();
		if ( ! path::is_sub(project->get_path(), copy_move_wizard->preview->dest))
            this->add_open_recent(copy_move_wizard->preview->dest);
        else
            adjust_ui_for_project();
    }
}

void MainWindow::on_menuMoveFiles_triggered()
{
	auto & project = *project_s.GetSynchronizedProxy();
    std::unique_ptr<MyWizardCopyMove> copy_move_wizard(new MyWizardCopyMove(*project, this, true));
    if (copy_move_wizard->exec())
	{
		auto & project = *project_s.GetSynchronizedProxy();
		if ( ! path::is_sub(project->get_path(), copy_move_wizard->preview->dest))
            this->add_open_recent(copy_move_wizard->preview->dest);
        adjust_ui_for_project();
    }
}

void MainWindow::on_menuSelect_all_triggered()
{
    ui->fillList->selectAll();
}

void MainWindow::on_menuSave_triggered()
{
	save(true);
}

void MainWindow::on_menuRename_triggered()
{
    QString rel_name = ui->pathEdit->text();
    eptg::fs::path p(eptg::str::to<std::string>(rel_name));
    QString stem = eptg::str::to<QString>(p.stem());
    int pos = rel_name.lastIndexOf(stem);
    if (pos != -1)
        ui->pathEdit->setSelection(pos, stem.size());

    ui->pathEdit->setFocus();
}

void MainWindow::on_pathEdit_editingFinished()
{
    std::set<QString> selected_names = names_from_list(ui->fillList->selectionModel()->selectedIndexes());
	if (selected_names.size() != 1)
		return;

	auto & project = *project_s.GetSynchronizedProxy();
    auto status = project->rename(*selected_names.begin(), ui->pathEdit->text());
    if (status == decltype(status)::DifferentFolders)
    {
        QToolTip::showText(ui->pathEdit->mapToGlobal(QPoint(0, 0)), "To change the file's folder, use File>Move", nullptr, QRect(), 3000);
        return;
    }
    else if (status == decltype(status)::DestinationExists)
    {
        QToolTip::showText(ui->pathEdit->mapToGlobal(QPoint(0, 0)), "File already exists.", nullptr, QRect(), 3000);
        return;
    }
    else if (status != decltype(status)::Success)
    {
        QToolTip::showText(ui->pathEdit->mapToGlobal(QPoint(0, 0)), "Unknown error.", nullptr, QRect(), 3000);
        return;
    }

    // restore selected item
    ui->fillList->selectedItems().front()->setData(Qt::EditRole, ui->pathEdit->text());

	select_next_file();
}

void MainWindow::on_previewCheckBox_stateChanged(int)
{
	std::set<QString> selected_names = names_from_list(ui->fillList->selectionModel()->selectedIndexes());

	this->preview_pictures(selected_names);
}


void MainWindow::on_menuClear_recents_triggered()
{
	ui->menuOpenRecent->clear();

	QSettings settings("ttt", "eptg");

	settings.beginWriteArray("recents");
	settings.endArray();
}
