#-------------------------------------------------
#
# Project created by QtCreator 2017-06-03T20:48:58
#
#-------------------------------------------------
QMAKE_MAC_SDK = macosx10.14
QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AnyKey
TEMPLATE = app
ICON = anykey_logo.icns
#RC_FILE = anykey_icon.rc


CONFIG += static
QT += network

SOURCES +=  main.cpp\
            mainwindow.cpp\
            launchagent.cpp

HEADERS  += mainwindow.h\
            launchagent.h

FORMS    += anykey.ui

RESOURCES += \
    AnyKey.qrc
