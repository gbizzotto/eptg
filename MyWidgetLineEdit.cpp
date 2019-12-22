
#include "MyWidgetLineEdit.h"


MyWidgetLineEdit::MyWidgetLineEdit(QWidget *parent)
    :QLineEdit(parent)
{}

void MyWidgetLineEdit::focusInEvent(QFocusEvent * e)
{
    QLineEdit::focusInEvent(e);
    setCursorPosition(text().length());
}
