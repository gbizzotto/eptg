QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15

CONFIG += c++17
linux {
    LIBS += -lstdc++fs
}

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
    MyDialogFindSimilar.cpp \
    MyDialogProcess.cpp \
    MyWidgetLineEdit.cpp \
    MyWidgetTable.cpp \
    MyWizardCopyMove.cpp \
    eptg/helpers.cpp \
    eptg/in.cpp \
    eptg/json.cpp \
    eptg/path.cpp \
    eptg/string.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    MyDialogFindSimilar.h \
    MyDialogProcess.h \
    MyWidgetLineEdit.h \
    MyWidgetTable.h \
    MyWizardCopyMove.h \
    eptg/constants.hpp \
    eptg/copy_move_data.hpp \
    eptg/fs.hpp \
    eptg/helpers.hpp \
    eptg/in.hpp \
    eptg/json.hpp \
    eptg/path.hpp \
    eptg/project.hpp \
    eptg/search.hpp \
    eptg/string.hpp \
    mainwindow.h \

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
