#ifndef FIND_SIMILAR_H
#define FIND_SIMILAR_H

#include <atomic>
#include <QDialog>
#include "ui_find_similar.h"
#include "model.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class FindSimilar; }
QT_END_NAMESPACE

class FindSimilarDialog : public QDialog, public Ui::FindSimilar {
    Q_OBJECT

private:
    const eptg::Model & model;
    std::atomic_bool go_on;

public:
    FindSimilarDialog(const eptg::Model & model, QWidget * parent = 0);
private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_FindSimilar_finished(int result);
};

#endif // FIND_SIMILAR_H
