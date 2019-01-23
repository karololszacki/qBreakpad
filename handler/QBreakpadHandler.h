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
    const wchar_t* applicationNameWChar() const { return m_applicationNameWChar; }
    const char* executablePath() const { return m_executablePath; }
    const wchar_t* executablePathWChar() const { return m_executablePathWChar; }
    const char* applicationVersion() const { return m_applicationVersion; }
    const wchar_t* applicationVersionWChar() const { return m_applicationVersionWChar; }
    const char* applicationBuild() const { return m_applicationBuild; }
    const wchar_t* applicationBuildWChar() const { return m_applicationBuildWChar; }

    void setSessionIdentifier(const QString& sessionIdentifier);
    const char* sessionIdentifier() const { return m_sessionIdentifier; }
    const wchar_t* sessionIdentifierWChar() const { return m_sessionIdentifierWChar; }

    void setJiraConfiguration(
            const QString& jiraHostname,
            const QString& jiraUsername,
            const QString& jiraPassword,
            const QString& jiraProjectKey,
            const QString& jiraTypeId);
    const char* jiraHostname() const { return m_jiraHostname; }
    const wchar_t* jiraHostnameWChar() const { return m_jiraHostnameWChar; }
    const char* jiraUsername() const { return m_jiraUsername; }
    const wchar_t* jiraUsernameWChar() const { return m_jiraUsernameWChar; }
    const char* jiraPassword() const { return m_jiraPassword; }
    const wchar_t* jiraPasswordWChar() const { return m_jiraPasswordWChar; }
    const char* jiraProjectKey() const { return m_jiraProjectKey; }
    const wchar_t* jiraProjectKeyWChar() const { return m_jiraProjectKeyWChar; }
    const char* jiraTypeId() const { return m_jiraTypeId; }
    const wchar_t* jiraTypeIdWChar() const { return m_jiraTypeIdWChar; }

public slots:
    void sendDumps();

private:
    QBreakpadHandlerPrivate* d;
    const char* m_crashReporterChar; // yes! It MUST be const char[]
    const wchar_t* m_crashReporterWChar;

    const char* m_applicationName;
    const wchar_t* m_applicationNameWChar;
    const char* m_executablePath;
    const wchar_t* m_executablePathWChar;
    const char* m_applicationVersion;
    const wchar_t* m_applicationVersionWChar;
    const char* m_applicationBuild;
    const wchar_t* m_applicationBuildWChar;

    const char* m_sessionIdentifier;
    const wchar_t* m_sessionIdentifierWChar;

    const char* m_jiraHostname;
    const wchar_t* m_jiraHostnameWChar;
    const char* m_jiraUsername;
    const wchar_t* m_jiraUsernameWChar;
    const char* m_jiraPassword;
    const wchar_t* m_jiraPasswordWChar;
    const char* m_jiraProjectKey;
    const wchar_t* m_jiraProjectKeyWChar;
    const char* m_jiraTypeId;
    const wchar_t* m_jiraTypeIdWChar;
};
#define QBreakpadInstance Singleton<QBreakpadHandler>::instance()

#endif	// QBREAKPAD_HANDLER_H
