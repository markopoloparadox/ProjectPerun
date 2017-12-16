#-------------------------------------------------
#
# Project created by QtCreator 2015-12-05T15:24:26
#
#-------------------------------------------------

QT += core gui
QT += network
QT += multimedia
CONFIG += c++14
QMAKE_CXXFLAGS += -std=c++0x -pthread
LIBS += -pthread

win32-g++:!contains(QMAKE_HOST.arch, x86_64) {
    LIBS += "C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Lib/user32.lib"
    LIBS += "C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Lib/Gdi32.lib"
} else {
    LIBS += "C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Lib/x64/user32.lib"
    LIBS += "C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Lib/x64/Gdi32.lib"
}

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ProjectPerun
TEMPLATE = app
RC_FILE = appicon.rc

SOURCES += main.cpp\
        loginwindow.cpp \
    mainwindow.cpp \
    addfriend.cpp \
    chatbox.cpp \
    gamelibrary.cpp \
    game_detection.cpp \
    funkcije.cpp

HEADERS  += loginwindow.h \
    mainwindow.h \
    addfriend.h \
    chatbox.h \
    funkcije.h \
    game_detection.h \
    gamelibrary.h

FORMS    += loginwindow.ui \
    mainwindow.ui \
    addfriend.ui \
    chatbox.ui \
    gamelibrary.ui

RESOURCES += \
    resources.qrc
