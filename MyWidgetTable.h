#ifndef MYTABLEWIDGET_HPP
#define MYTABLEWIDGET_HPP

#include <QTableWidget>
#include <QResizeEvent>

class MyWidgetTable : public QTableWidget
{
protected:
    void resizeEvent(QResizeEvent * e);

public:
    MyWidgetTable(QWidget * parent);
};

#endif // MYTABLEWIDGET_HPP
