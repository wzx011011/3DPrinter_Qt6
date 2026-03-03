#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSGRendererInterface>
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
  engine->rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
  engine->load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

  // Retranslate all QML qsTr() after language switch
  QObject::connect(&backend, &BackendContext::languageChanged,
                   engine, &QQmlApplicationEngine::retranslate);

  if (engine->rootObjects().isEmpty())
  {
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
