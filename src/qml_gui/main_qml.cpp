#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QtQml/qqml.h>
#include "qml_gui/Renderer/GLViewport.h"

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
    const MARGINS margins = {1, 1, 1, 1};
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
  QQuickWindow::setDefaultAlphaBuffer(true);
  // Force OpenGL backend — required for QQuickFramebufferObject on Windows.
  // Must be called before QGuiApplication is constructed.
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

  QGuiApplication app(argc, argv);
  app.setOrganizationName(QStringLiteral("CrealityDemo"));
  app.setApplicationName(QStringLiteral("Print7Shell"));

  // E5 — register 3-D viewport type
  qmlRegisterType<GLViewport>("CrealityGL", 1, 0, "GLViewport");

  BackendContext backend;

  // Qt 6.10 MSVC debug heap crash: QQmlApplicationEngine dtor destroys
  // QQuickFramebufferObject (GLViewport) and corrupts the heap.
  // Intentionally leak the engine to skip its destructor — same fix as
  // VisualRegressionTests. The OS reclaims all memory on process exit.
  auto *engine = new QQmlApplicationEngine;
  QObject::connect(engine, &QQmlEngine::warnings, engine,
                   [](const QList<QQmlError> &warnings) {
                     for (const QQmlError &error : warnings)
                     {
                       appendStartupLog(QStringLiteral("[QML WARNING] %1").arg(error.toString()));
                     }
                   });
  QObject::connect(engine, &QQmlApplicationEngine::objectCreationFailed, engine,
                   [](const QUrl &url) {
                     appendStartupLog(QStringLiteral("[QML CREATE FAILED] %1").arg(url.toString()));
                   });

  engine->rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
  appendStartupLog(QStringLiteral("Loading qrc:/qml/main.qml"));
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
    window->setColor(Qt::transparent);
    enableWindowShadowAndRoundedCorner(window);
  }
#endif

  return app.exec();
}
