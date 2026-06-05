#include "namedialog.h"
#include "ui_namedialog.h"

NameDialog::NameDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NameDialog)
{
    ui->setupUi(this);
    setWindowTitle("Put a name to a face");
}

NameDialog::~NameDialog()
{
    delete ui;
}

QString NameDialog::name() const
{
    return ui->nameEdit->text();
}
void NameDialog::setname(QString s)
{
    ui->nameEdit->setText(s);
    ui->nameEdit->setSelection(0, ui->nameEdit->text().length());
    ui->nameEdit->setFocus();
}
