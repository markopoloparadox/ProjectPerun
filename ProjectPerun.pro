#-------------------------------------------------
#
# Project created by QtCreator 2015-12-05T15:24:26
#
#-------------------------------------------------

QT       += core gui
QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ProjectPerun
TEMPLATE = app


SOURCES += main.cpp\
        loginwindow.cpp \
    mainwindow.cpp \
    addfriend.cpp

HEADERS  += loginwindow.h \
    mainwindow.h \
    addfriend.h

FORMS    += loginwindow.ui \
    mainwindow.ui \
    addfriend.ui

RESOURCES += \
    resources.qrc
