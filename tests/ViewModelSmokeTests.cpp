#include <QSignalSpy>
#include <QDir>
#include <QFileInfo>
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
      QStringLiteral("/third_party/CrealityPrint/resources/profiles/hotend.stl"));
}

class ViewModelSmokeTests final : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
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

  // Save as print tier — should only include print keys
  config.setActivePresetTier(QStringLiteral("print"));
  config.saveCurrentPreset();

  // Verify print preset has layer_height
  auto saved = preset.presetValues(config.currentPrintPreset().isEmpty()
                                    ? config.currentPreset()
                                    : config.currentPrintPreset());
  QVERIFY2(saved.contains(QStringLiteral("layer_height")),
           "layer_height should be in print preset after save");
  QCOMPARE(saved[QStringLiteral("layer_height")].toDouble(), 0.35);

  // Machine key should NOT be in the print preset (tier isolation)
  // (It might exist from initial load, but value should NOT be 999.0)
  if (saved.contains(QStringLiteral("machine_max_speed_x"))) {
    QVERIFY2(saved[QStringLiteral("machine_max_speed_x")].toDouble() != 999.0,
             "machine_max_speed_x should not have tier-crossed value in print preset");
  }
}

QTEST_MAIN(ViewModelSmokeTests)
#include "ViewModelSmokeTests.moc"
