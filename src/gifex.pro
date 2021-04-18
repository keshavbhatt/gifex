#-------------------------------------------------
#
# Project created by QtCreator 2020-04-29T11:53:19
#
#-------------------------------------------------

QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

TARGET = gifex
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
        controlbutton.cpp \
        error.cpp \
        main.cpp \
        mainwindow.cpp \
        utils.cpp \
        videoprocessor/rangeslider.cpp \
        videoprocessor/screenshot.cpp \
        videoprocessor/seekslider.cpp \
        videoprocessor/videoprocessor.cpp \
        waitingspinnerwidget.cpp

HEADERS += \
        controlbutton.h \
        error.h \
        mainwindow.h \
        utils.h \
        videoprocessor/rangeslider.h \
        videoprocessor/screenshot.h \
        videoprocessor/seekslider.h \
        videoprocessor/videoprocessor.h \
        waitingspinnerwidget.h

FORMS += \
        error.ui \
        mainwindow.ui \
        videoprocessor/console.ui \
        videoprocessor/screenshot.ui \
        videoprocessor/videoprocessor.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icons.qrc \
    qbreeze.qrc

DISTFILES +=
