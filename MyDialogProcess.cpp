
#include <QWidget>
#include <QAbstractButton>
#include <QProcess>
#include <QFileInfo>
#include <QMessageBox>
#include <QToolTip>
#include <QCursor>

#include "MyDialogProcess.h"
#include "eptg/project.hpp"
#include "eptg/helpers.hpp"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MyDialogProcess::MyDialogProcess(const eptg::Project<QString> & project, QWidget * parent)
    : QDialog(parent)
	, project(project)
	, main_window(static_cast<MainWindow*>(parent))
{
    setupUi(this);
    go_on.store(false);

	int selected_files_count = main_window->get_ui()->fillList->selectionModel()->selectedIndexes().count();
	int shown_files_count = names_from_list(main_window->get_ui()->fillList).size();
	this->selectedFilesRadio->setEnabled(selected_files_count != 0);
	this->shownFilesRadio->setChecked(selected_files_count == 0);
	this->selectedFilesRadio->setText(this->selectedFilesRadio->text() + " (" + QString::number(selected_files_count) + ")");
	this->   shownFilesRadio->setText(this->   shownFilesRadio->text() + " (" + QString::number(shown_files_count   ) + ")");
	this-> projectFilesRadio->setText(this-> projectFilesRadio->text() + " (" + QString::number(project.get_files().size()) + ")");
}

QStringList MyDialogProcess::get_commands() const
{
    const QString base_path = project.get_path();
    const QStringList base_commands = this->plainTextEdit->toPlainText().split('\n', QString::SkipEmptyParts);
    QStringList commands;

    std::vector<std::vector<int>> positions;
    for (const auto & base_command : base_commands)
    {
        if (QThread::currentThread()->isInterruptionRequested())
            return QStringList();

        positions.push_back({});
        for (int pos = base_command.indexOf('%', 0) ; pos != -1 ; pos = base_command.indexOf("%", pos+1))
            positions.back().insert(positions.back().begin(), pos);
    }

	auto make_command = [&](const QString & rel_path)
		{
			QFileInfo info(path::append(base_path, rel_path));
			const QString full_path = path::append(base_path, rel_path);

			auto position_it = positions.begin();
			for (auto command : base_commands)
			{
				for (int pos : *position_it)
				{
					if (pos+1 == command.size())
						continue;
					if (command[pos+1] == 'a')
						command.replace(pos, 2, info.filePath());
					else if (command[pos+1] == 'n')
						command.replace(pos, 2, info.fileName());
					else if (command[pos+1] == 'b')
						command.replace(pos, 2, base_path);
					else if (command[pos+1] == 'r')
						command.replace(pos, 2, rel_path);
					else if (command[pos+1] == 'f')
						command.replace(pos, 2, QFileInfo(rel_path).path());
				}
				commands.append(command);
				++position_it;
			}
		};

	if (selectedFilesRadio->isChecked())
		for (const QString & rel_path : names_from_list(main_window->get_ui()->fillList->selectionModel()->selectedIndexes()))
			if (QThread::currentThread()->isInterruptionRequested())
				return QStringList();
			else
				make_command(rel_path);
	else if (shownFilesRadio->isChecked())
		for (const QString & rel_path : names_from_list(main_window->get_ui()->fillList))
			if (QThread::currentThread()->isInterruptionRequested())
				return QStringList();
			else
				make_command(rel_path);
	else
		for (const QString & rel_path : keys(project.get_files().collection))
			if (QThread::currentThread()->isInterruptionRequested())
				return QStringList();
			else
				make_command(rel_path);

    return commands;
}

void MyDialogProcess::on_buttonBox_clicked(QAbstractButton *button)
{
    if (button == (QAbstractButton*)(buttonBox->button(buttonBox->Ok)))
    {
        go_on.store(true);
        this->progressBar->setEnabled(true);
        this->plainTextEdit->setEnabled(false);
        button->setEnabled(false);

        auto commands = get_commands();
        this->progressBar->setRange(0, commands.size());
        this->progressBar->setValue(0);
        int count = 0;
        QProcess process;
        for (const QString & cmd : commands)
        {
            if ( ! go_on.load())
                break;
            process.start(cmd);
            while ( ! process.waitForFinished(10) && process.state() != QProcess::ProcessState::NotRunning)
            {
                if ( ! go_on.load())
                {
                    process.kill();
                    break;
                }
            }
            QCoreApplication::processEvents();
            count++;
            this->progressBar->setValue(count);
        }

        this->progressBar->setEnabled(false);
        this->plainTextEdit->setEnabled(true);
        button->setEnabled(true);
        this->progressBar->setValue(0);
    }
    else if (button == (QAbstractButton*)(buttonBox->button(buttonBox->Cancel)))
    {
        go_on.store(false);
        this->progressBar->setEnabled(false);
        this->plainTextEdit->setEnabled(true);
        button->setEnabled(true);
    }
}

void MyDialogProcess::on_plainTextEdit_textChanged()
{
//    if (preview_thread)
//    {
//        preview_thread->requestInterruption();
//        preview_thread->wait();
//    }
//    preview_thread.reset(QThread::create([this]()
//        {
//            this->preview_commands = get_commands().join("<br/>");
//        }));
//    preview_thread->start();
//    QThread * launched_thread = preview_thread.get();
//    while( ! preview_thread->wait(1))
//        QCoreApplication::processEvents();
//    if (launched_thread == preview_thread.get() && ! preview_thread->isInterruptionRequested())
//        this->textBrowser->setText(this->preview_commands);
}

void MyDialogProcess::on_previewButton_clicked()
{
    this->textBrowser->setText(get_commands().join("<br/>"));
}

void MyDialogProcess::on_helpButton_clicked()
{
    QToolTip::showText(QCursor::pos(),
                       "%a : absolute path and file name\n"
                       "%n : file name without path\n"
                       "%b : absolute path of base folder\n"
                       "%r : file path and filename relative to base folder\n"
                       "%f : file path relative to base folder",
                       this->helpButton);
}
