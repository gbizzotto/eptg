#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
#include <QLabel>
#include <QLineEdit>
#include <QDialog>
#include <memory>
#include "eptg/project.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void show_similar_groups(const std::vector<std::vector<QString>> & groups);
    inline const Ui::MainWindow* Getui() const { return ui; }
    void save_current_file_tags();
    void open(const QString & pathName);
    void add_open_recent(const QString & pathName);
protected:
    void showEvent(QShowEvent *ev);
private slots:
    void save_current_tag_tags();
    void refresh_tag_list();
    bool open_containing_folder(const QStringList & paths) const;
    void go_to_first_untagged();
    void preview_pictures(const std::set<QString> & selected_items_text);
    void adjust_ui_for_project();

    void on_menuOpenRecent(QAction *action);

    void on_menuOpenFolder_triggered();

    void on_tagsEdit_returnPressed();

    bool eventFilter(QObject* obj, QEvent *event);

    void on_searchEdit_returnPressed();

    void on_editTagTags_returnPressed();

    void on_tagList_itemSelectionChanged();

    void on_fillList_itemSelectionChanged();

    void on_tabWidget_currentChanged(int index);

    void on_fillList_doubleClicked(const QModelIndex &index);

    void on_menuQuit_triggered();

    void on_menuOpenContainingFolder_triggered();

    void on_actionGoto_first_untagged_triggered();

    void on_menuFindSimilar_triggered();

    void on_menuProcess_triggered();

    void on_previewCheckBox_toggled(bool checked);

    void on_menuCopyFiles_triggered();

    void on_menuMoveFiles_triggered();

private:
    std::unique_ptr<eptg::Project<QString>> project;
    Ui::MainWindow *ui;
    QLabel *statusCountLabel;
    QLabel *statusPercentTaggedLabel;
    QLabel *statusSizeLabel;
    std::map<QString,int> known_tags;
};
#endif // MAINWINDOW_H
