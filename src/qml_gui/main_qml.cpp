#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QtQml/qqml.h>
#include "qml_gui/Renderer/GLViewport.h"
#include "core/debug/CrashHandlerWin.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif

enum DwmWindowCornerPreference
{
  DwmwcpDefault = 0,
  DwmwcpDoNotRound = 1,
  DwmwcpRound = 2,
  DwmwcpRoundSmall = 3
};

static void enableWindowShadowAndRoundedCorner(QWindow *window)
{
  if (window == nullptr)
  {
    return;
  }

  const HWND hwnd = reinterpret_cast<HWND>(window->winId());
  if (hwnd == nullptr)
  {
    return;
  }

  BOOL compositionEnabled = FALSE;
  if (SUCCEEDED(DwmIsCompositionEnabled(&compositionEnabled)) && compositionEnabled)
  {
    // Use margins {0,0,0,0} to avoid DWM glass effect making the window transparent.
    // Rounded corners are provided by DWMWA_WINDOW_CORNER_PREFERENCE below.
    const MARGINS margins = {0, 0, 0, 0};
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    const DwmWindowCornerPreference pref = DwmwcpRound;
    DwmSetWindowAttribute(hwnd,
                          DWMWA_WINDOW_CORNER_PREFERENCE,
                          &pref,
                          sizeof(pref));
  }
}
#endif

#include "BackendContext.h"

static void appendStartupLog(const QString &line)
{
  const QString path = QCoreApplication::applicationDirPath() + QStringLiteral("/startup_diagnostics.log");
  QFile file(path);
  if (!file.open(QIODevice::Append | QIODevice::Text))
  {
    return;
  }
  QTextStream out(&file);
  out << QDateTime::currentDateTime().toString(Qt::ISODateWithMs)
      << " " << line << "\n";
}

int main(int argc, char *argv[])
{
  // Do NOT set defaultAlphaBuffer(true) — it causes the window to be
  // fully transparent on some Windows 10 / GPU driver combinations even
  // when the window color is explicitly set to an opaque value.
  // Force OpenGL backend — required for QQuickFramebufferObject on Windows.
  // Must be called before QGuiApplication is constructed.
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

  // Enable QML debugging output for diagnostics
  qputenv("QT_LOGGING_RULES", "qt.qml.binding=true;qt.qml.connections=true");

  // Redirect all Qt messages to diagnostic log file
  if (qEnvironmentVariableIsSet("QML_DEBUG_LOG")) {
    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext &ctx, const QString &msg) {
      appendStartupLog(QString("[%1] %2: %3")
        .arg(type == QtWarningMsg ? "WRN" : type == QtCriticalMsg ? "CRI" : type == QtDebugMsg ? "DBG" : "INF",
             ctx.category ? ctx.category : "",
             msg));
    });
  }

  QGuiApplication app(argc, argv);
  app.setOrganizationName(QStringLiteral("CrealityDemo"));
  app.setApplicationName(QStringLiteral("Print7Shell"));
  const QString dumpDir = QCoreApplication::applicationDirPath() + QStringLiteral("/crash_dumps");
  QDir().mkpath(dumpDir);
  appendStartupLog(QStringLiteral("Crash dump dir prepared: %1").arg(dumpDir));
  CrashHandlerWin::install(dumpDir);
  appendStartupLog(QStringLiteral("Crash handler install requested"));

  // E5 — register 3-D viewport type
  qmlRegisterType<GLViewport>("CrealityGL", 1, 0, "GLViewport");

  BackendContext backend;

  // Qt 6.10 MSVC debug heap crash: QQmlApplicationEngine dtor destroys
  // QQuickFramebufferObject (GLViewport) and corrupts the heap.
  // Intentionally leak the engine to skip its destructor — same fix as
  // VisualRegressionTests. The OS reclaims all memory on process exit.
  auto *engine = new QQmlApplicationEngine;
  QObject::connect(engine, &QQmlEngine::warnings, engine,
                   [](const QList<QQmlError> &warnings)
                   {
                     for (const QQmlError &error : warnings)
                     {
                       appendStartupLog(QStringLiteral("[QML WARNING] %1").arg(error.toString()));
                     }
                   });
  QObject::connect(engine, &QQmlApplicationEngine::objectCreationFailed, engine, []()
                   {
                     appendStartupLog(QStringLiteral("QML object creation failed"));
                     QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  engine->rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
  engine->load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

  // Retranslate all QML qsTr() after language switch
  QObject::connect(&backend, &BackendContext::languageChanged,
                   engine, &QQmlApplicationEngine::retranslate);

  if (engine->rootObjects().isEmpty())
  {
    appendStartupLog(QStringLiteral("rootObjects is empty, exiting with -1"));
    return -1;
  }

#ifdef Q_OS_WIN
  if (auto *window = qobject_cast<QQuickWindow *>(engine->rootObjects().first()))
  {
    enableWindowShadowAndRoundedCorner(window);
  }
#endif

  return app.exec();
}
