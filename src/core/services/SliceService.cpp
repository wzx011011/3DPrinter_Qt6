#include "SliceService.h"

#include "core/services/ProjectServiceMock.h"
#include "core/services/AppSettingsService.h"
#include "core/util/PerfLog.h"

#include <QMetaObject>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>
#include <QCoreApplication>
#include <QDebug>
#include <algorithm>
#include <QDir>
#include <QStringList>
#include <QFileInfo>
#include <QDateTime>
#include <QFile>
#include <QThread>
#include <QSaveFile>
#include <QUrl>
#include <functional>
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
#ifdef HAS_LIBSLIC3R
  template <typename PolygonLike>
  void appendPolygonOutline(const PolygonLike &polygon, std::vector<float> &out, float y)
  {
    if (polygon.points.size() < 2)
      return;
    for (size_t i = 0; i < polygon.points.size(); ++i)
    {
      const auto &a = polygon.points[i];
      const auto &b = polygon.points[(i + 1) % polygon.points.size()];
      out.push_back(float(Slic3r::unscaled<double>(a.x())));
      out.push_back(y);
      out.push_back(float(Slic3r::unscaled<double>(a.y())));
      out.push_back(float(Slic3r::unscaled<double>(b.x())));
      out.push_back(y);
      out.push_back(float(Slic3r::unscaled<double>(b.y())));
    }
  }

  template <typename PolygonLike>
  void appendPolygonFan(const PolygonLike &polygon, std::vector<float> &out, float y)
  {
    if (polygon.points.size() < 3)
      return;
    const auto &origin = polygon.points.front();
    for (size_t i = 1; i + 1 < polygon.points.size(); ++i)
    {
      const auto &a = polygon.points[i];
      const auto &b = polygon.points[i + 1];
      for (const auto *point : {&origin, &a, &b})
      {
        out.push_back(float(Slic3r::unscaled<double>(point->x())));
        out.push_back(y);
        out.push_back(float(Slic3r::unscaled<double>(point->y())));
      }
    }
  }

  // P15.11: moved to SliceService::packSequentialClearance (public static) so
  // the drag-time preview packs through the SAME stream builder.
#endif

  QString formatDurationLabel(double seconds)
  {
    // P2.4: upstream compact DHMS format (libslic3r Utils.hpp
    // get_bbl_time_dhms) — NdNhNmNs / NhNmNs / NmNs / Ns, no separators.
    const qint64 totalSeconds = std::max<qint64>(0, qRound64(seconds));
    const qint64 days = totalSeconds / 86400;
    const qint64 hours = (totalSeconds % 86400) / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 secs = totalSeconds % 60;
    if (days > 0)
      return QStringLiteral("%1d%2h%3m%4s").arg(days).arg(hours).arg(minutes).arg(secs);
    if (hours > 0)
      return QStringLiteral("%1h%2m%3s").arg(hours).arg(minutes).arg(secs);
    if (minutes > 0)
      return QStringLiteral("%1m%2s").arg(minutes).arg(secs);
    return QStringLiteral("%1s").arg(secs);
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

  /// Phase 239 (ENGN-03): outcome of the worker-side chunked G-code copy.
  /// `cancelled` is reported separately from a plain failure so the GUI-thread
  /// delivery can set State::Cancelled (mirrors cancelSlice).
  struct ExportCopyOutcome
  {
    bool ok = false;
    bool cancelled = false;
    QString error;
    QString finalPath;
    qint64 finalBytes = 0;
  };

  /// Phase 239 (ENGN-03): the chunked 1 MiB copy that used to run inline on
  /// the GUI thread (exportSourceToPath). Runs inside the QtConcurrent worker;
  /// `onProgress` is invoked (on the worker thread) whenever the byte-derived
  /// percent changes -- callers wrap it in a queued invokeMethod so only the
  /// GUI thread touches SliceService members. Cancel aborts at the next chunk
  /// boundary via cancelWriting() (QSaveFile removes the partial target).
  ExportCopyOutcome copyGcodeChunked(const QString &sourceAbs,
                                     const QString &targetAbs,
                                     qint64 totalBytes,
                                     const std::shared_ptr<std::atomic_bool> &cancelFlag,
                                     const std::function<void(int)> &onProgress)
  {
    ExportCopyOutcome outcome;
    outcome.finalPath = targetAbs;

    QFile input(sourceAbs);
    if (!input.open(QIODevice::ReadOnly))
    {
      outcome.error = QObject::tr("Failed to read G-code source file");
      return outcome;
    }

    QSaveFile output(targetAbs);
    if (!output.open(QIODevice::WriteOnly))
    {
      outcome.error = QObject::tr("Failed to open G-code export target");
      return outcome;
    }

    constexpr qint64 kChunkSize = 1024 * 1024;
    QByteArray buffer;
    buffer.resize(int(kChunkSize));
    qint64 copiedBytes = 0;
    int lastPercent = -1;
    while (!input.atEnd())
    {
      if (cancelFlag && cancelFlag->load())
      {
        output.cancelWriting();
        outcome.cancelled = true;
        outcome.error = QObject::tr("Export cancelled");
        return outcome;
      }
      const qint64 readBytes = input.read(buffer.data(), buffer.size());
      if (readBytes < 0)
      {
        output.cancelWriting();
        outcome.error = QObject::tr("Failed to read G-code source file");
        return outcome;
      }
      if (output.write(buffer.constData(), readBytes) != readBytes)
      {
        output.cancelWriting();
        outcome.error = QObject::tr("Failed to write G-code export target");
        return outcome;
      }
      copiedBytes += readBytes;
      if (onProgress && totalBytes > 0)
      {
        qint64 pct64 = copiedBytes * 100 / totalBytes;
        if (pct64 < 0)
          pct64 = 0;
        else if (pct64 > 100)
          pct64 = 100;
        const int pct = int(pct64);
        if (pct != lastPercent)
        {
          lastPercent = pct;
          onProgress(pct);
        }
      }
    }

    if (totalBytes > 0 && copiedBytes != totalBytes)
    {
      output.cancelWriting();
      outcome.error = QObject::tr("G-code export byte count mismatch");
      return outcome;
    }

    if (!output.commit())
    {
      outcome.error = QObject::tr("Failed to finalize G-code export");
      return outcome;
    }

    const QFileInfo finalInfo(targetAbs);
    if (!finalInfo.exists() || !finalInfo.isFile() || (totalBytes > 0 && finalInfo.size() != totalBytes))
    {
      outcome.error = QObject::tr("G-code export verification failed");
      return outcome;
    }

    outcome.ok = true;
    outcome.finalBytes = finalInfo.size();
    return outcome;
  }
}

SliceService::SliceService(ProjectServiceMock *projectService, QObject *parent)
    : QObject(parent), projectService_(projectService)
{
  // Wave 3b (perf): queued cross-thread delivery of the in-memory preview.
  qRegisterMetaType<SlicePreviewData>("SlicePreviewData");
  if (projectService_)
    connect(projectService_, &ProjectServiceMock::plateRemoved, this,
            [this](int printId) { removePlateResultByPrintId(printId); });
}

SliceService::~SliceService()
{
  // Wave 3a (perf): stop the dedicated slice thread before members die (a
  // running QThread's destructor would terminate the process).
  // R-P0.2 (upstream BackgroundSlicingProcess.cpp:554-556, stop(): the Print
  // itself is cancelled, not just an external flag -- Print::process() only
  // observes Print::cancel(), so a long-running process() would otherwise
  // outlive wait() and destroy a running QThread = qFatal).
  if (activeCancelFlag_)
    activeCancelFlag_->store(true);
#ifdef HAS_LIBSLIC3R
  if (Slic3r::Print *active = activePrint_.load(std::memory_order_acquire))
    active->cancel();
#endif
  m_sliceThread.quit();
  m_sliceThread.wait(30000);
}

int SliceService::progress() const
{
  return progress_;
}

bool SliceService::slicing() const
{
  return slicing_;
}

bool SliceService::canSwitchPlate() const
{
  // B1: upstream BackgroundSlicingProcess::can_switch_print
  // (BackgroundSlicingProcess.cpp:140-155) refuses plate switches only while
  // a slice is RUNNING (Plater.cpp:13879 gates the preview swap on it).
  return !slicing_;
}

void SliceService::setDomainResultValid(int printId, bool valid)
{
  if (projectService_)
    projectService_->setPlateSliceResultValidByPrintId(printId, valid);
}

int SliceService::printIdForPlateIndex(int plateIndex) const
{
  return projectService_ ? projectService_->platePrintId(plateIndex) : -1;
}

int SliceService::plateIndexForPrintId(int printId) const
{
  return projectService_ ? projectService_->plateIndexForPrintId(printId) : -1;
}

void SliceService::setAllDomainResultsValid(bool valid)
{
  if (projectService_)
    projectService_->setAllSliceResultsValid(valid);
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
  resultPlatePrintId_ = -1;
  resultFilamentLabel_.clear();
  resultLayerCount_ = 0;
  resultCostLabel_.clear();
  emit progressChanged();
}

void SliceService::clearActiveTargetResult()
{
  if (activeTargetPrintId_ >= 0)
    removePlateResultByPrintId(activeTargetPrintId_);
  if (resultPlatePrintId_ == activeTargetPrintId_)
    clearStoredResult();
}

void SliceService::storePlateResult(int printId, const PlateSliceResult &result)
{
  if (printId >= 0)
  {
    plateResults_[printId] = result;
    setDomainResultValid(printId, true);
  }
}

void SliceService::removePlateResultByPrintId(int printId)
{
  if (printId < 0)
    return;
  const bool removed = plateResults_.remove(printId) > 0;
  if (removed)
    setDomainResultValid(printId, false);
  if (resultPlatePrintId_ == printId)
  {
    clearStoredResult();
    resultPlatePrintId_ = -1;
  }
  if (activeTargetPrintId_ == printId)
    activeTargetPrintId_ = -1;
  if (removed)
  {
    emit resultChanged();
    emit sliceResultCleared();
  }
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
  activeTargetPrintId_ = -1;
  plateResults_.clear();
  setAllDomainResultsValid(false);
  emit resultChanged();
  emit sliceResultCleared();
  emit stateChanged();
  emit sliceStateChanged();
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

  void restoreGenericEnumMaps(Slic3r::DynamicPrintConfig &config)
  {
#ifdef HAS_LIBSLIC3R
    for (const std::string &key : config.keys())
    {
      const Slic3r::ConfigOption *option = config.option(key);
      const auto *enumOption = dynamic_cast<const Slic3r::ConfigOptionEnumsGeneric *>(option);
      const auto *nullableEnumOption = dynamic_cast<const Slic3r::ConfigOptionEnumsGenericNullable *>(option);
      if ((!enumOption || enumOption->keys_map != nullptr) &&
          (!nullableEnumOption || nullableEnumOption->keys_map != nullptr))
      {
        continue;
      }

      const Slic3r::ConfigOptionDef *definition = Slic3r::print_config_def.get(key);
      const auto *values = dynamic_cast<const Slic3r::ConfigOptionInts *>(option);
      if (!definition || !values)
        continue;

      std::unique_ptr<Slic3r::ConfigOption> restored(definition->create_default_option());
      if (auto *restoredEnum = dynamic_cast<Slic3r::ConfigOptionEnumsGeneric *>(restored.get()))
      {
        restoredEnum->values = values->values;
      }
      else if (auto *restoredNullableEnum =
                   dynamic_cast<Slic3r::ConfigOptionEnumsGenericNullable *>(restored.get()))
      {
        restoredNullableEnum->values = values->values;
      }
      else
      {
        continue;
      }

      config.set_key_value(key, restored.release());
    }
#else
    Q_UNUSED(config);
#endif
  }
} // anonymous namespace

#ifdef HAS_LIBSLIC3R
// P15.11: shared value-stream packer (see SliceService.h). Identical body to
// the worker-local helper it replaces so the Print::validate payload format
// is unchanged; the drag-time preview (SequentialClearanceCompute::runCompute)
// packs through this too.
SequentialPrintClearance SliceService::packSequentialClearance(
    const Slic3r::Polygons &collision,
    const std::vector<std::pair<Slic3r::Polygon, float>> &height)
{
  SequentialPrintClearance out;
  for (const auto &polygon : collision)
  {
    appendPolygonOutline(polygon, out.collisionOutline, 0.025f);
    appendPolygonFan(polygon, out.collisionFill, 0.0125f);
  }
  for (const auto &entry : height)
    appendPolygonFan(entry.first, out.heightFill, entry.second);
  out.valid = !out.collisionOutline.empty() || !out.heightFill.empty();
  return out;
}

// P15.11: resolved plate config with the SAME merge sequence the slice worker
// runs before Print::apply (defaults -> enum restore -> preset injection ->
// plate overrides -> enum restore -> normalize_fdm). Bed-shape injection and
// the Marlin relative-E tweak are intentionally omitted: neither influences
// the clearance / skirt options the drag preview reads.
Slic3r::DynamicPrintConfig SliceService::makeResolvedPlateConfig(
    const QHash<QString, QVariant> &mergedPreset,
    const Slic3r::DynamicPrintConfig *plateCfg)
{
  Slic3r::DynamicPrintConfig config = Slic3r::DynamicPrintConfig::full_print_config();
  restoreGenericEnumMaps(config);
  if (!mergedPreset.isEmpty())
    injectPresetConfig(config, mergedPreset);
  if (plateCfg && !plateCfg->empty())
    config.apply(*plateCfg);
  restoreGenericEnumMaps(config);
  config.normalize_fdm();
  return config;
}
#endif

namespace
{
  // Wave 1.3 (perf): keep only the newest kSliceCacheKeep outputs in the
  // slice temp directory; names embed yyyyMMdd_HHmmss so lexicographic
  // order is chronological (oldest first).
  constexpr int kSliceCacheKeep = 12;

  void pruneSliceCache(const QString &dirPath)
  {
    QDir dir(dirPath);
    const QStringList entries = dir.entryList(
        {QStringLiteral("*.gcode")}, QDir::Files, QDir::Name);
    for (int i = 0; i < entries.size() - kSliceCacheKeep; ++i)
      QFile::remove(dir.filePath(entries.at(i)));
  }
} // namespace

void SliceService::startSlice(const QString &projectName)
{
  Q_UNUSED(projectName);

  if (slicing_ || exportActive_)
    return;

  const QString sourcePath = projectService_ ? projectService_->sourceFilePath() : QString{};
  const int targetPlateIndex = projectService_ ? projectService_->currentPlateIndex() : -1;
  if (projectService_ && !projectService_->currentPlateCanSlice())
  {
    sliceState_ = State::Error;
    statusLabel_ = QStringLiteral("Current plate is outside the printable area");
    emit progressChanged();
    emit stateChanged();
    emit sliceStateChanged();
    emit sliceFailed(statusLabel_);
    return;
  }
  const int targetPrintId = printIdForPlateIndex(targetPlateIndex);
  activeTargetPrintId_ = targetPrintId;
  removePlateResultByPrintId(targetPrintId);
  clearStoredResult();
  emit resultChanged();
  emit sliceResultCleared();

  QString targetPlateLabel;
#ifdef HAS_LIBSLIC3R
  std::unique_ptr<Slic3r::Model> modelForSlice;
  quint64 geometrySignatureOut = 0; // Wave 4a: plate geometry fingerprint
#endif
  if (projectService_)
  {
    const QStringList plateNames = projectService_->plateNames();
    if (targetPlateIndex >= 0 && targetPlateIndex < plateNames.size() && !plateNames[targetPlateIndex].isEmpty())
      targetPlateLabel = plateNames[targetPlateIndex];
    else if (targetPlateIndex >= 0)
      targetPlateLabel = QObject::tr("Plate %1").arg(targetPlateIndex + 1);
#ifdef HAS_LIBSLIC3R
    // Wave 4a (perf): reuse the persistent per-plate model clone when the
    // plate geometry signature is unchanged (config-only reslice); only a
    // geometry change pays for a fresh clone. The signature is computed on
    // the GUI thread while the model state is stable.
    const quint64 geometrySignature = projectService_->currentPlateGeometrySignature();
    bool reusePersistentModel = false;
    {
      // R-P0.1: m_persistentSlices is shared with the dedicated slice worker;
      // every structural access takes m_persistentMutex. Slot CONTENTS are
      // worker-only while slicing_, and this GUI block only runs when
      // !slicing_ (startSlice guard), so the locked lookup/erase cannot race
      // a live worker slot.
      QMutexLocker persistentLock(&m_persistentMutex);
      auto existingSlot = m_persistentSlices.find(targetPrintId);
      reusePersistentModel = existingSlot != m_persistentSlices.end() &&
          existingSlot->second.model &&
          existingSlot->second.geometrySignature == geometrySignature;
      if (reusePersistentModel) {
        modelForSlice.reset(); // reuse marker for the worker
        qInfo("[SliceService] reusing persistent plate model (sig=%llx)",
              static_cast<unsigned long long>(geometrySignature));
      } else {
        m_persistentSlices.erase(targetPrintId); // stale slot dies
      }
    }
    if (!reusePersistentModel)
      modelForSlice = projectService_->cloneCurrentPlateModel();
    geometrySignatureOut = geometrySignature;
#else
    quint64 geometrySignature = 0;
    geometrySignatureOut = geometrySignature;
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
  emit sliceStateChanged();
    emit sliceFailed(statusLabel_);
      activeTargetPrintId_ = -1;
    return;
  }

#ifdef HAS_LIBSLIC3R
  bool hasPersistentSlot = false;
  {
    // R-P0.1: locked structural access (shared with the slice worker).
    QMutexLocker persistentLock(&m_persistentMutex);
    hasPersistentSlot = m_persistentSlices.count(targetPrintId) > 0;
  }
  // Wave 4a: a null modelForSlice here means the persistent per-plate model
  // is being REUSED (geometry signature unchanged) — the slot's model has
  // the objects; only a freshly cloned empty model is an error.
  if ((!modelForSlice && !hasPersistentSlot)
      || (modelForSlice && modelForSlice->objects.empty()))
  {
    sliceState_ = State::Error;
    statusLabel_ = QStringLiteral("Current plate has no sliceable objects");
    qWarning("[SliceService] slice failed source=%s plate=%d reason=%s",
             sourcePath.toUtf8().constData(),
             targetPlateIndex,
             statusLabel_.toUtf8().constData());
    emit progressChanged();
    emit stateChanged();
  emit sliceStateChanged();
    emit sliceFailed(statusLabel_);
      activeTargetPrintId_ = -1;
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

  // Wave 3a (perf): dispatch onto the DEDICATED slice thread (upstream
  // BackgroundSlicingProcess thread_proc, BackgroundSlicingProcess.cpp:305-364)
  // instead of the shared QtConcurrent pool, so slicing never contends with
  // model load / emboss jobs. The shared_ptr payload keeps the queued
  // functor copyable while the Model moves through it.
  struct SliceJobPayload
  {
    QString sourcePath;
    int targetPlateIndex = -1;
    int targetPrintId = -1;
    QString targetPlateLabel;
#ifdef HAS_LIBSLIC3R
    std::unique_ptr<Slic3r::Model> model;
    quint64 geometrySignature = 0; // Wave 4a: 0 = reuse the persistent slot
#endif
  };
  auto payload = std::make_shared<SliceJobPayload>();
  payload->sourcePath = sourcePath;
  payload->targetPlateIndex = targetPlateIndex;
  payload->targetPrintId = targetPrintId;
  payload->targetPlateLabel = targetPlateLabel;
#ifdef HAS_LIBSLIC3R
  // An empty modelForSlice means the GUI stage marked the persistent slot
  // reusable; carry its geometry signature for the reuse assertion.
  payload->geometrySignature = modelForSlice ? geometrySignatureOut : 0;
  payload->model = std::move(modelForSlice);
#endif
  if (!m_sliceThread.isRunning()) {
    if (!m_sliceWorkerAttached) {
      m_sliceWorker.moveToThread(&m_sliceThread);
      m_sliceWorkerAttached = true;
    }
    m_sliceThread.setObjectName(QStringLiteral("owzx-slice"));
    m_sliceThread.start();
  }
  const bool posted = QMetaObject::invokeMethod(
      &m_sliceWorker,
      [receiver, cancelFlag, payload]()
                    {
    // Aliases keep the original worker body untouched.
    const QString &sourcePath = payload->sourcePath;
    const int targetPlateIndex = payload->targetPlateIndex;
    const int targetPrintId = payload->targetPrintId;
    const QString targetPlateLabel = payload->targetPlateLabel;
#ifdef HAS_LIBSLIC3R
    std::unique_ptr<Slic3r::Model> &modelForSlice = payload->model;
    // Wave 4a (perf): cancel/abort invalidate the persistent per-plate Print
    // state, so those paths drop the slot (erase is a no-op when absent).
    auto discardPersistent = [receiver, targetPrintId]() {
      if (receiver) {
        // R-P0.1: locked structural access (shared with the GUI thread).
        QMutexLocker persistentLock(&receiver->m_persistentMutex);
        receiver->m_persistentSlices.erase(targetPrintId);
      }
    };
#endif
    // OWZX_PERF_LOG gated: end-to-end slice worker wall time
    // (docs/perf-baseline.md).
    const PerfLog::Scope perfScope("slice.worker_total");
    QString errorText;
    QString outputPath;
    QString estimatedTimeLabel;
    QString resultWeightLabel;
    QString resultFilamentLabel;
    QString resultCostLabel;
    QString resultPlateLabel;
    // Phase 239 (ENGN-03): non-fatal Print::validate warning captured by value
    // (no libslic3r type escapes the worker). Emitted as validateWarning() on
    // the success branch only; the cancel/error branches never surface it.
    QString validationWarningText;
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
    SequentialPrintClearance capturedClearance{};
    // P2.3-3: captured-by-value support preview polylines. Same invariant as
    // the other captures: default-constructed (valid=false) in mock mode and
    // on any error/cancel path; only the HAS_LIBSLIC3R success branch fills it.
    SupportPreviewData capturedSupportPreview{};
    // Wave 3b (perf): in-memory preview moves (GCodeProcessorResult mirror,
    // Frozen Decision 1 — no libslic3r type escapes the worker).
    SlicePreviewData capturedPreview{};

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

      // Wave 4a (perf): the persistent per-plate Print lives across slices
      // on this dedicated thread (upstream PartPlate.cpp:3144). Reuse keeps
      // Print::apply incremental (config diff) and enables process(use_cache).
      PersistentSlice *slot = nullptr;
      {
        // R-P0.1: lock only the map structure; slot contents are used without
        // the lock afterwards (worker-only while slicing_ holds the gate).
        QMutexLocker persistentLock(&receiver->m_persistentMutex);
        auto &slices = receiver->m_persistentSlices;
        auto it = slices.find(targetPrintId);
        if (it == slices.end())
          it = slices.emplace(targetPrintId, PersistentSlice{}).first;
        slot = &it->second;
      }
      // Reuse contract from the GUI stage: an EMPTY incoming model clone
      // means the plate geometry signature is unchanged.
      const bool reusePrint =
          (payload->model == nullptr && slot->print != nullptr &&
           slot->model != nullptr);
      if (!reusePrint) {
        if (payload->model)
          slot->model = std::move(payload->model);
        slot->print = std::make_unique<Slic3r::Print>();
        slot->geometrySignature = payload->geometrySignature;
      }
      Slic3r::Print &print = *slot->print;
      receiver->activePrint_.store(&print, std::memory_order_release);

      notify(2, QObject::tr("Preparing current plate model"));
      // Wave 4a: the slot now owns the model (moved above); a null or empty
      // model here means the clone stage failed, not a reuse marker.
      const bool modelMissing = (slot->model == nullptr
                                 || slot->model->objects.empty());
      if (modelMissing)
        throw std::runtime_error("Current plate has no sliceable objects");
      {
        int totalVolumes = 0;
        int totalInstances = 0;
        for (const auto *obj : slot->model->objects) {
          if (obj) {
            totalVolumes += int(obj->volumes.size());
            totalInstances += int(obj->instances.size());
          }
        }
      if (totalVolumes == 0)
        throw std::runtime_error("cloned model has 0 volumes (mesh data missing)");
      }
      if (cancelFlag && cancelFlag->load())
      {
        // R-P0.1: drop activePrint_ BEFORE the slot dies so a concurrent
        // cancelSlice()/dtor can never dereference the destroyed Print (UAF).
        receiver->activePrint_.store(nullptr, std::memory_order_release);
        discardPersistent();
        throw std::runtime_error("Slicing cancelled");
      }

      notify(10, QObject::tr("Preparing slice parameters"));
      Slic3r::DynamicPrintConfig config = Slic3r::DynamicPrintConfig::full_print_config();
      // full_print_config() copies generic enum values without their key maps.
      // Restore them before preset deserialization can invoke enum parsing.
      restoreGenericEnumMaps(config);
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

      // Wave 3a (perf): the maps were already restored right after
      // full_print_config() above; injectPresetConfig/apply do not reset
      // them, so the second full ~800-option walk was redundant.



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

      // Wave 4a (perf): always apply against the PERSISTENT model instance —
      // Print::apply diffs configs/objects against the previous apply and
      // invalidates only the affected steps (PrintApply.cpp:1071-1141);
      // identical objects keep their sliced steps (Print.cpp:1851-1940).
      print.apply(*slot->model, config);

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
        print.set_calib_params(cp);
      }


      if (cancelFlag && cancelFlag->load())
      {
        // R-P0.1: drop activePrint_ BEFORE the slot dies so a concurrent
        // cancelSlice()/dtor can never dereference the destroyed Print (UAF).
        receiver->activePrint_.store(nullptr, std::memory_order_release);
        print.cancel();
        discardPersistent();
        throw std::runtime_error("Slicing cancelled");
      }

      // Pre-slice validation (align upstream BackgroundSlicingProcess::validate).
      // Phase 239 (ENGN-03): upstream Print::validate reports non-fatal
      // findings through the `warning` out-param (Print.cpp:1063;
      // layered_print_cleareance_valid Print.cpp:930-934 writes "too close to
      // others" into it while returning an empty error). A non-empty RETURN
      // value still aborts the slice; a warning does not -- it is delivered to
      // the GUI as a notification (upstream Plater.cpp:13742-13759
      // process_validation_warning) via the validateWarning signal.
      {
        Slic3r::StringObjectException validationWarning;
        Slic3r::Polygons collisionPolygons;
        std::vector<std::pair<Slic3r::Polygon, float>> heightPolygons;
        Slic3r::StringObjectException validationError = print.validate(
            &validationWarning, &collisionPolygons, &heightPolygons);
        capturedClearance = packSequentialClearance(collisionPolygons, heightPolygons);
        if (!validationError.string.empty())
          throw std::runtime_error("Slice validation failed: " + validationError.string);
        validationWarningText = QString::fromUtf8(validationWarning.string.c_str());
      }

      notify(25, QObject::tr("Running slice"));
      // Wave 4a (perf): use_cache lets libslic3r reuse already-completed
      // step results from the previous incremental apply (Print.hpp:841).
      long long cachedTimeCost = 0;
      print.process(&cachedTimeCost, reusePrint);
      if (cancelFlag && cancelFlag->load())
      {
        // R-P0.1: drop activePrint_ BEFORE the slot dies so a concurrent
        // cancelSlice()/dtor can never dereference the destroyed Print (UAF).
        receiver->activePrint_.store(nullptr, std::memory_order_release);
        print.cancel();
        discardPersistent();
        throw std::runtime_error("Slicing cancelled");
      }

      // v2.8 review W2: reset calibConfig (avoid sticking after calibration slice).
      if (receiver) receiver->calibConfig_.mode = 0;


      Slic3r::GCodeProcessorResult result;
      const PerfLog::Scope perfExportScope("slice.export_gcode");
      // Wave 1.3 (perf): timestamped slice outputs go into a bounded
      // slice_cache/ subdirectory and old entries are pruned, instead of
      // accumulating forever in the install directory. Upstream keeps
      // per-plate temp paths under a project-scoped backup dir
      // (PartPlate.cpp:2850-2859).
      // Wave 3a (perf): when a direct output path is set (the CLI knows its
      // --output-dir target up front), export_gcode writes STRAIGHT there —
      // no temp file + copy.
      QString targetPath;
      const QString directPath = receiver ? receiver->directOutputPath_ : QString{};
      if (!directPath.isEmpty()) {
        targetPath = directPath;
      } else {
        const QString appDir = QCoreApplication::applicationDirPath();
        const QString sliceCacheDir = QDir(appDir).filePath(QStringLiteral("slice_cache"));
        QDir().mkpath(sliceCacheDir);
        pruneSliceCache(sliceCacheDir);
        const QString baseName = QFileInfo(sourcePath).completeBaseName();
        const QString fileName = QStringLiteral("%1_%2.gcode")
                                     .arg(baseName)
                                     .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
        targetPath = QDir(sliceCacheDir).filePath(fileName);
      }
      const std::string generated = print.export_gcode(targetPath.toStdString(), &result);
      outputPath = QString::fromStdString(generated);
      estimatedTimeLabel = formatDurationLabel(result.print_statistics.modes[0].time);
      const auto &printStats = print.print_statistics();
      if (printStats.total_weight > 0.0)
        // P2.4: upstream weight format is "%.2fg" (SliceInfoPanel.cpp:324).
        resultWeightLabel = QStringLiteral("%1g").arg(QString::number(printStats.total_weight, 'f', 2));

      // Filament length
      if (printStats.total_used_filament > 0.0)
        resultFilamentLabel = QStringLiteral("%1 m").arg(QString::number(printStats.total_used_filament / 1000.0, 'f', 2));

      // Layer count (captured for main-thread delivery). All objects share
      // the plate layer grid, so the plate count is the MAX object layer
      // count (upstream layer_count semantics), not the sum.
      layerCount = 0;
      for (const Slic3r::PrintObject *obj : print.objects())
          layerCount = std::max(layerCount, static_cast<int>(obj->total_layer_count()));

      // P2.3-3: capture the support preview polylines (upstream support
      // preview renders SupportLayer::support_fills extrusions,
      // GLGizmoFdmSupports.cpp:897-907). Endpoint pairs are flattened to XYZ
      // float triples in bed millimeters: unscaled polyline points + plate
      // origin on X/Y (mirrors the wipe-tower capture) + layer print_z as Z.
      const Slic3r::Vec3d supportPlateOrigin = print.get_plate_origin();
      const double plateOriginX = supportPlateOrigin(0);
      const double plateOriginY = supportPlateOrigin(1);
      for (const Slic3r::PrintObject *pobj : print.objects())
      {
        if (!pobj) continue;
        for (const Slic3r::SupportLayer *sl : pobj->support_layers())
        {
          if (!sl) continue;
          const float printZ = float(sl->print_z);
          for (const Slic3r::ExtrusionEntity *entity : sl->support_fills.entities)
          {
            if (!entity) continue;
            const Slic3r::Polylines polylines = entity->as_polylines();
            for (const Slic3r::Polyline &pl : polylines)
            {
              for (size_t i = 0; i + 1 < pl.points.size(); ++i)
              {
                const Slic3r::Vec2d a = Slic3r::unscale(pl.points[i]);
                const Slic3r::Vec2d b = Slic3r::unscale(pl.points[i + 1]);
                capturedSupportPreview.segmentVerts.push_back(
                    float(a.x() + plateOriginX));
                capturedSupportPreview.segmentVerts.push_back(
                    float(a.y() + plateOriginY));
                capturedSupportPreview.segmentVerts.push_back(printZ);
                capturedSupportPreview.segmentVerts.push_back(
                    float(b.x() + plateOriginX));
                capturedSupportPreview.segmentVerts.push_back(
                    float(b.y() + plateOriginY));
                capturedSupportPreview.segmentVerts.push_back(printZ);
                ++capturedSupportPreview.segmentCount;
              }
            }
          }
        }
      }
      capturedSupportPreview.valid = capturedSupportPreview.segmentCount > 0;

      // Cost
      if (printStats.total_cost > 0.0)
        resultCostLabel = QStringLiteral("$%1").arg(QString::number(printStats.total_cost, 'f', 2));

      // Capture wipe-tower geometry using the APIs present in the locked
      // OrcaSlicer source tree. This version exposes WipeTowerData and the
      // print config, but not the newer mesh/getter readback helpers.
      if (print.has_wipe_tower())
      {
        const Slic3r::WipeTowerData &wtData = print.wipe_tower_data();
        const Slic3r::PrintConfig &printConfig = print.config();
        const Slic3r::Vec3d plateOrigin = print.get_plate_origin();
        const int plateIndex = targetPlateIndex >= 0 ? targetPlateIndex : 0;
        const float towerX = float(printConfig.wipe_tower_x.get_at(plateIndex) + plateOrigin(0));
        const float towerY = float(printConfig.wipe_tower_y.get_at(plateIndex) + plateOrigin(1));
        const float bodyWidth = float(printConfig.prime_tower_width.value);
        const float bodyDepth = wtData.depth;
        const float brimWidth = wtData.brim_width;
        capturedGeometry.valid = true;
        capturedGeometry.width = bodyWidth + 2.0f * brimWidth;
        capturedGeometry.depth = bodyDepth + 2.0f * brimWidth;
        capturedGeometry.height = wtData.height;
        capturedGeometry.brimWidth = brimWidth;
        capturedGeometry.x = towerX + bodyWidth * 0.5f;
        capturedGeometry.z = towerY + bodyDepth * 0.5f;
        capturedGeometry.hasRealMesh = false;
      }

      resultPlateLabel = targetPlateLabel;
      resultPlateIndex = targetPlateIndex;

      // Wave 3b (perf): capture the preview-relevant moves by value so the
      // GUI thread builds the preview from memory (upstream
      // GCodeViewer::load(GCodeProcessorResult), GCodeViewer.hpp:803) instead
      // of re-parsing the exported gcode text.
      {
        auto canonicalRole = [](Slic3r::ExtrusionRole role) -> qint32 {
          switch (role) {
          case Slic3r::erPerimeter: return 1;                 // Inner wall
          case Slic3r::erExternalPerimeter: return 2;         // Outer wall
          case Slic3r::erOverhangPerimeter: return 3;         // Overhang wall
          case Slic3r::erInternalInfill: return 4;            // Sparse infill
          case Slic3r::erSolidInfill: return 5;               // Internal solid
          case Slic3r::erTopSolidInfill: return 6;            // Top surface
          case Slic3r::erIroning: return 7;
          case Slic3r::erBridgeInfill: return 8;
          case Slic3r::erGapFill: return 9;
          case Slic3r::erSkirt: return 10;
          case Slic3r::erSupportMaterial: return 11;
          case Slic3r::erSupportMaterialInterface: return 12;
          case Slic3r::erWipeTower: return 13;
          case Slic3r::erCustom: return 14;
          case Slic3r::erBottomSurface: return 15;             // Bottom surface
          case Slic3r::erInternalBridgeInfill: return 16;
          case Slic3r::erBrim: return 17;
          case Slic3r::erSupportTransition: return 18;
          default: return 0;
          }
        };
        auto canonicalKind = [](Slic3r::EMoveType type) -> qint32 {
          switch (type) {
          case Slic3r::EMoveType::Extrude: return 0;
          case Slic3r::EMoveType::Travel: return 1;
          case Slic3r::EMoveType::Retract: return 2;
          case Slic3r::EMoveType::Unretract: return 3;
          case Slic3r::EMoveType::Wipe: return 4;
          case Slic3r::EMoveType::Seam: return 5;
          case Slic3r::EMoveType::Tool_change: return 6;
          default: return 7;
          }
        };

        capturedPreview.moves.reserve(int(result.moves.size()));
        qint32 layer = -1;
        float lastExtrusionZ = -1e30f;
        double totalFilament = 0.0;
        for (const Slic3r::GCodeProcessorResult::MoveVertex &move : result.moves) {
          if (move.type == Slic3r::EMoveType::Noop)
            continue;
          SlicePreviewMove out;
          out.x = move.position.x();
          out.y = move.position.y();
          out.z = move.position.z();
          out.deltaExtruder = move.delta_extruder;
          out.feedrateMMPerMin = move.feedrate * 60.0f;
          out.width = move.width;
          out.height = move.height;
          out.volumetricRate = move.volumetric_rate();
          out.fanSpeed = move.fan_speed;
          out.temperature = move.temperature;
          out.timeSecs = move.time;
          out.kind = canonicalKind(move.type);
          out.role = canonicalRole(move.extrusion_role);
          out.extruderId = int(move.extruder_id);
          if (move.type == Slic3r::EMoveType::Extrude) {
            if (std::abs(move.position.z() - lastExtrusionZ) > 1e-3f) {
              ++layer;
              lastExtrusionZ = move.position.z();
              capturedPreview.layerZs.append(move.position.z());
            }
            totalFilament += std::max(0.0f, move.delta_extruder);
            capturedPreview.roleTimeSecs[out.role] += double(move.time);
          }
          out.layer = layer;
          capturedPreview.moves.append(out);
        }
        capturedPreview.totalFilamentMm = totalFilament;
      }

      // This upstream source tree does not expose get_filament_maps() /
      // get_filament_map_mode(), so leave the readback invalid and keep the UI
      // from surfacing a stale auto recommendation.
      capturedFilamentMap.valid = false;

      receiver->activePrint_.store(nullptr, std::memory_order_release);
    }
    catch (const std::exception &ex)
    {
      receiver->activePrint_.store(nullptr, std::memory_order_release);
      discardPersistent(); // failed state is not reusable
      errorText = QString::fromUtf8(ex.what());
    }
    catch (...)
    {
      receiver->activePrint_.store(nullptr, std::memory_order_release);
      discardPersistent();
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
    // P2.4: mock weights follow the same "%.2fg" convention as the real path.
    resultWeightLabel = QStringLiteral("%1g").arg(12.5 + objectCount * 8.3, 0, 'f', 2);
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
      // R-P0.2: never block the worker on the GUI thread -- a modal native
      // loop or shutdown would deadlock here against the destructor's
      // m_sliceThread.wait(). Queued delivery + worker-side sleep instead.
      QMetaObject::invokeMethod(receiver, [receiver, pct, labels, step]() {
        if (!receiver) return;
        receiver->progress_ = pct;
        receiver->statusLabel_ = labels.value(step - 1, QObject::tr("Slicing"));
        emit receiver->progressChanged();
        emit receiver->progressUpdated(pct, receiver->statusLabel_);
      }, Qt::QueuedConnection);
      QThread::msleep(400);
    }
#endif

    if (!receiver)
      return;

    QMetaObject::invokeMethod(receiver, [receiver, cancelFlag, outputPath, errorText, estimatedTimeLabel, resultWeightLabel, resultPlateLabel, resultPlateIndex, targetPrintId, resultFilamentLabel, resultCostLabel, layerCount, validationWarningText, capturedGeometry, capturedFilamentMap, capturedClearance, capturedSupportPreview, capturedPreview]() {
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
        // Clearing the active result resets the display state; cancellation
        // remains the terminal state for this worker completion.
        receiver->sliceState_ = State::Cancelled;
        receiver->statusLabel_ = QObject::tr("Slicing cancelled");
        emit receiver->sequentialPrintClearanceReady(SequentialPrintClearance{});
        qInfo("[SliceService] slice cancelled plate=%d reason=%s",
              resultPlateIndex,
              receiver->statusLabel_.toUtf8().constData());
        emit receiver->progressChanged();
        emit receiver->resultChanged();
        emit receiver->sliceResultCleared();
        emit receiver->stateChanged();
        emit receiver->sliceFailed(receiver->statusLabel_);
        return;
      }

      if (!errorText.isEmpty())
      {
        receiver->sliceState_ = State::Error;
        receiver->statusLabel_ = errorText;
        receiver->clearActiveTargetResult();
        emit receiver->sequentialPrintClearanceReady(SequentialPrintClearance{});
        qWarning("[SliceService] slice failed plate=%d reason=%s",
                 resultPlateIndex,
                 errorText.toUtf8().constData());
        emit receiver->progressChanged();
        emit receiver->resultChanged();
        emit receiver->sliceResultCleared();
        emit receiver->stateChanged();
        emit receiver->sliceFailed(errorText);
        return;
      }

      // A plate may be deleted while the worker is finishing. Its stable ID
      // must still resolve before any result becomes visible.
      if (receiver->plateIndexForPrintId(targetPrintId) < 0)
      {
        receiver->clearStoredResult();
        receiver->activeTargetPrintId_ = -1;
        emit receiver->resultChanged();
        emit receiver->sliceResultCleared();
        emit receiver->stateChanged();
        emit receiver->sliceStateChanged();
        return;
      }

      receiver->sliceState_ = State::Completed;
      receiver->progress_ = 100;
      receiver->statusLabel_ = QObject::tr("Slice complete");
      receiver->outputPath_ = outputPath;
      receiver->estimatedTimeLabel_ = estimatedTimeLabel;
      receiver->resultWeightLabel_ = resultWeightLabel;
      receiver->resultPlateLabel_ = resultPlateLabel;
      receiver->resultPlateIndex_ = receiver->plateIndexForPrintId(targetPrintId);
      receiver->resultPlatePrintId_ = targetPrintId;
      receiver->resultFilamentLabel_ = resultFilamentLabel;
      receiver->resultCostLabel_ = resultCostLabel;
      receiver->resultLayerCount_ = layerCount;

      // Store per-plate result for multi-plate tracking by stable print identity.
      if (receiver->plateIndexForPrintId(targetPrintId) >= 0) {
        PlateSliceResult pr;
        pr.estimatedTimeLabel = estimatedTimeLabel;
        pr.resultWeightLabel = resultWeightLabel;
        pr.resultFilamentLabel = resultFilamentLabel;
        pr.resultCostLabel = resultCostLabel;
        pr.outputPath = outputPath;
        pr.resultLayerCount = receiver->resultLayerCount_;
        pr.totalFilamentMm = receiver->resultTotalFilamentMm();
        pr.source = int(ResultSource::ModelSlice);
        receiver->storePlateResult(targetPrintId, pr);
      }
      receiver->activeTargetPrintId_ = -1;
      qInfo("[SliceService] slice finished plate=%d output=%s layers=%d",
            resultPlateIndex,
            outputPath.toUtf8().constData(),
            layerCount);
      emit receiver->progressChanged();
      emit receiver->progressUpdated(100, receiver->statusLabel_);
      // Wave 3b (perf): in-memory preview FIRST — before resultChanged /
      // sliceFinished so the sync handlers see m_directPreviewValid and skip
      // the exported-file re-parse.
      if (!capturedPreview.moves.isEmpty())
        emit receiver->slicePreviewReady(capturedPreview);
      emit receiver->resultChanged();
      // Phase 239 (ENGN-03): surface the non-fatal validate warning BEFORE
      // sliceFinished so the notification is queued ahead of the completion
      // handling (upstream pushes the validate-warning notification from
      // validate_current_plate, Plater.cpp:13749).
      if (!validationWarningText.trimmed().isEmpty())
        emit receiver->validateWarning(validationWarningText.trimmed());
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
      emit receiver->sequentialPrintClearanceReady(capturedClearance);
      // P2.3-3: deliver the captured-by-value support preview polylines (same
      // success-branch gate as the other captures). A non-valid payload (no
      // supports generated) tells the ViewModel to drop any stale preview.
      qInfo("[SliceService] support-preview ready valid=%d segments=%d",
            capturedSupportPreview.valid ? 1 : 0,
            capturedSupportPreview.segmentCount);
      emit receiver->supportPreviewReady(capturedSupportPreview);
    }, Qt::QueuedConnection);
  }, Qt::QueuedConnection);
  if (!posted)
  {
    slicing_ = false;
    activeCancelFlag_.reset();
    activeTargetPrintId_ = -1;
    sliceState_ = State::Error;
    statusLabel_ = QStringLiteral("Failed to dispatch the slice worker");
    emit slicingChanged();
    emit progressChanged();
    emit stateChanged();
    emit sliceStateChanged();
    emit sliceFailed(statusLabel_);
    return;
  }
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
  emit sliceStateChanged();
}

bool SliceService::loadGCodeFromPrevious(const QString &gcodeFilePath)
{
  if (slicing_ || exportActive_)
    return false;

  const QFileInfo info(gcodeFilePath);
  const QString localPath = info.absoluteFilePath();
  const int targetPlateIndex = projectService_ ? projectService_->currentPlateIndex() : -1;
  const int targetPrintId = printIdForPlateIndex(targetPlateIndex);
  activeTargetPrintId_ = targetPrintId;
  // Phase 239 (ENGN-02): keep the previous per-plate labels (weight/filament/
  // cost/layer count). The reuse re-reads the SAME file the metadata was
  // computed from (upstream keeps gcode_result statistics alive on the
  // finished() branch, BackgroundSlicingProcess.cpp:199-221), so the labels
  // survive the re-store below; only the estimated time is re-derived fresh
  // from the parsed file.
  const PlateSliceResult previousMeta = plateResults_.value(targetPrintId);
  removePlateResultByPrintId(targetPrintId);
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
  emit sliceStateChanged();
    emit sliceFailed(statusLabel_);
      activeTargetPrintId_ = -1;
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

  QtConcurrent::run([receiver, cancelFlag, localPath, targetPlateIndex, targetPrintId, targetPlateLabel, previousMeta]()
                    {
    QString errorText;
    QString estimatedTimeLabel;

#ifdef HAS_LIBSLIC3R
    try
    {
      Slic3r::Print print;
      Slic3r::GCodeProcessorResult result;
      print.export_gcode_from_previous_file(localPath.toStdString(), &result);
      // Phase 239 (ENGN-02): the reused file IS the statistics source
      // (same readback startSlice uses; the parsed time survives the reuse
      // instead of collapsing to an empty label).
      estimatedTimeLabel = formatDurationLabel(result.print_statistics.modes[0].time);
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

    QMetaObject::invokeMethod(receiver, [receiver, cancelFlag, localPath, errorText, estimatedTimeLabel, targetPlateIndex, targetPrintId, targetPlateLabel, previousMeta]() {
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
        // Clearing the active result resets the display state; cancellation
        // remains the terminal state for this worker completion.
        receiver->sliceState_ = State::Cancelled;
        receiver->statusLabel_ = QObject::tr("Slicing cancelled");
        emit receiver->sequentialPrintClearanceReady(SequentialPrintClearance{});
        qInfo("[SliceService] slice cancelled plate=%d reason=%s",
              targetPlateIndex,
              receiver->statusLabel_.toUtf8().constData());
        emit receiver->progressChanged();
        emit receiver->resultChanged();
        emit receiver->sliceResultCleared();
        emit receiver->stateChanged();
        emit receiver->sliceFailed(receiver->statusLabel_);
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
        return;
      }

      if (receiver->plateIndexForPrintId(targetPrintId) < 0)
      {
        receiver->clearStoredResult();
        receiver->activeTargetPrintId_ = -1;
        emit receiver->resultChanged();
        emit receiver->sliceResultCleared();
        emit receiver->stateChanged();
        emit receiver->sliceStateChanged();
        return;
      }

      receiver->sliceState_ = State::Completed;
      receiver->progress_ = 100;
      receiver->statusLabel_ = QObject::tr("Existing G-code reuse complete");
      receiver->outputPath_ = localPath;
      receiver->estimatedTimeLabel_ = estimatedTimeLabel.isEmpty() ? previousMeta.estimatedTimeLabel : estimatedTimeLabel;
      receiver->resultPlateLabel_ = targetPlateLabel;
      receiver->resultPlateIndex_ = receiver->plateIndexForPrintId(targetPrintId);
      receiver->resultPlatePrintId_ = targetPrintId;
      receiver->resultWeightLabel_ = previousMeta.resultWeightLabel;
      receiver->resultFilamentLabel_ = previousMeta.resultFilamentLabel;
      receiver->resultCostLabel_ = previousMeta.resultCostLabel;
      receiver->resultLayerCount_ = previousMeta.resultLayerCount;
      if (receiver->plateIndexForPrintId(targetPrintId) >= 0)
      {
        PlateSliceResult pr;
        pr.estimatedTimeLabel = receiver->estimatedTimeLabel_;
        pr.resultWeightLabel = previousMeta.resultWeightLabel;
        pr.resultFilamentLabel = previousMeta.resultFilamentLabel;
        pr.resultCostLabel = previousMeta.resultCostLabel;
        pr.outputPath = localPath;
        pr.resultLayerCount = previousMeta.resultLayerCount;
        pr.totalFilamentMm = previousMeta.totalFilamentMm;
        pr.source = int(ResultSource::PreviousGCode);
        receiver->storePlateResult(targetPrintId, pr);
      }
      receiver->activeTargetPrintId_ = -1;
      emit receiver->progressChanged();
      emit receiver->progressUpdated(100, receiver->statusLabel_);
      emit receiver->resultChanged();
      emit receiver->sliceFinished(receiver->estimatedTimeLabel_);
    }, Qt::QueuedConnection); });

  return true;
}

void SliceService::startSlicePlate(int plateIndex)
{
  if (slicing_)
    return;
  if (!projectService_ || !projectService_->setCurrentPlateIndex(plateIndex))
  {
    sliceState_ = State::Error;
    statusLabel_ = QStringLiteral("Invalid plate selection");
    emit stateChanged();
    emit sliceStateChanged();
    emit sliceFailed(statusLabel_);
    return;
  }
  if (!projectService_->isPlateReadyForSlice(plateIndex))
  {
    sliceState_ = State::Error;
    statusLabel_ = QStringLiteral("Selected plate is outside the printable area");
    emit stateChanged();
    emit sliceStateChanged();
    emit sliceFailed(statusLabel_);
    return;
  }
  startSlice(projectService_->projectName());
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
  const int printId = printIdForPlateIndex(plateIndex);
  const auto it = plateResults_.constFind(printId);
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
  // B3 (upstream MainFrame.cpp:1897-1901 -> PartPlate.cpp:5022-5044): all-plate
  // file export requires every non-empty printable plate to hold a valid slice
  // result with printable, fully-placed instances, and at least one ready plate.
  if (!projectService_ || !projectService_->isAllSliceResultReadyForExport())
  {
    setExportStatus(State::Completed, progress_,
                    QObject::tr("Not all plates are ready for export"));
    logExportFailure(QStringLiteral("all"),
                     QString{},
                     directoryPath,
                     statusLabel_);
    emit exportFailed(statusLabel_);
    return false;
  }

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

  if (slicing_ || exportActive_)
  {
    const QString reason = slicing_
        ? QObject::tr("Cannot export while slicing")
        : QObject::tr("A G-code export is already in progress");
    logExportFailure(QStringLiteral("all"), QString{}, cleanDirectory, reason);
    emit exportFailed(reason);
    return false;
  }

  QString stem = sanitizeFileStem(baseName);
  if (baseName.trimmed().isEmpty())
  {
    stem = sanitizeFileStem(projectService_ ? projectService_->projectName() : QString{});
    if (stem == QStringLiteral("output") && projectService_)
      stem = sanitizeFileStem(QFileInfo(projectService_->sourceFilePath()).completeBaseName());
  }

  // Phase 239 (ENGN-03): snapshot the plate list on the GUI thread (locked /
  // non-printable plates and empty outputs are filtered here, reading only
  // GUI-thread members); the worker then copies the files sequentially on the
  // QtConcurrent thread -- one worker for the whole batch, mirroring upstream
  // BackgroundSlicingProcess exporting plates one by one in the background.
  struct PlateExportJob
  {
    QString sourceAbs;
    QString targetAbs;
  };
  QList<PlateExportJob> jobs;
  qInfo("[SliceService] export all start dir=%s base=%s storedPlates=%d",
        dir.absolutePath().toUtf8().constData(),
        stem.toUtf8().constData(),
        int(plateResults_.size()));
  for (auto it = plateResults_.constBegin(); it != plateResults_.constEnd(); ++it)
  {
    const int printId = it.key();
    const int plateIndex = plateIndexForPrintId(printId);
    if (plateIndex < 0)
      continue;
    if (projectService_ && (projectService_->isPlateLocked(plateIndex) || !projectService_->isPlatePrintable(plateIndex)))
      continue;
    if (it->outputPath.isEmpty())
      continue;

    PlateExportJob job;
    job.sourceAbs = it->outputPath;
    job.targetAbs = dir.filePath(ensureGcodeSuffix(QStringLiteral("%1_plate%2").arg(stem).arg(plateIndex + 1)));
    jobs.append(job);
  }

  if (jobs.isEmpty())
  {
    setExportStatus(State::Completed, progress_, QObject::tr("No sliced plates to export"));
    logExportFailure(QStringLiteral("all"),
                     QString{},
                     cleanDirectory,
                     statusLabel_);
    emit exportFailed(statusLabel_);
    return false;
  }

  activeExportCancelFlag_ = std::make_shared<std::atomic_bool>(false);
  exportActive_ = true;
  const quint64 generation = ++exportGeneration_;
  const QPointer<SliceService> receiver(this);
  const auto cancelFlag = activeExportCancelFlag_;
  const int totalJobs = jobs.size();

  setExportStatus(State::Exporting, 0, QObject::tr("Exporting G-code"));
  emit exportStarted(statusLabel_);

  QtConcurrent::run([receiver, cancelFlag, jobs, totalJobs, generation]() {
    QString failureReason;
    QString failureSource;
    QString failureTarget;
    bool cancelled = false;
    int doneJobs = 0;
    QStringList exportedPaths;

    for (const PlateExportJob &job : jobs)
    {
      const qint64 sourceBytes = QFileInfo(job.sourceAbs).size();
      const auto onProgress = [receiver, doneJobs, totalJobs](int percent) {
        if (!receiver)
          return;
        // Scale the per-file percent into the whole-batch range so the
        // progress bar advances monotonically across plates.
        const int overall = (doneJobs * 100 + percent) / totalJobs;
        QMetaObject::invokeMethod(receiver, [receiver, overall]() {
          if (!receiver)
            return;
          receiver->progress_ = overall;
          emit receiver->progressChanged();
          emit receiver->progressUpdated(overall, receiver->statusLabel_);
        }, Qt::QueuedConnection);
      };

      const ExportCopyOutcome outcome = copyGcodeChunked(job.sourceAbs, job.targetAbs, sourceBytes, cancelFlag, onProgress);
      if (outcome.cancelled)
      {
        cancelled = true;
        failureReason = outcome.error;
        failureSource = job.sourceAbs;
        failureTarget = job.targetAbs;
        break;
      }
      if (!outcome.ok)
      {
        failureReason = outcome.error;
        failureSource = job.sourceAbs;
        failureTarget = job.targetAbs;
        break;
      }
      exportedPaths.append(outcome.finalPath);
      ++doneJobs;
    }

    if (!receiver)
      return;

    QMetaObject::invokeMethod(receiver, [receiver, cancelled, failureReason, failureSource,
                                         failureTarget, exportedPaths, generation]() {
      if (!receiver || receiver->exportGeneration_ != generation)
        return;

      receiver->exportActive_ = false;
      receiver->activeExportCancelFlag_.reset();

      if (cancelled || !failureReason.isEmpty())
      {
        receiver->setExportStatus(cancelled ? State::Cancelled : State::Completed,
                                  receiver->progress_,
                                  failureReason);
        logExportFailure(QStringLiteral("all"), failureSource, failureTarget, failureReason);
        emit receiver->exportFailed(receiver->statusLabel_);
        return;
      }

      for (const QString &path : exportedPaths)
        emit receiver->exportFinished(path);
      receiver->setExportStatus(State::Completed, 100, QObject::tr("Exported all sliced plates"));
    }, Qt::QueuedConnection);
  });

  return true;
}

void SliceService::setExportStatus(State state, int progress, const QString &label)
{
  sliceState_ = state;
  progress_ = qBound(0, progress, 100);
  statusLabel_ = label;
  emit progressChanged();
  emit stateChanged();
  emit sliceStateChanged();
}

bool SliceService::exportSourceToPath(const QString &sourcePath, const QString &targetPath, const QString &displayName)
{
  const auto failExport = [this, &sourcePath, &targetPath](const QString &reason) {
    if (!slicing_ && !exportActive_)
      setExportStatus(sliceState_, progress_, reason);
    logExportFailure(QStringLiteral("single"), sourcePath, targetPath, reason);
    emit exportFailed(reason);
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

  if (slicing_)
    return failExport(QObject::tr("Cannot export while slicing"));
  if (exportActive_)
    return failExport(QObject::tr("A G-code export is already in progress"));

  // Phase 239 (ENGN-03): the copy itself runs on a QtConcurrent worker so a
  // multi-hundred-MB G-code never freezes the GUI thread (upstream runs the
  // export inside BackgroundSlicingProcess's background thread). Progress is
  // delivered through the queued progressUpdated signal; completion through
  // exportFinished / exportFailed. cancelExport() flips the shared flag and
  // the worker aborts at the next 1 MiB chunk boundary.
  activeExportCancelFlag_ = std::make_shared<std::atomic_bool>(false);
  exportActive_ = true;
  const quint64 generation = ++exportGeneration_;
  const QPointer<SliceService> receiver(this);
  const auto cancelFlag = activeExportCancelFlag_;
  const QString sourceAbs = srcInfo.absoluteFilePath();
  const QString targetAbs = targetInfo.absoluteFilePath();
  const qint64 totalBytes = srcInfo.size();

  setExportStatus(State::Exporting, 0, QObject::tr("Exporting G-code"));
  emit exportStarted(statusLabel_);

  QtConcurrent::run([receiver, cancelFlag, sourceAbs, targetAbs, displayName, totalBytes, generation]() {
    const auto onProgress = [receiver](int percent) {
      if (!receiver)
        return;
      QMetaObject::invokeMethod(receiver, [receiver, percent]() {
        if (!receiver)
          return;
        receiver->progress_ = percent;
        emit receiver->progressChanged();
        emit receiver->progressUpdated(percent, receiver->statusLabel_);
      }, Qt::QueuedConnection);
    };

    const ExportCopyOutcome outcome = copyGcodeChunked(sourceAbs, targetAbs, totalBytes, cancelFlag, onProgress);

    if (!receiver)
      return;

    QMetaObject::invokeMethod(receiver, [receiver, outcome, sourceAbs, targetAbs, displayName, generation]() {
      if (!receiver || receiver->exportGeneration_ != generation)
        return;

      receiver->exportActive_ = false;
      receiver->activeExportCancelFlag_.reset();

      if (outcome.cancelled)
      {
        receiver->setExportStatus(State::Cancelled, receiver->progress_, outcome.error);
        logExportFailure(QStringLiteral("single"), sourceAbs, targetAbs, outcome.error);
        emit receiver->exportFailed(receiver->statusLabel());
        return;
      }

      if (!outcome.ok)
      {
        receiver->setExportStatus(State::Completed, receiver->progress_, outcome.error);
        logExportFailure(QStringLiteral("single"), sourceAbs, targetAbs, outcome.error);
        emit receiver->exportFailed(receiver->statusLabel_);
        return;
      }

      const QFileInfo finalInfo(outcome.finalPath);
      const QString exportedName = finalInfo.fileName().isEmpty() ? displayName : finalInfo.fileName();
      receiver->setExportStatus(State::Completed, 100, QObject::tr("Exported: %1").arg(exportedName));
      qInfo("[SliceService] export finished source=%s target=%s bytes=%lld",
            sourceAbs.toUtf8().constData(),
            outcome.finalPath.toUtf8().constData(),
            static_cast<long long>(outcome.finalBytes));
      emit receiver->exportFinished(outcome.finalPath);
    }, Qt::QueuedConnection);
  });

  return true;
}

void SliceService::cancelExport()
{
  if (!exportActive_ || !activeExportCancelFlag_)
    return;

  activeExportCancelFlag_->store(true);
  statusLabel_ = QObject::tr("Cancelling G-code export");
  emit progressChanged();
  emit stateChanged();
  emit sliceStateChanged();
}

bool SliceService::hasPlateResult(int plateIndex) const
{
  const auto it = plateResults_.constFind(printIdForPlateIndex(plateIndex));
  if (it == plateResults_.constEnd() || it->outputPath.isEmpty())
    return false;
  return QFileInfo::exists(it->outputPath);
}

QString SliceService::plateEstimatedTime(int plateIndex) const
{
  auto it = plateResults_.constFind(printIdForPlateIndex(plateIndex));
  return it != plateResults_.constEnd() ? it->estimatedTimeLabel : QString();
}

QString SliceService::plateWeight(int plateIndex) const
{
  auto it = plateResults_.constFind(printIdForPlateIndex(plateIndex));
  return it != plateResults_.constEnd() ? it->resultWeightLabel : QString();
}

QString SliceService::plateFilament(int plateIndex) const
{
  auto it = plateResults_.constFind(printIdForPlateIndex(plateIndex));
  return it != plateResults_.constEnd() ? it->resultFilamentLabel : QString();
}

QString SliceService::plateCost(int plateIndex) const
{
  auto it = plateResults_.constFind(printIdForPlateIndex(plateIndex));
  return it != plateResults_.constEnd() ? it->resultCostLabel : QString();
}

int SliceService::plateLayerCount(int plateIndex) const
{
  auto it = plateResults_.constFind(printIdForPlateIndex(plateIndex));
  return it != plateResults_.constEnd() ? it->resultLayerCount : 0;
}

QString SliceService::plateOutputPath(int plateIndex) const
{
  auto it = plateResults_.constFind(printIdForPlateIndex(plateIndex));
  return it != plateResults_.constEnd() ? it->outputPath : QString();
}

int SliceService::plateResultSource(int plateIndex) const
{
  auto it = plateResults_.constFind(printIdForPlateIndex(plateIndex));
  return it != plateResults_.constEnd() ? it->source : int(ResultSource::None);
}

bool SliceService::activatePlateResult(int plateIndex)
{
  if (slicing_)
    return false;

  const int printId = printIdForPlateIndex(plateIndex);
  const auto it = plateResults_.constFind(printId);
  if (it == plateResults_.constEnd() || it->outputPath.isEmpty() || !QFileInfo::exists(it->outputPath))
  {
    if (resultPlateIndex_ != -1 || !outputPath_.isEmpty())
    {
      clearStoredResult();
      emit resultChanged();
      emit sliceResultCleared();
      emit stateChanged();
  emit sliceStateChanged();
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
  resultPlatePrintId_ = printId;
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
  emit sliceStateChanged();
  return true;
}

void SliceService::clearPlateResults()
{
  clearStoredResult();
  plateResults_.clear();
  setAllDomainResultsValid(false);
  emit resultChanged();
  emit sliceResultCleared();
  emit stateChanged();
  emit sliceStateChanged();
}

void SliceService::removePlateResult(int plateIndex)
{
  const int printId = printIdForPlateIndex(plateIndex);
  if (printId < 0)
    return;
  const bool removedPlateMetadata = plateResults_.remove(printId) > 0;
  if (removedPlateMetadata)
    setDomainResultValid(printId, false);
  bool clearedCurrentOutput = false;
  if (resultPlatePrintId_ == printId || resultPlateIndex_ == plateIndex)
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
  emit sliceStateChanged();
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

