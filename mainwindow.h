#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
#include <QLabel>
#include <memory>
#include "model.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void showEvent(QShowEvent *ev);
private:
    void open(const QString & pathName);
private slots:
    void fillListSelChanged();
    void saveCurrentFileTags();
    void saveCurrentTagTags();

    void on_menuOpenRecent(QAction *action);

    void on_menuOpenFolder_triggered();

    void on_tagsEdit_returnPressed();

    bool eventFilter(QObject* obj, QEvent *event);

    void on_searchEdit_returnPressed();

    void on_editTagTags_returnPressed();

    void on_tagList_itemSelectionChanged();

private:
    std::unique_ptr<eptg::Model> model;
    Ui::MainWindow *ui;
    QLabel *statusCountLabel;
    QLabel *statusPercentTaggedLabel;
    QLabel *statusSizeLabel;
    std::unique_ptr<QStringListModel> list_model;
    std::unique_ptr<QStringListModel> filtered_list_model;
    std::map<QString,int> known_tags;
};
#endif // MAINWINDOW_H
