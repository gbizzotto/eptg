#ifndef MYTABLEWIDGET_HPP
#define MYTABLEWIDGET_HPP

#include <QTableWidget>
#include <QResizeEvent>

class MyTableWidget : public QTableWidget
{
protected:
    void resizeEvent(QResizeEvent * e);

public:
    MyTableWidget(QWidget * parent);
};

#endif // MYTABLEWIDGET_HPP
