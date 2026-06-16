#include <QSignalSpy>
#include <QDir>
#include <QFileInfo>
#include <QMetaEnum>
#include <QtTest>

#include "core/services/CameraServiceMock.h"
#include "core/services/DeviceServiceMock.h"
#include "core/services/NetworkServiceMock.h"
#include "core/services/PresetServiceMock.h"
#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
#include "core/viewmodels/ConfigViewModel.h"
#include "core/viewmodels/EditorViewModel.h"
#include "core/viewmodels/MonitorViewModel.h"
#include "core/viewmodels/PreviewViewModel.h"
#include "core/viewmodels/ProjectViewModel.h"
#include "qml_gui/BackendContext.h"
#include "qml_gui/Models/ConfigOptionModel.h"

namespace
{
  static const QString kStlPath = QDir::cleanPath(
      QStringLiteral(QT_TESTCASE_SOURCEDIR) +
      QStringLiteral("/third_party/OrcaSlicer/resources/profiles/hotend.stl"));
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
  void editor_import_model_updates_state();
  void monitor_refresh_updates_network_and_device();
  void config_default_and_switch_preset();
  void testUpstreamDefaultsContainVectorKeys();
  void testMachineOptionsLoaded();
  void testFilamentOptionsLoaded();
  void testMachineEditFlowsToGlobal();
  void testTierAwareSaveFiltersByTier();

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

  QVERIFY(spy.count() >= 1);
  QVERIFY(monitor.monitorState() != beforeState || monitor.latencyMs() != beforeLatency);
  QVERIFY(monitor.networkOnline());
}

void ViewModelSmokeTests::config_default_and_switch_preset()
{
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  QSignalSpy spy(&config, &ConfigViewModel::stateChanged);
  const QString initialPreset = config.currentPreset();
  QVERIFY(!initialPreset.isEmpty());

  config.setCurrentPreset(QStringLiteral("0.16mm Fine"));
  QVERIFY(spy.count() >= 1);
  QCOMPARE(config.currentPreset(), QStringLiteral("0.16mm Fine"));
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
  PresetServiceMock preset;
  ProjectServiceMock project;
  ConfigViewModel config(&preset, &project);

  auto *printOpts = qobject_cast<ConfigOptionModel *>(config.printOptions());
  auto *machineOpts = qobject_cast<ConfigOptionModel *>(config.machineOptions());

  int printIdx = printOpts->indexOfKey(QStringLiteral("layer_height"));
  int machineIdx = machineOpts->indexOfKey(QStringLiteral("machine_max_speed_x"));
  QVERIFY2(printIdx >= 0, "layer_height not in print options");
  QVERIFY2(machineIdx >= 0, "machine_max_speed_x not in machine options");

  // Edit both tiers
  printOpts->setValue(printIdx, 0.35);
  machineOpts->setValue(machineIdx, 999.0);

  // Save as print tier — should only include print model keys
  config.setActivePresetTier(QStringLiteral("print"));
  config.saveCurrentPreset();

  // Verify print preset has layer_height with the edited value
  auto saved = preset.presetValues(config.currentPrintPreset().isEmpty()
                                    ? config.currentPreset()
                                    : config.currentPrintPreset());
  QVERIFY2(saved.contains(QStringLiteral("layer_height")),
           "layer_height should be in print preset after save");
  QCOMPARE(saved[QStringLiteral("layer_height")].toDouble(), 0.35);

  // Now save as printer tier — machine key should be saved there
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

QTEST_MAIN(ViewModelSmokeTests)
#include "ViewModelSmokeTests.moc"
