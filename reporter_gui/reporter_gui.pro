TEMPLATE = app
TARGET = reporter_gui
#Application version
#VERSION = 0.4.0

CONFIG += warn_on thread exceptions rtti stl
QT += gui core widgets network

OBJECTS_DIR = _build/obj
MOC_DIR = _build
win32 {
    DESTDIR = $$OUT_PWD
}

FORMS += \
    CrashReporter.ui

HEADERS += \
    CrashReporter.h \
    CrashReporterGzip.h \
    crashreporterconfig.h

SOURCES += \
    CrashReporter.cpp \
    CrashReporterGzip.cpp \
    main.cpp
