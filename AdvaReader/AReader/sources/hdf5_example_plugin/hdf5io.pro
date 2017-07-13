cache()
! include(../../common.pri){
    error( Couldnt find the common.pri file)
}
! include(../plugins.pri){
    error( Couldnt find the plugins.pri file)
}

QT       -= core gui

TARGET = hdf5io
TEMPLATE = lib
CONFIG += plugin c++11
VERSION=

DEFINES +=

SOURCES += \
    hdf5io.cpp \
    ../../shared/hdf.cpp

HEADERS += \
    hdf5io.h \
    ../../shared/hdf.h

INCLUDEPATH += ../../common ../../shared


OUTDIR = $$(OUTDIR)
isEmpty(OUTDIR){
    CONFIG(debug, debug|release){
        OUTDIR = ../../../../debug
        DEFINES += DEBUG
    }else{
        OUTDIR = ../../../../release
    }
}
DESTDIR = $${OUTDIR}/plugins

INCLUDEPATH += $$PWD/../../libs/hdf5/include

unix {
    QMAKE_POST_LINK = mv $${DESTDIR}/lib$${TARGET}.so $${DESTDIR}/$${TARGET}.so || true
    QMAKE_CXXFLAGS += -fvisibility=hidden
}

macx {
    QMAKE_EXTENSION_SHLIB = so
    LIBS += $$PWD/../../libs/hdf5/osx/libhdf5.a
    LIBS += $$PWD/../../libs/hdf5/osx/libhdf5_cpp.a
    LIBS += $$PWD/../../libs/hdf5/osx/libhdf5_hl.a
    LIBS += $$PWD/../../libs/hdf5/osx/libz.a
    LIBS += $$PWD/../../libs/hdf5/osx/libszip.a
}

unix:!macx {
    QMAKE_EXTENSION_SHLIB = so
    LIBS += $$PWD/../../libs/hdf5/linux/libhdf5_cpp.a
    LIBS += $$PWD/../../libs/hdf5/linux/libhdf5_hl.a
    LIBS += $$PWD/../../libs/hdf5/linux/libhdf5.a
    LIBS += $$PWD/../../libs/hdf5/linux/libz.a
    LIBS += $$PWD/../../libs/hdf5/linux/libszip.a
}

win32 {
    win32-msvc*:contains(QMAKE_TARGET.arch, x86_64):{
        LIBS += $$PWD/../../libs/hdf5/win64/libhdf5.lib
        LIBS += $$PWD/../../libs/hdf5/win64/libhdf5_cpp.lib
        LIBS += $$PWD/../../libs/hdf5/win64/libhdf5_hl.lib
        LIBS += $$PWD/../../libs/hdf5/win64/libzlib.lib
        LIBS += $$PWD/../../libs/hdf5/win64/libszip.lib
    }else{
        LIBS += $$PWD/../../libs/hdf5/win32/libhdf5.lib
        LIBS += $$PWD/../../libs/hdf5/win32/libhdf5_cpp.lib
        LIBS += $$PWD/../../libs/hdf5/win32/libhdf5_hl.lib
        LIBS += $$PWD/../../libs/hdf5/win32/libzlib.lib
        LIBS += $$PWD/../../libs/hdf5/win32/libszip.lib
    }
}


