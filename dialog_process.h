#ifndef DIALOG_PROCESS_H
#define DIALOG_PROCESS_H

#include <atomic>
#include <QDialog>
#include <QStringList>
#include <QThread>
#include "ui_process.h"
#include "model.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class Process; }
QT_END_NAMESPACE

class ProcessDialog : public QDialog, public Ui::Process
{
    Q_OBJECT

private:
    const eptg::Model & model;
    std::set<std::string> rel_paths;
    std::atomic_bool go_on;

    QStringList get_commands() const;

public:
    ProcessDialog(const eptg::Model & model, const std::set<std::string> & rel_paths, QWidget * parent);
private slots:
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_plainTextEdit_textChanged();
    void on_previewButton_clicked();
    void on_helpButton_clicked();
};

#endif // DIALOG_PROCESS_H