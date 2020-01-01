#ifndef DIALOG_PROCESS_H
#define DIALOG_PROCESS_H

#include <atomic>

#include <QDialog>
#include <QStringList>
#include <QThread>

#include "eptg/project.hpp"

#include "ui_process.h"
#include "mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Process; }
QT_END_NAMESPACE

class MyDialogProcess : public QDialog, public Ui::Process
{
    Q_OBJECT

private:
	const eptg::Project<QString> & project;
	MainWindow * main_window;
	std::atomic_bool go_on;

    QStringList get_commands() const;

public:
	MyDialogProcess(const eptg::Project<QString> & project, QWidget * parent);
private slots:
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_plainTextEdit_textChanged();
    void on_previewButton_clicked();
    void on_helpButton_clicked();
};

#endif // DIALOG_PROCESS_H
