#include "PreviewViewModel.h"

#include "core/services/SliceService.h"
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
    const QRegularExpression re(QStringLiteral("(?:^|\\s)%1(-?\\d+(?:\\.\\d+)?)").arg(axis));
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
    emit stateChanged(); });

  connect(sliceService_, &SliceService::progressChanged, this, &PreviewViewModel::stateChanged);
  connect(sliceService_, &SliceService::slicingChanged, this, &PreviewViewModel::stateChanged);
  connect(sliceService_, &SliceService::sliceFinished, this, [this](const QString &time)
          {
        estimatedTime_ = time;
        totalTime_ = time;
        rebuildFromGCode(sliceService_->outputPath());
        emit stateChanged(); });
  connect(sliceService_, &SliceService::sliceFailed, this, [this](const QString &)
          {
    playTimer_->stop();
    currentMove_ = 0;
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
  return {
      QStringLiteral("FeatureType"),
      QStringLiteral("Height"),
      QStringLiteral("Width"),
      QStringLiteral("Tool"),
      QStringLiteral("Feedrate"),
      QStringLiteral("FanSpeed"),
      QStringLiteral("Temperature"),
      QStringLiteral("ColorPrint"),
      QStringLiteral("FilamentId"),
      QStringLiteral("VolumetricRate"),
      QStringLiteral("LayerTime"),
      QStringLiteral("LayerTimeLog"),
      QStringLiteral("Acceleration")};
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
  if (segments_.empty() || currentMove_ < 0 || currentMove_ >= static_cast<int>(segments_.size())) {
    hasToolPosition_ = false;
    return;
  }
  const auto &seg = segments_[currentMove_];
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
  toolIsExtrusion_ = (seg.x1 != seg.x2 || seg.y1 != seg.y2 || seg.z1 != seg.z2) && seg.feedrate > 0;
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

void PreviewViewModel::rebuildFromGCode(const QString &filePath)
{
  gcodePreviewData_.clear();
  legendItems_.clear();
  segments_.clear();
  featureCount_.clear();
  m_layerTimes.clear();
  m_layerZs.clear();
  m_toolChangePositions.clear();
  m_extruderUsedLength.clear();
  m_extruderUsedWeight.clear();
  m_maxLayerTime = 0.f;
  layerCount_ = 0;
  moveCount_ = 0;
  currentMove_ = 0;
  currentLayerMin_ = 0;
  currentLayerMax_ = 0;
  filamentUsed_ = QStringLiteral("--");
  filamentWeight_ = QStringLiteral("--");
  extrudeMoveCount_ = 0;
  travelMoveCount_ = 0;
  toolChangeCount_ = 0;
  avgSpeed_ = QStringLiteral("--");
  estimatedCost_ = QStringLiteral("--");
  m_roleTimes.clear();
  m_moveAccumulatedTime.clear();

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
  float currentWidth = 0.f;
  float layerTime = 0.f;
  float prevLayerZ = 0.f;
  int layerStartSeg = 0;

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
      if (raw.contains(QStringLiteral("M106")))
        currentFanSpeed = parseSValue(raw);
      else if (raw.contains(QStringLiteral("M104")))
        currentTemp = parseSValue(raw);
      if (raw.contains(QStringLiteral("TIME_ELAPSED")))
      {
        const QRegularExpression re(QStringLiteral("TIME_ELAPSED[=:]\\s*(\\d+(?:\\.\\d+)?)"));
        const auto m = re.match(raw);
        if (m.hasMatch())
        {
          bool ok = false;
          layerTime = m.captured(1).toFloat(&ok);
        }
      }
      if (raw.contains(QStringLiteral("WIDTH")))
      {
        const QRegularExpression re(QStringLiteral("WIDTH[=:]\\s*(\\d+(?:\\.\\d+)?)"));
        const auto m = re.match(raw);
        if (m.hasMatch())
        {
          bool ok = false;
          currentWidth = m.captured(1).toFloat(&ok);
        }
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

    // Tool change command
    if (raw.startsWith('T') && raw.length() >= 2)
    {
      bool ok = false;
      const int tid = raw.mid(1).toInt(&ok);
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
    if (raw.startsWith(QStringLiteral("M204")))
    {
      float val = 0.f;
      if (parseAxis(raw, 'X', val) && val > 0.f) currentAccel = val;
      else if (parseAxis(raw, 'Y', val) && val > 0.f) currentAccel = val;
      else if (parseAxis(raw, 'Z', val) && val > 0.f) currentAccel = val;
      else if (parseAxis(raw, 'E', val) && val > 0.f) currentAccel = val;
      else if (parseAxis(raw, 'S', val) && val > 0.f) currentAccel = val;
    }

    if (!raw.startsWith("G0") && !raw.startsWith("G1"))
      continue;

    const float lineF = parseFValue(raw);
    if (lineF > 0.f)
      currentFeedrate = lineF;

    // Track feedrate for average speed calculation (extrusion moves only)
    if (raw.contains('E') && currentFeedrate > 0.f)
    {
      feedrateSum += currentFeedrate;
      ++feedrateCount;
    }

    float nx = x;
    float ny = y;
    float nz = z;
    float ne = e;
    parseAxis(raw, 'X', nx);
    parseAxis(raw, 'Y', ny);
    parseAxis(raw, 'Z', nz);
    parseAxis(raw, 'E', ne);

    const bool moved = (nx != x) || (ny != y) || (nz != z);
    if (!moved)
    {
      x = nx;
      y = ny;
      z = nz;
      e = ne;
      continue;
    }

    if (nz > z + 0.0001f)
    {
      // Save previous layer's elapsed time
      if (layer > 0 || m_layerTimes.isEmpty())
      {
        m_layerTimes.append(layerTime);
        m_maxLayerTime = qMax(m_maxLayerTime, layerTime);
      }
      ++layer;
      m_layerZs.append(nz); // 对齐上游 IMSlider hover tooltip 显示层 Z 高度
      // Compute layer time from TIME_ELAPSED at layer transitions
      layerTime = 0.f;
      prevLayerZ = nz;
      layerStartSeg = int(segments_.size());
    }

    const bool extruding = (ne > e + 0.00001f);
    if (extruding)
    {
      const float extrusionDelta = ne - e;
      totalFilamentUsed += extrusionDelta;
      ++extrudeMoveCount;
      // 每挤出机耗材追踪（对齐上游 PrintEstimatedStatistics volumes_per_extruder）
      m_extruderUsedLength[currentExtruder] += extrusionDelta;
      accumulateRoleTime(currentType, nx - x, ny - y, nz - z, currentFeedrate);
    }
    else
    {
      ++travelMoveCount;
      accumulateRoleTime(QStringLiteral("TRAVEL"), nx - x, ny - y, nz - z, currentFeedrate);
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
    seg.layer_time = layerTime;
    seg.acceleration = currentAccel;
    seg.extruder_id = currentExtruder;
    seg.layer = layer;
    seg.move = moveIndex;
    segments_.push_back(seg);

    // Accumulate elapsed time for move slider (对齐上游 IMSlider m_layers_times)
    {
      const float dist = std::sqrt((nx - x) * (nx - x) + (ny - y) * (ny - y) + (nz - z) * (nz - z));
      const float dt = currentFeedrate > 0.f ? dist / currentFeedrate * 60.f : 0.f;
      const float prev = m_moveAccumulatedTime.empty() ? 0.f : m_moveAccumulatedTime.back();
      m_moveAccumulatedTime.push_back(prev + dt);
    }

    featureCount_[style.label] += 1;

    x = nx;
    y = ny;
    z = nz;
    e = ne;
    ++moveIndex;
  }

  moveCount_ = moveIndex;
  layerCount_ = qMax(1, layer + 1);

  // Save last layer's time
  m_layerTimes.append(layerTime);
  m_maxLayerTime = qMax(m_maxLayerTime, layerTime);

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

  const int count = int(segments_.size());
  const int mode = viewModeIndex_;

  // Determine value range for gradient modes
  float minV = FLT_MAX, maxV = -FLT_MAX;
  for (const auto &s : segments_)
  {
    float v = 0.f;
    switch (mode)
    {
    case 1:  v = s.z1; break;         // Height (Z position)
    case 2:  v = s.width; break;      // Width
    case 4:  v = s.feedrate; break;    // Feedrate
    case 5:  v = qMax(0.f, s.fan_speed); break; // FanSpeed
    case 6:  v = s.temperature; break;  // Temperature
    case 3:  v = float(s.extruder_id); break; // Tool
    case 7:  v = float(s.extruder_id); break; // ColorPrint (via extruder)
    case 8:  v = float(s.extruder_id); break; // FilamentId
    case 9:  v = s.feedrate * s.width; break;   // VolumetricRate (proxy)
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
    const auto &s = segments_[i];
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

    if (mode == 0 || maxV <= minV)
    {
      // FeatureType or no valid range: use base colors
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
      case 1:  value = s.z1; break;
      case 2:  value = s.width; break;
      case 4:  value = s.feedrate; break;
      case 5:  value = qMax(0.f, s.fan_speed); break;
      case 6:  value = s.temperature; break;
      case 9:  value = s.feedrate * s.width; break;
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
      if (s.extruder_id > 0 || usedIds.isEmpty())
        usedIds.insert(s.extruder_id);
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
    for (int id : usedIds)
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

    // Upstream Range_Colors endpoints: #0d3264 (bluish) → #c22525 (reddish)
    static const QColor kGradStart(13, 50, 100);
    static const QColor kGradEnd(194, 37, 37);

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
