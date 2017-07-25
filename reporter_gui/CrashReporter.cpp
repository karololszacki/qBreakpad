/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2016,      Teo Mrnjavac <teo@kde.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "CrashReporter.h"

#ifdef Q_OS_LINUX
#include "linux-backtrace-generator/backtracegenerator.h"
#include "linux-backtrace-generator/crashedapplication.h"
#include "CrashReporterGzip.h"
#include <csignal> // POSIX kill
#endif

#ifdef Q_OS_WIN
#include <windows.h>
#include <rpc.h>
#endif

#include <QIcon>
#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>
#include <QHttpMultiPart>

#include <QJsonObject>
#include <QJsonDocument>

#include <QProcess>

#include "ui_CrashReporter.h"

#define RESPATH ":/data/"
#define PRODUCT_NAME "WaterWolf"

#define BASE_PROTOCOL   "https"
#define BASE_PORT       443
#define API_ISSUE_PATH  "/rest/api/2/issue/"

CrashReporter::CrashReporter( const QUrl& url, const QStringList& args )
: m_ui( 0 )
, m_reply( 0 )
, m_url( url )
{
    m_ui = new Ui::CrashReporter();
    m_ui->setupUi( this );
    m_ui->progressBar->setRange( 0, 100 );
    m_ui->progressBar->setValue( 0 );
    m_ui->progressLabel->setPalette( Qt::gray );

    #ifdef Q_OS_MAC
    QFont f = m_ui->bottomLabel->font();
    f.setPointSize( 10 );
    m_ui->bottomLabel->setFont( f );
    f.setPointSize( 11 );
    m_ui->progressLabel->setFont( f );
    m_ui->progressLabel->setIndent( 3 );
    #else
    m_ui->vboxLayout->setSpacing( 16 );
    m_ui->hboxLayout1->setSpacing( 16 );
    m_ui->progressBar->setTextVisible( false );
    m_ui->progressLabel->setIndent( 1 );
    m_ui->bottomLabel->setDisabled( true );
    m_ui->bottomLabel->setIndent( 1 );
    #endif //Q_OS_MAC

    m_request = new QNetworkRequest( m_url );

    m_minidump_file_path = args.value( 1 );

    m_ui->relaunchCheckbox->setText("Relaunch " + args.value(2));

    //hide until "send report" has been clicked
    m_ui->progressBox->setVisible( false );
    m_ui->progressLabel->setVisible( true );
    m_ui->progressLabel->setText( QString() );
    connect( m_ui->sendButton, SIGNAL( clicked() ), SLOT( onSendButton() ) );

    relaunchEnabled = true;
    m_ui->relaunchCheckbox->setChecked(relaunchEnabled);
    connect( m_ui->relaunchCheckbox, &QCheckBox::toggled, [=](bool checked) {
       relaunchEnabled = checked;
    });

    connect(this, &QDialog::accepted,  [=]() {
        relaunchApplication();
    });
    connect(this, &QDialog::rejected,  [=]() {
        relaunchApplication();
    });

    adjustSize();
    setFixedSize( size() );

    m_ui->bottomLabel->clear();
}


CrashReporter::~CrashReporter()
{
    delete m_request;
    delete m_reply;
}


void CrashReporter::
relaunchApplication()
{
    if (relaunchEnabled && executablePath() != NULL) {
        QFileInfo fi = QFileInfo(executablePath());
        QString fullPath = QDir::toNativeSeparators(fi.absoluteFilePath());

#ifdef Q_OS_WIN
        const wchar_t* fullPathWChar;

        wchar_t* wfullPath;
        std::wstring wsfullPath = fullPath.toStdWString();
        wfullPath = new wchar_t[ wsfullPath.size() + 10 ];
        wcscpy( wfullPath, wsfullPath.c_str() );
        fullPathWChar = wfullPath;

        if ( wcslen( fullPathWChar ) == 0 )
            return;

        wchar_t command[MAX_PATH * 5 + 6];
        wcscpy( command, fullPathWChar);

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
//            TerminateProcess( GetCurrentProcess(), 1 );
        }
#else

        m_ui->commentTextEdit->setPlainText(m_ui->commentTextEdit->toPlainText() + QString("\nlaunching %1").arg(QDir::toNativeSeparators(fi.absoluteFilePath())));
        QProcess *process = new QProcess(this);
        bool res;
        if (process->startDetached(fullPath)) {
            // succeeded
        } else {
        }

        res = process->waitForFinished();
        delete process;
        process = NULL;
#endif
    }
}

void
CrashReporter::setLogo( const QPixmap& logo )
{
    m_ui->logoLabel->setPixmap( logo.scaled( QSize( 55, 55 ), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
    setWindowIcon( logo );
}

void
CrashReporter::setText( const QString& text )
{
    m_ui->topLabel->setText(text);
}

void
CrashReporter::setBottomText( const QString& text )
{
    m_ui->bottomLabel->setText(text);
    m_ui->bottomLabel->setVisible(!text.isEmpty());
}

static QByteArray
contents( const QString& path )
{
    QFile f( path );
    f.open( QFile::ReadOnly );
    return f.readAll();
}


void
CrashReporter::send()
{
    if (!jiraHostname.isEmpty()) {
        sendToJira();
        return;
    }

    // TODO: check if dump file actually exists ...

    // add minidump file
    setReportData( "upload_file_minidump",
        contents( m_minidump_file_path ),
        "application/octet-stream",
        QFileInfo( m_minidump_file_path ).fileName().toUtf8() );


    QByteArray body;
    foreach (const QByteArray& name, m_formContents.keys() )
    {
        body += "--thkboundary\r\n";

        body += "Content-Disposition: form-data; name=\"" + name + "\"";

        if ( !m_formFileNames.value( name ).isEmpty() && !m_formContentTypes.value( name ).isEmpty() )
        {
            body += "; filename=\"" + m_formFileNames.value( name ) + "\"\r\n";
            body += "Content-Type: " + m_formContentTypes.value( name ) + "\r\n";
        }
        else
        {
            body += "\r\n";
        }

        body += "\r\n";

        body += m_formContents.value( name ) + "\r\n";
    }

    body += "--thkboundary\r\n";


    QNetworkAccessManager* nam = new QNetworkAccessManager( this );
    m_request->setHeader( QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=thkboundary" );
    m_reply = nam->post( *m_request, body );

    #if QT_VERSION < QT_VERSION_CHECK( 5, 0, 0 )
    connect( m_reply, SIGNAL( finished() ), SLOT( onDone() ), Qt::QueuedConnection );
    connect( m_reply, SIGNAL( uploadProgress( qint64, qint64 ) ), SLOT( onProgress( qint64, qint64 ) ) );
    #else
    connect( m_reply, &QNetworkReply::finished, this, &CrashReporter::onDone, Qt::QueuedConnection );
    connect( m_reply, &QNetworkReply::uploadProgress, this, &CrashReporter::onProgress );
    #endif
}


void
CrashReporter::onProgress( qint64 done, qint64 total )
{
    if ( total )
    {
        QString const msg = tr( "Uploaded %L1 of %L2 KB." ).arg( done / 1024 ).arg( total / 1024 );

        m_ui->progressBar->setMaximum( total );
        m_ui->progressBar->setValue( done );
        m_ui->progressLabel->setText( msg );
    }
}

void
CrashReporter::onDone()
{
    handleOnDone(m_reply->readAll());
}

void
CrashReporter::handleOnDone(QByteArray replyBytes, const QString& ticketKey)
{
    m_ui->progressBar->setValue( m_ui->progressBar->maximum() );
    m_ui->button->setText( tr( "Close" ) );

    QString const response = QString::fromUtf8( replyBytes );
    qDebug() << "RESPONSE:" << response;

    if ( ( m_reply->error() != QNetworkReply::NoError )
         || (jiraHostname.isEmpty() && !response.startsWith( "CrashID=" )) )
    {
        onFail( m_reply->error(), m_reply->errorString() );
    }
    else
    {
        QFile::remove(m_minidump_file_path);
        if (response.startsWith( "CrashID=" )) {
            QString crashId = response.split("\n").at(0).split("=").at(1);

            m_ui->progressLabel->setText( tr( "Sent! <b>Many thanks</b>. Please refer to crash <b>%1</b> in bug reports." ).arg(crashId) );
        } else {
            if (ticketKey.isEmpty()) {
                m_ui->progressLabel->setText( tr( "Sent! <b>Many thanks</b>." ));
            } else {
                m_ui->progressLabel->setText( tr( "Sent! <b>Many thanks</b>. Please refer to ticket <b>%1</b>." ).arg(ticketKey) );
            }
        }
    }
}

void
CrashReporter::onFail( int error, const QString& errorString )
{
    m_ui->button->setText( tr( "Close" ) );
    m_ui->progressLabel->setText( tr( "Failed to send crash info." ) );
    qDebug() << "Error:" << error << errorString;
}


void
CrashReporter::onSendButton()
{
    m_ui->progressBox->setVisible( true );
    m_ui->sendBox->setVisible( false );

    m_ui->commentTextEdit->setEnabled( false );

    setReportData( "Comments", m_ui->commentTextEdit->toPlainText().toUtf8() );
//    adjustSize();
//    setFixedSize( size() );

    QTimer::singleShot( 0, this, SLOT( send() ) );
}

void
CrashReporter::setReportData(const QByteArray& name, const QByteArray& content)
{
    m_formContents.insert( name, content );
}

void
CrashReporter::setReportData(const QByteArray& name, const QByteArray& content, const QByteArray& contentType, const QByteArray& fileName)
{
    setReportData( name, content );

    if( !contentType.isEmpty() && !fileName.isEmpty() )
    {
        m_formContentTypes.insert( name, contentType );
        m_formFileNames.insert( name, fileName );
    }
}

void CrashReporter::setApplicationData( const QCoreApplication* app )
{
    char* cappname;
    std::string sappname = app->applicationName().toStdString();
    cappname = new char[ sappname.size() + 1 ];
    strcpy( cappname, sappname.c_str() );
    m_applicationName = cappname;

    char* cappver;
    std::string sappver = app->applicationVersion().toStdString();
    cappver = new char[ sappver.size() + 1 ];
    strcpy( cappver, sappver.c_str() );
    m_applicationVersion = cappver;
}

void CrashReporter::setExecutablePath(const QString& path )
{
    char* cepath;
    std::string sepath = path.toStdString();
    cepath = new char[ sepath.size() + 1 ];
    strcpy( cepath, sepath.c_str() );
    m_executablePath = cepath;
}

void CrashReporter::setJiraConfiguration(
        const QString& jiraHostname,
        const QString& jiraUsername,
        const QString& jiraPassword,
        const QString& jiraProjectKey,
        const QString& jiraTypeId)
{
    this->jiraHostname = jiraHostname;
    this->jiraUsername = jiraUsername;
    this->jiraPassword = jiraPassword;
    this->jiraProjectKey = jiraProjectKey;
    this->jiraTypeId = jiraTypeId;
}

void CrashReporter::sendToJira()
{
    QStringList files;
    files.append(m_minidump_file_path);

    QJsonObject ticketObj;

    QJsonObject fieldsObj;
    fieldsObj["summary"] = QString(m_formContents.value(REPORT_KEY_PRODUCT_NAME)) + " Crash Report";
    fieldsObj["description"] = m_ui->commentTextEdit->toPlainText() + "\n\n" +
            QString("App: " + m_formContents.value(REPORT_KEY_PRODUCT_NAME)) + " " +
            QString(m_formContents.value(REPORT_KEY_VERSION)) + " " +
            QString(m_formContents.value(REPORT_KEY_BUILD_ID)) + "\n" +
            QString("Session: " + m_formContents.value(REPORT_KEY_SESSION_ID)) + "\n" +
            "Environment: " + QSysInfo::prettyProductName() + " " +
            QSysInfo::currentCpuArchitecture();

    QJsonObject projectObj;
    projectObj["key"] = jiraProjectKey;
    fieldsObj["project"] = projectObj;

    QJsonObject issueTypeObj;
    issueTypeObj["id"] = jiraTypeId;
    fieldsObj["issuetype"] = issueTypeObj;

    ticketObj["fields"] = fieldsObj;

    m_reply = postRequest(API_ISSUE_PATH, ticketObj);

    connect(m_reply, &QNetworkReply::uploadProgress, this, &CrashReporter::onProgress);

    QMetaObject::Connection c;
    c = connect(m_reply, &QNetworkReply::finished, [files, this, c]() {
        disconnect(c);

        QByteArray strReply = m_reply->readAll();
        if (m_reply->error() != QNetworkReply::NoError) {
            // some error
            handleOnDone(strReply);
            m_ui->commentTextEdit->setPlainText(m_ui->commentTextEdit->toPlainText() + "\nsome error? " + m_reply->readAll());
        } else {
            QJsonObject responseObj = QJsonDocument::fromJson(strReply).object();

            QJsonValue value;

            value = responseObj.value("key");
            if (!value.isUndefined() && value.isString()) {
                QString ticketKey = value.toString();

                if(!files.isEmpty()) {
                    m_reply = postRaw(QString("%1%2/attachments").arg(API_ISSUE_PATH).arg(ticketKey), files);
                    connect(m_reply, &QNetworkReply::uploadProgress, this, &CrashReporter::onProgress);
                    connect(m_reply, &QNetworkReply::finished, [strReply, this, ticketKey]() {
                        handleOnDone(strReply, ticketKey);

                        if (m_reply->error() != QNetworkReply::NoError) {
                            // some error
                            m_ui->commentTextEdit->setPlainText(m_ui->commentTextEdit->toPlainText() + "\nattachment error? " + strReply);
                        }
                    });
                } else {
                    handleOnDone(strReply, ticketKey);
                }
            } else {
                handleOnDone(strReply);
            }
        }
    });
}

void prepareRequest(QNetworkRequest &request, const QUrl &url, QString username, QString password)
{
    request.setUrl(url);

    QString concatenated = QString("%1:%2").arg(username).arg(password);
    QByteArray data = concatenated.toLocal8Bit().toBase64();
    QString headerData = "Basic " + data;
    request.setRawHeader("Authorization", headerData.toLocal8Bit());
}

QUrl baseURL(QString apiHostname)
{
    return QUrl(QString("%1://%2:%3").arg(BASE_PROTOCOL).arg(apiHostname).arg(BASE_PORT));
}

QNetworkReply * CrashReporter::postRequest(const QString &path, const QJsonObject &json, const QVariant &headerValue)
{
    QUrl url(baseURL(jiraHostname));
    url.setPath(path);

    QNetworkRequest request;
    prepareRequest(request, url, jiraUsername, jiraPassword);

    request.setHeader(QNetworkRequest::ContentTypeHeader, headerValue);

    const QString body = QJsonDocument(json).toJson(QJsonDocument::Compact);
    QByteArray buffer = body.toUtf8();

    QNetworkAccessManager* nam = new QNetworkAccessManager( this );
    return nam->post(request, buffer);
}

QNetworkReply * CrashReporter::postRaw(const QString &path, QStringList files)
{
    QUrl url(baseURL(jiraHostname));
    url.setPath(path);

    QNetworkRequest request;
    prepareRequest(request, url, jiraUsername, jiraPassword);
    request.setRawHeader("X-Atlassian-Token", "nocheck");

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    multiPart->setBoundary("---------------------jasglfuyqwreltjaslgjlkdaghflsdgh");

    request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=" + multiPart->boundary());

    foreach(QString filePath, files) {
        QFile *file = new QFile(filePath);
        if(file->exists()) {
            QFileInfo fileInfo(file->fileName());

            QHttpPart filePart;
            filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\""+ fileInfo.fileName() + "\""));

            file->open(QIODevice::ReadOnly);
            filePart.setBodyDevice(file);
            file->setParent(multiPart);

            multiPart->append(filePart);
        }
    }

    QNetworkAccessManager* nam = new QNetworkAccessManager( this );
    QNetworkReply *reply = nam->post(request, multiPart);
    multiPart->setParent(reply);
    return reply;
}
