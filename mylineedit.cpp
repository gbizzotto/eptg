
#include "mylineedit.h"


MyLineEdit::MyLineEdit(QWidget *parent)
    :QLineEdit(parent)
{}

void MyLineEdit::focusInEvent(QFocusEvent * e)
{
    QLineEdit::focusInEvent(e);
    setCursorPosition(text().length());
}
