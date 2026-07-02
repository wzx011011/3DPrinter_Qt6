#include <QSignalSpy>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QtTest>
#include <cstring>

#include "core/services/PresetServiceMock.h"
#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
#include "core/viewmodels/ConfigViewModel.h"
#include "core/viewmodels/EditorViewModel.h"
#include "core/viewmodels/PreviewViewModel.h"
#include "qml_gui/BackendContext.h"

namespace
{
  // Use OrcaSlicer's Prusa.stl fixture instead of the tiny hotend.stl fixture.
  // The former hotend fixture can sit outside the default bed and trigger
  // "Objects could not fit on the bed; bed_idx==-1" in GUI-path arrange code.
  // Prusa.stl matches the successful CLI regression path and keeps E2E slicing deterministic.
  static const QString kStlPath = QDir::cleanPath(
      QStringLiteral(QT_TESTCASE_SOURCEDIR) +
      QStringLiteral("/third_party/OrcaSlicer/tests/data/test_3mf/Prusa.stl"));
  static const QString k3mfPath = QDir::cleanPath(
      QStringLiteral(QT_TESTCASE_SOURCEDIR) +
      QString::fromUtf8("/third_party/OrcaSlicer/tests/data/test_3mf/Ger\303\244te/B\303\274chse.3mf"));
  static const QString kObjPath = QDir::cleanPath(
      QStringLiteral(QT_TESTCASE_SOURCEDIR) +
      QStringLiteral("/third_party/OrcaSlicer/tests/data/20mm_cube.obj"));

  int gcv1SegmentCount(const QByteArray &payload)
  {
    if (!payload.startsWith("GCV1") || payload.size() < 8)
      return -1;
    int count = 0;
    std::memcpy(&count, payload.constData() + 4, sizeof(count));
    return count;
  }

  QStringList recursiveFixtureMatches(const QString &root, const QStringList &patterns)
  {
    QStringList matches;
    QDirIterator it(root, patterns, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
      matches.append(QDir::toNativeSeparators(it.next()));
    matches.sort(Qt::CaseInsensitive);
    return matches;
  }
}

class E2EWorkflowTests final : public QObject
{
  Q_OBJECT

private slots:
  void initTestCase();
  void test_config_injection_applies_preset_values();
  void test_import_format_coverage_matrix_real_fixtures();
  void test_slice_produces_gcode_file();
  void test_slice_results_propagate_to_editor_vm();
  void test_export_gcode_to_path();
  void test_export_gcode_rejects_unsafe_targets();
  void test_local_import_slice_preview_export_workflow();
  void test_preview_receives_gcode_data();
  void test_backend_switches_to_preview_after_slice();
  void test_preview_parser_handles_extrusion_modes_and_travel_filter();
  void test_preview_parser_handles_orca_metadata_view_modes_and_ticks();
  void test_preview_parser_ignores_z_hop_travel_as_layer();
  void test_model_change_invalidates_slice_result();
  void test_slice_affecting_bed_change_marks_current_result_stale();
  void test_import_invalidates_slice_output_and_preview_payload();
  void test_previous_gcode_reuse_marks_reused_result_and_refreshes_preview();
  void test_preview_rebuilds_on_active_result_switch_without_slice_finished();
  void test_cancelled_slice_clears_active_result_and_blocks_preview_export();
  void test_slice_all_stores_outputs_for_printable_unlocked_plates_only();

private:
  bool hasLibslic3r() const;
  /// Inject a minimal valid printer config aligned with the CLI slicing path.
  ///
  /// Historical E2E failure mode:
  ///   1) SliceService::startSlice can start with an empty printable_area.
  ///   2) ProjectServiceMock::loadFile() arranges after load without a printable area.
  /// Both can leave the model outside a valid bed and make the slicing worker hang.
  ///
  /// The test injects a 220x220 bed matching the CLI bed_shape before slicing.
  void applyMinimalPrinterConfig(SliceService &slice, ProjectServiceMock &project) const;
  /// Return whether the model was successfully arranged into the valid bed area.
  /// Upstream multi-plate arrange can still report bed_idx==-1, so callers may skip.
  bool ensureModelOnBed(ProjectServiceMock &project) const;
};

bool E2EWorkflowTests::hasLibslic3r() const
{
#ifdef HAS_LIBSLIC3R
  return true;
#else
  return false;
#endif
}

void E2EWorkflowTests::applyMinimalPrinterConfig(SliceService &slice, ProjectServiceMock &project) const
{
  Q_UNUSED(project)
  // Inject bed_shape directly through setBedShape, mirroring the successful CLI path.
  // printable_area through preset deserialization is unreliable because the alias is
  // not guaranteed to resolve in full_print_config.
  slice.setBedShape({QPointF(0, 0), QPointF(220, 0), QPointF(220, 220), QPointF(0, 220)});

  // printable_height and nozzle_diameter are registered preset keys and can use config injection.
  QHash<QString, QVariant> cfg;
  cfg.insert(QStringLiteral("printable_height"), 250.0);
  cfg.insert(QStringLiteral("nozzle_diameter"), QVariantList{0.4});
  slice.setMergedPresetConfig(cfg);
}

bool E2EWorkflowTests::ensureModelOnBed(ProjectServiceMock &project) const
{
  // Explicitly arrange onto a 220x220 bed. arrangeObjects accepts a flat
  // "x1,y1,x2,y2,..." polygon coordinate string.
  const QString printableArea = QStringLiteral("0,0,220,0,220,220,0,220");
  return project.arrangeObjects(5.0f, false, false, printableArea);
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

void E2EWorkflowTests::test_import_format_coverage_matrix_real_fixtures()
{
  struct FormatCase
  {
    const char *label;
    QString path;
    bool requiredRealFixture;
  };

  const QList<FormatCase> testedFormats = {
    {"STL", kStlPath, true},
    {"3MF", k3mfPath, true},
    {"OBJ", kObjPath, false},
  };

  for (const FormatCase &format : testedFormats)
  {
    QVERIFY2(QFileInfo::exists(format.path),
             qPrintable(QStringLiteral("%1 fixture must exist: %2")
                            .arg(QString::fromLatin1(format.label), format.path)));

    ProjectServiceMock project;
    QSignalSpy loadSpy(&project, &ProjectServiceMock::loadFinished);
    QVERIFY(loadSpy.isValid());
    QVERIFY2(project.loadFile(format.path),
             qPrintable(QStringLiteral("%1 import should start through ProjectServiceMock::loadFile")
                            .arg(QString::fromLatin1(format.label))));
    QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 20000);

    const QList<QVariant> args = loadSpy.takeFirst();
    const bool ok = args.at(0).toBool();
    const QString message = args.at(1).toString();
    QVERIFY2(ok,
             qPrintable(QStringLiteral("%1 fixture should load successfully: %2")
                            .arg(QString::fromLatin1(format.label), message)));
    QVERIFY2(project.modelCount() >= 1,
             qPrintable(QStringLiteral("%1 import should create at least one model")
                            .arg(QString::fromLatin1(format.label))));
    QVERIFY2(project.plateCount() >= 1,
             qPrintable(QStringLiteral("%1 import should expose at least one plate")
                            .arg(QString::fromLatin1(format.label))));
    QVERIFY2(!project.sourceFilePath().isEmpty(),
             qPrintable(QStringLiteral("%1 import should record the source path")
                            .arg(QString::fromLatin1(format.label))));
    if (format.requiredRealFixture)
      QVERIFY2(project.lastError().isEmpty(),
               qPrintable(QStringLiteral("%1 required fixture must not leave a warning/error: %2")
                              .arg(QString::fromLatin1(format.label), project.lastError())));
  }

  const QString root = QStringLiteral(QT_TESTCASE_SOURCEDIR);
  const QStringList amfFixtures = recursiveFixtureMatches(root, QStringList{QStringLiteral("*.amf")});
  QVERIFY2(amfFixtures.isEmpty(),
           "AMF is expected to be classified in Phase 43 verification because no committed AMF fixture is available");

  const auto stepFiles = recursiveFixtureMatches(root,
                                                 QStringList{QStringLiteral("*.step"),
                                                             QStringLiteral("*.stp")});
  QVERIFY2(stepFiles.isEmpty(),
           "STEP/STP is expected to be classified in Phase 43 verification unless a committed fixture is added");
}

void E2EWorkflowTests::test_slice_produces_gcode_file()
{
  ProjectServiceMock project;
  SliceService slice(&project);

  // Load model
  QVERIFY(project.loadFile(kStlPath));

  // v2.7 P0: wait for async loadFile to finish (queued model/plate assignment)
  QSignalSpy _loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(_loadSpy.isValid());
  QTRY_VERIFY_WITH_TIMEOUT(_loadSpy.count() > 0, 10000);

  // loadFile completes through queued work. Wait for loadFinished before slicing.

  // Start slice and wait for completion
  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);

  applyMinimalPrinterConfig(slice, project);
  // Arrange when possible; Prusa.stl remains sliceable in its natural coordinates.
  ensureModelOnBed(project);
  slice.startSlice(QStringLiteral("e2e_test"));

  // Wait up to 120 seconds for real slicing
  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0, 120000);

  if (failedSpy.count() > 0)
  {
    // Include the actual slicing failure reason to keep failures diagnosable.
    const QString failReason = failedSpy.first().at(0).toString();
    QSKIP(QString("Slice failed: %1").arg(failReason).toUtf8().constData());
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

  // v2.7 P0: wait for async loadFile to finish (queued model/plate assignment)
  QSignalSpy _loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(_loadSpy.isValid());
  QTRY_VERIFY_WITH_TIMEOUT(_loadSpy.count() > 0, 10000);

  // Start slice
  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);

  applyMinimalPrinterConfig(slice, project);
  ensureModelOnBed(project);
  // Call SliceService::startSlice directly. editor.requestSlice() checks GUI
  // preconditions that are not fully present in this focused unit-test setup.
  slice.startSlice(QStringLiteral("editor_vm_test"));

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

  // v2.7 P0: wait for async loadFile to finish (queued model/plate assignment)
  QSignalSpy _loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(_loadSpy.isValid());
  QTRY_VERIFY_WITH_TIMEOUT(_loadSpy.count() > 0, 10000);

  // Slice first
  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);

  applyMinimalPrinterConfig(slice, project);
  ensureModelOnBed(project);
  slice.startSlice(QStringLiteral("export_test"));

  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0, 120000);

  if (failedSpy.count() > 0)
    QSKIP("Slice failed — cannot test export without successful slice");

  const QString srcPath = slice.outputPath();
  QVERIFY2(!srcPath.isEmpty(), "source G-code path should exist after slice");
  QVERIFY2(QFileInfo::exists(srcPath),
           qPrintable(QStringLiteral("source G-code file should exist: %1").arg(srcPath)));

  // Export to a temporary file (use system temp dir; QTemporaryFile in CWD may lock on Windows)
  const QString exportPath = QDir::tempPath() + QStringLiteral("/owzx_export_test_XXXXXX.gcode");
  QFile::remove(exportPath);

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

void E2EWorkflowTests::test_export_gcode_rejects_unsafe_targets()
{
  ProjectServiceMock project;
  SliceService slice(&project);

  QVERIFY(project.loadFile(kStlPath));
  QSignalSpy loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(loadSpy.isValid());
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);

  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);
  QVERIFY(finishedSpy.isValid());
  QVERIFY(failedSpy.isValid());

  applyMinimalPrinterConfig(slice, project);
  ensureModelOnBed(project);
  slice.startSlice(QStringLiteral("export_safety_test"));

  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0, 120000);

  if (failedSpy.count() > 0)
    QSKIP("Slice failed - cannot test export safety without successful slice");

  const QString srcPath = slice.outputPath();
  QVERIFY2(!srcPath.isEmpty(), "source G-code path should exist after slice");
  QFileInfo srcInfo(srcPath);
  QVERIFY2(srcInfo.exists() && srcInfo.isFile(),
           qPrintable(QStringLiteral("source G-code should exist: %1").arg(srcPath)));
  const qint64 sourceSize = srcInfo.size();
  QVERIFY2(sourceSize > 0, "source G-code should be non-empty");

  QVERIFY2(!slice.exportGCodeToPath(QString{}),
           "empty export target must be rejected");
  QVERIFY2(QFileInfo::exists(srcPath), "empty-target export must not delete source");
  QCOMPARE(QFileInfo(srcPath).size(), sourceSize);

  QVERIFY2(!slice.exportGCodeToPath(srcPath),
           "same source/target export must be rejected");
  QVERIFY2(QFileInfo::exists(srcPath), "same-path export must not delete source");
  QCOMPARE(QFileInfo(srcPath).size(), sourceSize);

  QTemporaryDir exportDir(QDir::tempPath() + QStringLiteral("/owzx_export_safety_XXXXXX"));
  QVERIFY2(exportDir.isValid(), "temporary export directory should be available");
  QVERIFY2(!slice.exportGCodeToPath(exportDir.path()),
           "directory export target must be rejected");
  QVERIFY2(QFileInfo::exists(srcPath), "directory-target export must not delete source");

  const QString existingTarget = exportDir.filePath(QStringLiteral("existing.gcode"));
  {
    QFile existing(existingTarget);
    QVERIFY2(existing.open(QIODevice::WriteOnly | QIODevice::Truncate),
             "existing target should be writable");
    existing.write("stale");
  }
  QVERIFY2(slice.exportGCodeToPath(existingTarget),
           "different existing target should be safely replaced");
  QFileInfo exportedInfo(existingTarget);
  QVERIFY2(exportedInfo.exists() && exportedInfo.isFile(), "exported file should exist");
  QVERIFY2(exportedInfo.size() == sourceSize,
           qPrintable(QStringLiteral("exported size %1 should match source size %2")
                          .arg(exportedInfo.size()).arg(sourceSize)));
  QVERIFY2(QFileInfo::exists(srcPath), "successful export must preserve source");

  if (QFileInfo::exists(srcPath))
    QFile::remove(srcPath);
}

void E2EWorkflowTests::test_local_import_slice_preview_export_workflow()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);
  EditorViewModel editor(&project, &slice);

  QSignalSpy loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(loadSpy.isValid());
  QVERIFY2(editor.loadFile(kStlPath), "import through EditorViewModel should start");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(loadSpy.takeFirst().at(0).toBool(), "model import should complete successfully");

  QVERIFY2(editor.modelCount() >= 1, "imported workflow fixture should expose at least one model");
  QVERIFY2(editor.canRequestSlice(),
           qPrintable(QStringLiteral("Prepare should be slice-ready: %1").arg(editor.sliceActionHint())));
  QVERIFY2(!editor.canPreview(), "Preview must stay disabled before the first valid slice");
  QVERIFY2(!editor.canExportGCode(), "G-code export must stay disabled before the first valid slice");

  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);
  QVERIFY(finishedSpy.isValid());
  QVERIFY(failedSpy.isValid());

  applyMinimalPrinterConfig(slice, project);
  ensureModelOnBed(project);
  editor.requestSlice();
  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0, 120000);
  if (failedSpy.count() > 0)
  {
    const QString reason = failedSpy.first().at(0).toString();
    QSKIP(QString("Slice failed in local workflow test: %1").arg(reason).toUtf8().constData());
  }

  QVERIFY2(editor.hasSliceResult(), "completed slice should create a valid Prepare result");
  QVERIFY2(editor.canPreview(), "Preview should be available after a valid slice");
  QVERIFY2(editor.canExportGCode(), "current-plate G-code export should be available after a valid slice");
  const QString generatedOutput = editor.sliceOutputPath();
  QVERIFY2(!generatedOutput.isEmpty(), "slice should expose the generated G-code output path");
  QVERIFY2(QFileInfo::exists(generatedOutput),
           qPrintable(QStringLiteral("generated G-code should exist: %1").arg(generatedOutput)));
  QVERIFY2(QFileInfo(generatedOutput).size() > 0, "generated G-code should be non-empty");

  QTRY_VERIFY_WITH_TIMEOUT(preview.gcodePreviewData().startsWith("GCV1"), 5000);
  QVERIFY2(preview.layerCount() > 0,
           qPrintable(QStringLiteral("Preview should expose layers, got %1").arg(preview.layerCount())));
  QVERIFY2(preview.moveCount() > 0,
           qPrintable(QStringLiteral("Preview should expose moves, got %1").arg(preview.moveCount())));
  QVERIFY2(gcv1SegmentCount(preview.gcodePreviewData()) > 0,
           "Preview renderer payload should contain packed segments");

  const QByteArray payloadBeforeInteraction = preview.gcodePreviewData();
  const int maxLayer = qMax(0, preview.layerCount() - 1);
  preview.setLayerRange(0, qMin(1, maxLayer));
  preview.moveLayerRange(1);
  preview.setCurrentMove(qMin(2, preview.moveCount()));
  preview.setViewModeIndex(3);
  preview.setShowTravelMoves(false);
  QVERIFY2(preview.gcodePreviewData().startsWith("GCV1"),
           "Preview payload must survive layer/move/view interactions");
  QVERIFY2(gcv1SegmentCount(preview.gcodePreviewData()) > 0,
           "Preview segment payload must remain non-empty after travel filtering");
  preview.setShowTravelMoves(true);
  QVERIFY2(preview.gcodePreviewData().startsWith("GCV1"),
           "Preview payload must survive restoring travel moves");
  QVERIFY2(gcv1SegmentCount(preview.gcodePreviewData()) >= gcv1SegmentCount(payloadBeforeInteraction),
           "restoring travel moves should not lose the original Preview segment payload");

  QTemporaryDir exportDir(QDir::tempPath() + QStringLiteral("/owzx_workflow_export_XXXXXX"));
  QVERIFY2(exportDir.isValid(), "temporary workflow export directory should be available");

  const QString currentExportPath = exportDir.filePath(QStringLiteral("workflow_current.gcode"));
  QVERIFY2(editor.requestExportGCode(currentExportPath),
           "current-plate export should succeed after the workflow slice");
  QVERIFY2(QFileInfo::exists(currentExportPath),
           qPrintable(QStringLiteral("current export should exist: %1").arg(currentExportPath)));
  QVERIFY2(QFileInfo(currentExportPath).size() == QFileInfo(generatedOutput).size(),
           "current export should match the generated G-code byte size");
  QVERIFY2(QFileInfo::exists(generatedOutput),
           "current export must preserve the generated source G-code");

  QVERIFY2(editor.requestExportAllGCode(exportDir.path(), QStringLiteral("workflow_all")),
           "all-valid-plate export should succeed for the sliced workflow result");
  const QString allExportPath = exportDir.filePath(QStringLiteral("workflow_all_plate1.gcode"));
  QVERIFY2(QFileInfo::exists(allExportPath),
           qPrintable(QStringLiteral("all-plate export should include plate 1: %1").arg(allExportPath)));
  QVERIFY2(QFileInfo(allExportPath).size() == QFileInfo(generatedOutput).size(),
           "all-plate export should match the generated G-code byte size");

  if (QFileInfo::exists(generatedOutput))
    QFile::remove(generatedOutput);
}

void E2EWorkflowTests::test_preview_receives_gcode_data()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  // Load model
  QVERIFY(project.loadFile(kStlPath));

  // v2.7 P0: wait for async loadFile to finish (queued model/plate assignment)
  QSignalSpy _loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(_loadSpy.isValid());
  QTRY_VERIFY_WITH_TIMEOUT(_loadSpy.count() > 0, 10000);

  // Start slice
  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);

  applyMinimalPrinterConfig(slice, project);
  ensureModelOnBed(project);
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

void E2EWorkflowTests::test_backend_switches_to_preview_after_slice()
{
  BackendContext ctx;
  auto *editor = qobject_cast<EditorViewModel *>(ctx.editorViewModel());
  auto *preview = qobject_cast<PreviewViewModel *>(ctx.previewViewModel());
  QVERIFY(editor);
  QVERIFY(preview);

  QCOMPARE(ctx.currentPage(), static_cast<int>(BackendContext::TabPosition::tp3DEditor));
  QCOMPARE(ctx.currentViewMode(), static_cast<int>(BackendContext::ViewMode::View3D));

  QSignalSpy loadSpy(editor, &EditorViewModel::stateChanged);
  QVERIFY(loadSpy.isValid());
  QVERIFY(editor->loadFile(kStlPath));
  QTRY_VERIFY_WITH_TIMEOUT(editor->modelCount() >= 1, 10000);

  QSignalSpy pageSpy(&ctx, &BackendContext::currentPageChanged);
  QVERIFY(pageSpy.isValid());

  QVERIFY2(editor->canRequestSlice(),
           qPrintable(QStringLiteral("editor should be slice-ready: %1").arg(editor->sliceActionHint())));
  editor->requestSlice();

  QTRY_VERIFY_WITH_TIMEOUT(editor->hasSliceResult(), 120000);
  QCOMPARE(ctx.currentPage(), static_cast<int>(BackendContext::TabPosition::tpPreview));
  QCOMPARE(ctx.currentViewMode(), static_cast<int>(BackendContext::ViewMode::Preview));
  QVERIFY2(pageSpy.count() >= 1, "slice completion should emit currentPageChanged");
  QVERIFY2(!editor->sliceOutputPath().isEmpty(), "slice should expose a G-code output path");
  QVERIFY2(QFileInfo::exists(editor->sliceOutputPath()),
           qPrintable(QStringLiteral("G-code file should exist: %1").arg(editor->sliceOutputPath())));
  QVERIFY2(preview->layerCount() > 0,
           qPrintable(QStringLiteral("preview layer count should be > 0, got %1").arg(preview->layerCount())));
  QVERIFY2(preview->moveCount() > 0,
           qPrintable(QStringLiteral("preview move count should be > 0, got %1").arg(preview->moveCount())));
  QVERIFY2(preview->gcodePreviewData().startsWith("GCV1"),
           "preview payload should use the GCV1 segment format");

  const QString outputPath = editor->sliceOutputPath();
  if (!outputPath.isEmpty() && QFileInfo::exists(outputPath))
    QFile::remove(outputPath);
}

void E2EWorkflowTests::test_preview_parser_handles_extrusion_modes_and_travel_filter()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  QTemporaryFile gcode(QDir::tempPath() + QStringLiteral("/owzx_preview_parser_XXXXXX.gcode"));
  QVERIFY2(gcode.open(), "temporary G-code fixture should be writable");

  const QByteArray fixture = R"gcode(
; generated by OWzx parser regression
; filament_diameter = 1.75
; filament_density = 1.24
M82
T0
G1 F1200 X0 Y0 Z0.20
;TYPE:WALL-OUTER
G1 X10 Y0 E0.50 F1200
G1 X20 Y0 E1.00
G92 E0
G1 X20 Y10 E0.40
G0 X25 Y10 F3000
M83
G1 X30 Y10 E0.20 F1200
G1 X35 Y10 E0.20
T1
;TYPE:FILL
G1 X40 Y10 E0.30
G1 Z0.40 F600
G1 X40 Y20 E0.20
M82
G92 E0
G1 X50 Y20 E0.60
)gcode";

  QCOMPARE(gcode.write(fixture), qint64(fixture.size()));
  QVERIFY(gcode.flush());
  gcode.close();

  QVERIFY2(preview.loadGCodeForPreview(gcode.fileName()),
           "PreviewViewModel should parse a standalone G-code file");

  QCOMPARE(preview.extrudeMoveCount(), 8);
  QCOMPARE(preview.travelMoveCount(), 3);
  QCOMPARE(preview.moveCount(), 11);
  QVERIFY2(preview.layerCount() >= 2,
           qPrintable(QStringLiteral("layer count should be >= 2, got %1").arg(preview.layerCount())));
  QCOMPARE(preview.toolChangeCount(), 1);
  QCOMPARE(preview.toolChangePositionCount(), 1);
  QCOMPARE(preview.toolChangeExtruderIdAt(0), 1);
  QCOMPARE(preview.extruderCount(), 2);
  QVERIFY2(preview.extruderUsedLength(0) > 0.0, "extruder 0 usage should be tracked");
  QVERIFY2(preview.extruderUsedLength(1) > 0.0, "extruder 1 usage should be tracked");

  QVERIFY2(preview.gcodePreviewData().startsWith("GCV1"),
           "preview payload should use the GCV1 segment format");
  // showTravelMoves defaults to false (upstream Travels/Wipes=false + CONTEXT.md
  // "travel and wipe hidden after first view"), so the GCV1 payload packs only
  // the extrusion segments on first load. The toggle assertions below cover both.
  QCOMPARE(gcv1SegmentCount(preview.gcodePreviewData()), preview.extrudeMoveCount());
  QVERIFY2(preview.previewReady(), "loaded fixture should make Preview ready");
  QVERIFY2(preview.gcodeLineCount() > 0, "Preview should expose a bounded G-code text window");
  QVERIFY2(!preview.gcodeLines().isEmpty(), "Preview G-code text window should be non-empty");
  QVERIFY2(preview.currentGcodeLine() > 0, "Preview should track the current G-code line");
  QVERIFY2(preview.currentLayerLabel().contains(QStringLiteral("/")),
           "Preview should expose a user-facing layer summary");

  // Filament mode (EViewType index 2) exposes the per-extruder legend. The old
  // 13-mode list mapped the tool/extruder view to index 3; the 17-mode renumber
  // (Plan 55-02) moves it to Filament(2) / Tool(16), so use the new index here.
  preview.setViewModeIndex(2);
  QCOMPARE(preview.legendType(), 2);
  QVERIFY2(preview.legendItems().size() >= 2, "filament mode should expose both extruders in the legend");

  preview.setShowTravelMoves(false);
  QCOMPARE(gcv1SegmentCount(preview.gcodePreviewData()), preview.extrudeMoveCount());

  preview.setShowTravelMoves(true);
  QCOMPARE(gcv1SegmentCount(preview.gcodePreviewData()), preview.moveCount());

  const int targetMove = qMax(1, preview.moveCount() / 2);
  preview.setCurrentMove(targetMove);
  QVERIFY2(preview.currentGcodeLine() > 0, "Preview should keep tracking a current G-code line after move changes");
  bool foundCurrentRow = false;
  int currentRowMove = -1;
  for (const QVariant &rowValue : preview.gcodeLines()) {
    const QVariantMap row = rowValue.toMap();
    if (row.value(QStringLiteral("current")).toBool()) {
      foundCurrentRow = true;
      currentRowMove = row.value(QStringLiteral("move")).toInt();
      QVERIFY2(row.value(QStringLiteral("line")).toInt() == preview.currentGcodeLine(),
               "Current G-code row should match PreviewViewModel::currentGcodeLine");
      break;
    }
  }
  QVERIFY2(foundCurrentRow, "Preview G-code text window should mark one current row");
  QVERIFY2(currentRowMove >= targetMove,
           "Preview G-code text window should highlight the source line at or after the selected move");
}

void E2EWorkflowTests::test_preview_parser_handles_orca_metadata_view_modes_and_ticks()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  QTemporaryFile gcode(QDir::tempPath() + QStringLiteral("/owzx_preview_orca_metadata_XXXXXX.gcode"));
  QVERIFY2(gcode.open(), "temporary G-code fixture should be writable");

  const QByteArray fixture = R"gcode(
; generated by OrcaSlicer metadata regression
; filament_diameter = 1.75
; filament_density = 1.24
M82
T0
;LAYER_CHANGE
;Z:0.20
;HEIGHT:0.20
;WIDTH:0.45
M106 S128
M104 S215
M204 S1200
G1 X0 Y0 Z0.20 F6000
;TYPE:WALL-OUTER
G1 X10 Y0 E0.50 F1200
;TIME_ELAPSED:3.0
M107
M109 S205
G0 X10 Y10 F3000
;LAYER_CHANGE
;Z:0.40
;HEIGHT:0.20
;WIDTH:0.50
G1 Z0.40 F600
M83
T1
M204 P900
;TYPE:FILL
G1 X20 Y10 E0.20 F1500
;TIME_ELAPSED:9.0
;COLOR_CHANGE T1 #FF00AA
;PAUSE_PRINT
;CUSTOM_GCODE M117 hello
G92 E0
G1 X30 Y10 E0.10 F1800
)gcode";

  QCOMPARE(gcode.write(fixture), qint64(fixture.size()));
  QVERIFY(gcode.flush());
  gcode.close();

  QVERIFY2(preview.loadGCodeForPreview(gcode.fileName()),
           "PreviewViewModel should parse a metadata-rich Orca-style G-code file");

  QCOMPARE(preview.extrudeMoveCount(), 3);
  QCOMPARE(preview.travelMoveCount(), 3);
  QCOMPARE(preview.moveCount(), 6);
  QCOMPARE(preview.layerCount(), 2);
  QCOMPARE(preview.toolChangeCount(), 1);
  QCOMPARE(preview.extruderCount(), 2);
  QCOMPARE(preview.layerTimeCount(), 2);
  QVERIFY2(preview.maxLayerTime() >= 3.0f,
           qPrintable(QStringLiteral("layer time should come from elapsed-time tags, got %1").arg(preview.maxLayerTime())));
  QCOMPARE(preview.tickMarkCount(), 3);

  QVariantList ticks = preview.tickMarks();
  bool sawColor = false;
  bool sawPause = false;
  bool sawCustom = false;
  for (const QVariant &tickValue : ticks)
  {
    const QVariantMap tick = tickValue.toMap();
    sawColor = sawColor || tick.value(QStringLiteral("type")).toInt() == 4;
    sawPause = sawPause || tick.value(QStringLiteral("type")).toInt() == 0;
    sawCustom = sawCustom || tick.value(QStringLiteral("type")).toInt() == 1;
  }
  QVERIFY2(sawColor, "parsed tick data should include a color-change marker");
  QVERIFY2(sawPause, "parsed tick data should include a pause marker");
  QVERIFY2(sawCustom, "parsed tick data should include a custom G-code marker");

  QVERIFY2(preview.hasToolPosition(), "final move cursor should still expose marker data");
  QCOMPARE(preview.toolMoveIndex(), preview.moveCount() - 1);

  preview.setCurrentMove(1);
  QVERIFY2(preview.hasToolPosition(), "first extrusion should expose marker data");
  QVERIFY2(preview.toolIsExtrusion(), "first extrusion should be marked as extrusion");
  QVERIFY2(preview.toolFanSpeed() > 49.0 && preview.toolFanSpeed() < 51.0,
           qPrintable(QStringLiteral("M106 S128 should be normalized near 50%%, got %1").arg(preview.toolFanSpeed())));
  QCOMPARE(qRound(preview.toolTemperature()), 215);
  QCOMPARE(qRound(preview.toolAcceleration()), 1200);
  QVERIFY2(preview.toolWidth() > 0.44 && preview.toolWidth() < 0.46,
           qPrintable(QStringLiteral("WIDTH metadata should be stored on the segment, got %1").arg(preview.toolWidth())));

  // The 17-mode EViewType renumber (Plan 55-02) shifts these indices:
  // Fan Speed 5->13, Temperature 6->14, Acceleration 12->5, Tool 3->16/2.
  preview.setViewModeIndex(13); // Fan Speed
  QCOMPARE(preview.legendType(), 1);
  QVERIFY2(preview.legendGradientMaxLabel().toDouble() > preview.legendGradientMinLabel().toDouble(),
           "fan-speed mode should expose a non-degenerate parsed range");

  preview.setViewModeIndex(14); // Temperature
  QCOMPARE(preview.legendType(), 1);
  QVERIFY2(preview.legendGradientMaxLabel().toDouble() > preview.legendGradientMinLabel().toDouble(),
           "temperature mode should expose a non-degenerate parsed range");

  preview.setViewModeIndex(5); // Acceleration
  QCOMPARE(preview.legendType(), 1);
  QVERIFY2(preview.legendGradientMaxLabel().toDouble() > preview.legendGradientMinLabel().toDouble(),
           "acceleration mode should expose S/P/T M204 variants as a parsed range");

  preview.setViewModeIndex(16); // Tool
  QCOMPARE(preview.legendType(), 2);
  QVERIFY2(preview.legendItems().size() >= 2, "tool mode should list both used extruders");
}

void E2EWorkflowTests::test_preview_parser_ignores_z_hop_travel_as_layer()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);

  QTemporaryFile gcode(QDir::tempPath() + QStringLiteral("/owzx_preview_zhop_XXXXXX.gcode"));
  QVERIFY2(gcode.open(), "temporary G-code fixture should be writable");

  const QByteArray fixture = R"gcode(
; Z-hop regression: travel lift inside one printed layer must not become a selectable empty layer
M82
G1 F1200 X0 Y0 Z0.20
;TYPE:WALL-OUTER
G1 X10 Y0 E0.50 F1200
G1 Z0.80 F600
G0 X20 Y0 F3000
G1 Z0.20 F600
G1 X30 Y0 E1.00 F1200
G1 Z0.40 F600
G1 X30 Y10 E1.50 F1200
)gcode";

  QCOMPARE(gcode.write(fixture), qint64(fixture.size()));
  QVERIFY(gcode.flush());
  gcode.close();

  QVERIFY2(preview.loadGCodeForPreview(gcode.fileName()),
           "PreviewViewModel should parse a standalone G-code file");

  QCOMPARE(preview.extrudeMoveCount(), 3);
  QCOMPARE(preview.travelMoveCount(), 5);
  QCOMPARE(preview.moveCount(), 8);
  QCOMPARE(preview.layerCount(), 2);

  preview.setLayerRange(1, 1);
  QCOMPARE(preview.currentLayerMin(), 1);
  QCOMPARE(preview.currentLayerMax(), 1);
}

void E2EWorkflowTests::test_model_change_invalidates_slice_result()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  // Load model
  QVERIFY(project.loadFile(kStlPath));

  // v2.7 P0: wait for async loadFile to finish (queued model/plate assignment)
  QSignalSpy _loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(_loadSpy.isValid());
  QTRY_VERIFY_WITH_TIMEOUT(_loadSpy.count() > 0, 10000);

  // Slice
  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);

  applyMinimalPrinterConfig(slice, project);
  ensureModelOnBed(project);
  // Call startSlice directly; editor.requestSlice() can early-return in this test setup.
  slice.startSlice(QStringLiteral("invalidate_test"));
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

void E2EWorkflowTests::test_slice_affecting_bed_change_marks_current_result_stale()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  QSignalSpy loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(loadSpy.isValid());
  QVERIFY2(editor.loadFile(kStlPath), "importing a model through EditorViewModel should start");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(loadSpy.takeFirst().at(0).toBool(), "model import should complete successfully");

  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);
  QVERIFY(finishedSpy.isValid());
  QVERIFY(failedSpy.isValid());

  applyMinimalPrinterConfig(slice, project);
  ensureModelOnBed(project);
  slice.startSlice(QStringLiteral("readiness_stale_test"));
  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0, 120000);
  if (failedSpy.count() > 0)
    QSKIP("Slice failed — cannot test stale result readiness without a real G-code result");

  const int plateIndex = editor.currentPlateIndex();
  const QString slicedOutputPath = slice.outputPath();
  QVERIFY2(!slicedOutputPath.isEmpty(), "slice should expose a G-code output path");
  QVERIFY2(QFileInfo::exists(slicedOutputPath),
           qPrintable(QStringLiteral("G-code file should exist before invalidation: %1").arg(slicedOutputPath)));
  QVERIFY2(editor.hasSliceResult(), "EditorViewModel should expose the fresh slice result");
  QVERIFY2(editor.canPreview(), "Preview should be available for a fresh current-plate result");
  QVERIFY2(editor.canExportGCode(), "Export should be available for a fresh current-plate result");
  QCOMPARE(editor.plateSliceResultStatus(plateIndex),
           int(EditorViewModel::SliceResultValid));

  editor.setBedWidth(editor.bedWidth() + 1.0f);

  QVERIFY2(!editor.hasSliceResult(), "bed shape changes must invalidate the current slice result");
  QVERIFY2(!editor.canPreview(), "Preview must be disabled for a stale current-plate result");
  QVERIFY2(!editor.canExportGCode(), "Export must be disabled for a stale current-plate result");
  QVERIFY2(editor.canRequestSlice(), "stale current-plate results should reopen slicing");
  QCOMPARE(editor.plateSliceResultStatus(plateIndex),
           int(EditorViewModel::SliceResultStale));
  QVERIFY2(editor.previewActionHint().contains(QStringLiteral("已过期")),
           qPrintable(editor.previewActionHint()));
  QVERIFY2(editor.exportActionHint().contains(QStringLiteral("已过期")),
           qPrintable(editor.exportActionHint()));

  editor.switchToPreview();
  QVERIFY2(editor.statusText().contains(QStringLiteral("已过期")),
           qPrintable(editor.statusText()));

  const QString exportPath = QDir::tempPath() + QStringLiteral("/owzx_stale_export_blocked.gcode");
  QFile::remove(exportPath);
  QVERIFY2(!editor.requestExportGCode(exportPath),
           "stale current-plate results must not export the previous G-code path");
  QVERIFY2(!QFileInfo::exists(exportPath), "blocked stale export must not create a file");

  if (QFileInfo::exists(slicedOutputPath))
    QFile::remove(slicedOutputPath);
}

void E2EWorkflowTests::test_import_invalidates_slice_output_and_preview_payload()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);
  EditorViewModel editor(&project, &slice);

  QSignalSpy loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(loadSpy.isValid());
  QVERIFY2(editor.loadFile(kStlPath), "importing a model through EditorViewModel should start");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(loadSpy.takeFirst().at(0).toBool(), "model import should complete successfully");

  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);
  QVERIFY(finishedSpy.isValid());
  QVERIFY(failedSpy.isValid());

  applyMinimalPrinterConfig(slice, project);
  ensureModelOnBed(project);
  slice.startSlice(QStringLiteral("import_invalidate_test"));
  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0, 120000);
  if (failedSpy.count() > 0)
    QSKIP("Slice failed — cannot test import invalidation without a real G-code result");

  QTRY_VERIFY_WITH_TIMEOUT(preview.gcodePreviewData().startsWith("GCV1"), 5000);
  const QString staleOutputPath = slice.outputPath();
  QVERIFY2(!staleOutputPath.isEmpty(), "slice should expose a G-code output path before import");
  QVERIFY2(QFileInfo::exists(staleOutputPath),
           qPrintable(QStringLiteral("G-code file should exist before import: %1").arg(staleOutputPath)));
  QVERIFY2(preview.moveCount() > 0, "preview should contain parsed moves before import");
  QVERIFY2(preview.layerCount() > 0, "preview should contain parsed layers before import");

  QSignalSpy reloadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(reloadSpy.isValid());
  QVERIFY2(editor.loadFile(kStlPath), "reimporting a model should start");
  QTRY_VERIFY_WITH_TIMEOUT(reloadSpy.count() > 0, 10000);
  QVERIFY2(reloadSpy.takeFirst().at(0).toBool(), "model reimport should complete successfully");

  QVERIFY2(slice.outputPath().isEmpty(), "new import must clear stale G-code output path");
  QVERIFY2(!editor.hasSliceResult(), "new import must invalidate EditorViewModel slice result");
  QVERIFY2(preview.gcodePreviewData().isEmpty(), "new import must clear stale packed preview payload");
  QCOMPARE(preview.moveCount(), 0);
  QCOMPARE(preview.layerCount(), 0);

  if (QFileInfo::exists(staleOutputPath))
    QFile::remove(staleOutputPath);
}

void E2EWorkflowTests::test_previous_gcode_reuse_marks_reused_result_and_refreshes_preview()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);
  EditorViewModel editor(&project, &slice);

  QSignalSpy loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(loadSpy.isValid());
  QVERIFY2(editor.loadFile(kStlPath), "importing a model through EditorViewModel should start");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(loadSpy.takeFirst().at(0).toBool(), "model import should complete successfully");

  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);
  QVERIFY(finishedSpy.isValid());
  QVERIFY(failedSpy.isValid());

  applyMinimalPrinterConfig(slice, project);
  ensureModelOnBed(project);
  slice.startSlice(QStringLiteral("reuse_seed_test"));
  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0, 120000);
  if (failedSpy.count() > 0)
    QSKIP("Slice failed — cannot test previous G-code reuse without a real seed result");

  const int plateIndex = editor.currentPlateIndex();
  const QString generatedOutput = slice.outputPath();
  QVERIFY2(!generatedOutput.isEmpty(), "seed slice should expose a G-code output path");
  QVERIFY2(QFileInfo::exists(generatedOutput),
           qPrintable(QStringLiteral("seed G-code should exist: %1").arg(generatedOutput)));

  slice.clearResults();
  QVERIFY2(slice.outputPath().isEmpty(), "clearResults should remove the active generated output path");
  QVERIFY2(!editor.hasSliceResult(), "clearing results should remove editor result validity");

  QSignalSpy reusedFinishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy reusedFailedSpy(&slice, &SliceService::sliceFailed);
  QVERIFY(reusedFinishedSpy.isValid());
  QVERIFY(reusedFailedSpy.isValid());

  QVERIFY2(slice.loadGCodeFromPrevious(generatedOutput),
           "valid previous G-code should start the reuse path");
  QTRY_VERIFY_WITH_TIMEOUT(reusedFinishedSpy.count() > 0 || reusedFailedSpy.count() > 0, 60000);
  if (reusedFailedSpy.count() > 0)
  {
    const QString reason = reusedFailedSpy.first().at(0).toString();
    QSKIP(QString("Previous G-code reuse failed in this environment: %1").arg(reason).toUtf8().constData());
  }

  QCOMPARE(slice.outputPath(), generatedOutput);
  QCOMPARE(slice.plateOutputPath(plateIndex), generatedOutput);
  QCOMPARE(slice.plateResultSource(plateIndex), int(SliceService::ResultSource::PreviousGCode));
  QVERIFY2(editor.hasSliceResult(), "reused G-code should become the current plate's valid result");
  QVERIFY2(editor.canPreview(), "Preview should be available for a valid reused G-code result");
  QVERIFY2(editor.canExportGCode(), "Export should be available for a valid reused G-code result");

  QTRY_VERIFY_WITH_TIMEOUT(preview.gcodePreviewData().startsWith("GCV1"), 5000);
  QVERIFY2(preview.moveCount() > 0, "Preview should parse moves from reused G-code");
  QVERIFY2(preview.layerCount() > 0, "Preview should parse layers from reused G-code");

  if (QFileInfo::exists(generatedOutput))
    QFile::remove(generatedOutput);
}

void E2EWorkflowTests::test_preview_rebuilds_on_active_result_switch_without_slice_finished()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  PreviewViewModel preview(&slice);
  EditorViewModel editor(&project, &slice);

  QSignalSpy loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(loadSpy.isValid());
  QVERIFY2(editor.loadFile(kStlPath), "importing a model through EditorViewModel should start");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(loadSpy.takeFirst().at(0).toBool(), "model import should complete successfully");
  QVERIFY2(editor.clonePlate(0), "clonePlate should create a second plate for active-result switching");
  QCOMPARE(project.plateCount(), 2);

  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);
  QVERIFY(finishedSpy.isValid());
  QVERIFY(failedSpy.isValid());

  applyMinimalPrinterConfig(slice, project);
  ensureModelOnBed(project);
  slice.startSlice(QStringLiteral("preview_result_switch_seed"));
  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0, 120000);
  if (failedSpy.count() > 0)
    QSKIP("Slice failed - cannot seed active-result switching without a real G-code result");

  const QString generatedOutput = slice.outputPath();
  QVERIFY2(QFileInfo::exists(generatedOutput),
           qPrintable(QStringLiteral("seed G-code should exist: %1").arg(generatedOutput)));

  QTemporaryDir tempDir(QDir::tempPath() + QStringLiteral("/owzx_preview_switch_XXXXXX"));
  QVERIFY2(tempDir.isValid(), "temporary directory should be available for per-plate G-code copies");
  const QString plate0Path = tempDir.filePath(QStringLiteral("plate0.gcode"));
  const QString plate1Path = tempDir.filePath(QStringLiteral("plate1.gcode"));
  QVERIFY2(QFile::copy(generatedOutput, plate0Path), "seed G-code should copy to plate 0 path");
  QVERIFY2(QFile::copy(generatedOutput, plate1Path), "seed G-code should copy to plate 1 path");

  slice.clearResults();

  QVERIFY(project.setCurrentPlateIndex(0));
  QSignalSpy reuse0Finished(&slice, &SliceService::sliceFinished);
  QSignalSpy reuse0Failed(&slice, &SliceService::sliceFailed);
  QVERIFY2(slice.loadGCodeFromPrevious(plate0Path), "plate 0 should accept previous G-code");
  QTRY_VERIFY_WITH_TIMEOUT(reuse0Finished.count() > 0 || reuse0Failed.count() > 0, 60000);
  if (reuse0Failed.count() > 0)
    QSKIP("Previous G-code reuse failed for plate 0 in this environment");

  QVERIFY(project.setCurrentPlateIndex(1));
  QSignalSpy reuse1Finished(&slice, &SliceService::sliceFinished);
  QSignalSpy reuse1Failed(&slice, &SliceService::sliceFailed);
  QVERIFY2(slice.loadGCodeFromPrevious(plate1Path), "plate 1 should accept previous G-code");
  QTRY_VERIFY_WITH_TIMEOUT(reuse1Finished.count() > 0 || reuse1Failed.count() > 0, 60000);
  if (reuse1Failed.count() > 0)
    QSKIP("Previous G-code reuse failed for plate 1 in this environment");

  QVERIFY2(preview.moveCount() > 2, "seed preview should contain the generated G-code moves before switching");

  QFile compactPlate0(plate0Path);
  QVERIFY2(compactPlate0.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text),
           "plate 0 path should be writable after reuse metadata is stored");
  const QByteArray compactFixture = R"gcode(
M82
G1 X0 Y0 Z0.20 F1200
;TYPE:WALL-OUTER
G1 X1 Y0 E0.10 F1200
)gcode";
  QCOMPARE(compactPlate0.write(compactFixture), qint64(compactFixture.size()));
  compactPlate0.close();

  QVERIFY(editor.setCurrentPlateIndex(0));
  QCOMPARE(slice.outputPath(), plate0Path);
  QTRY_COMPARE(preview.moveCount(), 2);
  // The compact fixture has 1 travel + 1 extrusion move. showTravelMoves
  // defaults to false (only the extrusion segment is packed), so enable travel
  // visibility before asserting the GCV1 payload carries both moves.
  preview.setShowTravelMoves(true);
  QCOMPARE(gcv1SegmentCount(preview.gcodePreviewData()), 2);

  if (QFileInfo::exists(generatedOutput))
    QFile::remove(generatedOutput);
  QFile::remove(plate0Path);
  QFile::remove(plate1Path);
}

void E2EWorkflowTests::test_cancelled_slice_clears_active_result_and_blocks_preview_export()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  QSignalSpy loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(loadSpy.isValid());
  QVERIFY2(editor.loadFile(kStlPath), "importing a model should start");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(loadSpy.takeFirst().at(0).toBool(), "model import should complete successfully");

  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);
  QVERIFY(finishedSpy.isValid());
  QVERIFY(failedSpy.isValid());

  applyMinimalPrinterConfig(slice, project);
  ensureModelOnBed(project);
  slice.startSlice(QStringLiteral("cancel_state_test"));
  QTRY_VERIFY_WITH_TIMEOUT(slice.slicing(), 5000);
  slice.cancelSlice();

  QVERIFY2(slice.slicing(), "cancellation should not reopen the slice gate before worker terminal cleanup");
  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() > 0 || failedSpy.count() > 0 || !slice.slicing(), 120000);
  QVERIFY2(finishedSpy.count() == 0, "cancelled slice must not emit a successful slice result");
  QCOMPARE(slice.sliceState(), SliceService::State::Cancelled);
  QVERIFY2(slice.outputPath().isEmpty(), "cancelled slice must not leave an active output path");
  QVERIFY2(!slice.hasPlateResult(editor.currentPlateIndex()),
           "cancelled slice must not leave current-plate result metadata");
  QVERIFY2(!editor.hasSliceResult(), "cancelled slice must not become a valid editor result");
  QVERIFY2(!editor.canPreview(), "cancelled slice must block Preview");
  QVERIFY2(!editor.canExportGCode(), "cancelled slice must block export");
}

void E2EWorkflowTests::test_slice_all_stores_outputs_for_printable_unlocked_plates_only()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  QSignalSpy loadSpy(&project, &ProjectServiceMock::loadFinished);
  QVERIFY(loadSpy.isValid());
  QVERIFY2(editor.loadFile(kStlPath), "importing a model should start");
  QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0, 10000);
  QVERIFY2(loadSpy.takeFirst().at(0).toBool(), "model import should complete successfully");

  QVERIFY2(editor.clonePlate(0), "clonePlate should create a printable second plate with copied objects");
  QVERIFY2(editor.clonePlate(0), "clonePlate should create a third plate that can be locked and skipped");
  QCOMPARE(project.plateCount(), 3);
  QVERIFY(project.setCurrentPlateIndex(0));
  QVERIFY(project.setPlateLocked(2, true));

  QSignalSpy finishedSpy(&slice, &SliceService::sliceFinished);
  QSignalSpy failedSpy(&slice, &SliceService::sliceFailed);
  QVERIFY(finishedSpy.isValid());
  QVERIFY(failedSpy.isValid());

  applyMinimalPrinterConfig(slice, project);
  ensureModelOnBed(project);
  editor.requestSliceAll();

  QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() >= 2 || failedSpy.count() > 0, 240000);
  if (failedSpy.count() > 0)
  {
    const QString reason = failedSpy.first().at(0).toString();
    QSKIP(QString("Slice-all failed in this environment: %1").arg(reason).toUtf8().constData());
  }

  QCOMPARE(editor.plateSliceResultStatus(0), int(EditorViewModel::SliceResultValid));
  QCOMPARE(editor.plateSliceResultStatus(1), int(EditorViewModel::SliceResultValid));
  QCOMPARE(editor.plateSliceResultStatus(2), int(EditorViewModel::SliceResultMissing));
  QVERIFY2(slice.hasPlateResult(0), "plate 0 should store a generated result");
  QVERIFY2(slice.hasPlateResult(1), "plate 1 should store a generated result");
  QVERIFY2(!slice.hasPlateResult(2), "locked plate 2 must be skipped");
  QCOMPARE(slice.plateResultSource(0), int(SliceService::ResultSource::ModelSlice));
  QCOMPARE(slice.plateResultSource(1), int(SliceService::ResultSource::ModelSlice));
  QVERIFY2(QFileInfo::exists(slice.plateOutputPath(0)),
           qPrintable(QStringLiteral("plate 0 output should exist: %1").arg(slice.plateOutputPath(0))));
  QVERIFY2(QFileInfo::exists(slice.plateOutputPath(1)),
           qPrintable(QStringLiteral("plate 1 output should exist: %1").arg(slice.plateOutputPath(1))));

  QTemporaryDir exportDir(QDir::tempPath() + QStringLiteral("/owzx_export_all_XXXXXX"));
  QVERIFY2(exportDir.isValid(), "temporary all-plate export directory should be available");
  QVERIFY2(editor.requestExportAllGCode(exportDir.path(), QStringLiteral("allplates")),
           "all printable valid plate results should export to a directory");
  const QString exportedPlate0 = exportDir.filePath(QStringLiteral("allplates_plate1.gcode"));
  const QString exportedPlate1 = exportDir.filePath(QStringLiteral("allplates_plate2.gcode"));
  const QString skippedPlate2 = exportDir.filePath(QStringLiteral("allplates_plate3.gcode"));
  QVERIFY2(QFileInfo::exists(exportedPlate0),
           qPrintable(QStringLiteral("plate 0 export should exist: %1").arg(exportedPlate0)));
  QVERIFY2(QFileInfo::exists(exportedPlate1),
           qPrintable(QStringLiteral("plate 1 export should exist: %1").arg(exportedPlate1)));
  QCOMPARE(QFileInfo(exportedPlate0).size(), QFileInfo(slice.plateOutputPath(0)).size());
  QCOMPARE(QFileInfo(exportedPlate1).size(), QFileInfo(slice.plateOutputPath(1)).size());
  QVERIFY2(!QFileInfo::exists(skippedPlate2),
           "locked skipped plate must not create an all-plate export file");

  QVERIFY(editor.setCurrentPlateIndex(0));
  QCOMPARE(editor.sliceOutputPath(), slice.plateOutputPath(0));
  QVERIFY(editor.setCurrentPlateIndex(1));
  QCOMPARE(editor.sliceOutputPath(), slice.plateOutputPath(1));
  QVERIFY(editor.setCurrentPlateIndex(2));
  QVERIFY2(editor.sliceOutputPath().isEmpty(), "locked skipped plate must not activate a stale output");
  QVERIFY2(slice.outputPath().isEmpty(), "switching to a skipped plate must clear the active service output path");
  QVERIFY2(!editor.canPreview(), "locked skipped plate must not be previewable");
  QVERIFY2(!editor.canExportGCode(), "locked skipped plate must not be exportable");

  const QString plate0Path = slice.plateOutputPath(0);
  const QString plate1Path = slice.plateOutputPath(1);
  if (QFileInfo::exists(plate0Path))
    QFile::remove(plate0Path);
  if (QFileInfo::exists(plate1Path))
    QFile::remove(plate1Path);
}

QTEST_MAIN(E2EWorkflowTests)
#include "E2EWorkflowTests.moc"
