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

#ifndef CRASHREPORTER_H
#define CRASHREPORTER_H

#include <QDialog>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QByteArray>
#include <QCoreApplication>

namespace Ui
{
    class CrashReporter;
}

#ifdef Q_OS_LINUX
class BacktraceGenerator;
#endif

#define REPORT_KEY_PRODUCT_NAME     "ProductName"
#define REPORT_KEY_VERSION          "Version"
#define REPORT_KEY_BUILD_ID         "BuildID"
#define REPORT_KEY_RELEASE_CHANNEL  "ReleaseChannel"
#define REPORT_KEY_SESSION_ID       "SessionId"

class CrashReporter : public QDialog
{
    Q_OBJECT

public:
    CrashReporter( const QUrl& url, const QStringList& argv );
    virtual ~CrashReporter( );

    void setLogo(const QPixmap& logo);
    void setText(const QString& text);
    void setBottomText(const QString& text);

    void setReportData(const QByteArray& name, const QByteArray& content);
    void setReportData(const QByteArray& name, const QByteArray& content, const QByteArray& contentType, const QByteArray& fileName);

    void setApplicationData( const QCoreApplication* app );
    void setExecutablePath(const QString& executablePath );
    const char* applicationName() const { return m_applicationName; }
    const char* executablePath() const { return m_executablePath; }
    const char* applicationVersion() const { return m_applicationVersion; }

    void setJiraConfiguration(
            const QString& jiraHostname,
            const QString& jiraUsername,
            const QString& jiraPassword,
            const QString& jiraProjectKey,
            const QString& jiraTypeId);

protected:

private:
    Ui::CrashReporter* m_ui;
#ifdef Q_OS_LINUX
    BacktraceGenerator* m_btg;
#endif

    void relaunchApplication();
    void handleOnDone(QByteArray replyBytes, const QString& ticketKey = "");

    QNetworkReply * postRequest(const QString &path, const QJsonObject &json, const QVariant &headerValue="application/json");
    QNetworkReply * postRaw(const QString &path, QStringList files);
    void sendToJira();
    QString jiraHostname;
    QString jiraUsername;
    QString jiraPassword;
    QString jiraProjectKey;
    QString jiraTypeId;

    bool relaunchEnabled;

    QString m_minidump_file_path;
    QNetworkRequest* m_request;
    QNetworkReply* m_reply;
    QUrl m_url;

    const char* m_applicationName;
    const char* m_executablePath;
    const char* m_applicationVersion;

    QMap < QByteArray, QByteArray > m_formContents;
    QMap < QByteArray, QByteArray > m_formContentTypes;
    QMap < QByteArray, QByteArray > m_formFileNames;


public slots:
    void send();

private slots:
    void onDone();
    void onProgress( qint64 done, qint64 total );
    void onFail( int error, const QString& errorString );
    void onSendButton();
};

#endif // CRASHREPORTER_H
