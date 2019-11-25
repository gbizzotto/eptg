#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
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

private:
    void open(const QString & pathName);

private slots:
    void fillListSelChanged();

    void on_menuOpenRecent(QAction *action);

    void on_menuOpenFolder_triggered();

    void on_tagsEdit_returnPressed();

    bool eventFilter(QObject* obj, QEvent *event);

    void on_searchEdit_returnPressed();

private:
    std::unique_ptr<eptg::Model> model;
    Ui::MainWindow *ui;
    std::unique_ptr<QStringListModel> list_model;
    std::unique_ptr<QStringListModel> filtered_list_model;
};
#endif // MAINWINDOW_H
