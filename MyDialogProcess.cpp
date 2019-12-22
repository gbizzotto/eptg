
#include <QWidget>
#include <QAbstractButton>
#include <QProcess>
#include <QFileInfo>
#include <QMessageBox>
#include <QToolTip>
#include <QCursor>

#include "ui_process.h"
#include "MyDialogProcess.h"
#include "eptg/project.hpp"
#include "eptg/helpers.hpp"

#include "mainwindow.h"

MyDialogProcess::MyDialogProcess(const eptg::Project<QString> & project, const std::set<QString> & rel_paths, QWidget * parent)
    : QDialog(parent)
    , project(project)
    , rel_paths(rel_paths)
{
    setupUi(this);
    go_on.store(false);
}

QStringList MyDialogProcess::get_commands() const
{
    const QString base_path = project.path;
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

    for (const QString & rel_path : rel_paths)
    {
        if (QThread::currentThread()->isInterruptionRequested())
            return QStringList();

        QFileInfo info(path::append(base_path, rel_path));
        const QString full_path = path::append(base_path, rel_path);

        auto position_it = positions.begin();
        for (auto command : base_commands)
        {
            if (QThread::currentThread()->isInterruptionRequested())
                return QStringList();

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
    }
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
