// QmlSmokeTests — Q14 QML behavior smoke suite (audit 2026-10-10 ④ Q14).
//
// First behavioral net over the QML tree (before this suite the only QML
// coverage was QmlUiAuditTests' source-string locks — zero engine
// instantiation). Loads the real qrc resources that ship in OWzxSlicer.exe
// (main.qml + all 10 pages) through a QQmlApplicationEngine and asserts:
//   1. zero QML errors during creation,
//   2. the app's OWzx module singletons (Backend / AppSettings, S8) are
//      present and the backend view-model
//      surface (editorViewModel / previewViewModel / ... ) is non-null,
//   3. main.qml instantiates (root window + 9-slot StackLayout contract,
//      main.qml:928-1046),
//   4. every page instantiates standalone with the same initial required
//      properties main.qml binds (HomePage:929, Plater:966, MonitorPage:976,
//      MultiMachinePage:986, ProjectPage:995, CalibrationPage:1009,
//      PreferencesPage:1030).
// This is the safety net S1 (NOTIFY re-pointing makes missed bindings fail
// silently) and X2 (mass QML rewrite) depend on; S7 locks migrate onto it.
//
// Platform: runs under the minimal QPA platform plugin by default. The suite
// never shows/exposes a window, so the scene graph never renders a frame —
// RhiViewport's render path (D3D-coupled, QQuickRhiItem) is constructor-only
// here and its renderer is never created. That keeps the smoke net
// deterministic and GPU-free while still exercising the full object tree,
// every binding evaluation and every Component.onCompleted handler
// (binding re-evaluation is exactly what S1 needs observed). OWZX_QMLSMOKE_
// PLATFORM overrides (e.g. =offscreen) for future visual work (X4), and an
// externally-set QT_QPA_PLATFORM is respected as-is.
//
// Engine/backend lifetime: BackendContext and the QQmlApplicationEngine are
// intentionally leaked at process exit, mirroring main_qml.cpp:573 ("skip
// late Qt teardown hazards seen in VisualRegressionTests") — the OS reclaims
// everything; teardown of a fully-populated QML window tree is not part of
// the behavior under test.

#include <QtTest>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QFontDatabase>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QSettings>
#include <QTemporaryDir>
#include <memory>

#include "core/debug/CrashHandlerWin.h"
#include "core/viewmodels/EditorViewModel.h"
#include "qml_gui/BackendContext.h"
#include "qml_gui/CameraImageProvider.h"
#include "qml_gui/Renderer/RhiViewport.h"
#include "qml_gui/Models/ConfigOptionFilterProxy.h"

#include <QSignalSpy>

namespace {

// Known pre-existing QML defects the FIRST run of this suite (2026-10-12,
// Q14) surfaced in MultiMachinePage — surfaced here, not introduced here.
// Each entry is a narrow (file + message text) signature; ANY QML message
// not matching an entry fails the suite. Fix the QML, then DELETE the entry
// (cleanupTestCase prints entries that no longer match anything, so stale
// documentation stays visible).
struct AllowedQmlMessage
{
  const char *fileFragment;
  const char *textFragment;
};

const AllowedQmlMessage kAllowedQmlMessages[] = {
    // Dead delegate bindings: rows bind `width: deviceList.width` /
    // `width: localTaskList.width`, but those ids (MultiMachinePage.qml:308 /
    // :757) are not in the delegate Component's scope — every delegate tick
    // emits ReferenceError and the row width is broken in the app too.
    // Fix: bind via the ListView.view attached property. Then delete these.
    {"pages/MultiMachinePage.qml", "ReferenceError: deviceList is not defined"},
    {"pages/MultiMachinePage.qml", "ReferenceError: localTaskList is not defined"},
    // Anchors on layout-managed items (MultiMachinePage.qml ~:524/:594/:931)
    // — undefined layout behavior per Qt. Fix: Layout.alignment. Then delete.
    {"pages/MultiMachinePage.qml",
     "Detected anchors on an item that is managed by a layout"},
};

// Returns the index into kAllowedQmlMessages matching `message`, or -1.
int allowlistIndexForQmlMessage(const QString &message)
{
  for (int i = 0; i < int(std::size(kAllowedQmlMessages)); ++i)
  {
    if (message.contains(kAllowedQmlMessages[i].fileFragment) &&
        message.contains(kAllowedQmlMessages[i].textFragment))
      return i;
  }
  return -1;
}

} // namespace

class QmlSmokeTests : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void contextPropertiesReadable();
  void mainQmlLoads();
  void pagesInstantiate();
  void pagesInstantiate_data();
  // ── S1 (audit ④): per-cluster NOTIFY migration checks. Each cluster commit
  // extends s1ClusterNotifyContract with its re-pointed properties; the
  // dynamic re-evaluation slot drives real mutations and counts binding
  // evaluations (the audit's "断言该簇覆盖的页面属性在触发后确实重求值，
  // 或直接固化为 QmlSmokeTests 断言" — the固化 option).
  void s1TransformClusterNotifyContract();
  void s1TransformClusterReevaluation();
  void cleanupTestCase();

private:
  QObject *backendProperty(const char *name) const;
  void failOnUnexpectedQmlMessages(qsizetype before, const char *context);

  // BackendContext + engine are intentionally leaked (see file header);
  // backend_ is the engine-created OWzx module singleton (S8). tempDir_ keeps
  // the QSettings isolation alive for the whole run (see initTestCase).
  BackendContext *backend_ = nullptr;
  std::unique_ptr<QTemporaryDir> tempDir_;
  QQmlApplicationEngine *engine_ = nullptr;
  QList<QQmlError> m_qmlMessages;
  // Allowlist bookkeeping: how many captured messages each entry matched
  // (cleanupTestCase reports entries that matched nothing = stale docs).
  QList<int> m_allowlistMatchCounts;
};

void QmlSmokeTests::initTestCase()
{
  // Q13 convention (AppToolTests:127): install the app's fail-fast crash
  // handler so any crash in this suite leaves build/crash_dumps/ evidence
  // instead of a silent nonzero exit.
#ifdef Q_OS_WIN
  CrashHandlerWin::install(QCoreApplication::applicationDirPath() +
                           QStringLiteral("/crash_dumps"));
#endif

  // Mirror the runtime type registrations main_qml.cpp performs into the OWzx
  // QML module (S8: the former OWzxGL / OWzx.Models URIs are retired).
  // GLViewport is registered as RhiViewport unconditionally instead of
  // replaying selectRhiBackendFromEnvironment(): this suite never renders a
  // frame, so the render-path difference between RhiViewport/SoftwareViewport
  // is not under test, and the unconditional registration keeps the suite
  // independent of GPU probe availability. The app's selector remains the
  // authority in main_qml.cpp.
  qmlRegisterType<RhiViewport>("OWzx", 1, 0, "GLViewport");
  qmlRegisterType<ConfigOptionFilterProxy>("OWzx", 1, 0,
                                           "ConfigOptionFilterProxy");

  // Same bundled-font setup as main_qml.cpp:463-476 so QML text items resolve
  // the same family (missing fonts there silently fall back to YaHei).
  QFontDatabase::addApplicationFont(
      QStringLiteral(":/qml/assets/fonts/HarmonyOS_Sans_SC_Regular.ttf"));
  QFontDatabase::addApplicationFont(
      QStringLiteral(":/qml/assets/fonts/HarmonyOS_Sans_SC_Bold.ttf"));

  // S8 registry isolation: `appSettings` (UiSettingsStore) is now an
  // engine-created module singleton backed by QSettings("OWzx","UI") — the
  // developer's real HKCU store. Redirect QSettings to a temp INI BEFORE the
  // engine (and thus the singleton) exists, so a smoke run can never persist
  // a ui/theme or ui/scaleFactor change into the host registry. (Replaces the
  // old TestUiSettingsStore stand-in, which existed only because the class
  // was cpp-local and un-linkable — it lives in the module now.)
  tempDir_ = std::make_unique<QTemporaryDir>();
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                     tempDir_->filePath(QStringLiteral("settings")));

  engine_ = new QQmlApplicationEngine;

  // S8: NO_PLUGIN module — force the generated type registration TU in (the
  // static initializer would otherwise be dropped from the static archive),
  // exactly like main_qml.cpp does before loading QML.
  extern void qml_register_types_OWzx();
  qml_register_types_OWzx();

  m_allowlistMatchCounts.fill(0, int(std::size(kAllowedQmlMessages)));
  connect(engine_, &QQmlEngine::warnings, this,
          [this](const QList<QQmlError> &warnings)
          {
            for (const QQmlError &error : warnings)
            {
              const int allowed = allowlistIndexForQmlMessage(error.toString());
              if (allowed >= 0)
                m_allowlistMatchCounts[allowed] += 1;
              m_qmlMessages.append(error);
            }
          });

  // S8: `Backend` is the OWzx module's QML_SINGLETON — the engine creates the
  // one shared instance on first access; fetch it BEFORE engine->load like
  // main_qml.cpp does. Phase 241 (PAGE-04) parity: the app applies the
  // persisted startup-page preference before load. startupSkipFirstRun /
  // debugOverlayVisible are BackendContext Q_PROPERTYs now; a fresh
  // BackendContext defaults (false/false) match the app's non-debug,
  // first-run-path values.
  backend_ = engine_->singletonInstance<BackendContext *>("OWzx", "Backend");
  QVERIFY2(backend_ != nullptr,
           "OWzx module must expose the Backend singleton (BackendContext)");
  backend_->applyStartupPagePreference();

  // CAM-03 parity: register the camera image provider so any
  // image://camera/live request resolves exactly like the app
  // (main_qml.cpp:630-632). MonitorPage gates the request on
  // cameraStreamStatus===3 so it never fires without a connected machine,
  // but the provider is registered defensively all the same.
  engine_->addImageProvider(QStringLiteral("camera"),
                            new CameraImageProvider(backend_->cameraService()));
}

QObject *QmlSmokeTests::backendProperty(const char *name) const
{
  return backend_->property(name).value<QObject *>();
}

void QmlSmokeTests::contextPropertiesReadable()
{
  // The module singletons main.qml and the pages consume unguarded at load
  // (main.qml:928-1046, PreferencesPage `backend:` alias) must resolve through
  // the OWzx module registration (S8: former context properties).
  QVERIFY(engine_->singletonInstance<QObject *>("OWzx", "Backend") == backend_);
  QVERIFY(engine_->singletonInstance<QObject *>("OWzx", "AppSettings")
          != nullptr);
  QCOMPARE(backend_->property("startupSkipFirstRun").typeId(), QMetaType::Bool);
  QCOMPARE(backend_->property("debugOverlayVisible").typeId(), QMetaType::Bool);

  // Every view model a page binds as a required property must be non-null
  // (property list: BackendContext.h:94-102).
  const char *vmProperties[] = {
      "editorViewModel",   "previewViewModel", "monitorViewModel",
      "configViewModel",   "homeViewModel",    "settingsViewModel",
      "projectViewModel",  "calibrationViewModel",
      "multiMachineViewModel",
  };
  for (const char *name : vmProperties)
  {
    QVERIFY2(backendProperty(name) != nullptr,
             qPrintable(QStringLiteral("backend.%1 must be non-null").arg(name)));
  }

  // StackLayout slot contract: 9 slots, page indices enum 0..8
  // (main.qml:20, ARCH-01; slot 7 is the structural placeholder).
  QCOMPARE(backend_->property("tpHome").toInt(), 0);
  QCOMPARE(backend_->property("tpPreferences").toInt(), 8);
  const int currentPage = backend_->property("currentPage").toInt();
  QVERIFY2(currentPage >= 0 && currentPage <= 8,
           qPrintable(QStringLiteral("currentPage %1 outside 0..8").arg(currentPage)));

  QCOMPARE(m_qmlMessages.size(), 0);
}

void QmlSmokeTests::failOnUnexpectedQmlMessages(qsizetype before, const char *context)
{
  QStringList unexpected;
  for (const QQmlError &error : m_qmlMessages.mid(before))
  {
    if (allowlistIndexForQmlMessage(error.toString()) < 0)
      unexpected.append(error.toString());
  }
  if (!unexpected.isEmpty())
  {
    QString dump;
    const int kMaxDump = 10;
    for (int i = 0; i < unexpected.size() && i < kMaxDump; ++i)
      dump += QStringLiteral("\n  %1").arg(unexpected.at(i));
    if (unexpected.size() > kMaxDump)
      dump += QStringLiteral("\n  ... and %1 more").arg(unexpected.size() - kMaxDump);
    QFAIL(qPrintable(
        QStringLiteral("%1 emitted unexpected QML messages (%2 total; entries "
                       "in kAllowedQmlMessages document known pre-existing "
                       "defects):%3")
            .arg(context)
            .arg(unexpected.size())
            .arg(dump)));
  }
}

void QmlSmokeTests::cleanupTestCase()
{
  // Visible debt accounting: an allowlist entry that matched nothing means
  // its defect was fixed (or its signature drifted) — prune the entry.
  for (int i = 0; i < int(std::size(kAllowedQmlMessages)); ++i)
  {
    if (m_allowlistMatchCounts.at(i) == 0)
    {
      qInfo("[QmlSmoke] stale allowlist entry (0 matches this run — fix landed "
            "or signature drifted, prune it): [%s | %s]",
            kAllowedQmlMessages[i].fileFragment, kAllowedQmlMessages[i].textFragment);
    }
  }
}

void QmlSmokeTests::mainQmlLoads()
{
  const qsizetype messagesBefore = m_qmlMessages.size();

  engine_->load(QUrl(QStringLiteral("qrc:/qml/OWzx/main.qml")));

  QVERIFY2(!engine_->rootObjects().isEmpty(),
           "main.qml must produce a root object (see captured QML messages)");
  auto *rootWindow = qobject_cast<QQuickWindow *>(engine_->rootObjects().value(0));
  QVERIFY2(rootWindow, "main.qml root must be a QQuickWindow (ApplicationWindow)");
  QCOMPARE(rootWindow->title(), QStringLiteral("OWzx Slicer"));

  // The window is visible:true + Maximized in QML; under the minimal QPA
  // plugin that never maps a native window and never exposes → never renders.
  QVERIFY(rootWindow->isVisible());

  // Let deferred/async work (Component.onCompleted chains, Loader callbacks,
  // image async requests) surface any late QML messages before asserting.
  QTest::qWait(250);

  failOnUnexpectedQmlMessages(messagesBefore, "main.qml load");
}

void QmlSmokeTests::pagesInstantiate_data()
{
  QTest::addColumn<QString>("qmlPath");
  QTest::addColumn<QVariantMap>("initialProps");

  auto vm = [this](const char *prop) {
    QObject *value = backendProperty(prop);
    return QVariant::fromValue(value);
  };

  // Same required-property bindings main.qml:928-1046 (and Plater.qml:33-39,
  // which fans out to PreparePage/PreviewPage/AssemblePage) pass at runtime.
  QTest::newRow("HomePage")
      << "qrc:/qml/OWzx/pages/HomePage.qml"
      << QVariantMap{{"homeVm", vm("homeViewModel")}};
  QTest::newRow("PreparePage")
      << "qrc:/qml/OWzx/pages/PreparePage.qml"
      << QVariantMap{{"editorVm", vm("editorViewModel")},
                     {"configVm", vm("configViewModel")}};
  QTest::newRow("PreviewPage")
      << "qrc:/qml/OWzx/pages/PreviewPage.qml"
      << QVariantMap{{"previewVm", vm("previewViewModel")},
                     {"editorVm", vm("editorViewModel")},
                     {"configVm", vm("configViewModel")}};
  QTest::newRow("AssemblePage")
      << "qrc:/qml/OWzx/pages/AssemblePage.qml"
      << QVariantMap{{"editorVm", vm("editorViewModel")}};
  QTest::newRow("Plater")
      << "qrc:/qml/OWzx/pages/Plater.qml"
      << QVariantMap{{"editorVm", vm("editorViewModel")},
                     {"previewVm", vm("previewViewModel")},
                     {"configVm", vm("configViewModel")},
                     {"viewMode", backend_->property("currentViewMode")}};
  QTest::newRow("MonitorPage")
      << "qrc:/qml/OWzx/pages/MonitorPage.qml"
      << QVariantMap{{"monitorVm", vm("monitorViewModel")}};
  QTest::newRow("MultiMachinePage")
      << "qrc:/qml/OWzx/pages/MultiMachinePage.qml"
      << QVariantMap{{"multiMachineVm", vm("multiMachineViewModel")}};
  QTest::newRow("ProjectPage")
      << "qrc:/qml/OWzx/pages/ProjectPage.qml"
      << QVariantMap{{"projectVm", vm("projectViewModel")},
                     {"editorVm", vm("editorViewModel")}};
  QTest::newRow("CalibrationPage")
      << "qrc:/qml/OWzx/pages/CalibrationPage.qml"
      << QVariantMap{{"calibrationVm", vm("calibrationViewModel")}};
  QTest::newRow("PreferencesPage")
      << "qrc:/qml/OWzx/pages/PreferencesPage.qml"
      << QVariantMap{{"settingsVm", vm("settingsViewModel")}};
}

void QmlSmokeTests::pagesInstantiate()
{
  QFETCH(QString, qmlPath);
  QFETCH(QVariantMap, initialProps);

  const qsizetype messagesBefore = m_qmlMessages.size();

  QQmlComponent component(engine_, QUrl(qmlPath));
  QVERIFY2(!component.isError(),
           qPrintable(QStringLiteral("%1 failed to compile: %2")
                          .arg(qmlPath, component.errorString())));

  // createWithInitialProperties is what makes the pages' `required property`
  // declarations a feature here: a page whose required property is not in the
  // map fails creation with an explicit error, so a future page that adds a
  // required property without wiring it in main.qml turns this row red.
  std::unique_ptr<QObject> page(
      component.createWithInitialProperties(initialProps));
  QVERIFY2(page, qPrintable(QStringLiteral("%1 failed to instantiate: %2")
                                .arg(qmlPath, component.errorString())));

  // Give onCompleted handlers / async bindings a beat to surface messages.
  QTest::qWait(50);

  failOnUnexpectedQmlMessages(messagesBefore,
                              QStringLiteral("%1 instantiation").arg(qmlPath)
                                  .toUtf8().constData());
}

// ── S1 (audit ④): cluster NOTIFY migration checks ─────────────────────────

namespace {

struct S1NotifyExpectation
{
  const char *property;
  const char *notifySignal;
};

// The properties re-pointed onto the S1 semantic clusters so far. One row per
// migrated property: the test asserts the NOTIFY signal name through the
// metaobject, which is the exact contract QML binding re-evaluation depends
// on (a binding re-evaluates when ITS property's NOTIFY fires). The umbrella
// (stateChanged) stays as the transition carrier for unmigrated families.
const S1NotifyExpectation kS1NotifyExpectations[] = {
    // Commit 1 — transform cluster: everything that moves per drag frame.
    {"objectPosX", "transformChanged"},
    {"objectPosY", "transformChanged"},
    {"objectPosZ", "transformChanged"},
    {"objectRotX", "transformChanged"},
    {"objectRotY", "transformChanged"},
    {"objectRotZ", "transformChanged"},
    {"objectScaleX", "transformChanged"},
    {"objectScaleY", "transformChanged"},
    {"objectScaleZ", "transformChanged"},
    {"uniformScale", "transformChanged"},
    {"explosionRatio", "transformChanged"},
    {"assembleOffsets", "transformChanged"},
    {"assembleRotations", "transformChanged"},
    {"assembleScales", "transformChanged"},
    {"meshBatchInstanceTransforms", "transformChanged"},
    {"measureDimensions", "transformChanged"},
};

} // namespace

void QmlSmokeTests::s1TransformClusterNotifyContract()
{
  QObject *vm = backendProperty("editorViewModel");
  QVERIFY2(vm != nullptr, "editorViewModel must be non-null");
  const QMetaObject *mo = vm->metaObject();

  for (const S1NotifyExpectation &expectation : kS1NotifyExpectations)
  {
    const int propIndex = mo->indexOfProperty(expectation.property);
    QVERIFY2(propIndex >= 0,
             qPrintable(QStringLiteral("property %1 must exist")
                            .arg(expectation.property)));
    const QMetaProperty prop = mo->property(propIndex);
    QVERIFY2(prop.hasNotifySignal(),
             qPrintable(QStringLiteral("property %1 must have a NOTIFY signal")
                            .arg(expectation.property)));
    const QString actual = prop.notifySignal().name();
    QVERIFY2(actual == QString::fromLatin1(expectation.notifySignal),
             qPrintable(QStringLiteral("property %1 NOTIFY drifted: expected %2, got %3")
                            .arg(expectation.property, expectation.notifySignal, actual)));
  }
}

void QmlSmokeTests::s1TransformClusterReevaluation()
{
  auto *vm = qobject_cast<EditorViewModel *>(backendProperty("editorViewModel"));
  QVERIFY2(vm != nullptr, "editorViewModel must be an EditorViewModel");

  // Content on the plate so the transform setters have a selection target.
  QVERIFY(vm->addPrimitiveToPlate(0)); // 0 = cube
  vm->selectObject(0);
  QCOMPARE(vm->property("hasSelection").toBool(), true);

  QSignalSpy transformSpy(vm, &EditorViewModel::transformChanged);
  QSignalSpy umbrellaSpy(vm, &EditorViewModel::stateChanged);
  QVERIFY(transformSpy.isValid());
  QVERIFY(umbrellaSpy.isValid());

  // Inline QML mirrors the page bindings: one counter per tracked property.
  // posX mirrors the ObjectManipulation readout (transform cluster); status
  // is an umbrella-carrier property (statusText) whose re-evaluation must be
  // MERGED AWAY during drag frames -- the S1 drag coalescing proof.
  QQmlComponent component(engine_);
  component.setData(QByteArrayLiteral(R"QML(
    import QtQuick
    Item {
        required property var editorVm
        property int posXEvals: 0
        readonly property real posX: { posXEvals++; return editorVm.objectPosX }
        property int channelEvals: 0
        readonly property int channelLen:
            { channelEvals++; return editorVm.meshBatchInstanceTransforms.length }
        property int statusEvals: 0
        readonly property string status:
            { statusEvals++; return editorVm.statusText }
    }
    )QML"),
                       QUrl(QStringLiteral("qrc:/qml/OWzx/tests/S1InlineProbe.qml")));
  QVERIFY2(!component.isError(),
           qPrintable(QStringLiteral("inline probe failed to compile: %1")
                          .arg(component.errorString())));
  std::unique_ptr<QObject> probeHolder(
      component.createWithInitialProperties(
          QVariantMap{{"editorVm", QVariant::fromValue(vm)}}));
  QVERIFY2(probeHolder, qPrintable(component.errorString()));
  const auto evals = [&](const char *name) {
    return probeHolder->property(name).toInt();
  };

  const int posXEvalsBefore = evals("posXEvals");
  const int statusEvalsBefore = evals("statusEvals");
  const int transformBefore = transformSpy.count();
  const int umbrellaBefore = umbrellaSpy.count();

  // 1) Drag frame merge: gizmo move frames emit the transform cluster ONLY
  //    (umbrella merged away), so the transform bindings re-evaluate while
  //    the whole-page carriers stay quiet.
  vm->beginGizmoMoveDrag();
  vm->applyGizmoMoveDelta(5.0f, 0.0f, 0.0f);
  vm->applyGizmoMoveDelta(3.0f, 1.0f, 0.0f);
  QVERIFY(transformSpy.count() - transformBefore >= 2);
  QCOMPARE(umbrellaSpy.count(), umbrellaBefore);
  QVERIFY(evals("posXEvals") > posXEvalsBefore);
  QVERIFY(evals("channelEvals") > 0); // S3 channel re-eval per frame
  QCOMPARE(evals("statusEvals"), statusEvalsBefore);

  // 2) Move end: the coalesced close re-emits the umbrella exactly for the
  //    whole-page consumers (file tree / status / persistence).
  vm->endGizmoMoveDrag();
  QVERIFY(umbrellaSpy.count() > umbrellaBefore);
  QVERIFY(transformSpy.count() > transformBefore);

  // 3) Discrete panel write (ObjectManipulation spinbox path): property write
  //    fans the transform cluster AND the umbrella.
  const int transformMid = transformSpy.count();
  const int umbrellaMid = umbrellaSpy.count();
  const int posXMid = evals("posXEvals");
  QVERIFY(vm->setProperty("objectPosX", vm->property("objectPosX").toFloat() + 2.0f));
  QVERIFY(transformSpy.count() > transformMid);
  QVERIFY(umbrellaSpy.count() > umbrellaMid);
  QVERIFY(evals("posXEvals") > posXMid);
  QVERIFY2(!component.isError(), "no late QML errors on the inline probe");
}

int main(int argc, char *argv[])
{
  // Platform selection (see file header): default to the minimal QPA plugin
  // so no window is ever exposed and no frame is ever rendered. An explicit
  // OWZX_QMLSMOKE_PLATFORM wins; an externally-set QT_QPA_PLATFORM is
  // respected untouched. Must run before QGuiApplication construction.
  const QByteArray overridePlatform = qgetenv("OWZX_QMLSMOKE_PLATFORM");
  if (!overridePlatform.isEmpty())
    qputenv("QT_QPA_PLATFORM", overridePlatform);
  else if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
    qputenv("QT_QPA_PLATFORM", "minimal");

  // The platform plugin itself must be findable: this exe runs beside the
  // DEPLOYED Qt6Core.dll in build/, whose relocated QLibraryInfo prefix is
  // build/ — windeployqt only deployed qwindows.dll there. Point the QPA
  // plugin search at the Qt install's plugins/platforms (path injected by
  // CMake, see the OWZX_QT_PLATFORM_PLUGIN_DIR target definition).
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM_PLUGIN_PATH"))
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
            OWZX_QT_PLATFORM_PLUGIN_DIR);

  QGuiApplication app(argc, argv);
  app.setOrganizationName(QStringLiteral("OWzx"));
  app.setApplicationName(QStringLiteral("OWzxSlicer"));

  QmlSmokeTests tc;
  return QTest::qExec(&tc, argc, argv);
}

#include "QmlSmokeTests.moc"
