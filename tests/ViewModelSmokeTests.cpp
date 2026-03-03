#include <QSignalSpy>
#include <QtTest>

#include "core/services/DeviceServiceMock.h"
#include "core/services/NetworkServiceMock.h"
#include "core/services/PresetServiceMock.h"
#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceServiceMock.h"
#include "core/viewmodels/ConfigViewModel.h"
#include "core/viewmodels/EditorViewModel.h"
#include "core/viewmodels/MonitorViewModel.h"
#include "core/viewmodels/PreviewViewModel.h"

class ViewModelSmokeTests final : public QObject
{
  Q_OBJECT

private slots:
  void editor_import_model_updates_state();
  void preview_receives_slice_progress();
  void monitor_refresh_updates_network_and_device();
  void config_default_and_switch_preset();
};

void ViewModelSmokeTests::editor_import_model_updates_state()
{
  ProjectServiceMock project;
  SliceServiceMock slice;
  EditorViewModel editor(&project, &slice);

  QCOMPARE(editor.modelCount(), 0);

  QSignalSpy spy(&editor, &EditorViewModel::stateChanged);
  editor.importMockModel();

  QVERIFY(spy.count() >= 1);
  QCOMPARE(editor.modelCount(), 1);
}

void ViewModelSmokeTests::preview_receives_slice_progress()
{
  SliceServiceMock slice;
  PreviewViewModel preview(&slice);

  QSignalSpy spy(&preview, &PreviewViewModel::stateChanged);
  slice.startSlice(QStringLiteral("demo"));

  QTRY_VERIFY_WITH_TIMEOUT(preview.progress() > 0, 1500);
  QVERIFY(spy.count() > 0);

  QTRY_VERIFY_WITH_TIMEOUT(!preview.slicing(), 6000);
  QCOMPARE(preview.progress(), 100);
  QCOMPARE(preview.estimatedTime(), QStringLiteral("01:42:16"));
}

void ViewModelSmokeTests::monitor_refresh_updates_network_and_device()
{
  DeviceServiceMock device;
  NetworkServiceMock network;
  MonitorViewModel monitor(&device, &network);

  const QString beforeState = monitor.firstDeviceState();
  const int beforeLatency = monitor.latencyMs();

  QSignalSpy spy(&monitor, &MonitorViewModel::stateChanged);
  monitor.refresh();

  QVERIFY(spy.count() >= 1);
  QVERIFY(monitor.firstDeviceState() != beforeState || monitor.latencyMs() != beforeLatency);
  QVERIFY(monitor.online());
}

void ViewModelSmokeTests::config_default_and_switch_preset()
{
  PresetServiceMock preset;
  ConfigViewModel config(&preset);

  QCOMPARE(config.currentPreset(), QStringLiteral("0.20mm Standard @Creality K1C"));
  QCOMPARE(config.layerHeight(), 0.2);

  QSignalSpy spy(&config, &ConfigViewModel::stateChanged);
  config.setCurrentPreset(QStringLiteral("0.16mm Fine"));

  QVERIFY(spy.count() >= 1);
  QCOMPARE(config.currentPreset(), QStringLiteral("0.16mm Fine"));
}

QTEST_MAIN(ViewModelSmokeTests)
#include "ViewModelSmokeTests.moc"
