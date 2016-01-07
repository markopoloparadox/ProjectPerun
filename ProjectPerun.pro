#-------------------------------------------------
#
# Project created by QtCreator 2015-12-05T15:24:26
#
#-------------------------------------------------

QT       += core gui
QT += network
CONFIG += c++14
QMAKE_CXXFLAGS += -std=c++0x -pthread
LIBS += -pthread

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ProjectPerun
TEMPLATE = app


SOURCES += main.cpp\
        loginwindow.cpp \
    mainwindow.cpp \
    addfriend.cpp \
    chatbox.cpp \
    gamelibrary.cpp \
    game_detection.cpp \
    launch_game.cpp \
    funkcije.cpp

HEADERS  += loginwindow.h \
    mainwindow.h \
    addfriend.h \
    chatbox.h \
    funkcije.h \
    launch_game.h \
    game_detection.h \
    gamelibrary.h

FORMS    += loginwindow.ui \
    mainwindow.ui \
    addfriend.ui \
    chatbox.ui \
    gamelibrary.ui

RESOURCES += \
    resources.qrc
