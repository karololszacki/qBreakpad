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

#include <QDir>
#include <QProcess>
#include <QCoreApplication>
#include <string>
#include <iostream>

#include "QBreakpadHandler.h"
#include "QBreakpadHttpUploader.h"

#define QBREAKPAD_VERSION  0x000400

#if defined(Q_OS_MAC)
#include "client/mac/handler/exception_handler.h"
#elif defined(Q_OS_LINUX)
#include "client/linux/handler/exception_handler.h"
#elif defined(Q_OS_WIN)
#include "client/windows/handler/exception_handler.h"
#endif

#if defined(Q_OS_WIN)
bool DumpCallback(const wchar_t* dump_dir,
                                    const wchar_t* minidump_id,
                                    void* context,
                                    EXCEPTION_POINTERS* exinfo,
                                    MDRawAssertionInfo* assertion,
                                    bool succeeded)
#elif defined(Q_OS_MAC)
bool DumpCallback(const char *dump_dir,
                                    const char *minidump_id,
                                    void *context, bool succeeded)
#else
bool DumpCallback(const google_breakpad::MinidumpDescriptor& descriptor,
                                    void* context,
                                    bool succeeded)
#endif
{
    // DON'T USE THE HEAP!!!
    // So that indeed means, no QStrings, no qDebug(), no QAnything, seriously!

    if ( !succeeded )
    {
        printf("Could not write crash dump file");
        return false;
    }

#ifdef Q_OS_WIN

    const wchar_t* crashReporter = static_cast<QBreakpadHandler*>(context)->crashReporterWChar();
    if ( /*!s_active ||*/ wcslen( crashReporter ) == 0 )
        return false;

    const wchar_t* applicationName = static_cast<QBreakpadHandler*>(context)->applicationNameWChar();
    if ( wcslen( applicationName ) == 0 )
        return false;
    const wchar_t* applicationVersion = static_cast<QBreakpadHandler*>(context)->applicationVersionWChar();
    if ( wcslen( applicationVersion ) == 0 )
        return false;
    const wchar_t* applicationBuild = static_cast<QBreakpadHandler*>(context)->applicationBuildWChar();
    if ( wcslen( applicationBuild ) == 0 )
        return false;

    const wchar_t* sessionIdentifier = static_cast<QBreakpadHandler*>(context)->sessionIdentifierWChar();
    if ( wcslen( sessionIdentifier ) == 0 )
        return false;

    const wchar_t* executablePath = static_cast<QBreakpadHandler*>(context)->executablePathWChar();
//    if ( wcslen( executablePath ) == 0 )
//        return false;

    const wchar_t* jiraHostname = static_cast<QBreakpadHandler*>(context)->jiraHostnameWChar();
    const wchar_t* jiraUsername = static_cast<QBreakpadHandler*>(context)->jiraUsernameWChar();
    const wchar_t* jiraPassword = static_cast<QBreakpadHandler*>(context)->jiraPasswordWChar();
    const wchar_t* jiraProjectKey = static_cast<QBreakpadHandler*>(context)->jiraProjectKeyWChar();
    const wchar_t* jiraTypeId = static_cast<QBreakpadHandler*>(context)->jiraTypeIdWChar();

    wchar_t command[MAX_PATH * 5 + 6];
    wcscpy( command, crashReporter);
    wcscat( command, L" \"");
    wcscat( command, dump_dir );
    wcscat( command, L"/" );
    wcscat( command, minidump_id );
    wcscat( command, L".dmp\"" );

    wcscat( command, L" \"");
    wcscat( command, applicationName );
    wcscat( command, L"\"" );

    wcscat( command, L" \"");
    wcscat( command, applicationVersion );
    wcscat( command, L"\"" );

    wcscat( command, L" \"");
    wcscat( command, applicationBuild );
    wcscat( command, L"\"" );

    wcscat( command, L" \"");
    wcscat( command, sessionIdentifier );
    wcscat( command, L"\"" );

    if ( wcslen( executablePath ) > 0 ) {
        wcscat( command, L" \"");
        wcscat( command, executablePath );
        wcscat( command, L"\"" );
    }

    if ( wcslen( jiraHostname ) > 0
         && wcslen( jiraUsername ) > 0
         && wcslen( jiraPassword ) > 0
         && wcslen( jiraProjectKey ) > 0
         && wcslen( jiraTypeId ) > 0) {

        wcscat( command, L" \"");
        wcscat( command, jiraHostname );
        wcscat( command, L"\"" );

        wcscat( command, L" \"");
        wcscat( command, jiraUsername );
        wcscat( command, L"\"" );

        wcscat( command, L" \"");
        wcscat( command, jiraPassword );
        wcscat( command, L"\"" );

        wcscat( command, L" \"");
        wcscat( command, jiraProjectKey );
        wcscat( command, L"\"" );

        wcscat( command, L" \"");
        wcscat( command, jiraTypeId );
        wcscat( command, L"\"" );
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory( &si, sizeof( si ) );
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_SHOWNORMAL;
    ZeroMemory( &pi, sizeof(pi) );

    if ( CreateProcess( NULL, command, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ) )
    {
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
        TerminateProcess( GetCurrentProcess(), 1 );
    }

    return succeeded;
#else

    const char* crashReporter = static_cast<QBreakpadHandler*>(context)->crashReporterChar();
    if ( /*!s_active ||*/ strlen( crashReporter ) == 0 )
        return false;

    const char* applicationName = static_cast<QBreakpadHandler*>(context)->applicationName();
    if ( strlen( applicationName ) == 0 )
        return false;
    const char* applicationVersion = static_cast<QBreakpadHandler*>(context)->applicationVersion();
    if ( strlen( applicationVersion ) == 0 )
        return false;
    const char* applicationBuild = static_cast<QBreakpadHandler*>(context)->applicationBuild();
    if ( strlen( applicationBuild ) == 0 )
        return false;

    const char* sessionIdentifier = static_cast<QBreakpadHandler*>(context)->sessionIdentifier();
    if ( strlen( sessionIdentifier ) == 0 )
        return false;

    const char* executablePath = static_cast<QBreakpadHandler*>(context)->executablePath();
//    if ( strlen( executablePath ) == 0 )
//        return false;

    const char* jiraHostname = static_cast<QBreakpadHandler*>(context)->jiraHostname();
    const char* jiraUsername = static_cast<QBreakpadHandler*>(context)->jiraUsername();
    const char* jiraPassword = static_cast<QBreakpadHandler*>(context)->jiraPassword();
    const char* jiraProjectKey = static_cast<QBreakpadHandler*>(context)->jiraProjectKey();
    const char* jiraTypeId = static_cast<QBreakpadHandler*>(context)->jiraTypeId();

#ifdef Q_OS_LINUX
    const char* path = descriptor.path();
#else // Q_OS_MAC
    const char* extension = "dmp";

    char path[4096];
    strcpy(path, dump_dir);
    strcat(path, "/");
    strcat(path, minidump_id);
    strcat(path, ".");
    strcat(path,  extension);
#endif

    printf("Dump file was written to: %s\n", path);

    pid_t pid = fork();
    if ( pid == -1 ) // fork failed
        return false;
    if ( pid == 0 )
    {
        qDebug() << "launching with" << executablePath;
        // we are the fork
        execl( crashReporter,
               crashReporter,
               path,
               applicationName,
               applicationVersion,
               applicationBuild,
               sessionIdentifier,
               executablePath,
               jiraHostname,
               jiraUsername,
               jiraPassword,
               jiraProjectKey,
               jiraTypeId,
               (char*) 0 );

        // execl replaces this process, so no more code will be executed
        // unless it failed. If it failed, then we should return false.
        printf( "Error: Can't launch CrashReporter!\n" );
        return false;
    }
#ifdef Q_OS_LINUX
    // If we're running on Linux, we expect that the CrashReporter component will
    // attach gdb, do its thing and then kill this process, so we hang here for the
    // time being, on purpose.          -- Teo 3/2016
    pause();
#endif

    // we called fork()
    return succeeded;
#endif
}

class QBreakpadHandlerPrivate
{
public:
    google_breakpad::ExceptionHandler* pExptHandler;
    QString dumpPath;
    QUrl uploadUrl;
};

//------------------------------------------------------------------------------
QString QBreakpadHandler::version()
{
    return QString("%1.%2.%3").arg(
        QString::number((QBREAKPAD_VERSION >> 16) & 0xff),
        QString::number((QBREAKPAD_VERSION >> 8) & 0xff),
        QString::number(QBREAKPAD_VERSION & 0xff));
}

QBreakpadHandler::QBreakpadHandler() :
    d(new QBreakpadHandlerPrivate())
{
}

QBreakpadHandler::~QBreakpadHandler()
{
    delete d;
}

void QBreakpadHandler::setSessionIdentifier(const QString& sessionIdentifier)
{
    char* csessionIdentifier;
    std::string ssessionIdentifier = sessionIdentifier.toStdString();
    csessionIdentifier = new char[ ssessionIdentifier.size() + 1 ];
    strcpy( csessionIdentifier, ssessionIdentifier.c_str() );
    m_sessionIdentifier = csessionIdentifier;

    wchar_t* wsessionIdentifier;
    std::wstring wssessionIdentifier = sessionIdentifier.toStdWString();
    wsessionIdentifier = new wchar_t[ wssessionIdentifier.size() + 10 ];
    wcscpy( wsessionIdentifier, wssessionIdentifier.c_str() );
    m_sessionIdentifierWChar = wsessionIdentifier;
}

void QBreakpadHandler::setDumpPathAndHandlerApp(const QString& dumpPath, const QString& handlerPath)
{
    QString absPath = dumpPath;
    if(!QDir::isAbsolutePath(absPath)) {
        absPath = QDir::cleanPath(qApp->applicationDirPath() + "/" + dumpPath);
    }
    Q_ASSERT(QDir::isAbsolutePath(absPath));

    QDir().mkpath(absPath);
    if (!QDir().exists(absPath)) {
        qDebug("Failed to set dump path which not exists: %s", qPrintable(absPath));
        return;
    }

    d->dumpPath = absPath;

    #if defined Q_OS_LINUX
    d->pExptHandler = new google_breakpad::ExceptionHandler(google_breakpad::MinidumpDescriptor(absPath.toStdString()),
                                                            /*FilterCallback*/ 0,
                                                            DumpCallback,
                                                            /*context*/ 0,
                                                            true,
                                                            -1);
    #elif defined Q_OS_MAC
    d->pExptHandler =  new google_breakpad::ExceptionHandler( absPath.toStdString(), NULL, DumpCallback, this, true, NULL);
    #elif defined Q_OS_WIN
    //     d->pExptHandler = new google_breakpad::ExceptionHandler( absPath.toStdString(), 0, DumpCallback, this, true, 0 );
    d->pExptHandler = new google_breakpad::ExceptionHandler( absPath.toStdWString(), 0, DumpCallback, this, true, 0 );
    #endif

    setCrashReporter( handlerPath );
    #ifdef Q_OS_LINUX
    setApplicationData( qApp, applicationBuild() );
    #endif
}

void QBreakpadHandler::setCrashReporter( const QString& crashReporter )
{
    QString crashReporterPath;
    QString localReporter = QString( "%1/%2" ).arg( QCoreApplication::instance()->applicationDirPath() ).arg( crashReporter );
    QString globalReporter = QString( "%1/../libexec/%2" ).arg( QCoreApplication::instance()->applicationDirPath() ).arg( crashReporter );

    if ( QFileInfo( localReporter ).exists() )
        crashReporterPath = localReporter;
    else if ( QFileInfo( globalReporter ).exists() )
        crashReporterPath = globalReporter;
    else {
        qDebug() << "Could not find \"" << crashReporter << "\" in ../libexec or application path";
        crashReporterPath = crashReporter;
    }


    // crash reporter path as char*
    char* creporter;
    std::string sreporter = crashReporterPath.toStdString();
    creporter = new char[ sreporter.size() + 1 ];
    strcpy( creporter, sreporter.c_str() );
    m_crashReporterChar = creporter;

//    qDebug() << "m_crashReporterChar: " << m_crashReporterChar;

    // crash reporter path as wchart_t*
    wchar_t* wreporter;
    std::wstring wsreporter = crashReporterPath.toStdWString();
    wreporter = new wchar_t[ wsreporter.size() + 10 ];
    wcscpy( wreporter, wsreporter.c_str() );
    m_crashReporterWChar = wreporter;
//    std::wcout << "m_crashReporterWChar: " << m_crashReporterWChar;
}

QString QBreakpadHandler::uploadUrl() const
{
    return d->uploadUrl.toString();
}

QStringList QBreakpadHandler::dumpFileList() const
{
    if(!d->dumpPath.isNull() && !d->dumpPath.isEmpty()) {
        QDir dumpDir(d->dumpPath);
        dumpDir.setNameFilters(QStringList()<<"*.dmp");
        return dumpDir.entryList();
    }

    return QStringList();
}

void QBreakpadHandler::setUploadUrl(const QUrl &url)
{
    if(!url.isValid() || url.isEmpty())
        return;

    d->uploadUrl = url;
}

void QBreakpadHandler::setApplicationData( const QCoreApplication* app, const QString& appBuildString )
{
    char* cappname;
    std::string sappname = app->applicationName().toStdString();
    cappname = new char[ sappname.size() + 1 ];
    strcpy( cappname, sappname.c_str() );
    m_applicationName = cappname;

    wchar_t* wappname;
    std::wstring wsappname = app->applicationName().toStdWString();
    wappname = new wchar_t[ wsappname.size() + 10 ];
    wcscpy( wappname, wsappname.c_str() );
    m_applicationNameWChar = wappname;

    char* cepath;
    std::string sepath = app->applicationFilePath().toStdString();
    cepath = new char[ sepath.size() + 1 ];
    strcpy( cepath, sepath.c_str() );
    m_executablePath = cepath;

    wchar_t* wepath;
    std::wstring wsepath = app->applicationFilePath().toStdWString();
    wepath = new wchar_t[ wsepath.size() + 10 ];
    wcscpy( wepath, wsepath.c_str() );
    m_executablePathWChar = wepath;

    char* cappver;
    std::string sappver = app->applicationVersion().toStdString();
    cappver = new char[ sappver.size() + 1 ];
    strcpy( cappver, sappver.c_str() );
    m_applicationVersion = cappver;

    wchar_t* wappver;
    std::wstring wsappver = app->applicationVersion().toStdWString();
    wappver = new wchar_t[ wsappver.size() + 10 ];
    wcscpy( wappver, wsappver.c_str() );
    m_applicationVersionWChar = wappver;

    char* cappbuild;
    std::string sappbuild = appBuildString.toStdString();
    cappbuild = new char[ sappbuild.size() + 1 ];
    strcpy( cappbuild, sappbuild.c_str() );
    m_applicationBuild = cappbuild;

    wchar_t* wappbuild;
    std::wstring wsappbuild = appBuildString.toStdWString();
    wappbuild = new wchar_t[ wsappbuild.size() + 10 ];
    wcscpy( wappbuild, wsappbuild.c_str() );
    m_applicationBuildWChar = wappbuild;
}

void QBreakpadHandler::setJiraConfiguration(
        const QString& jiraHostname,
        const QString& jiraUsername,
        const QString& jiraPassword,
        const QString& jiraProjectKey,
        const QString& jiraTypeId)
{
    char* cjiraHostname;
    std::string sjiraHostname = jiraHostname.toStdString();
    cjiraHostname = new char[ sjiraHostname.size() + 1 ];
    strcpy( cjiraHostname, sjiraHostname.c_str() );
    m_jiraHostname = cjiraHostname;

    wchar_t* wjiraHostname;
    std::wstring wsjiraHostname = jiraHostname.toStdWString();
    wjiraHostname = new wchar_t[ wsjiraHostname.size() + 10 ];
    wcscpy( wjiraHostname, wsjiraHostname.c_str() );
    m_jiraHostnameWChar = wjiraHostname;

    char* cjiraUsername;
    std::string sjiraUsername = jiraUsername.toStdString();
    cjiraUsername = new char[ sjiraUsername.size() + 1 ];
    strcpy( cjiraUsername, sjiraUsername.c_str() );
    m_jiraUsername = cjiraUsername;

    wchar_t* wjiraUsername;
    std::wstring wsjiraUsername = jiraUsername.toStdWString();
    wjiraUsername = new wchar_t[ wsjiraUsername.size() + 10 ];
    wcscpy( wjiraUsername, wsjiraUsername.c_str() );
    m_jiraUsernameWChar = wjiraUsername;

    char* cjiraPassword;
    std::string sjiraPassword = jiraPassword.toStdString();
    cjiraPassword = new char[ sjiraPassword.size() + 1 ];
    strcpy( cjiraPassword, sjiraPassword.c_str() );
    m_jiraPassword = cjiraPassword;

    wchar_t* wjiraPassword;
    std::wstring wsjiraPassword = jiraPassword.toStdWString();
    wjiraPassword = new wchar_t[ wsjiraPassword.size() + 10 ];
    wcscpy( wjiraPassword, wsjiraPassword.c_str() );
    m_jiraPasswordWChar = wjiraPassword;

    char* cjiraProjectKey;
    std::string sjiraProjectKey = jiraProjectKey.toStdString();
    cjiraProjectKey = new char[ sjiraProjectKey.size() + 1 ];
    strcpy( cjiraProjectKey, sjiraProjectKey.c_str() );
    m_jiraProjectKey = cjiraProjectKey;

    wchar_t* wjiraProjectKey;
    std::wstring wsjiraProjectKey = jiraProjectKey.toStdWString();
    wjiraProjectKey = new wchar_t[ wsjiraProjectKey.size() + 10 ];
    wcscpy( wjiraProjectKey, wsjiraProjectKey.c_str() );
    m_jiraProjectKeyWChar = wjiraProjectKey;

    char* cjiraTypeId;
    std::string sjiraTypeId = jiraTypeId.toStdString();
    cjiraTypeId = new char[ sjiraTypeId.size() + 1 ];
    strcpy( cjiraTypeId, sjiraTypeId.c_str() );
    m_jiraTypeId = cjiraTypeId;

    wchar_t* wjiraTypeId;
    std::wstring wsjiraTypeId = jiraTypeId.toStdWString();
    wjiraTypeId = new wchar_t[ wsjiraTypeId.size() + 10 ];
    wcscpy( wjiraTypeId, wsjiraTypeId.c_str() );
    m_jiraTypeIdWChar = wjiraTypeId;
}

void QBreakpadHandler::sendDumps()
{
    if(!d->dumpPath.isNull() && !d->dumpPath.isEmpty()) {
        QDir dumpDir(d->dumpPath);
        dumpDir.setNameFilters(QStringList()<<"*.dmp");
        QStringList dumpFiles = dumpDir.entryList();

        foreach(QString itDmpFileName, dumpFiles) {
            qDebug() << "Sending " << QString(itDmpFileName);
            QBreakpadHttpUploader *sender = new QBreakpadHttpUploader(d->uploadUrl);
            sender->uploadDump(d->dumpPath + "/" + itDmpFileName);
        }
    }
}

