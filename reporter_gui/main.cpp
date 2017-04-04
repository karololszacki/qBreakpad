#include <QApplication>
#include <QTimer>
#include <QDateTime>
#include <iostream>
#include "CrashReporter.h"
#include "crashreporterconfig.h"

const char* k_usage =
    "Usage:\n"
    "  CrashReporter <dumpFilePath>\n";

int main(int argc, char* argv[])
{
    QCoreApplication::setOrganizationName( QLatin1String( CRASHREPORTER_ORGANIZATION_NAME ) );
    QCoreApplication::setOrganizationDomain( QLatin1String( CRASHREPORTER_ORGANIZATION_DOMAIN ) );
    QCoreApplication::setApplicationName( QLatin1String( CRASHREPORTER_APPLICATION_NAME ) );
    QCoreApplication::setApplicationVersion( QLatin1String( CRASHREPORTER_APPLICATION_VERSION ) );

    QApplication app(argc, argv);

//    if ( app.arguments().size() != 2 )
//    {
//        std::cout << k_usage;
//        return 1;
//    }

    CrashReporter reporter( QUrl( CRASHREPORTER_SUBMIT_URL ),  app.arguments() );

    reporter.setWindowTitle( CRASHREPORTER_PRODUCT_NAME );
    reporter.setText("<html><head/><body><p><span style=\"font-weight:600;\">Sorry!</span> " CRASHREPORTER_PRODUCT_NAME " crashed. Please tell us about it! " CRASHREPORTER_PRODUCT_NAME " has created an error report for you that can help improve the stability in the future. You can now send this report directly to the " CRASHREPORTER_PRODUCT_NAME " developers.</p><p>Can you tell us what you were doing when this happened?</p></body></html>");

    reporter.setReportData( "BuildID", CRASHREPORTER_BUILD_ID );
    reporter.setReportData( "ProductName",  CRASHREPORTER_PRODUCT_NAME );
    reporter.setReportData( "Version", CRASHREPORTER_VERSION_STRING );
    reporter.setReportData( "ReleaseChannel", CRASHREPORTER_RELEASE_CHANNEL);

    reporter.show();

    return app.exec();
}
