TEMPLATE = lib
TARGET = qBreakpad
#Application version
VERSION = 0.4.0

include($$PWD/../conf.pri)

CONFIG += warn_on thread exceptions rtti stl
QT -= gui
QT += core network

OBJECTS_DIR = _build/obj
MOC_DIR = _build
win32 {
    DESTDIR = $$OUT_PWD
}

### qBreakpad
HEADERS += \
    $$PWD/singletone/call_once.h \
    $$PWD/singletone/singleton.h
include($$PWD/../qBreakpad.pri)

## google-breakpad
include($$PWD/../third_party/breakpad.pri)


SOURCES += \
    $$PWD/QBreakpadHandler.cpp \
    $$PWD/QBreakpadHttpUploader.cpp
