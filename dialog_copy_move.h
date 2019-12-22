#ifndef DIALOG_COPY_MOVE_H
#define DIALOG_COPY_MOVE_H

#include <atomic>
#include <QWizard>
#include <QDir>
#include "ui_copy_move_wizard.h"
#include "project.hpp"
#include "mainwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CopyMove; }
QT_END_NAMESPACE

struct CopyMoveData
{
    enum TreeType
    {
        None,
        Preserve,
        Tag,
    };

    bool is_move;
    bool is_selected;
    QString dest;
    TreeType tree_type;
    bool overwrite;
    QString tag;

    const eptg::Project<QString> & project;
    std::map<QString,QString> files;
    size_t name_collision_count;

    inline CopyMoveData(const eptg::Project<QString> & project, bool move, bool selected, QString dest, TreeType tree, bool overwrite, QString tag="")
        : is_move(move)
        , is_selected(selected)
        , dest(QDir::cleanPath(QDir(dest).isRelative() ? PathAppend(project.path, dest) : std::move(dest)))
        , tree_type(tree)
        , overwrite(overwrite)
        , tag(std::move(tag))
        , project(project)
        , name_collision_count(0)
    {}

    inline bool operator==(const CopyMoveData & other) const
    {
        return is_move     == other.is_move
            && is_selected == other.is_selected
            && dest        == other.dest
            && tree_type   == other.tree_type
            && overwrite   == other.overwrite
            && tag         == other.tag
            ;
    }
    inline bool operator!=(const CopyMoveData & other) const
    {
        return !operator==(other);
    }

    template<typename C_all, typename C_sel>
    void process(const C_all & all_files, const C_sel & selected_files)
    {
        auto add_file = [this](const QString & rel_path)
            {
                QString new_rel_path;
                if (tree_type == TreeType::None)
                    new_rel_path = get_preview_flat(rel_path);
                else if (tree_type == TreeType::Preserve)
                    new_rel_path = get_preview_preserve(rel_path);
                else if (tree_type == TreeType::Tag)
                    new_rel_path = get_preview_tag(rel_path, this->tag);
                if ( ! overwrite)
                    new_rel_path = adjust_for_duplicity(new_rel_path);
                else
                    count_for_duplicity(new_rel_path); // this updates the duplicity counter
                files.emplace(new_rel_path, rel_path);
            };

        if (is_selected)
            for (const QString & rel_path : selected_files)
                add_file(rel_path);
        else
            for (const QString & rel_path : all_files)
                add_file(rel_path);
    }

    inline QString get_preview_flat(const QString & rel_path) const
    {
        return QFileInfo(rel_path).fileName();
    }
    inline QString get_preview_preserve(const QString & rel_path) const
    {
        return rel_path;
    }
    inline QString get_preview_tag(const QString & rel_path, const QString & top_tag) const
    {
        std::vector<std::vector<QString>> paths = project.get_tag_paths(rel_path, top_tag);
        if (paths.empty())
            return get_preview_flat(rel_path);
        else
        {
            return PathAppend(
                    std::accumulate(paths[0].rbegin(), paths[0].rend(), QString(""),
                        [](const QString & result, const QString & folder)
                            {
                                if (result == "")
                                    return folder;
                                else
                                    return PathAppend(result, folder);
                            }
                        ),
                    QFileInfo(rel_path).fileName()
                );
        }
    }

    inline QString adjust_for_duplicity(const QString & rel_path)
    {
        QFileInfo info(rel_path);
        QFileInfo info_new(rel_path);
        int i = 2;
        while(QFileInfo(PathAppend(dest, info_new.fileName())).exists() || in(files, info_new.fileName()))
        {
            QString new_rel_path = info.baseName() + " (" + QString::number(i) + ")." + info.suffix();
            if (info.path() != ".")
                new_rel_path = PathAppend(info.path(), new_rel_path);
            info_new = QFileInfo(new_rel_path);
            i++;
            name_collision_count++;
        }
        return info_new.fileName();
    }
    inline void count_for_duplicity(const QString & new_rel_path)
    {
        if (QFileInfo(PathAppend(dest, new_rel_path)).exists() || in(files, new_rel_path))
            name_collision_count++;
    }
};

class CopyMoveDialog : public QWizard, public Ui::CopyMoveWizard
{
    Q_OBJECT

private:
    eptg::Project<QString> & project;
    bool is_move;
    MainWindow * main_window;
    std::atomic_bool go_on;

public:
    std::unique_ptr<CopyMoveData> preview;

public:
    CopyMoveDialog(eptg::Project<QString> & project, QWidget * parent, bool is_move);
    QString get_preview_flat    (const QString & rel_path) const;
    QString get_preview_preserve(const QString & rel_path) const;
    QString get_preview_tag     (const QString & rel_path, const QString & top_tag) const;
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
