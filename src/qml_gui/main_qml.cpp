#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QStringList>
#include <QVector>
#include <QtQml/qqml.h>
#include <functional>
#include "qml_gui/Renderer/RhiBackendSelector.h"
#include "qml_gui/Renderer/RhiViewport.h"
#include "qml_gui/Renderer/SoftwareViewport.h"
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
#include "qml_gui/CameraImageProvider.h"

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

struct StartupOpenRequest
{
  QString page;
  QStringList dialogs;
  QStringList modelPaths;
  bool skipFirstRun = false;
};

struct StartupPageRoute
{
  QStringList aliases;
  int page = 0;
};

struct StartupDialogRoute
{
  QStringList aliases;
  std::function<void(BackendContext &)> open;
};

static QString normalizeStartupToken(QString token)
{
  token = token.trimmed().toLower();
  token.replace(QLatin1Char('_'), QLatin1Char('-'));
  return token;
}

static bool routeMatches(const QStringList &aliases, const QString &value)
{
  const QString normalized = normalizeStartupToken(value);
  for (const QString &alias : aliases)
  {
    if (normalizeStartupToken(alias) == normalized)
      return true;
  }
  return false;
}

static QVector<StartupPageRoute> startupPageRoutes()
{
  using Tab = BackendContext::TabPosition;
  return {
      {{QStringLiteral("home")}, static_cast<int>(Tab::tpHome)},
      {{QStringLiteral("prepare"), QStringLiteral("3d"), QStringLiteral("editor"), QStringLiteral("plater")},
       static_cast<int>(Tab::tp3DEditor)},
      {{QStringLiteral("preview")}, static_cast<int>(Tab::tpPreview)},
      {{QStringLiteral("device"), QStringLiteral("monitor")}, static_cast<int>(Tab::tpDevice)},
      {{QStringLiteral("multi-device"), QStringLiteral("multi")}, static_cast<int>(Tab::tpMultiDevice)},
      {{QStringLiteral("project")}, static_cast<int>(Tab::tpProject)},
      {{QStringLiteral("calibration"), QStringLiteral("calibrate")}, static_cast<int>(Tab::tpCalibration)},
  };
}

static QVector<StartupDialogRoute> startupDialogRoutes()
{
  return {
      {{QStringLiteral("settings:printer"), QStringLiteral("printer-settings")},
       [](BackendContext &backend) { backend.forwardSettingsRequest(QStringLiteral("printer")); }},
      {{QStringLiteral("settings:filament"), QStringLiteral("settings:material"),
        QStringLiteral("filament-settings"), QStringLiteral("material-settings")},
       [](BackendContext &backend) { backend.forwardSettingsRequest(QStringLiteral("filament")); }},
      {{QStringLiteral("settings:process"), QStringLiteral("settings:print"),
        QStringLiteral("process-settings"), QStringLiteral("print-settings")},
       [](BackendContext &backend) { backend.forwardSettingsRequest(QStringLiteral("process")); }},
      {{QStringLiteral("config-wizard"), QStringLiteral("wizard")},
       [](BackendContext &backend) { backend.showConfigWizard(); }},
      {{QStringLiteral("bed-shape"), QStringLiteral("bed")},
       [](BackendContext &backend) { backend.showBedShapeDialog(); }},
      {{QStringLiteral("ams-settings"), QStringLiteral("ams")},
       [](BackendContext &backend) { backend.showAMSSettingsDialog(); }},
      {{QStringLiteral("firmware")},
       [](BackendContext &backend) { backend.showFirmwareDialog(); }},
      {{QStringLiteral("speed-limit")},
       [](BackendContext &backend) { backend.showSpeedLimitDialog(); }},
      {{QStringLiteral("wipe-tower")},
       [](BackendContext &backend) { backend.showWipeTowerDialog(); }},
      {{QStringLiteral("print-host")},
       [](BackendContext &backend) { backend.showPrintHostDialog(); }},
      {{QStringLiteral("plugin-manager"), QStringLiteral("plugins")},
       [](BackendContext &backend) { backend.showPluginManagerDialog(); }},
      {{QStringLiteral("lite-mode"), QStringLiteral("enable-lite-mode")},
       [](BackendContext &backend) { backend.showEnableLiteModeDialog(); }},
  };
}

static StartupOpenRequest parseStartupOpenRequest(QCoreApplication &app)
{
  QCommandLineParser parser;
  parser.setApplicationDescription(QStringLiteral("OWzx Slicer GUI"));
  parser.addHelpOption();
  parser.addVersionOption();

  QCommandLineOption openPageOption(
      QStringLiteral("open-page"),
      QStringLiteral("Open a top-level page after QML startup."),
      QStringLiteral("page"));
  QCommandLineOption openDialogOption(
      QStringLiteral("open-dialog"),
      QStringLiteral("Open a dialog after QML startup. Can be specified multiple times."),
      QStringLiteral("dialog"));
  QCommandLineOption loadModelOption(
      QStringLiteral("load-model"),
      QStringLiteral("Load a model file after QML startup. Can be specified multiple times."),
      QStringLiteral("path"));
  QCommandLineOption skipFirstRunOption(
      QStringLiteral("skip-first-run"),
      QStringLiteral("Mark the first-run config wizard complete for this startup."));

  parser.addOption(openPageOption);
  parser.addOption(openDialogOption);
  parser.addOption(loadModelOption);
  parser.addOption(skipFirstRunOption);
  parser.process(app);

  StartupOpenRequest request;
  request.page = parser.value(openPageOption);
  request.dialogs = parser.values(openDialogOption);
  request.modelPaths = parser.values(loadModelOption);
  request.skipFirstRun = parser.isSet(skipFirstRunOption);
  return request;
}

// FIXTURE-02: applies the parsed startup open-page / load-model / open-dialog
// requests directly against the backend. The readiness gate (rootObjects check
// + QQuickWindow::frameSwapped one-shot) lives at the call site in main(),
// where the QQmlApplicationEngine is in scope. This function only runs after
// the gate has fired, so the QML scene graph has rendered at least one frame
// and screenshots are deterministic. The previous zero-delay timer trick fired
// on the next event-loop iteration, before the first frame was guaranteed.
static void applyStartupOpenRequests(const StartupOpenRequest &request,
                                     BackendContext &backend)
{
  if (!request.page.isEmpty())
  {
    bool handled = false;
    for (const StartupPageRoute &route : startupPageRoutes())
    {
      if (routeMatches(route.aliases, request.page))
      {
        backend.requestSelectTab(route.page);
        appendStartupLog(QStringLiteral("Startup open-page handled: %1").arg(request.page));
        handled = true;
        break;
      }
    }
    if (!handled)
      appendStartupLog(QStringLiteral("Startup open-page ignored: %1").arg(request.page));
  }

  for (const QString &modelPath : request.modelPaths)
  {
    const bool loaded = backend.topbarImportModel(modelPath);
    appendStartupLog(QStringLiteral("Startup load-model %1: %2")
                         .arg(loaded ? QStringLiteral("handled") : QStringLiteral("failed"),
                              modelPath));
  }

  for (const QString &dialog : request.dialogs)
  {
    bool handled = false;
    for (const StartupDialogRoute &route : startupDialogRoutes())
    {
      if (routeMatches(route.aliases, dialog))
      {
        route.open(backend);
        appendStartupLog(QStringLiteral("Startup open-dialog handled: %1").arg(dialog));
        handled = true;
        break;
      }
    }
    if (!handled)
      appendStartupLog(QStringLiteral("Startup open-dialog ignored: %1").arg(dialog));
  }
}

int main(int argc, char *argv[])
{
  if (!qEnvironmentVariableIsSet("OWZX_RHI_RENDERER"))
    qputenv("OWZX_RHI_RENDERER", "auto");

  // RhiBackendSelector owns D3D11-first / D3D12 explicit opt-in policy.
  const RhiBackendSelection rhiSelection = selectRhiBackendFromEnvironment();

  if (rhiSelection.canUseRhi) {
    QQuickWindow::setGraphicsApi(rhiSelection.selectedGraphicsApi);
  } else {
    if (rhiSelection.enabled)
      appendStartupLog(QStringLiteral("QRhi requested but unavailable; falling back to software viewport"));
    qputenv("QT_QUICK_BACKEND", "software");
  }

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
  app.setOrganizationName(QStringLiteral("OWzx"));
  app.setApplicationName(QStringLiteral("OWzxSlicer"));
  const StartupOpenRequest startupOpenRequest = parseStartupOpenRequest(app);
  const QString dumpDir = QCoreApplication::applicationDirPath() + QStringLiteral("/crash_dumps");
  QDir().mkpath(dumpDir);
  appendStartupLog(QStringLiteral("Crash dump dir prepared: %1").arg(dumpDir));
  CrashHandlerWin::install(dumpDir);
  appendStartupLog(QStringLiteral("Crash handler install requested"));
  if (rhiSelection.enabled)
    appendStartupLog(QStringLiteral("QRhi backend selection: %1").arg(rhiSelection.diagnostics()));

  // Register the stable QML type name. RhiViewport is the default path, with
  // SoftwareViewport retained as a driver/init fallback when QRhi is unavailable.
  if (rhiSelection.canUseRhi)
    qmlRegisterType<RhiViewport>("OWzxGL", 1, 0, "GLViewport");
  else
    qmlRegisterType<SoftwareViewport>("OWzxGL", 1, 0, "GLViewport");

  BackendContext backend;

  // Intentionally leak the engine to skip late Qt teardown hazards seen in
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
  engine->rootContext()->setContextProperty(QStringLiteral("startupSkipFirstRun"),
                                            startupOpenRequest.skipFirstRun);
  // The main window is frameless + maximized by default (declared directly in
  // main.qml) for screenshot parity with OrcaSlicer. No context-property toggle
  // is exposed — the frameless shell is always on.

  // v2.6 CAM-03：注册摄像头图像提供者（image://camera/live），供 MonitorPage 实时视频显示
  // provider 归 engine 所有，engine 在进程退出时被故意 leak（见上方注释），故不手动释放。
  engine->addImageProvider(QStringLiteral("camera"),
                           new CameraImageProvider(backend.cameraService()));

  engine->load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

  // Retranslate all QML qsTr() after language switch
  QObject::connect(&backend, &BackendContext::languageChanged,
                   engine, &QQmlApplicationEngine::retranslate);

  if (engine->rootObjects().isEmpty())
  {
    appendStartupLog(QStringLiteral("rootObjects is empty, exiting with -1"));
    return -1;
  }

  // FIXTURE-02: gate argv fixture application on BOTH (a) the QML object tree
  // being built (guaranteed by the rootObjects().isEmpty() check above, since
  // engine->load() is synchronous) AND (b) the first QQuickWindow::frameSwapped
  // signal (the scene graph has rendered at least one frame). The previous
  // zero-delay timer trick ran on the next event-loop iteration, before a frame
  // was guaranteed, so external screenshot capture could catch a blank/partial
  // window. The gate fires EXACTLY ONCE on the first frameSwapped (one-shot
  // connection: disconnect in the handler). If the root object is not a
  // QQuickWindow (defensive - shouldn't happen for main.qml), apply immediately
  // with a startup-log warning so the fixture plumbing degrades gracefully
  // instead of hanging forever. The empty-request case skips the gate entirely.
  if (!startupOpenRequest.page.isEmpty()
      || !startupOpenRequest.dialogs.isEmpty()
      || !startupOpenRequest.modelPaths.isEmpty())
  {
    auto *rootWindow = qobject_cast<QQuickWindow *>(engine->rootObjects().value(0));
    if (rootWindow)
    {
      auto *frameGateConnection = new QMetaObject::Connection;
      *frameGateConnection = QObject::connect(
          rootWindow, &QQuickWindow::frameSwapped, rootWindow,
          [&backend, startupOpenRequest, frameGateConnection]() {
            QObject::disconnect(*frameGateConnection);
            delete frameGateConnection;
            applyStartupOpenRequests(startupOpenRequest, backend);
          },
          Qt::QueuedConnection);
    }
    else
    {
      appendStartupLog(QStringLiteral("FIXTURE-02: root object is not a QQuickWindow; applying startup requests immediately (degraded mode)"));
      applyStartupOpenRequests(startupOpenRequest, backend);
    }
  }

#ifdef Q_OS_WIN
  if (qEnvironmentVariableIsSet("OWZX_FRAMELESS"))
  {
    if (auto *window = qobject_cast<QQuickWindow *>(engine->rootObjects().first()))
    {
      enableWindowShadowAndRoundedCorner(window);
    }
  }
#endif

  return app.exec();
}
