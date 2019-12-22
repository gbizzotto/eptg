#ifndef FIND_SIMILAR_H
#define FIND_SIMILAR_H

#include <atomic>
#include <QDialog>
#include "ui_find_similar.h"
#include "eptg/project.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class FindSimilar; }
QT_END_NAMESPACE

class MyDialogFindSimilar : public QDialog, public Ui::FindSimilar
{
    Q_OBJECT

private:
    const eptg::Project<QString> & project;
    std::atomic_bool go_on;

public:
    MyDialogFindSimilar(const eptg::Project<QString> & project, QWidget * parent);
private slots:
    void on_buttonBox_clicked(QAbstractButton *button);
    void on_FindSimilar_finished(int result);
};

#endif // FIND_SIMILAR_H
