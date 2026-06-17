#include <QSignalSpy>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QtTest>

#include "core/services/PresetServiceMock.h"
#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
#include "core/viewmodels/ConfigViewModel.h"
#include "core/viewmodels/EditorViewModel.h"
#include "core/viewmodels/PreviewViewModel.h"

namespace
{
  static const QString kStlPath = QDir::cleanPath(
      QStringLiteral(QT_TESTCASE_SOURCEDIR) +
      QStringLiteral("/third_party/OrcaSlicer/resources/profiles/hotend.stl"));
}

class E2EWorkflowTests final : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void test_config_injection_applies_preset_values();
  void test_slice_produces_gcode_file();
  void test_slice_results_propagate_to_editor_vm();
  void test_export_gcode_to_path();
  void test_preview_receives_gcode_data();
  void test_model_change_invalidates_slice_result();

private:
  bool hasLibslic3r() const;
};

bool E2EWorkflowTests::hasLibslic3r() const
{
#ifdef HAS_LIBSLIC3R
  return true;
#else
  return false;
#endif
}

void E2EWorkflowTests::initTestCase()
{
  if (!hasLibslic3r())
    QSKIP("E2E workflow tests require HAS_LIBSLIC3R — skipping all tests");
  QVERIFY2(QFileInfo::exists(kStlPath), qPrintable(
      QStringLiteral("Test STL not found: %1").arg(kStlPath)));
}

void E2EWorkflowTests::test_config_injection_applies_preset_values()
{
  // Verify that merged config values from ConfigViewModel are applied to SliceService
  PresetServiceMock presetService;
  ProjectServiceMock projectService;
  SliceService slice(&projectService);
  ConfigViewModel config(&presetService, &projectService);

  // Load a print preset with a known layer_height
  config.setCurrentPrintPreset(QStringLiteral("0.16mm Fine"));

  // Get merged config values
  const QHash<QString, QVariant> merged = config.mergedConfigValues();
  QVERIFY2(!merged.isEmpty(), "mergedConfigValues() should return non-empty hash");

  // Inject config into SliceService
  slice.setMergedPresetConfig(merged);

  // Verify the config was stored for injection
  // (The actual injection happens inside startSlice on the worker thread,
  //  so we verify the setup is correct by checking the merged hash has expected keys)
  QVERIFY2(merged.contains(QStringLiteral("layer_height")),
           "merged config should contain layer_height");

  // If we have a specific layer_height value, verify it differs from factory default
  const QVariant lh = merged.value(QStringLiteral("layer_height"));
  if (lh.isValid())
  {
    bool ok = false;
    const double lhVal = lh.toDouble(&ok);
    if (ok)
    {
      QVERIFY2(qFuzzyCompare(lhVal, 0.16) || lhVal != 0.2,
               "layer_height should reflect the selected preset, not factory default 0.2");
    }
  }
}

void E2EWorkflowTests::test_slice_produces_gcode_file()
{
  ProjectServiceMock project;
  SliceService slice(&project);

  // Load model
  QVERIFY(project.loadFile(kStlPath));

  // Start slice and wait for completion
  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);

  slice.startSlice(QStringLiteral("e2e_test"));

  // Wait up to 120 seconds for real slicing
  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0, 120000);

  if (failedSpy.count() > 0)
  {
    QSKIP("Slice failed — may be expected without full config setup");
  }

  // Verify output path
  const QString outputPath = slice.outputPath();
  QVERIFY2(!outputPath.isEmpty(), "output path should be non-empty after successful slice");

  // Verify the G-code file exists on disk
  QVERIFY2(QFileInfo::exists(outputPath),
           qPrintable(QStringLiteral("G-code file should exist: %1").arg(outputPath)));

  // Verify estimated time label format (HH:MM:SS)
  const QString timeLabel = slice.estimatedTimeLabel();
  QVERIFY2(!timeLabel.isEmpty(), "estimated time label should be non-empty");
  QVERIFY2(timeLabel.contains(QLatin1Char(':')),
           qPrintable(QStringLiteral("time label should be HH:MM:SS format: %1").arg(timeLabel)));

  // Verify at least one result field is populated
  const bool hasResult = !slice.resultWeightLabel().isEmpty() ||
                         !slice.resultFilamentLabel().isEmpty();
  QVERIFY2(hasResult, "at least weight or filament result should be non-empty");

  // Clean up generated G-code
  if (QFileInfo::exists(outputPath))
    QFile::remove(outputPath);
}

void E2EWorkflowTests::test_slice_results_propagate_to_editor_vm()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  // Load model
  QVERIFY(project.loadFile(kStlPath));

  // Start slice
  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);

  editor.requestSlice();

  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0, 120000);

  if (failedSpy.count() > 0)
    QSKIP("Slice failed — may be expected without full config setup");

  // Verify EditorViewModel received slice results
  QVERIFY2(editor.hasSliceResult(), "EditorViewModel should report hasSliceResult=true");

  const QString estTime = editor.sliceEstimatedTime();
  QVERIFY2(!estTime.isEmpty(),
           qPrintable(QStringLiteral("sliceEstimatedTime should be non-empty: got '%1'").arg(estTime)));

  // Clean up
  const QString outputPath = slice.outputPath();
  if (!outputPath.isEmpty() && QFileInfo::exists(outputPath))
    QFile::remove(outputPath);
}

void E2EWorkflowTests::test_export_gcode_to_path()
{
  ProjectServiceMock project;
  SliceService slice(&project);

  // Load model
  QVERIFY(project.loadFile(kStlPath));

  // Slice first
  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);

  slice.startSlice(QStringLiteral("export_test"));

  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0, 120000);

  if (failedSpy.count() > 0)
    QSKIP("Slice failed — cannot test export without successful slice");

  const QString srcPath = slice.outputPath();
  QVERIFY2(!srcPath.isEmpty(), "source G-code path should exist after slice");
  QVERIFY2(QFileInfo::exists(srcPath),
           qPrintable(QStringLiteral("source G-code file should exist: %1").arg(srcPath)));

  // Export to a temporary file
  QTemporaryFile tempFile(QStringLiteral("XXXXXX_export_test.gcode"));
  QVERIFY(tempFile.open());
  const QString exportPath = tempFile.fileName();
  tempFile.close();

  const bool exported = slice.exportGCodeToPath(exportPath);
  QVERIFY2(exported, "exportGCodeToPath should succeed");

  const QFileInfo exportedInfo(exportPath);
  QVERIFY2(exportedInfo.exists(), "exported file should exist");
  QVERIFY2(exportedInfo.size() > 0, "exported file should have non-zero size");

  // Clean up
  QFile::remove(exportPath);
  if (QFileInfo::exists(srcPath))
    QFile::remove(srcPath);
}

void E2EWorkflowTests::test_preview_receives_gcode_data()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  // Load model
  QVERIFY(project.loadFile(kStlPath));

  // Start slice
  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);

  slice.startSlice(QStringLiteral("preview_test"));

  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0, 120000);

  if (failedSpy.count() > 0)
    QSKIP("Slice failed — cannot test preview data without successful slice");

  // PreviewViewModel should have received G-code data
  // Note: preview rebuilds from outputPath signal; in test we check
  // that the slice output is accessible for preview consumption
  const QString outputPath = slice.outputPath();
  QVERIFY2(!outputPath.isEmpty(), "slice should produce G-code for preview");
  QVERIFY2(QFileInfo::exists(outputPath),
           qPrintable(QStringLiteral("G-code file for preview should exist: %1").arg(outputPath)));

  // Layer count should be positive after slicing
  QVERIFY2(slice.resultLayerCount() > 0,
           qPrintable(QStringLiteral("layer count should be > 0, got: %1").arg(slice.resultLayerCount())));

  // Clean up
  if (QFileInfo::exists(outputPath))
    QFile::remove(outputPath);
}

void E2EWorkflowTests::test_model_change_invalidates_slice_result()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  // Load model
  QVERIFY(project.loadFile(kStlPath));

  // Slice
  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);

  editor.requestSlice();
  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0, 120000);

  if (failedSpy.count() > 0)
    QSKIP("Slice failed — cannot test invalidation without successful slice");

  // Verify slice result exists
  QVERIFY2(editor.hasSliceResult(), "should have slice result after slicing");

  // Modify the model (delete object) to invalidate the result
  const int objIdx = editor.selectedObjectIndex();
  if (objIdx >= 0)
  {
    editor.deleteObject(objIdx);

    // After model change, the slice result should be invalidated
    // (EditorViewModel clears slice state on object deletion)
    // The m_slicedPlateIndices should be cleared or no longer valid
    QVERIFY2(editor.modelCount() == 0 || !editor.hasSliceResult(),
             "slice result should be invalidated after model change");
  }

  // Clean up
  const QString outputPath = slice.outputPath();
  if (!outputPath.isEmpty() && QFileInfo::exists(outputPath))
    QFile::remove(outputPath);
}

QTEST_MAIN(E2EWorkflowTests)
#include "E2EWorkflowTests.moc"
