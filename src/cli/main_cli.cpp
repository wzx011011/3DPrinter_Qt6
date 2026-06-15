#include <QCoreApplication>
#include "CliRunner.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("owzx-cli"));
    app.setApplicationVersion(QStringLiteral("2.4.0-dev"));

    CliRunner runner(&app);
    return runner.run();
}
