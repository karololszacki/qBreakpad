#include <QApplication>
#include <QTimer>
#include <QDateTime>
#include <iostream>
#include "CrashReporter.h"
#include "crashreporterconfig.h"

const char* k_usage =
    "Usage:\n"
    "  CrashReporter <dumpFilePath> <applicationName> <applicationVersion> <applicationBuild> <applicationPath?>\n";

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName( QLatin1String( CRASHREPORTER_ORGANIZATION_NAME ) );
    QCoreApplication::setOrganizationDomain( QLatin1String( CRASHREPORTER_ORGANIZATION_DOMAIN ) );
    QCoreApplication::setApplicationName( QLatin1String( CRASHREPORTER_APPLICATION_NAME ) );
    QCoreApplication::setApplicationVersion( QLatin1String( CRASHREPORTER_APPLICATION_VERSION ) );

    QApplication app(argc, argv);

    if ( app.arguments().size() < 5 )
    {
        std::cout << k_usage;
        return 1;
    }

    CrashReporter reporter( QUrl( CRASHREPORTER_SUBMIT_URL ),  app.arguments() );

    QString applicationName = app.arguments().value( 2 );
    QString applicationVersion = app.arguments().value( 3 );
    QString applicationBuild = app.arguments().value( 4 );

    if (app.arguments().size() >= 6) {
        reporter.setExecutablePath(app.arguments().value( 5 ));
    }

    reporter.setWindowTitle( applicationName + " Diagnostics");
    reporter.setText("<html><head/><body><p><span style=\"font-weight:600;\">Sorry!</span> " + applicationName + " stopped. Please tell us about it! We've created an error report for you that can help improve the stability in the future. You can now send this report directly to the " + applicationName + " developers.</p><p>Can you tell us what you were doing when this happened?</p></body></html>");

    reporter.setReportData( "ProductName",  applicationName.toUtf8() );
    reporter.setReportData( "Version", applicationVersion.toUtf8() );
    reporter.setReportData( "BuildID", applicationBuild.toUtf8() );
    reporter.setReportData( "ReleaseChannel", CRASHREPORTER_RELEASE_CHANNEL);

    reporter.show();

    return app.exec();
}
