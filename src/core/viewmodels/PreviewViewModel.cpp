#include "PreviewViewModel.h"

#include "core/services/SliceService.h"
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QTimer>
#include <QFileInfo>
#include <QHash>
#include <QColor>
#include <cstring>
#include <cfloat>
#include <cmath>
#include <algorithm>

namespace
{
  // 对齐上游 short_and_splitted_time
  QString formatTime(float totalSecs)
  {
    if (totalSecs < 0.f) totalSecs = 0.f;
    if (totalSecs < 60.f) return QStringLiteral("%1s").arg(totalSecs, 0, 'f', 1);
    if (totalSecs < 3600.f) return QStringLiteral("%1m%2s").arg(int(totalSecs / 60.f)).arg(int(totalSecs) % 60, 2, 10, QChar('0'));
    const int h = int(totalSecs / 3600.f);
    const int m = int((totalSecs - h * 3600.f) / 60.f);
    return QStringLiteral("%1h%2m").arg(h).arg(m, 2, 10, QChar('0'));
  }

  // Parse "H:MM:SS" or "HhXXm" time string back to seconds
  float parseTimeSecs(const QString &time)
  {
    const auto parts = time.split(':');
    if (parts.size() == 3)
      return parts[0].toFloat() * 3600.f + parts[1].toFloat() * 60.f + parts[2].toFloat();
    // Try "XhYYm" format
    const QRegularExpression re(QStringLiteral("(\\d+)h(\\d+)m"));
    const QRegularExpressionMatch match = re.match(time);
    if (match.hasMatch())
      return match.captured(1).toFloat() * 3600.f + match.captured(2).toFloat() * 60.f;
    // Try "XXs" format
    if (time.endsWith('s'))
      return time.left(time.size() - 1).toFloat();
    return 0.f;
  }

  struct PackedSegment
  {
    float x1;
    float y1;
    float z1;
    float x2;
    float y2;
    float z2;
    float r;
    float g;
    float b;
    float feedrate;
    float fan_speed;
    float temperature;
    float width;
    float layer_time;
    float acceleration;
    int extruder_id;
    int layer;
    int move;
  };

  // Upstream-matched gradient: 10-color Range_Colors from CrealityPrint GCodeViewer
  struct ColorResult { float r, g, b; };

  static constexpr int kRangeColorCount = 10;
  static const float kRangeColors[kRangeColorCount][3] = {
    {0.043f, 0.173f, 0.478f}, // #0b2c7a bluish
    {0.075f, 0.349f, 0.522f}, // #135985
    {0.110f, 0.533f, 0.569f}, // #1c8891
    {0.016f, 0.839f, 0.059f}, // #04d60f green
    {0.667f, 0.949f, 0.000f}, // #aaf200
    {0.988f, 0.976f, 0.012f}, // #fcf903 yellow
    {0.961f, 0.808f, 0.039f}, // #f5ce0a
    {0.820f, 0.408f, 0.188f}, // #d16830
    {0.761f, 0.322f, 0.235f}, // #c2523c
    {0.580f, 0.149f, 0.086f}, // #942616 reddish
  };

  // Linearly interpolate between two colors
  static ColorResult lerpColor(const float a[3], const float b[3], float t)
  {
    return { a[0] + t * (b[0] - a[0]),
             a[1] + t * (b[1] - a[1]),
             a[2] + t * (b[2] - a[2]) };
  }

  // Map a scalar value within [minV, maxV] to a gradient color using upstream Range_Colors
  ColorResult valueToGradient(float value, float minV, float maxV)
  {
    if (maxV <= minV)
      return lerpColor(kRangeColors[4], kRangeColors[5], 0.5f);
    const float step = (maxV - minV) / float(kRangeColorCount - 1);
    const float global_t = std::max(0.f, value - minV) / step;
    const int lowIdx = qBound(0, int(global_t), kRangeColorCount - 1);
    const int highIdx = qBound(0, lowIdx + 1, kRangeColorCount - 1);
    return lerpColor(kRangeColors[lowIdx], kRangeColors[highIdx], global_t - float(lowIdx));
  }

  struct FeatureStyle
  {
    QString label;
    QString color;
    float r;
    float g;
    float b;
  };

  FeatureStyle styleFor(const QString &type)
  {
    const QString t = type.toUpper();
    if (t.contains("WALL") || t.contains("PERIMETER"))
      return {QStringLiteral("外壁"), QStringLiteral("#FF8C3A"), 1.0f, 0.55f, 0.23f};
    if (t.contains("INFILL") || t.contains("FILL"))
      return {QStringLiteral("填充"), QStringLiteral("#38A4FF"), 0.22f, 0.64f, 1.0f};
    if (t.contains("SUPPORT"))
      return {QStringLiteral("支撑"), QStringLiteral("#8C63FF"), 0.55f, 0.39f, 1.0f};
    if (t.contains("TRAVEL"))
      return {QStringLiteral("空驶"), QStringLiteral("#6E7785"), 0.43f, 0.47f, 0.52f};
    return {QStringLiteral("其他"), QStringLiteral("#53D890"), 0.33f, 0.84f, 0.56f};
  }

  bool parseAxis(const QString &line, QChar axis, float &value)
  {
    const QRegularExpression re(QStringLiteral("(?:^|\\s)%1([+-]?(?:\\d+(?:\\.\\d*)?|\\.\\d+))").arg(axis));
    const auto m = re.match(line);
    if (!m.hasMatch())
      return false;
    bool ok = false;
    const float v = m.captured(1).toFloat(&ok);
    if (!ok)
      return false;
    value = v;
    return true;
  }

  float parseSValue(const QString &line)
  {
    const QRegularExpression re(QStringLiteral("\\bS(-?\\d+(?:\\.\\d+)?)\\b"));
    const auto m = re.match(line);
    if (!m.hasMatch())
      return -1.f;
    bool ok = false;
    const float v = m.captured(1).toFloat(&ok);
    return ok ? v : -1.f;
  }

  float parseFValue(const QString &line)
  {
    const QRegularExpression re(QStringLiteral("\\bF(-?\\d+(?:\\.\\d+)?)\\b"));
    const auto m = re.match(line);
    if (!m.hasMatch())
      return -1.f;
    bool ok = false;
    const float v = m.captured(1).toFloat(&ok);
    return ok ? v : -1.f;
  }

  bool parseTaggedValue(const QString &line, const QString &tag, float &value)
  {
    const QRegularExpression re(QStringLiteral("%1\\s*[:=]\\s*([+-]?(?:\\d+(?:\\.\\d*)?|\\.\\d+))")
                                     .arg(QRegularExpression::escape(tag)),
                                 QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(line);
    if (!m.hasMatch())
      return false;
    bool ok = false;
    const float v = m.captured(1).toFloat(&ok);
    if (!ok)
      return false;
    value = v;
    return true;
  }

  float normalizeFanSpeed(float raw)
  {
    if (raw < 0.f)
      return 100.f;
    return qBound(0.f, raw / 255.f * 100.f, 100.f);
  }

  int parseToolToken(const QString &line, int fallback)
  {
    const QRegularExpression re(QStringLiteral("\\bT(\\d+)\\b"),
                                QRegularExpression::CaseInsensitiveOption);
    const auto m = re.match(line);
    if (!m.hasMatch())
      return fallback;
    bool ok = false;
    const int tool = m.captured(1).toInt(&ok);
    return ok ? tool : fallback;
  }

  QString parseColorToken(const QString &line)
  {
    const QRegularExpression re(QStringLiteral("#[0-9A-Fa-f]{6}(?:[0-9A-Fa-f]{2})?"));
    const auto m = re.match(line);
    return m.hasMatch() ? m.captured(0) : QString{};
  }

  bool isSameZ(float a, float b)
  {
    return std::fabs(a - b) <= 0.0001f;
  }
}

PreviewViewModel::PreviewViewModel(SliceService *sliceService, QObject *parent)
    : QObject(parent), sliceService_(sliceService)
{
  playTimer_ = new QTimer(this);
  playTimer_->setInterval(24);
  connect(playTimer_, &QTimer::timeout, this, [this]()
          {
    if (moveCount_ <= 0)
      return;
    if (currentMove_ >= moveCount_)
    {
      playTimer_->stop();
      return;
    }
    currentMove_ = qMin(currentMove_ + 12, moveCount_);
    updateToolPositionData();
    emit stateChanged(); });

  connect(sliceService_, &SliceService::progressChanged, this, &PreviewViewModel::stateChanged);
  connect(sliceService_, &SliceService::slicingChanged, this, &PreviewViewModel::stateChanged);
  connect(sliceService_, &SliceService::resultChanged, this, [this]()
          {
    syncPreviewWithActiveResult();
    emit stateChanged(); });
  connect(sliceService_, &SliceService::sliceFinished, this, [this](const QString &time)
          {
    if (!time.isEmpty())
    {
      estimatedTime_ = time;
      totalTime_ = time;
    }
    syncPreviewWithActiveResult();
    emit stateChanged(); });
  connect(sliceService_, &SliceService::sliceResultCleared, this, [this]()
          {
    resetPreviewState();
    emit stateChanged(); });
  connect(sliceService_, &SliceService::sliceFailed, this, [this](const QString &)
          {
    resetPreviewState();
    emit stateChanged(); });
}

int PreviewViewModel::progress() const
{
  return sliceService_ ? sliceService_->progress() : 0;
}

bool PreviewViewModel::slicing() const
{
  return sliceService_ ? sliceService_->slicing() : false;
}

bool PreviewViewModel::isPlaying() const
{
  return playTimer_ && playTimer_->isActive();
}

QString PreviewViewModel::estimatedTime() const
{
  return estimatedTime_;
}

QString PreviewViewModel::currentTime() const
{
  // Look up accumulated time at current move (对齐上游 IMSlider::get_label)
  if (m_moveAccumulatedTime.empty() || currentMove_ <= 0)
    return QStringLiteral("0s");
  const int idx = qMin(currentMove_, int(m_moveAccumulatedTime.size()) - 1);
  return formatTime(m_moveAccumulatedTime[idx]);
}

QString PreviewViewModel::timeAtMove(int moveIndex) const
{
  // Look up accumulated time at arbitrary move position (对齐上游 IMSlider hover 提示)
  if (m_moveAccumulatedTime.empty() || moveIndex <= 0)
    return QStringLiteral("0s");
  const int idx = qMin(moveIndex, int(m_moveAccumulatedTime.size()) - 1);
  return formatTime(m_moveAccumulatedTime[idx]);
}

QStringList PreviewViewModel::viewModes() const
{
  // 对齐上游 GCodeViewer get_view_type_string 显示名称
  return {
      QStringLiteral("Line Type"),
      QStringLiteral("Layer Height"),
      QStringLiteral("Line Width"),
      QStringLiteral("Tool"),
      QStringLiteral("Speed"),
      QStringLiteral("Fan Speed"),
      QStringLiteral("Temperature"),
      QStringLiteral("Filament"),
      QStringLiteral("Filament ID"),
      QStringLiteral("Flow"),
      QStringLiteral("Layer Time"),
      QStringLiteral("Layer Time (log)"),
      QStringLiteral("Acceleration")};
}

bool PreviewViewModel::loadGCodeForPreview(const QString &filePath)
{
  const QFileInfo info(filePath);
  if (!info.exists() || !info.isFile())
  {
    resetPreviewState();
    emit stateChanged();
    return false;
  }

  rebuildFromGCode(info.absoluteFilePath());
  emit stateChanged();
  return !gcodePreviewData_.isEmpty();
}

void PreviewViewModel::syncPreviewWithActiveResult()
{
  const QString activePath = sliceService_ ? sliceService_->outputPath() : QString{};
  if (activePath.isEmpty())
  {
    resetPreviewState();
    return;
  }

  const QFileInfo info(activePath);
  if (!info.exists() || !info.isFile())
  {
    resetPreviewState();
    return;
  }

  const QString activeTime = sliceService_->estimatedTimeLabel();
  estimatedTime_ = activeTime.isEmpty() ? QStringLiteral("--:--:--") : activeTime;
  totalTime_ = estimatedTime_;
  rebuildFromGCode(info.absoluteFilePath());
}

void PreviewViewModel::setLayerRange(int minLayer, int maxLayer)
{
  if (layerCount_ <= 0)
    return;
  const int lo = qBound(0, minLayer, layerCount_ - 1);
  const int hi = qBound(lo, maxLayer, layerCount_ - 1);
  if (lo == currentLayerMin_ && hi == currentLayerMax_)
    return;
  currentLayerMin_ = lo;
  currentLayerMax_ = hi;
  emit stateChanged();
}

void PreviewViewModel::jumpToLayer(int oneIndexedLayer)
{
  if (layerCount_ <= 0)
    return;
  // 上游 IMSlider::do_go_to_layer 接收 1-indexed，内部转为 0-indexed
  const int zeroBased = qBound(0, oneIndexedLayer - 1, layerCount_ - 1);
  setLayerRange(zeroBased, zeroBased);
}

void PreviewViewModel::moveLayerRange(int delta)
{
  if (layerCount_ <= 0)
    return;
  const int span = currentLayerMax_ - currentLayerMin_;
  int newMin = currentLayerMin_ + delta;
  int newMax = currentLayerMax_ + delta;
  // 钳位：保持 span 不变
  if (newMin < 0) {
    newMin = 0;
    newMax = qMin(span, layerCount_ - 1);
  }
  if (newMax >= layerCount_) {
    newMax = layerCount_ - 1;
    newMin = qMax(0, newMax - span);
  }
  setLayerRange(newMin, newMax);
}

void PreviewViewModel::setCurrentMove(int move)
{
  const int clamped = qBound(0, move, moveCount_);
  if (clamped == currentMove_)
    return;
  currentMove_ = clamped;
  updateToolPositionData();
  emit stateChanged();
}

void PreviewViewModel::updateToolPositionData()
{
  if (segments_.empty() || currentMove_ < 0) {
    hasToolPosition_ = false;
    return;
  }
  const int idx = qMin(currentMove_, static_cast<int>(segments_.size()) - 1);
  const auto &seg = segments_[idx];
  // 上游 GCodeViewer::Marker 使用 move 的终点位置
  hasToolPosition_ = true;
  toolX_ = seg.x2;
  toolY_ = seg.y2;
  toolZ_ = seg.z2;
  toolFeedrate_ = seg.feedrate;
  toolFanSpeed_ = seg.fan_speed;
  toolTemperature_ = seg.temperature;
  toolWidth_ = seg.width;
  toolLayerTime_ = seg.layer_time;
  toolAcceleration_ = seg.acceleration;
  toolExtruderId_ = seg.extruder_id;
  toolLayer_ = seg.layer;
  toolMoveIndex_ = seg.move;
  // 挤出移动判断：起点和终点不同且有 feedrate
  toolIsExtrusion_ = !seg.isTravel;
}

void PreviewViewModel::playAnimation()
{
  if (moveCount_ <= 0)
    return;
  playTimer_->start();
  emit stateChanged();
}

void PreviewViewModel::pauseAnimation()
{
  playTimer_->stop();
  emit stateChanged();
}

void PreviewViewModel::togglePlayPause()
{
  if (isPlaying())
    pauseAnimation();
  else
    playAnimation();
}

void PreviewViewModel::setViewModeIndex(int index)
{
  const int clamped = qBound(0, index, viewModes().size() - 1);
  if (clamped == viewModeIndex_)
    return;
  viewModeIndex_ = clamped;
  recolorAndPackSegments();
  emit stateChanged();
}

void PreviewViewModel::setStealthMode(bool enabled)
{
  if (stealthMode_ == enabled)
    return;
  stealthMode_ = enabled;
  // Recalculate totalTime with stealth multiplier (~1.4x slower due to reduced accel/jerk)
  // 对齐上游 PrintEstimatedStatistics::modes[1] — stealth mode
  if (!totalTime_.contains("--"))
  {
    totalTime_ = stealthMode_
        ? formatTime(parseTimeSecs(estimatedTime_) * 1.4f)
        : estimatedTime_;
  }
  emit stateChanged();
}

void PreviewViewModel::setShowTravelMoves(bool enabled)
{
  if (showTravelMoves_ == enabled)
    return;
  showTravelMoves_ = enabled;
  recolorAndPackSegments();
  emit stateChanged();
}

void PreviewViewModel::setShowBed(bool enabled)
{
  if (showBed_ == enabled)
    return;
  showBed_ = enabled;
  emit stateChanged();
}

void PreviewViewModel::setShowMarker(bool enabled)
{
  if (showMarker_ == enabled)
    return;
  showMarker_ = enabled;
  emit stateChanged();
}

QVariantMap PreviewViewModel::legendItem(const QString &label, const QString &color, int count) const
{
  QVariantMap item;
  item.insert(QStringLiteral("label"), label);
  item.insert(QStringLiteral("color"), color);
  item.insert(QStringLiteral("count"), count);
  return item;
}

void PreviewViewModel::resetPreviewState()
{
  if (playTimer_)
    playTimer_->stop();
  gcodePreviewData_.clear();
  legendItems_.clear();
  segments_.clear();
  featureCount_.clear();
  m_layerTimes.clear();
  m_layerZs.clear();
  m_toolChangePositions.clear();
  m_extruderUsedLength.clear();
  m_extruderUsedWeight.clear();
  m_roleTimes.clear();
  m_moveAccumulatedTime.clear();
  const bool hadTicks = !tickMarks_.isEmpty();
  tickMarks_.clear();
  m_maxLayerTime = 0.f;
  layerCount_ = 0;
  moveCount_ = 0;
  currentMove_ = 0;
  currentLayerMin_ = 0;
  currentLayerMax_ = 0;
  estimatedTime_ = QStringLiteral("--:--:--");
  totalTime_ = QStringLiteral("--:--:--");
  filamentUsed_ = QStringLiteral("--");
  filamentWeight_ = QStringLiteral("--");
  extrudeMoveCount_ = 0;
  travelMoveCount_ = 0;
  toolChangeCount_ = 0;
  avgSpeed_ = QStringLiteral("--");
  estimatedCost_ = QStringLiteral("--");
  m_legendType = 0;
  m_legendGradMinLabel.clear();
  m_legendGradMaxLabel.clear();
  m_legendGradMinColor.clear();
  m_legendGradMaxColor.clear();
  hasToolPosition_ = false;
  toolX_ = 0;
  toolY_ = 0;
  toolZ_ = 0;
  toolFeedrate_ = 0;
  toolFanSpeed_ = 0;
  toolTemperature_ = 0;
  toolWidth_ = 0;
  toolLayerTime_ = 0;
  toolAcceleration_ = 0;
  toolExtruderId_ = 0;
  toolLayer_ = 0;
  toolMoveIndex_ = 0;
  toolIsExtrusion_ = false;
  if (hadTicks)
    emit tickMarksChanged();
}

void PreviewViewModel::rebuildFromGCode(const QString &filePath)
{
  const QString preservedEstimatedTime = estimatedTime_;
  const QString preservedTotalTime = totalTime_;
  resetPreviewState();
  estimatedTime_ = preservedEstimatedTime;
  totalTime_ = preservedTotalTime;

  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  segments_.reserve(30000);

  float x = 0.f;
  float y = 0.f;
  float z = 0.f;
  float e = 0.f;
  int layer = 0;
  int moveIndex = 0;
  QString currentType = QStringLiteral("TRAVEL");

  // Track metadata for view-mode coloring
  float currentFeedrate = 0.f;
  float currentFanSpeed = 0.f;
  float currentTemp = 0.f;
  float currentAccel = 0.f;
  int currentExtruder = 0;
  bool relativeExtrusion = false;
  float currentWidth = 0.f;
  float currentHeight = 0.f;
  float elapsedTime = 0.f;
  float layerStartElapsed = 0.f;
  float currentLayerTime = 0.f;
  float printLayerZ = 0.f;
  bool hasPrintLayerZ = false;

  // Filament tracking
  float totalFilamentUsed = 0.f; // mm of filament extruded
  float filamentDiameter = 1.75f; // default
  float filamentDensity = 1.24f;  // default PLA g/cm3
  int extrudeMoveCount = 0;
  int travelMoveCount = 0;
  int toolChangeCount = 0;
  double feedrateSum = 0.0;
  int feedrateCount = 0;

  // Per-role time tracking (对齐上游 PrintEstimatedStatistics::roles_times)
  QHash<QString, double> roleTimeAccum; // TYPE label → accumulated time in seconds
  auto accumulateRoleTime = [&](const QString &role, float dx, float dy, float dz, float feed) {
    if (feed <= 0.f) return;
    const float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    roleTimeAccum[role] += dist / feed * 60.0; // feed is mm/min, dist is mm → seconds
  };

  while (!file.atEnd())
  {
    const QString raw = QString::fromUtf8(file.readLine()).trimmed();
    if (raw.isEmpty())
      continue;

    if (raw.startsWith(';'))
    {
      if (raw.startsWith(QStringLiteral(";TYPE:")))
        currentType = raw.mid(6).trimmed();
      else
      {
        const QRegularExpression featureRe(QStringLiteral("^;\\s*FEATURE\\s*:\\s*(.+)$"),
                                           QRegularExpression::CaseInsensitiveOption);
        const auto featureMatch = featureRe.match(raw);
        if (featureMatch.hasMatch())
          currentType = featureMatch.captured(1).trimmed();
      }

      const QString upperComment = raw.toUpper();
      auto appendTick = [&](OWzx::TickType type, const QString &extra = QString{}) {
        OWzx::TickCode tick;
        tick.tick = qMax(0, layer);
        tick.type = type;
        tick.extruder = parseToolToken(raw, currentExtruder);
        tick.color = parseColorToken(raw);
        tick.extra = extra;
        tickMarks_.append(tick);
      };
      if (upperComment.contains(QStringLiteral("COLOR_CHANGE")))
      {
        appendTick(OWzx::TickType::ColorChange);
      }
      else if (upperComment.contains(QStringLiteral("PAUSE_PRINT")))
      {
        appendTick(OWzx::TickType::PausePrint);
      }
      else if (upperComment.contains(QStringLiteral("CUSTOM_GCODE")))
      {
        const int tagPos = upperComment.indexOf(QStringLiteral("CUSTOM_GCODE"));
        QString extra = tagPos >= 0 ? raw.mid(tagPos + int(QStringLiteral("CUSTOM_GCODE").size())).trimmed() : QString{};
        if (extra.startsWith(QLatin1Char(':')))
          extra = extra.mid(1).trimmed();
        appendTick(OWzx::TickType::CustomGcode, extra);
      }
      else if (upperComment.contains(QStringLiteral("MANUAL_TOOL_CHANGE")))
      {
        appendTick(OWzx::TickType::ToolChange);
      }

      float taggedValue = 0.f;
      if (parseTaggedValue(raw, QStringLiteral("TIME_ELAPSED"), taggedValue))
      {
        elapsedTime = taggedValue;
        currentLayerTime = qMax(0.f, elapsedTime - layerStartElapsed);
      }
      if (parseTaggedValue(raw, QStringLiteral("LINE_WIDTH"), taggedValue)
          || parseTaggedValue(raw, QStringLiteral("WIDTH"), taggedValue))
      {
        if (taggedValue > 0.f)
          currentWidth = taggedValue;
      }
      if (parseTaggedValue(raw, QStringLiteral("LAYER_HEIGHT"), taggedValue)
          || parseTaggedValue(raw, QStringLiteral("HEIGHT"), taggedValue))
      {
        if (taggedValue > 0.f)
          currentHeight = taggedValue;
      }
      if (raw.contains(QStringLiteral("filament_diameter")))
      {
        const QRegularExpression re(QLatin1String("filament_diameter[=:]\\s*(\\d+(?:\\.\\d+)?)"), QRegularExpression::CaseInsensitiveOption);
        const auto m = re.match(raw);
        if (m.hasMatch())
        {
          bool ok = false;
          const float v = m.captured(1).toFloat(&ok);
          if (ok && v > 0.f) filamentDiameter = v;
        }
      }
      if (raw.contains(QStringLiteral("filament_density")))
      {
        const QRegularExpression re(QLatin1String("filament_density[=:]\\s*(\\d+(?:\\.\\d+)?)"), QRegularExpression::CaseInsensitiveOption);
        const auto m = re.match(raw);
        if (m.hasMatch())
        {
          bool ok = false;
          const float v = m.captured(1).toFloat(&ok);
          if (ok && v > 0.f) filamentDensity = v;
        }
      }
      continue;
    }

    QString command = raw;
    const int commentPos = command.indexOf(QLatin1Char(';'));
    if (commentPos >= 0)
      command = command.left(commentPos).trimmed();
    if (command.isEmpty())
      continue;

    const QString upper = command.toUpper();

    if (upper.startsWith(QStringLiteral("M106")))
    {
      currentFanSpeed = normalizeFanSpeed(parseSValue(upper));
      continue;
    }

    if (upper.startsWith(QStringLiteral("M107")))
    {
      currentFanSpeed = 0.f;
      continue;
    }

    if (upper.startsWith(QStringLiteral("M104")) || upper.startsWith(QStringLiteral("M109")))
    {
      const float temp = parseSValue(upper);
      if (temp >= 0.f)
        currentTemp = temp;
      continue;
    }

    if (upper == QStringLiteral("M82") || upper.startsWith(QStringLiteral("M82 ")))
    {
      relativeExtrusion = false;
      continue;
    }

    if (upper == QStringLiteral("M83") || upper.startsWith(QStringLiteral("M83 ")))
    {
      relativeExtrusion = true;
      continue;
    }

    if (upper.startsWith(QStringLiteral("G92")))
    {
      float resetE = e;
      if (parseAxis(upper, 'E', resetE))
        e = resetE;
      continue;
    }

    // Tool change command
    if (upper.startsWith('T') && upper.length() >= 2)
    {
      bool ok = false;
      const int tid = upper.mid(1).section(QLatin1Char(' '), 0, 0).toInt(&ok);
      if (ok && tid >= 0)
      {
        if (tid != currentExtruder) {
          ++toolChangeCount;
          // 记录工具切换位置（对齐上游 IMSlider colored band）
          m_toolChangePositions.push_back({moveIndex, tid});
        }
        currentExtruder = tid;
      }
    }

    // Acceleration command: M204 S... X... Y... Z... E...
    if (upper.startsWith(QStringLiteral("M204")))
    {
      float val = 0.f;
      if (parseAxis(upper, 'X', val) && val > 0.f) currentAccel = val;
      else if (parseAxis(upper, 'Y', val) && val > 0.f) currentAccel = val;
      else if (parseAxis(upper, 'Z', val) && val > 0.f) currentAccel = val;
      else if (parseAxis(upper, 'E', val) && val > 0.f) currentAccel = val;
      else if (parseAxis(upper, 'P', val) && val > 0.f) currentAccel = val;
      else if (parseAxis(upper, 'T', val) && val > 0.f) currentAccel = val;
      else if (parseAxis(upper, 'S', val) && val > 0.f) currentAccel = val;
    }

    const bool isG0 = upper == QStringLiteral("G0") || upper.startsWith(QStringLiteral("G0 "));
    const bool isG1 = upper == QStringLiteral("G1") || upper.startsWith(QStringLiteral("G1 "));
    if (!isG0 && !isG1)
      continue;

    const float lineF = parseFValue(upper);
    if (lineF > 0.f)
      currentFeedrate = lineF;

    float nx = x;
    float ny = y;
    float nz = z;
    float ne = e;
    parseAxis(upper, 'X', nx);
    parseAxis(upper, 'Y', ny);
    parseAxis(upper, 'Z', nz);

    float parsedE = 0.f;
    float extrusionDelta = 0.f;
    const bool hasE = parseAxis(upper, 'E', parsedE);
    if (hasE)
    {
      if (relativeExtrusion)
      {
        extrusionDelta = parsedE;
        ne = e + parsedE;
      }
      else
      {
        ne = parsedE;
        extrusionDelta = ne - e;
      }
    }

    const bool moved = (nx != x) || (ny != y) || (nz != z);
    if (!moved)
    {
      x = nx;
      y = ny;
      z = nz;
      if (hasE)
        e = ne;
      continue;
    }

    const bool extruding = hasE && extrusionDelta > 0.00001f;
    if (extruding)
    {
      // Upstream Preview layers are printed extrusion layers. Z-hop/travel
      // lifts move the nozzle but must not create empty selectable layers.
      if (!hasPrintLayerZ)
      {
        printLayerZ = nz;
        hasPrintLayerZ = true;
        m_layerZs.append(printLayerZ);
      }
      else if (!isSameZ(nz, printLayerZ))
      {
        m_layerTimes.append(currentLayerTime);
        m_maxLayerTime = qMax(m_maxLayerTime, currentLayerTime);
        ++layer;
        printLayerZ = nz;
        m_layerZs.append(printLayerZ);
        layerStartElapsed = elapsedTime;
        currentLayerTime = 0.f;
      }
    }

    const float dx = nx - x;
    const float dy = ny - y;
    const float dz = nz - z;
    const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    float volumetricRate = 0.f;
    if (extruding && dist > 0.f && currentFeedrate > 0.f && extrusionDelta > 0.f)
    {
      const float filamentArea = 3.14159265f * (filamentDiameter * 0.5f) * (filamentDiameter * 0.5f);
      const float mm3PerMm = extrusionDelta * filamentArea / dist;
      volumetricRate = currentFeedrate / 60.f * mm3PerMm;
    }

    if (extruding)
    {
      totalFilamentUsed += extrusionDelta;
      ++extrudeMoveCount;
      if (currentFeedrate > 0.f)
      {
        feedrateSum += currentFeedrate;
        ++feedrateCount;
      }
      // 每挤出机耗材追踪（对齐上游 PrintEstimatedStatistics volumes_per_extruder）
      m_extruderUsedLength[currentExtruder] += extrusionDelta;
      accumulateRoleTime(currentType, dx, dy, dz, currentFeedrate);
    }
    else
    {
      ++travelMoveCount;
      accumulateRoleTime(QStringLiteral("TRAVEL"), dx, dy, dz, currentFeedrate);
    }
    const FeatureStyle style = styleFor(extruding ? currentType : QStringLiteral("TRAVEL"));

    StoredSegment seg;
    seg.x1 = x;
    seg.y1 = y;
    seg.z1 = z;
    seg.x2 = nx;
    seg.y2 = ny;
    seg.z2 = nz;
    seg.baseR = style.r;
    seg.baseG = style.g;
    seg.baseB = style.b;
    seg.feedrate = currentFeedrate;
    seg.fan_speed = currentFanSpeed;
    seg.temperature = currentTemp;
    seg.width = currentWidth;
    seg.height = currentHeight > 0.f ? currentHeight : nz;
    seg.layer_time = currentLayerTime;
    seg.acceleration = currentAccel;
    seg.volumetric_rate = volumetricRate;
    seg.extruder_id = currentExtruder;
    seg.layer = layer;
    seg.move = moveIndex;
    seg.isTravel = !extruding;
    segments_.push_back(seg);

    // Accumulate elapsed time for move slider (对齐上游 IMSlider m_layers_times)
    {
      const float dt = currentFeedrate > 0.f ? dist / currentFeedrate * 60.f : 0.f;
      const float prev = m_moveAccumulatedTime.empty() ? 0.f : m_moveAccumulatedTime.back();
      m_moveAccumulatedTime.push_back(prev + dt);
    }

    featureCount_[style.label] += 1;

    x = nx;
    y = ny;
    z = nz;
    if (hasE)
      e = ne;
    ++moveIndex;
  }

  moveCount_ = moveIndex;
  layerCount_ = qMax(1, hasPrintLayerZ ? layer + 1 : 1);

  // Save last layer's time
  m_layerTimes.append(currentLayerTime);
  m_maxLayerTime = qMax(m_maxLayerTime, currentLayerTime);

  currentLayerMin_ = 0;
  currentLayerMax_ = layerCount_ - 1;
  currentMove_ = moveCount_;
  extrudeMoveCount_ = extrudeMoveCount;
  travelMoveCount_ = travelMoveCount;
  toolChangeCount_ = toolChangeCount;

  // Average speed
  if (feedrateCount > 0)
    avgSpeed_ = QStringLiteral("%1 mm/s").arg(feedrateSum / feedrateCount, 0, 'f', 1);
  else
    avgSpeed_ = QStringLiteral("--");

  if (segments_.empty())
  {
    filamentUsed_ = QStringLiteral("--");
    filamentWeight_ = QStringLiteral("--");
    estimatedCost_ = QStringLiteral("--");
  }
  else
  {
    filamentUsed_ = QStringLiteral("%1 m").arg(totalFilamentUsed / 1000.f, 0, 'f', 2);
    // Weight = volume (mm³) × density (g/cm³) × 1e-3 (cm³/mm³)
    // volume = length_mm × π × (diameter/2)²
    const float volume_mm3 = totalFilamentUsed * 3.14159265f * (filamentDiameter * 0.5f) * (filamentDiameter * 0.5f);
    const float weight_g = volume_mm3 * filamentDensity * 0.001f;
    filamentWeight_ = QStringLiteral("%1 g").arg(weight_g, 0, 'f', 1);

    // 计算每挤出机耗材重量（对齐上游 PrintEstimatedStatistics volumes_per_extruder）
    m_extruderUsedWeight.clear();
    for (auto it = m_extruderUsedLength.constBegin(); it != m_extruderUsedLength.constEnd(); ++it)
    {
      const float vol = it.value() * 3.14159265f * (filamentDiameter * 0.5f) * (filamentDiameter * 0.5f);
      m_extruderUsedWeight[it.key()] = vol * filamentDensity * 0.001f;
    }

    // Estimated cost: weight_kg * price_per_kg (default ~$20/kg PLA)
    constexpr float pricePerKg = 20.0f; // dollars per kg
    estimatedCost_ = QStringLiteral("$%1").arg(weight_g / 1000.0f * pricePerKg, 0, 'f', 2);
  }

  // Build role time breakdown (对齐上游 PrintEstimatedStatistics::roles_times)
  m_roleTimes.clear();
  // Map upstream TYPE labels to Chinese display names
  static const QHash<QString, QString> roleLabels = {
      {QStringLiteral("WALL-INNER"),          tr("内壁")},
      {QStringLiteral("WALL-OUTER"),          tr("外壁")},
      {QStringLiteral("WALL"),                tr("墙壁")},
      {QStringLiteral("SKIN"),                tr("顶面/底面")},
      {QStringLiteral("FILL"),                tr("填充")},
      {QStringLiteral("SUPPORT"),             tr("支撑")},
      {QStringLiteral("SUPPORT-INTERFACE"),   tr("支撑面")},
      {QStringLiteral("BRIDGE"),              tr("桥接")},
      {QStringLiteral("PERIMETER"),           tr("轮廓")},
      {QStringLiteral("EXTERNAL"),            tr("外壁")},
      {QStringLiteral("INTERNAL"),            tr("内壁")},
      {QStringLiteral("OVERHANG"),            tr("悬空")},
      {QStringLiteral("IRONING"),             tr("熨烫")},
      {QStringLiteral("SKIRT"),               tr("裙边")},
      {QStringLiteral("BRIM"),                tr("底座")},
      {QStringLiteral("WIPE"),                tr("擦料")},
      {QStringLiteral("PRIME-TOWER"),         tr("擦料塔")},
      {QStringLiteral("CUSTOM"),              tr("自定义")},
      {QStringLiteral("TRAVEL"),              tr("空驶")},
  };
  for (auto it = roleTimeAccum.cbegin(); it != roleTimeAccum.cend(); ++it)
  {
    RoleTimeEntry entry;
    entry.name = roleLabels.value(it.key(), it.key());
    entry.timeSecs = it.value();
    m_roleTimes.append(entry);
  }
  // Sort by time descending
  std::sort(m_roleTimes.begin(), m_roleTimes.end(),
            [](const RoleTimeEntry &a, const RoleTimeEntry &b) { return a.timeSecs > b.timeSecs; });

  updateToolPositionData();
  recolorAndPackSegments();
  qInfo("[PreviewViewModel] payload file=%s bytes=%lld layers=%d moves=%d segments=%d travelVisible=%d",
        filePath.toUtf8().constData(),
        static_cast<long long>(QFileInfo(filePath).size()),
        layerCount_,
        moveCount_,
        int(segments_.size()),
        showTravelMoves_ ? 1 : 0);
  if (!tickMarks_.isEmpty())
  {
    std::stable_sort(tickMarks_.begin(), tickMarks_.end());
    emit tickMarksChanged();
  }
}

int PreviewViewModel::roleTimeCount() const { return m_roleTimes.size(); }

QString PreviewViewModel::roleTimeName(int i) const
{
  return (i >= 0 && i < m_roleTimes.size()) ? m_roleTimes[i].name : QString{};
}

QString PreviewViewModel::roleTimeValue(int i) const
{
  if (i < 0 || i >= m_roleTimes.size()) return {};
  const double secs = m_roleTimes[i].timeSecs;
  if (secs < 60.0) return QStringLiteral("%1s").arg(secs, 0, 'f', 1);
  if (secs < 3600.0) return QStringLiteral("%1m %2s").arg(int(secs / 60.0)).arg(int(secs) % 60, 2, 10, QChar('0'));
  const int h = int(secs / 3600.0);
  const int m = int((secs - h * 3600.0) / 60.0);
  return QStringLiteral("%1h %2m").arg(h).arg(m, 2, 10, QChar('0'));
}

double PreviewViewModel::roleTimePercent(int i) const
{
  if (i < 0 || i >= m_roleTimes.size()) return 0.0;
  double totalSecs = 0.0;
  for (const auto &rt : m_roleTimes) totalSecs += rt.timeSecs;
  return totalSecs > 0.0 ? (m_roleTimes[i].timeSecs / totalSecs * 100.0) : 0.0;
}

int PreviewViewModel::layerTimeCount() const { return m_layerTimes.size(); }
float PreviewViewModel::layerTimeAt(int layer) const
{
  return (layer >= 0 && layer < m_layerTimes.size()) ? m_layerTimes[layer] : 0.f;
}
float PreviewViewModel::maxLayerTime() const { return m_maxLayerTime; }

float PreviewViewModel::minLayerTime() const
{
  if (m_layerTimes.isEmpty()) return 0.f;
  float minT = m_layerTimes[0];
  for (float t : m_layerTimes)
    if (t < minT) minT = t;
  return minT;
}

float PreviewViewModel::avgLayerTime() const
{
  if (m_layerTimes.isEmpty()) return 0.f;
  double sum = 0.0;
  for (float t : m_layerTimes) sum += t;
  return float(sum / m_layerTimes.size());
}

float PreviewViewModel::layerZAt(int layer) const
{
  if (layer < 0 || layer >= m_layerZs.size())
    return 0.f;
  return m_layerZs[layer];
}

int PreviewViewModel::toolChangePositionCount() const
{
  return m_toolChangePositions.size();
}

int PreviewViewModel::toolChangePositionAt(int i) const
{
  if (i < 0 || i >= m_toolChangePositions.size())
    return 0;
  return m_toolChangePositions[i].moveIndex;
}

int PreviewViewModel::toolChangeExtruderIdAt(int i) const
{
  if (i < 0 || i >= m_toolChangePositions.size())
    return 0;
  return m_toolChangePositions[i].extruderId;
}

QString PreviewViewModel::extruderColor(int extruderId) const
{
  // 上游 extruder_colors: 8 种固定颜色循环
  static const char *colors[] = {
    "#009688", "#f44336", "#2196f3", "#ff9800",
    "#9c27b0", "#4caf50", "#ff5722", "#607d8b"
  };
  return QString::fromUtf8(colors[extruderId % 8]);
}

int PreviewViewModel::extruderCount() const
{
  return m_extruderUsedLength.size();
}

double PreviewViewModel::extruderUsedLength(int extruderId) const
{
  return m_extruderUsedLength.value(extruderId, 0.0) / 1000.0; // mm → m
}

double PreviewViewModel::extruderUsedWeight(int extruderId) const
{
  return m_extruderUsedWeight.value(extruderId, 0.0); // grams
}

void PreviewViewModel::recolorAndPackSegments()
{
  gcodePreviewData_.clear();
  legendItems_.clear();

  if (segments_.empty())
    return;

  const int mode = viewModeIndex_;
  std::vector<int> visibleIndices;
  visibleIndices.reserve(segments_.size());
  for (int i = 0; i < int(segments_.size()); ++i)
  {
    if (showTravelMoves_ || !segments_[i].isTravel)
      visibleIndices.push_back(i);
  }

  if (visibleIndices.empty())
    return;

  const int count = int(visibleIndices.size());

  // Determine value range for gradient modes
  float minV = FLT_MAX, maxV = -FLT_MAX;
  for (const int idx : visibleIndices)
  {
    const auto &s = segments_[idx];
    float v = 0.f;
    switch (mode)
    {
    case 1:  v = s.height; break;      // Layer height
    case 2:  v = s.width; break;      // Width
    case 4:  v = s.feedrate; break;    // Feedrate
    case 5:  v = qMax(0.f, s.fan_speed); break; // FanSpeed
    case 6:  v = s.temperature; break;  // Temperature
    case 3:  v = float(s.extruder_id); break; // Tool
    case 7:  v = float(s.extruder_id); break; // ColorPrint (via extruder)
    case 8:  v = float(s.extruder_id); break; // FilamentId
    case 9:  v = s.volumetric_rate; break;      // VolumetricRate
    case 10: v = s.layer_time; break;   // LayerTime
    case 11: v = s.layer_time > 0.f ? std::log(s.layer_time) : 0.f; break; // LayerTimeLog
    case 12: v = s.acceleration; break;  // Acceleration
    default: continue;
    }
    if (v < minV) minV = v;
    if (v > maxV) maxV = v;
  }

  // Build packed segments with view-mode colors
  std::vector<PackedSegment> packed;
  packed.resize(count);

  // Tool/extruder color palette (matching upstream m_tool_colors)
  static const float toolColors[][3] = {
    {0.95f, 0.55f, 0.22f},  // Extruder 0 - orange
    {0.22f, 0.55f, 0.87f},  // Extruder 1 - blue
    {0.30f, 0.69f, 0.46f},  // Extruder 2 - green
    {0.61f, 0.35f, 0.71f},  // Extruder 3 - purple
    {0.91f, 0.30f, 0.24f},  // Extruder 4 - red
    {0.10f, 0.74f, 0.61f},  // Extruder 5 - teal
    {0.95f, 0.77f, 0.06f},  // Extruder 6 - yellow
    {0.91f, 0.12f, 0.55f},  // Extruder 7 - pink
  };
  static constexpr int kToolColorCount = int(sizeof(toolColors) / sizeof(toolColors[0]));

  for (int i = 0; i < count; ++i)
  {
    const auto &s = segments_[visibleIndices[i]];
    auto &p = packed[i];

    p.x1 = s.x1; p.y1 = s.y1; p.z1 = s.z1;
    p.x2 = s.x2; p.y2 = s.y2; p.z2 = s.z2;
    p.feedrate = s.feedrate;
    p.fan_speed = s.fan_speed;
    p.temperature = s.temperature;
    p.width = s.width;
    p.layer_time = s.layer_time;
    p.acceleration = s.acceleration;
    p.extruder_id = s.extruder_id;
    p.layer = s.layer;
    p.move = s.move;

    if (mode == 0)
    {
      // FeatureType: use parsed role colors.
      p.r = s.baseR;
      p.g = s.baseG;
      p.b = s.baseB;
    }
    else if (mode == 3 || mode == 7 || mode == 8)
    {
      // Tool / ColorPrint / FilamentId: fixed palette per extruder
      const auto &tc = toolColors[s.extruder_id % kToolColorCount];
      p.r = tc[0]; p.g = tc[1]; p.b = tc[2];
    }
    else
    {
      float value = 0.f;
      switch (mode)
      {
      case 1:  value = s.height; break;
      case 2:  value = s.width; break;
      case 4:  value = s.feedrate; break;
      case 5:  value = qMax(0.f, s.fan_speed); break;
      case 6:  value = s.temperature; break;
      case 9:  value = s.volumetric_rate; break;
      case 10: value = s.layer_time; break;
      case 11: value = s.layer_time > 0.f ? std::log(s.layer_time) : 0.f; break;
      case 12: value = s.acceleration; break;
      default: break;
      }
      const ColorResult c = valueToGradient(value, minV, maxV);
      p.r = c.r;
      p.g = c.g;
      p.b = c.b;
    }
  }

  // Pack to binary
  gcodePreviewData_.resize(8 + count * int(sizeof(PackedSegment)));
  std::memcpy(gcodePreviewData_.data(), "GCV1", 4);
  std::memcpy(gcodePreviewData_.data() + 4, &count, 4);
  std::memcpy(gcodePreviewData_.data() + 8, packed.data(), size_t(count) * sizeof(PackedSegment));

  buildLegendItems(mode, minV, maxV);
}

void PreviewViewModel::buildLegendItems(int mode, float minV, float maxV)
{
  legendItems_.clear();
  m_legendType = 0; // default: discrete
  m_legendGradMinLabel.clear();
  m_legendGradMaxLabel.clear();
  m_legendGradMinColor.clear();
  m_legendGradMaxColor.clear();

  if (mode == 0)
  {
    // FeatureType legend (discrete)
    static const QHash<QString, QString> colorMap = {
        {QStringLiteral("外壁"), QStringLiteral("#FF8C3A")},
        {QStringLiteral("填充"), QStringLiteral("#38A4FF")},
        {QStringLiteral("支撑"), QStringLiteral("#8C63FF")},
        {QStringLiteral("空驶"), QStringLiteral("#6E7785")},
        {QStringLiteral("其他"), QStringLiteral("#53D890")}};
    for (auto it = featureCount_.cbegin(); it != featureCount_.cend(); ++it)
      legendItems_.append(legendItem(it.key(), colorMap.value(it.key(), QStringLiteral("#53D890")), it.value()));
  }
  else if (mode == 3 || mode == 7 || mode == 8)
  {
    // Tool / ColorPrint / FilamentId legend: extruder palette (对齐上游 extruder_colors)
    m_legendType = 2; // extruder palette
    QSet<int> usedIds;
    for (const auto &s : segments_)
      usedIds.insert(s.extruder_id);
    QList<int> sortedIds = usedIds.values();
    std::sort(sortedIds.begin(), sortedIds.end());
    static const float toolColors[][3] = {
        {0.95f, 0.55f, 0.22f},
        {0.22f, 0.55f, 0.87f},
        {0.30f, 0.69f, 0.46f},
        {0.61f, 0.35f, 0.71f},
        {0.91f, 0.30f, 0.24f},
        {0.10f, 0.74f, 0.61f},
        {0.95f, 0.77f, 0.06f},
        {0.91f, 0.12f, 0.55f}};
    static constexpr int kN = int(sizeof(toolColors) / sizeof(toolColors[0]));
    for (int id : sortedIds)
    {
      const auto &tc = toolColors[id % kN];
      const QString col = QStringLiteral("#%1%2%3")
          .arg(int(tc[0]*255), 2, 16, QLatin1Char('0'))
          .arg(int(tc[1]*255), 2, 16, QLatin1Char('0'))
          .arg(int(tc[2]*255), 2, 16, QLatin1Char('0'));
      const QString label = QStringLiteral("Extruder %1").arg(id);
      int cnt = 0;
      for (const auto &s : segments_)
        if (s.extruder_id == id) ++cnt;
      legendItems_.append(legendItem(label, col, cnt));
    }
  }
  else
  {
    // Gradient legend (对齐上游 Range_Colors 10-stop bluish→reddish gradient)
    m_legendType = 1; // gradient
    QString label;
    switch (mode)
    {
    case 1:  label = QStringLiteral("高度"); break;
    case 2:  label = QStringLiteral("线宽"); break;
    case 4:  label = QStringLiteral("进给速率"); break;
    case 5:  label = QStringLiteral("风扇转速"); break;
    case 6:  label = QStringLiteral("温度"); break;
    case 9:  label = QStringLiteral("体积速率"); break;
    case 10: label = QStringLiteral("层时间"); break;
    case 11: label = QStringLiteral("层时间(对数)"); break;
    case 12: label = QStringLiteral("加速度"); break;
    default: label = viewModes().value(mode); break;
    }

    const QString minStr = (minV <= FLT_MAX) ? QString::number(minV, 'f', 1) : QStringLiteral("--");
    const QString maxStr = (maxV >= -FLT_MAX) ? QString::number(maxV, 'f', 1) : QStringLiteral("--");

    // Upstream Range_Colors endpoints: #0b2c7a (bluish) → #942616 (reddish)
    static const QColor kGradStart(11, 44, 122);
    static const QColor kGradEnd(148, 38, 22);

    m_legendGradMinLabel = minStr;
    m_legendGradMaxLabel = maxStr;
    m_legendGradMinColor = QStringLiteral("#%1%2%3")
        .arg(kGradStart.red(), 2, 16, QLatin1Char('0'))
        .arg(kGradStart.green(), 2, 16, QLatin1Char('0'))
        .arg(kGradStart.blue(), 2, 16, QLatin1Char('0'));
    m_legendGradMaxColor = QStringLiteral("#%1%2%3")
        .arg(kGradEnd.red(), 2, 16, QLatin1Char('0'))
        .arg(kGradEnd.green(), 2, 16, QLatin1Char('0'))
        .arg(kGradEnd.blue(), 2, 16, QLatin1Char('0'));

    // Still populate legendItems_ with min/max for backward compat
    legendItems_.append(legendItem(minStr, m_legendGradMinColor, 0));
    legendItems_.append(legendItem(maxStr, m_legendGradMaxColor, 0));
  }
}

QVariantList PreviewViewModel::tickMarks() const
{
  QVariantList result;
  for (const auto& tc : tickMarks_) {
    QVariantMap m;
    m[QStringLiteral("tick")] = tc.tick;
    m[QStringLiteral("type")] = static_cast<int>(tc.type);
    m[QStringLiteral("extruder")] = tc.extruder;
    m[QStringLiteral("color")] = tc.color;
    m[QStringLiteral("extra")] = tc.extra;
    result.append(m);
  }
  return result;
}

int PreviewViewModel::tickMarkCount() const
{
  return tickMarks_.size();
}

void PreviewViewModel::addPauseAtLayer(int layer)
{
  for (const auto& tc : tickMarks_) {
    if (tc.tick == layer) {
      qWarning("addPauseAtLayer: tick already exists at layer %d", layer);
      return;
    }
  }
  OWzx::TickCode tc;
  tc.tick = layer;
  tc.type = OWzx::TickType::PausePrint;
  tickMarks_.append(tc);
  std::sort(tickMarks_.begin(), tickMarks_.end());
  emit tickMarksChanged();
}

void PreviewViewModel::addCustomGcodeAtLayer(int layer, const QString& gcode)
{
  for (const auto& tc : tickMarks_) {
    if (tc.tick == layer) {
      qWarning("addCustomGcodeAtLayer: tick already exists at layer %d", layer);
      return;
    }
  }
  OWzx::TickCode tc;
  tc.tick = layer;
  tc.type = OWzx::TickType::CustomGcode;
  tc.extra = gcode;
  tickMarks_.append(tc);
  std::sort(tickMarks_.begin(), tickMarks_.end());
  emit tickMarksChanged();
}

void PreviewViewModel::removeTickAtLayer(int layer)
{
  for (int i = 0; i < tickMarks_.size(); ++i) {
    if (tickMarks_[i].tick == layer) {
      tickMarks_.removeAt(i);
      emit tickMarksChanged();
      return;
    }
  }
}

void PreviewViewModel::editCustomGcodeAtLayer(int layer, const QString& newGcode)
{
  for (int i = 0; i < tickMarks_.size(); ++i) {
    if (tickMarks_[i].tick == layer) {
      tickMarks_[i].extra = newGcode;
      emit tickMarksChanged();
      return;
    }
  }
  qWarning("editCustomGcodeAtLayer: no tick at layer %d", layer);
}

void PreviewViewModel::addFilamentChangeAtLayer(int layer, int extruderId)
{
  for (const auto& tc : tickMarks_) {
    if (tc.tick == layer) {
      qWarning("addFilamentChangeAtLayer: tick already exists at layer %d", layer);
      return;
    }
  }
  OWzx::TickCode tc;
  tc.tick = layer;
  tc.type = OWzx::TickType::ToolChange;
  tc.extruder = extruderId;
  tickMarks_.append(tc);
  std::sort(tickMarks_.begin(), tickMarks_.end());
  emit tickMarksChanged();
}

QVariantMap PreviewViewModel::tickAtLayer(int layer) const
{
  for (const auto& tc : tickMarks_) {
    if (tc.tick == layer) {
      QVariantMap m;
      m[QStringLiteral("tick")] = tc.tick;
      m[QStringLiteral("type")] = static_cast<int>(tc.type);
      m[QStringLiteral("extruder")] = tc.extruder;
      m[QStringLiteral("color")] = tc.color;
      m[QStringLiteral("extra")] = tc.extra;
      return m;
    }
  }
  return QVariantMap();
}

void PreviewViewModel::clearAllTicks()
{
  if (tickMarks_.isEmpty())
    return;
  tickMarks_.clear();
  emit tickMarksChanged();
}
