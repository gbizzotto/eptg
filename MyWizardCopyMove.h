#ifndef DIALOG_COPY_MOVE_H
#define DIALOG_COPY_MOVE_H

#include <atomic>

#include <QWizard>
#include <QDir>
#include <QString>

#include "eptg/project.hpp"
#include "eptg/copy_move_data.hpp"
#include "ui_copy_move_wizard.h"

#include "mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CopyMove; }
QT_END_NAMESPACE

class MyWizardCopyMove : public QWizard, public Ui::CopyMoveWizard
{
    Q_OBJECT

private:
    eptg::Project<QString> & project;
    bool is_move;
    MainWindow * main_window;
    std::atomic_bool go_on;

public:
    std::unique_ptr<eptg::CopyMoveData<QString>> preview;

public:
    MyWizardCopyMove(eptg::Project<QString> & project, QWidget * parent, bool is_move);
    QString get_preview_flat    (const QString & rel_path) const;
    QString get_preview_preserve(const QString & rel_path) const;
    QString get_preview_tag     (const std::vector<std::vector<QString>> & paths) const;
    template<typename C>
    QStringList make_preview_list(const C & rel_path_container);
    void make_preview();
    void display_preview();

private slots:
    void on_toolButton_clicked();
    void on_destFolderLineEdit_textChanged(const QString &arg1);
    void on_CopyMoveWizard_currentIdChanged(int id);
    void on_treeNoneRadio_toggled(bool checked);
    void on_treePreserveRadio_toggled(bool checked);
    void on_treeTagRadio_toggled(bool checked);
    void on_previewButton_clicked();
    void on_treeTagEdit_returnPressed();
    void on_CopyMoveWizard_finished(int result);
};

#endif // DIALOG_COPY_MOVE_H
