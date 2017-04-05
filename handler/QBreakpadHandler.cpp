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

#ifdef Q_OS_WIN

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
        // we are the fork
        execl( crashReporter,
               crashReporter,
               path,
               applicationName,
               applicationVersion,
               applicationBuild,
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
    m_crash_handler =  new google_breakpad::ExceptionHandler(
                           google_breakpad::MinidumpDescriptor( absPath.toStdString() ),
                           NULL,
                           DumpCallback,
                           this,
                           true,
                           -1 );
    m_crash_handler->set_crash_handler(GetCrashInfo);
    #elif defined Q_OS_MAC
    d->pExptHandler =  new google_breakpad::ExceptionHandler( absPath.toStdString(), NULL, DumpCallback, this, true, NULL);
    #elif defined Q_OS_WIN
    //     d->pExptHandler = new google_breakpad::ExceptionHandler( absPath.toStdString(), 0, DumpCallback, this, true, 0 );
    d->pExptHandler = new google_breakpad::ExceptionHandler( absPath.toStdWString(), 0, DumpCallback, this, true, 0 );
    #endif

    setCrashReporter( handlerPath );
    #ifdef Q_OS_LINUX
    setApplicationData( qApp );
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


    // cache reporter path as char*
    char* creporter;
    std::string sreporter = crashReporterPath.toStdString();
    creporter = new char[ sreporter.size() + 1 ];
    strcpy( creporter, sreporter.c_str() );
    m_crashReporterChar = creporter;

//    qDebug() << "m_crashReporterChar: " << m_crashReporterChar;

    // cache reporter path as wchart_t*
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

    char* cepath;
    std::string sepath = app->applicationFilePath().toStdString();
    cepath = new char[ sepath.size() + 1 ];
    strcpy( cepath, sepath.c_str() );
    m_executablePath = cepath;

    char* cappver;
    std::string sappver = app->applicationVersion().toStdString();
    cappver = new char[ sappver.size() + 1 ];
    strcpy( cappver, sappver.c_str() );
    m_applicationVersion = cappver;

    char* cappbuild;
    std::string sappbuild = appBuildString.toStdString();
    cappbuild = new char[ sappbuild.size() + 1 ];
    strcpy( cappbuild, sappbuild.c_str() );
    m_applicationBuild = cappbuild;
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

