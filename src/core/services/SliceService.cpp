#include "SliceService.h"

#include "core/services/ProjectServiceMock.h"
#include "core/services/AppSettingsService.h"

#include <QMetaObject>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QStringList>
#include <QFileInfo>
#include <QDateTime>
#include <QFile>
#include <QThread>
#include <QSaveFile>
#include <QUrl>
#include <stdexcept>

#ifdef HAS_LIBSLIC3R
#include <libslic3r/Model.hpp>
#include <libslic3r/Print.hpp>
#include <libslic3r/PrintConfig.hpp>
#include <libslic3r/GCode/GCodeProcessor.hpp>
#include <libslic3r/Utils.hpp>
#endif

namespace
{
  QString formatDurationLabel(double seconds)
  {
    const qint64 totalSeconds = std::max<qint64>(0, qRound64(seconds));
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 secs = totalSeconds % 60;
    return QStringLiteral("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
  }

  QString localPathFromDialogValue(const QString &pathOrUrl)
  {
    const QUrl url(pathOrUrl);
    return url.isLocalFile() ? url.toLocalFile() : pathOrUrl;
  }

  QString comparablePath(const QFileInfo &info)
  {
    const QString path = info.exists() ? info.canonicalFilePath() : info.absoluteFilePath();
#ifdef Q_OS_WIN
    return QDir::cleanPath(path).toCaseFolded();
#else
    return QDir::cleanPath(path);
#endif
  }

  QString sanitizeFileStem(const QString &stem)
  {
    QString out = stem.trimmed();
    if (out.isEmpty())
      out = QStringLiteral("output");

    static const QString invalidChars = QStringLiteral("<>:\"/\\|?*");
    for (QChar &ch : out)
    {
      if (invalidChars.contains(ch) || ch.unicode() < 0x20)
        ch = QChar('_');
    }

    while (out.contains(QStringLiteral("__")))
      out.replace(QStringLiteral("__"), QStringLiteral("_"));
    out = out.trimmed();
    while (out.endsWith('.') || out.endsWith(' '))
      out.chop(1);
    return out.isEmpty() ? QStringLiteral("output") : out;
  }

  QString ensureGcodeSuffix(QString fileName)
  {
    if (!fileName.endsWith(QStringLiteral(".gcode"), Qt::CaseInsensitive))
      fileName += QStringLiteral(".gcode");
    return fileName;
  }

  void logExportFailure(const QString &operation,
                        const QString &sourcePath,
                        const QString &targetPath,
                        const QString &reason)
  {
    qWarning("[SliceService] export failed op=%s source=%s target=%s reason=%s",
             operation.toUtf8().constData(),
             sourcePath.toUtf8().constData(),
             targetPath.toUtf8().constData(),
             reason.toUtf8().constData());
  }
}

SliceService::SliceService(ProjectServiceMock *projectService, QObject *parent)
    : QObject(parent), projectService_(projectService)
{
}

int SliceService::progress() const
{
  return progress_;
}

bool SliceService::slicing() const
{
  return slicing_;
}

QString SliceService::statusLabel() const
{
  return statusLabel_;
}

QString SliceService::outputPath() const
{
  return outputPath_;
}

QString SliceService::estimatedTimeLabel() const
{
  return estimatedTimeLabel_;
}

QString SliceService::resultWeightLabel() const
{
  return resultWeightLabel_;
}

QString SliceService::resultPlateLabel() const
{
  return resultPlateLabel_;
}

int SliceService::resultPlateIndex() const
{
  return resultPlateIndex_;
}

QString SliceService::resultFilamentLabel() const
{
  return resultFilamentLabel_;
}

int SliceService::resultLayerCount() const
{
  return resultLayerCount_;
}

QString SliceService::resultCostLabel() const
{
  return resultCostLabel_;
}

double SliceService::resultTotalFilamentMm() const
{
  // Parse total filament length from resultFilamentLabel_, for example "3.45 m".
  if (resultFilamentLabel_.isEmpty()) return 0.0;
  bool ok = false;
  const double val = resultFilamentLabel_.left(resultFilamentLabel_.indexOf(' ')).toDouble(&ok);
  return ok ? val * 1000.0 : 0.0; // m to mm
}

void SliceService::clearStoredResult()
{
  progress_ = 0;
  sliceState_ = State::Idle;
  statusLabel_ = QStringLiteral("Waiting to slice");
  outputPath_.clear();
  estimatedTimeLabel_.clear();
  resultWeightLabel_.clear();
  resultPlateLabel_.clear();
  resultPlateIndex_ = -1;
  resultFilamentLabel_.clear();
  resultLayerCount_ = 0;
  resultCostLabel_.clear();
  emit progressChanged();
}

void SliceService::clearActiveTargetResult()
{
  if (activeTargetPlateIndex_ >= 0)
    plateResults_.remove(activeTargetPlateIndex_);
  if (resultPlateIndex_ == activeTargetPlateIndex_)
    clearStoredResult();
}

void SliceService::storePlateResult(int plateIndex, const PlateSliceResult &result)
{
  if (plateIndex >= 0)
    plateResults_[plateIndex] = result;
}

void SliceService::clearResults()
{
  const bool wasSlicing = slicing_;
  if (wasSlicing && activeCancelFlag_)
  {
    activeCancelFlag_->store(true);
#ifdef HAS_LIBSLIC3R
    if (Slic3r::Print *active = activePrint_.load(std::memory_order_acquire))
      active->cancel();
#endif
  }

  clearStoredResult();
  activeTargetPlateIndex_ = -1;
  plateResults_.clear();
  emit resultChanged();
  emit sliceResultCleared();
  emit stateChanged();
}

void SliceService::setMergedPresetConfig(const QHash<QString, QVariant> &config)
{
  mergedPresetConfig_ = config;
}

namespace
{
  /// Type-aware config value injection into DynamicPrintConfig
  /// Skips keys that don't exist in the config schema
  void injectPresetConfig(Slic3r::DynamicPrintConfig &config, const QHash<QString, QVariant> &presetValues)
  {
#ifdef HAS_LIBSLIC3R
    for (auto it = presetValues.constBegin(); it != presetValues.constEnd(); ++it)
    {
      const std::string key = it.key().toStdString();
      Slic3r::ConfigOption *opt = config.option(key, false);
      if (!opt)
        continue;

      const QVariant &value = it.value();
      bool setOk = false;

      switch (static_cast<QMetaType::Type>(value.typeId()))
      {
      case QMetaType::Double:
      {
        auto *floatOpt = dynamic_cast<Slic3r::ConfigOptionFloat *>(opt);
        if (floatOpt)
        {
          floatOpt->value = value.toDouble();
          setOk = true;
          break;
        }
        break;
      }
      case QMetaType::Int:
      {
        auto *intOpt = dynamic_cast<Slic3r::ConfigOptionInt *>(opt);
        if (intOpt)
        {
          intOpt->value = value.toInt();
          setOk = true;
          break;
        }
        auto *boolOpt = dynamic_cast<Slic3r::ConfigOptionBool *>(opt);
        if (boolOpt)
        {
          boolOpt->value = value.toInt() ? 1 : 0;
          setOk = true;
          break;
        }
        break;
      }
      case QMetaType::Bool:
      {
        auto *boolOpt = dynamic_cast<Slic3r::ConfigOptionBool *>(opt);
        if (boolOpt)
        {
          boolOpt->value = value.toBool() ? 1 : 0;
          setOk = true;
          break;
        }
        break;
      }
      case QMetaType::QVariantList:
      {
        // coFloats, coInts, coPoints, coStrings serialize to Slic3r format.
        const auto list = value.toList();
        if (list.isEmpty())
          break;
        QStringList parts;
        for (const auto &item : list)
          parts << item.toString();
        try
        {
          config.set_deserialize_strict(key, parts.join(",").toStdString(), false);
          setOk = true;
        }
        catch (...)
        {
        }
        break;
      }
      default:
        break;
      }

      if (!setOk)
      {
        const std::string strVal = value.toString().toStdString();
        if (!strVal.empty())
        {
          try
          {
            config.set_deserialize_strict(key, strVal, false);
          }
          catch (...)
          {
          }
        }
      }
    }
#endif
  }
} // anonymous namespace

void SliceService::startSlice(const QString &projectName)
{
  Q_UNUSED(projectName);

  if (slicing_)
    return;

  const QString sourcePath = projectService_ ? projectService_->sourceFilePath() : QString{};
  const int targetPlateIndex = projectService_ ? projectService_->currentPlateIndex() : -1;
  activeTargetPlateIndex_ = targetPlateIndex;
  plateResults_.remove(targetPlateIndex);
  clearStoredResult();
  emit resultChanged();
  emit sliceResultCleared();

  QString targetPlateLabel;
#ifdef HAS_LIBSLIC3R
  std::unique_ptr<Slic3r::Model> modelForSlice;
#endif
  if (projectService_)
  {
    const QStringList plateNames = projectService_->plateNames();
    if (targetPlateIndex >= 0 && targetPlateIndex < plateNames.size() && !plateNames[targetPlateIndex].isEmpty())
      targetPlateLabel = plateNames[targetPlateIndex];
    else if (targetPlateIndex >= 0)
      targetPlateLabel = QObject::tr("Plate %1").arg(targetPlateIndex + 1);
#ifdef HAS_LIBSLIC3R
    modelForSlice = projectService_->cloneCurrentPlateModel();
#endif
  }

  if (sourcePath.isEmpty())
  {
    sliceState_ = State::Error;
    statusLabel_ = QStringLiteral("No sliceable model found");
    qWarning("[SliceService] slice failed source=%s plate=%d reason=%s",
             sourcePath.toUtf8().constData(),
             targetPlateIndex,
             statusLabel_.toUtf8().constData());
    emit progressChanged();
    emit stateChanged();
    emit sliceFailed(statusLabel_);
    activeTargetPlateIndex_ = -1;
    return;
  }

#ifdef HAS_LIBSLIC3R
  if (!modelForSlice || modelForSlice->objects.empty())
  {
    sliceState_ = State::Error;
    statusLabel_ = QStringLiteral("Current plate has no sliceable objects");
    qWarning("[SliceService] slice failed source=%s plate=%d reason=%s",
             sourcePath.toUtf8().constData(),
             targetPlateIndex,
             statusLabel_.toUtf8().constData());
    emit progressChanged();
    emit stateChanged();
    emit sliceFailed(statusLabel_);
    activeTargetPlateIndex_ = -1;
    return;
  }
#endif

  slicing_ = true;
  sliceState_ = State::Slicing;
  progress_ = 0;
  statusLabel_ = QStringLiteral("Preparing slice");
  outputPath_.clear();
  activeCancelFlag_ = std::make_shared<std::atomic_bool>(false);
  qInfo("[SliceService] slice start source=%s plate=%d label=%s",
        sourcePath.toUtf8().constData(),
        targetPlateIndex,
        targetPlateLabel.toUtf8().constData());
  emit slicingChanged();
  emit progressChanged();
  emit progressUpdated(progress_, statusLabel_);

  const QPointer<SliceService> receiver(this);
  const auto cancelFlag = activeCancelFlag_;

  QtConcurrent::run([receiver, cancelFlag, sourcePath, targetPlateIndex, targetPlateLabel
#ifdef HAS_LIBSLIC3R
                     , modelForSlice = std::move(modelForSlice)
#endif
                     ]() mutable
                    {
    QString errorText;
    QString outputPath;
    QString estimatedTimeLabel;
    QString resultWeightLabel;
    QString resultFilamentLabel;
    QString resultCostLabel;
    QString resultPlateLabel;
    int resultPlateIndex = -1;
    int layerCount = 0;
    // Phase 100 (WTREAD-01): captured-by-value wipe-tower geometry. Stays
    // default-constructed (valid=false) in mock mode and on any error/cancel
    // path; only the HAS_LIBSLIC3R success branch populates it. Captured by
    // value into the GUI-thread delivery lambda below so no Print* escapes
    // the worker (Frozen Decision 1).
    WipeTowerGeometry capturedGeometry{};
    // Phase 108 (FMAP-01): captured-by-value filament-map auto-recommendation.
    // Same invariant as capturedGeometry: default-constructed (valid=false) in
    // mock mode and on any error/cancel path; only the HAS_LIBSLIC3R success
    // branch populates it. Captured by value into the GUI-thread delivery
    // lambda below so no Print* escapes the worker (Frozen Decision 1).
    FilamentMapResult capturedFilamentMap{};

#ifdef HAS_LIBSLIC3R
    try
    {
      auto notify = [receiver](int progress, const QString &label) {
        if (!receiver)
          return;
        QMetaObject::invokeMethod(receiver, [receiver, progress, label]() {
          if (!receiver)
            return;
          receiver->progress_ = progress;
          receiver->statusLabel_ = label;
          emit receiver->progressChanged();
          emit receiver->progressUpdated(progress, label);
        }, Qt::QueuedConnection);
      };

      notify(2, QObject::tr("Preparing current plate model"));
      if (!modelForSlice || modelForSlice->objects.empty())
        throw std::runtime_error("Current plate has no sliceable objects");
      {
        int totalVolumes = 0;
        int totalInstances = 0;
        for (const auto *obj : modelForSlice->objects) {
          if (obj) {
            totalVolumes += int(obj->volumes.size());
            totalInstances += int(obj->instances.size());
          }
        }
        if (totalVolumes == 0)
          throw std::runtime_error("cloned model has 0 volumes (mesh data missing)");
      }
      if (cancelFlag && cancelFlag->load())
        throw std::runtime_error("Slicing cancelled");

      notify(10, QObject::tr("Preparing slice parameters"));
      Slic3r::DynamicPrintConfig config = Slic3r::DynamicPrintConfig::full_print_config();
      // v2.7 P0: bed_shape injection (mirror CLI CliRunner.cpp:397-399)
      // v2.8 W3: use persisted bed size when bedShape_ was not set explicitly.
      QVector<QPointF> bedPoints = receiver->bedShape_;
      if (bedPoints.isEmpty() && receiver->appSettings_) {
        const QSizeF bedSize = receiver->appSettings_->bedSize();
        bedPoints = {
          QPointF(0, 0),
          QPointF(bedSize.width(), 0),
          QPointF(bedSize.width(), bedSize.height()),
          QPointF(0, bedSize.height())
        };
      }
      if (!bedPoints.isEmpty())
      {
        auto *bedPts = new Slic3r::ConfigOptionPoints();
        for (const QPointF &p : bedPoints)
          bedPts->values.emplace_back(
              static_cast<coord_t>(p.x() * 1000.0),
              static_cast<coord_t>(p.y() * 1000.0));
        config.set_key_value("bed_shape", bedPts);
      }

      // Inject user-selected preset values into config, aligned with PresetBundle::full_fff_config.
      // This overwrites factory defaults with the 3-tier merged hierarchy.
      if (receiver && !receiver->mergedPresetConfig_.isEmpty())
      {
        injectPresetConfig(config, receiver->mergedPresetConfig_);
        receiver->mergedPresetConfig_.clear(); // one-shot injection per slice
      }

      // v3.0 Phase 19 (D-15): merge the FULL per-plate config (not just 3 hardcoded
      // keys), aligning with upstream update_slice_context_to_current_plate.
      // DynamicPrintConfig::apply(other) merges other's keys into this with other
      // winning — so plate overrides take precedence over the preset config.
      // This honors ALL per-plate overrides (filament_maps, print_compatible_*,
      // layer sequences, curr_bed_type, print_sequence, spiral_mode, etc.), fixing
      // the gap-analysis issue where arbitrary per-plate overrides were dropped.
      if (receiver && receiver->projectService_) {
        const int plateIdx = targetPlateIndex >= 0 ? targetPlateIndex
                             : receiver->projectService_->currentPlateIndex();
        if (plateIdx >= 0) {
          if (const Slic3r::DynamicPrintConfig *plateCfg =
                  receiver->projectService_->plateDynamicConfig(plateIdx)) {
            if (!plateCfg->empty()) {
              config.apply(*plateCfg);
            }
          }
        }
      }

      Slic3r::Print print;
      receiver->activePrint_.store(&print, std::memory_order_release);

      print.set_status_callback([receiver](const Slic3r::PrintBase::SlicingStatus &st) {
        if (!receiver)
          return;
        const int p = qBound(0, st.percent, 100);
        const QString label = st.text.empty() ? QObject::tr("Slicing") : QString::fromStdString(st.text);
        QMetaObject::invokeMethod(receiver, [receiver, p, label]() {
          if (!receiver)
            return;
          if (p >= 0)
            receiver->progress_ = p;
          receiver->statusLabel_ = label;
          emit receiver->progressChanged();
          emit receiver->progressUpdated(receiver->progress_, label);
        }, Qt::QueuedConnection);
      });

      notify(18, QObject::tr("Applying slice parameters"));

      // Set up directories for Print::apply()
      {
        const QString tempDir = QDir::tempPath();
        Slic3r::set_temporary_dir(tempDir.toStdString());
        Slic3r::set_data_dir(QCoreApplication::applicationDirPath().toStdString());
        Slic3r::set_resources_dir((QCoreApplication::applicationDirPath() + "/resources").toStdString());
      }

      config.normalize_fdm();

      // For Marlin with relative E distances, layer_gcode must reset extruder position
      // (upstream Print::validate enforces this for non-BBL printers)
      {
        auto *useRel = config.option<Slic3r::ConfigOptionBool>("use_relative_e_distances", false);
        auto *gcodeFlavor = config.option<Slic3r::ConfigOptionEnum<Slic3r::GCodeFlavor>>("gcode_flavor", false);
        auto *layerGcode = config.option<Slic3r::ConfigOptionString>("layer_change_gcode", false);
        if (useRel && useRel->value && gcodeFlavor &&
            (gcodeFlavor->value == Slic3r::gcfMarlinLegacy || gcodeFlavor->value == Slic3r::gcfMarlinFirmware) &&
            layerGcode && layerGcode->value.find("G92 E0") == std::string::npos)
        {
          layerGcode->value = "G92 E0\n" + layerGcode->value;
        }
      }

      print.apply(*modelForSlice, config);

      // v2.7 P1: inject calibration parameters after apply() and before process().
      // GCode::do_export then enters Calib_PA_Line / Calib_Flow_Rate / Calib_Temp_Tower.

      if (receiver && receiver->calibConfig_.mode != 0)
      {
        Slic3r::Calib_Params cp;
        cp.mode = static_cast<Slic3r::CalibMode>(receiver->calibConfig_.mode);
        cp.start = receiver->calibConfig_.start;
        cp.end = receiver->calibConfig_.end;
        cp.step = receiver->calibConfig_.step;
        cp.print_numbers = receiver->calibConfig_.printNumbers;
        cp.test_model = receiver->calibConfig_.testModel;
        print.set_calib_params(cp);
      }


      if (cancelFlag && cancelFlag->load())
      {
        print.cancel();
        throw std::runtime_error("Slicing cancelled");
      }

      // Pre-slice validation (align upstream BackgroundSlicingProcess::validate)
      {
        Slic3r::StringObjectException validationError = print.validate();
        if (!validationError.string.empty())
          throw std::runtime_error("Slice validation failed: " + validationError.string);
      }

      notify(25, QObject::tr("Running slice"));
      print.process();
      if (cancelFlag && cancelFlag->load())
      {
        print.cancel();
        throw std::runtime_error("Slicing cancelled");
      }

      // v2.8 review W2: reset calibConfig (avoid sticking after calibration slice).
      if (receiver) receiver->calibConfig_.mode = 0;


      Slic3r::GCodeProcessorResult result;
      const QString appDir = QCoreApplication::applicationDirPath();
      const QString baseName = QFileInfo(sourcePath).completeBaseName();
      const QString fileName = QStringLiteral("%1_%2.gcode")
                                   .arg(baseName)
                                   .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
      const QString targetPath = QDir(appDir).filePath(fileName);
      const std::string generated = print.export_gcode(targetPath.toStdString(), &result);
      outputPath = QString::fromStdString(generated);
      estimatedTimeLabel = formatDurationLabel(result.print_statistics.modes[0].time);
      const auto &printStats = print.print_statistics();
      if (printStats.total_weight > 0.0)
        resultWeightLabel = QStringLiteral("%1 g").arg(QString::number(printStats.total_weight, 'f', 2));

      // Filament length
      if (printStats.total_used_filament > 0.0)
        resultFilamentLabel = QStringLiteral("%1 m").arg(QString::number(printStats.total_used_filament / 1000.0, 'f', 2));

      // Layer count (captured for main-thread delivery)
      layerCount = 0;
      for (const Slic3r::PrintObject *obj : print.objects())
          layerCount += static_cast<int>(obj->total_layer_count());

      // Cost
      if (printStats.total_cost > 0.0)
        resultCostLabel = QStringLiteral("$%1").arg(QString::number(printStats.total_cost, 'f', 2));

      // Phase 100 (WTREAD-01): capture real wipe-tower geometry BY VALUE before
      // the Print is invalidated (activePrint_.store(nullptr) below). No Print*
      // or WipeTowerData* may escape the worker (Frozen Decision 1). The
      // width/position derivation matches upstream Print.cpp:2871-2873
      // (bbx span + rib_offset). The Qt renderer uses X/Z as the bed plane
      // (upstream Y maps to Qt Z). has_wipe_tower() (Print.hpp:988) is the gate:
      // when false, capturedGeometry.valid stays false (WTREAD-02).
      //
      // Phase 100 REVIEW W1: upstream pt0 = bbx.min + rib_offset is the tower
      // CORNER, but the Qt consumer GizmoGeometry::buildWipeTowerVertices
      // (GizmoGeometry.cpp:465-469) treats its x/z args as a box CENTER
      // (uses x - hw / x + hw). Convert corner -> center here so the rendered
      // box sits on the true sliced position (otherwise it is offset by
      // +width/2, +depth/2).
      if (print.has_wipe_tower())
      {
        const Slic3r::BoundingBoxf bbx = print.get_wipe_tower_bbx();
        const float depth = print.get_wipe_tower_depth();
        const Slic3r::Vec2f ribOffset = print.get_rib_offset();
        const Slic3r::WipeTowerData &wtData = print.wipe_tower_data();
        capturedGeometry.valid = true;
        capturedGeometry.width = float(bbx.max.x() - bbx.min.x());
        capturedGeometry.depth = depth;
        capturedGeometry.height = wtData.height;
        capturedGeometry.brimWidth = wtData.brim_width;
        // Corner -> center: add half the bed-plane extents so the consumer
        // (which expects a center) renders the box at the true position.
        capturedGeometry.x = float(bbx.min.x() + ribOffset.x()) + capturedGeometry.width * 0.5f;
        capturedGeometry.z = float(bbx.min.y() + ribOffset.y()) + capturedGeometry.depth * 0.5f;
        capturedGeometry.ribOffsetX = ribOffset.x();
        capturedGeometry.ribOffsetY = ribOffset.y();

        // Phase 109 (WTMESH-01/02): capture the real wipe-tower mesh BY VALUE
        // when the engine populated wipe_tower_mesh_data (Print.hpp:766). This
        // RE-OPENS Phase 99 Frozen Decision 2 (Option B was LOCKED future
        // upgrade). The Option A dim fields above stay populated so the Option
        // A fallback still works when hasRealMesh is false.
        //
        // Mirrors upstream load_real_wipe_tower_preview (3DScene.cpp:906-914):
        // start from real_wipe_tower_mesh, merge real_brim_mesh when present,
        // run convex_hull_3d() to get the silhouette shell, extract its.vertices
        // as flattened XYZ floats. NO TriangleMesh* or its* escapes the worker
        // -- meshVertices is a pure float vector (Frozen Decision 1 extended).
        // wipe_tower_mesh_data->clear() resets it to nullopt (Print.hpp:776), so
        // single-material / pre-slice / mock paths leave hasRealMesh=false and
        // the renderer takes the Option A dimensioned-box path.
        if (wtData.wipe_tower_mesh_data != std::nullopt)
        {
          const Slic3r::WipeTowerData::WipeTowerMeshData &meshData =
              *wtData.wipe_tower_mesh_data;
          Slic3r::TriangleMesh mergedMesh = meshData.real_wipe_tower_mesh;
          // Upstream 3DScene.cpp:907-909 conditionally merges the brim mesh.
          // We mirror that: merge when the brim mesh carries geometry. The
          // engine populates real_brim_mesh alongside real_wipe_tower_mesh when
          // a brim is present, so the vertex count check is the safe gate.
          if (!meshData.real_brim_mesh.its.vertices.empty())
            mergedMesh.merge(meshData.real_brim_mesh);
          if (!mergedMesh.its.vertices.empty())
          {
            const Slic3r::TriangleMesh shell = mergedMesh.convex_hull_3d();
            // Phase 109 REVIEW CRITICAL-1: shell.its.vertices is a DEDUPLICATED
            // POINT CLOUD, not a triangle soup (its_convex_hull at
            // TriangleMesh.cpp:1342-1424 builds a separate its.indices facet
            // list). Upstream renders the hull via init_from(shell) which uses
            // BOTH. Expand the indices into a flattened XYZ triangle soup here
            // so downstream consumers (buildWipeTowerMeshVertices + SoftwareViewport
            // mirror) receive the triangle payload they expect, not a point cloud
            // that would render as garbage GL_TRIANGLES.
            const auto &sv = shell.its.vertices;
            const auto &si = shell.its.indices;
            if (!sv.empty() && !si.empty())
            {
              capturedGeometry.meshVertices.reserve(si.size() * 9);
              for (const Slic3r::Vec3i32 &f : si)
              {
                for (int k = 0; k < 3; ++k)
                {
                  const Slic3r::Vec3f &v = sv[f[k]];
                  capturedGeometry.meshVertices.push_back(v.x());
                  capturedGeometry.meshVertices.push_back(v.y());
                  capturedGeometry.meshVertices.push_back(v.z());
                }
              }
              capturedGeometry.hasRealMesh = true;
            }
          }
        }
      }

      resultPlateLabel = targetPlateLabel;
      resultPlateIndex = targetPlateIndex;

      // Phase 108 (FMAP-01): capture the filament-map auto-recommendation BY
      // VALUE before the Print is invalidated (activePrint_.store(nullptr)
      // below). Mirrors the WipeTowerGeometry capture above (Frozen Decision 1:
      // no Print* may escape the worker). Reads Print::get_filament_maps()
      // (Print.cpp:3051) -- the per-extruder mapping the engine computed inside
      // print.process() at Print.cpp:2484-2491 (only when mode < fmmManual).
      // The mode is read via Print::get_filament_map_mode() (Print.cpp:3056)
      // and stored as the Phase 107 OWzx::FilamentMapMode enum (the upstream
      // Slic3r::FilamentMapMode has identical numeric values, so a
      // static_cast<int> round-trips losslessly). valid is set ONLY when the
      // auto-recommendation actually ran (mode < fmmManual per Print.cpp:2485):
      // when the user picked Manual, the engine does not compute an auto-map
      // and there is nothing to surface (mirrors WTREAD-02 gate logic).
      {
        const int mapModeInt = static_cast<int>(print.get_filament_map_mode());
        capturedFilamentMap.mode =
            static_cast<OWzx::FilamentMapMode>(mapModeInt);
        if (mapModeInt < static_cast<int>(OWzx::FilamentMapMode::fmmManual)) {
          capturedFilamentMap.maps = print.get_filament_maps();
          capturedFilamentMap.valid = true;
        }
      }

      receiver->activePrint_.store(nullptr, std::memory_order_release);
    }
    catch (const std::exception &ex)
    {
      receiver->activePrint_.store(nullptr, std::memory_order_release);
      errorText = QString::fromUtf8(ex.what());
    }
    catch (...)
    {
      receiver->activePrint_.store(nullptr, std::memory_order_release);
      errorText = QObject::tr("Slicing failed");
    }
#else
    // Mock mode: simulate slicing progress with fake results
    Q_UNUSED(sourcePath);

    // Generate mock result values based on plate objects count
    int objectCount = 0;
    QString plateLabelForMock = targetPlateLabel;
    if (receiver && receiver->projectService_) {
      objectCount = receiver->projectService_->objectNames().size();
    }

    const int mockLayers = 80 + objectCount * 40;
    const double mockTimeSecs = 120.0 + objectCount * 180.0;
    estimatedTimeLabel = formatDurationLabel(mockTimeSecs);
    resultWeightLabel = QStringLiteral("%1 g").arg(12.5 + objectCount * 8.3, 0, 'f', 1);
    resultFilamentLabel = QStringLiteral("%1 m").arg(2.5 + objectCount * 1.8, 0, 'f', 1);
    const double weightKg = (12.5 + objectCount * 8.3) / 1000.0;
    resultCostLabel = QStringLiteral("$%1").arg(weightKg * 20.0, 0, 'f', 2);
    resultPlateLabel = plateLabelForMock;
    resultPlateIndex = targetPlateIndex;
    outputPath = QStringLiteral("(mock) %1_plate%2.gcode")
                   .arg(QFileInfo(sourcePath).completeBaseName())
                   .arg(targetPlateIndex + 1);

    // Simulate progress with 1-second steps
    const int totalSteps = 5;
    for (int step = 1; step <= totalSteps; ++step) {
      if (cancelFlag && cancelFlag->load())
        break;
      const int pct = step * 100 / totalSteps;
      const QStringList labels = {
        QObject::tr("Preparing slice parameters"),
        QObject::tr("Generating layers"),
        QObject::tr("Generating supports"),
        QObject::tr("Calculating paths"),
        QObject::tr("Exporting G-code")
      };
      QMetaObject::invokeMethod(receiver, [receiver, pct, labels, step]() {
        if (!receiver) return;
        receiver->progress_ = pct;
        receiver->statusLabel_ = labels.value(step - 1, QObject::tr("Slicing"));
        emit receiver->progressChanged();
        emit receiver->progressUpdated(pct, receiver->statusLabel_);
      }, Qt::BlockingQueuedConnection);
      QThread::msleep(400);
    }
#endif

    if (!receiver)
      return;

    QMetaObject::invokeMethod(receiver, [receiver, cancelFlag, outputPath, errorText, estimatedTimeLabel, resultWeightLabel, resultPlateLabel, resultPlateIndex, resultFilamentLabel, resultCostLabel, layerCount, capturedGeometry, capturedFilamentMap]() {
      if (!receiver)
        return;

      receiver->slicing_ = false;
      receiver->activeCancelFlag_.reset();
      emit receiver->slicingChanged();

      if (cancelFlag && cancelFlag->load())
      {
        receiver->sliceState_ = State::Cancelled;
        receiver->statusLabel_ = QObject::tr("Slicing cancelled");
        receiver->clearActiveTargetResult();
        qInfo("[SliceService] slice cancelled plate=%d reason=%s",
              resultPlateIndex,
              receiver->statusLabel_.toUtf8().constData());
        emit receiver->progressChanged();
        emit receiver->resultChanged();
        emit receiver->sliceResultCleared();
        emit receiver->stateChanged();
        emit receiver->sliceFailed(receiver->statusLabel_);
        receiver->activeTargetPlateIndex_ = -1;
        return;
      }

      if (!errorText.isEmpty())
      {
        receiver->sliceState_ = State::Error;
        receiver->statusLabel_ = errorText;
        receiver->clearActiveTargetResult();
        qWarning("[SliceService] slice failed plate=%d reason=%s",
                 resultPlateIndex,
                 errorText.toUtf8().constData());
        emit receiver->progressChanged();
        emit receiver->resultChanged();
        emit receiver->sliceResultCleared();
        emit receiver->stateChanged();
        emit receiver->sliceFailed(errorText);
        receiver->activeTargetPlateIndex_ = -1;
        return;
      }

      receiver->sliceState_ = State::Completed;
      receiver->progress_ = 100;
      receiver->statusLabel_ = QObject::tr("Slice complete");
      receiver->outputPath_ = outputPath;
      receiver->estimatedTimeLabel_ = estimatedTimeLabel;
      receiver->resultWeightLabel_ = resultWeightLabel;
      receiver->resultPlateLabel_ = resultPlateLabel;
      receiver->resultPlateIndex_ = resultPlateIndex;
      receiver->resultFilamentLabel_ = resultFilamentLabel;
      receiver->resultCostLabel_ = resultCostLabel;
      receiver->resultLayerCount_ = layerCount;

      // Store per-plate result for multi-plate tracking
      if (resultPlateIndex >= 0) {
        PlateSliceResult pr;
        pr.estimatedTimeLabel = estimatedTimeLabel;
        pr.resultWeightLabel = resultWeightLabel;
        pr.resultFilamentLabel = resultFilamentLabel;
        pr.resultCostLabel = resultCostLabel;
        pr.outputPath = outputPath;
        pr.resultLayerCount = receiver->resultLayerCount_;
        pr.totalFilamentMm = receiver->resultTotalFilamentMm();
        pr.source = int(ResultSource::ModelSlice);
        receiver->storePlateResult(resultPlateIndex, pr);
      }
      receiver->activeTargetPlateIndex_ = -1;
      qInfo("[SliceService] slice finished plate=%d output=%s layers=%d",
            resultPlateIndex,
            outputPath.toUtf8().constData(),
            layerCount);
      emit receiver->progressChanged();
      emit receiver->progressUpdated(100, receiver->statusLabel_);
      emit receiver->resultChanged();
      emit receiver->sliceFinished(receiver->estimatedTimeLabel_);
      // Phase 100 (WTREAD-01): deliver the captured-by-value wipe-tower geometry
      // to the GUI thread. Emitted on the success branch only (cancel/error
      // branches above return early without reaching here). The
      // EditorViewModel::onWipeTowerGeometryReady slot applies the has_wipe_tower()
      // gate (WTREAD-02): when capturedGeometry.valid is false, showWipeTower
      // stays false and no placeholder dims are pushed.
      qInfo("[SliceService] wipe-tower ready valid=%d w=%.2f d=%.2f h=%.2f x=%.2f z=%.2f",
            capturedGeometry.valid ? 1 : 0,
            capturedGeometry.width, capturedGeometry.depth,
            capturedGeometry.height, capturedGeometry.x, capturedGeometry.z);
      emit receiver->wipeTowerGeometryReady(capturedGeometry);
      // Phase 108 (FMAP-01): deliver the captured-by-value filament-map
      // auto-recommendation to the GUI thread. Emitted on the success branch
      // only (the cancel/error branches above return early without reaching
      // here) -- same gate as wipeTowerGeometryReady (v4.4 WTREAD-02). The
      // EditorViewModel::onFilamentMapReady slot applies the valid gate: when
      // capturedFilamentMap.valid is false (user picked Manual, so the engine
      // computed no auto-map), hasAutoFilamentMap stays false and no stale map
      // leaks to the Phase 110 UI.
      qInfo("[SliceService] filament-map ready valid=%d mode=%d count=%d",
            capturedFilamentMap.valid ? 1 : 0,
            static_cast<int>(capturedFilamentMap.mode),
            static_cast<int>(capturedFilamentMap.maps.size()));
      emit receiver->filamentMapReady(capturedFilamentMap);
    }, Qt::QueuedConnection); });
}

void SliceService::cancelSlice()
{
  if (!slicing_ || !activeCancelFlag_)
    return;

  activeCancelFlag_->store(true);
  statusLabel_ = QObject::tr("Cancelling slice");
#ifdef HAS_LIBSLIC3R
  if (Slic3r::Print *active = activePrint_.load(std::memory_order_acquire))
    active->cancel();
#endif
  emit progressChanged();
  emit stateChanged();
}

bool SliceService::loadGCodeFromPrevious(const QString &gcodeFilePath)
{
  if (slicing_)
    return false;

  const QFileInfo info(gcodeFilePath);
  const QString localPath = info.absoluteFilePath();
  const int targetPlateIndex = projectService_ ? projectService_->currentPlateIndex() : -1;
  activeTargetPlateIndex_ = targetPlateIndex;
  plateResults_.remove(targetPlateIndex);
  clearStoredResult();
  emit resultChanged();
  emit sliceResultCleared();

  if (!info.exists() || !info.isFile())
  {
    sliceState_ = State::Error;
    statusLabel_ = QObject::tr("G-code file does not exist");
    qWarning("[SliceService] slice failed source=%s plate=%d reason=%s",
             localPath.toUtf8().constData(),
             targetPlateIndex,
             statusLabel_.toUtf8().constData());
    emit progressChanged();
    emit stateChanged();
    emit sliceFailed(statusLabel_);
    activeTargetPlateIndex_ = -1;
    return false;
  }

  slicing_ = true;
  progress_ = 0;
  statusLabel_ = QObject::tr("Reusing existing G-code");
  outputPath_.clear();
  activeCancelFlag_ = std::make_shared<std::atomic_bool>(false);
  emit slicingChanged();
  emit progressChanged();
  emit progressUpdated(progress_, statusLabel_);

  const QPointer<SliceService> receiver(this);
  const auto cancelFlag = activeCancelFlag_;
  QString targetPlateLabel;
  if (projectService_)
  {
    const QStringList plateNames = projectService_->plateNames();
    if (targetPlateIndex >= 0 && targetPlateIndex < plateNames.size() && !plateNames[targetPlateIndex].isEmpty())
      targetPlateLabel = plateNames[targetPlateIndex];
    else if (targetPlateIndex >= 0)
      targetPlateLabel = QObject::tr("Plate %1").arg(targetPlateIndex + 1);
  }

  QtConcurrent::run([receiver, cancelFlag, localPath, targetPlateIndex, targetPlateLabel]()
                    {
    QString errorText;

#ifdef HAS_LIBSLIC3R
    try
    {
      Slic3r::Print print;
      Slic3r::GCodeProcessorResult result;
      print.export_gcode_from_previous_file(localPath.toStdString(), &result);
    }
    catch (const std::exception &ex)
    {
      errorText = QString::fromUtf8(ex.what());
    }
    catch (...)
    {
      errorText = QObject::tr("Failed to reuse G-code");
    }
#else
    Q_UNUSED(localPath);
    errorText = QObject::tr("Current build does not enable libslic3r");
#endif

    if (!receiver)
      return;

    QMetaObject::invokeMethod(receiver, [receiver, cancelFlag, localPath, errorText, targetPlateIndex, targetPlateLabel]() {
      if (!receiver)
        return;

      receiver->slicing_ = false;
      receiver->activeCancelFlag_.reset();
      emit receiver->slicingChanged();

      if (cancelFlag && cancelFlag->load())
      {
        receiver->sliceState_ = State::Cancelled;
        receiver->statusLabel_ = QObject::tr("Slicing cancelled");
        receiver->clearActiveTargetResult();
        qInfo("[SliceService] slice cancelled plate=%d reason=%s",
              targetPlateIndex,
              receiver->statusLabel_.toUtf8().constData());
        emit receiver->progressChanged();
        emit receiver->resultChanged();
        emit receiver->sliceResultCleared();
        emit receiver->stateChanged();
        emit receiver->sliceFailed(receiver->statusLabel_);
        receiver->activeTargetPlateIndex_ = -1;
        return;
      }

      if (!errorText.isEmpty())
      {
        receiver->sliceState_ = State::Error;
        receiver->statusLabel_ = errorText;
        receiver->clearActiveTargetResult();
        qWarning("[SliceService] slice failed source=%s plate=%d reason=%s",
                 localPath.toUtf8().constData(),
                 targetPlateIndex,
                 errorText.toUtf8().constData());
        emit receiver->progressChanged();
        emit receiver->resultChanged();
        emit receiver->sliceResultCleared();
        emit receiver->stateChanged();
        emit receiver->sliceFailed(errorText);
        receiver->activeTargetPlateIndex_ = -1;
        return;
      }

      receiver->sliceState_ = State::Completed;
      receiver->progress_ = 100;
      receiver->statusLabel_ = QObject::tr("Existing G-code reuse complete");
      receiver->outputPath_ = localPath;
      receiver->resultPlateLabel_ = targetPlateLabel;
      receiver->resultPlateIndex_ = targetPlateIndex;
      if (targetPlateIndex >= 0)
      {
        PlateSliceResult pr;
        pr.estimatedTimeLabel = QString{};
        pr.resultWeightLabel = QString{};
        pr.resultFilamentLabel = QString{};
        pr.resultCostLabel = QString{};
        pr.outputPath = localPath;
        pr.resultLayerCount = 0;
        pr.totalFilamentMm = 0.0;
        pr.source = int(ResultSource::PreviousGCode);
        receiver->storePlateResult(targetPlateIndex, pr);
      }
      receiver->activeTargetPlateIndex_ = -1;
      emit receiver->progressChanged();
      emit receiver->progressUpdated(100, receiver->statusLabel_);
      emit receiver->resultChanged();
      emit receiver->sliceFinished(QString{});
    }, Qt::QueuedConnection); });

  return true;
}

void SliceService::startSlicePlate(int plateIndex)
{
  if (slicing_)
    return;
  if (projectService_)
    projectService_->setCurrentPlateIndex(plateIndex);
  startSlice(projectService_ ? projectService_->projectName() : QString{});
}

bool SliceService::exportGCodeToPath(const QString &targetPath)
{
  return exportSourceToPath(outputPath_, targetPath, defaultExportGCodeFileName(resultPlateIndex_));
}

QString SliceService::defaultExportGCodeFileName(int plateIndex) const
{
  QString baseName;
  if (projectService_)
  {
    baseName = projectService_->projectName();
    if (baseName.isEmpty())
      baseName = QFileInfo(projectService_->sourceFilePath()).completeBaseName();
  }
  if (baseName.isEmpty())
    baseName = QFileInfo(outputPath_).completeBaseName();
  baseName = sanitizeFileStem(baseName);

  const int resolvedPlate = plateIndex >= 0 ? plateIndex : (projectService_ ? projectService_->currentPlateIndex() : resultPlateIndex_);
  const int totalPlates = projectService_ ? projectService_->plateCount() : plateResults_.size();
  if (resolvedPlate >= 0 && totalPlates > 1)
  {
    QString plateLabel;
    if (projectService_)
    {
      const QStringList names = projectService_->plateNames();
      if (resolvedPlate < names.size())
        plateLabel = sanitizeFileStem(names[resolvedPlate]);
    }
    if (plateLabel.isEmpty())
      plateLabel = QStringLiteral("plate%1").arg(resolvedPlate + 1);
    baseName += QStringLiteral("_%1").arg(plateLabel);
  }

  return ensureGcodeSuffix(baseName);
}

bool SliceService::exportPlateGCodeToPath(int plateIndex, const QString &targetPath)
{
  const auto it = plateResults_.constFind(plateIndex);
  if (it == plateResults_.constEnd())
  {
    setExportStatus(State::Completed, progress_, QObject::tr("No G-code result for plate %1").arg(plateIndex + 1));
    logExportFailure(QStringLiteral("plate"),
                     QString{},
                     targetPath,
                     statusLabel_);
    emit exportFailed(statusLabel_);
    return false;
  }
  return exportSourceToPath(it->outputPath, targetPath, defaultExportGCodeFileName(plateIndex));
}

bool SliceService::exportAllPlateGCodeToDirectory(const QString &directoryPath, const QString &baseName)
{
  const QString cleanDirectory = QDir::cleanPath(localPathFromDialogValue(directoryPath).trimmed());
  if (cleanDirectory.isEmpty())
  {
    setExportStatus(State::Completed, progress_, QObject::tr("Choose a G-code export directory"));
    logExportFailure(QStringLiteral("all"),
                     QString{},
                     directoryPath,
                     statusLabel_);
    emit exportFailed(statusLabel_);
    return false;
  }

  QDir dir(cleanDirectory);
  if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
  {
    setExportStatus(State::Completed, progress_, QObject::tr("Failed to create export directory"));
    logExportFailure(QStringLiteral("all"),
                     QString{},
                     cleanDirectory,
                     statusLabel_);
    emit exportFailed(statusLabel_);
    return false;
  }
  if (!QFileInfo(cleanDirectory).isDir())
  {
    setExportStatus(State::Completed, progress_, QObject::tr("G-code export target is not a directory"));
    logExportFailure(QStringLiteral("all"),
                     QString{},
                     cleanDirectory,
                     statusLabel_);
    emit exportFailed(statusLabel_);
    return false;
  }

  QString stem = sanitizeFileStem(baseName);
  if (baseName.trimmed().isEmpty())
  {
    stem = sanitizeFileStem(projectService_ ? projectService_->projectName() : QString{});
    if (stem == QStringLiteral("output") && projectService_)
      stem = sanitizeFileStem(QFileInfo(projectService_->sourceFilePath()).completeBaseName());
  }

  bool exportedAny = false;
  qInfo("[SliceService] export all start dir=%s base=%s storedPlates=%d",
        dir.absolutePath().toUtf8().constData(),
        stem.toUtf8().constData(),
        int(plateResults_.size()));
  for (auto it = plateResults_.constBegin(); it != plateResults_.constEnd(); ++it)
  {
    const int plateIndex = it.key();
    if (projectService_ && (projectService_->isPlateLocked(plateIndex) || !projectService_->isPlatePrintable(plateIndex)))
      continue;
    if (it->outputPath.isEmpty())
      continue;

    const QString target = dir.filePath(ensureGcodeSuffix(QStringLiteral("%1_plate%2").arg(stem).arg(plateIndex + 1)));
    if (!exportPlateGCodeToPath(plateIndex, target))
      return false;
    exportedAny = true;
  }

  if (!exportedAny)
  {
    setExportStatus(State::Completed, progress_, QObject::tr("No sliced plates to export"));
    logExportFailure(QStringLiteral("all"),
                     QString{},
                     cleanDirectory,
                     statusLabel_);
    emit exportFailed(statusLabel_);
    return false;
  }

  setExportStatus(State::Completed, 100, QObject::tr("Exported all sliced plates"));
  return true;
}

void SliceService::setExportStatus(State state, int progress, const QString &label)
{
  sliceState_ = state;
  progress_ = qBound(0, progress, 100);
  statusLabel_ = label;
  emit progressChanged();
  emit stateChanged();
}

bool SliceService::exportSourceToPath(const QString &sourcePath, const QString &targetPath, const QString &displayName)
{
  const auto failExport = [this, &sourcePath, &targetPath](const QString &reason) {
    setExportStatus(State::Completed, progress_, reason);
    logExportFailure(QStringLiteral("single"), sourcePath, targetPath, statusLabel_);
    emit exportFailed(statusLabel_);
    return false;
  };

  if (sourcePath.isEmpty())
  {
    return failExport(QObject::tr("No G-code to export; slice first"));
  }

  const QFileInfo srcInfo(localPathFromDialogValue(sourcePath));
  if (!srcInfo.exists() || !srcInfo.isFile())
  {
    return failExport(QObject::tr("G-code source file does not exist"));
  }
  if (srcInfo.size() <= 0)
  {
    return failExport(QObject::tr("G-code source file is empty"));
  }

  const QString cleanTarget = QDir::cleanPath(localPathFromDialogValue(targetPath).trimmed());
  if (cleanTarget.isEmpty())
  {
    return failExport(QObject::tr("Choose a G-code export path"));
  }

  QFileInfo targetInfo(cleanTarget);
  if (targetInfo.exists() && targetInfo.isDir())
  {
    return failExport(QObject::tr("G-code export target is a directory"));
  }

  QDir targetDir = targetInfo.absoluteDir();
  if (!targetDir.exists() && !targetDir.mkpath(QStringLiteral(".")))
  {
    return failExport(QObject::tr("Failed to create G-code export directory"));
  }

  targetInfo = QFileInfo(cleanTarget);
  if (comparablePath(srcInfo) == comparablePath(targetInfo))
  {
    return failExport(QObject::tr("Choose a different G-code export path"));
  }

  QFile input(srcInfo.absoluteFilePath());
  if (!input.open(QIODevice::ReadOnly))
  {
    return failExport(QObject::tr("Failed to read G-code source file"));
  }

  setExportStatus(State::Exporting, 95, QObject::tr("Exporting G-code"));
  emit exportStarted(statusLabel_);

  QSaveFile output(targetInfo.absoluteFilePath());
  if (!output.open(QIODevice::WriteOnly))
  {
    return failExport(QObject::tr("Failed to open G-code export target"));
  }

  constexpr qint64 kChunkSize = 1024 * 1024;
  QByteArray buffer;
  buffer.resize(int(kChunkSize));
  qint64 copiedBytes = 0;
  while (!input.atEnd())
  {
    const qint64 readBytes = input.read(buffer.data(), buffer.size());
    if (readBytes < 0)
    {
      output.cancelWriting();
      return failExport(QObject::tr("Failed to read G-code source file"));
    }
    if (output.write(buffer.constData(), readBytes) != readBytes)
    {
      output.cancelWriting();
      return failExport(QObject::tr("Failed to write G-code export target"));
    }
    copiedBytes += readBytes;
  }

  if (copiedBytes != srcInfo.size())
  {
    output.cancelWriting();
    return failExport(QObject::tr("G-code export byte count mismatch"));
  }

  if (!output.commit())
  {
    return failExport(QObject::tr("Failed to finalize G-code export"));
  }

  const QFileInfo finalInfo(targetInfo.absoluteFilePath());
  if (!finalInfo.exists() || !finalInfo.isFile() || finalInfo.size() != srcInfo.size())
  {
    return failExport(QObject::tr("G-code export verification failed"));
  }

  const QString exportedName = finalInfo.fileName().isEmpty() ? displayName : finalInfo.fileName();
  setExportStatus(State::Completed, 100, QObject::tr("Exported: %1").arg(exportedName));
  qInfo("[SliceService] export finished source=%s target=%s bytes=%lld",
        srcInfo.absoluteFilePath().toUtf8().constData(),
        finalInfo.absoluteFilePath().toUtf8().constData(),
        static_cast<long long>(finalInfo.size()));
  emit exportFinished(finalInfo.absoluteFilePath());
  return true;
}

bool SliceService::hasPlateResult(int plateIndex) const
{
  const auto it = plateResults_.constFind(plateIndex);
  if (it == plateResults_.constEnd() || it->outputPath.isEmpty())
    return false;
  return QFileInfo::exists(it->outputPath);
}

QString SliceService::plateEstimatedTime(int plateIndex) const
{
  auto it = plateResults_.constFind(plateIndex);
  return it != plateResults_.constEnd() ? it->estimatedTimeLabel : QString();
}

QString SliceService::plateWeight(int plateIndex) const
{
  auto it = plateResults_.constFind(plateIndex);
  return it != plateResults_.constEnd() ? it->resultWeightLabel : QString();
}

QString SliceService::plateFilament(int plateIndex) const
{
  auto it = plateResults_.constFind(plateIndex);
  return it != plateResults_.constEnd() ? it->resultFilamentLabel : QString();
}

QString SliceService::plateCost(int plateIndex) const
{
  auto it = plateResults_.constFind(plateIndex);
  return it != plateResults_.constEnd() ? it->resultCostLabel : QString();
}

int SliceService::plateLayerCount(int plateIndex) const
{
  auto it = plateResults_.constFind(plateIndex);
  return it != plateResults_.constEnd() ? it->resultLayerCount : 0;
}

QString SliceService::plateOutputPath(int plateIndex) const
{
  auto it = plateResults_.constFind(plateIndex);
  return it != plateResults_.constEnd() ? it->outputPath : QString();
}

int SliceService::plateResultSource(int plateIndex) const
{
  auto it = plateResults_.constFind(plateIndex);
  return it != plateResults_.constEnd() ? it->source : int(ResultSource::None);
}

bool SliceService::activatePlateResult(int plateIndex)
{
  if (slicing_)
    return false;

  const auto it = plateResults_.constFind(plateIndex);
  if (it == plateResults_.constEnd() || it->outputPath.isEmpty() || !QFileInfo::exists(it->outputPath))
  {
    if (resultPlateIndex_ != -1 || !outputPath_.isEmpty())
    {
      clearStoredResult();
      emit resultChanged();
      emit sliceResultCleared();
      emit stateChanged();
    }
    return false;
  }

  sliceState_ = State::Completed;
  progress_ = 100;
  statusLabel_ = it->source == int(ResultSource::PreviousGCode)
      ? QObject::tr("Existing G-code reuse complete")
      : QObject::tr("Slice complete");
  outputPath_ = it->outputPath;
  estimatedTimeLabel_ = it->estimatedTimeLabel;
  resultWeightLabel_ = it->resultWeightLabel;
  resultPlateIndex_ = plateIndex;
  resultFilamentLabel_ = it->resultFilamentLabel;
  resultLayerCount_ = it->resultLayerCount;
  resultCostLabel_ = it->resultCostLabel;

  if (projectService_)
  {
    const QStringList plateNames = projectService_->plateNames();
    if (plateIndex >= 0 && plateIndex < plateNames.size() && !plateNames[plateIndex].isEmpty())
      resultPlateLabel_ = plateNames[plateIndex];
    else if (plateIndex >= 0)
      resultPlateLabel_ = QObject::tr("Plate %1").arg(plateIndex + 1);
    else
      resultPlateLabel_.clear();
  }

  emit progressChanged();
  emit resultChanged();
  emit stateChanged();
  return true;
}

void SliceService::clearPlateResults()
{
  clearStoredResult();
  plateResults_.clear();
  emit resultChanged();
  emit sliceResultCleared();
  emit stateChanged();
}

void SliceService::removePlateResult(int plateIndex)
{
  const bool removedPlateMetadata = plateResults_.remove(plateIndex) > 0;
  bool clearedCurrentOutput = false;
  if (resultPlateIndex_ == plateIndex)
  {
    clearStoredResult();
    clearedCurrentOutput = true;
  }

  if (removedPlateMetadata || clearedCurrentOutput)
  {
    emit resultChanged();
    if (clearedCurrentOutput)
      emit sliceResultCleared();
    emit stateChanged();
  }
}


void SliceService::setBedShape(const QVector<QPointF> &pointsMm)
{
  bedShape_ = pointsMm;
}


void SliceService::setCalibParams(int mode, double start, double end, double step,
                                  bool printNumbers, int testModel)
{
  calibConfig_ = {mode, start, end, step, printNumbers, testModel};
}

