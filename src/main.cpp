#include <QApplication>
#include <QCoreApplication>

#include "MainWindow.h"
#include "core/debug/CrashHandlerWin.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  CrashHandlerWin::install(QCoreApplication::applicationDirPath() + QStringLiteral("/crash_dumps"));

  MainWindow window;
  window.show();

  return app.exec();
}
