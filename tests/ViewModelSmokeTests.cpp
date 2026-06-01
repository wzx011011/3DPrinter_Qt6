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

QTEST_MAIN(ViewModelSmokeTests)
#include "ViewModelSmokeTests.moc"
