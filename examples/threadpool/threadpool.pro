#-------------------------------------------------
#
# Project created by QtCreator 2017-05-06T10:26:52
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = threadpool
TEMPLATE = app


SOURCES += main.cpp\
    MainWindow.cpp \
    Worker.cpp \
    $$PWD/../../Profiler.cpp

HEADERS  += MainWindow.h \
    Worker.h \
    $$PWD/../../Profiler.h

FORMS    += MainWindow.ui

DEFINES += USE_PROFILER
