#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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
    void openRecent(const QString & pathName);

private slots:
    void on_menuOpenRecent(QAction *action);

    void on_menuOpenFolder_triggered();

    void on_fillList_currentRowChanged(int currentRow);

    void on_tagsEdit_returnPressed();

    //void changeEvent(QEvent *e);
    bool eventFilter(QObject* obj, QEvent *event);

    void on_searchEdit_returnPressed();

    void on_searchList_currentRowChanged(int currentRow);

private:
    std::unique_ptr<eptg::Model> model;
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
