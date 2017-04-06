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

#include <QIcon>
#include <QDebug>
#include <QTimer>
#include <QDir>
#include <QDateTime>
#include <QStandardPaths>

#include <QJsonObject>
#include <QJsonDocument>

#include <QProcess>

#include "ui_CrashReporter.h"

#define RESPATH ":/data/"
#define PRODUCT_NAME "WaterWolf"

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

    //hide until "send report" has been clicked
    m_ui->progressBox->setVisible( false );
    m_ui->progressLabel->setVisible( true );
    m_ui->progressLabel->setText( QString() );
    connect( m_ui->sendButton, SIGNAL( clicked() ), SLOT( onSendButton() ) );

    connect(this, &QDialog::accepted,  [=]() {
        relaunchApplication();
    });
    connect(this, &QDialog::rejected,  [=]() {
        relaunchApplication();
    });

    adjustSize();
    setFixedSize( size() );
}


CrashReporter::~CrashReporter()
{
    delete m_request;
    delete m_reply;
}


void CrashReporter::
relaunchApplication()
{
    if (executablePath() != NULL) {
        QProcess *process=new QProcess(this);
        bool res;
        process->startDetached(executablePath());
        res = process->waitForFinished();
        delete process;
        process = NULL;
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
    QByteArray data = m_reply->readAll();
    m_ui->progressBar->setValue( m_ui->progressBar->maximum() );
    m_ui->button->setText( tr( "Close" ) );

    QString const response = QString::fromUtf8( data );
    qDebug() << "RESPONSE:" << response;

    if ( ( m_reply->error() != QNetworkReply::NoError ) || !response.startsWith( "CrashID=" ) )
    {
        onFail( m_reply->error(), m_reply->errorString() );
    }
    else
    {
        QString crashId = response.split("\n").at(0).split("=").at(1);

        m_ui->progressLabel->setText( tr( "Sent! <b>Many thanks</b>. Please refer to crash <b>%1</b> in bug reports." ).arg(crashId) );
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
    adjustSize();
    setFixedSize( size() );

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

void CrashReporter::setExecutablePath(const QString& executablePath )
{
    char* cepath;
    std::string sepath = executablePath.toStdString();
    cepath = new char[ sepath.size() + 1 ];
    strcpy( cepath, sepath.c_str() );
    m_executablePath = cepath;
}

void CrashReporter::sendToJira()
{
//    QStringList files;

//    QJsonObject ticketObj;

//    QJsonObject fieldsObj;
//    fieldsObj["summary"] = instance->_dialog->getSubject();
//    fieldsObj["description"] = tr("%1\n\n--\n%2\n%3 : %4").arg(instance->_dialog->getNotes()).arg(Utilities::getApplicationDescription(true)).arg(username).arg(session);

//    QJsonObject projectObj;
//    projectObj["key"] = "DIAG";
//    fieldsObj["project"] = projectObj;

//    const int typeIndex = instance->_dialog->getTypeIndex();
//    QString typeID = "10005";
//    if (typeIndex == 1) { // support requested
//        typeID = "10007";
//    } else if (typeIndex == 2) { // feature ideas
//        typeID = "10006";
//    } else { // test logs
//        // nothing to do, current value is what we want
//    }

//    QJsonObject issueTypeObj;
//    issueTypeObj["id"] = typeID;
////    issueTypeObj["name"] = "Story";
//    fieldsObj["issuetype"] = issueTypeObj;

//    ticketObj["fields"] = fieldsObj;

//    QNetworkReply *reply = instance->postRequest(API_ISSUE_PATH, ticketObj);

//    QMetaObject::Connection c1;
//    c1 = connect(reply, &QNetworkReply::finished, [reply, files, instance, c1]() {
//        disconnect(c1);

//        if (reply->error() != QNetworkReply::NoError) {
//            // some error
//            instance->handleReportSubmitted();
//        } else {
//            QByteArray strReply = reply->readAll();
//            QJsonObject responseObj = QJsonDocument::fromJson(strReply).object();

//            QJsonValue value;

//            value = responseObj.value("key");
//            if (!value.isUndefined() && value.isString()) {
//                QString ticketKey = value.toString();

//                if(!files.isEmpty()) {
//                    QMetaObject::Connection c2;
//                    QNetworkReply *reply = instance->postRaw(QString("%1%2/attachments").arg(API_ISSUE_PATH).arg(ticketKey), files);
//                    c2 = connect(reply, &QNetworkReply::finished, [reply, instance, c2]() {
//                        disconnect(c2);

//                        instance->handleReportSubmitted();

//                        if (reply->error() != QNetworkReply::NoError) {
//                            // some error
//                            qDebug() << "attachment error: " << reply->error() << reply->readAll();
//                        }
//                    });
//                } else {
//                    instance->handleReportSubmitted();
//                }
//            } else {
//                instance->handleReportSubmitted();
//            }
//        }
//    });
}

void prepareRequest(QNetworkRequest &request, const QUrl &url)
{
//    request.setUrl(url);

//    QString concatenated = QString("%1:%2").arg(API_USER).arg(API_PASS);
//    QByteArray data = concatenated.toLocal8Bit().toBase64();
//    QString headerData = "Basic " + data;
//    request.setRawHeader("Authorization", headerData.toLocal8Bit());
}

QNetworkReply * CrashReporter::postRequest(const QString &path, const QJsonObject &json, const QVariant &headerValue)
{
//    QUrl url(baseURL());
//    url.setPath(path);

//    QNetworkRequest request;
//    prepareRequest(request, url);

//    request.setHeader(QNetworkRequest::ContentTypeHeader, headerValue);

//    const QString body = QJsonDocument(json).toJson(QJsonDocument::Compact);
//    QByteArray buffer = body.toUtf8();

//    return _qnam->post(request, buffer);
    return NULL;
}

QNetworkReply * CrashReporter::postRaw(const QString &path, QStringList files)
{
//    QUrl url(baseURL());
//    url.setPath(path);

//    QNetworkRequest request;
//    prepareRequest(request, url);
//    request.setRawHeader("X-Atlassian-Token", "nocheck");

//    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
//    multiPart->setBoundary("---------------------jasglfuyqwreltjaslgjlkdaghflsdgh");

//    request.setHeader(QNetworkRequest::ContentTypeHeader, "multipart/form-data; boundary=" + multiPart->boundary());

//    foreach(QString filePath, files) {
//        QFile *file = new QFile(filePath);
//        if(file->exists()) {
//            QFileInfo fileInfo(file->fileName());

//            QHttpPart filePart;
//            filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\""+ fileInfo.fileName() + "\""));

//            file->open(QIODevice::ReadOnly);
//            filePart.setBodyDevice(file);
//            file->setParent(multiPart);

//            multiPart->append(filePart);
//        }
//    }

//    QNetworkReply *reply = _qnam->post(request, multiPart);
//    multiPart->setParent(reply);
//    return reply;
    return NULL;
}
