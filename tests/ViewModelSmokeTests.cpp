#include <QSignalSpy>
#include <QDir>
#include <QFileInfo>
#include <QtTest>

#include "qml_gui/BackendContext.h"
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

class ViewModelSmokeTests final : public QObject
{
  Q_OBJECT

private slots:
  void editor_import_model_updates_state();
  void preview_receives_slice_progress();
  void slice_reuse_previous_gcode_file();
  void monitor_refresh_updates_network_and_device();
  void config_default_and_switch_preset();
  void topbar_new_save_and_navigation_behaviors();
  void topbar_open_import_with_temp_model_file();
};

void ViewModelSmokeTests::editor_import_model_updates_state()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  QCOMPARE(editor.modelCount(), 0);

  QSignalSpy spy(&editor, &EditorViewModel::stateChanged);
  editor.importMockModel();

  QVERIFY(spy.count() >= 1);
  QCOMPARE(editor.modelCount(), 1);
}

void ViewModelSmokeTests::preview_receives_slice_progress()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  QSignalSpy spy(&preview, &PreviewViewModel::stateChanged);
  slice.startSlice(QStringLiteral("demo"));

  QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, 1500);
  QCOMPARE(preview.slicing(), false);
  QCOMPARE(preview.progress(), 0);
  QCOMPARE(preview.estimatedTime(), QStringLiteral("--:--:--"));
}

void ViewModelSmokeTests::slice_reuse_previous_gcode_file()
{
  ProjectServiceMock project;
  SliceService slice(&project);

  const QString gcodePath = QDir::cleanPath(QStringLiteral(QT_TESTCASE_SOURCEDIR) +
                                            QStringLiteral("/third_party/CrealityPrint/resources/printers/ams_load.gcode"));
  QVERIFY2(QFileInfo::exists(gcodePath), qPrintable(gcodePath));

  QSignalSpy progressSpy(&slice, &SliceService::progressUpdated);
  QVERIFY(slice.loadGCodeFromPrevious(gcodePath));

  QTRY_VERIFY_WITH_TIMEOUT(!slice.slicing(), 5000);
  QVERIFY(progressSpy.count() > 0);
  QCOMPARE(slice.progress(), 100);
  QCOMPARE(slice.outputPath(), QFileInfo(gcodePath).absoluteFilePath());
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

void ViewModelSmokeTests::topbar_new_save_and_navigation_behaviors()
{
  BackendContext backend;

  auto *editor = qobject_cast<EditorViewModel *>(backend.editorViewModel());
  auto *project = qobject_cast<ProjectViewModel *>(backend.projectViewModel());
  QVERIFY(editor != nullptr);
  QVERIFY(project != nullptr);

  editor->importMockModel();
  project->saveProjectAs(QStringLiteral("C:/tmp/preexisting.3mf"));
  QVERIFY(editor->modelCount() > 0);
  QVERIFY(!project->currentProjectPath().isEmpty());

  backend.topbarNewProject();
  QCOMPARE(backend.currentPage(), 1);
  QCOMPARE(editor->modelCount(), 0);
  QCOMPARE(project->currentProjectPath(), QString{});
  QCOMPARE(editor->statusText(), QStringLiteral("已新建项目"));

  QVERIFY(!backend.topbarSaveProject());
  QVERIFY(backend.topbarSaveProjectAs(QStringLiteral("C:/tmp/new_saved_project.3mf")));
  QVERIFY(backend.topbarSaveProject());
  QCOMPARE(project->currentProjectPath(), QStringLiteral("C:/tmp/new_saved_project.3mf"));

  backend.setCurrentPage(1);
  backend.openSettings();
  QCOMPARE(backend.currentPage(), 11);
  backend.setCurrentPage(8);
  QCOMPARE(backend.currentPage(), 8);
  backend.setCurrentPage(0);
  QCOMPARE(backend.currentPage(), 0);
}

void ViewModelSmokeTests::topbar_open_import_with_temp_model_file()
{
  BackendContext backend;
  auto *editor = qobject_cast<EditorViewModel *>(backend.editorViewModel());
  auto *project = qobject_cast<ProjectViewModel *>(backend.projectViewModel());
  QVERIFY(editor != nullptr);
  QVERIFY(project != nullptr);

  const QString stlPath = QDir::cleanPath(QStringLiteral(QT_TESTCASE_SOURCEDIR) +
                                          QStringLiteral("/third_party/CrealityPrint/resources/profiles/hotend.stl"));
  QVERIFY2(QFileInfo::exists(stlPath), qPrintable(stlPath));

  QVERIFY(backend.topbarImportModel(stlPath));
  QCOMPARE(backend.currentPage(), 1);
  QTRY_VERIFY_WITH_TIMEOUT(editor->modelCount() > 0, 5000);

  QVERIFY(backend.topbarOpenProject(stlPath));
  QCOMPARE(backend.currentPage(), 1);
  QCOMPARE(project->currentProjectPath(), stlPath);
  QVERIFY(project->recentProjects().contains(stlPath));
}

QTEST_MAIN(ViewModelSmokeTests)
#include "ViewModelSmokeTests.moc"
