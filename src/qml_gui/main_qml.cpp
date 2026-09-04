#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QLibrary>
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
#include <tlhelp32.h>
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

#ifdef Q_OS_WIN
// P7.1: remote-control tools (ToDesk / 向日葵 / AnyDesk / TeamViewer /
// RustDesk) drive the console session directly — SM_REMOTESESSION stays 0 —
// so their presence is detected by process name instead. Matches the exe
// name by case-insensitive prefix so "_Service"/"_x64" suffixes also hit.
static bool remoteControlToolRunning()
{
  const wchar_t *kMarkers[] = {
      L"todesk", L"sunlogin", L"oray", L"anydesk", L"teamviewer", L"rustdesk",
  };
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE)
    return false;
  PROCESSENTRY32W entry{};
  entry.dwSize = sizeof(entry);
  bool found = false;
  if (Process32FirstW(snapshot, &entry))
  {
    do
    {
      const QString exe =
          QString::fromWCharArray(entry.szExeFile).toLower();
      for (const wchar_t *marker : kMarkers)
      {
        if (exe.startsWith(QString::fromWCharArray(marker)))
        {
          found = true;
          break;
        }
      }
    } while (!found && Process32NextW(snapshot, &entry));
  }
  CloseHandle(snapshot);
  return found;
}
#endif

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

// FIXTURE-04 (anti-feature): the 4 QCommandLineOption flags parsed below
// (--open-page, --open-dialog, --load-model, --skip-first-run) are OWzx-only
// test-evidence plumbing. Upstream OrcaSlicer has NO equivalent argv surface;
// upstream argv is CLI-only (--load / --slice / positional at OrcaSlicer.cpp:7183).
// These flags exist so external screenshot capture can reach a target GUI state
// without simulated clicks, and they are gated on objectCreated +
// QQuickWindow::frameSwapped (FIXTURE-02) so screenshots are deterministic.
// They MUST NOT be promoted to a user-facing deep-link product feature. See
// tests/data/fixture_recipes.md for the canonical argv combos and
// REQUIREMENTS.md (WS3 Out of Scope) for the anti-feature contract.
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
  // WebEngine quick module init MUST run before QGuiApplication (it sets
  // Qt::AA_ShareOpenGLContexts so Chromium's GPU context can share with the
  // scene graph). Resolved dynamically: statically linking WebEngineQuick
  // into the exe makes the loader abort pre-main on machines with old
  // dependency DLLs beside the exe; loading it here keeps startup decoupled.
  // When the DLL is absent the chat panel degrades gracefully (QML plugin
  // import fails with a message, the rest of the app runs).
  {
    QLibrary webEngineQuick(QStringLiteral("Qt6WebEngineQuick"));
    if (webEngineQuick.load()) {
      using WebEngineInit = void (*)();
      const QString kInitMangled =
          QStringLiteral("?initialize@QtWebEngineQuick@@YAXXZ");
      if (auto *init = reinterpret_cast<WebEngineInit>(
              webEngineQuick.resolve(kInitMangled.toUtf8().constData())))
        init();
    }
  }

  if (!qEnvironmentVariableIsSet("OWZX_RHI_RENDERER"))
    qputenv("OWZX_RHI_RENDERER", "auto");

  // Phase 105 (D3D12-01): forward OWZX_D3D12_DEBUG to Qt's QSG RHI debug
  // mechanism so the LIVE QQuickRhiItem render path emits GPU validation
  // output for Phase 106 triage. This MUST run BEFORE QGuiApplication
  // construction because Qt Quick reads QSG_RHI_DEBUG during QGuiApplication
  // startup (it configures the scene-graph RHI backend init). The crash
  // (0xc0000005) fires in RhiViewportRenderer.cpp:282-298 (beginPass-after-
  // resourceUpdate on the live D3D12 render path), NOT at the probe
  // QRhi::create, so the probe-path enableDebugLayer alone is insufficient.
  // QSG_RHI_DEBUG covers the live render path (DL-03 option a). Fully env-
  // gated (DL-04 / Pitfall 5): when OWZX_D3D12_DEBUG is unset, no QSG_RHI_DEBUG
  // leak into the default OWzxSlicer.exe build.
  if (qEnvironmentVariableIsSet("OWZX_D3D12_DEBUG"))
    qputenv("QSG_RHI_DEBUG", "1");

  // RhiBackendSelector owns D3D11-first / D3D12 explicit opt-in policy.
  // D3D12 is NOT the default because it crashes at QQuickWindow swapchain
  // init on AMD Radeon APU (v5.7 Phase 211 finding). OWZX_RHI_RENDERER=d3d12
  // opts in on verified discrete-GPU hosts.

#ifdef Q_OS_WIN
  // P7.1 (remote access): under a remote session (ToDesk / RDP / virtual
  // display drivers), GPU-presented swapchain frames are frequently invisible
  // to the remote capture pipeline — the window LOOKS frozen (no hover
  // highlight, clicks appear dead) while input actually lands. Route the
  // whole UI to the Qt Quick 2D software backend, which presents through
  // GDI-compatible bitmaps every capture path sees. GLViewport then resolves
  // to SoftwareViewport (QQuickPaintedItem), so the 3D canvas renders through
  // the same path. Trade-off: lower peak render performance in remote
  // sessions — accepted in exchange for a fully operable window.
  //
  // Detection covers BOTH transport kinds:
  //   - SM_REMOTESESSION: true for terminal-services remoting (RDP).
  //   - remoteControlToolRunning(): ToDesk / 向日葵 / AnyDesk / TeamViewer /
  //     RustDesk do NOT create a terminal-services session — the marker is
  //     their own process running on the host.
  const bool remoteSession = GetSystemMetrics(SM_REMOTESESSION) != 0
      || remoteControlToolRunning();
  // Wave 2 GUI perf: always-on remote-control SERVICES (ToDesk/Sunlogin
  // running in the background of a local session) must not permanently pin
  // the app to software rendering. OWZX_FORCE_RHI=1 overrides the detection
  // for local benchmarking / driver debugging (docs/perf-baseline.md).
  const bool forceRhi = qEnvironmentVariableIntValue("OWZX_FORCE_RHI") != 0;
  if (remoteSession && !forceRhi) {
    qputenv("QT_QUICK_BACKEND", "software");
    appendStartupLog(QStringLiteral(
        "Remote session detected: software rendering + SoftwareViewport enabled"));
  }
#else
  const bool remoteSession = false;
#endif

  const RhiBackendSelection rhiSelection = selectRhiBackendFromEnvironment();

  if (rhiSelection.canUseRhi && (!remoteSession || forceRhi)) {
    QQuickWindow::setGraphicsApi(rhiSelection.selectedGraphicsApi);
  } else {
    if (rhiSelection.enabled)
      appendStartupLog(QStringLiteral("QRhi requested but unavailable; falling back to software viewport"));
    qputenv("QT_QUICK_BACKEND", "software");
  }

  // Enable QML debugging output for diagnostics
  // qml.debug=true keeps console.log visible in the diagnostics file (the
  // rules string replaces defaults, so it must be listed explicitly).
  qputenv("QT_LOGGING_RULES",
          "qt.qml.binding=true;qt.qml.connections=true;qml.debug=true");

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
  // SoftwareViewport retained as a driver/init fallback when QRhi is unavailable
  // and forced for remote sessions (P7.1) where GPU frames are not capturable.
  if (rhiSelection.canUseRhi && (!remoteSession || forceRhi))
    qmlRegisterType<RhiViewport>("OWzxGL", 1, 0, "GLViewport");
  else
    qmlRegisterType<SoftwareViewport>("OWzxGL", 1, 0, "GLViewport");

  BackendContext backend;
  // Phase 241 (PAGE-04): the persisted startup-page preference
  // (showHomePage/defaultPage, upstream app_config "show_home_page" /
  // "default_page") drives the initial page BEFORE QML loads, so the
  // StackLayout lands on the user's chosen page on the first frame.
  backend.applyStartupPagePreference();
  appendStartupLog(QStringLiteral("BackendContext constructed"));

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
  appendStartupLog(QStringLiteral("context property set, loading main.qml"));
  engine->rootContext()->setContextProperty(QStringLiteral("startupSkipFirstRun"),
                                            startupOpenRequest.skipFirstRun);
  // The main window is frameless + maximized by default (declared directly in
  // main.qml) for screenshot parity with OrcaSlicer. No context-property toggle
  // is exposed — the frameless shell is always on.

  // v2.6 CAM-03：注册摄像头图像提供者（image://camera/live），供 MonitorPage 实时视频显示
  // provider 归 engine 所有，engine 在进程退出时被故意 leak（见上方注释），故不手动释放。
  engine->addImageProvider(QStringLiteral("camera"),
                           new CameraImageProvider(backend.cameraService()));

  appendStartupLog(QStringLiteral("calling engine->load(main.qml)"));
  engine->load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
  appendStartupLog(QStringLiteral("engine->load returned"));

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
      // Wave 2 GUI perf (bugfix): the old one-shot freed the
      // QMetaObject::Connection inside the handler; with Qt::QueuedConnection
      // a SECOND frameSwapped meta-call can already sit in the queue when the
      // first handler runs (heavy first frames, e.g. a 2M-triangle
      // --load-model), and the second delivery dereferenced the freed
      // connection (AV in QObjectPrivate::removeConnection). The shared-ptr
      // guard makes the handler idempotent with no raw delete.
      auto frameGateConnection = std::make_shared<QMetaObject::Connection>();
      *frameGateConnection = QObject::connect(
          rootWindow, &QQuickWindow::frameSwapped, rootWindow,
          [&backend, startupOpenRequest, frameGateConnection]() {
            if (!(*frameGateConnection))
              return; // already fired once
            QObject::disconnect(*frameGateConnection);
            *frameGateConnection = QMetaObject::Connection{};
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

  appendStartupLog(QStringLiteral("entering app.exec() event loop"));
  return app.exec();
}
