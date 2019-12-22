#ifndef MYLINEEDIT_H
#define MYLINEEDIT_H

#include <QLineEdit>
#include <QFocusEvent>

class MyWidgetLineEdit : public QLineEdit
{
protected:
    void focusInEvent(QFocusEvent *e);

public:
    MyWidgetLineEdit(QWidget *parent);
};

#endif // MYLINEEDIT_H
