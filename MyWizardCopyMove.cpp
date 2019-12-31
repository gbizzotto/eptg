
#include <memory>

#include <QFileDialog>

#include "MyWizardCopyMove.h"
#include "eptg/project.hpp"
#include "eptg/helpers.hpp"
#include "eptg/constants.hpp"
#include "eptg/path.hpp"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MyWizardCopyMove::MyWizardCopyMove(eptg::Project<QString> & project, QWidget * parent, bool is_move)
    : QWizard(parent)
    , project(project)
    , is_move(is_move)
    , main_window(static_cast<MainWindow*>(parent))
{
    setupUi(this);
    go_on.store(false);

    int selected_files_count = main_window->get_ui()->fillList->selectionModel()->selectedIndexes().count();
    int shown_files_count = names_from_list(main_window->get_ui()->fillList).size();
    this->selectedFilesRadio->setEnabled(selected_files_count != 0);
    this->shownFilesRadio->setChecked(selected_files_count == 0);
    this->setWindowTitle(is_move ? "Move files" : "Copy files");
    this->selectedFilesRadio->setText(this->selectedFilesRadio->text() + " (" + QString::number(selected_files_count) + ")");
    this->   shownFilesRadio->setText(this->   shownFilesRadio->text() + " (" + QString::number(shown_files_count   ) + ")");
	this-> projectFilesRadio->setText(this-> projectFilesRadio->text() + " (" + QString::number(project.get_files().size()) + ")");
    this->destFolderLineEdit->setText(project.get_path());
}

void MyWizardCopyMove::on_toolButton_clicked()
{
    QString default_path_for_open_dialog = project.get_path();
    if (destFolderLineEdit->text().size() != 0)
        default_path_for_open_dialog = destFolderLineEdit->text();
    QString path = QFileDialog::getExistingDirectory(this, "Select folder", default_path_for_open_dialog);
    if (path.size() != 0)
        destFolderLineEdit->setText(path);
}

void MyWizardCopyMove::on_destFolderLineEdit_textChanged(const QString &edited_text)
{
    QStringList messages;

    if ( ! path::is_sub(project.get_path(), edited_text))
    {
        messages += "- <font color=red>"
                    "Warning: Moving files out of this project.<br/>"
                    "New project will be added to menu File>Open Recent."
                    "</font>";
        if (path::parent_has(edited_text, QString(PROJECT_FILE_NAME)))
            messages += "- <font color=black>"
                        "Warning: Moving files to an existing project.<br/>"
                        "Destination project will be updated to include the new files."
                        "</font>";
    }

    this->moveDestWarningLabel->setText(messages.join("<br/><br/>"));
}

void MyWizardCopyMove::make_preview()
{
    auto new_preview = std::make_unique<eptg::CopyMoveData<QString>>(
         project.get_path()
        ,is_move
        , selectedFilesRadio->isChecked()
         ?eptg::CopyMoveData<QString>::WhichFiles::Selected
         :( shownFilesRadio->isChecked()
           ?eptg::CopyMoveData<QString>::WhichFiles::Shown
           :eptg::CopyMoveData<QString>::WhichFiles::All
          )
        ,destFolderLineEdit->text()
        ,treeNoneRadio->isChecked()?eptg::CopyMoveData<QString>::TreeType::None:(this->treePreserveRadio->isChecked()?eptg::CopyMoveData<QString>::TreeType::Preserve:eptg::CopyMoveData<QString>::TreeType::Tag)
        ,overwriteRadio->isChecked()
        ,treeTagEdit->text()
        );

    if ( !preview || *preview != *new_preview)
    {
        preview.swap(new_preview);
        preview->process(project
						,keys(project.get_files().collection)
                        ,names_from_list(main_window->get_ui()->fillList)
                        ,names_from_list(main_window->get_ui()->fillList->selectionModel()->selectedIndexes())
                        );
    }
}

void MyWizardCopyMove::display_preview()
{
    make_preview();

    QStringList preview_files;
    for (const QString & new_path : keys(preview->files))
        preview_files.append(path::append(preview->dest, new_path));
    this->previewBrowser->setText(preview_files.join("<br/>"));
}

void MyWizardCopyMove::on_CopyMoveWizard_currentIdChanged(int page_id)
{
    if (page_id == 4)
        display_preview();
    else if (page_id == 5)
    {
        make_preview();
        actionLabel->setText(preview->is_move ? "Move files" : "Copy files");
        whatFilesLabel->setText((preview->which_files == eptg::CopyMoveData<QString>::WhichFiles::Selected)
                               ?"Selected files (" + QString::number(main_window->get_ui()->fillList->selectionModel()->selectedIndexes().count()) + ")"
                               :((preview->which_files == eptg::CopyMoveData<QString>::WhichFiles::Shown)
                                 ?"All shown (" + QString::number(names_from_list(main_window->get_ui()->fillList).size()) + ")"
								 :"All files (" + QString::number(project.get_files().size()) + ")"
                                )
                               );
        whereToLabel->setText(preview->dest);

        if (preview->tree_type == eptg::CopyMoveData<QString>::TreeType::None)
            treeStructureLabel->setText("None (flat)");
        else if (preview->tree_type == eptg::CopyMoveData<QString>::TreeType::Preserve)
            treeStructureLabel->setText("Preserve");
        else if (preview->tree_type == eptg::CopyMoveData<QString>::TreeType::Tag)
            treeStructureLabel->setText("Follow tag '" + preview->tag + "'");

        dupnamesLabel->setText(QString::number(preview->name_collision_count) + (preview->overwrite ? " to overwrite" : " to rename"));
    }
}

void MyWizardCopyMove::on_treeNoneRadio_toggled(bool checked)
{
    if (checked)
        display_preview();
}

void MyWizardCopyMove::on_treePreserveRadio_toggled(bool checked)
{
    if (checked)
        display_preview();
}

void MyWizardCopyMove::on_treeTagRadio_toggled(bool checked)
{
    this->treeTagEdit->setEnabled(checked);
    this->previewButton->setEnabled(checked);
}

void MyWizardCopyMove::on_previewButton_clicked()
{
    display_preview();
}

void MyWizardCopyMove::on_treeTagEdit_returnPressed()
{
    display_preview();
}

void MyWizardCopyMove::on_CopyMoveWizard_finished(int result)
{
    if (result == 0)
        return;

    make_preview();

	eptg::Project<QString> dest_project = project.execute(*preview);

    if (path::is_sub(project.get_path(), preview->dest)) // internal copy or move
		project.absorb(dest_project);
    else
		dest_project.save();
}
