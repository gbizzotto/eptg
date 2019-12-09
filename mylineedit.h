#ifndef MYLINEEDIT_H
#define MYLINEEDIT_H

#include <QLineEdit>
#include <QFocusEvent>

class MyLineEdit : public QLineEdit
{
protected:
    void focusInEvent(QFocusEvent *e);

public:
    MyLineEdit(QWidget *parent);
};

#endif // MYLINEEDIT_H
