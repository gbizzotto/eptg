
#include <atomic>

#include "eptg/project.hpp"
#include "eptg/helpers.hpp"

#include "MyDialogFindSimilar.h"
#include "ui_find_similar.h"
#include "mainwindow.h"

#include <QAbstractButton>
#include <QCoreApplication>
#include <QPushButton>

MyDialogFindSimilar::MyDialogFindSimilar(const eptg::Project<QString> & project, QWidget * parent)
    : QDialog(parent)
    , project(project)
{
    setupUi(this);
    go_on.store(false);
}

void MyDialogFindSimilar::on_buttonBox_clicked(QAbstractButton *button)
{
    if (button == (QAbstractButton*)(buttonBox->button(buttonBox->Ok)))
    {
        go_on.store(true);
        this->progressBar->setEnabled(true);
        this->horizontalSlider->setEnabled(false);
        button->setEnabled(false);
		std::vector<std::vector<QString>> groups_of_similars = get_similar(project.get_path(), project.get_files().collection, this->horizontalSlider->value(),
            [this](size_t count, size_t total) -> bool
            {
                this->progressBar->setValue(int(100*count/total));
                QCoreApplication::processEvents();
                return go_on.load();
            });
        MainWindow * main = (MainWindow*) this->parent();
        main->show_similar_groups(groups_of_similars);

        this->progressBar->setEnabled(false);
        this->horizontalSlider->setEnabled(true);
        button->setEnabled(true);
    }
    else if (button == (QAbstractButton*)(buttonBox->button(buttonBox->Cancel)))
    {
        go_on.store(false);
        this->progressBar->setEnabled(false);
        this->horizontalSlider->setEnabled(true);
        button->setEnabled(true);
    }
}

void MyDialogFindSimilar::on_FindSimilar_finished(int)
{
    go_on.store(false);
    this->progressBar->setEnabled(false);
    this->horizontalSlider->setEnabled(true);
    buttonBox->button(buttonBox->Ok)->setEnabled(true);
}
