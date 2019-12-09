#include "mytablewidget.h"

#include <QScrollBar>

void MyTableWidget::resizeEvent(QResizeEvent *)
{
    this->setColumnWidth(0, (this->width() - this->verticalScrollBar()->width()) / 3 * 2);
    this->setColumnWidth(1,  this->width() - this->verticalScrollBar()->width() - this->columnWidth(0) -2); //-2 for grid lines
}

MyTableWidget::MyTableWidget(QWidget * parent)
    :QTableWidget(parent)
{}
