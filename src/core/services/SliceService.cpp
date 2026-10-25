#include "SliceService.h"

#include "core/services/ProjectService.h"
#include "core/services/AppSettingsService.h"

#include <QMetaObject>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QStringList>
#include <QFileInfo>
#include <QDateTime>
#include <QFile>
#include <QThread>
#include <QSaveFile>
#include <QUrl>
#include <functional>
#include <mutex>
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
      // AXES-ROT: scene Z negates the slic3r depth axis.
      out.push_back(float(Slic3r::unscaled<double>(a.x())));
      out.push_back(y);
      out.push_back(-float(Slic3r::unscaled<double>(a.y())));
      out.push_back(float(Slic3r::unscaled<double>(b.x())));
      out.push_back(y);
      out.push_back(-float(Slic3r::unscaled<double>(b.y())));
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
        // AXES-ROT: scene Z negates the slic3r depth axis.
        out.push_back(float(Slic3r::unscaled<double>(point->x())));
        out.push_back(y);
        out.push_back(-float(Slic3r::unscaled<double>(point->y())));
      }
    }
  }

  // P15.11: moved to SliceService::packSequentialClearance (public static) so
  // the drag-time preview packs through the SAME stream builder.

  // ── P4 (step profile, docs/开发计划_2026-10-10.md §6d) ──────────────────
  // Trailing sentinel key of the step timeline: recorded when the worker's
  // success path finishes. It carries no step semantics; PerfBench drops it
  // from the reported profile and uses its firstAtMs as the end anchor so
  // the LAST real step (gcode_export) gets a visible duration too.
  constexpr const char *kStepTimelineDoneKey = "done";

  /// Maps a raw engine set_status text to the §6d step key. The TEXT is the
  /// authority (percent is ambiguous across steps); prefix rules also cover
  /// the parameterized per-layer/per-object progress texts. An unmapped text
  /// becomes "unknown_<sanitized>" so an engine wording change (upstream
  /// submodule bump) surfaces as a new key and turns the gate red -- that is
  /// the designed re-gold trigger, not a defect. Texts are the canonical
  /// English literals: the engine routes them through
  /// Slic3r::I18N::translate, which is identity unless a translate callback
  /// is installed, and no process in this repo installs one.
  QString stepKeyForEngineText(const std::string &rawText)
  {
    const QString text = QString::fromUtf8(rawText.c_str());
    // Ordered prefix rules (printable texts first, spammy parameterized
    // variants after). Sources: PrintObjectSlice.cpp:829, PrintObject.cpp:
    // 465/568/801/862/894/925/955/985/1018-1054, Print.cpp:2624/2943,
    // TreeSupport.cpp:1728-1764/2747, TreeSupport3D.cpp:3546/3601,
    // GCode.cpp:4280/4383 (vendored third_party/OrcaSlicer).
    static const std::vector<std::pair<const char *, const char *>> kPrefixRules = {
        {"Slicing mesh", "mesh_slicing"},
        {"Generating walls", "perimeters"},
        {"Generating infill regions", "infill_regions"},
        {"Generating infill toolpath", "infill_toolpath"},
        {"Z contouring", "z_contouring"},
        {"Z contoured layer", "z_contouring"},
        {"Detect overhangs for auto-lift", "overhang_lift"},
        {"Checking support necessity", "support"},
        {"Support:", "support"},
        {"Generating support", "support"},
        {"Optimizing toolpath", "path_optimization"},
        {"Generating skirt & brim", "skirt_brim"},
        {"Generating G-code", "gcode_export"},
        {"Exporting G-code", "gcode_export"},
    };
    for (const auto &rule : kPrefixRules)
    {
      if (text.startsWith(QLatin1String(rule.first)))
        return QLatin1String(rule.second);
    }
    // unknown_* fallback: sanitize the raw text into a stable diagnostic key.
    QString sanitized;
    sanitized.reserve(qMin<qsizetype>(text.size(), 48));
    for (const QChar ch : text.left(48))
    {
      const ushort u = ch.unicode();
      if ((u >= 'a' && u <= 'z') || (u >= 'A' && u <= 'Z') || (u >= '0' && u <= '9'))
        sanitized.append(ch.toLower());
      else if (!sanitized.endsWith(QLatin1Char('_')))
        sanitized.append(QLatin1Char('_'));
    }
    while (sanitized.endsWith(QLatin1Char('_')))
      sanitized.chop(1);
    return QStringLiteral("unknown_") + sanitized;
  }

  /// One slice worker activation's step profiler. Owns the timeline storage
  /// behind a shared_ptr so a callback captured by the PERSISTENT Print can
  /// never dangle: the print keeps the previous activation's callback until
  /// the next slice re-registers set_status_callback, and the shared_ptr
  /// keeps both recorder and storage alive for that window. record() is
  /// mutex-guarded because set_status fires from TBB parallel sections
  /// (Z-contour parallel_for, tree-support contact/branch phases) as well as
  /// the worker thread, and dedupes by key (first occurrence + count) to
  /// strip per-layer progress spam.
  struct StepTimelineRecorder
  {
    QElapsedTimer timer;
    std::shared_ptr<std::vector<SliceStepTiming>> sink;
    std::mutex mutex;

    explicit StepTimelineRecorder(
        std::shared_ptr<std::vector<SliceStepTiming>> timelineSink)
        : sink(std::move(timelineSink))
    {
      timer.start();
    }

    void record(const QString &key, const QString &text, int percent)
    {
      std::lock_guard<std::mutex> lock(mutex);
      for (SliceStepTiming &entry : *sink)
      {
        if (entry.key == key)
        {
          ++entry.count;
          return;
        }
      }
      sink->push_back(SliceStepTiming{key, text, percent, timer.elapsed(), 1});
    }
  };
#endif

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

SliceService::SliceService(ProjectService *projectService, QObject *parent)
    : QObject(parent), projectService_(projectService)
{
  // PLATE-PRINT-LIFECYCLE (batch 2): a deleted plate releases its persistent
  // Print/model slot immediately (printIndex identities are never reused, so
  // the entry could never re-validate) and its stored G-code result entry.
  // The GUI-thread invalidateAllSliceResults flow on top of deletePlate keeps
  // the rest of the result store in sync.
  if (projectService_)
  {
    connect(projectService_, &ProjectService::plateRemoved, this, [this](int printIndex) {
#ifdef HAS_LIBSLIC3R
      releasePersistentSlice(printIndex);
#endif
      if (printIndex >= 0 && plateResults_.remove(printIndex) > 0)
        emit resultChanged();
    });
  }
}

SliceService::~SliceService()
{
  // PLATE-PRINT-LIFECYCLE (batch 2) destruction order (design resolution of
  // review P0-2, docs/代码评审报告_2026-09-04.md section 4):
  //  1. Cancel first: flip the flag and cancel the active Print so a running
  //     Print::process throws at the next step boundary (PrintBase state
  //     machine calls throw_if_canceled on every step start/done,
  //     PrintBase.hpp:150-186 + 542). The load-then-cancel pair is safe
  //     since review P1-1: the worker no longer destroys the Print -- a
  //     cancelled/failed slot is carried into the queued completion lambda,
  //     so the Print outlives this load/cancel race window.
  //  2. Join the worker via its QFuture BEFORE any member is destroyed. This
  //     is a plain blocking wait on the QtConcurrent task (no owned QThread,
  //     so the qFatal wait()-timeout failure mode of the perf/waves WIP
  //     cannot happen) and it guarantees the worker -- the single owner of
  //     the in-flight PersistentSlice -- finishes touching this service's
  //     state and posts its completion before teardown continues. Idle slots
  //     in persistentSlices_ then die here on the GUI thread.
  //     Review P2-4: ALL workers are tracked now -- the slice worker plus
  //     the previous-G-code, export-all, and export-copy auxiliary workers
  //     each carry a stored future that the destructor cancels (flags) and
  //     joins below. No worker class relies on call-site discipline.
  //  Review P1-2 (BackendContext): the context destructor deletes the slice
  //  service FIRST so the joined worker cannot touch an already-destroyed
  //  ProjectService (Qt would otherwise destroy children in creation
  //  order).
  if (activeCancelFlag_)
    activeCancelFlag_->store(true);
  if (activeExportCancelFlag_)
    activeExportCancelFlag_->store(true);
#ifdef HAS_LIBSLIC3R
  if (Slic3r::Print *active = activePrint_.load(std::memory_order_acquire))
    active->cancel();
#endif
  // Review P2-4: join EVERY worker this service ever launched -- the slice
  // worker plus the three auxiliary ones -- so none can touch this object
  // (or, via BackendContext ~dtor ordering, the project service) after its
  // owner began teardown.
  activeSliceFuture_.waitForFinished();
  previousGcodeFuture_.waitForFinished();
  exportAllFuture_.waitForFinished();
  exportCopyFuture_.waitForFinished();
}

int SliceService::persistentSliceCount() const
{
#ifdef HAS_LIBSLIC3R
  return int(persistentSlices_.size());
#else
  return 0;
#endif
}

qint64 SliceService::persistentSliceReuseCount() const
{
#ifdef HAS_LIBSLIC3R
  return persistentSliceReuseCount_;
#else
  return 0;
#endif
}

qint64 SliceService::persistentSliceRebuildCount() const
{
#ifdef HAS_LIBSLIC3R
  return persistentSliceRebuildCount_;
#else
  return 0;
#endif
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

void SliceService::setDomainResultValid(int plateIndex, bool valid)
{
  // B3: mirror result-store changes into the PartPlate domain flag so the
  // all-plates print/export aggregates (PartPlate.cpp:4989-5044) act on the
  // same truth this service exposes.
  if (projectService_)
    projectService_->setPlateSliceResultValid(plateIndex, valid);
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
  resultFilamentLabel_.clear();
  resultLayerCount_ = 0;
  resultCostLabel_.clear();
#ifdef HAS_LIBSLIC3R
  // P1: the direct-hand-off payload belongs to the stored result -- drop it
  // with the rest (the delivery id keeps increasing so a stale queued sync
  // can never resurrect it).
  activeProcessorResult_.reset();
#endif
  emit progressChanged();
}

void SliceService::clearActiveTargetResult()
{
  if (activeTargetPlateIndex_ >= 0)
  {
    const int resultKey = activeTargetResultKey_ >= 0
        ? activeTargetResultKey_ : resultKeyForPlateIndex(activeTargetPlateIndex_);
    plateResults_.remove(resultKey);
    const int currentPlateIndex = plateIndexForResultKey(resultKey);
    if (currentPlateIndex >= 0)
      setDomainResultValid(currentPlateIndex, false);
  }
  if (resultPlateIndex_ == activeTargetPlateIndex_)
    clearStoredResult();
  activeTargetResultKey_ = -1;
}

int SliceService::resultKeyForPlateIndex(int plateIndex) const
{
  if (projectService_) {
    const int stableId = projectService_->platePrintIndex(plateIndex);
    if (stableId >= 0)
      return stableId;
  }
  return plateIndex;
}

int SliceService::plateIndexForResultKey(int resultKey) const
{
  if (projectService_) {
    const int plateIndex = projectService_->plateIndexForPrintIndex(resultKey);
    if (plateIndex >= 0)
      return plateIndex;
  }
  return resultKey;
}

const PlateSliceResult *SliceService::resultForPlateIndex(int plateIndex) const
{
  const auto it = plateResults_.constFind(resultKeyForPlateIndex(plateIndex));
  return it == plateResults_.constEnd() ? nullptr : &it.value();
}

bool SliceService::removeResultForPlateIndex(int plateIndex)
{
  return plateResults_.remove(resultKeyForPlateIndex(plateIndex)) > 0;
}

void SliceService::storePlateResultForKey(int resultKey, const PlateSliceResult &result)
{
  if (resultKey >= 0)
    plateResults_[resultKey] = result;
}

void SliceService::storePlateResult(int plateIndex, const PlateSliceResult &result)
{
  if (plateIndex >= 0)
  {
    storePlateResultForKey(resultKeyForPlateIndex(plateIndex), result);
    setDomainResultValid(plateIndex, true);
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
  activeTargetPlateIndex_ = -1;
  activeTargetResultKey_ = -1;
  plateResults_.clear();
  // Review P1-3: project-scoped reset bumps the generation so queued
  // completion lambdas from the discarded context are dropped on arrival.
  ++sliceGeneration_;
#ifdef HAS_LIBSLIC3R
  // PLATE-PRINT-LIFECYCLE (batch 2): clearResults is the project-scoped reset
  // (import, clear workspace, undo/redo, external scene change) -- those flows
  // can mutate anything, so every idle persistent slot is dropped and the next
  // slice of any plate is a conservative full rebuild. In-flight slots are not
  // in the map (single-ownership handoff) and die on their worker path.
  persistentSlices_.clear();
#endif
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
        // owzx-fix (MODEL-BED): printable_area entries are "XxY" strings
        // (3MF project-settings dialect). The generic comma-join deserializer
        // cannot parse that format and threw (silently swallowed), so a model
        // authored for a 350x350 bed never delivered its bed through
        // --load-settings and its prime tower landed outside the default bed.
        // Parse the point pairs here; the bed_shape translation after the
        // plate merge turns this into the engine's bed.
        if (key == "printable_area")
        {
          auto *pointsOpt = dynamic_cast<Slic3r::ConfigOptionPoints *>(opt);
          if (pointsOpt)
          {
            Slic3r::ConfigOptionPoints parsed;
            bool ok = true;
            for (const auto &item : list)
            {
              const QString s = item.toString();
              const qsizetype xIdx = s.indexOf(QLatin1Char('x'));
              if (xIdx <= 0) { ok = false; break; }
              bool okX = false, okY = false;
              const double x = s.left(xIdx).toDouble(&okX);
              const double y = s.mid(xIdx + 1).toDouble(&okY);
              if (!okX || !okY) { ok = false; break; }
              parsed.values.emplace_back(x * 1000.0, y * 1000.0);
            }
            if (ok && parsed.values.size() >= 3)
            {
              config.set_key_value("printable_area",
                                   new Slic3r::ConfigOptionPoints(parsed.values));
              setOk = true;
              break;
            }
          }
        }
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
  activeTargetPlateIndex_ = targetPlateIndex;
  const int targetResultKey = resultKeyForPlateIndex(targetPlateIndex);
  activeTargetResultKey_ = targetResultKey;
  plateResults_.remove(targetResultKey);
  setDomainResultValid(targetPlateIndex, false);
  clearStoredResult();
#ifdef HAS_LIBSLIC3R
  // PLATE-PRINT-LIFECYCLE (batch 2): drop slots of plates pruned through
  // paths that bypass deletePlate before the reuse decision below.
  reconcilePersistentSlices();
#endif
  emit resultChanged();
  emit sliceResultCleared();

  QString targetPlateLabel;
#ifdef HAS_LIBSLIC3R
  std::unique_ptr<Slic3r::Model> modelForSlice;
  // PLATE-PRINT-LIFECYCLE (batch 2) incremental-reslice decision (upstream
  // BackgroundSlicingProcess truth: PartPlate keeps a persistent per-plate
  // Print and re-applies into it, PartPlate.cpp:2839-2840; Print::apply
  // diffs configs/objects against the previous apply and invalidates only
  // the affected steps, PrintApply.cpp:1022+ and Print.cpp:73-353; process()
  // re-runs only invalidated steps -- the PrintBase step state machine skips
  // still-DONE steps, PrintBase.hpp:150-186).
  //
  // Judgment (conservative: prefer a full rebuild over a wrong reuse):
  //  - REUSE the persistent Print+model clone only when the plate geometry
  //    fingerprint is UNCHANGED (mesh did not move/mutate) and no calibration
  //    params are involved on either side (set_calib_params is not modeled by
  //    apply()). Config/preset/bed changes do NOT block reuse: they flow
  //    through Print::apply's diff-based invalidation, exactly like upstream.
  //  - FULL REBUILD (fresh clone + fresh Print) on any geometry change,
  //    missing/stale slot, calibration involvement, cancel or error -- a
  //    cancelled or failed Print state is never reused.
  std::shared_ptr<PersistentSlice> reusedSlice;
  quint64 plateGeometrySignature = 0;
  bool reusePersistentPrint = false;
#endif
  if (projectService_)
  {
    const QStringList plateNames = projectService_->plateNames();
    if (targetPlateIndex >= 0 && targetPlateIndex < plateNames.size() && !plateNames[targetPlateIndex].isEmpty())
      targetPlateLabel = plateNames[targetPlateIndex];
    else if (targetPlateIndex >= 0)
      targetPlateLabel = QObject::tr("Plate %1").arg(targetPlateIndex + 1);
#ifdef HAS_LIBSLIC3R
    // The fingerprint is computed on the GUI thread while the model state is
    // stable (O(objects) metadata walk -- no mesh content traversal).
    plateGeometrySignature = projectService_->currentPlateGeometrySignature();
    const auto slotIt = persistentSlices_.find(targetResultKey);
    if (slotIt != persistentSlices_.end())
    {
      const bool signatureMatch = slotIt->second &&
          slotIt->second->geometrySignature == plateGeometrySignature;
      const bool calibClean = calibConfig_.mode == 0 && !slotIt->second->calibParamsApplied;
      if (signatureMatch && calibClean)
      {
        // Take the slot out of the map: the GUI thread gives up ownership
        // here and does not touch it again until the completion lambda
        // re-inserts it (single-owner handoff, no lock).
        reusedSlice = std::move(slotIt->second);
        reusePersistentPrint = true;
        qInfo("[SliceService] reusing persistent plate print (sig=%llx, print id %d)",
              static_cast<unsigned long long>(plateGeometrySignature),
              targetResultKey);
      }
      else
      {
        qInfo("[SliceService] dropping stale persistent plate print (sig %llx vs %llx, calib %d/%d, print id %d)",
              static_cast<unsigned long long>(slotIt->second ? slotIt->second->geometrySignature : 0ull),
              static_cast<unsigned long long>(plateGeometrySignature),
              int(calibConfig_.mode),
              int(slotIt->second && slotIt->second->calibParamsApplied),
              targetResultKey);
      }
      persistentSlices_.erase(slotIt);
    }
    if (!reusePersistentPrint)
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
    emit sliceStateChanged();
    emit sliceFailed(statusLabel_);
    activeTargetPlateIndex_ = -1;
    activeTargetResultKey_ = -1;
    return;
  }

#ifdef HAS_LIBSLIC3R
  // On the reuse path modelForSlice is intentionally null -- the slot's
  // model already holds the plate objects; only a fresh clone without a
  // reused slot means the plate has no sliceable objects.
  if ((!modelForSlice && !reusedSlice)
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
    activeTargetPlateIndex_ = -1;
    activeTargetResultKey_ = -1;
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

  // Phase 3 (Qt 6.12 modernization): the worker reports its verdict through
  // a QPromise<SliceOutcome> in addition to the queued completion lambda.
  // Every exit (success/failure/cancel) fulfills it, so a copied future can
  // be waited or chained without touching service state. The MOCK
  // simulator's progress rides the same future (setProgressValue); the real
  // engine's progress keeps its queued signal path (already non-blocking).
  QPromise<SliceOutcome> outcomePromise;
  outcomePromise.setProgressRange(0, 100);
  outcomePromise.start();
  lastSliceOutcome_ = outcomePromise.future();

  // PLATE-PRINT-LIFECYCLE (batch 2): keep the future so ~SliceService can
  // cancel-then-join the worker (no orphaned worker can race destruction).
  activeSliceFuture_ = QtConcurrent::run([receiver, cancelFlag, sourcePath, targetPlateIndex, targetResultKey, targetPlateLabel
#ifdef HAS_LIBSLIC3R
                     , modelForSlice = std::move(modelForSlice)
                     // PLATE-PRINT-LIFECYCLE (batch 2): the persistent slot
                     // (null on the full-rebuild path) is handed to THIS
                     // worker with single ownership; the GUI thread will not
                     // touch it until the queued completion lambda runs.
                     , persistentSlot = std::move(reusedSlice)
                     , plateGeometrySignature
#endif
                     // Review P1-3: project reset generation at start time.
                     , generation = receiver ? receiver->sliceGeneration_ : quint64(0)
                     , outcomePromise = std::move(outcomePromise)
                     ]() mutable
                    {
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
    // P4 (step profile): shared storage of this activation's step timeline.
    // Declared for BOTH builds so the completion lambda assigns uniformly;
    // the mock path below never fills it (empty timeline = mock truth).
    auto stepTimeline = std::make_shared<std::vector<SliceStepTiming>>();
#ifdef HAS_LIBSLIC3R
    // P1 (preview direct hand-off): the structured GCodeProcessorResult, born
    // in a shared_ptr and filled in place by Print::export_gcode on the
    // success path (see the export call below). Stays null on cancel/error.
    std::shared_ptr<Slic3r::GCodeProcessorResult> processorResult;
#endif

#ifdef HAS_LIBSLIC3R
    // PLATE-PRINT-LIFECYCLE (batch 2): true when this run reused a persistent
    // Print (reported by the completion lambda for diagnostics only).
    bool reusedPersistentPrint = false;
    try
    {
      // P4 (step profile): one recorder per worker activation. Shared_ptr
      // everywhere: the set_status callback below is stored on the
      // PERSISTENT Print and outlives this try block until the next slice
      // re-registers it, so by-value captures keep it alive (and harmless).
      const auto stepRecorder = std::make_shared<StepTimelineRecorder>(stepTimeline);
      auto notify = [receiver, stepRecorder](const char *stepKey, int progress, const QString &label) {
        // P4: the orchestration points stamp the START of the phase that
        // follows them (model_prep / config_prep / param_apply). The fourth
        // point below stamps "validate" again after print.validate ran; the
        // true validate begin is stamped right before print.validate, and
        // the recorder's by-key dedupe keeps that earlier stamp (count=2).
        stepRecorder->record(QString::fromLatin1(stepKey), label, progress);
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

      // PLATE-PRINT-LIFECYCLE (batch 2): resolve the working Print/model pair
      // from the persistent slot (reuse) or build a fresh slot (full rebuild).
      // The slot leaves this lambda exactly once: on success it is carried
      // into the GUI-thread completion lambda for re-insertion; on cancel or
      // failure it is destroyed HERE on the worker thread so stale engine
      // state can never be resurrected (conservative invalidation).
      if (persistentSlot && (!persistentSlot->print || !persistentSlot->model))
        persistentSlot.reset();  // defensive: a torn slot is a full rebuild
      if (!persistentSlot)
      {
        persistentSlot = std::make_shared<PersistentSlice>();
        persistentSlot->model = std::move(modelForSlice);
        persistentSlot->print = std::make_unique<Slic3r::Print>();
        persistentSlot->geometrySignature = plateGeometrySignature;
      }
      else
      {
        reusedPersistentPrint = true;
      }
      Slic3r::Model *modelForSlicePtr = persistentSlot->model.get();
      Slic3r::Print &print = *persistentSlot->print;
      receiver->activePrint_.store(&print, std::memory_order_release);

      notify("model_prep", 2, QObject::tr("Preparing current plate model"));
      if (!modelForSlicePtr || modelForSlicePtr->objects.empty())
        throw std::runtime_error("Current plate has no sliceable objects");
      {
        int totalVolumes = 0;
        int totalInstances = 0;
        for (const auto *obj : modelForSlicePtr->objects) {
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

      notify("config_prep", 10, QObject::tr("Preparing slice parameters"));
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

      // owzx-fix (MODEL-BED): 3MF project settings carry "printable_area",
      // not "bed_shape" — models authored for a different bed than the
      // machine preset (e.g. 350x350 Creality models on the SPARKX 220
      // preset) kept the preset bed, so prime-tower/wipe-tower positions
      // from the model fell outside the bed and slice validation failed.
      // The loaded model's own bed wins; without it the preset bed stands.
      if (const auto *printableArea =
              config.option<Slic3r::ConfigOptionPoints>("printable_area", false)) {
        if (printableArea->size() >= 3)
          config.set_key_value("bed_shape",
                               new Slic3r::ConfigOptionPoints(printableArea->values));
      }

      // owzx-fix (CP-DIALECT-DEFAULT): multicolor_method follows upstream
      // Orca behavior — upstream carries no guard and emits toolchange
      // macros whenever toolchanges occur. When neither the preset nor the
      // model set the key explicitly, derive it from the filament count
      // (>1 filament → toolchange templates evaluate, matching upstream's
      // unguarded behavior) instead of defaulting to false, which silently
      // dropped the CP toolchange block for multicolor models.
      {
        bool explicitValue = false;
        if (receiver && !receiver->mergedPresetConfig_.isEmpty() &&
            receiver->mergedPresetConfig_.contains(
                QStringLiteral("multicolor_method")))
          explicitValue = true;
        if (!explicitValue && receiver && receiver->projectService_) {
          const int plateIdx2 = targetPlateIndex >= 0
                                    ? targetPlateIndex
                                    : receiver->projectService_->currentPlateIndex();
          if (plateIdx2 >= 0) {
            if (const Slic3r::DynamicPrintConfig *plateCfg2 =
                    receiver->projectService_->plateDynamicConfig(plateIdx2))
              explicitValue = plateCfg2->has("multicolor_method");
          }
        }
        if (!explicitValue) {
          const auto *filamentDiameters =
              config.option<Slic3r::ConfigOptionFloats>("filament_diameter", false);
          config.set_key_value(
              "multicolor_method",
              new Slic3r::ConfigOptionBool(filamentDiameters &&
                                           filamentDiameters->size() > 1));
        }
      }

      restoreGenericEnumMaps(config);

      print.set_status_callback([receiver, stepRecorder](const Slic3r::PrintBase::SlicingStatus &st) {
        // P4 (step profile): wall-clock step stamping from the engine
        // callback. The normalized TEXT is the key authority; record() is
        // mutex-guarded (this callback may fire from TBB parallel support /
        // Z-contour sections) and dedupes by key so per-layer progress
        // spam becomes a count instead of timeline noise.
        if (!st.text.empty())
          stepRecorder->record(stepKeyForEngineText(st.text),
                               QString::fromStdString(st.text), st.percent);
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

      notify("param_apply", 18, QObject::tr("Applying slice parameters"));

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

      // PLATE-PRINT-LIFECYCLE (batch 2): always apply against the persistent
      // slot's model. Print::apply diffs the incoming model/config against
      // the previous apply and invalidates ONLY the affected steps
      // (print_diff/object_diff/region_diff, PrintApply.cpp:1022+;
      // invalidate_state_by_config_options, Print.cpp:73-353), and process()
      // skips still-DONE steps through the PrintBase state machine -- this is
      // the upstream incremental-reslice mechanism (BackgroundSlicingProcess
      // keeps one persistent Print per plate and calls apply+process with
      // default use_cache=false, BackgroundSlicingProcess.cpp:229/277/441;
      // no upstream call site passes use_cache=true).
      print.apply(*modelForSlicePtr, config);

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
        // The slot must never be reused across a calibration slice
        // (conservative: apply() does not model calib params).
        persistentSlot->calibParamsApplied = true;
      }


      if (cancelFlag && cancelFlag->load())
      {
        print.cancel();
        throw std::runtime_error("Slicing cancelled");
      }

      // Pre-slice validation (align upstream BackgroundSlicingProcess::validate).
      // 8b93cc5df baseline: upstream Print::validate reports non-fatal
      // findings through a std::vector<StringObjectException> out-param
      // (Print.hpp:952); upstream Plater.cpp:13742-13759 iterates the vector
      // and delivers each entry to the GUI as a validation warning
      // notification (our validateWarning signal). A non-empty RETURN value
      // still aborts the slice; warnings do not.
      {
        std::vector<Slic3r::StringObjectException> validationWarnings;
        Slic3r::Polygons collisionPolygons;
        std::vector<std::pair<Slic3r::Polygon, float>> heightPolygons;
        // P4 (step profile): stamp the TRUE begin of the validate phase here
        // (not at the "Running slice" notify point below, which fires only
        // after validate returned) so param_apply's delta covers print.apply
        // alone and validate's delta covers Print::validate. The notify(25)
        // point re-records "validate"; the by-key dedupe keeps this stamp.
        stepRecorder->record(QStringLiteral("validate"),
                             QObject::tr("Pre-slice validation"), 24);
        Slic3r::StringObjectException validationError = print.validate(
            &validationWarnings, &collisionPolygons, &heightPolygons);
        capturedClearance = packSequentialClearance(collisionPolygons, heightPolygons);
        if (!validationError.string.empty())
          throw std::runtime_error("Slice validation failed: " + validationError.string);
        QString validationWarningJoined;
        for (const Slic3r::StringObjectException &warning : validationWarnings)
        {
          if (warning.string.empty())
            continue;
          if (!validationWarningJoined.isEmpty())
            validationWarningJoined += QLatin1Char('\n');
          validationWarningJoined += QString::fromUtf8(warning.string.c_str());
        }
        validationWarningText = validationWarningJoined;
      }

      notify("validate", 25, QObject::tr("Running slice"));
      // Default use_cache=false: step-level reuse is governed by the apply()
      // invalidation set and the PrintBase step state machine (upstream
      // contract -- no upstream call site passes use_cache=true).
      print.process();
      if (cancelFlag && cancelFlag->load())
      {
        print.cancel();
        throw std::runtime_error("Slicing cancelled");
      }

      // v2.8 review W2: reset calibConfig (avoid sticking after calibration slice).
      if (receiver) receiver->calibConfig_.mode = 0;


      // P1 (preview direct hand-off): the GCodeProcessorResult is constructed
      // INSIDE a shared_ptr and print.export_gcode fills it in place -- the
      // structured product is never value-copied nor re-parsed from the
      // exported text (upstream GCodeViewer::load_as_gcode consumes the same
      // object directly, GCodeViewer.hpp:186). The shared ownership crosses to
      // the GUI thread through the queued completion lambda below.
      processorResult = std::make_shared<Slic3r::GCodeProcessorResult>();
      const QString appDir = QCoreApplication::applicationDirPath();
      const QString baseName = QFileInfo(sourcePath).completeBaseName();
      const QString fileName = QStringLiteral("%1_%2.gcode")
                                   .arg(baseName)
                                   .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));
      const QString targetPath = QDir(appDir).filePath(fileName);
      const std::string generated = print.export_gcode(targetPath.toStdString(), processorResult.get());
      outputPath = QString::fromStdString(generated);
      estimatedTimeLabel = formatDurationLabel(processorResult->print_statistics.modes[0].time);
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

      // This upstream source tree does not expose get_filament_maps() /
      // get_filament_map_mode(), so leave the readback invalid and keep the UI
      // from surfacing a stale auto recommendation.
      capturedFilamentMap.valid = false;

      // P4 (step profile): success-path end anchor. The "done" sentinel's
      // firstAtMs closes gcode_export's duration downstream (PerfBench
      // derives each step's ms from adjacent firstAtMs deltas and drops the
      // sentinel from the reported profile).
      stepRecorder->record(QLatin1String(kStepTimelineDoneKey),
                           QObject::tr("Slice worker finished"), 100);

      receiver->activePrint_.store(nullptr, std::memory_order_release);
    }
    catch (const std::exception &ex)
    {
      receiver->activePrint_.store(nullptr, std::memory_order_release);
      // PLATE-PRINT-LIFECYCLE (batch 2) / review P1-1: the slot is NOT
      // destroyed here. It is carried by value into the queued completion
      // lambda (which only re-inserts it on success), so a cancelled or
      // failed Print is never reused while the Print object itself stays
      // ALIVE until the GUI thread drops the lambda. Destroying it here
      // would race ~SliceService/cancelSlice, which load activePrint_ and
      // call cancel() between this store(nullptr) and the destruction.
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
      // Phase 3: Queued (was Blocking) -- the worker no longer stalls when
      // the GUI thread is busy; the msleep below already paces the steps.
      // Progress also rides the outcome future for future-based consumers.
      outcomePromise.setProgressValue(pct);
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

    // Phase 3: fulfill the outcome promise on EVERY exit (the early
    // receiver-gone return below included) so a chained future never hangs.
    {
      SliceOutcome outcome;
      outcome.errorText = errorText;
      outcome.cancelled = cancelFlag && cancelFlag->load();
      outcome.outputPath = outputPath;
      outcome.plateIndex = resultPlateIndex;
      outcome.plateLabel = resultPlateLabel;
      outcome.generation = generation;
      outcomePromise.addResult(outcome);
    }

    if (!receiver)
      return;

    QMetaObject::invokeMethod(receiver, [receiver, cancelFlag, outputPath, errorText, estimatedTimeLabel, resultWeightLabel, resultPlateLabel, resultPlateIndex, targetResultKey, resultFilamentLabel, resultCostLabel, layerCount, validationWarningText, capturedGeometry, capturedFilamentMap, capturedClearance, stepTimeline
#ifdef HAS_LIBSLIC3R
                                         // PLATE-PRINT-LIFECYCLE (batch 2):
                                         // on success carries the finished
                                         // persistent Print back to the GUI
                                         // thread; on cancel/failure it now
                                         // ALSO carries the slot (review
                                         // P1-1) so the Print stays alive
                                         // until this lambda dies on the GUI
                                         // thread -- never destroyed while
                                         // ~SliceService/cancelSlice may
                                         // still call activePrint_->cancel().
                                         , persistentSlot, reusedPersistentPrint
                                         , processorResult
#endif
                                         // Review P1-3: generation gate.
                                         , generation
    ]() {
      if (!receiver)
        return;

      receiver->slicing_ = false;
      receiver->activeCancelFlag_.reset();
      // P4 (step profile): carry the worker's step timeline out on EVERY
      // ending. A cancelled/failed slice's partial timeline is the truthful
      // profile of that activation; the mock path delivers it empty.
      receiver->lastStepTimeline_ = std::move(*stepTimeline);
      emit receiver->slicingChanged();

      if (cancelFlag && cancelFlag->load())
      {
        receiver->sliceState_ = State::Cancelled;
        receiver->statusLabel_ = QObject::tr("Slicing cancelled");
        receiver->clearActiveTargetResult();
        emit receiver->sequentialPrintClearanceReady(SequentialPrintClearance{});
        qInfo("[SliceService] slice cancelled plate=%d reason=%s",
              resultPlateIndex,
              receiver->statusLabel_.toUtf8().constData());
        emit receiver->progressChanged();
        emit receiver->resultChanged();
        emit receiver->sliceResultCleared();
        emit receiver->stateChanged();
        emit receiver->sliceFailed(receiver->statusLabel_);
        receiver->activeTargetPlateIndex_ = -1;
        receiver->activeTargetResultKey_ = -1;
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
        receiver->activeTargetPlateIndex_ = -1;
        receiver->activeTargetResultKey_ = -1;
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
      // Review P1-3: gated on the slice generation. A project load/reset
      // (invalidateAllSliceResults -> clearResults) bumps the generation and
      // rebuilds the plate list, so a completion lambda queued after the
      // reset must not attach its stale result -- the new project's plate 0
      // can legitimately carry the same printIndex again (the counter resets
      // with the list), which would defeat the identity liveness checks
      // below on their own.
      if (resultPlateIndex >= 0 && generation == receiver->sliceGeneration_) {
        PlateSliceResult pr;
        pr.estimatedTimeLabel = estimatedTimeLabel;
        pr.resultWeightLabel = resultWeightLabel;
        pr.resultFilamentLabel = resultFilamentLabel;
        pr.resultCostLabel = resultCostLabel;
        pr.outputPath = outputPath;
        pr.resultLayerCount = receiver->resultLayerCount_;
        pr.totalFilamentMm = receiver->resultTotalFilamentMm();
        pr.source = int(ResultSource::ModelSlice);
        receiver->storePlateResultForKey(targetResultKey, pr);
        const int currentPlateIndex = receiver->plateIndexForResultKey(targetResultKey);
        if (currentPlateIndex >= 0)
          receiver->setDomainResultValid(currentPlateIndex, true);
      }
#ifdef HAS_LIBSLIC3R
      // PLATE-PRINT-LIFECYCLE (batch 2): the slice succeeded and the worker
      // handed the persistent Print back -- re-insert it for the next slice
      // unless the plate vanished mid-slice or the generation moved on (a
      // stale Print from a discarded project must never re-enter the new
      // project's slot store), then the slot dies with this lambda.
      if (persistentSlot && generation == receiver->sliceGeneration_)
      {
        // Authoritative liveness check: the stable printIndex must still
        // resolve to a live plate (plateIndexForResultKey's positional
        // fallback would always succeed here, so resolve directly).
        if (targetResultKey >= 0 && receiver->projectService_
            && receiver->projectService_->plateIndexForPrintIndex(targetResultKey) >= 0)
        {
          receiver->persistentSlices_[targetResultKey] = persistentSlot;
          if (reusedPersistentPrint)
          {
            ++receiver->persistentSliceReuseCount_;
            qInfo("[SliceService] persistent print reused (print id %d, reuse count %lld)",
                  targetResultKey,
                  static_cast<long long>(receiver->persistentSliceReuseCount_));
          }
          else
          {
            ++receiver->persistentSliceRebuildCount_;
            qInfo("[SliceService] persistent print stored (print id %d, rebuild count %lld)",
                  targetResultKey,
                  static_cast<long long>(receiver->persistentSliceRebuildCount_));
          }
        }
        // No explicit reset: this completion lambda is const; the shared_ptr
        // copy dies with it right after the re-insert above.
      }
#endif
      receiver->activeTargetPlateIndex_ = -1;
      receiver->activeTargetResultKey_ = -1;
      qInfo("[SliceService] slice finished plate=%d output=%s layers=%d",
            resultPlateIndex,
            outputPath.toUtf8().constData(),
            layerCount);
      if (processorResult && !processorResult->moves.empty())
      {
        receiver->activeProcessorResult_ = processorResult;
        ++receiver->processorResultId_;
      }
      emit receiver->progressChanged();
      emit receiver->progressUpdated(100, receiver->statusLabel_);
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
    }, Qt::QueuedConnection); });
}

#ifdef HAS_LIBSLIC3R
std::shared_ptr<const Slic3r::GCodeProcessorResult> SliceService::activeProcessorResult(
    quint64 *resultId) const
{
  if (resultId)
    *resultId = processorResultId_;
  return activeProcessorResult_;
}

void SliceService::releaseProcessorResult()
{
  activeProcessorResult_.reset();
}
#endif

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
  const int targetResultKey = resultKeyForPlateIndex(targetPlateIndex);
  activeTargetPlateIndex_ = targetPlateIndex;
  activeTargetResultKey_ = targetResultKey;
  // Phase 239 (ENGN-02): keep the previous per-plate labels (weight/filament/
  // cost/layer count). The reuse re-reads the SAME file the metadata was
  // computed from (upstream keeps gcode_result statistics alive on the
  // finished() branch, BackgroundSlicingProcess.cpp:199-221), so the labels
  // survive the re-store below; only the estimated time is re-derived fresh
  // from the parsed file.
  const PlateSliceResult previousMeta = resultForPlateIndex(targetPlateIndex)
      ? *resultForPlateIndex(targetPlateIndex) : PlateSliceResult{};
  removeResultForPlateIndex(targetPlateIndex);
  setDomainResultValid(targetPlateIndex, false);
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
    activeTargetPlateIndex_ = -1;
    activeTargetResultKey_ = -1;
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

  previousGcodeFuture_ = QtConcurrent::run([receiver, cancelFlag, localPath, targetPlateIndex, targetResultKey, targetPlateLabel, previousMeta]()
                    {
    QString errorText;
    QString estimatedTimeLabel;

#ifdef HAS_LIBSLIC3R
    // P1 (preview direct hand-off): same in-place shared_ptr pattern as the
    // startSlice worker -- the reuse parse fills the structured result where
    // it lives, no value copy, no text re-parse on the GUI thread.
    std::shared_ptr<Slic3r::GCodeProcessorResult> processorResult;
    try
    {
      Slic3r::Print print;
      processorResult = std::make_shared<Slic3r::GCodeProcessorResult>();
      print.export_gcode_from_previous_file(localPath.toStdString(), processorResult.get());
      // Phase 239 (ENGN-02): the reused file IS the statistics source
      // (same readback startSlice uses; the parsed time survives the reuse
      // instead of collapsing to an empty label).
      estimatedTimeLabel = formatDurationLabel(processorResult->print_statistics.modes[0].time);
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

    QMetaObject::invokeMethod(receiver, [receiver, cancelFlag, localPath, errorText, estimatedTimeLabel, targetPlateIndex, targetResultKey, targetPlateLabel, previousMeta
#ifdef HAS_LIBSLIC3R
                                         // P1 (preview direct hand-off): rides
                                         // the same queued delivery chain.
                                         , processorResult
#endif
                                         ]() {
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
        emit receiver->sequentialPrintClearanceReady(SequentialPrintClearance{});
        qInfo("[SliceService] slice cancelled plate=%d reason=%s",
              targetPlateIndex,
              receiver->statusLabel_.toUtf8().constData());
        emit receiver->progressChanged();
        emit receiver->resultChanged();
        emit receiver->sliceResultCleared();
        emit receiver->stateChanged();
        emit receiver->sliceFailed(receiver->statusLabel_);
        receiver->activeTargetPlateIndex_ = -1;
        receiver->activeTargetResultKey_ = -1;
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
        receiver->activeTargetResultKey_ = -1;
        return;
      }

      receiver->sliceState_ = State::Completed;
      receiver->progress_ = 100;
      receiver->statusLabel_ = QObject::tr("Existing G-code reuse complete");
      receiver->outputPath_ = localPath;
      receiver->estimatedTimeLabel_ = estimatedTimeLabel.isEmpty() ? previousMeta.estimatedTimeLabel : estimatedTimeLabel;
      receiver->resultPlateLabel_ = targetPlateLabel;
      receiver->resultPlateIndex_ = targetPlateIndex;
      receiver->resultWeightLabel_ = previousMeta.resultWeightLabel;
      receiver->resultFilamentLabel_ = previousMeta.resultFilamentLabel;
      receiver->resultCostLabel_ = previousMeta.resultCostLabel;
      receiver->resultLayerCount_ = previousMeta.resultLayerCount;
      if (targetPlateIndex >= 0)
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
        receiver->storePlateResultForKey(targetResultKey, pr);
        const int currentPlateIndex = receiver->plateIndexForResultKey(targetResultKey);
        if (currentPlateIndex >= 0)
          receiver->setDomainResultValid(currentPlateIndex, true);
      }
      receiver->activeTargetPlateIndex_ = -1;
      receiver->activeTargetResultKey_ = -1;
      // P1 (preview direct hand-off): publish before resultChanged so the
      // PreviewViewModel sync consumes the structured result directly.
      if (processorResult && !processorResult->moves.empty())
      {
        receiver->activeProcessorResult_ = processorResult;
        ++receiver->processorResultId_;
      }
      else
      {
        receiver->activeProcessorResult_.reset();
      }
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
  const PlateSliceResult *result = resultForPlateIndex(plateIndex);
  if (!result)
  {
    setExportStatus(State::Completed, progress_, QObject::tr("No G-code result for plate %1").arg(plateIndex + 1));
    logExportFailure(QStringLiteral("plate"),
                     QString{},
                     targetPath,
                     statusLabel_);
    emit exportFailed(statusLabel_);
    return false;
  }
  return exportSourceToPath(result->outputPath, targetPath, defaultExportGCodeFileName(plateIndex));
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
    const int plateIndex = plateIndexForResultKey(it.key());
    if (plateIndex < 0)
      continue;  // plate was deleted after the result was produced
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

  exportAllFuture_ = QtConcurrent::run([receiver, cancelFlag, jobs, totalJobs, generation]() {
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

  exportCopyFuture_ = QtConcurrent::run([receiver, cancelFlag, sourceAbs, targetAbs, displayName, totalBytes, generation]() {
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
  const PlateSliceResult *result = resultForPlateIndex(plateIndex);
  return result && !result->outputPath.isEmpty()
      && QFileInfo::exists(result->outputPath);
}

QString SliceService::plateEstimatedTime(int plateIndex) const
{
  const PlateSliceResult *result = resultForPlateIndex(plateIndex);
  return result ? result->estimatedTimeLabel : QString();
}

QString SliceService::plateWeight(int plateIndex) const
{
  const PlateSliceResult *result = resultForPlateIndex(plateIndex);
  return result ? result->resultWeightLabel : QString();
}

QString SliceService::plateFilament(int plateIndex) const
{
  const PlateSliceResult *result = resultForPlateIndex(plateIndex);
  return result ? result->resultFilamentLabel : QString();
}

QString SliceService::plateCost(int plateIndex) const
{
  const PlateSliceResult *result = resultForPlateIndex(plateIndex);
  return result ? result->resultCostLabel : QString();
}

int SliceService::plateLayerCount(int plateIndex) const
{
  const PlateSliceResult *result = resultForPlateIndex(plateIndex);
  return result ? result->resultLayerCount : 0;
}

QString SliceService::plateOutputPath(int plateIndex) const
{
  const PlateSliceResult *result = resultForPlateIndex(plateIndex);
  return result ? result->outputPath : QString();
}

int SliceService::plateResultSource(int plateIndex) const
{
  const PlateSliceResult *result = resultForPlateIndex(plateIndex);
  return result ? result->source : int(ResultSource::None);
}

bool SliceService::activatePlateResult(int plateIndex)
{
  if (slicing_)
    return false;

  const PlateSliceResult *result = resultForPlateIndex(plateIndex);
  if (!result || result->outputPath.isEmpty() || !QFileInfo::exists(result->outputPath))
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
  statusLabel_ = result->source == int(ResultSource::PreviousGCode)
      ? QObject::tr("Existing G-code reuse complete")
      : QObject::tr("Slice complete");
  outputPath_ = result->outputPath;
  estimatedTimeLabel_ = result->estimatedTimeLabel;
  resultWeightLabel_ = result->resultWeightLabel;
  resultPlateIndex_ = plateIndex;
  resultFilamentLabel_ = result->resultFilamentLabel;
  resultLayerCount_ = result->resultLayerCount;
  resultCostLabel_ = result->resultCostLabel;

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
  activeTargetResultKey_ = -1;
  plateResults_.clear();
  // Review P1-3: same generation bump as clearResults.
  ++sliceGeneration_;
#ifdef HAS_LIBSLIC3R
  // PLATE-PRINT-LIFECYCLE (batch 2): same conservative reset as clearResults.
  persistentSlices_.clear();
#endif
  setAllDomainResultsValid(false);
  emit resultChanged();
  emit sliceResultCleared();
  emit stateChanged();
  emit sliceStateChanged();
}

#ifdef HAS_LIBSLIC3R
void SliceService::reconcilePersistentSlices()
{
  // Drop slots whose stable printIndex no longer resolves to a live plate.
  // Catches plate pruning paths that bypass deletePlate (e.g. deleteObject
  // emptying trailing plates). Positional fallback keys (plates without a
  // stable identity) never resolve through plateIndexForPrintIndex and are
  // treated as dead -- no persistence for identity-less plates (conservative).
  for (auto it = persistentSlices_.begin(); it != persistentSlices_.end();)
  {
    if (!projectService_ || projectService_->plateIndexForPrintIndex(it->first) < 0)
    {
      qInfo("[SliceService] persistent print released (plate gone) print id %d", it->first);
      it = persistentSlices_.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

void SliceService::releasePersistentSlice(int printIndex)
{
  if (printIndex < 0)
    return;
  if (persistentSlices_.erase(printIndex) > 0)
    qInfo("[SliceService] persistent print released (plate removed) print id %d", printIndex);
}
#endif

void SliceService::removePlateResult(int plateIndex)
{
  const bool removedPlateMetadata = removeResultForPlateIndex(plateIndex);
  if (removedPlateMetadata)
    setDomainResultValid(plateIndex, false);
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

