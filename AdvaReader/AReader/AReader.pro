#-------------------------------------------------
#
# Project created by QtCreator 2017-07-12T15:36:34
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AReader
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    sources/qmpxframe/qmpxframe.cpp \
    sources/qmpxframe/qmpxframepanel.cpp \
    sources/mpxframe.cpp \
    sources/metadata.cpp

HEADERS  += mainwindow.h \
    sources/qmpxframe/qmpxframe.h \
    sources/qmpxframe/qmpxframepanel.h \
    sources/mpxframe.h \
    sources/metadata.h

INCLUDEPATH += ./sources \
               ./sources/hdf5/include \
               ./sources/qmpxframe \
               ./sources/tiff

FORMS    += mainwindow.ui

RESOURCES += \
    sources/qmpxframe/qmpxframepanel.qrc

DEFINES += NO_HDF5
