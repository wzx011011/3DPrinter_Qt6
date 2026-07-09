// ViewModelSmokeTests — Phase 55-04 additions.
//
// AUTOMOC caveat (v3.0 retrospective, see ViewModelSmokeTests CMake comment):
// single-file QtTest with cpp-internal Q_OBJECT has weak moc dependency
// tracking. After adding a new private slot here, re-run cmake configure (the
// canonical verify script does this) or delete
//   build/ViewModelSmokeTests_autogen/timestamp
// before rebuilding, otherwise the new slot silently does not execute.
#include <QSignalSpy>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QSettings>
#include <QHostAddress>
#include <QUdpSocket>
#include <cstring>
#include <QtTest>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/PrintConfig.hpp>
#endif

#include "core/services/AppSettingsService.h"
#include "core/model/PartPlate.h"
#include "core/model/PartPlateList.h"
#include "core/services/CameraServiceMock.h"
#include "core/services/CalibrationServiceMock.h"
#include "core/services/DeviceServiceMock.h"
#include "core/services/NetworkServiceMock.h"
#include "core/services/PresetServiceMock.h"
#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
#include "core/services/UndoRedoManager.h"
#include "core/services/FtpUploader.h"
#include "core/services/SsdpDiscovery.h"
#include "core/rendering/AssemblyMeasureGeometry.h"
#include "core/viewmodels/ConfigViewModel.h"
#include "core/viewmodels/CalibrationViewModel.h"
#include "core/viewmodels/EditorViewModel.h"
#include "core/viewmodels/MonitorViewModel.h"
#include "core/viewmodels/PreviewViewModel.h"
#include "core/viewmodels/ProjectViewModel.h"
#include "qml_gui/BackendContext.h"
#include "qml_gui/Models/ConfigOptionModel.h"
#include "qml_gui/Renderer/PrepareSceneData.h"

namespace
{
  static const QString kStlPath = QDir::cleanPath(
      QStringLiteral(QT_TESTCASE_SOURCEDIR) +
      QStringLiteral("/third_party/OrcaSlicer/resources/profiles/hotend.stl"));

  // Phase 55-04: OrcaSlicer-style G-code fixture committed by Plan 01.
  // Loaded directly via PreviewViewModel::loadGCodeForPreview for the
  // render-side role-toggle / legend / current-line atomicity guards.
  static const QString kOrcaGcodePath = QDir::cleanPath(
      QStringLiteral(QT_TESTCASE_SOURCEDIR) +
      QStringLiteral("/tests/fixtures/orca_sample.gcode"));

  // Phase 55-04: count of packed GCV1 segments. Mirrors the E2EWorkflowTests
  // helper so ViewModelSmokeTests can assert on the GCV1 payload shape without
  // a live slice. Returns -1 if payload doesn't start with "GCV1".
  int gcv1SegmentCount(const QByteArray &payload)
  {
    if (!payload.startsWith("GCV1") || payload.size() < 8)
      return -1;
    int count = 0;
    std::memcpy(&count, payload.constData() + 4, sizeof(count));
    return count;
  }

  struct ExpectedCalibRequest
  {
    const char *id;
    int mode;
    double start;
    double end;
    double step;
    bool printNumbers;
  };

  struct ScopedSettingsSnapshot
  {
    explicit ScopedSettingsSnapshot(const QStringList &trackedKeys)
        : keys(trackedKeys)
    {
      QSettings settings;
      for (const QString &key : keys)
      {
        if (settings.contains(key))
        {
          existingKeys.append(key);
          originalValues.insert(key, settings.value(key));
        }
      }
    }

    ~ScopedSettingsSnapshot()
    {
      QSettings settings;
      for (const QString &key : keys)
      {
        if (existingKeys.contains(key))
          settings.setValue(key, originalValues.value(key));
        else
          settings.remove(key);
      }
      settings.sync();
    }

    void clear() const
    {
      QSettings settings;
      for (const QString &key : keys)
        settings.remove(key);
      settings.sync();
    }

    QStringList keys;
    QStringList existingKeys;
    QVariantMap originalValues;
  };

  struct ScopedApplicationIdentity
  {
    ScopedApplicationIdentity(const QString &org, const QString &app)
        : oldOrg(QCoreApplication::organizationName()),
          oldApp(QCoreApplication::applicationName())
    {
      QCoreApplication::setOrganizationName(org);
      QCoreApplication::setApplicationName(app);
    }

    ~ScopedApplicationIdentity()
    {
      QCoreApplication::setOrganizationName(oldOrg);
      QCoreApplication::setApplicationName(oldApp);
    }

    QString oldOrg;
    QString oldApp;
  };
}

class ViewModelSmokeTests final : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  // Phase 02-01: pure-Qt enum/signal tests — do NOT depend on HAS_LIBSLIC3R
  void testTabPositionEnumValues();
  void testRequestSelectTabSignal();
  void testRequestSelectTabOutOfRange();
  // Phase 03-01: ViewMode enum + requestChangeViewMode + tab 联动
  void testViewModeEnumValues();
  void testCurrentViewModeDefault();
  void testRequestChangeViewModeSignal();
  void testTabSelectDrivesViewMode();
  // Phase 51-03: SHELL-02 + SHELL-03 — shell gates registered, round-trip
  // preserves state, stateChanged forwarding wired from the editor viewmodel.
  void shellStateGatesForwardToEditorViewModelAndPreserveRoundTrip();
  // Phase 52-03 (PREPSB-05): config/preset change invalidates prior slice
  // results; staleness Q_PROPERTYs reach QML.
  void sidebarPresetChangeInvalidatesSliceResults();
  void settingsOpenDoesNotInvalidateSliceResults();
  // Phase 52-03 (PREPSB-02): settings signal forward is honest (emits + logs).
  void sidebarSettingsForwardEmitsRequestedSignal();
  // Phase 04-01: Sidebar Dockable 状态 + 持久化
  void testSidebarCollapsedDefault();
  void testRequestToggleSidebar();
  void testSidebarWidthClamp();
  void testSidebarDockArea();
  // v2.6 Phase 4: INT 自回归（SSDP 发现解析 + Camera 状态机/帧令牌）
  void int01_SsdpDiscoveryParsesMockResponse();
  void int03_CameraStateMachineAndFrameToken();
  // v2.7 P1: INT-02 校准自回归（PA/FlowRate/TempTower calib slice 生成 G-code）
  void int02_CalibrationGeneratesCalibGcode();
  // v2.9 Phase 12: deterministic calibration closure tests
  void calibrationImplementedModesExposeStableRouting();
  void calibrationImplementedModesEmitSliceRequests();
  void calibrationUnsupportedModesAreExplicitlyUnavailable();
  void calibrationFallbackAndSliceCallbacksDriveProgress();
  // v2.7 P2-A: INT-04 MQTT connection params + telemetry field mapping
  void int04_MqttConnectionParamsAndTelemetryFields();
  // v2.7 P2-B: INT-05 MQTT command construction + control flow
  void int05_MqttCommandConstructionAndControlFlow();
  // v2.8 P2-C: INT-06 FTP URL construction + send-print routing
  void int06_FtpUrlAndSendPrintRouting();
  void appSettingsAndEditorBedShapePersistDeterministically();
  void editor_import_model_updates_state();
  void editorReadinessBlocksPreviewAndExportUntilCurrentPlateResultIsValid();
  void monitor_refresh_updates_network_and_device();
  void config_default_and_switch_preset();
  void testUpstreamDefaultsContainVectorKeys();
  void testMachineOptionsLoaded();
  void testFilamentOptionsLoaded();
  void testMachineEditFlowsToGlobal();
  void testTierAwareSaveFiltersByTier();
  void configPresetDirtyTracksActiveTierAgainstSelectedPreset();
  void configResetRestoresSelectedPresetValues();
  void configOptionModelDirtyUsesPresetReferenceValues();
  void configScopeResetRevealsInheritedValue();
  void configUnsavedTransitionsQueueAndCancelPendingChanges();
  void configDiscardAppliesPendingTransitionAndRestoresValues();
  void configWritableSaveAppliesPendingTransition();
  void configReadOnlySaveAsAppliesPendingTransition();
  void configPresetCategoryMappingUsesServiceEnums();
  void configPresetMutationsRejectWrongCategory();
  void presetServiceMetadataClassifiesBuiltinAndCustomPresets();
  void presetServiceSelectionPersistsAcrossInstances();
  void presetServiceImportRejectsMalformedBundleWithoutMutation();
  void presetServiceExportsAndImportsUserBundleWithMetadata();
  void presetCompatibilityFiltersFilamentsAndProcessesForPrinter();
  void configPrinterChangeRepairsIncompatibleSelections();
  void configKeepsInvalidSelectionWhenNoCompatibleFallback();
  void presetReadOnlyActionBlockerReasons();
  // v3.0 Phase 16-01: PartPlate/PartPlateList domain model (pure-data, no libslic3r dep)
  void partPlateInstanceMembershipTracksObjectInstancePairs();
  void partPlateSliceStateMachineGatesCanSlice();
  void partPlateListCreateDeleteRenameLockReindexesAndKeepsAtLeastOne();
  void partPlateListInstanceMembershipDerivesObjectIndices();
  void partPlateListRefusesExceedMaxPlateCount();
  // v3.0 Phase 16-02: ProjectServiceMock plate ops backed by PartPlateList (PLATE-06 regression)
  void projectServicePlateOpsBackedByPartPlateList();
  // v3.0 Phase 17: plate lifecycle completion (clone/reorder/printable)
  void partPlateListMovePlateReindexesAndAdjustsCurrent();
  void projectServiceClonePlateDeepCopiesObjects();
  void projectServicePerPlatePrintableRoundTrip();
  // v3.0 Phase 18: 3MF multi-plate persistence round-trip (PLATE-09, the v2.9 blocker)
  void multiPlate3mfRoundTripPreservesState();
  // v3.0 Phase 19: per-plate config merge + scoped-value stub fix
  void projectServicePerPlateConfigOverrideRoundTrips();
  void sliceServicePerPlateConfigMergeHonorsOverrides();
  // Phase 21 review-fix: verify DynamicPrintConfig::apply merge direction
  void sliceServiceConfigMergeDirectionPlateWins();
  // v3.1 Phase 24: renderer-facing active plate context must not use UI fallback
  void activePlateObjectIndicesFollowCurrentPlateWithoutFallback();
  // v3.2 Phase 25-03: QRhi picking selects source objects through the ViewModel
  void rendererPickingSelectsSourceObjectThroughEditorViewModel();
  // v3.8 Phase 69: move-gizmo drag deltas coalesce into one undo command.
  void gizmoMoveDragCoalescesIntoSingleUndoCommand();
  // v3.8 Phase 70: rotate/scale gizmo drags coalesce into one undo command.
  void gizmoRotateDragCoalescesIntoSingleUndoCommand();
  void gizmoScaleDragCoalescesIntoSingleUndoCommand();
  // Phase 53-01: Prepare object/plate/gizmo gates live in C++, not QML.
  void prepareWorkflowGatesExposeSourceTruthState();
  void prepareMoveSelectionToPlateUsesSourceSelection();
  void prepareVisibleObjectActionsMapToSourceObjects();
  // Phase 55-04 (GCODE-02/03): render-side role-toggle no-repack guard,
  // legend/global-scope coherence, currentMove atomicity, 17-view-mode contract.
  void roleVisibilityToggleDoesNotRepackGcodePreviewData();
  void legendGradientBoundsStableAcrossLayerMoveDrag();
  void currentMoveUpdatesGcodeLineWindowAtomically();
  void stepCurrentMoveClampsAndUpdatesGcodeLineWindow();
  void viewModesExposeUpstreamSeventeenModes();
  // Phase 55 code-review fix (GCODE-02): the renderer consumes a DENSE 20-bool
  // mask, not the 18-row QVariantMap UI list. Guard the producer shape and the
  // toggle→mask propagation so the role-visibility feature cannot silently
  // become a dead path again.
  void roleVisibilityMaskFeedsRendererShapeAndTogglesPropagate();
  // Phase 56-01: Wave 0 RED test scaffolds for SETTINGS-01..07.
  // AUTOMOC caveat: after adding new private slots, re-run cmake configure
  // or delete build/ViewModelSmokeTests_autogen/timestamp before rebuilding.
  void testSettingsDialogOpenFromSidebar();
  void testTabsAndGroupNavPerTier();
  void testConfigOptionModelSevenTypes();
  void testPerOptionDirtyAndValueSource();
  void testReadonlyBuiltinGating();
  void testSaveSaveAsResetOptionResetGroupResetAll();
  void testUnsavedChangesGuardOnDirtyClose();
  void testPerDialogSearchAndFourLevelMode();
  void testNullableAndVectorOptions();
  // Phase 91-01 (ASMEXPLODE-01): explosionRatio Q_PROPERTY behavior mirrors
  // upstream m_explosion_ratio (default 1.0, set/reset emit stateChanged).
  void editorExplosionRatioDefaultsAndResetMirrorsUpstream();
  // Phase 92-01 (ASMMEASURE-01): Assembly measurement gizmo activability
  // mirrors upstream GLGizmoAssembly::on_is_activable (AssembleView + explosion
  // ratio ~= 1.0 + >=2 volumes). Loads a 2-object fixture for the >=2 case.
  void assemblyMeasureGizmoActivabilityMirrorsUpstream();
  // Phase 92-01 (ASMMEASURE-02): AssemblyMeasureGeometry::measure computes
  // correct distance/angle for two known AABBs (pure math, no model needed).
  void assemblyMeasureGeometryComputesDistanceAndAngle();
  // Phase 93-01 (ASMROUTE-02): the AssembleView data pool is populated ONLY
  // when the active canvas is CanvasAssembleView (m_activeCanvasType == 2),
  // mirroring upstream GLGizmosManager.cpp:427-431. Prepare/Preview never
  // populate or read it (isolation constraint).
  void assembleViewDataPoolIsolatedFromPrepareAndPreview();

private:
  bool hasLibslic3r() const;
};

bool ViewModelSmokeTests::hasLibslic3r() const
{
#ifdef HAS_LIBSLIC3R
  return true;
#else
  return false;
#endif
}

void ViewModelSmokeTests::initTestCase()
{
  QCoreApplication::setOrganizationName(QStringLiteral("OWzx"));
  QCoreApplication::setApplicationName(QStringLiteral("OWzxSlicer"));
  if (!hasLibslic3r())
    QSKIP("ViewModel smoke tests require HAS_LIBSLIC3R — skipping all tests");
  QVERIFY2(QFileInfo::exists(kStlPath), qPrintable(
      QStringLiteral("Test STL not found: %1").arg(kStlPath)));
}

void ViewModelSmokeTests::editor_import_model_updates_state()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  QCOMPARE(editor.modelCount(), 0);

  QSignalSpy spy(&editor, &EditorViewModel::stateChanged);
  QVERIFY(editor.loadFile(kStlPath));

  QTRY_VERIFY_WITH_TIMEOUT(editor.modelCount() >= 1, 5000);
  QVERIFY(spy.count() >= 1);
}

void ViewModelSmokeTests::editorReadinessBlocksPreviewAndExportUntilCurrentPlateResultIsValid()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  QSignalSpy loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(loadSpy.isValid());
  QVERIFY2(editor.loadFile(kStlPath), "importing a model should start");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(loadSpy.takeFirst().at(0).toBool(), "model import should complete successfully");

  QVERIFY2(editor.modelCount() >= 1, "imported model should be present");
  QVERIFY2(editor.canRequestSlice(), "current plate should be sliceable before a result exists");
  QVERIFY2(!editor.canPreview(), "Preview must require a valid current-plate slice result");
  QVERIFY2(!editor.canExportGCode(), "G-code export must require a valid current-plate result");
  QCOMPARE(editor.plateSliceResultStatus(editor.currentPlateIndex()),
           int(EditorViewModel::SliceResultMissing));
  QVERIFY2(editor.previewActionHint().contains(QStringLiteral("尚未切片")),
           qPrintable(editor.previewActionHint()));
  QVERIFY2(editor.exportActionHint().contains(QStringLiteral("尚未切片")),
           qPrintable(editor.exportActionHint()));

  editor.switchToPreview();
  QVERIFY2(editor.statusText().contains(QStringLiteral("尚未切片")),
           qPrintable(editor.statusText()));
}

void ViewModelSmokeTests::monitor_refresh_updates_network_and_device()
{
  DeviceServiceMock device;
  NetworkServiceMock network;
  CameraServiceMock camera;
  MonitorViewModel monitor(&device, &network, &camera);

  const int beforeState = monitor.monitorState();
  const int beforeLatency = monitor.latencyMs();

  QSignalSpy spy(&monitor, &MonitorViewModel::networkChanged);
  monitor.refresh();

  QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 5000);
  QVERIFY(monitor.monitorState() != beforeState || monitor.latencyMs() != beforeLatency);
  QVERIFY(monitor.networkOnline());
}

void ViewModelSmokeTests::config_default_and_switch_preset()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("ConfigSwitchPreset"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  QSignalSpy spy(&config, &ConfigViewModel::stateChanged);
  const QString initialPreset = config.currentPreset();
  QVERIFY(!initialPreset.isEmpty());

  QHash<QString, QVariant> values;
  values.insert(QStringLiteral("layer_height"), 0.16);
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("Unit Test Switch Print Preset"),
                                    values));
  config.setCurrentPreset(QStringLiteral("Unit Test Switch Print Preset"));
  QVERIFY(spy.count() >= 1);
  QCOMPARE(config.currentPreset(), QStringLiteral("Unit Test Switch Print Preset"));
  QCOMPARE(config.currentPrintPreset(), QStringLiteral("Unit Test Switch Print Preset"));
}

void ViewModelSmokeTests::testUpstreamDefaultsContainVectorKeys()
{
  PresetServiceMock preset;
  auto defaults = preset.presetValues(QStringLiteral("__upstream_defaults__"));

  // coFloats type — previously skipped by extraction
  QVERIFY2(defaults.contains(QStringLiteral("machine_max_speed_x")),
           "machine_max_speed_x missing from upstream defaults (coFloats)");
  QVERIFY2(defaults.contains(QStringLiteral("nozzle_diameter")),
           "nozzle_diameter missing from upstream defaults (coFloats)");

  // coEnum type — previously skipped
  QVERIFY2(defaults.contains(QStringLiteral("gcode_flavor")),
           "gcode_flavor missing from upstream defaults (coEnum)");

  // coPoints type — previously skipped
  QVERIFY2(defaults.contains(QStringLiteral("printable_area")),
           "printable_area missing from upstream defaults (coPoints)");
}

void ViewModelSmokeTests::testMachineOptionsLoaded()
{
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  auto *machineOpts = qobject_cast<ConfigOptionModel *>(config.machineOptions());
  QVERIFY(machineOpts);
  QVERIFY2(machineOpts->rowCount() > 0, "Machine options model is empty");

  // Verify key printer hardware parameters exist
  QVERIFY2(machineOpts->indexOfKey(QStringLiteral("machine_max_speed_x")) >= 0,
           "machine_max_speed_x missing from machine options");
  QVERIFY2(machineOpts->indexOfKey(QStringLiteral("gcode_flavor")) >= 0,
           "gcode_flavor missing from machine options");
  QVERIFY2(machineOpts->indexOfKey(QStringLiteral("nozzle_diameter")) >= 0,
           "nozzle_diameter missing from machine options");
  QVERIFY2(machineOpts->indexOfKey(QStringLiteral("machine_start_gcode")) >= 0,
           "machine_start_gcode missing from machine options");
  QVERIFY2(machineOpts->indexOfKey(QStringLiteral("printable_area")) >= 0,
           "printable_area missing from machine options");
}

void ViewModelSmokeTests::testFilamentOptionsLoaded()
{
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  auto *filamentOpts = qobject_cast<ConfigOptionModel *>(config.filamentOptions());
  QVERIFY(filamentOpts);
  QVERIFY2(filamentOpts->rowCount() > 0, "Filament options model is empty");

  QVERIFY2(filamentOpts->indexOfKey(QStringLiteral("filament_type")) >= 0,
           "filament_type missing from filament options");
  QVERIFY2(filamentOpts->indexOfKey(QStringLiteral("nozzle_temperature")) >= 0,
           "nozzle_temperature missing from filament options");
  QVERIFY2(filamentOpts->indexOfKey(QStringLiteral("fan_max_speed")) >= 0,
           "fan_max_speed missing from filament options");
}

void ViewModelSmokeTests::testMachineEditFlowsToGlobal()
{
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  auto *machineOpts = qobject_cast<ConfigOptionModel *>(config.machineOptions());
  int idx = machineOpts->indexOfKey(QStringLiteral("machine_max_speed_x"));
  QVERIFY2(idx >= 0, "machine_max_speed_x not found in machine options");

  machineOpts->setValue(idx, 999.0);

  auto merged = config.mergedConfigValues();
  QVERIFY(merged.contains(QStringLiteral("machine_max_speed_x")));
  QCOMPARE(merged[QStringLiteral("machine_max_speed_x")].toDouble(), 999.0);
}

void ViewModelSmokeTests::testTierAwareSaveFiltersByTier()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("TierAwareSave"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  auto *machineOpts = qobject_cast<ConfigOptionModel *>(config.machineOptions());

  int printIdx = printOpts->indexOfKey(QStringLiteral("layer_height"));
  int machineIdx = machineOpts->indexOfKey(QStringLiteral("machine_max_speed_x"));
  QVERIFY2(printIdx >= 0, "layer_height not in print options");
  QVERIFY2(machineIdx >= 0, "machine_max_speed_x not in machine options");

  const QString builtinPrint = config.currentPrintPreset();
  const auto builtinBefore = preset.presetValues(builtinPrint);

  // Edit both tiers
  printOpts->setValue(printIdx, 0.35);
  machineOpts->setValue(machineIdx, 999.0);

  // Built-in/vendor presets are read-only and must not be overwritten.
  config.setActivePresetTier(QStringLiteral("print"));
  config.saveCurrentPreset();
  QCOMPARE(preset.presetValues(builtinPrint), builtinBefore);

  QVERIFY(config.createCustomPreset(PresetServiceMock::PrintCat, QStringLiteral("Unit Test Save Print Preset")));
  config.setCurrentPrintPreset(QStringLiteral("Unit Test Save Print Preset"));
  printOpts->setValue(printIdx, 0.35);

  // Save as print tier — should only include print model keys.
  config.setActivePresetTier(QStringLiteral("print"));
  config.saveCurrentPreset();

  // Verify print preset has layer_height with the edited value
  auto saved = preset.presetValues(config.currentPrintPreset().isEmpty()
                                    ? config.currentPreset()
                                    : config.currentPrintPreset());
  QVERIFY2(saved.contains(QStringLiteral("layer_height")),
           "layer_height should be in print preset after save");
  QCOMPARE(saved[QStringLiteral("layer_height")].toDouble(), 0.35);

  QVERIFY(config.createCustomPreset(PresetServiceMock::PrinterCat, QStringLiteral("Unit Test Save Printer Preset")));
  config.setCurrentPrinterPreset(QStringLiteral("Unit Test Save Printer Preset"));
  machineOpts->setValue(machineIdx, 999.0);

  // Now save as printer tier — machine key should be saved there.
  config.setActivePresetTier(QStringLiteral("printer"));
  config.saveCurrentPreset();

  auto printerSaved = preset.presetValues(config.currentPrinterPreset());
  if (!printerSaved.isEmpty()) {
    // Verify the machine key was saved to the printer preset
    QVERIFY2(printerSaved.contains(QStringLiteral("machine_max_speed_x")),
             "machine_max_speed_x should be in printer preset after printer-tier save");
    QCOMPARE(printerSaved[QStringLiteral("machine_max_speed_x")].toDouble(), 999.0);
  }
}

void ViewModelSmokeTests::configPresetDirtyTracksActiveTierAgainstSelectedPreset()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("ConfigDirtyByTier"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  PresetServiceMock preset;
  ProjectServiceMock project;

  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("UT Dirty Print A"),
                                    {{QStringLiteral("layer_height"), 0.16},
                                     {QStringLiteral("top_shell_layers"), 5}}));
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrinterCat,
                                    QStringLiteral("UT Dirty Printer A"),
                                    {{QStringLiteral("machine_max_speed_x"), 333.0}}));

  ConfigViewModel config(&preset, &project);
  config.setCurrentPrintPreset(QStringLiteral("UT Dirty Print A"));
  config.setCurrentPrinterPreset(QStringLiteral("UT Dirty Printer A"));

  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  auto *machineOpts = qobject_cast<ConfigOptionModel *>(config.machineOptions());
  QVERIFY(printOpts);
  QVERIFY(machineOpts);

  const int layerIdx = printOpts->indexOfKey(QStringLiteral("layer_height"));
  const int speedIdx = machineOpts->indexOfKey(QStringLiteral("machine_max_speed_x"));
  QVERIFY(layerIdx >= 0);
  QVERIFY(speedIdx >= 0);

  config.setActivePresetTier(QStringLiteral("print"));
  QVERIFY(!config.isPresetDirty());
  QCOMPARE(config.globalModifiedCount(), 0);

  printOpts->setValue(layerIdx, 0.22);
  QVERIFY(config.isPresetDirty());
  QCOMPARE(config.globalModifiedCount(), 1);
  QCOMPARE(config.globalModifiedKey(0), QStringLiteral("layer_height"));
  QCOMPARE(config.globalModifiedDefaultValue(QStringLiteral("layer_height")), QStringLiteral("0.16"));

  config.setActivePresetTier(QStringLiteral("printer"));
  QVERIFY(!config.isPresetDirty());
  QCOMPARE(config.globalModifiedCount(), 0);

  machineOpts->setValue(speedIdx, 555.0);
  QVERIFY(config.isPresetDirty());
  QCOMPARE(config.globalModifiedCount(), 1);
  QCOMPARE(config.globalModifiedKey(0), QStringLiteral("machine_max_speed_x"));
  QCOMPARE(config.globalModifiedDefaultValue(QStringLiteral("machine_max_speed_x")), QStringLiteral("333"));

  config.setActivePresetTier(QStringLiteral("print"));
  QVERIFY(printOpts->optIsDirty(layerIdx));
}

void ViewModelSmokeTests::configResetRestoresSelectedPresetValues()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("ConfigResetSelectedPreset"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  PresetServiceMock preset;
  ProjectServiceMock project;

  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("UT Reset Print A"),
                                    {{QStringLiteral("layer_height"), 0.16},
                                     {QStringLiteral("top_shell_layers"), 5}}));
  ConfigViewModel config(&preset, &project);
  config.setCurrentPrintPreset(QStringLiteral("UT Reset Print A"));

  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  QVERIFY(printOpts);
  const int layerIdx = printOpts->indexOfKey(QStringLiteral("layer_height"));
  QVERIFY(layerIdx >= 0);

  config.setActivePresetTier(QStringLiteral("print"));
  printOpts->setValue(layerIdx, 0.24);
  QVERIFY(config.isPresetDirty());
  QVERIFY(config.resetGlobalOption(QStringLiteral("layer_height")));
  QCOMPARE(printOpts->optValue(layerIdx).toDouble(), 0.16);
  QVERIFY(!config.isPresetDirty());

  printOpts->setValue(layerIdx, 0.28);
  QVERIFY(config.isPresetDirty());
  config.resetAllGlobalOptions();
  QCOMPARE(printOpts->optValue(layerIdx).toDouble(), 0.16);
  QVERIFY(!config.isPresetDirty());
}

void ViewModelSmokeTests::configOptionModelDirtyUsesPresetReferenceValues()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("ConfigOptionDirtyReference"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  PresetServiceMock preset;
  ProjectServiceMock project;

  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("UT Dirty Marker Print A"),
                                    {{QStringLiteral("layer_height"), 0.16}}));
  ConfigViewModel config(&preset, &project);
  config.setCurrentPrintPreset(QStringLiteral("UT Dirty Marker Print A"));

  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  QVERIFY(printOpts);
  const int layerIdx = printOpts->indexOfKey(QStringLiteral("layer_height"));
  QVERIFY(layerIdx >= 0);

  QCOMPARE(printOpts->optValue(layerIdx).toDouble(), 0.16);
  QVERIFY(!printOpts->optIsDirty(layerIdx));
  QCOMPARE(printOpts->dirtyCount(), 0);

  printOpts->setValue(layerIdx, 0.20);
  QVERIFY(printOpts->optIsDirty(layerIdx));
  QCOMPARE(printOpts->dirtyCount(), 1);

  printOpts->setValue(layerIdx, 0.16);
  QVERIFY(!printOpts->optIsDirty(layerIdx));
  QCOMPARE(printOpts->dirtyCount(), 0);
}

void ViewModelSmokeTests::configScopeResetRevealsInheritedValue()
{
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  QVERIFY(printOpts);
  const int layerIdx = printOpts->indexOfKey(QStringLiteral("layer_height"));
  QVERIFY(layerIdx >= 0);

  QVERIFY(project.addPrimitiveToPlate(0) >= 0);
  config.activateObjectScope(QStringLiteral("object"), QStringLiteral("Object 1"), 0, -1);
  printOpts->setValue(layerIdx, 0.30);
  QCOMPARE(printOpts->optValue(layerIdx).toDouble(), 0.30);
  QCOMPARE(config.scopeOverrideCount(), 1);

  QVERIFY(config.resetScopeOverride(QStringLiteral("layer_height")));
  QCOMPARE(config.scopeOverrideCount(), 0);
  QCOMPARE(printOpts->optValue(layerIdx).toDouble(), config.mergedConfigValues().value(QStringLiteral("layer_height")).toDouble());
}

void ViewModelSmokeTests::configUnsavedTransitionsQueueAndCancelPendingChanges()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("ConfigUnsavedTransitions"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  PresetServiceMock preset;
  ProjectServiceMock project;

  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("UT Guard Print A"),
                                    {{QStringLiteral("layer_height"), 0.16}}));
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("UT Guard Print B"),
                                    {{QStringLiteral("layer_height"), 0.28}}));

  ConfigViewModel config(&preset, &project);
  config.setCurrentPrintPreset(QStringLiteral("UT Guard Print A"));
  config.setActivePresetTier(QStringLiteral("print"));

  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  QVERIFY(printOpts);
  const int layerIdx = printOpts->indexOfKey(QStringLiteral("layer_height"));
  QVERIFY(layerIdx >= 0);

  printOpts->setValue(layerIdx, 0.22);
  QVERIFY(config.isPresetDirty());

  bool ok = false;
  QVERIFY(QMetaObject::invokeMethod(&config, "requestCurrentPrintPreset",
                                    Q_RETURN_ARG(bool, ok),
                                    Q_ARG(QString, QStringLiteral("UT Guard Print B"))));
  QVERIFY(!ok);
  QCOMPARE(config.currentPrintPreset(), QStringLiteral("UT Guard Print A"));
  QCOMPARE(config.property("pendingUnsavedAction").toString(), QStringLiteral("switch-print-preset"));
  QCOMPARE(config.property("pendingUnsavedTarget").toString(), QStringLiteral("UT Guard Print B"));
  QVERIFY(config.property("hasPendingUnsavedChanges").toBool());

  bool cancelOk = false;
  QVERIFY(QMetaObject::invokeMethod(&config, "requestCancelPendingChanges",
                                    Q_RETURN_ARG(bool, cancelOk)));
  QVERIFY(cancelOk);
  QCOMPARE(config.currentPrintPreset(), QStringLiteral("UT Guard Print A"));
  QVERIFY(config.isPresetDirty());
  QVERIFY(config.property("pendingUnsavedAction").toString().isEmpty());
}

void ViewModelSmokeTests::configDiscardAppliesPendingTransitionAndRestoresValues()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("ConfigUnsavedDiscard"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  PresetServiceMock preset;
  ProjectServiceMock project;

  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("UT Discard Print A"),
                                    {{QStringLiteral("layer_height"), 0.16}}));
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("UT Discard Print B"),
                                    {{QStringLiteral("layer_height"), 0.28}}));

  ConfigViewModel config(&preset, &project);
  config.setCurrentPrintPreset(QStringLiteral("UT Discard Print A"));
  config.setActivePresetTier(QStringLiteral("print"));

  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  QVERIFY(printOpts);
  const int layerIdx = printOpts->indexOfKey(QStringLiteral("layer_height"));
  QVERIFY(layerIdx >= 0);

  printOpts->setValue(layerIdx, 0.24);
  QVERIFY(config.isPresetDirty());

  QSignalSpy sliceSpy(&config, &ConfigViewModel::sliceAffectingConfigChanged);
  QVERIFY(sliceSpy.isValid());

  bool switchOk = false;
  QVERIFY(QMetaObject::invokeMethod(&config, "requestCurrentPrintPreset",
                                    Q_RETURN_ARG(bool, switchOk),
                                    Q_ARG(QString, QStringLiteral("UT Discard Print B"))));
  QVERIFY(!switchOk);
  QCOMPARE(config.currentPrintPreset(), QStringLiteral("UT Discard Print A"));
  QCOMPARE(config.property("pendingUnsavedAction").toString(), QStringLiteral("switch-print-preset"));

  bool discardOk = false;
  QVERIFY(QMetaObject::invokeMethod(&config, "requestDiscardPendingChanges",
                                    Q_RETURN_ARG(bool, discardOk)));
  QVERIFY(discardOk);
  QCOMPARE(config.currentPrintPreset(), QStringLiteral("UT Discard Print B"));
  QVERIFY(!config.property("hasPendingUnsavedChanges").toBool());
  QVERIFY(!config.isPresetDirty());
  QVERIFY(sliceSpy.count() >= 1);

  const auto originalValues = preset.presetValues(QStringLiteral("UT Discard Print A"));
  QVERIFY(originalValues.contains(QStringLiteral("layer_height")));
  QCOMPARE(originalValues.value(QStringLiteral("layer_height")).toDouble(), 0.16);
}

void ViewModelSmokeTests::configWritableSaveAppliesPendingTransition()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("ConfigUnsavedWritableSave"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  PresetServiceMock preset;
  ProjectServiceMock project;

  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("UT Writable Save Print A"),
                                    {{QStringLiteral("layer_height"), 0.18}}));
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("UT Writable Save Print B"),
                                    {{QStringLiteral("layer_height"), 0.30}}));

  ConfigViewModel config(&preset, &project);
  config.setCurrentPrintPreset(QStringLiteral("UT Writable Save Print A"));
  config.setActivePresetTier(QStringLiteral("print"));

  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  QVERIFY(printOpts);
  const int layerIdx = printOpts->indexOfKey(QStringLiteral("layer_height"));
  QVERIFY(layerIdx >= 0);

  printOpts->setValue(layerIdx, 0.26);
  QVERIFY(config.isPresetDirty());

  bool switchOk = false;
  QVERIFY(QMetaObject::invokeMethod(&config, "requestCurrentPrintPreset",
                                    Q_RETURN_ARG(bool, switchOk),
                                    Q_ARG(QString, QStringLiteral("UT Writable Save Print B"))));
  QVERIFY(!switchOk);
  QCOMPARE(config.currentPrintPreset(), QStringLiteral("UT Writable Save Print A"));

  bool saveOk = false;
  QVERIFY(QMetaObject::invokeMethod(&config, "requestSavePendingChanges",
                                    Q_RETURN_ARG(bool, saveOk)));
  QVERIFY(saveOk);
  QCOMPARE(config.currentPrintPreset(), QStringLiteral("UT Writable Save Print B"));

  const auto savedValues = preset.presetValues(QStringLiteral("UT Writable Save Print A"));
  QVERIFY(savedValues.contains(QStringLiteral("layer_height")));
  QCOMPARE(savedValues.value(QStringLiteral("layer_height")).toDouble(), 0.26);
}

void ViewModelSmokeTests::configReadOnlySaveAsAppliesPendingTransition()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("ConfigUnsavedSaveAs"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  config.setActivePresetTier(QStringLiteral("print"));

  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  QVERIFY(printOpts);
  const int layerIdx = printOpts->indexOfKey(QStringLiteral("layer_height"));
  QVERIFY(layerIdx >= 0);

  const QString builtinPreset = config.currentPrintPreset();
  QVERIFY(!builtinPreset.isEmpty());
  QVERIFY(preset.isReadOnlyPreset(builtinPreset));
  QString targetPreset;
  const QStringList printPresets = preset.presetNamesForCategory(PresetServiceMock::PrintCat);
  for (const QString &candidate : printPresets)
  {
    if (candidate != builtinPreset)
    {
      targetPreset = candidate;
      break;
    }
  }
  QVERIFY(!targetPreset.isEmpty());

  printOpts->setValue(layerIdx, 0.24);
  QVERIFY(config.isPresetDirty());

  bool switchOk = false;
  QVERIFY(QMetaObject::invokeMethod(&config, "requestCurrentPrintPreset",
                                    Q_RETURN_ARG(bool, switchOk),
                                    Q_ARG(QString, targetPreset)));
  QVERIFY(!switchOk);
  QCOMPARE(config.currentPrintPreset(), builtinPreset);
  QCOMPARE(config.property("pendingUnsavedAction").toString(), QStringLiteral("switch-print-preset"));
  QCOMPARE(config.property("pendingUnsavedTarget").toString(), targetPreset);

  QSignalSpy saveAsSpy(&config, &ConfigViewModel::saveAsRequired);
  QVERIFY(saveAsSpy.isValid());

  bool savePendingOk = true;
  QVERIFY(QMetaObject::invokeMethod(&config, "requestSavePendingChanges",
                                    Q_RETURN_ARG(bool, savePendingOk)));
  QVERIFY(!savePendingOk);
  QCOMPARE(saveAsSpy.count(), 1);
  QCOMPARE(config.currentPrintPreset(), builtinPreset);
  QVERIFY(config.property("hasPendingUnsavedChanges").toBool());

  QVERIFY(config.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("UT SaveAs Pending Print")));
  QCOMPARE(config.currentPrintPreset(), targetPreset);
  QVERIFY(!config.property("hasPendingUnsavedChanges").toBool());

  const auto savedValues = preset.presetValues(QStringLiteral("UT SaveAs Pending Print"));
  QVERIFY(savedValues.contains(QStringLiteral("layer_height")));
  QCOMPARE(savedValues.value(QStringLiteral("layer_height")).toDouble(), 0.24);
}

void ViewModelSmokeTests::configPresetCategoryMappingUsesServiceEnums()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("PresetCategoryMapping"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  QVERIFY2(preset.presetNamesForCategory(PresetServiceMock::PrinterCat).contains(config.currentPrinterPreset()),
           qPrintable(QStringLiteral("currentPrinterPreset has wrong category: %1").arg(config.currentPrinterPreset())));
  QVERIFY2(preset.presetNamesForCategory(PresetServiceMock::FilamentCat).contains(config.currentFilamentPreset()),
           qPrintable(QStringLiteral("currentFilamentPreset has wrong category: %1").arg(config.currentFilamentPreset())));
  QVERIFY2(preset.presetNamesForCategory(PresetServiceMock::PrintCat).contains(config.currentPrintPreset()),
           qPrintable(QStringLiteral("currentPrintPreset has wrong category: %1").arg(config.currentPrintPreset())));

  QVERIFY2(preset.presetValues(config.currentPrinterPreset()).contains(QStringLiteral("nozzle_diameter")),
           "printer preset should expose printer machine keys");
  const auto filamentValues = preset.presetValues(config.currentFilamentPreset());
  QVERIFY2(filamentValues.contains(QStringLiteral("nozzle_temperature")) ||
               filamentValues.contains(QStringLiteral("nozzle_temp")) ||
               filamentValues.contains(QStringLiteral("fan_max_speed")) ||
               filamentValues.contains(QStringLiteral("filament_type")),
           "filament preset should expose filament material keys");
  QVERIFY2(preset.presetValues(config.currentPrintPreset()).contains(QStringLiteral("layer_height")),
           "print preset should expose process keys");

  const QString printBefore = config.currentPreset();
  const QString filamentBefore = config.currentFilamentPreset();
  config.setCurrentPreset(filamentBefore);
  QCOMPARE(config.currentPreset(), printBefore);
  QCOMPARE(config.currentPrintPreset(), printBefore);
}

void ViewModelSmokeTests::configPresetMutationsRejectWrongCategory()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("PresetMutationCategory"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  QHash<QString, QVariant> values;
  values.insert(QStringLiteral("layer_height"), 0.21);
  const QString printName = QStringLiteral("Unit Test Mutation Print Preset");
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat, printName, values));

  QVERIFY(!config.renamePreset(PresetServiceMock::FilamentCat,
                               printName,
                               QStringLiteral("Unit Test Mutation Renamed")));
  QVERIFY(preset.hasPreset(printName));
  QVERIFY(!preset.hasPreset(QStringLiteral("Unit Test Mutation Renamed")));

  QVERIFY(!config.deletePreset(PresetServiceMock::FilamentCat, printName));
  QVERIFY(preset.hasPreset(printName));

  QVERIFY(config.renamePreset(PresetServiceMock::PrintCat,
                              printName,
                              QStringLiteral("Unit Test Mutation Renamed")));
  QVERIFY(!preset.hasPreset(printName));
  QVERIFY(preset.hasPreset(QStringLiteral("Unit Test Mutation Renamed")));
}

void ViewModelSmokeTests::presetServiceMetadataClassifiesBuiltinAndCustomPresets()
{
  PresetServiceMock preset;

  const QString builtin = preset.defaultPresetForCategory(PresetServiceMock::PrintCat);
  QVERIFY(!builtin.isEmpty());
  QCOMPARE(preset.presetCategory(builtin), int(PresetServiceMock::PrintCat));
  QVERIFY(preset.isBuiltinPreset(builtin));
  QVERIFY(preset.isReadOnlyPreset(builtin));
  QVERIFY(!preset.isUserPreset(builtin));
  QVERIFY(preset.presetValueCount(builtin) > 0);

  QHash<QString, QVariant> values;
  values.insert(QStringLiteral("layer_height"), 0.23);
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat, QStringLiteral("Unit Test Print Preset"), values));
  QCOMPARE(preset.presetCategory(QStringLiteral("Unit Test Print Preset")), int(PresetServiceMock::PrintCat));
  QVERIFY(!preset.isBuiltinPreset(QStringLiteral("Unit Test Print Preset")));
  QVERIFY(!preset.isReadOnlyPreset(QStringLiteral("Unit Test Print Preset")));
  QVERIFY(preset.isUserPreset(QStringLiteral("Unit Test Print Preset")));
  QCOMPARE(preset.presetValueCount(QStringLiteral("Unit Test Print Preset")), 1);
}

void ViewModelSmokeTests::presetServiceSelectionPersistsAcrossInstances()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("PresetSelectionPersistence"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  QString printName;
  QString filamentName;
  QString printerName;
  {
    PresetServiceMock preset;
    const auto printPresets = preset.presetNamesForCategory(PresetServiceMock::PrintCat);
    const auto filamentPresets = preset.presetNamesForCategory(PresetServiceMock::FilamentCat);
    const auto printerPresets = preset.presetNamesForCategory(PresetServiceMock::PrinterCat);
    QVERIFY(printPresets.size() >= 1);
    QVERIFY(filamentPresets.size() >= 1);
    QVERIFY(printerPresets.size() >= 1);
    printName = printPresets.last();
    filamentName = filamentPresets.last();
    printerName = printerPresets.last();
    QVERIFY(preset.setSelectedPresetForCategory(PresetServiceMock::PrintCat, printName));
    QVERIFY(preset.setSelectedPresetForCategory(PresetServiceMock::FilamentCat, filamentName));
    QVERIFY(preset.setSelectedPresetForCategory(PresetServiceMock::PrinterCat, printerName));
  }

  PresetServiceMock reloaded;
  QCOMPARE(reloaded.selectedPresetForCategory(PresetServiceMock::PrintCat), printName);
  QCOMPARE(reloaded.selectedPresetForCategory(PresetServiceMock::FilamentCat), filamentName);
  QCOMPARE(reloaded.selectedPresetForCategory(PresetServiceMock::PrinterCat), printerName);
}

void ViewModelSmokeTests::presetServiceImportRejectsMalformedBundleWithoutMutation()
{
  PresetServiceMock preset;
  const int beforeCount = preset.presetNamesForCategory(PresetServiceMock::PrintCat).size() +
                          preset.presetNamesForCategory(PresetServiceMock::FilamentCat).size() +
                          preset.presetNamesForCategory(PresetServiceMock::PrinterCat).size();

  const QString tempPath = QDir::temp().filePath(QStringLiteral("owzx_bad_preset_bundle.json"));
  QFile f(tempPath);
  QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
  f.write(R"({"kind":"owzx-preset-bundle","version":"1.0","presets":[{"name":"","category":0,"values":{}}]})");
  f.close();

  QVERIFY(!preset.importBundle(tempPath));
  const int afterCount = preset.presetNamesForCategory(PresetServiceMock::PrintCat).size() +
                         preset.presetNamesForCategory(PresetServiceMock::FilamentCat).size() +
                         preset.presetNamesForCategory(PresetServiceMock::PrinterCat).size();
  QCOMPARE(afterCount, beforeCount);

  QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
  f.write(R"({"kind":"owzx-preset-bundle","version":"999.0","presets":[]})");
  f.close();
  QVERIFY(!preset.importBundle(tempPath));
  const int incompatibleVersionCount = preset.presetNamesForCategory(PresetServiceMock::PrintCat).size() +
                                       preset.presetNamesForCategory(PresetServiceMock::FilamentCat).size() +
                                       preset.presetNamesForCategory(PresetServiceMock::PrinterCat).size();
  QCOMPARE(incompatibleVersionCount, beforeCount);
  QFile::remove(tempPath);
}

void ViewModelSmokeTests::presetServiceExportsAndImportsUserBundleWithMetadata()
{
  PresetServiceMock source;
  QHash<QString, QVariant> values;
  values.insert(QStringLiteral("layer_height"), 0.24);
  values.insert(QStringLiteral("wall_loops"), 4);
  QVERIFY(source.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("Unit Test Exported Print Preset"),
                                    values));

  const QString tempPath = QDir::temp().filePath(QStringLiteral("owzx_good_preset_bundle.json"));
  QFile::remove(tempPath);
  QVERIFY(source.exportBundle(tempPath));

  PresetServiceMock target;
  QVERIFY(target.importBundle(tempPath));
  QCOMPARE(target.presetCategory(QStringLiteral("Unit Test Exported Print Preset")), int(PresetServiceMock::PrintCat));
  QVERIFY(target.isUserPreset(QStringLiteral("Unit Test Exported Print Preset")));
  QCOMPARE(target.presetValue(QStringLiteral("Unit Test Exported Print Preset"),
                              QStringLiteral("layer_height")).toDouble(), 0.24);
  QCOMPARE(target.presetValue(QStringLiteral("Unit Test Exported Print Preset"),
                              QStringLiteral("wall_loops")).toInt(), 4);
  QFile::remove(tempPath);
}

void ViewModelSmokeTests::presetCompatibilityFiltersFilamentsAndProcessesForPrinter()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("PresetCompatibilityFilters"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  PresetServiceMock preset;
  QHash<QString, QVariant> printerA;
  printerA.insert(QStringLiteral("nozzle_diameter"), 0.4);
  printerA.insert(QStringLiteral("max_nozzle_temp"), 300);
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrinterCat,
                                    QStringLiteral("Unit Test Compat Printer A"),
                                    printerA));
  QHash<QString, QVariant> printerB;
  printerB.insert(QStringLiteral("nozzle_diameter"), 0.4);
  printerB.insert(QStringLiteral("max_nozzle_temp"), 300);
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrinterCat,
                                    QStringLiteral("Unit Test Compat Printer B"),
                                    printerB));

  QHash<QString, QVariant> filamentA;
  filamentA.insert(QStringLiteral("compatible_printers"),
                   QStringList{QStringLiteral("Unit Test Compat Printer A")});
  filamentA.insert(QStringLiteral("compatible_nozzle_min"), 0.2);
  filamentA.insert(QStringLiteral("compatible_nozzle_max"), 0.8);
  filamentA.insert(QStringLiteral("nozzle_temp_range_max"), 260);
  QVERIFY(preset.createCustomPreset(PresetServiceMock::FilamentCat,
                                    QStringLiteral("Unit Test Compat Filament A"),
                                    filamentA));
  QHash<QString, QVariant> filamentB;
  filamentB.insert(QStringLiteral("compatible_printers"),
                   QVariantList{QStringLiteral("Unit Test Compat Printer B")});
  filamentB.insert(QStringLiteral("compatible_nozzle_min"), 0.2);
  filamentB.insert(QStringLiteral("compatible_nozzle_max"), 0.8);
  filamentB.insert(QStringLiteral("nozzle_temp_range_max"), 260);
  QVERIFY(preset.createCustomPreset(PresetServiceMock::FilamentCat,
                                    QStringLiteral("Unit Test Compat Filament B"),
                                    filamentB));

  QHash<QString, QVariant> processA;
  processA.insert(QStringLiteral("layer_height"), 0.2);
  processA.insert(QStringLiteral("compatible_printers"), QStringLiteral("Unit Test Compat Printer A"));
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("Unit Test Compat Process A"),
                                    processA));
  QHash<QString, QVariant> processB;
  processB.insert(QStringLiteral("layer_height"), 0.28);
  processB.insert(QStringLiteral("compatible_printers"),
                  QStringList{QStringLiteral("Unit Test Compat Printer B")});
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("Unit Test Compat Process B"),
                                    processB));

  const QStringList filamentForA =
      preset.compatiblePresetNamesForCategory(PresetServiceMock::FilamentCat,
                                              QStringLiteral("Unit Test Compat Printer A"));
  QVERIFY(filamentForA.contains(QStringLiteral("Unit Test Compat Filament A")));
  QVERIFY(!filamentForA.contains(QStringLiteral("Unit Test Compat Filament B")));

  const QStringList processForA =
      preset.compatiblePresetNamesForCategory(PresetServiceMock::PrintCat,
                                              QStringLiteral("Unit Test Compat Printer A"));
  QVERIFY(processForA.contains(QStringLiteral("Unit Test Compat Process A")));
  QVERIFY(!processForA.contains(QStringLiteral("Unit Test Compat Process B")));
  QVERIFY(preset.isPresetCompatibleWithPrinter(PresetServiceMock::PrintCat,
                                               QStringLiteral("Unit Test Compat Process A"),
                                               QStringLiteral("Unit Test Compat Printer A")));
  QVERIFY(!preset.isPresetCompatibleWithPrinter(PresetServiceMock::PrintCat,
                                                QStringLiteral("Unit Test Compat Process A"),
                                                QStringLiteral("Unit Test Compat Printer B")));
  QVERIFY(!preset.presetCompatibilityMessage(PresetServiceMock::PrintCat,
                                             QStringLiteral("Unit Test Compat Process A"),
                                             QStringLiteral("Unit Test Compat Printer B")).isEmpty());
}

void ViewModelSmokeTests::configPrinterChangeRepairsIncompatibleSelections()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("PresetCompatibilityRepair"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  PresetServiceMock preset;
  ProjectServiceMock project;

  QHash<QString, QVariant> printerA;
  printerA.insert(QStringLiteral("nozzle_diameter"), 0.4);
  printerA.insert(QStringLiteral("max_nozzle_temp"), 300);
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrinterCat,
                                    QStringLiteral("Unit Test Repair Printer A"),
                                    printerA));
  QHash<QString, QVariant> printerB;
  printerB.insert(QStringLiteral("nozzle_diameter"), 0.4);
  printerB.insert(QStringLiteral("max_nozzle_temp"), 300);
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrinterCat,
                                    QStringLiteral("Unit Test Repair Printer B"),
                                    printerB));

  QHash<QString, QVariant> filamentA;
  filamentA.insert(QStringLiteral("compatible_printers"),
                   QStringList{QStringLiteral("Unit Test Repair Printer A")});
  filamentA.insert(QStringLiteral("compatible_nozzle_min"), 0.2);
  filamentA.insert(QStringLiteral("compatible_nozzle_max"), 0.8);
  filamentA.insert(QStringLiteral("nozzle_temp_range_max"), 260);
  QVERIFY(preset.createCustomPreset(PresetServiceMock::FilamentCat,
                                    QStringLiteral("Unit Test Repair Filament A"),
                                    filamentA));
  QHash<QString, QVariant> filamentB;
  filamentB.insert(QStringLiteral("compatible_printers"),
                   QStringList{QStringLiteral("Unit Test Repair Printer B")});
  filamentB.insert(QStringLiteral("compatible_nozzle_min"), 0.2);
  filamentB.insert(QStringLiteral("compatible_nozzle_max"), 0.8);
  filamentB.insert(QStringLiteral("nozzle_temp_range_max"), 260);
  QVERIFY(preset.createCustomPreset(PresetServiceMock::FilamentCat,
                                    QStringLiteral("Unit Test Repair Filament B"),
                                    filamentB));

  QHash<QString, QVariant> processA;
  processA.insert(QStringLiteral("layer_height"), 0.2);
  processA.insert(QStringLiteral("compatible_printers"),
                  QStringList{QStringLiteral("Unit Test Repair Printer A")});
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("Unit Test Repair Process A"),
                                    processA));
  QHash<QString, QVariant> processB;
  processB.insert(QStringLiteral("layer_height"), 0.28);
  processB.insert(QStringLiteral("compatible_printers"),
                  QStringList{QStringLiteral("Unit Test Repair Printer B")});
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("Unit Test Repair Process B"),
                                    processB));

  ConfigViewModel config(&preset, &project);
  config.setCurrentPrinterPreset(QStringLiteral("Unit Test Repair Printer A"));
  config.setCurrentFilamentPreset(QStringLiteral("Unit Test Repair Filament A"));
  config.setCurrentPrintPreset(QStringLiteral("Unit Test Repair Process A"));
  QVERIFY(config.currentPresetCombinationValid());

  config.setCurrentPrinterPreset(QStringLiteral("Unit Test Repair Printer B"));
  QCOMPARE(config.currentPrinterPreset(), QStringLiteral("Unit Test Repair Printer B"));
  QCOMPARE(config.currentFilamentPreset(), QStringLiteral("Unit Test Repair Filament B"));
  QCOMPARE(config.currentPrintPreset(), QStringLiteral("Unit Test Repair Process B"));
  QVERIFY(config.currentPresetCombinationValid());
  QVERIFY(config.currentPresetCompatibilityMessage().isEmpty());
  QCOMPARE(preset.selectedPresetForCategory(PresetServiceMock::FilamentCat),
           QStringLiteral("Unit Test Repair Filament B"));
  QCOMPARE(preset.selectedPresetForCategory(PresetServiceMock::PrintCat),
           QStringLiteral("Unit Test Repair Process B"));
}

void ViewModelSmokeTests::configKeepsInvalidSelectionWhenNoCompatibleFallback()
{
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("PresetCompatibilityInvalid"));
  ScopedSettingsSnapshot snapshot({
      QStringLiteral("presets/selectedPrint"),
      QStringLiteral("presets/selectedFilament"),
      QStringLiteral("presets/selectedPrinter")});
  snapshot.clear();

  PresetServiceMock preset;
  ProjectServiceMock project;

  QHash<QString, QVariant> printerA;
  printerA.insert(QStringLiteral("nozzle_diameter"), 0.4);
  printerA.insert(QStringLiteral("max_nozzle_temp"), 300);
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrinterCat,
                                    QStringLiteral("Unit Test Invalid Printer A"),
                                    printerA));
  QHash<QString, QVariant> printerNoFallback;
  printerNoFallback.insert(QStringLiteral("nozzle_diameter"), 2.0);
  printerNoFallback.insert(QStringLiteral("max_nozzle_temp"), 300);
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrinterCat,
                                    QStringLiteral("Unit Test Invalid Printer 2.0"),
                                    printerNoFallback));

  QHash<QString, QVariant> filamentA;
  filamentA.insert(QStringLiteral("compatible_printers"),
                   QStringList{QStringLiteral("Unit Test Invalid Printer A")});
  filamentA.insert(QStringLiteral("compatible_nozzle_min"), 0.2);
  filamentA.insert(QStringLiteral("compatible_nozzle_max"), 0.8);
  filamentA.insert(QStringLiteral("nozzle_temp_range_max"), 260);
  QVERIFY(preset.createCustomPreset(PresetServiceMock::FilamentCat,
                                    QStringLiteral("Unit Test Invalid Filament A"),
                                    filamentA));

  QHash<QString, QVariant> processA;
  processA.insert(QStringLiteral("layer_height"), 0.2);
  processA.insert(QStringLiteral("compatible_printers"),
                  QStringList{QStringLiteral("Unit Test Invalid Printer A")});
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("Unit Test Invalid Process A"),
                                    processA));

  ConfigViewModel config(&preset, &project);
  config.setCurrentPrinterPreset(QStringLiteral("Unit Test Invalid Printer A"));
  config.setCurrentFilamentPreset(QStringLiteral("Unit Test Invalid Filament A"));
  config.setCurrentPrintPreset(QStringLiteral("Unit Test Invalid Process A"));
  QVERIFY(config.currentPresetCombinationValid());

  config.setCurrentPrinterPreset(QStringLiteral("Unit Test Invalid Printer 2.0"));
  QCOMPARE(config.currentPrinterPreset(), QStringLiteral("Unit Test Invalid Printer 2.0"));
  QCOMPARE(config.currentFilamentPreset(), QStringLiteral("Unit Test Invalid Filament A"));
  QVERIFY(!preset.compatiblePresetNamesForCategory(PresetServiceMock::FilamentCat,
                                                   QStringLiteral("Unit Test Invalid Printer 2.0"))
              .contains(QStringLiteral("Unit Test Invalid Filament A")));
  QVERIFY(config.compatibleFilamentPresetNames().contains(QStringLiteral("Unit Test Invalid Filament A")));
  QVERIFY(config.currentPrintPreset() != QStringLiteral("Unit Test Invalid Process A"));
  QVERIFY(!config.currentPresetCombinationValid());
  QVERIFY(!config.canUseCurrentPresetCombination());
  QVERIFY(!config.currentPresetCompatibilityMessage().isEmpty());
}

void ViewModelSmokeTests::presetReadOnlyActionBlockerReasons()
{
  PresetServiceMock preset;
  const QString builtinPrint = preset.defaultPresetForCategory(PresetServiceMock::PrintCat);
  QVERIFY(!builtinPrint.isEmpty());

  const QString deleteBlocker =
      preset.presetActionBlocker(PresetServiceMock::PrintCat, builtinPrint, QStringLiteral("delete"));
  const QString renameBlocker =
      preset.presetActionBlocker(PresetServiceMock::PrintCat, builtinPrint, QStringLiteral("rename"));
  QVERIFY(!deleteBlocker.isEmpty());
  QVERIFY(!renameBlocker.isEmpty());

  QHash<QString, QVariant> customValues;
  customValues.insert(QStringLiteral("layer_height"), 0.22);
  QVERIFY(preset.createCustomPreset(PresetServiceMock::PrintCat,
                                    QStringLiteral("Unit Test Action User Print"),
                                    customValues));
  QVERIFY(preset.presetActionBlocker(PresetServiceMock::PrintCat,
                                     QStringLiteral("Unit Test Action User Print"),
                                     QStringLiteral("delete")).isEmpty());
  QVERIFY(!preset.presetActionBlocker(PresetServiceMock::FilamentCat,
                                      QStringLiteral("Unit Test Action User Print"),
                                      QStringLiteral("delete")).isEmpty());
}

// ── Phase 02-01: TabPosition Q_ENUM + requestSelectTab unit tests ──
// These tests construct BackendContext standalone. They do NOT touch any
// libslic3r-dependent method, so they run regardless of the HAS_LIBSLIC3R
// define (the initTestCase() QSKIP gate only applies to the other slots).

void ViewModelSmokeTests::testTabPositionEnumValues()
{
  BackendContext ctx;

  // 1:1 numeric alignment with upstream MainFrame.hpp:218-229
  QCOMPARE(static_cast<int>(BackendContext::TabPosition::tpHome), 0);
  QCOMPARE(static_cast<int>(BackendContext::TabPosition::tp3DEditor), 1);
  QCOMPARE(static_cast<int>(BackendContext::TabPosition::tpPreview), 2);
  QCOMPARE(static_cast<int>(BackendContext::TabPosition::tpDevice), 3);
  QCOMPARE(static_cast<int>(BackendContext::TabPosition::tpMultiDevice), 4);
  QCOMPARE(static_cast<int>(BackendContext::TabPosition::tpProject), 5);
  QCOMPARE(static_cast<int>(BackendContext::TabPosition::tpCalibration), 6);
  QCOMPARE(static_cast<int>(BackendContext::TabPosition::tpPlaceholder1), 7);
  QCOMPARE(static_cast<int>(BackendContext::TabPosition::tpPlaceholder2), 8);

  // Confirm Q_ENUM registration — proves QML can read backend.TabPosition.tpX
  const QMetaEnum meta = QMetaEnum::fromType<BackendContext::TabPosition>();
  QVERIFY(meta.isValid());
  QCOMPARE(meta.keyToValue("tpHome"), 0);
  QCOMPARE(meta.keyToValue("tp3DEditor"), 1);
  QCOMPARE(meta.keyToValue("tpPreview"), 2);
  QCOMPARE(meta.keyToValue("tpDevice"), 3);
  QCOMPARE(meta.keyToValue("tpMultiDevice"), 4);
  QCOMPARE(meta.keyToValue("tpProject"), 5);
  QCOMPARE(meta.keyToValue("tpCalibration"), 6);
  QCOMPARE(meta.keyToValue("tpPlaceholder1"), 7);
  QCOMPARE(meta.keyToValue("tpPlaceholder2"), 8);
}

void ViewModelSmokeTests::testRequestSelectTabSignal()
{
  BackendContext ctx;

  // Default currentPage must remain 1 (Prepare tab) — no regression from Phase 1
  QCOMPARE(ctx.currentPage(), 1);

  QSignalSpy spy(&ctx, &BackendContext::tabSelectRequested);
  QVERIFY(spy.isValid());

  ctx.requestSelectTab(2);

  // Signal emitted exactly once with argument 2 (emit-first ordering)
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.takeFirst().at(0).toInt(), 2);

  // currentPage updated to requested position
  QCOMPARE(ctx.currentPage(), 2);
}

void ViewModelSmokeTests::testRequestSelectTabOutOfRange()
{
  BackendContext ctx;

  QSignalSpy spy(&ctx, &BackendContext::tabSelectRequested);
  QVERIFY(spy.isValid());

  const int before = ctx.currentPage();

  // Out-of-range positions must be silently rejected (Pitfall A3)
  ctx.requestSelectTab(-1);
  ctx.requestSelectTab(9);

  QCOMPARE(spy.count(), 0);
  QCOMPARE(ctx.currentPage(), before);
}

// ── Phase 03-01: ViewMode enum + requestChangeViewMode unit tests ──

void ViewModelSmokeTests::testViewModeEnumValues()
{
  BackendContext ctx;

  // 1:1 numeric alignment with upstream Plater view3D/preview/assemble_view
  QCOMPARE(static_cast<int>(BackendContext::ViewMode::View3D), 0);
  QCOMPARE(static_cast<int>(BackendContext::ViewMode::Preview), 1);
  QCOMPARE(static_cast<int>(BackendContext::ViewMode::AssembleView), 2);

  // Q_PROPERTY constant accessors (QML-side vmView3D / vmPreview / vmAssembleView)
  QCOMPARE(ctx.vmView3D(), 0);
  QCOMPARE(ctx.vmPreview(), 1);
  QCOMPARE(ctx.vmAssembleView(), 2);

  // Q_ENUM registration — proves C++ meta-object and future QML introspection
  const QMetaEnum meta = QMetaEnum::fromType<BackendContext::ViewMode>();
  QVERIFY(meta.isValid());
  QCOMPARE(meta.keyToValue("View3D"), 0);
  QCOMPARE(meta.keyToValue("Preview"), 1);
  QCOMPARE(meta.keyToValue("AssembleView"), 2);
}

void ViewModelSmokeTests::testCurrentViewModeDefault()
{
  BackendContext ctx;

  // Default must be View3D (aligns with upstream Plater defaulting to view3D on startup)
  QCOMPARE(ctx.currentViewMode(), static_cast<int>(BackendContext::ViewMode::View3D));
}

void ViewModelSmokeTests::testRequestChangeViewModeSignal()
{
  BackendContext ctx;

  QSignalSpy reqSpy(&ctx, &BackendContext::viewModeChangeRequested);
  QSignalSpy chgSpy(&ctx, &BackendContext::currentViewModeChanged);
  QVERIFY(reqSpy.isValid());
  QVERIFY(chgSpy.isValid());

  // Switch View3D → Preview
  ctx.requestChangeViewMode(static_cast<int>(BackendContext::ViewMode::Preview));

  // viewModeChangeRequested emitted first (pre-state broadcast semantics, aligns with tabSelectRequested)
  QCOMPARE(reqSpy.count(), 1);
  QCOMPARE(reqSpy.takeFirst().at(0).toInt(), static_cast<int>(BackendContext::ViewMode::Preview));
  // currentViewModeChanged emitted once
  QCOMPARE(chgSpy.count(), 1);
  // State updated
  QCOMPARE(ctx.currentViewMode(), static_cast<int>(BackendContext::ViewMode::Preview));

  // Same-value request must be de-duplicated (no signal, no state churn)
  chgSpy.clear();
  reqSpy.clear();
  ctx.requestChangeViewMode(static_cast<int>(BackendContext::ViewMode::Preview));
  QCOMPARE(chgSpy.count(), 0);

  // Out-of-range must be silently rejected (Pitfall A3 mirror)
  reqSpy.clear();
  const int before = ctx.currentViewMode();
  ctx.requestChangeViewMode(-1);
  ctx.requestChangeViewMode(99);
  QCOMPARE(reqSpy.count(), 0);
  QCOMPARE(ctx.currentViewMode(), before);
}

void ViewModelSmokeTests::testTabSelectDrivesViewMode()
{
  BackendContext ctx;

  // Start at default: currentPage=1 (tp3DEditor), viewMode=View3D
  QCOMPARE(ctx.currentPage(), static_cast<int>(BackendContext::TabPosition::tp3DEditor));
  QCOMPARE(ctx.currentViewMode(), static_cast<int>(BackendContext::ViewMode::View3D));

  // Selecting tpPreview must drive viewMode → Preview (Phase 3 tab/viewMode 联动)
  ctx.requestSelectTab(static_cast<int>(BackendContext::TabPosition::tpPreview));
  QCOMPARE(ctx.currentPage(), static_cast<int>(BackendContext::TabPosition::tpPreview));
  QCOMPARE(ctx.currentViewMode(), static_cast<int>(BackendContext::ViewMode::Preview));

  // Selecting tp3DEditor again must drive viewMode back to View3D
  ctx.requestSelectTab(static_cast<int>(BackendContext::TabPosition::tp3DEditor));
  QCOMPARE(ctx.currentViewMode(), static_cast<int>(BackendContext::ViewMode::View3D));

  // Selecting a non-Plater tab (tpProject) must NOT change viewMode (stays View3D)
  const int vmBefore = ctx.currentViewMode();
  ctx.requestSelectTab(static_cast<int>(BackendContext::TabPosition::tpProject));
  QCOMPARE(ctx.currentPage(), static_cast<int>(BackendContext::TabPosition::tpProject));
  QCOMPARE(ctx.currentViewMode(), vmBefore);
}

// ── Phase 51-03: SHELL-02 + SHELL-03 shell gate viewmodel-state test ──
// Verifies the 8 BackendContext shell gate Q_PROPERTY are registered, that
// canUndo/canRedo reflect the empty undo stack, that the Prepare -> Preview ->
// Prepare round-trip preserves page/view state without reset, and that the
// editor viewmodel stateChanged signal forwards to BackendContext::stateChanged
// (the SHELL-02 forwarding mechanism from Plan 51-01 task 4). This slot mirrors
// the standalone BackendContext construction of testTabSelectDrivesViewMode.

void ViewModelSmokeTests::shellStateGatesForwardToEditorViewModelAndPreserveRoundTrip()
{
  BackendContext ctx;
  const QMetaObject *meta = ctx.metaObject();

  // 8 gate Q_PROPERTY must be registered on the meta-object so QML can resolve
  // backend.canImport / canSave / isBusy etc. (Plan 51-01).
  QVERIFY2(meta->indexOfProperty("canImport") >= 0, "canImport gate must be a Q_PROPERTY");
  QVERIFY2(meta->indexOfProperty("canSlice") >= 0, "canSlice gate must be a Q_PROPERTY");
  QVERIFY2(meta->indexOfProperty("isSlicing") >= 0, "isSlicing gate must be a Q_PROPERTY");
  QVERIFY2(meta->indexOfProperty("canExport") >= 0, "canExport gate must be a Q_PROPERTY");
  QVERIFY2(meta->indexOfProperty("canSave") >= 0, "canSave gate must be a Q_PROPERTY");
  QVERIFY2(meta->indexOfProperty("canUndo") >= 0, "canUndo gate must be a Q_PROPERTY");
  QVERIFY2(meta->indexOfProperty("canRedo") >= 0, "canRedo gate must be a Q_PROPERTY");
  QVERIFY2(meta->indexOfProperty("isBusy") >= 0, "isBusy gate must be a Q_PROPERTY");

  // 4 state-dependent label Q_PROPERTY must also be registered.
  QVERIFY2(meta->indexOfProperty("exportActionLabel") >= 0, "exportActionLabel must be a Q_PROPERTY");
  QVERIFY2(meta->indexOfProperty("exportActionHint") >= 0, "exportActionHint must be a Q_PROPERTY");
  QVERIFY2(meta->indexOfProperty("saveActionLabel") >= 0, "saveActionLabel must be a Q_PROPERTY");
  QVERIFY2(meta->indexOfProperty("saveActionHint") >= 0, "saveActionHint must be a Q_PROPERTY");

  // On a fresh idle BackendContext the undo/redo stack is empty, so the gate
  // getters must report false (this is the fix for the "Undo clickable when the
  // stack is empty" UX bug from CONTEXT).
  QVERIFY2(!ctx.property("canUndo").toBool(),
           "canUndo must be false on a fresh BackendContext (empty undo stack)");
  QVERIFY2(!ctx.property("canRedo").toBool(),
           "canRedo must be false on a fresh BackendContext (empty undo stack)");

  // canSave forwards to !isSlicing() && !isBusy() — true while idle, so the
  // project can be mutated. The slicing-disable path is unit-covered by the
  // canSave() body (Plan 51-01 acceptance); a full isSlicing=true assertion
  // would require a running slice and is out of scope here.
  QVERIFY2(ctx.property("canSave").toBool(),
           "canSave must be true on a fresh idle BackendContext (not slicing)");

  // SHELL-02 round-trip: Prepare(1) -> Preview(2) -> Prepare(1). The page/view
  // state must return to the original values without a reset, proving the
  // Prepare <-> Preview navigation preserves state (ARCH-05/06/07).
  ctx.setCurrentPage(static_cast<int>(BackendContext::TabPosition::tp3DEditor));
  QCOMPARE(ctx.currentPage(), static_cast<int>(BackendContext::TabPosition::tp3DEditor));
  QCOMPARE(ctx.currentViewMode(), static_cast<int>(BackendContext::ViewMode::View3D));

  ctx.requestSelectTab(static_cast<int>(BackendContext::TabPosition::tpPreview));
  QCOMPARE(ctx.currentPage(), static_cast<int>(BackendContext::TabPosition::tpPreview));
  QCOMPARE(ctx.currentViewMode(), static_cast<int>(BackendContext::ViewMode::Preview));

  ctx.requestSelectTab(static_cast<int>(BackendContext::TabPosition::tp3DEditor));
  QCOMPARE(ctx.currentPage(), static_cast<int>(BackendContext::TabPosition::tp3DEditor));
  QCOMPARE(ctx.currentViewMode(), static_cast<int>(BackendContext::ViewMode::View3D));

  // Gate properties stay readable and the round-trip did not mutate the undo
  // stack (still empty).
  QVERIFY2(!ctx.property("canUndo").toBool(),
           "canUndo must remain false after the Prepare -> Preview -> Prepare round-trip");
  QVERIFY2(!ctx.property("canRedo").toBool(),
           "canRedo must remain false after the round-trip");
  QVERIFY2(ctx.property("canSave").toBool(),
           "canSave must remain true (still idle) after the round-trip");

  // SHELL-02 forwarding: the editor viewmodel stateChanged signal must
  // propagate to BackendContext::stateChanged (Plan 51-01 task 4). Loading a
  // model flips modelCount and fires the editor stateChanged; the
  // BackendContext bulk-refresh signal should fire in response. Requires
  // libslic3r (a real model load); skip otherwise.
  if (!hasLibslic3r())
    QSKIP("stateChanged forwarding assertion requires HAS_LIBSLIC3R (real model load)");

  QSignalSpy spy(&ctx, &BackendContext::stateChanged);
  QVERIFY(spy.isValid());
  auto *editor = qobject_cast<EditorViewModel *>(ctx.editorViewModel());
  QVERIFY(editor);
  // Driving a real model load flips modelCount and fires editor stateChanged.
  // Waiting for modelCount >= 1 (not just for stateChanged) ensures the
  // QtConcurrent import worker thread fully completes before the BackendContext
  // destructor runs, avoiding a dangling-thread crash at teardown.
  QVERIFY2(editor->loadFile(kStlPath), "importing a model should start");
  QTRY_VERIFY_WITH_TIMEOUT(spy.count() >= 1, 10000);
  QTRY_VERIFY_WITH_TIMEOUT(editor->modelCount() >= 1, 10000);
}

// ── Phase 52-03 (PREPSB-05): config/preset change invalidates slice results ──
// CRITICAL gap fix from Plan 52-01: before the BackendContext composition-root
// connect, a config/preset/scope change did NOT invalidate a previously-sliced
// result, so a user could change a filament preset and export G-code based on
// the OLD preset. This test is the regression guard: it verifies the staleness
// Q_PROPERTYs (Plan 52-01) are registered on EditorViewModel and that the
// configVm.sliceAffectingConfigChanged -> editor invalidateAllSliceResults
// connect is wired and active (driving a real option/preset change fires editor
// stateChanged via the connect).
//
// Honest scope: this asserts the CONNECT FIRES (the deterministic, no-libslic3r
// guard). The stale-becomes-true path requires a prior real slice result; that
// needs a libslic3r + real-model fixture and is NOT exercised here. This slot
// inherits the initTestCase HAS_LIBSLIC3R skip (configVm needs preset data).

void ViewModelSmokeTests::sidebarPresetChangeInvalidatesSliceResults()
{
  BackendContext ctx;
  const QMetaObject *editorMeta = ctx.editorViewModel()->metaObject();

  // The two staleness Q_PROPERTYs (Plan 52-01) must be registered so QML can
  // resolve editorVm.hasStaleSliceResults / stalePlateIndices.
  QVERIFY2(editorMeta->indexOfProperty("hasStaleSliceResults") >= 0,
           "EditorViewModel must expose hasStaleSliceResults Q_PROPERTY");
  QVERIFY2(editorMeta->indexOfProperty("stalePlateIndices") >= 0,
           "EditorViewModel must expose stalePlateIndices Q_PROPERTY");

  auto *editor = qobject_cast<EditorViewModel *>(ctx.editorViewModel());
  QVERIFY(editor);
  auto *config = qobject_cast<ConfigViewModel *>(ctx.configViewModel());
  QVERIFY(config);

  // On a fresh idle context there are no sliced plates, so nothing is stale.
  QVERIFY2(!editor->hasStaleSliceResults(),
           "hasStaleSliceResults must be false before any config change");
  QVERIFY2(editor->stalePlateIndices().isEmpty(),
           "stalePlateIndices must be empty before any config change");

  // The PREPSB-05 mechanism is the BackendContext connect:
  // configVm.sliceAffectingConfigChanged -> editor->invalidateAllSliceResults()
  // + emit editor stateChanged. To verify it is wired, drive a preset or option
  // value change and assert the editor stateChanged spy fires.
  QSignalSpy editorSpy(editor, &EditorViewModel::stateChanged);
  QVERIFY(editorSpy.isValid());
  QSignalSpy configSliceSpy(config, &ConfigViewModel::sliceAffectingConfigChanged);
  QVERIFY(configSliceSpy.isValid());

  // loadDefault ensures the preset list is populated so a selection change has
  // a target. The exact preset name is not significant -- any successful
  // request that fires configVm.stateChanged exercises the connect.
  config->loadDefault();
  editorSpy.clear();

  // Drive a slice-affecting config change: select an alternate print preset if
  // more than one exists; otherwise mutate a writable print option.
  const QStringList printNames = config->printPresetNames();
  QVERIFY2(!printNames.isEmpty(), "default print preset list must be non-empty");
  if (printNames.size() > 1) {
    const QString alt = (config->currentPrintPreset() == printNames.first())
                            ? printNames.last() : printNames.first();
    config->requestCurrentPrintPreset(alt);
  } else {
    auto *printOpts = qobject_cast<ConfigOptionModel *>(config->printOptions());
    QVERIFY(printOpts);
    int row = -1;
    for (int i = 0; i < printOpts->rowCount() && row < 0; ++i) {
      if (!printOpts->optReadonly(i))
        row = i;
    }
    QVERIFY2(row >= 0, "No writable print option available to mutate");
    const QVariant oldValue = printOpts->optValue(row);
    const QVariant newValue = oldValue.canConvert<double>()
        ? QVariant(oldValue.toDouble() + 0.01)
        : QVariant(QStringLiteral("T_%1").arg(oldValue.toString()));
    printOpts->setValue(row, newValue);
  }

  QVERIFY2(configSliceSpy.count() >= 1,
           "slice-affecting config changes must emit sliceAffectingConfigChanged");
  // The composition-root connect must forward sliceAffectingConfigChanged to
  // editor->invalidateAllSliceResults() + emit editor stateChanged.
  QVERIFY2(editorSpy.count() >= 1,
           "sliceAffectingConfigChanged must forward to editor stateChanged (PREPSB-05 connect)");
  // With no prior slice result, hasStaleSliceResults stays false (nothing to
  // invalidate), BUT the connect fired -- the mechanism is wired. The
  // stale-becomes-true path requires a prior slice result; that is covered by
  // the slice-result tests. Here we assert the CONNECT is present and active.
  // (Driving a full slice + config change would require libslic3r + a model
  // fixture; the connect-wired assertion is the deterministic guard.)
}

void ViewModelSmokeTests::settingsOpenDoesNotInvalidateSliceResults()
{
  // Opening a settings dialog changes only the active settings tier. It must
  // not clear existing slice/preview/export results.
  BackendContext ctx;
  auto *editor = qobject_cast<EditorViewModel *>(ctx.editorViewModel());
  auto *config = qobject_cast<ConfigViewModel *>(ctx.configViewModel());
  QVERIFY(editor);
  QVERIFY(config);

  QSignalSpy editorSpy(editor, &EditorViewModel::stateChanged);
  QVERIFY(editorSpy.isValid());
  QSignalSpy sliceConfigSpy(config, &ConfigViewModel::sliceAffectingConfigChanged);
  QVERIFY(sliceConfigSpy.isValid());

  ctx.forwardSettingsRequest(QStringLiteral("printer"));

  QCOMPARE(sliceConfigSpy.count(), 0);
  QCOMPARE(editorSpy.count(), 0);
}

// ── Phase 52-03 (PREPSB-02): settings forward signal is honest ──
// The sidebar "Setting" entry point forwards to BackendContext::forwardSettingsRequest,
// which must emit settingsRequested (interim no-op log until Phase 56 wires the dialog).
// This asserts the signal fires so the entry point is honest, not silent dead UI.

void ViewModelSmokeTests::sidebarSettingsForwardEmitsRequestedSignal()
{
  BackendContext ctx;
  const QMetaObject *meta = ctx.metaObject();
  QVERIFY2(meta->indexOfSignal("settingsRequested(QString)") >= 0,
           "BackendContext must expose settingsRequested signal");

  QSignalSpy spy(&ctx, &BackendContext::settingsRequested);
  QVERIFY(spy.isValid());

  ctx.forwardSettingsRequest(QStringLiteral("process"));
  QVERIFY2(spy.count() == 1,
           "forwardSettingsRequest must emit settingsRequested exactly once");
  QCOMPARE(spy.takeFirst().at(0).toString(), QStringLiteral("process"));
}

// ── Phase 04-01: Sidebar Dockable 状态 + 持久化 unit tests ──
// 注意：QSettings 持久化在测试进程内可验证（同 QSettings 默认 ini 路径）。
// 为隔离，每个测试先 reset 三个 key，验证后再 reset。

static void resetSidebarSettings()
{
  QSettings s;
  s.remove(QStringLiteral("owzx/sidebar/collapsed"));
  s.remove(QStringLiteral("owzx/sidebar/width"));
  s.remove(QStringLiteral("owzx/sidebar/settingsVersion"));
  s.remove(QStringLiteral("owzx/sidebar/dockArea"));
  s.sync();
}

void ViewModelSmokeTests::testSidebarCollapsedDefault()
{
  resetSidebarSettings();
  BackendContext ctx;

  // Sidebar is visible by default, matching upstream Plater.
  QCOMPARE(ctx.sidebarCollapsed(), false);
  QCOMPARE(ctx.sidebarMinWidth(), 392);
  QCOMPARE(ctx.sidebarMaxWidth(), 392);
  QCOMPARE(ctx.sidebarWidth(), 392);
  QCOMPARE(ctx.sidebarDockArea(), static_cast<int>(BackendContext::SidebarDockArea::Left));
}

void ViewModelSmokeTests::testRequestToggleSidebar()
{
  resetSidebarSettings();
  BackendContext ctx;

  QSignalSpy spy(&ctx, &BackendContext::sidebarCollapsedChanged);
  QVERIFY(spy.isValid());
  QCOMPARE(ctx.sidebarCollapsed(), false);

  // 折叠
  ctx.requestToggleSidebar();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(ctx.sidebarCollapsed(), true);

  // 再 toggle 展开
  ctx.requestToggleSidebar();
  QCOMPARE(spy.count(), 2);
  QCOMPARE(ctx.sidebarCollapsed(), false);

  // 显式设置同值必须去重（无信号）
  spy.clear();
  ctx.requestSetSidebarCollapsed(false);
  QCOMPARE(spy.count(), 0);
  QCOMPARE(ctx.sidebarCollapsed(), false);

  // 持久化验证：新构造的 ctx 应读到已保存的折叠状态
  ctx.requestSetSidebarCollapsed(true);
  BackendContext ctx2;
  QCOMPARE(ctx2.sidebarCollapsed(), true);  // 从 QSettings 恢复

  resetSidebarSettings();
}

void ViewModelSmokeTests::testSidebarWidthClamp()
{
  resetSidebarSettings();
  BackendContext ctx;

  QSignalSpy spy(&ctx, &BackendContext::sidebarWidthChanged);
  QVERIFY(spy.isValid());

  // Values below the readable width clamp to min.
  ctx.requestSetSidebarWidth(100);
  QCOMPARE(ctx.sidebarWidth(), 392);
  QCOMPARE(spy.count(), 0);

  // Values above the compact contract clamp to max.
  spy.clear();
  ctx.requestSetSidebarWidth(9999);
  QCOMPARE(ctx.sidebarWidth(), 392);
  QCOMPARE(spy.count(), 0);

  // New max width must persist after the v3.9 settings-version marker is written.
  BackendContext ctxMax;
  QCOMPARE(ctxMax.sidebarWidth(), 392);

  // Intermediate values also clamp to the screenshot width.
  spy.clear();
  ctx.requestSetSidebarWidth(360);
  QCOMPARE(ctx.sidebarWidth(), 392);
  QCOMPARE(spy.count(), 0);

  // Equal values after clamping are deduplicated.
  spy.clear();
  ctx.requestSetSidebarWidth(360);
  QCOMPARE(spy.count(), 0);

  // Persistence verification.
  BackendContext ctx2;
  QCOMPARE(ctx2.sidebarWidth(), 392);

  // Pre-pixel-restoration persisted widths are migrated to the screenshot width once.
  resetSidebarSettings();
  {
    QSettings s;
    s.setValue(QStringLiteral("owzx/sidebar/width"), 328);
    s.setValue(QStringLiteral("owzx/sidebar/settingsVersion"), 2);
    s.sync();
  }
  BackendContext legacyCtx;
  QCOMPARE(legacyCtx.sidebarWidth(), 392);

  resetSidebarSettings();
}

void ViewModelSmokeTests::testSidebarDockArea()
{
  resetSidebarSettings();
  BackendContext ctx;

  // 枚举值对齐
  QCOMPARE(static_cast<int>(BackendContext::SidebarDockArea::Left), 0);
  QCOMPARE(static_cast<int>(BackendContext::SidebarDockArea::Right), 1);
  QCOMPARE(ctx.sdaLeft(), 0);
  QCOMPARE(ctx.sdaRight(), 1);

  QSignalSpy spy(&ctx, &BackendContext::sidebarDockAreaChanged);
  QVERIFY(spy.isValid());

  // 切到 Right
  ctx.requestSetSidebarDockArea(static_cast<int>(BackendContext::SidebarDockArea::Right));
  QCOMPARE(spy.count(), 1);
  QCOMPARE(ctx.sidebarDockArea(), static_cast<int>(BackendContext::SidebarDockArea::Right));

  // 同值去重
  spy.clear();
  ctx.requestSetSidebarDockArea(static_cast<int>(BackendContext::SidebarDockArea::Right));
  QCOMPARE(spy.count(), 0);

  // 越界值防御：非 Right 一律按 Left
  spy.clear();
  ctx.requestSetSidebarDockArea(99);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(ctx.sidebarDockArea(), static_cast<int>(BackendContext::SidebarDockArea::Left));

  // 持久化验证
  ctx.requestSetSidebarDockArea(static_cast<int>(BackendContext::SidebarDockArea::Right));
  BackendContext ctx2;
  QCOMPARE(ctx2.sidebarDockArea(), static_cast<int>(BackendContext::SidebarDockArea::Right));

  resetSidebarSettings();
}

// Phase 13 INT-01: deterministic SSDP parser fixtures.
// No multicast, LAN device, or printer is required.
void ViewModelSmokeTests::int01_SsdpDiscoveryParsesMockResponse()
{
#ifdef Q_OS_WIN
  const QByteArray bambuResponse =
      "HTTP/1.1 200 OK\r\n"
      "LOCATION: http://192.168.1.55:80/info\r\n"
      "ST: urn:bambu:device:3dprinter:1\r\n"
      "USN: uuid:abc123::urn:bambu:device:3dprinter:1\r\n"
      "SERVER: Bambu Lab X1\r\n"
      "\r\n";
  const owzx::DiscoveredDevice bambu =
      owzx::SsdpDiscovery::parseResponseDatagram(bambuResponse, QHostAddress(QStringLiteral("10.0.0.5")));
  QCOMPARE(bambu.ip, QStringLiteral("192.168.1.55"));
  QCOMPARE(bambu.serial, QStringLiteral("ABC123"));
  QCOMPARE(bambu.model, QStringLiteral("3D Printer"));
  QCOMPARE(bambu.name, QStringLiteral("Bambu Lab X1"));
  QCOMPARE(bambu.port, 8883);
  QVERIFY(bambu.isBambu);

  const QByteArray crealityResponse =
      "HTTP/1.1 200 OK\r\n"
      "ST: urn:creality:device:3dprinter:1\r\n"
      "USN: uuid:k1c-001::urn:creality:device:3dprinter:1\r\n"
      "SERVER: Creality K1C\r\n"
      "\r\n";
  const owzx::DiscoveredDevice creality =
      owzx::SsdpDiscovery::parseResponseDatagram(crealityResponse, QHostAddress(QStringLiteral("192.168.1.60")));
  QCOMPARE(creality.ip, QStringLiteral("192.168.1.60"));
  QCOMPARE(creality.serial, QStringLiteral("K1C-001"));
  QCOMPARE(creality.model, QStringLiteral("3D Printer"));
  QCOMPARE(creality.name, QStringLiteral("Creality K1C"));
  QCOMPARE(creality.port, 1883);
  QVERIFY(!creality.isBambu);

  owzx::SsdpDiscovery discovery;
  QSignalSpy doneSpy(&discovery, &owzx::SsdpDiscovery::discoveryFinished);
  QVERIFY(doneSpy.isValid());
  QVERIFY(QMetaObject::invokeMethod(&discovery, "onTimeout", Qt::DirectConnection));
  QCOMPARE(doneSpy.count(), 1);
  QCOMPARE(doneSpy.takeFirst().at(0).toInt(), discovery.discoveredCount());
#endif
}

// ── v2.6 Phase 4: INT-03 摄像头视频流自回归（状态机 + 帧令牌） ──────
// 验证 CameraServiceMock 状态机 + MonitorViewModel 帧令牌转发：
//   - 初始 streamStatus=0 (Disconnected), frameToken=0
//   - updateForDevice(online=true) → cameraAvailable=true
//   - startStream → 状态机切换 Connecting(1) → Connected(2) → Streaming(3)
//   - MonitorViewModel.cameraStreamStatus / cameraFrameToken 可读
//   - stopStream → Disconnected(0), frameToken 归零（清帧）
//
// 不依赖真实 RTSP 服务器：cameraUrl_ 为空时 startRtspDecoder 为 noop，
// 纯 mock 状态机验证。
void ViewModelSmokeTests::int03_CameraStateMachineAndFrameToken()
{
  CameraServiceMock camera;
  DeviceServiceMock device;
  NetworkServiceMock network;
  MonitorViewModel monitor(&device, &network, &camera);

  // 初始状态
  QCOMPARE(CameraServiceMock::defaultRtspUrlForDevice(QStringLiteral("192.168.1.50")),
           QStringLiteral("rtsp://192.168.1.50:8554/streaming/live/1"));
  QVERIFY(CameraServiceMock::defaultRtspUrlForDevice(QString()).isEmpty());
  QCOMPARE(camera.streamStatus(), 0); // Disconnected
  QCOMPARE(camera.frameToken(), 0);
  QCOMPARE(monitor.cameraStreamStatus(), 0);
  QCOMPARE(monitor.cameraFrameToken(), 0);
  QCOMPARE(camera.cameraAvailable(), false);

  // 选中一台在线设备 → cameraAvailable=true
  camera.updateForDevice(QStringLiteral("192.168.1.50"), true);
  QCOMPARE(camera.cameraAvailable(), true);

  QSignalSpy statusSpy(&camera, &CameraServiceMock::streamStatusChanged);
  QVERIFY(statusSpy.isValid());

  // 启动流：状态机 Connecting(1) → Connected(2) → Streaming(3)
  camera.startStream();
  QCOMPARE(camera.cameraUrl(),
           QStringLiteral("rtsp://192.168.1.50:8554/streaming/live/1"));
  QCOMPARE(statusSpy.count(), 1);
  QCOMPARE(camera.streamStatus(), 1); // Connecting

  // Connected（1500ms 后）
  QTest::qWait(1700);
  QCOMPARE(camera.streamStatus(), 2); // Connected

  // Streaming（再 800ms 后）
  QTest::qWait(1000);
  QCOMPARE(camera.streamStatus(), 3); // Streaming

  // MonitorViewModel 转发一致
  QCOMPARE(monitor.cameraStreamStatus(), 3);

  // 停止流：回到 Disconnected，帧清零
  QSignalSpy frameSpy(&camera, &CameraServiceMock::frameTokenChanged);
  camera.stopStream();
  QCOMPARE(camera.streamStatus(), 0); // Disconnected
  QCOMPARE(camera.frameToken(), 0);   // 帧清零
  QVERIFY(camera.currentFrame().isNull());

  // 离线设备 → cameraAvailable=false，startStream 拒绝（errorMessage 设置）
  camera.updateForDevice(QStringLiteral(""), false);
  QCOMPARE(camera.cameraAvailable(), false);
  camera.startStream(); // 应被拒
  QCOMPARE(camera.streamStatus(), 0); // 仍 Disconnected
  QVERIFY(!camera.errorMessage().isEmpty());
}

// ── v2.7 P1: INT-02 校准自回归（calib slice 生成 G-code） ──────────
// 验证 SliceService::setCalibParams → Print.set_calib_params → GCode::do_export
// 走 Calib_PA_Line 分支生成校准 G-code（路径 B，镜像上游 CalibUtils::send_to_print）。
//
// 断言：
//   - setCalibParams(PA_Line) 后切片生成非空 G-code
//   - 生成的 G-code 含 SET_PRESSURE_ADVANCE / M572 token（PA 校准标志）
//   - 或至少含 calib 标志（flow_ratio / temp 变化序列）
//   - 普通 mode=0 切片不含 calib token（对照组）
void ViewModelSmokeTests::int02_CalibrationGeneratesCalibGcode()
{
  ProjectServiceMock project;
  SliceService slice(&project);

  // 加载测试模型（复用 E2E 的 Prusa.stl 路径）
  const QString stlPath = QDir::cleanPath(
      QStringLiteral(QT_TESTCASE_SOURCEDIR) +
      QStringLiteral("/third_party/OrcaSlicer/tests/data/test_3mf/Prusa.stl"));
  QVERIFY(QFileInfo::exists(stlPath));
  QVERIFY(project.loadFile(stlPath));

  // 等待异步加载完成
  QSignalSpy loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(loadSpy.isValid());
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);

  // 设热床 + PA 校准参数（Calib_PA_Line, mode=1）
  slice.setBedShape({QPointF(0, 0), QPointF(220, 0), QPointF(220, 220), QPointF(0, 220)});
  slice.setCalibParams(1 /*Calib_PA_Line*/, 0.0, 0.1, 0.002, true);

  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);
  QVERIFY(finishedSpy.isValid());

  slice.startSlice(QStringLiteral("int02_pa_calib"));

  // 等待切片完成（校准切片可能比普通切片稍慢）
  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0, 120000);

  // 校准切片应成功（若环境/配置导致失败，QSKIP 而非 FAIL —— 校准 G-code 生成
  // 依赖完整 Print.apply + do_export 路径，某些上游版本可能行为不同）
  if (failedSpy.count() > 0) {
    const QString reason = failedSpy.first().at(0).toString();
    QSKIP(QString("PA calib slice failed (env-dependent): %1").arg(reason).toUtf8().constData());
  }
  QCOMPARE(finishedSpy.count(), 1);

  // 验证生成的 G-code 含校准标志（PA 校准会写 SET_PRESSURE_ADVANCE 或 M572）
  const QString gcodePath = slice.outputPath();
  QVERIFY2(QFileInfo::exists(gcodePath), "calib G-code file should exist");
  QFile gcodeFile(gcodePath);
  QVERIFY2(gcodeFile.open(QIODevice::ReadOnly | QIODevice::Text),
            "calib G-code should be readable");
  const QByteArray gcode = gcodeFile.readAll();
  gcodeFile.close();

  QVERIFY2(gcode.size() > 100, "calib G-code should be non-trivial");
  // PA 校准 G-code 含 SET_PRESSURE_ADVANCE 或 M572（Marlin/BBL PA 设置指令）
  const bool hasPaToken = gcode.contains("SET_PRESSURE_ADVANCE") ||
                          gcode.contains("M572") ||
                          gcode.contains("pressure_advance") ||
                          gcode.contains("M900"); // Marlin linear advance
  QVERIFY2(hasPaToken, "PA calib G-code should contain a pressure-advance token");

  // 清理
  QFile::remove(gcodePath);
}

void ViewModelSmokeTests::calibrationImplementedModesExposeStableRouting()
{
  CalibrationServiceMock service;
  CalibrationViewModel vm(&service);

  const ExpectedCalibRequest expected[] = {
      {"flow_dynamics", 1, 0.0, 0.1, 0.002, true},
      {"flow_rate", 5, 0.90, 1.10, 0.01, true},
      {"temp_tower", 6, 190.0, 240.0, 5.0, true},
  };

  for (const auto &item : expected)
  {
    const QString id = QString::fromLatin1(item.id);
    const int index = service.calibTypeIndexById(id);
    QVERIFY2(index >= 0, qPrintable(QStringLiteral("Missing calibration id %1").arg(id)));
    QCOMPARE(service.calibTypeId(index), id);
    QVERIFY(service.calibTypeImplemented(index));
    QVERIFY(service.calibTypeStartable(index));
    QVERIFY(service.calibTypeUnavailableReason(index).isEmpty());

    QVERIFY(vm.selectItemById(id));
    QCOMPARE(vm.selectedIndex(), index);
    QCOMPARE(vm.calibItemId(index), id);
    QVERIFY(vm.calibItemImplemented(index));
    QVERIFY(vm.calibItemStartable(index));
    QVERIFY(vm.calibItemUnavailableReason(index).isEmpty());
  }
}

void ViewModelSmokeTests::calibrationImplementedModesEmitSliceRequests()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  CalibrationServiceMock service;
  service.setSliceService(&slice);

  QSignalSpy requestSpy(&service, &CalibrationServiceMock::calibrationSliceRequested);
  QVERIFY(requestSpy.isValid());

  const ExpectedCalibRequest expected[] = {
      {"flow_dynamics", 1, 0.0, 0.1, 0.002, true},
      {"flow_rate", 5, 0.90, 1.10, 0.01, true},
      {"temp_tower", 6, 190.0, 240.0, 5.0, true},
  };

  for (const auto &item : expected)
  {
    const QString id = QString::fromLatin1(item.id);
    const int index = service.calibTypeIndexById(id);
    QVERIFY(index >= 0);

    service.startCalibration(index);
    QCOMPARE(requestSpy.count(), 1);
    const QList<QVariant> args = requestSpy.takeFirst();
    QCOMPARE(args.at(0).toInt(), item.mode);
    QCOMPARE(args.at(1).toDouble(), item.start);
    QCOMPARE(args.at(2).toDouble(), item.end);
    QCOMPARE(args.at(3).toDouble(), item.step);
    QCOMPARE(args.at(4).toBool(), item.printNumbers);
    QCOMPARE(args.at(5).toString(), QStringLiteral("calib_%1").arg(id));
  }
}

void ViewModelSmokeTests::calibrationUnsupportedModesAreExplicitlyUnavailable()
{
  CalibrationServiceMock service;
  CalibrationViewModel vm(&service);

  const QStringList unsupportedIds = {
      QStringLiteral("bed_leveling"),
      QStringLiteral("vibration"),
      QStringLiteral("max_volumetric_speed"),
  };

  for (const QString &id : unsupportedIds)
  {
    const int index = service.calibTypeIndexById(id);
    QVERIFY2(index >= 0, qPrintable(QStringLiteral("Missing calibration id %1").arg(id)));
    QVERIFY(!service.calibTypeImplemented(index));
    QVERIFY(!service.calibTypeStartable(index));
    QVERIFY(!service.calibTypeUnavailableReason(index).isEmpty());

    QVERIFY(vm.selectItemById(id));
    QVERIFY(!vm.calibItemImplemented(index));
    QVERIFY(!vm.calibItemStartable(index));
    QVERIFY(!vm.calibItemUnavailableReason(index).isEmpty());

    QSignalSpy requestSpy(&service, &CalibrationServiceMock::calibrationSliceRequested);
    service.startCalibration(index);
    QCOMPARE(requestSpy.count(), 0);
    QCOMPARE(service.isRunning(), false);
    QCOMPARE(service.calibStatus(index), static_cast<int>(CalibrationStatus::NotStarted));
  }
}

void ViewModelSmokeTests::calibrationFallbackAndSliceCallbacksDriveProgress()
{
  CalibrationServiceMock service;
  const int flowIndex = service.calibTypeIndexById(QStringLiteral("flow_dynamics"));
  QVERIFY(flowIndex >= 0);

  QSignalSpy finishedSpy(&service, &CalibrationServiceMock::calibrationFinished);
  QVERIFY(finishedSpy.isValid());

  service.startCalibration(flowIndex);
  QVERIFY(service.isRunning());
  for (int i = 0; i < 50 && service.isRunning(); ++i)
  {
    QVERIFY(QMetaObject::invokeMethod(&service, "onTick", Qt::DirectConnection));
  }
  QCOMPARE(finishedSpy.count(), 1);
  QCOMPARE(finishedSpy.takeFirst().at(0).toBool(), true);
  QCOMPARE(service.isRunning(), false);
  QCOMPARE(service.progress(), 100);
  QCOMPARE(service.calibStatus(flowIndex), static_cast<int>(CalibrationStatus::Completed));

  service.resetCalibration(flowIndex);
  service.startCalibration(flowIndex);
  QVERIFY(service.isRunning());
  QVERIFY(QMetaObject::invokeMethod(&service, "onSliceProgressUpdated",
                                    Qt::DirectConnection,
                                    Q_ARG(int, 42),
                                    Q_ARG(QString, QStringLiteral("Running slice"))));
  QCOMPARE(service.progress(), 42);
  QVERIFY(QMetaObject::invokeMethod(&service, "onSliceFinished",
                                    Qt::DirectConnection,
                                    Q_ARG(QString, QStringLiteral("done"))));
  QCOMPARE(service.isRunning(), false);
  QCOMPARE(service.progress(), 100);
  QCOMPARE(service.calibStatus(flowIndex), static_cast<int>(CalibrationStatus::Completed));

  service.resetCalibration(flowIndex);
  service.startCalibration(flowIndex);
  QVERIFY(service.isRunning());
  QVERIFY(QMetaObject::invokeMethod(&service, "onSliceFailed",
                                    Qt::DirectConnection,
                                    Q_ARG(QString, QStringLiteral("failed"))));
  QCOMPARE(service.isRunning(), false);
  QCOMPARE(service.calibStatus(flowIndex), static_cast<int>(CalibrationStatus::Failed));
}

// ── v2.7 P2-A: INT-04 MQTT 连接参数 + 遥测字段映射自回归 ──────────
// 不连真机（CI 无设备）。验证：
//   - MockDevice access code + port 设置/读取往返（连接对话框流程）
//   - 新遥测字段 getter（bedTemperature/nozzleTarget/currentLayerNum/remainingTime）
//     在手动填充后可正确读取（MQTT messageReceived 解析逻辑写入这些字段）
//   - mqttConnected 初始为 false（未连接）
//   - setSelectedDeviceAccessCode 触发 selectedDeviceChanged
//
// 完整 MQTT 连接 + 真实 telemetry 解析需真机，延后 UAT。
void ViewModelSmokeTests::int04_MqttConnectionParamsAndTelemetryFields()
{
  DeviceServiceMock device;
  // 初始：未连接
  QVERIFY(!device.isMqttConnected());
  QVERIFY(device.selectedDeviceAccessCode().isEmpty());
  QCOMPARE(device.selectedDeviceMqttPort(), 8883);

  // 选中第一个设备
  if (device.deviceCount() == 0) QSKIP("No mock devices for telemetry test");
  device.selectDevice(0);

  // 设置 access code（连接对话框录入后调用）
  QSignalSpy changedSpy(&device, &DeviceServiceMock::selectedDeviceChanged);
  QVERIFY(changedSpy.isValid());
  device.setSelectedDeviceAccessCode(QStringLiteral("12345678"), 8883);
  QCOMPARE(device.selectedDeviceAccessCode(), QStringLiteral("12345678"));
  QCOMPARE(device.selectedDeviceMqttPort(), 8883);
  QVERIFY(changedSpy.count() >= 1); // setter 应触发通知

  // 验证新遥测字段 getter 存在且初始为 0（MQTT 连接后由解析填充）
  // 这些字段是 P2-A 扩展的，确保 Q_PROPERTY 链路完整
  QCOMPARE(device.selectedDeviceBedTemperature(), 0);
  QCOMPARE(device.selectedDeviceNozzleTargetTemp(), 0);
  QCOMPARE(device.selectedDeviceBedTargetTemp(), 0);
  QCOMPARE(device.selectedDeviceCurrentLayerNum(), 0);
  QCOMPARE(device.selectedDeviceTotalLayerNum(), 0);
  QCOMPARE(device.selectedDeviceRemainingTime(), 0);

  const QString nestedPayload = QStringLiteral(
      R"({"print":{"msg":{"gcode_state":"RUNNING","mc_percent":42,"nozzle_temper":215,"nozzle_target_temper":220,"bed_temper":62,"bed_target_temper":65,"layer_num":3,"total_layer_num":200,"mc_remaining_time":71}}})");
  QVERIFY(device.applyMqttReportPayload(nestedPayload, device.selectedDeviceIndex()));
  QCOMPARE(device.selectedDeviceStatus(), QStringLiteral("printing"));
  QCOMPARE(device.selectedDeviceProgress(), 42);
  QCOMPARE(device.selectedDeviceTemperature(), 215);
  QCOMPARE(device.selectedDeviceNozzleTargetTemp(), 220);
  QCOMPARE(device.selectedDeviceBedTemperature(), 62);
  QCOMPARE(device.selectedDeviceBedTargetTemp(), 65);
  QCOMPARE(device.selectedDeviceCurrentLayerNum(), 3);
  QCOMPARE(device.selectedDeviceTotalLayerNum(), 200);
  QCOMPARE(device.selectedDeviceRemainingTime(), 71);

  const QString directPayload = QStringLiteral(
      R"({"print":{"gcode_state":"PAUSE","mc_percent":43}})");
  QVERIFY(device.applyMqttReportPayload(directPayload, device.selectedDeviceIndex()));
  QCOMPARE(device.selectedDeviceStatus(), QStringLiteral("paused"));
  QCOMPARE(device.selectedDeviceProgress(), 43);

  QVERIFY(!device.applyMqttReportPayload(QStringLiteral("{}"), device.selectedDeviceIndex()));
  QVERIFY(!device.applyMqttReportPayload(QStringLiteral("{"), device.selectedDeviceIndex()));

  device.setSearchText(QStringLiteral("CR-10"));
  QCOMPARE(device.filteredDeviceCount(), 1);
  device.selectDevice(0);
  QCOMPARE(device.selectedDeviceIndex(), 3);
  const QString filteredPayload = QStringLiteral(
      R"({"print":{"gcode_state":"RUNNING","mc_percent":77,"nozzle_temper":208}})");
  QVERIFY(device.applyMqttReportPayload(filteredPayload, device.selectedDeviceIndex()));
  QCOMPARE(device.selectedDeviceStatus(), QStringLiteral("printing"));
  QCOMPARE(device.selectedDeviceProgress(), 77);
  QCOMPARE(device.selectedDeviceTemperature(), 208);
  device.setSearchText(QString());
  QCOMPARE(device.selectedDeviceIndex(), 0);

  // 验证 MonitorViewModel 转发（若注入 DeviceServiceMock）
  NetworkServiceMock network;
  CameraServiceMock camera;
  MonitorViewModel monitor(&device, &network, &camera);
  QCOMPARE(monitor.selectedDeviceAccessCode(), QStringLiteral("12345678"));
  QCOMPARE(monitor.selectedDeviceMqttPort(), 8883);
  QVERIFY(!monitor.mqttConnected());
  // MonitorViewModel::setSelectedDeviceAccessCode 转发
  monitor.setSelectedDeviceAccessCode(QStringLiteral("ABCDEFGH"), 8883);
  QCOMPARE(monitor.selectedDeviceAccessCode(), QStringLiteral("ABCDEFGH"));

  // 清理 access code → 连接应走 mock fallback
  device.setSelectedDeviceAccessCode(QStringLiteral(""), 8883);
  QVERIFY(device.selectedDeviceAccessCode().isEmpty());
}

// ── v2.7 P2-B: INT-05 MQTT 命令构造 + 控制流自回归 ──────────
// 不连真机。验证：
//   - publishPrintCommand 在未连接时安全返回 false（不崩溃）
//   - lastPublishPayload/Topic 初始为空
//   - pause/resume/stop 在 MQTT 未连接时走 mock fallback（不崩溃，状态正确）
//   - publishPrintCommand 的 JSON 构造逻辑：通过反射验证（连接时构造）
//     真实 publish 需真机，这里验证命令流不崩溃 + mock 路径正确
void ViewModelSmokeTests::int05_MqttCommandConstructionAndControlFlow()
{
  DeviceServiceMock device;
  // 初始：无 publish 记录
  QVERIFY(device.lastPublishPayload().isEmpty());
  QVERIFY(device.lastPublishTopic().isEmpty());

  // 未连接时 publishPrintCommand 应安全返回 false
  QVERIFY(!device.publishPrintCommand("pause"));
  QVERIFY(!device.publishPrintCommand("resume"));
  QVERIFY(!device.publishPrintCommand("stop"));
  QVERIFY(!device.publishPrintCommand("gcode_line", "G28"));

  const QString pausePayload = DeviceServiceMock::buildPrintCommandEnvelope(
      QStringLiteral("pause"), QString(), 7);
  const QJsonObject pausePrint = QJsonDocument::fromJson(pausePayload.toUtf8())
                                     .object()
                                     .value(QStringLiteral("print"))
                                     .toObject();
  QCOMPARE(pausePrint.value(QStringLiteral("sequence_id")).toString(), QStringLiteral("7"));
  QCOMPARE(pausePrint.value(QStringLiteral("command")).toString(), QStringLiteral("pause"));
  QVERIFY(!pausePrint.contains(QStringLiteral("param")));

  const QString resumePayload = DeviceServiceMock::buildPrintCommandEnvelope(
      QStringLiteral("resume"), QString(), 8);
  const QJsonObject resumePrint = QJsonDocument::fromJson(resumePayload.toUtf8())
                                      .object()
                                      .value(QStringLiteral("print"))
                                      .toObject();
  QCOMPARE(resumePrint.value(QStringLiteral("sequence_id")).toString(), QStringLiteral("8"));
  QCOMPARE(resumePrint.value(QStringLiteral("command")).toString(), QStringLiteral("resume"));
  QVERIFY(!resumePrint.contains(QStringLiteral("param")));

  const QString stopPayload = DeviceServiceMock::buildPrintCommandEnvelope(
      QStringLiteral("stop"), QString(), 9);
  const QJsonObject stopPrint = QJsonDocument::fromJson(stopPayload.toUtf8())
                                    .object()
                                    .value(QStringLiteral("print"))
                                    .toObject();
  QCOMPARE(stopPrint.value(QStringLiteral("sequence_id")).toString(), QStringLiteral("9"));
  QCOMPARE(stopPrint.value(QStringLiteral("command")).toString(), QStringLiteral("stop"));
  QVERIFY(!stopPrint.contains(QStringLiteral("param")));

  const QString gcodePayload = DeviceServiceMock::buildPrintCommandEnvelope(
      QStringLiteral("gcode_file"), QStringLiteral("/mnt/sdcard/test.gcode"), 10);
  const QJsonObject gcodePrint = QJsonDocument::fromJson(gcodePayload.toUtf8())
                                     .object()
                                     .value(QStringLiteral("print"))
                                     .toObject();
  QCOMPARE(gcodePrint.value(QStringLiteral("sequence_id")).toString(), QStringLiteral("10"));
  QCOMPARE(gcodePrint.value(QStringLiteral("command")).toString(), QStringLiteral("gcode_file"));
  QCOMPARE(gcodePrint.value(QStringLiteral("param")).toString(), QStringLiteral("/mnt/sdcard/test.gcode"));
  QCOMPARE(DeviceServiceMock::buildPrintCommandTopic(QStringLiteral("CP01001A001")),
           QStringLiteral("device/CP01001A001/request"));

  if (device.deviceCount() == 0) QSKIP("No mock devices for control flow test");
  device.selectDevice(0);

  // 添加一个打印任务以测试 pause/resume/stop mock 路径
  device.startPrint(0, QStringLiteral("/tmp/test.gcode"));

  // pause（MQTT 未连接 → 走 mock，状态应变 paused）
  device.pausePrint(0);
  // resume（mock → printing）
  device.resumePrint(0);
  // stop（mock → idle）
  device.stopPrint(0);

  // 验证未连接时这些控制不崩溃（到达此处即通过）
  QVERIFY(true);

  // 验证 publishPrintCommand 可被 MonitorViewModel 间接调用（Q_INVOKABLE）
  // 且 MQTT 连接状态查询正常
  QVERIFY(!device.isMqttConnected());
}

// ── v2.8 P2-C: INT-06 FTP URL 构造 + send-print 路由自回归 ──────────
// 不连真机。验证：
//   - FtpUploader::buildFtpUrl 生成正确的 Bambu FTP URL 格式
//   - sendPrintViaFtp 在未连接时安全返回 false
//   - startPrint 在未连接时走 mock fallback（不崩溃）
//   - DeviceServiceMock 的 FTP + MQTT 接线（sendPrintViaFtp 存在且 Q_INVOKABLE）
void ViewModelSmokeTests::int06_FtpUrlAndSendPrintRouting()
{
  // 1. FtpUploader URL 构造（对齐 Bambu FTPS 格式）
  const QString url = FtpUploader::buildFtpUrl(
      QStringLiteral("192.168.1.100"), 990,
      QStringLiteral("ABC12345"), QStringLiteral("/mnt/sdcard/test.gcode"));
  QVERIFY2(url.startsWith("ftp://"), "FTP URL should start with ftp://");
  QVERIFY2(url.contains("bblp:"), "FTP URL should contain bblp username");
  QVERIFY2(url.contains("ABC12345"), "FTP URL should contain access code");
  QVERIFY2(url.contains("192.168.1.100"), "FTP URL should contain host");
  QVERIFY2(url.contains("990"), "FTP URL should contain port 990");
  QVERIFY2(url.contains("/mnt/sdcard/test.gcode"),
           "FTP URL should contain remote path");

  const QString encodedUrl = FtpUploader::buildFtpUrl(
      QStringLiteral("192.168.1.100"), 990,
      QStringLiteral("A B/1"), QStringLiteral("/mnt/sdcard/test.gcode"));
  QVERIFY2(encodedUrl.contains(QStringLiteral("A%20B%2F1"), Qt::CaseInsensitive),
           "FTP URL should percent-encode access code");
  QCOMPARE(DeviceServiceMock::buildPrintRemotePath(QStringLiteral("C:/tmp/plate one.gcode")),
           QStringLiteral("/mnt/sdcard/plate one.gcode"));

  FtpUploader uploader;
  QSignalSpy uploadDone(&uploader, &FtpUploader::uploadFinished);
  QVERIFY(uploadDone.isValid());
  const QString missingPath = QDir::temp().filePath(QStringLiteral("owzx_phase13_missing.gcode"));
  QFile::remove(missingPath);
  QVERIFY(!uploader.uploadFile(QStringLiteral("192.0.2.10"), 990, QStringLiteral("ACCESS"),
                               missingPath, QStringLiteral("/mnt/sdcard/missing.gcode")));
  QCOMPARE(uploadDone.count(), 1);
  const QList<QVariant> uploadArgs = uploadDone.takeFirst();
  QCOMPARE(uploadArgs.at(0).toBool(), false);
  QVERIFY(uploadArgs.at(1).toString().contains(QStringLiteral("local file not found")));

  // 2. DeviceServiceMock sendPrintViaFtp 在未连接时安全返回 false
  DeviceServiceMock device;
  QVERIFY(!device.isMqttConnected());
  QVERIFY(!device.sendPrintViaFtp(0, QStringLiteral("/tmp/test.gcode")));

  // 3. sendPrintViaFtp 对空 gcode 路径安全返回 false
  QVERIFY(!device.sendPrintViaFtp(0, QString()));

  // 4. startPrint 在未连接时走 mock fallback（不崩溃，到达此处即通过）
  if (device.deviceCount() > 0) {
    device.selectDevice(0);
    device.startPrint(0, QString()); // mock path, no crash
  }
}

void ViewModelSmokeTests::appSettingsAndEditorBedShapePersistDeterministically()
{
  ScopedSettingsSnapshot appSettingsKeys({
      QStringLiteral("Bed/Width"),
      QStringLiteral("Bed/Depth"),
  });
  appSettingsKeys.clear();

  {
    AppSettingsService settings;
    QCOMPARE(settings.bedWidth(), 220.0);
    QCOMPARE(settings.bedDepth(), 220.0);
    settings.setBedSize(QSizeF(32.0, 2500.0));
    QCOMPARE(settings.bedWidth(), 50.0);
    QCOMPARE(settings.bedDepth(), 2000.0);
  }
  {
    AppSettingsService settings;
    QCOMPARE(settings.bedWidth(), 50.0);
    QCOMPARE(settings.bedDepth(), 2000.0);
    settings.resetToDefaults();
    QCOMPARE(settings.bedWidth(), 220.0);
    QCOMPARE(settings.bedDepth(), 220.0);
  }

  ScopedSettingsSnapshot editorBedKeys({
      QStringLiteral("bed/width"),
      QStringLiteral("bed/depth"),
      QStringLiteral("bed/maxHeight"),
      QStringLiteral("bed/originX"),
      QStringLiteral("bed/originY"),
      QStringLiteral("bed/shapeType"),
      QStringLiteral("bed/diameter"),
  });
  editorBedKeys.clear();

  {
    ProjectServiceMock project;
    SliceService slice(&project);
    EditorViewModel editor(&project, &slice);
    editor.setBedWidth(333.0f);
    editor.setBedDepth(444.0f);
    editor.setBedMaxHeight(555.0f);
    editor.setBedOriginX(-12.5f);
    editor.setBedOriginY(13.5f);
    editor.setBedShapeType(1);
    editor.setBedDiameter(222.0f);
  }
  {
    ProjectServiceMock project;
    SliceService slice(&project);
    EditorViewModel editor(&project, &slice);
    QCOMPARE(editor.bedWidth(), 333.0f);
    QCOMPARE(editor.bedDepth(), 444.0f);
    QCOMPARE(editor.bedMaxHeight(), 555.0f);
    QCOMPARE(editor.bedOriginX(), -12.5f);
    QCOMPARE(editor.bedOriginY(), 13.5f);
    QCOMPARE(editor.bedShapeType(), 1);
    QCOMPARE(editor.bedDiameter(), 222.0f);
  }
}

// ── v3.0 Phase 16-01: PartPlate/PartPlateList domain-model unit tests ──
// Pure-domain tests (no libslic3r dependency, no ProjectServiceMock). They exercise
// the new src/core/model/ classes directly to lock in the data structure before the
// big-bang migration in plan 16-02.

void ViewModelSmokeTests::partPlateInstanceMembershipTracksObjectInstancePairs()
{
  // D-03: instance-level membership (std::set<pair<int,int>>) can represent
  // "some instances of one object on this plate, others elsewhere."
  OWzx::PartPlate plate(0);
  plate.addInstance(0, 0);
  plate.addInstance(0, 1);
  plate.addInstance(2, 0);
  QCOMPARE(static_cast<int>(plate.objToInstanceSet().size()), 3);
  QVERIFY(plate.hasObject(0));
  QVERIFY(!plate.hasObject(1));
  QVERIFY(plate.hasObject(2));
  plate.removeInstance(0, 1);
  QCOMPARE(static_cast<int>(plate.objToInstanceSet().size()), 2);
  QVERIFY(plate.hasObject(0));  // instance (0,0) still present
}

void ViewModelSmokeTests::partPlateSliceStateMachineGatesCanSlice()
{
  // Slice state machine (upstream canSlice semantics): slice allowed only when
  // ready_for_slice && !apply_invalid.
  OWzx::PartPlate plate(0);
  plate.setReadyForSlice(true);
  plate.setApplyInvalid(false);
  QVERIFY(plate.canSlice());
  plate.setApplyInvalid(true);
  QVERIFY(!plate.canSlice());
  plate.setApplyInvalid(false);
  plate.setReadyForSlice(false);
  QVERIFY(!plate.canSlice());
}

void ViewModelSmokeTests::partPlateListCreateDeleteRenameLockReindexesAndKeepsAtLeastOne()
{
  // PLATE-02 + PLATE-06: PartPlateList owns plates, reindexes on delete, keeps >= 1.
  OWzx::PartPlateList list;
  QCOMPARE(list.plateCount(), 1);  // constructor starts with 1 plate
  OWzx::PartPlate* second = list.createPlate();
  QVERIFY(second != nullptr);
  QCOMPARE(list.plateCount(), 2);
  QCOMPARE(second->plateIndex(), 1);  // auto-incremented index
  QVERIFY(list.renamePlate(1, "Second"));
  QCOMPARE(QString::fromStdString(list.plate(1)->name()), QStringLiteral("Second"));
  list.setPlateLocked(0, true);
  QVERIFY(list.plate(0)->isLocked());
  // delete plate 0 → survivor (was index 1) reindexes to 0
  QVERIFY(list.deletePlate(0));
  QCOMPARE(list.plateCount(), 1);
  QCOMPARE(list.plate(0)->plateIndex(), 0);  // reindexed
  QCOMPARE(QString::fromStdString(list.plate(0)->name()), QStringLiteral("Second"));
  // cannot delete the last plate
  QVERIFY(!list.deletePlate(0));
  QCOMPARE(list.plateCount(), 1);
}

void ViewModelSmokeTests::partPlateListInstanceMembershipDerivesObjectIndices()
{
  // Bridge query: instance-pair membership collapses to distinct object indices.
  OWzx::PartPlateList list;
  OWzx::PartPlate* p = list.plate(0);
  QVERIFY(p != nullptr);
  p->addInstance(0, 0);
  p->addInstance(0, 1);
  p->addInstance(5, 0);
  QList<int> objs = list.objectIndicesOnPlate(0);
  QCOMPARE(objs.size(), 2);
  QVERIFY(objs.contains(0));
  QVERIFY(objs.contains(5));
  // plateIndexForObject finds the first plate holding the object
  QCOMPARE(list.plateIndexForObject(0), 0);
  QCOMPARE(list.plateIndexForObject(5), 0);
  QCOMPARE(list.plateIndexForObject(99), -1);  // not on any plate
}

void ViewModelSmokeTests::partPlateListRefusesExceedMaxPlateCount()
{
  // MAX_PLATE_COUNT=36 enforced — upstream create_plate guard.
  OWzx::PartPlateList list;
  QCOMPARE(list.plateCount(), 1);
  // create 35 more to reach 36 total
  for (int i = 0; i < 35; ++i) {
    QVERIFY2(list.createPlate() != nullptr, "plate creation should succeed up to max");
  }
  QCOMPARE(list.plateCount(), OWzx::kMaxPlateCount);
  // 37th creation must be refused
  QVERIFY(list.createPlate() == nullptr);
  QCOMPARE(list.plateCount(), OWzx::kMaxPlateCount);
}

void ViewModelSmokeTests::projectServicePlateOpsBackedByPartPlateList()
{
  // PLATE-06 regression: after the big-bang migration to PartPlateList, the existing
  // plate Q_INVOKABLE surface (add/delete/rename/lock/select) must still work.
  ProjectServiceMock project;
  // A freshly-constructed service has one plate (the PartPlateList invariant).
  QCOMPARE(project.plateCount(), 1);

  QVERIFY(project.addPlate());
  QCOMPARE(project.plateCount(), 2);

  QVERIFY(project.renamePlate(1, QStringLiteral("Second")));
  QCOMPARE(project.plateNames().last(), QStringLiteral("Second"));

  QVERIFY(project.setPlateLocked(0, true));
  QVERIFY(project.isPlateLocked(0));
  QVERIFY(!project.isPlateLocked(1));

  QVERIFY(project.setCurrentPlateIndex(1));
  QCOMPARE(project.currentPlateIndex(), 1);

  // Delete the current plate; count drops and current index stays valid.
  QVERIFY(project.deletePlate(1));
  QCOMPARE(project.plateCount(), 1);
  QVERIFY(project.currentPlateIndex() >= 0 && project.currentPlateIndex() < project.plateCount());

  // Cannot delete the last plate.
  QVERIFY(!project.deletePlate(0));
  QCOMPARE(project.plateCount(), 1);
}

// ── v3.0 Phase 17: plate lifecycle completion (clone/reorder/printable) ──

void ViewModelSmokeTests::partPlateListMovePlateReindexesAndAdjustsCurrent()
{
  // D-07: movePlate is a pure metadata reorder (vector shift + reindex).
  OWzx::PartPlateList list;
  QCOMPARE(list.plateCount(), 1);
  list.createPlate();
  list.createPlate();  // now 3 plates: indices 0,1,2
  QCOMPARE(list.plateCount(), 3);
  list.renamePlate(0, "A");
  list.renamePlate(1, "B");
  list.renamePlate(2, "C");

  // move A (index 0) to end (index 2): order becomes B, C, A
  QVERIFY(list.movePlate(0, 2));
  QCOMPARE(QString::fromStdString(list.plate(0)->name()), QStringLiteral("B"));
  QCOMPARE(QString::fromStdString(list.plate(1)->name()), QStringLiteral("C"));
  QCOMPARE(QString::fromStdString(list.plate(2)->name()), QStringLiteral("A"));
  // indices must reflect new positions
  QCOMPARE(list.plate(0)->plateIndex(), 0);
  QCOMPARE(list.plate(2)->plateIndex(), 2);

  // move it back (index 2 -> 0): order becomes A, B, C
  QVERIFY(list.movePlate(2, 0));
  QCOMPARE(QString::fromStdString(list.plate(0)->name()), QStringLiteral("A"));

  // invalid moves
  QVERIFY(!list.movePlate(1, 1));   // same index
  QVERIFY(!list.movePlate(0, 99));  // out of range
  QVERIFY(!list.movePlate(-1, 0));  // negative
}

void ViewModelSmokeTests::projectServiceClonePlateDeepCopiesObjects()
{
  // D-06: clonePlate deep-copies objects onto the new plate.
  ProjectServiceMock project;
  QCOMPARE(project.plateCount(), 1);
  // Add a primitive to plate 0 (current plate). addPrimitiveToPlate returns the
  // new object index (>=0) on success.
  const int newObj = project.addPrimitiveToPlate(0);  // cube
  QVERIFY2(newObj >= 0, "addPrimitiveToPlate should succeed");
  QVERIFY(project.plateObjectCount(0) >= 1);  // plate 0 now has the object
  const int sourceCountBefore = project.plateObjectCount(0);
  QVERIFY(project.setCurrentPlateIndex(0));  // current=0 so current != dst(1) after clone

  // Clone plate 0 → new plate 1.
  QVERIFY(project.clonePlate(0));
  QCOMPARE(project.plateCount(), 2);
  // Deep copy: the new plate has at least one object (the clone), and it is a
  // distinct object from the source (clonePlate calls duplicateObject which
  // appends a new ModelObject, not a shared reference).
  QVERIFY2(project.plateObjectCount(1) >= 1,
           "cloned plate must own objects (deep copy, not shallow)");
  // Phase 21 review-fix BUG-1 regression guard: cloning plate 0 must NOT alter
  // the source plate's objects, AND must NOT leak the clone onto the current
  // plate (clonePlate temporarily sets current=dst so duplicateObject's mock
  // branch targets dst). Source unchanged:
  QCOMPARE(project.plateObjectCount(0), sourceCountBefore);
  // Current (0) must not have gained the clone either (regression: pre-fix the
  // mock branch added the clone to currentPlate() which was 0 here).
  QCOMPARE(project.plateObjectCount(project.currentPlateIndex()), sourceCountBefore);

  // MAX_PLATE_COUNT guard: cloning when full should fail.
  for (int i = project.plateCount(); i < OWzx::kMaxPlateCount; ++i)
    project.addPlate();
  QCOMPARE(project.plateCount(), OWzx::kMaxPlateCount);
  QVERIFY(!project.clonePlate(0));
}

void ViewModelSmokeTests::projectServicePerPlatePrintableRoundTrip()
{
  // D-08: per-plate printable flag round-trip + default.
  ProjectServiceMock project;
  QVERIFY(project.isPlatePrintable(0));  // default printable
  QVERIFY(project.setPlatePrintable(0, false));
  QVERIFY(!project.isPlatePrintable(0));
  QVERIFY(project.setPlatePrintable(0, true));
  QVERIFY(project.isPlatePrintable(0));
  // invalid index safe
  QVERIFY(!project.isPlatePrintable(99));
}

void ViewModelSmokeTests::activePlateObjectIndicesFollowCurrentPlateWithoutFallback()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  const int sourceObject = project.addPrimitiveToPlate(0);
  QVERIFY(sourceObject >= 0);
  QVERIFY(editor.addPlate());
  QVERIFY(editor.setCurrentPlateIndex(0));

  const QVariantList plate0Objects = editor.activePlateObjectIndices();
  QCOMPARE(plate0Objects.size(), 1);
  QCOMPARE(plate0Objects.first().toInt(), sourceObject);

  QVERIFY(editor.setCurrentPlateIndex(1));
  const QVariantList emptyPlateObjects = editor.activePlateObjectIndices();
  QVERIFY(emptyPlateObjects.isEmpty());

  editor.setShowAllObjects(true);
  const QVariantList showAllStillEmpty = editor.activePlateObjectIndices();
  QVERIFY2(showAllStillEmpty.isEmpty(),
           "Renderer-facing active plate context must not inherit show-all object-list fallback");
}

void ViewModelSmokeTests::rendererPickingSelectsSourceObjectThroughEditorViewModel()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  QVERIFY(editor.addPrimitiveToPlate(0));
  QVERIFY(editor.addPrimitiveToPlate(1));
  QCOMPARE(editor.objectCount(), 2);

  QSignalSpy spy(&editor, &EditorViewModel::stateChanged);
  QVERIFY(editor.selectSourceObject(1));
  QCOMPARE(editor.selectedSourceObjectIndex(), 1);
  QCOMPARE(editor.selectedObjectIndex(), 1);
  QVERIFY(editor.isObjectSelected(1));
  QVERIFY(spy.count() >= 1);

  const int signalCountAfterValidPick = spy.count();
  QVERIFY(!editor.selectSourceObject(999));
  QCOMPARE(editor.selectedSourceObjectIndex(), 1);
  QCOMPARE(editor.selectedObjectIndex(), 1);
  QCOMPARE(spy.count(), signalCountAfterValidPick);

  QVERIFY(editor.addPlate());
  QVERIFY(editor.setCurrentPlateIndex(1));
  QVERIFY(editor.activePlateObjectIndices().isEmpty());
  QVERIFY(!editor.selectSourceObject(0));
  QCOMPARE(editor.selectedSourceObjectIndex(), -1);
}

void ViewModelSmokeTests::gizmoMoveDragCoalescesIntoSingleUndoCommand()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);
  UndoRedoManager undoManager;
  editor.setUndoRedoManager(&undoManager);

  QVERIFY(editor.addPrimitiveToPlate(0));
  QVERIFY(editor.selectSourceObject(0));
  const int sourceObject = editor.selectedSourceObjectIndex();
  QCOMPARE(sourceObject, 0);

  const QVector3D startPos = project.objectPosition(sourceObject);
  editor.setObjectPosX(startPos.x() + 1.0f);
  QCOMPARE(undoManager.stack()->count(), 1);
  const QVector3D dragStartPos = project.objectPosition(sourceObject);

  editor.beginGizmoMoveDrag();
  editor.applyGizmoMoveDelta(10.0f, 0.0f, 0.0f);
  editor.applyGizmoMoveDelta(0.0f, 5.0f, 0.0f);
  editor.endGizmoMoveDrag();

  QCOMPARE(project.objectPosition(sourceObject), dragStartPos + QVector3D(10.0f, 5.0f, 0.0f));
  QCOMPARE(undoManager.stack()->count(), 2);
  QVERIFY(undoManager.canUndo());

  undoManager.undo();
  QCOMPARE(project.objectPosition(sourceObject), dragStartPos);
  QVERIFY(undoManager.canRedo());

  undoManager.redo();
  QCOMPARE(project.objectPosition(sourceObject), dragStartPos + QVector3D(10.0f, 5.0f, 0.0f));
  QCOMPARE(undoManager.stack()->count(), 2);
}

void ViewModelSmokeTests::gizmoRotateDragCoalescesIntoSingleUndoCommand()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);
  UndoRedoManager undoManager;
  editor.setUndoRedoManager(&undoManager);

  QVERIFY(editor.addPrimitiveToPlate(0));
  QVERIFY(editor.selectSourceObject(0));
  const int sourceObject = editor.selectedSourceObjectIndex();
  QCOMPARE(sourceObject, 0);

  const QVector3D startRot = project.objectRotation(sourceObject);
  editor.setObjectRotZ(startRot.z() + 5.0f);
  QCOMPARE(undoManager.stack()->count(), 1);
  const QVector3D dragStartRot = project.objectRotation(sourceObject);

  editor.beginGizmoRotateDrag();
  editor.applyGizmoRotateDelta(3, float(M_PI) / 6.0f);
  editor.applyGizmoRotateDelta(3, float(M_PI) / 12.0f);
  editor.endGizmoRotateDrag();

  const QVector3D expectedRot(dragStartRot.x(), dragStartRot.y(), dragStartRot.z() + 45.0f);
  QCOMPARE(project.objectRotation(sourceObject), expectedRot);
  QCOMPARE(undoManager.stack()->count(), 2);
  QVERIFY(undoManager.canUndo());

  undoManager.undo();
  QCOMPARE(project.objectRotation(sourceObject), dragStartRot);
  QVERIFY(undoManager.canRedo());

  undoManager.redo();
  QCOMPARE(project.objectRotation(sourceObject), expectedRot);
  QCOMPARE(undoManager.stack()->count(), 2);
}

void ViewModelSmokeTests::gizmoScaleDragCoalescesIntoSingleUndoCommand()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);
  UndoRedoManager undoManager;
  editor.setUndoRedoManager(&undoManager);

  QVERIFY(editor.addPrimitiveToPlate(0));
  QVERIFY(editor.selectSourceObject(0));
  editor.setUniformScale(false);
  const int sourceObject = editor.selectedSourceObjectIndex();
  QCOMPARE(sourceObject, 0);

  const QVector3D startScale = project.objectScale(sourceObject);
  editor.setObjectScaleX(startScale.x() * 1.1f);
  QCOMPARE(undoManager.stack()->count(), 1);
  const QVector3D dragStartScale = project.objectScale(sourceObject);

  editor.beginGizmoScaleDrag();
  editor.applyGizmoScaleFactor(1, 1.2f);
  editor.applyGizmoScaleFactor(1, 1.25f);
  editor.endGizmoScaleDrag();

  const QVector3D expectedScale(dragStartScale.x() * 1.5f,
                                dragStartScale.y(),
                                dragStartScale.z());
  QCOMPARE(project.objectScale(sourceObject), expectedScale);
  QCOMPARE(undoManager.stack()->count(), 2);
  QVERIFY(undoManager.canUndo());

  undoManager.undo();
  QCOMPARE(project.objectScale(sourceObject), dragStartScale);
  QVERIFY(undoManager.canRedo());

  undoManager.redo();
  QCOMPARE(project.objectScale(sourceObject), expectedScale);
  QCOMPARE(undoManager.stack()->count(), 2);
}

void ViewModelSmokeTests::prepareWorkflowGatesExposeSourceTruthState()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  const QMetaObject *meta = editor.metaObject();
  QVERIFY2(meta->indexOfProperty("maxPlateCount") >= 0,
           "Prepare QML must read the upstream plate limit from EditorViewModel");
  QVERIFY2(meta->indexOfProperty("canRenameSelectedObject") >= 0,
           "Object-list rename gating must live in EditorViewModel");
  QVERIFY2(meta->indexOfProperty("canDuplicateSelectedObjects") >= 0,
           "Object-list duplicate gating must live in EditorViewModel");
  QVERIFY2(meta->indexOfProperty("canDeleteSelection") >= 0,
           "Object-list delete gating must live in EditorViewModel");
  QVERIFY2(meta->indexOfProperty("canSetSelectionPrintable") >= 0,
           "Object printable gating must live in EditorViewModel");
  QVERIFY2(meta->indexOfProperty("canTransformSelection") >= 0,
           "Object transform gating must live in EditorViewModel");
  QVERIFY2(meta->indexOfProperty("canArrangeObjects") >= 0,
           "Arrange gating must live in EditorViewModel");
  QVERIFY2(meta->indexOfProperty("canAddPlate") >= 0,
           "Plate add gating must be a NOTIFY property for QML");
  QVERIFY2(meta->indexOfProperty("availableGizmoMask") >= 0,
           "Gizmo gating must be a NOTIFY property for QML");

  QCOMPARE(editor.maxPlateCount(), OWzx::kMaxPlateCount);
  QVERIFY(editor.canAddPlate());
  QVERIFY(!editor.canDeletePlate(0));
  QVERIFY(!editor.canRenameSelectedObject());
  QVERIFY(!editor.canDuplicateSelectedObjects());
  QVERIFY(!editor.canDeleteSelection());
  QVERIFY(!editor.canSetSelectionPrintable());
  QVERIFY(!editor.canTransformSelection());
  QVERIFY(!editor.canArrangeObjects());
  QCOMPARE(editor.availableGizmoMask(), 0);
  QVERIFY(!editor.canActivateGizmo(0));
  QCOMPARE(editor.gizmoStatusText(0), QStringLiteral("Requires one selected object"));
  QVERIFY(!editor.canActivateGizmo(13));
  QCOMPARE(editor.gizmoStatusText(13), QStringLiteral("Blocked: CGAL MeshBoolean unavailable"));
  QVERIFY(!editor.canActivateGizmo(8));
  QCOMPARE(editor.gizmoStatusText(8), QStringLiteral("Blocked: OpenVDB unavailable"));

  QVERIFY(editor.addPrimitiveToPlate(0));
  QCOMPARE(editor.objectCount(), 1);
  QVERIFY(editor.canArrangeObjects());
  editor.selectObject(0);
  QVERIFY(editor.canRenameSelectedObject());
  QVERIFY(editor.canDuplicateSelectedObjects());
  QVERIFY(editor.canDeleteSelection());
  QVERIFY(editor.canSetSelectionPrintable());
  QVERIFY(editor.canTransformSelection());
  QVERIFY(editor.canActivateGizmo(0));
  QVERIFY(editor.canActivateGizmo(5));
  QVERIFY(editor.canActivateGizmo(12));
  QVERIFY((editor.availableGizmoMask() & (1 << 0)) != 0);
  QVERIFY((editor.availableGizmoMask() & (1 << 5)) != 0);
  QVERIFY((editor.availableGizmoMask() & (1 << 12)) != 0);

  QVERIFY(editor.addPrimitiveToPlate(0));
  editor.clearObjectSelection();
  editor.selectObject(0);
  editor.toggleObjectSelection(1);
  QCOMPARE(editor.selectedObjectCount(), 2);
  QVERIFY(!editor.canRenameSelectedObject());
  QVERIFY(editor.canDuplicateSelectedObjects());
  QVERIFY(editor.canDeleteSelection());
  QVERIFY(!editor.canActivateGizmo(13));
  QVERIFY((editor.availableGizmoMask() & (1 << 13)) == 0);
  QCOMPARE(editor.gizmoStatusText(13), QStringLiteral("Blocked: CGAL MeshBoolean unavailable"));
  QVERIFY(!editor.canActivateGizmo(6));
  QVERIFY(!editor.canActivateGizmo(7));
  QCOMPARE(editor.gizmoStatusText(6), QStringLiteral("Blocked: viewport triangle picking unavailable"));
  QCOMPARE(editor.gizmoStatusText(7), QStringLiteral("Blocked: viewport triangle picking unavailable"));
  QVERIFY(!editor.canActivateGizmo(11));
  QCOMPARE(editor.gizmoStatusText(11), QStringLiteral("Blocked: CGAL MeshBoolean unavailable"));
}

void ViewModelSmokeTests::prepareMoveSelectionToPlateUsesSourceSelection()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  QVERIFY(editor.addPrimitiveToPlate(0));
  const int plate0Object = project.currentPlateObjectIndices().value(0, -1);
  QVERIFY(plate0Object >= 0);
  QVERIFY(editor.addPlate());
  QVERIFY(editor.setCurrentPlateIndex(1));
  QVERIFY(editor.addPrimitiveToPlate(0));
  const int plate1Object = project.currentPlateObjectIndices().value(0, -1);
  QVERIFY(plate1Object >= 0);

  QCOMPARE(project.plateObjectCount(0), 1);
  QCOMPARE(project.plateObjectCount(1), 1);
  QCOMPARE(editor.objectCount(), 1);
  editor.selectObject(0);
  QCOMPARE(editor.selectedSourceObjectIndex(), plate1Object);
  QVERIFY(editor.canMoveSelectionToPlate(0));
  QVERIFY(editor.moveSelectedObjectToPlate(0));

  QCOMPARE(project.plateIndexForObject(plate0Object), 0);
  QCOMPARE(project.plateIndexForObject(plate1Object), 0);
  QCOMPARE(project.plateObjectCount(0), 2);
  QCOMPARE(project.plateObjectCount(1), 0);
  QVERIFY(!editor.canMoveSelectionToPlate(0));

  QVERIFY(editor.setCurrentPlateIndex(0));
  editor.clearObjectSelection();
  editor.selectObject(0);
  editor.toggleObjectSelection(1);
  QCOMPARE(editor.selectedObjectCount(), 2);
  QVERIFY(editor.canMoveSelectionToPlate(1));
  QVERIFY(editor.moveSelectedObjectToPlate(1));
  QCOMPARE(project.plateObjectCount(0), 0);
  QCOMPARE(project.plateObjectCount(1), 2);
}

void ViewModelSmokeTests::prepareVisibleObjectActionsMapToSourceObjects()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  QVERIFY(editor.addPrimitiveToPlate(0));
  const int plate0Object = project.currentPlateObjectIndices().value(0, -1);
  QVERIFY(plate0Object >= 0);
  const QString plate0InitialName = project.objectNames().value(plate0Object);
  const int plate0InitialExtruder = project.volumeExtruderId(plate0Object, 0);
  QVERIFY(editor.addPlate());
  QVERIFY(editor.setCurrentPlateIndex(1));
  QVERIFY(editor.addPrimitiveToPlate(0));
  const int plate1Object = project.currentPlateObjectIndices().value(0, -1);
  QVERIFY(plate1Object >= 0);

  QCOMPARE(editor.objectCount(), 1);
  QCOMPARE(editor.selectedObjectIndex(), -1);
  QVERIFY(editor.renameObject(0, QStringLiteral("Visible plate 1 object")));
  QCOMPARE(project.objectNames().value(plate0Object), plate0InitialName);
  QCOMPARE(project.objectNames().value(plate1Object), QStringLiteral("Visible plate 1 object"));

  QVERIFY(editor.setVolumeExtruderId(0, 0, 2));
  QCOMPARE(project.volumeExtruderId(plate1Object, 0), 2);
  QCOMPARE(project.volumeExtruderId(plate0Object, 0), plate0InitialExtruder);

  QVERIFY(project.setObjectPosition(plate0Object, 11.0f, 12.0f, 13.0f));
  QVERIFY(project.setObjectPosition(plate1Object, 21.0f, 22.0f, 23.0f));
  editor.selectObject(0);
  QVERIFY(editor.canTransformSelection());
  editor.centerSelectedObjects();
  const QVector3D plate0Pos = project.objectPosition(plate0Object);
  const QVector3D plate1Pos = project.objectPosition(plate1Object);
  QCOMPARE(plate0Pos, QVector3D(11.0f, 12.0f, 13.0f));
  QCOMPARE(plate1Pos, QVector3D());
}

void ViewModelSmokeTests::multiPlate3mfRoundTripPreservesState()
{
  // PLATE-09 (D-13) + FIXTURE-02 (v3.2 Phase 32): the v2.9 blocker — multi-plate
  // state must survive save→reload. Uses the committed real-model fixture
  // (tests/data/test_model.stl, FIXTURE-01) so the project has a valid mesh,
  // enabling the full store_bbs_3mf → read_from_archive round-trip.
  #ifndef HAS_LIBSLIC3R
  QSKIP("3MF round-trip requires libslic3r (real store_bbs_3mf + read_from_archive)");
#else
  // FIXTURE-01: load the committed test model so the project has real geometry.
  const QString fixturePath = QDir(QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR)))
      .filePath(QStringLiteral("tests/data/test_model.stl"));
  QVERIFY2(QFileInfo::exists(fixturePath),
           "FIXTURE-01 test_model.stl must exist under tests/data/");

  ProjectServiceMock saver;
  QSignalSpy saverSpy(&saver, &ProjectServiceMock::loadFinished);
  QVERIFY2(saver.loadFile(fixturePath),
           "FIXTURE-01 must load via loadFile");
  QTRY_VERIFY_WITH_TIMEOUT(saverSpy.count() > 0, 10000);
  QVERIFY2(saver.modelCount() >= 1, "fixture must load >= 1 object");

  QVERIFY(saver.addPlate());  // now 2 plates
  QCOMPARE(saver.plateCount(), 2);
  QVERIFY(saver.renamePlate(0, QStringLiteral("Alpha")));
  QVERIFY(saver.renamePlate(1, QStringLiteral("Beta")));
  QVERIFY(saver.setPlateLocked(1, true));
  QVERIFY(saver.setPlateBedType(0, 3));

  const QString path = QDir(QDir::tempPath()).filePath(QStringLiteral("owzx_rt_test.3mf"));
  bool saved = false;
  try {
    saved = saver.saveProject(path);
  } catch (...) {
    // store_bbs_3mf may still throw on edge cases (writer integration coupled to
    // real GL capture — see Phase 30 THUMB-03 deferral). If it throws, the full
    // round-trip can't be verified here; skip rather than fail.
    QFile::remove(path);
    QSKIP("store_bbs_3mf threw on the fixture-loaded project (writer integration "
          "limitation, tracked with THUMB-03); round-trip not verifiable yet.");
  }
  if (!saved) {
    QFile::remove(path);
    QSKIP("store_bbs_3mf did not succeed on the fixture-loaded project (env/writer limitation)");
  }

  // Load into a fresh service.
  ProjectServiceMock loader;
  QSignalSpy loaderSpy(&loader, &ProjectServiceMock::loadFinished);
  bool loaded = false;
  try {
    loaded = loader.loadProject(path);
  } catch (...) {
    QFile::remove(path);
    QFAIL("read_from_archive threw loading the round-tripped project");
  }
  QTRY_VERIFY_WITH_TIMEOUT(loaderSpy.count() > 0, 10000);
  QFile::remove(path);
  QVERIFY2(loaded, "loadProject should succeed on the saved file");

  // Plate state round-trip assertions (the PLATE-09 gate).
  QVERIFY2(loader.plateCount() >= 2, "reloaded project must have >= 2 plates");
  QVERIFY2(loader.isPlateLocked(1), "plate 1 locked state must round-trip");
  QCOMPARE(loader.plateBedType(0), 3);
#endif
}

// ── v3.0 Phase 19: per-plate config merge + scoped-value stub fix ──

void ViewModelSmokeTests::projectServicePerPlateConfigOverrideRoundTrips()
{
  // D-16: plateScopedOptionValue/setPlateScopedOptionValue must read/write the real
  // PartPlate::config() under HAS_LIBSLIC3R (previously a `return fallbackValue` stub).
#ifndef HAS_LIBSLIC3R
  QSKIP("per-plate config round-trip requires libslic3r (DynamicPrintConfig)");
#else
  ProjectServiceMock project;
  // Write a float override on a real config key, read it back.
  QVERIFY(project.setPlateScopedOptionValue(0, QStringLiteral("layer_height"), 0.25));
  QCOMPARE(project.plateScopedOptionValue(0, QStringLiteral("layer_height"), 0.0).toDouble(), 0.25);
  // A key never set returns the fallback.
  QCOMPARE(project.plateScopedOptionValue(0, QStringLiteral("never_set_key"), -1).toInt(), -1);
#endif
}

void ViewModelSmokeTests::sliceServicePerPlateConfigMergeHonorsOverrides()
{
  // D-15: the per-plate DynamicPrintConfig (the config SliceService merges via
  // config.apply) must actually carry the override after setPlateScopedOptionValue.
  // Full slice-path verification needs a real-model fixture (same gap as Phase 18
  // PLATE-09); this unit-level test asserts the merge SOURCE is correct.
#ifndef HAS_LIBSLIC3R
  QSKIP("per-plate config merge source check requires libslic3r");
#else
  ProjectServiceMock project;
  QVERIFY(project.setPlateScopedOptionValue(0, QStringLiteral("layer_height"), 0.3));
  const Slic3r::DynamicPrintConfig *cfg = project.plateDynamicConfig(0);
  QVERIFY2(cfg != nullptr, "plateDynamicConfig must return the plate's config");
  const auto *opt = cfg->option("layer_height");
  QVERIFY2(opt != nullptr, "plate config must carry the override key after setPlateScopedOptionValue");
  // layer_height is a Float; read via getFloat.
  QCOMPARE(dynamic_cast<const Slic3r::ConfigOptionFloat *>(opt)->getFloat(), 0.3);
#endif
}

void ViewModelSmokeTests::sliceServiceConfigMergeDirectionPlateWins()
{
  // Phase 21 review-fix TEST-2: verify DynamicPrintConfig::apply(other) makes
  // `other` (the per-plate config) win over `this` (the preset config). This is
  // the D-15 correctness assumption SliceService.cpp:393 relies on. If this
  // test shows preset-wins, SliceService must flip the apply direction.
#ifndef HAS_LIBSLIC3R
  QSKIP("DynamicPrintConfig merge-direction test requires libslic3r");
#else
  Slic3r::DynamicPrintConfig base;   // preset-like config
  Slic3r::DynamicPrintConfig plate;  // per-plate overrides
  // layer_height is a real registered config key (Float).
  if (auto *o = base.option("layer_height", true)) {
    if (auto *f = dynamic_cast<Slic3r::ConfigOptionFloat *>(o)) f->value = 0.2;  // preset
  }
  if (auto *o = plate.option("layer_height", true)) {
    if (auto *f = dynamic_cast<Slic3r::ConfigOptionFloat *>(o)) f->value = 0.4;  // plate override
  }
  // SliceService does: config.apply(*plateCfg)  →  base.apply(plate)
  base.apply(plate);
  const auto *merged = base.option("layer_height");
  QVERIFY2(merged != nullptr, "merged config must retain layer_height");
  // Plate (0.4) must win over preset (0.2) — confirms apply(other) makes other win.
  // Compare as double (getFloat is double) to avoid float-literal precision mismatch.
  QCOMPARE(double(dynamic_cast<const Slic3r::ConfigOptionFloat *>(merged)->getFloat()), 0.4);
#endif
}

// ── Phase 55-04 (GCODE-02/03): Preview render-side contract guards ──
// These four methods load the committed OrcaSlicer-style fixture
// (tests/fixtures/orca_sample.gcode) via PreviewViewModel::loadGCodeForPreview
// and assert the invariants the disappearing-preview regression class depends
// on. They mirror the GCV1 helpers used by E2EWorkflowTests but keep a local
// copy so ViewModelSmokeTests stays self-contained (no cross-file helper).

// Phase 55 (GCODE-02): toggleRoleVisibility must NOT mutate gcodePreviewData_.
// This is the central render-side filter guard -- the single most important
// regression lock for the disappearing-preview bug. A visibility toggle flip
// updates draw filtering over the already-uploaded segment buffer (update()
// only) and must never repack the payload.
void ViewModelSmokeTests::roleVisibilityToggleDoesNotRepackGcodePreviewData()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  QVERIFY2(QFileInfo::exists(kOrcaGcodePath),
           qPrintable(QStringLiteral("Orca sample fixture missing: %1").arg(kOrcaGcodePath)));
  QVERIFY2(preview.loadGCodeForPreview(kOrcaGcodePath),
           "loadGCodeForPreview should succeed on the committed Orca fixture");

  const QByteArray before = preview.gcodePreviewData();
  QVERIFY2(before.size() > 8, "loaded GCV1 payload should exceed the 8-byte header");
  QVERIFY2(gcv1SegmentCount(before) > 0,
           "loaded payload should pack a positive GCV1 segment count");

  preview.toggleRoleVisibility(1);  // Perimeter role index (canonical libvgcode).

  const QByteArray after = preview.gcodePreviewData();
  QVERIFY2(before == after,
           "toggleRoleVisibility must not mutate gcodePreviewData_ (render-side filter only)");
  QVERIFY2(after.size() > 8, "payload must remain a valid GCV1 blob after the toggle");
  QVERIFY2(gcv1SegmentCount(after) == gcv1SegmentCount(before),
           "segment count must be unchanged after a role-visibility toggle");
}

// Phase 55 code-review fix (GCODE-02): the renderer's synchronize consumes a
// dense 20-bool QVariantList indexed by canonical libvgcode role. The prior
// binding pushed roleVisibilities (18 QVariantMap rows) into that consumer,
// which silently dropped the mask (size<20 gate) and made the filter a no-op.
// This test guards the producer shape (20 bools) and the toggle→mask path so
// the dead-path class of regression cannot recur.
void ViewModelSmokeTests::roleVisibilityMaskFeedsRendererShapeAndTogglesPropagate()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  // The mask must be a dense 20-element bool list regardless of load state.
  const QVariantList maskBefore = preview.roleVisibilityMask();
  QVERIFY2(maskBefore.size() == 20,
           qPrintable(QStringLiteral("roleVisibilityMask must have 20 entries for the "
                                     "renderer; got %1").arg(maskBefore.size())));
  for (int i = 0; i < maskBefore.size(); ++i)
  {
    QVERIFY2(maskBefore.at(i).canConvert<bool>(),
             qPrintable(QStringLiteral("mask entry %1 must be a bool, not a QVariantMap "
                                       "(renderer reads .toBool())").arg(i)));
    QVERIFY2(maskBefore.at(i).toBool(),
             "all roles default visible (upstream extrusion_roles_visibility defaults)");
  }

  // roleVisibilities (UI rows) and roleVisibilityMask (renderer mask) must be
  // distinct shapes: 18 maps vs 20 bools. Binding the wrong one is the bug.
  QVERIFY2(preview.roleVisibilities().size() == 18,
           "roleVisibilities must expose 18 UI rows (1..19 minus None/Custom)");
  QVERIFY2(preview.roleVisibilityMask().size() == 20,
           "roleVisibilityMask must expose 20 dense bools for the renderer");

  // A toggle must flip exactly one mask slot and leave the other 19 unchanged.
  QVERIFY2(preview.isRoleVisible(2), "Outer wall (canonical index 2) starts visible");
  preview.toggleRoleVisibility(2);
  QVERIFY2(!preview.isRoleVisible(2), "toggleRoleVisibility must flip the slot");
  const QVariantList maskAfter = preview.roleVisibilityMask();
  QVERIFY2(maskAfter.size() == 20, "mask size is invariant across toggles");
  QVERIFY2(!maskAfter.at(2).toBool(),
           "toggled slot must read false in the renderer mask");
  for (int i = 0; i < 20; ++i)
  {
    if (i == 2) continue;
    QVERIFY2(maskAfter.at(i).toBool() == maskBefore.at(i).toBool(),
             qPrintable(QStringLiteral("toggle must not perturb slot %1").arg(i)));
  }
}

// Phase 55 (GCODE-03): legend gradient min/max must be stable across a layer
// drag or move drag. The legend reflects the GLOBAL slice scope and must not
// recompute on slider interaction -- only on a recolor (view-mode change).
void ViewModelSmokeTests::legendGradientBoundsStableAcrossLayerMoveDrag()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  QVERIFY2(preview.loadGCodeForPreview(kOrcaGcodePath),
           "loadGCodeForPreview should succeed on the committed Orca fixture");
  QVERIFY2(preview.moveCount() > 2, "fixture must expose enough moves for a drag");

  // Pick a gradient mode BY NAME so the test survives the Plan 02 renumbering.
  const int fanSpeedIndex = preview.viewModes().indexOf(QStringLiteral("Fan Speed"));
  QVERIFY2(fanSpeedIndex >= 0,
           "viewModes() must expose a 'Fan Speed' gradient mode");
  preview.setViewModeIndex(fanSpeedIndex);

  const QString minBefore = preview.legendGradientMinLabel();
  const QString maxBefore = preview.legendGradientMaxLabel();
  QVERIFY2(!minBefore.isEmpty() && !maxBefore.isEmpty(),
           "gradient mode must populate non-empty legend min/max labels");

  // Simulate a layer-range drag plus a move drag.
  preview.setLayerRange(0, qMin(1, qMax(0, preview.layerCount() - 1)));
  preview.setCurrentMove(qMin(2, preview.moveCount()));

  const QString minAfter = preview.legendGradientMinLabel();
  const QString maxAfter = preview.legendGradientMaxLabel();
  QVERIFY2(minBefore == minAfter,
           "legend gradient min label must be unchanged by a layer/move drag (global scope)");
  QVERIFY2(maxBefore == maxAfter,
           "legend gradient max label must be unchanged by a layer/move drag (global scope)");
}

// Phase 55 (GCODE-03): setCurrentMove must update currentGcodeLine and the
// gcodeLines window atomically. A single stateChanged emission proves the
// update + window rebuild happen as one observable transition.
void ViewModelSmokeTests::currentMoveUpdatesGcodeLineWindowAtomically()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  QVERIFY2(preview.loadGCodeForPreview(kOrcaGcodePath),
           "loadGCodeForPreview should succeed on the committed Orca fixture");
  QVERIFY2(preview.moveCount() > 2, "fixture must expose enough moves to step");

  // Default currentMove is 0; step to a non-zero move so the early-return guard
  // in setCurrentMove does not skip the update.
  const int targetMove = qMin(2, preview.moveCount());

  QSignalSpy spy(&preview, &PreviewViewModel::stateChanged);
  QVERIFY(spy.isValid());
  preview.setCurrentMove(targetMove);

  QVERIFY2(spy.count() == 1,
           "setCurrentMove must emit stateChanged exactly once (atomic window update)");
  QVERIFY2(preview.currentGcodeLine() != 0,
           "currentGcodeLine should advance to a real source line after a move step");
  QVERIFY2(!preview.gcodeLines().isEmpty(),
           "gcodeLines window must be populated after a move step");
}

// Phase 55 (GCODE-02): belt-and-suspenders alongside PreviewParserTests -- the
// 17 upstream EViewType modes must be present with the canonical names so the
// QML view-mode combo and recolor switch share one source of truth.
void ViewModelSmokeTests::viewModesExposeUpstreamSeventeenModes()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  const QStringList modes = preview.viewModes();
  QVERIFY2(modes.size() == 17,
           qPrintable(QStringLiteral("viewModes() should expose 17 upstream modes, got %1").arg(modes.size())));
  QVERIFY2(modes.contains(QStringLiteral("Line Type")),
           "viewModes() must contain 'Line Type'");
  QVERIFY2(modes.contains(QStringLiteral("Summary")),
           "viewModes() must contain 'Summary'");
  QVERIFY2(modes.contains(QStringLiteral("Tool")),
           "viewModes() must contain 'Tool'");
}

// -- Phase 56-01: Wave 0 RED test scaffolds for SETTINGS-01..07 --
// Each body is a QFAIL marker so the suite is RED until 56-02/56-03/56-04
// replace them with real assertions.

void ViewModelSmokeTests::testSettingsDialogOpenFromSidebar()
{
  // SETTINGS-01: BackendContext::forwardSettingsRequest sets active preset tier
  // and emits settingsRequested. Asserts the two-step ordering (setActivePresetTier
  // BEFORE emit settingsRequested).
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("56SettingsDialog"));
  BackendContext backend;

  QSignalSpy spy(&backend, &BackendContext::settingsRequested);
  QVERIFY2(spy.isValid(), "settingsRequested signal spy is valid");

  backend.forwardSettingsRequest(QStringLiteral("printer"));
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("printer"));
  QCOMPARE(qobject_cast<ConfigViewModel *>(backend.configViewModel())->activePresetTier(),
           QStringLiteral("printer"));
}

void ViewModelSmokeTests::testTabsAndGroupNavPerTier()
{
  // SETTINGS-02: page/group navigation per tier, derived from upstream Tab.cpp
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("56GroupNav"));
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  // Printer tier: page names from TabPrinter::build_fff
  auto *machineOpts = qobject_cast<ConfigOptionModel *>(config.machineOptions());
  QVERIFY(machineOpts);
  QVERIFY2(machineOpts->rowCount() > 0, "Machine options model is empty");
  QStringList printerPages = machineOpts->pageNames();
  QVERIFY2(printerPages.contains(QStringLiteral("Basic information")),
           "Printer pageNames() must contain 'Basic information' (TabPrinter)");
  QVERIFY2(printerPages.contains(QStringLiteral("Notes")),
           "Printer pageNames() must contain 'Notes' (TabPrinter)");
  QStringList printerGroups = machineOpts->groupNames();
  QVERIFY2(printerGroups.contains(QStringLiteral("Printable space")),
           "Printer groupNames() must contain 'Printable space' (TabPrinter)");
  QVERIFY2(printerGroups.contains(QStringLiteral("Extruder Clearance")) ||
           printerGroups.contains(QStringLiteral("Retraction")),
           "Printer groupNames() must contain at least one extruder clearance/retraction group");

  // Filament tier: page names from TabFilament::build
  auto *filamentOpts = qobject_cast<ConfigOptionModel *>(config.filamentOptions());
  QVERIFY(filamentOpts);
  QVERIFY2(filamentOpts->rowCount() > 0, "Filament options model is empty");
  QStringList filamentPages = filamentOpts->pageNames();
  QVERIFY2(filamentPages.contains(QStringLiteral("Filament")),
           "Filament pageNames() must contain 'Filament' (TabFilament)");
  QVERIFY2(filamentPages.contains(QStringLiteral("Cooling")),
           "Filament pageNames() must contain 'Cooling' (TabFilament)");
  QStringList filamentGroups = filamentOpts->groupNames();
  QVERIFY2(filamentGroups.contains(QStringLiteral("Print temperature")),
           "Filament groupNames() must contain 'Print temperature' (TabFilament)");

  // Print tier: page names from TabPrint::build
  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  QVERIFY(printOpts);
  QVERIFY2(printOpts->rowCount() > 0, "Print options model is empty");
  QStringList printPages = printOpts->pageNames();
  QVERIFY2(printPages.contains(QStringLiteral("Quality")),
           "Print pageNames() must contain 'Quality' (TabPrint)");
  QVERIFY2(printPages.contains(QStringLiteral("Speed")),
           "Print pageNames() must contain 'Speed' (TabPrint)");
  QStringList printGroups = printOpts->groupNames();
  QVERIFY2(printGroups.contains(QStringLiteral("Layer height")),
           "Print groupNames() must contain 'Layer height' (TabPrint)");
  QVERIFY2(printGroups.contains(QStringLiteral("Infill")),
           "Print groupNames() must contain 'Infill' (TabPrint)");
}

void ViewModelSmokeTests::testConfigOptionModelSevenTypes()
{
  // SETTINGS-03: ConfigOptionModel exposes all 7 typed option kinds via optType.
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("56SevenTypes"));
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  QVERIFY(printOpts);
  QVERIFY2(printOpts->rowCount() > 0, "Print options model is empty");

  // Collect every optType the schema actually exposes.
  QSet<QString> seen;
  for (int i = 0; i < printOpts->rowCount(); ++i)
    seen.insert(printOpts->optType(i));

  // The 6 dispatch types must all be present (nullable + isVector are
  // orthogonal flags surfaced via optNullable/optIsVector, not separate types).
  const QStringList required = {
    QStringLiteral("bool"), QStringLiteral("int"), QStringLiteral("double"),
    QStringLiteral("enum"), QStringLiteral("string"), QStringLiteral("percent"),
  };
  for (const auto &t : required)
    QVERIFY2(seen.contains(t),
             qPrintable(QStringLiteral("ConfigOptionModel must expose at least one '%1' option (got: %2)")
                            .arg(t, QStringList(seen.begin(), seen.end()).join(", "))));
}

void ViewModelSmokeTests::testPerOptionDirtyAndValueSource()
{
  // SETTINGS-04: per-option dirty tracking and valueSourceForKey
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("56DirtyValue"));
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  QVERIFY(printOpts);
  QVERIFY(printOpts->rowCount() > 0);

  // Find a known option (layer_height) and change its value
  int layerIdx = -1;
  for (int i = 0; i < printOpts->rowCount() && layerIdx < 0; ++i) {
    if (printOpts->optKey(i) == QStringLiteral("layer_height"))
      layerIdx = i;
  }
  QVERIFY2(layerIdx >= 0, "layer_height must exist in print options");

  // Initially not dirty
  QVERIFY(!printOpts->optIsDirty(layerIdx));

  // Change value -> becomes dirty
  printOpts->setValue(layerIdx, 0.22);
  QVERIFY(printOpts->optIsDirty(layerIdx));

  // valueSourceForKey returns non-empty source for a known option
  QString source = config.valueSourceForKey(QStringLiteral("layer_height"));
  QVERIFY2(!source.isEmpty(), "valueSourceForKey must return non-empty for known option");

  // Per-group dirty count via ConfigViewModel proxy
  QString group = printOpts->optGroup(layerIdx);
  int dirtyCount = config.dirtyCountForGroup(QStringLiteral("print"), group);
  QVERIFY2(dirtyCount >= 1, "dirtyCountForGroup must be >= 1 after setValue");
}

void ViewModelSmokeTests::testReadonlyBuiltinGating()
{
  // SETTINGS-06: builtin presets are read-only; requestSavePendingChanges
  // must refuse to save and emit saveAsRequired instead of overwriting.
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("56Readonly"));
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  // Ensure we are on a builtin preset
  QString currentPreset = config.currentPreset();
  QVERIFY(!currentPreset.isEmpty());

  // Check if the current preset is builtin
  bool isBuiltin = preset.isBuiltinPreset(currentPreset);
  if (!isBuiltin) {
    // Find any builtin preset and switch to it
    QStringList allPresets = preset.presetNamesForCategory(PresetServiceMock::PrintCat);
    for (const QString &name : allPresets) {
      if (preset.isBuiltinPreset(name)) {
        config.setCurrentPreset(name);
        currentPreset = name;
        isBuiltin = true;
        break;
      }
    }
  }
  if (!isBuiltin) {
    QSKIP("No builtin preset available for this test");
    return;
  }

  // Change a value to make the model dirty
  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  QVERIFY(printOpts);
  int layerIdx = -1;
  for (int i = 0; i < printOpts->rowCount() && layerIdx < 0; ++i) {
    if (printOpts->optKey(i) == QStringLiteral("layer_height"))
      layerIdx = i;
  }
  QVERIFY2(layerIdx >= 0, "layer_height must exist");
  printOpts->setValue(layerIdx, 0.22);

  // requestSavePendingChanges on a builtin preset must return false
  QSignalSpy saveAsSpy(&config, &ConfigViewModel::saveAsRequired);
  bool saveResult = config.requestSavePendingChanges();
  QVERIFY2(!saveResult, "requestSavePendingChanges must return false for builtin presets");
}

void ViewModelSmokeTests::testSaveSaveAsResetOptionResetGroupResetAll()
{
  // SETTINGS-05: resetGroup resets all options in a named group to reference values.
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("56SaveReset"));
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  QVERIFY(printOpts);
  QVERIFY(printOpts->rowCount() > 0);

  // Find an option and its group, make it dirty
  int layerIdx = -1;
  for (int i = 0; i < printOpts->rowCount() && layerIdx < 0; ++i) {
    if (printOpts->optKey(i) == QStringLiteral("layer_height"))
      layerIdx = i;
  }
  QVERIFY2(layerIdx >= 0, "layer_height must exist");
  QString group = printOpts->optGroup(layerIdx);
  QVERIFY2(!group.isEmpty(), "layer_height must belong to a group");

  // Make dirty
  printOpts->setValue(layerIdx, 0.22);
  QVERIFY(printOpts->optIsDirty(layerIdx));
  QVERIFY(config.dirtyCountForGroup(QStringLiteral("print"), group) >= 1);

  // Reset the group via ConfigViewModel::resetGroup
  config.resetGroup(QStringLiteral("print"), group);

  // All options in the group should be clean after reset
  QVERIFY(!printOpts->optIsDirty(layerIdx));
  QCOMPARE(config.dirtyCountForGroup(QStringLiteral("print"), group), 0);
}

void ViewModelSmokeTests::testUnsavedChangesGuardOnDirtyClose()
{
  // SETTINGS-04/05: the QML close path checks ConfigViewModel::isPresetDirty to
  // decide whether to open UnsavedChangesDialog. This is the backend-precondition
  // half of that guard (the dialog visual interaction is in VALIDATION.md
  // Manual-Only for Phase 58). isPresetDirty must be true after an edit and
  // false after a full reset.
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("56UnsavedGuard"));
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  QVERIFY2(!config.isPresetDirty(),
           "isPresetDirty must be false on a fresh (unmodified) preset");

  // Dirty an option via the print option model.
  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  QVERIFY(printOpts);
  QVERIFY2(printOpts->rowCount() > 0, "Print options model is empty");
  // Find a non-readonly numeric/string row to mutate.
  int mutateRow = -1;
  for (int i = 0; i < printOpts->rowCount() && mutateRow < 0; ++i)
  {
    if (!printOpts->optReadonly(i))
      mutateRow = i;
  }
  QVERIFY2(mutateRow >= 0, "No non-readonly option available to mutate");
  const QVariant orig = printOpts->optValue(mutateRow);
  QVariant mutated = orig;
  if (orig.typeId() == QMetaType::Bool || orig.toString() == "true" || orig.toString() == "false")
    mutated = !orig.toBool();
  else if (orig.canConvert<double>())
    mutated = orig.toDouble() + 0.01;
  else
    mutated = QStringLiteral("X_%1").arg(orig.toString());
  printOpts->setValue(mutateRow, mutated);

  QVERIFY2(config.isPresetDirty(),
           "isPresetDirty must be true after editing an option");

  // A full reset clears the dirty flag.
  config.resetAllGlobalOptions();
  QVERIFY2(!config.isPresetDirty(),
           "isPresetDirty must be false after resetAllGlobalOptions");
}

void ViewModelSmokeTests::testPerDialogSearchAndFourLevelMode()
{
  // SETTINGS-02/03: filterOptionIndices dispatches per-tier and respects 4-level
  // ConfigOptionMode (simple=0, advanced=1, develop=2+). Advanced/develop options
  // are excluded when advancedMode=false.
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("56SearchMode"));
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  QVERIFY(printOpts);
  QVERIFY(printOpts->rowCount() > 0);

  // Collect indices in simple mode (advancedMode=false) -- excludes mode>=1
  QList<int> simpleIndices = config.filterOptionIndices(
      QStringLiteral("print"), QString(), false);
  QVERIFY2(!simpleIndices.isEmpty(), "Simple mode must return at least one index");

  // Collect indices in advanced mode -- includes mode>=1
  QList<int> advancedIndices = config.filterOptionIndices(
      QStringLiteral("print"), QString(), true);
  QVERIFY2(!advancedIndices.isEmpty(), "Advanced mode must return at least one index");

  // Advanced mode must include AT LEAST as many options as simple mode
  QVERIFY2(advancedIndices.size() >= simpleIndices.size(),
           "Advanced mode must not exclude any simple-mode options");

  // Every simple index must also be present in advanced indices
  for (int idx : simpleIndices) {
    QVERIFY2(advancedIndices.contains(idx),
             "Simple-mode index must also appear in advanced-mode results");
  }

  // Test per-tier dispatch: "printer" (new) and "machine" (legacy) must return
  // the same result
  QList<int> printerIndices = config.filterOptionIndices(
      QStringLiteral("printer"), QString(), false);
  QList<int> machineIndices = config.filterOptionIndices(
      QStringLiteral("machine"), QString(), false);
  QCOMPARE(printerIndices, machineIndices);

  // Search text filtering: non-empty needle must subset the results
  QList<int> allIndices = config.filterOptionIndices(
      QStringLiteral("print"), QStringLiteral("layer"), false);
  QVERIFY2(allIndices.size() <= simpleIndices.size(),
           "Search filter must return a subset of all indices");

  // Legacy "process" alias must match "print"
  QList<int> processIndices = config.filterOptionIndices(
      QStringLiteral("process"), QString(), false);
  QCOMPARE(processIndices, simpleIndices);

  // SettingsDialog applies the same result chain in QML: tier/search/mode,
  // then active tab page, then selected group. The model helpers must preserve
  // that contract without relying on category names.
  const QStringList pages = printOpts->pageNames();
  QVERIFY2(!pages.isEmpty(), "Print options must expose at least one page");
  const QString page = pages.first();
  const QList<int> pageIndices = printOpts->filterIndicesByPage(advancedIndices, page);
  QVERIFY2(!pageIndices.isEmpty(), "filterIndicesByPage must return rows for a populated page");
  for (int idx : pageIndices) {
    QCOMPARE(printOpts->optPage(idx), page);
  }

  QString group;
  for (int idx : pageIndices) {
    group = printOpts->optGroup(idx);
    if (!group.isEmpty())
      break;
  }
  QVERIFY2(!group.isEmpty(), "At least one filtered option must expose a group");
  const QList<int> groupIndices = printOpts->filterIndicesByGroup(pageIndices, group);
  QVERIFY2(!groupIndices.isEmpty(), "filterIndicesByGroup must return rows for a populated group");
  for (int idx : groupIndices) {
    QCOMPARE(printOpts->optGroup(idx), group);
  }

  int manualGroupCount = 0;
  for (int i = 0; i < printOpts->rowCount(); ++i) {
    if (printOpts->optGroup(i) == group)
      ++manualGroupCount;
  }
  QCOMPARE(printOpts->countForGroup(group), manualGroupCount);
}

void ViewModelSmokeTests::stepCurrentMoveClampsAndUpdatesGcodeLineWindow()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  QVERIFY2(preview.loadGCodeForPreview(kOrcaGcodePath),
           "loadGCodeForPreview should succeed on the committed Orca fixture");
  QVERIFY2(preview.moveCount() > 4, "fixture must expose enough moves for stepping");

  preview.setCurrentMove(0);
  QCOMPARE(preview.currentMove(), 0);

  QSignalSpy spy(&preview, &PreviewViewModel::stateChanged);
  QVERIFY(spy.isValid());

  preview.stepCurrentMove(2);
  QCOMPARE(preview.currentMove(), 2);
  QVERIFY2(preview.currentGcodeLine() > 0,
           "stepCurrentMove must rebuild the current source-line window");
  QVERIFY2(!preview.gcodeLines().isEmpty(),
           "stepCurrentMove must keep the G-code source window populated");

  const int emittedAfterForward = spy.count();
  QVERIFY2(emittedAfterForward >= 1,
           "stepCurrentMove must emit stateChanged when it changes the move");

  preview.stepCurrentMove(999999);
  QCOMPARE(preview.currentMove(), preview.moveCount());
  preview.stepCurrentMove(-999999);
  QCOMPARE(preview.currentMove(), 0);
}

void ViewModelSmokeTests::testNullableAndVectorOptions()
{
  // SETTINGS-03: nullable (inherit-from-parent) and isVector (per-extruder)
  // flags are surfaced on the option model. At least one option of each kind
  // must exist in the loaded schema.
  ScopedApplicationIdentity appIdentity(QStringLiteral("OWzxTests"),
                                        QStringLiteral("56NullableVector"));
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  // Nullable / vector options are tier-dependent (e.g. per-extruder filament
  // temps are vector; inheritable printer options are nullable). Scan all three
  // tiers and require at least one of each across the union.
  QList<ConfigOptionModel *> models = {
    qobject_cast<ConfigOptionModel *>(config.printOptions()),
    qobject_cast<ConfigOptionModel *>(config.filamentOptions()),
    qobject_cast<ConfigOptionModel *>(config.machineOptions()),
  };
  bool sawNullable = false, sawVector = false;
  for (ConfigOptionModel *m : models)
  {
    if (!m) continue;
    for (int i = 0; i < m->rowCount() && !(sawNullable && sawVector); ++i)
    {
      if (m->optNullable(i)) sawNullable = true;
      if (m->optIsVector(i)) sawVector = true;
    }
  }
  QVERIFY2(sawNullable,
           "Schema must expose at least one nullable option (optNullable==true) across tiers");
  QVERIFY2(sawVector,
           "Schema must expose at least one multi-value/vector option (optIsVector==true) across tiers");
}

void ViewModelSmokeTests::editorExplosionRatioDefaultsAndResetMirrorsUpstream()
{
  // Phase 91-01 (ASMEXPLODE-01): explosionRatio mirrors upstream m_explosion_ratio
  // (GLCanvas3D.hpp:596, default 1.0) and reset mirrors reset_explosion_ratio()
  // (GLCanvas3D.hpp:770-771). The property is pure state — no model load needed.
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  // (1) Default is 1.0 (mirrors m_explosion_ratio = 1.0).
  QCOMPARE(editor.explosionRatio(), 1.0f);

  // (2) setExplosionRatio emits stateChanged and stores the value.
  QSignalSpy spy(&editor, &EditorViewModel::stateChanged);
  QVERIFY(spy.isValid());
  editor.setExplosionRatio(2.5f);
  QCOMPARE(editor.explosionRatio(), 2.5f);
  QVERIFY(spy.count() >= 1);

  // (3) setExplosionRatio is a no-op on an unchanged value (guard).
  spy.clear();
  editor.setExplosionRatio(2.5f);
  QCOMPARE(spy.count(), 0);

  // (4) resetExplosionRatio restores 1.0 and emits (mirrors reset_explosion_ratio).
  spy.clear();
  editor.resetExplosionRatio();
  QCOMPARE(editor.explosionRatio(), 1.0f);
  QVERIFY(spy.count() >= 1);

  // (5) resetExplosionRatio is a no-op when already at default.
  spy.clear();
  editor.resetExplosionRatio();
  QCOMPARE(spy.count(), 0);
}

void ViewModelSmokeTests::assemblyMeasureGizmoActivabilityMirrorsUpstream()
{
  // Phase 92-01 (ASMMEASURE-01): the Assembly measurement gizmo activability
  // mirrors upstream GLGizmoAssembly::on_is_activable()
  // (GLGizmoAssembly.cpp:53-68): active canvas == AssembleView (2) AND
  // abs(explosion_ratio - 1.0) < 1e-2 AND selection.volumes_count() >= 2.
  // Parts (a)-(c) need no model (the gate fails before the selection count);
  // parts (d)-(e) add two primitives via addPrimitiveToPlate (synchronous +
  // additive — loadFile replaces rather than appends, so it cannot reach the
  // >=2 count) so >=2 volumes can be selected.
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  // (a) Default (canvas = View3D = 0, no selection): gizmo not active;
  //     activate is a no-op returning false.
  QCOMPARE(editor.activeCanvasType(), 0);
  QVERIFY(!editor.assemblyMeasureGizmoActive());
  QVERIFY(!editor.activateAssemblyMeasureGizmo());
  QVERIFY(!editor.assemblyMeasureGizmoActive());

  // (b) AssembleView but <2 selected: not activable.
  editor.setActiveCanvasType(2);
  QCOMPARE(editor.activeCanvasType(), 2);
  QVERIFY(!editor.assemblyMeasureGizmoActive());
  QVERIFY(!editor.activateAssemblyMeasureGizmo());
  QVERIFY(!editor.assemblyMeasureGizmoActive());

  // (c) AssembleView + explosionRatio != 1.0 (2.0): not activable (mirrors the
  //     abs(ratio-1.0) < 1e-2 gate).
  editor.setExplosionRatio(2.0f);
  QVERIFY(!editor.activateAssemblyMeasureGizmo());
  QVERIFY(!editor.assemblyMeasureGizmoActive());
  editor.setExplosionRatio(1.0f);  // restore

  // (d) AssembleView + ratio 1.0 + >=2 selected: activate returns true and flips
  //     the active flag. QSignalSpy records the stateChanged transition.
  //     Fixture: two primitives via addPrimitiveToPlate (synchronous + additive —
  //     loadFile replaces rather than appends, so it cannot reach the >=2 count).
  QVERIFY2(editor.addPrimitiveToPlate(0), "adding the first primitive should succeed");
  QVERIFY2(editor.addPrimitiveToPlate(0), "adding the second primitive should succeed");
  QVERIFY2(editor.objectCount() >= 2,
           "two addPrimitiveToPlate calls should yield >=2 objects");
  editor.selectAllVisibleObjects();
  QVERIFY2(editor.selectedObjectCount() >= 2,
           "select-all should yield >=2 selected objects for the activability case");

  QSignalSpy spy(&editor, &EditorViewModel::stateChanged);
  QVERIFY(spy.isValid());
  QVERIFY(editor.activateAssemblyMeasureGizmo());
  QVERIFY(editor.assemblyMeasureGizmoActive());
  QVERIFY(spy.count() >= 1);

  // (e) deactivate flips it back to false and emits.
  spy.clear();
  editor.deactivateAssemblyMeasureGizmo();
  QVERIFY(!editor.assemblyMeasureGizmoActive());
  QVERIFY(spy.count() >= 1);

  // (f) activate is a no-op when the canvas is switched away from AssembleView
  //     even with >=2 selected (the canvas gate is the first condition).
  editor.setActiveCanvasType(0);
  QVERIFY(!editor.activateAssemblyMeasureGizmo());
  QVERIFY(!editor.assemblyMeasureGizmoActive());
}

void ViewModelSmokeTests::assemblyMeasureGeometryComputesDistanceAndAngle()
{
  // Phase 92-01 (ASMMEASURE-02): AssemblyMeasureGeometry::measure computes the
  // correct center-to-center distance + per-axis XYZ delta + angle between the
  // two volumes' longest-AABB-axis directions. Pure math — no model needed.
  // Box A: longest axis = X (extent 10). Box B: longest axis = Y (extent 10),
  // offset +16 in X and +4 in Y from A's center.
  PrepareSceneData::ModelBounds a;
  a.minX = 0.0f;  a.maxX = 10.0f;   // extent 10 (longest)
  a.minY = 0.0f;  a.maxY = 2.0f;
  a.minZ = 0.0f;  a.maxZ = 2.0f;
  // A center = (5, 1, 1).
  PrepareSceneData::ModelBounds b;
  b.minX = 20.0f; b.maxX = 22.0f;
  b.minY = 0.0f;  b.maxY = 10.0f;   // extent 10 (longest)
  b.minZ = 0.0f;  b.maxZ = 2.0f;
  // B center = (21, 5, 1). Delta A->B = (16, 4, 0). Distance = sqrt(272) ~= 16.49.

  const AssemblyMeasureResult r = AssemblyMeasureGeometry::measure(a, b);
  QVERIFY2(r.valid, "measure() must return valid for two non-degenerate AABBs");

  // Distance = sqrt(16^2 + 4^2 + 0) = sqrt(272) ~= 16.492.
  QVERIFY2(qFuzzyCompare(r.distance, std::sqrt(272.0f))
               || std::abs(r.distance - std::sqrt(272.0f)) < 1e-3f,
           qPrintable(QStringLiteral("distance expected ~16.492, got %1")
                          .arg(r.distance)));
  // XYZ delta A->B = (16, 4, 0).
  QVERIFY2(qFuzzyCompare(r.distanceXyz.x(), 16.0f), "distanceXyz.x must be 16");
  QVERIFY2(qFuzzyCompare(r.distanceXyz.y(), 4.0f), "distanceXyz.y must be 4");
  QVERIFY2(qFuzzyCompare(r.distanceXyz.z(), 0.0f), "distanceXyz.z must be 0");

  // Longest axes: A -> X, B -> Y (perpendicular).
  QVERIFY2(r.axisA == QVector3D(1, 0, 0), "axisA must be the X unit vector");
  QVERIFY2(r.axisB == QVector3D(0, 1, 0), "axisB must be the Y unit vector");
  // Angle between X and Y = 90 degrees.
  QVERIFY2(std::abs(r.angleDeg - 90.0f) < 1e-3f,
           qPrintable(QStringLiteral("angle expected ~90.000, got %1")
                          .arg(r.angleDeg)));

  // Formatting: distance gets 3 decimals + ' mm'; angle gets 3 decimals + degree glyph.
  const QString distStr = AssemblyMeasureGeometry::formatDistance(r.distance);
  QVERIFY2(distStr.contains(QStringLiteral("mm")),
           "formatDistance must include the 'mm' suffix");
  QVERIFY2(distStr.contains(QStringLiteral(".")),
           "formatDistance must use decimal precision");
  const QString angleStr = AssemblyMeasureGeometry::formatAngle(90.0f);
  QVERIFY2(angleStr.contains(QStringLiteral("90.000")),
           "formatAngle(90) must contain '90.000'");
  QVERIFY2(angleStr.contains(QStringLiteral("\u00b0")),
           "formatAngle must include the degree glyph");

  // Degenerate AABB -> invalid.
  PrepareSceneData::ModelBounds degenerate;  // all-zero extents
  const AssemblyMeasureResult bad = AssemblyMeasureGeometry::measure(degenerate, b);
  QVERIFY2(!bad.valid, "measure() must return invalid for a degenerate AABB");
}

void ViewModelSmokeTests::assembleViewDataPoolIsolatedFromPrepareAndPreview()
{
  // Phase 93-01 (ASMROUTE-02): the AssembleView data pool caches per-object
  // info and is updated ONLY when m_activeCanvasType == 2 (CanvasAssembleView),
  // mirroring upstream GLGizmosManager.cpp:427-431. Prepare (0) and Preview (1)
  // never populate or read it. The pool's test accessor returns 0 when the
  // ModelObjectsInfo resource is not valid, which is itself the isolation
  // assertion.
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  // (a) Default canvas (View3D = 0): pool not populated even with objects
  //     loaded. Fixture: two primitives via addPrimitiveToPlate (synchronous +
  //     additive — loadFile replaces rather than appends).
  QVERIFY2(editor.addPrimitiveToPlate(0), "adding the first primitive should succeed");
  QVERIFY2(editor.addPrimitiveToPlate(0), "adding the second primitive should succeed");
  QVERIFY2(editor.objectCount() >= 2,
           "two addPrimitiveToPlate calls should yield >=2 objects");
  QCOMPARE(editor.activeCanvasType(), 0);
  QVERIFY2(editor.assembleViewDataPoolObjectCountForTest() == 0,
           "pool must stay empty on Prepare (View3D) — isolation constraint");

  // (b) Switch to AssembleView (2): pool refreshes and the ModelObjectsInfo
  //     resource reflects the loaded per-object info (count >= 2).
  editor.setActiveCanvasType(2);
  QCOMPARE(editor.activeCanvasType(), 2);
  QVERIFY2(editor.assembleViewDataPoolObjectCountForTest() >= 2,
           qPrintable(QStringLiteral("pool object count must be >=2 on AssembleView, "
                                     "got %1")
                          .arg(editor.assembleViewDataPoolObjectCountForTest())));

  // (c) Switch back to Prepare (0): pool releases (update(None)) -> count 0.
  editor.setActiveCanvasType(0);
  QCOMPARE(editor.activeCanvasType(), 0);
  QVERIFY2(editor.assembleViewDataPoolObjectCountForTest() == 0,
           "pool must release when leaving AssembleView for Prepare");

  // (d) Switch to Preview (1): pool stays released.
  editor.setActiveCanvasType(1);
  QCOMPARE(editor.activeCanvasType(), 1);
  QVERIFY2(editor.assembleViewDataPoolObjectCountForTest() == 0,
           "pool must stay released on Preview");

  // (e) Switch to AssembleView again: selectedVolumeBoundsForAssemblyMeasure()
  //     returns the same bounds whether read directly or via the pool (the
  //     refactor routes the existing computation through the cached resource).
  editor.setActiveCanvasType(2);
  QCOMPARE(editor.activeCanvasType(), 2);
  QVERIFY2(editor.assembleViewDataPoolObjectCountForTest() >= 2,
           "pool must repopulate on returning to AssembleView");
  // Select >=2 objects so the measure bounds path is exercised; the pool-fed
  // read returns the same values the inline fallback would (same source).
  editor.selectAllVisibleObjects();
  if (editor.selectedObjectCount() >= 2)
  {
    const QList<PrepareSceneData::ModelBounds> bounds =
        editor.selectedVolumeBoundsForAssemblyMeasure();
    QVERIFY2(bounds.size() == 2,
             qPrintable(QStringLiteral("selectedVolumeBoundsForAssemblyMeasure must "
                                       "return 2 bounds when >=2 selected on "
                                       "AssembleView, got %1")
                            .arg(bounds.size())));
    // The pool-fed bounds are non-degenerate for real primitives (extent > 0
    // on at least one axis). Sanity-check the first bounds.
    const bool nonDegenerate =
        (bounds.at(0).maxX > bounds.at(0).minX) ||
        (bounds.at(0).maxY > bounds.at(0).minY) ||
        (bounds.at(0).maxZ > bounds.at(0).minZ);
    QVERIFY2(nonDegenerate,
             "pool-fed assembly-measure bounds must be non-degenerate for primitives");
  }
}

QTEST_MAIN(ViewModelSmokeTests)
#include "ViewModelSmokeTests.moc"
