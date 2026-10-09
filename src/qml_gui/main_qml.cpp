#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlAbstractUrlInterceptor>
#include <QQmlContext>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QLibrary>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QSettings>
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
#include "qml_gui/Models/ConfigOptionFilterProxy.h"
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
  // --qml-dev: load the QML tree from the source checkout instead of the
  // compiled-in resources. UI iterations then need no rcc/TU/link pass --
  // edit a .qml, relaunch. Falls back to qrc when the source layout is not
  // next to the exe (installed / CI builds).
  bool qmlDev = false;
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
  QCommandLineOption qmlDevOption(
      QStringLiteral("qml-dev"),
      QStringLiteral("Load QML from the source tree instead of embedded resources "
                     "(UI iteration without rebuild; falls back to qrc when the "
                     "source layout is unavailable)."));

  parser.addOption(openPageOption);
  parser.addOption(openDialogOption);
  parser.addOption(loadModelOption);
  parser.addOption(skipFirstRunOption);
  parser.addOption(qmlDevOption);
  parser.process(app);

  StartupOpenRequest request;
  request.page = parser.value(openPageOption);
  request.dialogs = parser.values(openDialogOption);
  request.modelPaths = parser.values(loadModelOption);
  request.skipFirstRun = parser.isSet(skipFirstRunOption);
  request.qmlDev = parser.isSet(qmlDevOption);
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

// Phase 171/173 (P1/P3): thin QSettings facade for QML. QSettings itself
// does not expose value()/setValue() as Q_INVOKABLE, so a bare QSettings
// context property is call-blind from QML; main.qml's Theme bridge and the
// appearance actions call through this store instead. Backed by the
// dedicated "OWzx/UI" store -- the same one main() reads ui/scaleFactor
// from pre-app-creation, so registry seeding pre-launch is honored.
class UiSettingsStore : public QObject
{
  Q_OBJECT

public:
  explicit UiSettingsStore(QObject *parent = nullptr) : QObject(parent) {}

  Q_INVOKABLE QVariant value(const QString &key, const QVariant &defaultValue = {}) const
  {
    return settings_.value(key, defaultValue);
  }

  Q_INVOKABLE void setValue(const QString &key, const QVariant &val)
  {
    settings_.setValue(key, val);
  }

private:
  QSettings settings_{QStringLiteral("OWzx"), QStringLiteral("UI")};
};

int main(int argc, char *argv[])
{
  // WebEngine quick module init MUST run before QGuiApplication (it sets
  // Qt::AA_ShareOpenGLContexts so Chromium's GPU context can share with the
  // scene graph). Resolved dynamically: statically linking WebEngineQuick
  // into the exe makes the loader abort pre-main on machines with old
  // dependency DLLs beside the exe; loading it here keeps startup decoupled.
  // When the DLL is absent the chat panel degrades gracefully (QML plugin
  // import fails inside the lazy aiChatPanelLoader, the rest of the app
  // runs).
  // OWZX_DISABLE_WEBENGINE=1 skips the init entirely (main.qml must be the
  // source-based lazy Loader for this to stay survivable). Diagnostic/work-
  // around knob for the Chromium GUI-thread pump interfering with the Qt
  // event dispatcher on this app (2026-09-12 freeze investigation).
  if (!qEnvironmentVariableIsSet("OWZX_DISABLE_WEBENGINE")) {
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

  // Qt 6.10 routes window update requests through QDxgiVSyncService: when a
  // window maps to a DXGI output, QWindowsWindow::requestUpdate() registers a
  // vsync callback and stops the platform timer fallback, so update requests
  // are ONLY delivered from QDxgiVSyncThread (which loops on
  // IDXGIOutput::WaitForVBlank and fires the callback on success). On this
  // stack (AMD amdxx64, Win11 build 26200) WaitForVBlank returns in <=1 ms or
  // fails, and that thread then just msleeps -- the callback never fires.
  // Result: the Quick render thread blocks forever in QWaitCondition::wait
  // and nothing is presented after the first frames (input keeps being
  // processed invisibly; only resize/expose forces a frame). Reproduced 100%
  // at launch and verified via cdb render-thread stacks; setting
  // QT_D3D_NO_VBLANK_THREAD=1 (Qt's own escape hatch, read once when the
  // service is constructed) makes supportsWindow() return false so
  // requestUpdate() falls back to the timer-based delivery. Verified 2026-
  // 09-12: tab switches, click-to-page and orbit drags present live, clock
  // repaints, render thread sits in QRhi::beginFrame instead of the dead
  // wait. Must run before QGuiApplication; a user-set value is respected.
  if (!qEnvironmentVariableIsSet("QT_D3D_NO_VBLANK_THREAD"))
    qputenv("QT_D3D_NO_VBLANK_THREAD", "1");

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
  const RhiBackendSelection rhiSelection = selectRhiBackendFromEnvironment();

  if (rhiSelection.canUseRhi) {
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

  // Phase 173 (P3): UI scale. QT_SCALE_FACTOR must be set before the
  // QGuiApplication is constructed, so read it straight from the same
  // "OWzx/UI" settings store the command palette writes to. Values outside
  // [0.5, 3.0] are ignored (falls back to the system default 1.0).
  {
    QSettings uiScale(QSettings::NativeFormat, QSettings::UserScope,
                      QStringLiteral("OWzx"), QStringLiteral("UI"));
    bool scaleOk = false;
    const double scaleValue =
        uiScale.value(QStringLiteral("ui/scaleFactor")).toDouble(&scaleOk);
    if (scaleOk && scaleValue >= 0.5 && scaleValue <= 3.0
        && !qFuzzyCompare(scaleValue, 1.0)) {
      qputenv("QT_SCALE_FACTOR", QString::number(scaleValue).toUtf8());
      appendStartupLog(QStringLiteral("QT_SCALE_FACTOR=%1").arg(scaleValue));
    }
  }

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

  // ctl-4 (upstream Label.cpp:22,99-100): upstream privately installs
  // "HarmonyOS Sans SC" via AddPrivateFont and builds every Head_/Body_
  // system font on that family, so the whole UI renders in it. Load the
  // bundled TTFs into QFontDatabase BEFORE the QML engine starts and install
  // the family as the application font, so every QML Text item without an
  // explicit font.family inherits it instead of falling back to Segoe UI.
  // The family name must stay in sync with the Theme.fontFamily token.
  // If the resources are missing (addApplicationFont returns -1) fall back
  // to Microsoft YaHei, which is CJK-safe on Windows.
  {
    const int regularFontId = QFontDatabase::addApplicationFont(
        QStringLiteral(":/qml/assets/fonts/HarmonyOS_Sans_SC_Regular.ttf"));
    const int boldFontId = QFontDatabase::addApplicationFont(
        QStringLiteral(":/qml/assets/fonts/HarmonyOS_Sans_SC_Bold.ttf"));
    QString uiFontFamily = QStringLiteral("HarmonyOS Sans SC");
    if (regularFontId < 0 && boldFontId < 0)
      uiFontFamily = QStringLiteral("Microsoft YaHei");
    QFont uiFont(uiFontFamily);
    uiFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(uiFont);
    appendStartupLog(QStringLiteral("UI font family: %1 (regular id %2, bold id %3)")
                         .arg(uiFontFamily).arg(regularFontId).arg(boldFontId));
  }

#ifdef Q_OS_WIN
  // WIN-AI-ORPHAN (crash-safe child containment): this process hosts the AI
  // sidecar tree (python agent.py -> claude.exe/node.exe) via QProcess. If
  // the app dies -- especially on a crash -- the QProcess children survive
  // as orphans; an orphaned SDK client then reconnects to the NEXT app
  // instance's loopback MCP server (fixed aiPort) and its stale-session
  // churn races the server during startup (observed 2026-09-13..15 as
  // delayed Qt6Core!QObjectPrivate::removeConnection heap corruption ~60s
  // after every launch, escalating 6/6 once the first orphan existed).
  // Putting THIS process into a kill-on-close job object makes every child
  // inherit membership, so the whole tree is reaped by the kernel on any
  // exit path -- clean quit, taskkill, or crash. Nested jobs (Win8+) are
  // legal, so an outer harness job does not break this.
  {
    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (job) {
      JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit = {};
      limit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
      if (SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                                  &limit, sizeof(limit))) {
        if (!AssignProcessToJobObject(job, GetCurrentProcess()))
          CloseHandle(job);  // keep no live handle when assignment failed
      } else {
        CloseHandle(job);
      }
      // Deliberately leak the job handle on success: it is closed by the
      // kernel when this process exits, which is exactly the trigger for
      // KILL_ON_JOB_CLOSE.
    }
  }
#endif

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

  // Per-page C++ filter proxies for the params UIs (LeftSidebar pages and
  // SettingsDialog lists) are declared in place from QML.
  qmlRegisterType<ConfigOptionFilterProxy>("OWzx.Models", 1, 0, "ConfigOptionFilterProxy");

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

  // Dev-only QML source override (docs/ui-reference/restoration-map.md §5
  // loop): set OWZX_DEV_QML_DIR=<src/qml_gui absolute path> to redirect every
  // qrc:/qml/... load to the source tree, so QML edits only need an app
  // restart instead of the rcc+link rebuild cycle. Never set in production;
  // pair with QML_DISABLE_DISK_CACHE=1 so the qrc-keyed qmlc cache cannot
  // serve stale bytecode across qrc/file swaps. The interceptor instance is
  // intentionally leaked, matching the engine's leak-on-purpose lifetime.
  const QString devQmlDir = qEnvironmentVariable("OWZX_DEV_QML_DIR");
  if (!devQmlDir.isEmpty())
  {
    class DevQmlSourceInterceptor : public QQmlAbstractUrlInterceptor
    {
    public:
      explicit DevQmlSourceInterceptor(QString baseDir) : baseDir_(std::move(baseDir)) {}
      QUrl intercept(const QUrl &url, DataType type) override
      {
        Q_UNUSED(type);
        if (url.scheme() != QLatin1String("qrc"))
          return url;
        const QString path = url.path();
        const QString prefix = QLatin1String("/qml/");
        if (!path.startsWith(prefix))
          return url;
        return QUrl::fromLocalFile(baseDir_ + path.mid(prefix.size() - 1));
      }
    private:
      QString baseDir_;
    };
    engine->addUrlInterceptor(new DevQmlSourceInterceptor(QDir::fromNativeSeparators(devQmlDir)));
    appendStartupLog(QStringLiteral("dev QML source override active: %1").arg(devQmlDir));
  }
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
  // Phase 171 (P1): appearance store. QSettings' value()/setValue() are not
  // Q_INVOKABLE, so a bare QSettings context property is call-blind from
  // QML; this thin store re-exposes the same API as invokables. Uses the
  // dedicated "OWzx/UI" store (same one main() reads ui/scaleFactor from)
  // so seeding the registry pre-launch works for screenshots/tests.
  engine->rootContext()->setContextProperty(QStringLiteral("appSettings"),
                                            new UiSettingsStore(&app));
  engine->rootContext()->setContextProperty(QStringLiteral("startupSkipFirstRun"),
                                            startupOpenRequest.skipFirstRun);
  // layout-1: the removed StatusBar used to surface backend.latencyBrief
  // unconditionally at the window bottom. That debug data is env-gated now:
  // main.qml renders a tiny bottom-right latency overlay only when this
  // context property is true (QML_DEBUG_LOG or OWZX_DEBUG_OVERLAY set).
  engine->rootContext()->setContextProperty(
      QStringLiteral("debugOverlayVisible"),
      QVariant(qEnvironmentVariableIsSet("QML_DEBUG_LOG")
               || qEnvironmentVariableIsSet("OWZX_DEBUG_OVERLAY")));
  // The main window is frameless + maximized by default (declared directly in
  // main.qml) for screenshot parity with OrcaSlicer. No context-property toggle
  // is exposed — the frameless shell is always on.

  // v2.6 CAM-03：注册摄像头图像提供者（image://camera/live），供 MonitorPage 实时视频显示
  // provider 归 engine 所有，engine 在进程退出时被故意 leak（见上方注释），故不手动释放。
  engine->addImageProvider(QStringLiteral("camera"),
                           new CameraImageProvider(backend.cameraService()));

  appendStartupLog(QStringLiteral("calling engine->load(main.qml)"));
  // --qml-dev: load from the source checkout (dev UI iteration, no rebuild).
  // The tree must sit one level up from the exe (build/ -> src/qml_gui),
  // matching the qrc layout under /qml so relative imports resolve the same.
  QUrl mainQmlUrl(QStringLiteral("qrc:/qml/main.qml"));
  if (startupOpenRequest.qmlDev)
  {
    const QString devMainQml = QCoreApplication::applicationDirPath()
                               + QStringLiteral("/../src/qml_gui/main.qml");
    if (QFileInfo::exists(devMainQml))
    {
      mainQmlUrl = QUrl::fromLocalFile(QDir::cleanPath(devMainQml));
      // Filesystem QML reads its own qmldir/components -- keep the resource
      // import path too so OWzx-registered types and the conf stay available.
      appendStartupLog(QStringLiteral("qml-dev: loading %1").arg(mainQmlUrl.toString()));
    }
    else
    {
      appendStartupLog(QStringLiteral("qml-dev: %1 not found, falling back to qrc")
                           .arg(devMainQml));
    }
  }
  engine->load(mainQmlUrl);
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

  appendStartupLog(QStringLiteral("entering app.exec() event loop"));
  return app.exec();
}

#include "main_qml.moc"
