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
  // Aligned with upstream short_and_splitted_time.
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
    int role;  // CANONICAL libvgcode EGCodeExtrusionRole index (0..19) -- must match GcvPackedSegment exactly.
  };
  // Producer-side wire-format lockstep guard (Phase 55 code-review Warning fix:
  // the static_assert previously existed only on the renderer side). PackedSegment
  // must be byte-identical to GcvPackedSegment in RhiViewportRenderer.cpp; a
  // layout drift here would silently corrupt the GCV1 preview blob at runtime.
  static_assert(sizeof(PackedSegment) == 76, "PackedSegment must be 76 bytes (matches GcvPackedSegment)");

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

  // Maps the upstream ;TYPE: display string DIRECTLY to the canonical libvgcode
  // EGCodeExtrusionRole index (the canonical role index throughout the Qt6 codebase).
  // Source strings: libslic3r/ExtrusionEntity.cpp:583-608 (role_to_string).
  // Target indices:  libvgcode/include/Types.hpp:131-157 (EGCodeExtrusionRole).
  // The two enums DIVERGE past index 6 -- do NOT translate via the libslic3r integer.
  struct RoleMapEntry { const char *name; int role; };
  static const RoleMapEntry kRoleMap[] = {
      {"Inner wall",              1},  // Perimeter
      {"Outer wall",              2},  // ExternalPerimeter
      {"Overhang wall",           3},  // OverhangPerimeter
      {"Sparse infill",           4},  // InternalInfill
      {"Internal solid infill",   5},  // SolidInfill
      {"Top surface",             6},  // TopSolidInfill
      {"Ironing",                 7},  // Ironing             (NOT 8 -- libslic3r idx)
      {"Bridge",                  8},  // BridgeInfill        (NOT 9)
      {"Gap infill",              9},  // GapFill             (NOT 11)
      {"Skirt",                  10},  // Skirt               (NOT 12)
      {"Support",                11},  // SupportMaterial     (NOT 14)
      {"Support interface",      12},  // SupportMaterialInterface (NOT 15)
      {"Prime tower",            13},  // WipeTower           (NOT 17)
      {"Custom",                 14},  // Custom              (NOT 18)
      {"Bottom surface",         15},  // BottomSurface       (NOT 7)
      {"Internal Bridge",        16},  // InternalBridgeInfill(NOT 10)
      {"Brim",                   17},  // Brim                (NOT 13)
      {"Support transition",     18},  // SupportTransition   (NOT 16)
      {"Multiple",               19},  // Mixed               (identical in both enums)
  };

  // Role default colors from upstream DEFAULT_EXTRUSION_ROLES_COLORS.
  // Source: libvgcode/src/ViewerImpl.cpp:283-305.
  // Indexed by CANONICAL libvgcode EGCodeExtrusionRole (matches kRoleMap output).
  static const float kRoleColors[][3] = {
      {230 / 255.f, 179 / 255.f, 179 / 255.f}, // 0  None
      {255 / 255.f, 230 / 255.f,  77 / 255.f}, // 1  Perimeter
      {255 / 255.f, 125 / 255.f,  56 / 255.f}, // 2  ExternalPerimeter
      { 31 / 255.f,  31 / 255.f, 255 / 255.f}, // 3  OverhangPerimeter
      {176 / 255.f,  48 / 255.f,  41 / 255.f}, // 4  InternalInfill
      {150 / 255.f,  84 / 255.f, 204 / 255.f}, // 5  SolidInfill
      {240 / 255.f,  64 / 255.f,  64 / 255.f}, // 6  TopSolidInfill
      {255 / 255.f, 140 / 255.f, 105 / 255.f}, // 7  Ironing
      { 77 / 255.f, 128 / 255.f, 186 / 255.f}, // 8  BridgeInfill
      {255 / 255.f, 255 / 255.f, 255 / 255.f}, // 9  GapFill
      {  0 / 255.f, 135 / 255.f, 110 / 255.f}, // 10 Skirt
      {  0 / 255.f, 255 / 255.f,   0 / 255.f}, // 11 SupportMaterial
      {  0 / 255.f, 128 / 255.f,   0 / 255.f}, // 12 SupportMaterialInterface
      {179 / 255.f, 227 / 255.f, 171 / 255.f}, // 13 WipeTower
      { 94 / 255.f, 209 / 255.f, 148 / 255.f}, // 14 Custom
      {102 / 255.f,  92 / 255.f, 199 / 255.f}, // 15 BottomSurface
      { 77 / 255.f, 128 / 255.f, 186 / 255.f}, // 16 InternalBridgeInfill
      {  0 / 255.f,  59 / 255.f, 110 / 255.f}, // 17 Brim
      {  0 / 255.f,  64 / 255.f,   0 / 255.f}, // 18 SupportTransition
      {128 / 255.f, 128 / 255.f, 128 / 255.f}, // 19 Mixed
  };

  // Upstream display labels for the canonical libvgcode role index, used by
  // roleVisibilities() for the QML Repeater (English ASCII only).
  static const char *kRoleLabels[] = {
      "Unknown",            // 0  None
      "Inner wall",         // 1
      "Outer wall",         // 2
      "Overhang wall",      // 3
      "Sparse infill",      // 4
      "Internal solid infill", // 5
      "Top surface",        // 6
      "Ironing",            // 7
      "Bridge",             // 8
      "Gap infill",         // 9
      "Skirt",              // 10
      "Support",            // 11
      "Support interface",  // 12
      "Prime tower",        // 13
      "Custom",             // 14
      "Bottom surface",     // 15
      "Internal Bridge",    // 16
      "Brim",               // 17
      "Support transition", // 18
      "Multiple",           // 19
  };

  // Map an upstream ;TYPE: display string to its canonical libvgcode index.
  // Travel/unrecognized -> 0 (None). Never indexes kRoleColors out of bounds.
  int roleForTypeImpl(const QString &type)
  {
    const QString t = type.trimmed();
    for (const auto &entry : kRoleMap)
    {
      if (t.compare(QString::fromUtf8(entry.name), Qt::CaseSensitive) == 0)
        return entry.role;
    }
    return 0;
  }

  // Canonical view-mode indices matching upstream libvgcode EViewType order
  // (libvgcode/include/Types.hpp:80-103). Every mode-to-field mapping in
  // recolorAndPackSegments() and buildLegendItems() uses these named constants
  // instead of raw integers so the 17-mode renumber cannot silently mislabel a
  // gradient (55-RESEARCH Pitfall 2).
  enum EViewType
  {
    VT_Summary = 0,          // statistics only, no gradient legend
    VT_LineType = 1,         // FeatureType: per-role colors (kRoleColors)
    VT_Filament = 2,         // ColorPrint: per-extruder palette
    VT_Speed = 3,            // gradient on feedrate
    VT_ActualSpeed = 4,      // uniform (data unavailable in fixture-driven path)
    VT_Acceleration = 5,     // gradient on acceleration
    VT_Jerk = 6,             // uniform (data unavailable)
    VT_Height = 7,           // gradient on layer height
    VT_Width = 8,            // gradient on line width
    VT_Flow = 9,             // gradient on volumetric_rate
    VT_ActualFlow = 10,      // uniform (data unavailable)
    VT_LayerTime = 11,       // gradient on layer_time
    VT_LayerTimeLog = 12,    // gradient on log(layer_time)
    VT_FanSpeed = 13,        // gradient on fan_speed
    VT_Temperature = 14,     // gradient on temperature
    VT_PressureAdvance = 15, // uniform (data unavailable)
    VT_Tool = 16             // per-extruder palette
  };

  // One-time log guard for the modes whose underlying field is unavailable in
  // the fixture-driven path (Jerk/PA/ActualSpeed/ActualFlow). Logs once per mode
  // so users are informed without spamming on every recolor.
  bool logOnceIfNeeded(int mode)
  {
    static bool logged[4] = {false, false, false, false};
    static const int modes[4] = {VT_ActualSpeed, VT_Jerk, VT_ActualFlow, VT_PressureAdvance};
    for (int i = 0; i < 4; ++i)
    {
      if (modes[i] == mode && !logged[i])
      {
        logged[i] = true;
        qInfo("[Preview] Jerk/PA/ActualSpeed/ActualFlow data unavailable in fixture-driven path (mode=%d)", mode);
        return true;
      }
    }
    return false;
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
  // All extrusion roles visible by default, matching upstream extrusion_roles_visibility
  // (libvgcode/src/Settings.hpp:49-71). showTravelMoves_ defaults to false in the header.
  m_roleVisibility.fill(true);
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
    rebuildGcodeLineWindow();
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

bool PreviewViewModel::previewReady() const
{
  return !gcodePreviewData_.isEmpty() && moveCount_ > 0;
}

QString PreviewViewModel::previewStatusText() const
{
  if (slicing())
    return tr("正在切片，预览将在切片完成后更新");
  if (previewReady())
    return tr("预览已就绪");
  return tr("请先切片或载入 G-code");
}

QString PreviewViewModel::currentLayerLabel() const
{
  if (layerCount_ <= 0)
    return tr("-- / --");
  return tr("%1-%2 / %3")
      .arg(currentLayerMin_ + 1)
      .arg(currentLayerMax_ + 1)
      .arg(layerCount_);
}

QString PreviewViewModel::currentMoveLabel() const
{
  if (moveCount_ <= 0)
    return tr("-- / --");
  return tr("%1 / %2").arg(currentMove_).arg(moveCount_);
}

QString PreviewViewModel::plateSummary() const
{
  if (!sliceService_ || sliceService_->resultPlateIndex() < 0)
    return tr("当前盘");

  const QString label = sliceService_->resultPlateLabel();
  if (!label.isEmpty())
    return label;
  return tr("盘 %1").arg(sliceService_->resultPlateIndex() + 1);
}

QString PreviewViewModel::warningSummary() const
{
  if (!previewReady())
    return previewStatusText();
  if (travelMoveCount_ <= 0)
    return tr("未检测到空驶移动");
  return tr("空驶 %1，挤出 %2").arg(travelMoveCount_).arg(extrudeMoveCount_);
}

QString PreviewViewModel::currentTime() const
{
  // Look up accumulated time at the current move, aligned with upstream IMSlider::get_label.
  if (m_moveAccumulatedTime.empty() || currentMove_ <= 0)
    return QStringLiteral("0s");
  const int idx = qMin(currentMove_, int(m_moveAccumulatedTime.size()) - 1);
  return formatTime(m_moveAccumulatedTime[idx]);
}

QString PreviewViewModel::timeAtMove(int moveIndex) const
{
  // Look up accumulated time at an arbitrary move position, aligned with upstream IMSlider hover labels.
  if (m_moveAccumulatedTime.empty() || moveIndex <= 0)
    return QStringLiteral("0s");
  const int idx = qMin(moveIndex, int(m_moveAccumulatedTime.size()) - 1);
  return formatTime(m_moveAccumulatedTime[idx]);
}

QStringList PreviewViewModel::viewModes() const
{
  // The 17 upstream EViewType display names in upstream update_by_mode order
  // (libvgcode/include/Types.hpp:80-103 + GCodeViewer.cpp:66-103). Index order
  // matches the EViewType enum so viewModeIndex_ maps 1:1 to the recolor switch.
  return {
      QStringLiteral("Summary"),
      QStringLiteral("Line Type"),
      QStringLiteral("Filament"),
      QStringLiteral("Speed"),
      QStringLiteral("Actual Speed"),
      QStringLiteral("Acceleration"),
      QStringLiteral("Jerk"),
      QStringLiteral("Layer Height"),
      QStringLiteral("Line Width"),
      QStringLiteral("Flow"),
      QStringLiteral("Actual Flow"),
      QStringLiteral("Layer Time"),
      QStringLiteral("Layer Time (log)"),
      QStringLiteral("Fan Speed"),
      QStringLiteral("Temperature"),
      QStringLiteral("Pressure Advance"),
      QStringLiteral("Tool")};
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
  // Upstream IMSlider::do_go_to_layer receives 1-indexed input and converts it to 0-indexed state.
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
  // Clamp while preserving the current span.
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
  rebuildGcodeLineWindow();
  emit stateChanged();
}

void PreviewViewModel::stepCurrentMove(int delta)
{
  if (delta == 0)
    return;
  setCurrentMove(currentMove_ + delta);
}

void PreviewViewModel::updateToolPositionData()
{
  if (segments_.empty() || currentMove_ < 0) {
    hasToolPosition_ = false;
    return;
  }
  const int idx = qMin(currentMove_, static_cast<int>(segments_.size()) - 1);
  const auto &seg = segments_[idx];
  // Upstream GCodeViewer::Marker uses the endpoint of the current move.
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
  // Treat the current move as extrusion when the parser classified it as non-travel.
  toolIsExtrusion_ = !seg.isTravel;
}

void PreviewViewModel::rebuildGcodeLineWindow()
{
  gcodeLines_.clear();
  gcodeLineCount_ = m_gcodeSourceLines.size();
  currentGcodeLine_ = 0;

  if (m_gcodeSourceLines.isEmpty())
    return;

  int anchor = m_gcodeSourceLines.size() - 1;
  for (int i = 0; i < m_gcodeSourceLines.size(); ++i) {
    if (m_gcodeSourceLines[i].moveIndex >= currentMove_) {
      anchor = i;
      break;
    }
  }

  currentGcodeLine_ = m_gcodeSourceLines[anchor].lineNumber;
  const int start = qMax(0, anchor - 24);
  const int end = qMin(m_gcodeSourceLines.size() - 1, anchor + 28);
  for (int i = start; i <= end; ++i) {
    const SourceGcodeLine &source = m_gcodeSourceLines[i];
    QVariantMap row;
    row.insert(QStringLiteral("line"), source.lineNumber);
    row.insert(QStringLiteral("move"), source.moveIndex);
    row.insert(QStringLiteral("text"), source.text);
    row.insert(QStringLiteral("current"), i == anchor);
    gcodeLines_.append(row);
  }
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
  // Aligned with upstream PrintEstimatedStatistics::modes[1] stealth mode.
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

int PreviewViewModel::roleForType(const QString &type) const
{
  return roleForTypeImpl(type);
}

QColor PreviewViewModel::roleColor(int roleIndex) const
{
  if (roleIndex < 0 || roleIndex >= 20)
    roleIndex = 0;
  const auto &c = kRoleColors[roleIndex];
  return QColor::fromRgbF(c[0], c[1], c[2]);
}

bool PreviewViewModel::isRoleVisible(int roleIndex) const
{
  if (roleIndex < 0 || roleIndex >= int(m_roleVisibility.size()))
    return true;
  return m_roleVisibility[roleIndex];
}

void PreviewViewModel::toggleRoleVisibility(int roleIndex)
{
  if (roleIndex < 0 || roleIndex >= int(m_roleVisibility.size()))
    return;
  // Render-side filter only: flip the mask and emit stateChanged(). Does NOT
  // call recolorAndPackSegments() and does NOT mutate gcodePreviewData_
  // (Phase 41 interaction-stability invariant; the renderer skips masked spans).
  m_roleVisibility[roleIndex] = !m_roleVisibility[roleIndex];
  emit stateChanged();
}

QVariantList PreviewViewModel::roleVisibilities() const
{
  // Rows in ascending canonical libvgcode index order so the QML UI row order
  // is deterministic and matches the color-swatch assignment. None(0) and
  // Custom(14) are hidden per the UI-SPEC copywriting table but remain in the
  // m_roleVisibility array for safe indexing.
  QVariantList rows;
  static const int kExcludedRoles[] = {0, 14};  // None, Custom
  for (int role = 1; role < 20; ++role)
  {
    bool excluded = false;
    for (int ex : kExcludedRoles)
    {
      if (role == ex) { excluded = true; break; }
    }
    if (excluded)
      continue;
    const auto &c = kRoleColors[role];
    const QString color = QStringLiteral("#%1%2%3")
        .arg(qBound(0, int(c[0] * 255.f + 0.5f), 255), 2, 16, QLatin1Char('0'))
        .arg(qBound(0, int(c[1] * 255.f + 0.5f), 255), 2, 16, QLatin1Char('0'))
        .arg(qBound(0, int(c[2] * 255.f + 0.5f), 255), 2, 16, QLatin1Char('0'));
    QVariantMap row;
    row.insert(QStringLiteral("roleIndex"), role);
    row.insert(QStringLiteral("label"), QString::fromUtf8(kRoleLabels[role]));
    row.insert(QStringLiteral("color"), color);
    row.insert(QStringLiteral("visible"), m_roleVisibility[role]);
    rows.append(row);
  }
  return rows;
}

QVariantList PreviewViewModel::roleVisibilityMask() const
{
  // Dense 20-bool mask for the renderer's render-side role filter. The renderer
  // (RhiViewportRenderer::synchronize) expects a flat QVariantList of 20 bools
  // indexed by canonical libvgcode role; roleVisibilities() (18 QVariantMap
  // rows for the UI Repeater) has the wrong shape for that consumer. Bind THIS
  // property to GLViewport.roleVisibility, not roleVisibilities. (Phase 55
  // code-review Critical fix: the prior binding fed 18 maps into a consumer
  // that requires 20 bools, so the filter was a no-op.)
  QVariantList mask;
  mask.reserve(20);
  for (int i = 0; i < 20; ++i)
    mask.append(m_roleVisibility[i]);
  return mask;
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
  gcodeLineCount_ = 0;
  currentGcodeLine_ = 0;
  gcodeLines_.clear();
  m_gcodeSourceLines.clear();
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

  // Per-role time tracking aligned with upstream PrintEstimatedStatistics::roles_times.
  QHash<QString, double> roleTimeAccum; // TYPE label to accumulated time in seconds.
  auto accumulateRoleTime = [&](const QString &role, float dx, float dy, float dz, float feed) {
    if (feed <= 0.f) return;
    const float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    roleTimeAccum[role] += dist / feed * 60.0; // feed is mm/min, dist is mm to seconds.
  };

  int sourceLineNumber = 0;
  while (!file.atEnd())
  {
    const QString raw = QString::fromUtf8(file.readLine()).trimmed();
    ++sourceLineNumber;
    if (raw.isEmpty())
      continue;

    auto appendSourceLine = [&](int sourceMoveIndex) {
      SourceGcodeLine line;
      line.lineNumber = sourceLineNumber;
      line.moveIndex = sourceMoveIndex;
      line.text = raw;
      m_gcodeSourceLines.append(line);
    };

    if (raw.startsWith(';'))
    {
      appendSourceLine(qMax(0, moveIndex));
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
    if (command.isEmpty()) {
      appendSourceLine(qMax(0, moveIndex));
      continue;
    }

    const QString upper = command.toUpper();
    appendSourceLine(qMax(0, moveIndex));

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
          // Record the tool-change position for the upstream-style colored band.
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
      // Track filament usage per extruder, aligned with upstream PrintEstimatedStatistics.
      m_extruderUsedLength[currentExtruder] += extrusionDelta;
      accumulateRoleTime(currentType, dx, dy, dz, currentFeedrate);
    }
    else
    {
      ++travelMoveCount;
      accumulateRoleTime(QStringLiteral("TRAVEL"), dx, dy, dz, currentFeedrate);
    }
    // Fine-grained role assignment: map the ;TYPE: display string DIRECTLY to
    // the canonical libvgcode index and bake the per-role base color from
    // kRoleColors (FeatureType mode). Both arrays use the same canonical index,
    // so every role -- including the divergent ones (Ironing->7, Bottom
    // surface->15) -- gets the correct color and visibility slot.
    const int role = extruding ? roleForTypeImpl(currentType) : 0;

    StoredSegment seg;
    seg.x1 = x;
    seg.y1 = y;
    seg.z1 = z;
    seg.x2 = nx;
    seg.y2 = ny;
    seg.z2 = nz;
    seg.baseR = kRoleColors[role][0];
    seg.baseG = kRoleColors[role][1];
    seg.baseB = kRoleColors[role][2];
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
    seg.role = role;
    segments_.push_back(seg);

    // Accumulate elapsed time for the move slider, aligned with upstream IMSlider m_layers_times.
    {
      const float dt = currentFeedrate > 0.f ? dist / currentFeedrate * 60.f : 0.f;
      const float prev = m_moveAccumulatedTime.empty() ? 0.f : m_moveAccumulatedTime.back();
      m_moveAccumulatedTime.push_back(prev + dt);
    }

    featureCount_[QString::fromUtf8(kRoleLabels[role])] += 1;

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
    // Weight = volume_mm3 * density_g_per_cm3 * 1e-3.
    // volume_mm3 = length_mm * pi * (diameter / 2)^2.
    const float volume_mm3 = totalFilamentUsed * 3.14159265f * (filamentDiameter * 0.5f) * (filamentDiameter * 0.5f);
    const float weight_g = volume_mm3 * filamentDensity * 0.001f;
    filamentWeight_ = QStringLiteral("%1 g").arg(weight_g, 0, 'f', 1);

    // Compute per-extruder filament weight, aligned with upstream PrintEstimatedStatistics.
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

  // Build role time breakdown, aligned with upstream PrintEstimatedStatistics::roles_times.
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
  rebuildGcodeLineWindow();
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
  // Upstream extruder_colors: fixed 8-color cycle.
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
  return m_extruderUsedLength.value(extruderId, 0.0) / 1000.0; // mm to m
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

  // Determine value range for gradient modes. Uses the EViewType enum so the
  // 17-mode renumber cannot silently mislabel a gradient (55-RESEARCH Pitfall 2).
  // Summary / LineType / Filament / Tool / ActualSpeed / Jerk / ActualFlow /
  // PressureAdvance do not contribute to the gradient range.
  float minV = FLT_MAX, maxV = -FLT_MAX;
  for (const int idx : visibleIndices)
  {
    const auto &s = segments_[idx];
    float v = 0.f;
    switch (mode)
    {
    case VT_Height:       v = s.height; break;
    case VT_Width:        v = s.width; break;
    case VT_Speed:        v = s.feedrate; break;
    case VT_Acceleration: v = s.acceleration; break;
    case VT_Flow:         v = s.volumetric_rate; break;
    case VT_LayerTime:    v = s.layer_time; break;
    case VT_LayerTimeLog: v = s.layer_time > 0.f ? std::log(s.layer_time) : 0.f; break;
    case VT_FanSpeed:     v = qMax(0.f, s.fan_speed); break;
    case VT_Temperature:  v = s.temperature; break;
    default: continue;  // VT_Summary, VT_LineType, VT_Filament, VT_Tool, and the
                        // data-unavailable modes (ActualSpeed/Jerk/ActualFlow/PA)
                        // do not compute a gradient range.
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
    p.role = s.role;

    if (mode == VT_LineType)
    {
      // FeatureType: use the baked per-role base color (kRoleColors).
      p.r = s.baseR;
      p.g = s.baseG;
      p.b = s.baseB;
    }
    else if (mode == VT_Filament || mode == VT_Tool)
    {
      // Filament (ColorPrint) / Tool: fixed palette per extruder.
      const auto &tc = toolColors[s.extruder_id % kToolColorCount];
      p.r = tc[0]; p.g = tc[1]; p.b = tc[2];
    }
    else if (mode == VT_ActualSpeed || mode == VT_Jerk || mode == VT_ActualFlow || mode == VT_PressureAdvance)
    {
      // Data unavailable in the fixture-driven path: render a uniform mid-gradient
      // color. Log once per mode so users are informed without spamming.
      logOnceIfNeeded(mode);
      const ColorResult c = valueToGradient((minV + maxV) * 0.5f, minV, maxV);
      p.r = c.r; p.g = c.g; p.b = c.b;
    }
    else if (mode == VT_Summary)
    {
      // Summary: statistics only; segments still draw in their baked role color.
      p.r = s.baseR; p.g = s.baseG; p.b = s.baseB;
    }
    else
    {
      float value = 0.f;
      switch (mode)
      {
      case VT_Height:       value = s.height; break;
      case VT_Width:        value = s.width; break;
      case VT_Speed:        value = s.feedrate; break;
      case VT_Acceleration: value = s.acceleration; break;
      case VT_Flow:         value = s.volumetric_rate; break;
      case VT_LayerTime:    value = s.layer_time; break;
      case VT_LayerTimeLog: value = s.layer_time > 0.f ? std::log(s.layer_time) : 0.f; break;
      case VT_FanSpeed:     value = qMax(0.f, s.fan_speed); break;
      case VT_Temperature:  value = s.temperature; break;
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
  m_legendType = 0; // default: discrete / no legend
  m_legendGradMinLabel.clear();
  m_legendGradMaxLabel.clear();
  m_legendGradMinColor.clear();
  m_legendGradMaxColor.clear();

  if (mode == VT_Summary)
  {
    // Summary: statistics only -- no gradient legend (upstream EViewType::Summary).
    // m_legendType stays 0 (discrete) and legendItems_ stays empty.
    return;
  }

  if (mode == VT_LineType)
  {
    // FeatureType legend (discrete): per-role swatches indexed by the canonical
    // libvgcode role present in the parsed segments.
    for (auto it = featureCount_.cbegin(); it != featureCount_.cend(); ++it)
    {
      // Look up the role color by matching the feature label to kRoleLabels.
      QString color = QStringLiteral("#53D890");
      for (int r = 0; r < 20; ++r)
      {
        if (it.key() == QString::fromUtf8(kRoleLabels[r]))
        {
          const auto &c = kRoleColors[r];
          color = QStringLiteral("#%1%2%3")
              .arg(qBound(0, int(c[0] * 255.f + 0.5f), 255), 2, 16, QLatin1Char('0'))
              .arg(qBound(0, int(c[1] * 255.f + 0.5f), 255), 2, 16, QLatin1Char('0'))
              .arg(qBound(0, int(c[2] * 255.f + 0.5f), 255), 2, 16, QLatin1Char('0'));
          break;
        }
      }
      legendItems_.append(legendItem(it.key(), color, it.value()));
    }
  }
  else if (mode == VT_Filament || mode == VT_Tool)
  {
    // Filament (ColorPrint) / Tool legend: extruder palette.
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
    // Gradient legend aligned with upstream Range_Colors.
    m_legendType = 1; // gradient
    QString label;
    switch (mode)
    {
    case VT_Height:       label = QStringLiteral("Layer Height"); break;
    case VT_Width:        label = QStringLiteral("Line Width"); break;
    case VT_Speed:        label = QStringLiteral("Speed"); break;
    case VT_Acceleration: label = QStringLiteral("Acceleration"); break;
    case VT_Flow:         label = QStringLiteral("Flow"); break;
    case VT_LayerTime:    label = QStringLiteral("Layer Time"); break;
    case VT_LayerTimeLog: label = QStringLiteral("Layer Time (log)"); break;
    case VT_FanSpeed:     label = QStringLiteral("Fan Speed"); break;
    case VT_Temperature:  label = QStringLiteral("Temperature"); break;
    // Data-unavailable modes render a uniform gradient; label by mode name.
    case VT_ActualSpeed:      label = QStringLiteral("Actual Speed"); break;
    case VT_Jerk:             label = QStringLiteral("Jerk"); break;
    case VT_ActualFlow:       label = QStringLiteral("Actual Flow"); break;
    case VT_PressureAdvance:  label = QStringLiteral("Pressure Advance"); break;
    default: label = viewModes().value(mode); break;
    }

    const QString minStr = (minV <= FLT_MAX) ? QString::number(minV, 'f', 1) : QStringLiteral("--");
    const QString maxStr = (maxV >= -FLT_MAX) ? QString::number(maxV, 'f', 1) : QStringLiteral("--");

    // Upstream Range_Colors endpoints: #0b2c7a (bluish) to #942616 (reddish).
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
