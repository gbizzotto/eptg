
#include <atomic>

#include "find_similar.h"
#include "ui_find_similar.h"
#include "model.hpp"
#include "helpers.hpp"
#include "mainwindow.h"

#include <QAbstractButton>
#include <QCoreApplication>
#include <QPushButton>

FindSimilarDialog::FindSimilarDialog(const eptg::Model & model, QWidget * parent)
    : QDialog(parent)
    , model(model)
{
    setupUi(this);
    go_on.store(false);
    this->setWindowTitle("Find similar pictures");
}

void FindSimilarDialog::on_buttonBox_accepted()
{
}

void FindSimilarDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if (button == (QAbstractButton*)(buttonBox->button(buttonBox->Ok)))
    {
        go_on.store(true);
        this->progressBar->setEnabled(true);
        this->horizontalSlider->setEnabled(false);
        button->setEnabled(false);
        std::vector<std::vector<std::string>> groups_of_similars = get_similar(model, this->horizontalSlider->value(),
            [this](size_t count, size_t total) -> bool
            {
                this->progressBar->setValue(int(100*count/total));
                QCoreApplication::processEvents();
                return go_on.load();
            });
        MainWindow * main = (MainWindow*) this->parent();
        main->ShowSimilarGroups(groups_of_similars);

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

void FindSimilarDialog::on_FindSimilar_finished(int)
{
    go_on.store(false);
    this->progressBar->setEnabled(false);
    this->horizontalSlider->setEnabled(true);
    buttonBox->button(buttonBox->Ok)->setEnabled(true);
}
