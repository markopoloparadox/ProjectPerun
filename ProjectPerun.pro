#-------------------------------------------------
#
# Project created by QtCreator 2015-12-05T15:24:26
#
#-------------------------------------------------

QT       += core gui
QT += network
CONFIG += c++14

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ProjectPerun
TEMPLATE = app


SOURCES += main.cpp\
        loginwindow.cpp \
    mainwindow.cpp \
    addfriend.cpp \
    chatbox.cpp

HEADERS  += loginwindow.h \
    mainwindow.h \
    addfriend.h \
    chatbox.h

FORMS    += loginwindow.ui \
    mainwindow.ui \
    addfriend.ui \
    chatbox.ui

RESOURCES += \
    resources.qrc
