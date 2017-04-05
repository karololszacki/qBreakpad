/*
 *  Copyright (C) 2009 Aleksey Palazhchenko
 *  Copyright (C) 2014 Sergey Shambir
 *  Copyright (C) 2016 Alexander Makarov
 *
 * This file is a part of Breakpad-qt library.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 */

#ifndef QBREAKPAD_HANDLER_H
#define QBREAKPAD_HANDLER_H

#include <QCoreApplication>
#include <QString>
#include <QUrl>
#include "singletone/singleton.h"

namespace google_breakpad {
    class ExceptionHandler;
    class MinidumpDescriptor;
}

class QBreakpadHandlerPrivate;

class QBreakpadHandler: public QObject
{
    Q_OBJECT
public:
    static QString version();

    QBreakpadHandler();
    ~QBreakpadHandler();

    QString uploadUrl() const;
    QString dumpPath() const;
    QStringList dumpFileList() const;

    void setDumpPathAndHandlerApp(const QString& dumpPath, const QString& handlerPath);
    void setCrashReporter(const QString& crashReporter);
    const char* crashReporterChar() const { return m_crashReporterChar; }
    const wchar_t* crashReporterWChar() const { return m_crashReporterWChar; }

    void setUploadUrl(const QUrl& url);

    void setApplicationData( const QCoreApplication* app, const QString& appBuildString );
    const char* applicationName() const { return m_applicationName; }
    const char* executablePath() const { return m_executablePath; }
    const char* applicationVersion() const { return m_applicationVersion; }
    const char* applicationBuild() const { return m_applicationBuild; }

public slots:
    void sendDumps();

private:
    QBreakpadHandlerPrivate* d;
    const char* m_crashReporterChar; // yes! It MUST be const char[]
    const wchar_t* m_crashReporterWChar;

    const char* m_applicationName;
    const char* m_executablePath;
    const char* m_applicationVersion;
    const char* m_applicationBuild;
};
#define QBreakpadInstance Singleton<QBreakpadHandler>::instance()

#endif	// QBREAKPAD_HANDLER_H
