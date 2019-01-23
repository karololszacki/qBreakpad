TEMPLATE = lib
TARGET = qBreakpad
#Application version
#VERSION = 0.4.0

CONFIG += warn_on thread exceptions rtti stl
QT -= gui
QT += core network

message()
message("----------------- qBreakpad -----------------")

include ($$PWD/../qtcompilercheck.pri)

### qBreakpad config
include($$PWD/../config.pri)

## google-breakpad
include($$PWD/../third_party/breakpad.pri)

HEADERS += \
    $$PWD/singletone/call_once.h \
    $$PWD/singletone/singleton.h \
    $$PWD/QBreakpadHandler.h

SOURCES += \
    $$PWD/QBreakpadHandler.cpp
