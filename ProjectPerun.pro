#-------------------------------------------------
#
#-------------------------------------------------

QT += core gui
QT += network
QT += multimedia
CONFIG += c++14
QMAKE_CXXFLAGS += -std=c++0x -pthread
LIBS += -pthread

win32-g++:!contains(QMAKE_HOST.arch, x86_64) {
    LIBS += "C:/Program Files/Microsoft SDKs/Windows/v7.1/Lib/user32.lib"
    LIBS += "C:/Program Files/Microsoft SDKs/Windows/v7.1/Lib/Gdi32.lib"
} else {
    LIBS += "C:/Program Files/Microsoft SDKs/Windows/v7.1/Lib/x64/user32.lib"
    LIBS += "C:/Program Files/Microsoft SDKs/Windows/v7.1/Lib/x64/Gdi32.lib"
}
# Project created by QtCreator 2015-12-05T15:24:26
#

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ProjectPerun
TEMPLATE = app
RC_FILE = appicon.rc

SOURCES += main.cpp\
    GameDetection.cpp \
    ChatWindow.cpp \
    GameLibraryWindow.cpp \
    LoginWindow.cpp \
    MainWindow.cpp \
    UsefulFunctions.cpp \
    AddFriendWindow.cpp

HEADERS  += \
    GameDetection.h \
    ChatWindow.h \
    GameLibraryWindow.h \
    LoginWindow.h \
    MainWindow.h \
    UsefulFunctions.h \
    AddFriendWindow.h

FORMS    += \
    ChatWindow.ui \
    GameLibraryWindow.ui \
    LoginWindow.ui \
    MainWindow.ui \
    AddFriendWindow.ui

RESOURCES += \
    resources.qrc
