#include "MyWidgetTable.h"

#include <QScrollBar>

void MyWidgetTable::resizeEvent(QResizeEvent *)
{
    this->setColumnWidth(0, (this->width() - this->verticalScrollBar()->width()) / 3 * 2);
    this->setColumnWidth(1,  this->width() - this->verticalScrollBar()->width() - this->columnWidth(0) -2); //-2 for grid lines
}

MyWidgetTable::MyWidgetTable(QWidget * parent)
    :QTableWidget(parent)
{}
