QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dialog_copy_move.cpp \
    dialog_find_similar.cpp \
    dialog_process.cpp \
    helpers.cpp \
    main.cpp \
    mainwindow.cpp \
    mylineedit.cpp \
    mytablewidget.cpp \
    search.cpp

HEADERS += \
    constants.hpp \
    dialog_copy_move.h \
    dialog_find_similar.h \
    dialog_process.h \
    helpers.hpp \
    mainwindow.h \
    mylineedit.h \
    mytablewidget.h \
    project.hpp \
    search.hpp

FORMS += \
    copy_move_wizard.ui \
    find_similar.ui \
    mainwindow.ui \
    process.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    ../../../Pictures/conte mais.jpg
