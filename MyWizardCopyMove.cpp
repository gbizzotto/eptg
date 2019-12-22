
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

    int selected_files_count = main_window->Getui()->fillList->selectionModel()->selectedIndexes().count();
    this->selectedFilesRadio->setEnabled(selected_files_count != 0);
    this->allFilesRadio->setChecked(selected_files_count == 0);
    this->setWindowTitle(is_move ? "Move files" : "Copy files");
    this->selectedFilesRadio->setText(this->selectedFilesRadio->text() + " (" + QString::number(main_window->Getui()->fillList->selectionModel()->selectedIndexes().count()) + ")");
    this->allFilesRadio->setText(this->allFilesRadio->text() + " (" + QString::number(main_window->Getui()->fillList->count()) + ")");
    this->destFolderLineEdit->setText(project.path);
}

void MyWizardCopyMove::on_toolButton_clicked()
{
    QString default_path_for_open_dialog = project.path;
    if (destFolderLineEdit->text().size() != 0)
        default_path_for_open_dialog = destFolderLineEdit->text();
    QString path = QFileDialog::getExistingDirectory(this, "Select folder", default_path_for_open_dialog);
    if (path.size() != 0)
        destFolderLineEdit->setText(path);
}

void MyWizardCopyMove::on_destFolderLineEdit_textChanged(const QString &arg1)
{
    QStringList messages;

    if ( ! path::is_sub(project.path, arg1))
    {
        messages += "- <font color=red>"
                    "Warning: Moving files out of this project.<br/>"
                    "New project will be added to menu File>Open Recent."
                    "</font>";
        if (path::parent_has(arg1, QString(PROJECT_FILE_NAME)))
            messages += "- <font color=black>"
                        "Warning: Moving files to an existing project.<br/>"
                        "Destination project will be updated to include the new files."
                        "</font>";
    }

    this->moveDestWarningLabel->setText(messages.join("<br/><br/>"));
}

void MyWizardCopyMove::make_preview()
{
    auto new_preview = std::make_unique<CopyMoveData>(
         project
        ,is_move
        ,selectedFilesRadio->isChecked()
        ,destFolderLineEdit->text()
        ,treeNoneRadio->isChecked()?CopyMoveData::TreeType::None:(this->treePreserveRadio->isChecked()?CopyMoveData::TreeType::Preserve:CopyMoveData::TreeType::Tag)
        ,overwriteRadio->isChecked()
        ,treeTagEdit->text()
        );

    if ( !preview || *preview != *new_preview)
    {
        preview.swap(new_preview);
        preview->process(keys(project.files.collection)
                        ,names_from_list_selection(main_window->Getui()->fillList->selectionModel()->selectedIndexes())
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
        whatFilesLabel->setText(preview->is_selected
                               ?"Selected files (" + QString::number(main_window->Getui()->fillList->selectionModel()->selectedIndexes().count()) + ")"
                               :"All files (" + QString::number(project.files.size()) + ")"
                               );
        whereToLabel->setText(preview->dest);

        if (preview->tree_type == CopyMoveData::TreeType::None)
            treeStructureLabel->setText("None (flat)");
        else if (preview->tree_type == CopyMoveData::TreeType::Preserve)
            treeStructureLabel->setText("Preserve");
        else if (preview->tree_type == CopyMoveData::TreeType::Tag)
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

    auto dest_project = eptg::load(preview->dest);
    std::set<QString> tags_used;

    bool is_internal = path::is_sub(project.path, preview->dest);

    for (const auto & filenames_tuple : preview->files)
    {
        QString new_rel_path = std::get<0>(filenames_tuple);
        QString old_rel_path = std::get<1>(filenames_tuple);

        QFileInfo dest_info(path::append(preview->dest, new_rel_path));
        QDir dest_dir(dest_info.path());

        if ( ! dest_dir.exists())
            if ( ! dest_dir.mkpath(dest_dir.path()))
            {
                // TODO: report error
                continue;
            }

        // actual files
        QFile::copy(path::append(project.path , old_rel_path)
                   ,path::append(preview->dest, new_rel_path)
                   );
        if (preview->is_move)
            QFile(path::append(project.path, old_rel_path)).remove();

        // files in Project
        eptg::File<QString> & new_file = dest_project->files.insert(new_rel_path, eptg::File<QString>{});
        const eptg::File<QString> * old_file = project.files.find(old_rel_path);

        // tags in Project
        for (const QString & tag : old_file->inherited_tags)
        {
            new_file.insert_tag(tag);
            if ( ! is_internal)
                tags_used.insert(tag);
        }

        if (is_move)
            project.files.erase(old_rel_path);
    }

    // tag inheritance
    std::set<QString> tags_used_done;
    while( ! tags_used.empty())
    {
        const QString & tag = *tags_used.begin();
        const eptg::Tag<QString> * tag_in_old_project =      project. tags.find(tag);
              eptg::Tag<QString> & tag_in_new_project = dest_project->tags.insert(tag, eptg::Tag<QString>{});

        if (tag_in_old_project)
            for (const QString & inherited_tag : tag_in_old_project->inherited_tags)
            {
                tag_in_new_project.insert_tag(inherited_tag);
                if (tags_used_done.find(inherited_tag) == tags_used_done.end())
                    tags_used.insert(inherited_tag);
            }

        tags_used_done.insert(tag);
        tags_used.erase(tag);
    }

    if (is_internal)
        project.absorb(*dest_project);
    else
        eptg::save(dest_project);
}
