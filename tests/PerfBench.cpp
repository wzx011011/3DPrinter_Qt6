// PerfBench — stage-level headless benchmark for the OWzx perf evaluation
// (docs/perf-baseline.md, scripts/perf/). Runs the REAL service pipeline
// (ProjectServiceMock -> meshData -> PrepareSceneData -> ObjectPicking,
// SliceService, PreviewViewModel gcode parse) against the synthetic
// benchmark models and reports per-stage wall time + process memory.
//
// Not part of the pass/fail verify gate: invoke via
//   scripts/perf/run_stage_bench.ps1
// which sets OWZX_PERF_LOG=1, or run the exe directly with
//   OWZX_PERF_LOG=1 PERF_BENCH_MODELS=... ./PerfBench.exe
//
// AUTOMOC caveat (see ViewModelSmokeTests CMake comment): single-file QtTest
// with cpp-internal Q_OBJECT — re-run cmake configure after adding slots.
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>
#include <QtTest>

#include <cmath>

#include "core/rendering/ObjectPicking.h"
#include "core/services/PresetServiceMock.h"
#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
#include "core/util/PerfLog.h"
#include "core/viewmodels/PreviewViewModel.h"
#include "qml_gui/Renderer/PrepareSceneData.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

namespace
{

double processMemoryMiB()
{
#ifdef Q_OS_WIN
  PROCESS_MEMORY_COUNTERS counters{};
  if (GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters)))
    return double(counters.WorkingSetSize) / 1048576.0;
#endif
  return -1.0;
}

int batchCountFromMeshData(const QByteArray &meshData)
{
  // Real meshData layout (ProjectServiceMock::meshData): int32 batchCount,
  // then per batch { int32 objectId, int32 triCount, triCount*9 floats },
  // then a 6-float scene bbox trailer.
  if (meshData.size() < static_cast<qsizetype>(sizeof(int32_t)))
    return 0;
  int count = 0;
  std::memcpy(&count, meshData.constData(), sizeof(count));
  return count;
}

} // namespace

class PerfBench final : public QObject
{
  Q_OBJECT

private slots:
  void stages();

private:
  void benchModel(const QString &modelPath, bool includeSlice,
                  QJsonObject *reportOut);
  QJsonObject m_root;
};

void PerfBench::benchModel(const QString &modelPath, bool includeSlice,
                           QJsonObject *reportOut)
{
  QJsonObject modelReport;
  const QString modelName = QFileInfo(modelPath).fileName();
  qInfo("[BENCH] ==== %s ====", qPrintable(modelName));

  // ── Stage 1: model load (real import path) ─────────────────────────────
  ProjectServiceMock projectService;
  QSignalSpy loadSpy(&projectService, &ProjectServiceMock::loadFinished);
  QElapsedTimer timer;
  timer.start();
  QVERIFY2(projectService.loadFile(modelPath), "loadFile failed");
  {
    QEventLoop loop;
    QTimer::singleShot(120000, &loop, &QEventLoop::quit);
    QObject::connect(&projectService, &ProjectServiceMock::loadFinished,
                     &loop, &QEventLoop::quit);
    if (loadSpy.isEmpty())
      loop.exec();
  }
  const qint64 loadMs = timer.elapsed();
  const double memAfterLoad = processMemoryMiB();
  modelReport[QStringLiteral("load_ms")] = double(loadMs);
  modelReport[QStringLiteral("mem_after_load_mib")] = memAfterLoad;
  qInfo("[BENCH] %-24s %8.1f ms | mem=%8.1f MiB", "load", double(loadMs),
        memAfterLoad);

  // ── Stage 2: meshData TLV blob build (de-indexed, transform baked) ─────
  timer.start();
  PerfLog::memory("before meshData");
  const QByteArray meshData = projectService.meshData();
  const qint64 meshDataMs = timer.elapsed();
  const double memAfterMesh = processMemoryMiB();
  modelReport[QStringLiteral("meshdata_ms")] = double(meshDataMs);
  modelReport[QStringLiteral("meshdata_bytes")] = double(meshData.size());
  modelReport[QStringLiteral("mem_after_meshdata_mib")] = memAfterMesh;
  qInfo("[BENCH] %-24s %8.1f ms | blob=%6.1f MiB mem=%8.1f MiB", "meshData",
        double(meshDataMs), meshData.size() / 1048576.0, memAfterMesh);

  // ── Stage 3: PrepareSceneData CPU expansion (per-page scene copy) ──────
  PrepareSceneData scene;
  const int batchCount = batchCountFromMeshData(meshData);
  QList<int> sourceIndices;
  for (int i = 0; i < batchCount; ++i)
    sourceIndices.append(i);
  timer.start();
  scene.setModelMeshData(meshData, sourceIndices, sourceIndices);
  const qint64 expandMs = timer.elapsed();
  const double memAfterExpand = processMemoryMiB();
  modelReport[QStringLiteral("scene_expand_ms")] = double(expandMs);
  modelReport[QStringLiteral("scene_vertex_count")] =
      double(scene.modelVertices().size());
  modelReport[QStringLiteral("mem_after_expand_mib")] = memAfterExpand;
  qInfo("[BENCH] %-24s %8.1f ms | verts=%d mem=%8.1f MiB", "scene_expand",
        double(expandMs), scene.modelVertices().size(), memAfterExpand);

  // ── Stage 4: object picking sweep (hover-cost proxy) ───────────────────
  // Fan 32 rays at the scene bbox center (from the batch bounds union) at a
  // typical camera distance through the production ObjectPicking::pick path
  // (ray -> AABB prefilter -> Moller-Trumbore).
  {
    float cminX = 1e30f, cminY = 1e30f, cminZ = 1e30f;
    float cmaxX = -1e30f, cmaxY = -1e30f, cmaxZ = -1e30f;
    for (const auto &batch : scene.modelBatches()) {
      cminX = std::min(cminX, batch.bounds.minX);
      cminY = std::min(cminY, batch.bounds.minY);
      cminZ = std::min(cminZ, batch.bounds.minZ);
      cmaxX = std::max(cmaxX, batch.bounds.maxX);
      cmaxY = std::max(cmaxY, batch.bounds.maxY);
      cmaxZ = std::max(cmaxZ, batch.bounds.maxZ);
    }
    const QVector3D center((cminX + cmaxX) * 0.5f, (cminY + cmaxY) * 0.5f,
                           (cminZ + cmaxZ) * 0.5f);

    const int kRayCount = 32;
    int hits = 0;
    QVector<double> rayMs;
    timer.start();
    for (int i = 0; i < kRayCount; ++i) {
      QElapsedTimer rayTimer;
      rayTimer.start();
      const float angle = float(i) * (2.0f * float(M_PI) / kRayCount);
      const QVector3D origin(center.x() + 200.0f * std::cos(angle),
                             center.y() + 150.0f,
                             center.z() + 200.0f * std::sin(angle));
      const QVector3D direction = (center - origin).normalized();
      const ObjectPicking::Hit hit =
          ObjectPicking::pick(origin, direction, scene.modelVertices(),
                              scene.modelBatches());
      rayMs.append(double(rayTimer.nsecsElapsed()) / 1e6);
      if (hit.isValid())
        ++hits;
    }
    const qint64 sweepMs = timer.elapsed();
    std::sort(rayMs.begin(), rayMs.end());
    const double p50 = rayMs.isEmpty() ? -1.0 : rayMs[rayMs.size() / 2];
    const double p95 = rayMs.isEmpty()
                           ? -1.0
                           : rayMs[qMin(rayMs.size() - 1, rayMs.size() * 95 / 100)];
    modelReport[QStringLiteral("pick_sweep_ms")] = double(sweepMs);
    modelReport[QStringLiteral("pick_ray_p50_ms")] = p50;
    modelReport[QStringLiteral("pick_ray_p95_ms")] = p95;
    modelReport[QStringLiteral("pick_hits")] = hits;
    qInfo("[BENCH] %-24s %8.1f ms | p50=%6.3f p95=%6.3f hits=%d/%d",
          "pick_sweep", double(sweepMs), p50, p95, hits, kRayCount);
  }

  // ── Stage 5: slice + preview parse (400k tier only by default) ─────────
  if (includeSlice) {
    PresetServiceMock presetService;
    SliceService sliceService(&projectService);
    QHash<QString, QVariant> merged;
    // Same fairness pins as scripts/perf/run_owzx_bench.ps1: layer 0.2 with
    // auto line widths, matching the Orca default profile.
    merged.insert(QStringLiteral("layer_height"), 0.2);
    merged.insert(QStringLiteral("line_width"), 0);
    merged.insert(QStringLiteral("initial_layer_line_width"), 0);
    merged.insert(QStringLiteral("outer_wall_line_width"), 0);
    merged.insert(QStringLiteral("inner_wall_line_width"), 0);
    merged.insert(QStringLiteral("sparse_infill_line_width"), 0);
    merged.insert(QStringLiteral("top_surface_line_width"), 0);
    merged.insert(QStringLiteral("support_line_width"), 0);
    sliceService.setMergedPresetConfig(merged);

    timer.start();
    QSignalSpy finishedSpy(&sliceService, &SliceService::sliceFinished);
    QSignalSpy failedSpy(&sliceService, &SliceService::sliceFailed);
    sliceService.startSlice(projectService.projectName());
    {
      QEventLoop loop;
      QTimer::singleShot(300000, &loop, &QEventLoop::quit);
      QObject::connect(&sliceService, &SliceService::sliceFinished, &loop,
                       &QEventLoop::quit);
      QObject::connect(&sliceService, &SliceService::sliceFailed, &loop,
                       &QEventLoop::quit);
      if (finishedSpy.isEmpty() && failedSpy.isEmpty())
        loop.exec();
    }
    const qint64 sliceMs = timer.elapsed();
    const double memAfterSlice = processMemoryMiB();
    modelReport[QStringLiteral("slice_ms")] = double(sliceMs);
    modelReport[QStringLiteral("mem_after_slice_mib")] = memAfterSlice;
    QVERIFY2(!failedSpy.isEmpty() || !finishedSpy.isEmpty(), "slice did not finish");
    qInfo("[BENCH] %-24s %8.1f ms | mem=%8.1f MiB", "slice_total",
          double(sliceMs), memAfterSlice);

    // Preview parse stage: the GUI-thread gcode re-parse after each slice.
    const QString gcodePath = sliceService.outputPath();
    if (!gcodePath.isEmpty() && QFileInfo::exists(gcodePath)) {
      PreviewViewModel preview(&projectService, &sliceService);
      timer.start();
      const bool loaded = preview.loadGCodeForPreview(gcodePath);
      const qint64 parseMs = timer.elapsed();
      const double memAfterParse = processMemoryMiB();
      modelReport[QStringLiteral("preview_parse_ms")] = double(parseMs);
      modelReport[QStringLiteral("mem_after_preview_mib")] = memAfterParse;
      qInfo("[BENCH] %-24s %8.1f ms | ok=%d mem=%8.1f MiB", "preview_parse",
            double(parseMs), int(loaded), memAfterParse);
    }
  }

  *reportOut = modelReport;
}

void PerfBench::stages()
{
  const QString modelsDir = qEnvironmentVariable("PERF_BENCH_MODELS_DIR",
                                                 QStringLiteral("build/perf_models"));
  const QString tiers = qEnvironmentVariable("PERF_BENCH_TIERS",
                                              QStringLiteral("400k,2m"));
  const bool includeSlice = qEnvironmentVariable("PERF_BENCH_SLICE", "1") != "0";

  struct Tier
  {
    QString name;
    QString file;
    bool slice;
  };
  const QList<Tier> available = {
      {QStringLiteral("400k"), QStringLiteral("bench_sphere_400k.stl"), true},
      {QStringLiteral("2m"), QStringLiteral("bench_sphere_2m.stl"), false},
      {QStringLiteral("5m"), QStringLiteral("bench_sphere_5m.stl"), false},
      {QStringLiteral("grid"), QStringLiteral("bench_grid_20x100k.stl"), false},
  };

  QJsonObject models;
  bool ranAny = false;
  for (const Tier &tier : available) {
    if (!tiers.contains(tier.name, Qt::CaseInsensitive))
      continue;
    const QString path = QDir::cleanPath(modelsDir + QLatin1Char('/') + tier.file);
    if (!QFileInfo::exists(path)) {
      qInfo("[BENCH] SKIP %s (missing %s)", qPrintable(tier.name),
            qPrintable(path));
      continue;
    }
    QJsonObject modelReport;
    benchModel(path, includeSlice && tier.slice, &modelReport);
    models[tier.name] = modelReport;
    ranAny = true;
  }
  QVERIFY2(ranAny, "no benchmark models found — run "
                   "python scripts/perf/gen_benchmark_models.py first");

  m_root[QStringLiteral("models")] = models;

  // Machine-readable report next to the CLI bench outputs.
  const QString outPath = qEnvironmentVariable(
      "PERF_BENCH_REPORT", QStringLiteral("build/perf_out/stage_report.json"));
  QDir().mkpath(QFileInfo(outPath).absolutePath());
  QFile out(outPath);
  if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    out.write(QJsonDocument(m_root).toJson(QJsonDocument::Indented));
    out.close();
    qInfo("[BENCH] report written: %s", qPrintable(outPath));
  }
}

QTEST_MAIN(PerfBench)
#include "PerfBench.moc"
