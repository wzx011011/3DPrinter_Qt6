#include "PreviewViewModel.h"

#include "core/services/SliceService.h"
#include "core/services/ProjectService.h"
#ifdef HAS_LIBSLIC3R
// Phase 118 (TICK-02/TICK-03): libslic3r custom-g-code write-back path.
#include <libslic3r/CustomGCode.hpp>
#include <libslic3r/Model.hpp>
// P1 (preview direct hand-off): structured slice result + ExtrusionRole for
// the canonical-role translation below.
#include <libslic3r/GCode/GCodeProcessor.hpp>
#include <libslic3r/ExtrusionEntity.hpp>
#endif
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QSettings>
#include <QTimer>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QColor>
#include <cstring>
#include <cfloat>
#include <cmath>
#include <algorithm>

namespace
{
  // 上游无时间轴播放，OWzx 增强：upstream GCodeViewer only owns the two
  // IMSlider handles (GCodeViewer.hpp:214-215) and IMSlider.hpp carries no
  // play/pause facility, so this playback cadence is OWzx-native. Base 1.0x
  // interval 24ms; the speed multiplier is clamped to [0.25, 8] (setPlaybackSpeed).
  constexpr int kPlaybackBaseIntervalMs = 24;
  constexpr double kPlaybackSpeedMin = 0.25;
  constexpr double kPlaybackSpeedMax = 8.0;

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

  // Tooltip "Type" row: upstream to_string(EMoveType)
  // (GCodeViewer.cpp:277-294), keyed by the Qt6 MoveKind classification.
  QString moveTypeToString(int kind)
  {
    switch (kind)
    {
      case 0: return QStringLiteral("Extrude");     // KindExtrude
      case 1: return QStringLiteral("Travel");      // KindTravel
      case 2: return QStringLiteral("Retract");     // KindRetract
      case 3: return QStringLiteral("Unretract");   // KindUnretract
      case 4: return QStringLiteral("Wipe");        // KindWipe
      case 5: return QStringLiteral("Seam");        // KindSeam
      default: return QStringLiteral("Unknown");
    }
  }

  // Tooltip "Line Type" row + FeatureType detail: upstream
  // to_string(EGCodeExtrusionRole) (GCodeViewer.cpp:296-322). Kept separate
  // from kRoleLabels so the legend labels stay untouched.
  QString moveRoleToString(int role)
  {
    static const QList<QString> labels = [] {
      QList<QString> l;
      l.reserve(21);
      l.push_back(QStringLiteral("Unknown"));               // 0  None
      l.push_back(QStringLiteral("Inner wall"));            // 1  Perimeter
      l.push_back(QStringLiteral("Outer wall"));            // 2  ExternalPerimeter
      l.push_back(QStringLiteral("Overhang wall"));         // 3  OverhangPerimeter
      l.push_back(QStringLiteral("Sparse infill"));         // 4  InternalInfill
      l.push_back(QStringLiteral("Internal solid infill")); // 5  SolidInfill
      l.push_back(QStringLiteral("Top surface"));           // 6  TopSolidInfill
      l.push_back(QStringLiteral("Ironing"));               // 7  Ironing
      l.push_back(QStringLiteral("Bridge"));                // 8  BridgeInfill
      l.push_back(QStringLiteral("Gap infill"));            // 9  GapFill
      l.push_back(QStringLiteral("Skirt"));                 // 10 Skirt
      l.push_back(QStringLiteral("Support"));               // 11 SupportMaterial
      l.push_back(QStringLiteral("Support interface"));     // 12 SupportMaterialInterface
      l.push_back(QStringLiteral("Prime tower"));           // 13 WipeTower
      l.push_back(QStringLiteral("Custom"));                // 14 Custom
      l.push_back(QStringLiteral("Bottom surface"));        // 15 BottomSurface
      l.push_back(QStringLiteral("Internal bridge"));       // 16 InternalBridgeInfill
      l.push_back(QStringLiteral("Brim"));                  // 17 Brim
      l.push_back(QStringLiteral("Support transition"));    // 18 SupportTransition
      l.push_back(QStringLiteral("Mixed"));                 // 19 Mixed
      return l;
    }();
    return (role >= 0 && role < labels.size()) ? labels[role] : labels[0];
  }

  // Upstream get_time_dhms (libslic3r/Utils.hpp:554-577): "DDd HHh MMm SSs".
  QString formatTimeDhms(float secs)
  {
    if (secs < 0.f)
      secs = 0.f;
    int days = int(secs / 86400.f);
    secs -= float(days) * 86400.f;
    int hours = int(secs / 3600.f);
    secs -= float(hours) * 3600.f;
    int minutes = int(secs / 60.f);
    secs -= float(minutes) * 60.f;
    if (days > 0)
      return QStringLiteral("%1d %2h %3m %4s").arg(days).arg(hours).arg(minutes).arg(int(secs));
    if (hours > 0)
      return QStringLiteral("%1h %2m %3s").arg(hours).arg(minutes).arg(int(secs));
    if (minutes > 0)
      return QStringLiteral("%1m %2s").arg(minutes).arg(int(secs));
    if (secs > 1.f)
      return QStringLiteral("%1s").arg(int(secs));
    return QStringLiteral("%1s").arg(double(secs), 0, 'f', 6);
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
    float height;           // P17.2: extrusion height for the solid-prism render
    float layer_time;
    float acceleration;
    float jerk;              // v5.11: M205 jerk (mm/s)
    float pressure_advance;  // v5.11: M900 / SET_PRESSURE_ADVANCE PA value
    float actual_speed;      // v5.11: feedrate * M220 speed factor (actual mm/s)
    float actual_flow;       // v5.11: M221 flow factor percent
    int extruder_id;
    int layer;
    int move;
    int role;  // CANONICAL libvgcode EGCodeExtrusionRole index (0..19) -- must match GcvPackedSegment exactly.
  };
  // Producer-side wire-format lockstep guard (Phase 55 code-review Warning fix:
  // the static_assert previously existed only on the renderer side). PackedSegment
  // must be byte-identical to GcvPackedSegment in RhiViewportRenderer.cpp; a
  // layout drift here would silently corrupt the GCV1 preview blob at runtime.
  static_assert(sizeof(PackedSegment) == 96, "PackedSegment must be 96 bytes (20 floats + 4 ints, matches GcvPackedSegment)");

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
  // PERF(PREV-PARSE): the lookup hash is built once; the previous per-call
  // loop constructed up to 20 QStrings per invocation and this runs 2-3x per
  // parsed move (376k moves on the 400k bench sphere).
  int roleForTypeImpl(const QString &type)
  {
    static const QHash<QString, int> roleHash = [] {
      QHash<QString, int> m;
      m.reserve(int(sizeof(kRoleMap) / sizeof(kRoleMap[0])));
      for (const auto &entry : kRoleMap)
        m.insert(QString::fromUtf8(entry.name), entry.role);
      return m;
    }();
    return roleHash.value(type.trimmed(), 0);
  }

  // Phase 238 (PREV-03): base colors for the non-extrusion move kinds.
  // Travel = upstream Travel_Colors[0] "Move", Retract/Unretract/Seam =
  // upstream Options_Colors (GCodeViewer.cpp:718-727), Wipe = upstream
  // Wipe_Color YELLOW (GCodeViewer.cpp:750). Upstream renders retract/
  // unretract/seam as GL_POINTS with these colors; the Qt6 line pipeline
  // renders them as small vertical tick segments (documented delta -- the
  // RHI abstraction has no glPointSize equivalent).
  static const float kTravelColor[3]  = {0.219f, 0.282f, 0.609f};
  static const float kRetractColor[3] = {0.803f, 0.135f, 0.839f};
  static const float kUnretractColor[3] = {0.287f, 0.679f, 0.810f};
  static const float kSeamColor[3]    = {0.900f, 0.900f, 0.900f};
  static const float kWipeColor[3]    = {1.000f, 1.000f, 0.000f};

  // Height of the vertical tick segment used to render zero-displacement
  // moves (retract/unretract/seam) in the line pipeline.
  constexpr float kMarkerTickHeight = 0.6f;

  // Phase 238 (PREV-03): base color for a non-extrusion move kind.
  const float *kindBaseColor(int kind)
  {
    switch (kind)
    {
      case 1: return kTravelColor;    // Travel
      case 2: return kRetractColor;   // Retract
      case 3: return kUnretractColor; // Unretract
      case 4: return kWipeColor;      // Wipe
      case 5: return kSeamColor;      // Seam
      default: return kRoleColors[0]; // None
    }
  }

  // ── P1 review follow-up: single-source symbols shared by BOTH preview
  // entries (rebuildFromGCode text parse + loadFromProcessorResult direct
  // walk) so the dual-entry products cannot drift apart. ──
  // Upstream travel_color(path) tri-state by delta_extruder sign
  // (GCodeViewer.cpp:3234-3236): >0 Extrude / <0 Retract, else plain Move.
  static const float kTravelExtrudeColor[3] = {0.112f, 0.422f, 0.103f};
  static const float kTravelRetractColor[3] = {0.505f, 0.064f, 0.028f};

  // Fallback filament scalars when neither the gcode config block (text
  // path) nor the structured result (direct path) supplies a value.
  constexpr float kDefaultFilamentDiameter = 1.75f;  // mm
  constexpr float kDefaultFilamentDensity = 1.24f;   // g/cm3 (PLA default)

  // Filament cross-section area mm^2 = pi*(d/2)^2 -- the shared formula
  // behind the volumetric-rate, weight and split conversions.
  inline float filamentCrossAreaMm2(float diameterMm)
  {
    return 3.14159265f * (diameterMm * 0.5f) * (diameterMm * 0.5f);
  }

  // Phase 238 (PREV-05) category split, ported from upstream
  // GCodeProcessor.cpp:3002-3010: support roles (SupportMaterial 11 /
  // SupportMaterialInterface 12 / SupportTransition 18) -> support,
  // WipeTower (Prime tower, 13) -> tower, everything else -> model.
  // Returns the m_filamentSplitLength index: 0=model, 1=support, 3=tower
  // (2=flushed is accumulated at the unretract-while-flushing branch).
  inline int filamentSplitBucketForRole(int role)
  {
    if (role == 11 || role == 12 || role == 18)
      return 1;
    if (role == 13)
      return 3;
    return 0;
  }

  // Phase 238 (PREV-03): legend label for a non-extrusion move kind,
  // matching the upstream options_items display strings
  // (GCodeViewer.cpp:4936-4950: Travel/Seams/Retract/Unretract/Wipe).
  QString kindFeatureLabel(int kind)
  {
    switch (kind)
    {
      case 1: return QStringLiteral("Travel");
      case 2: return QStringLiteral("Retract");
      case 3: return QStringLiteral("Unretract");
      case 4: return QStringLiteral("Wipe");
      case 5: return QStringLiteral("Seam");
      default: return QString::fromUtf8(kRoleLabels[0]);
    }
  }

  // PERF(PREV-PARSE): canonical role label built once (was a QString
  // allocation per extruding move in the parser loop).
  const QString &roleLabel(int role)
  {
    static const QList<QString> labels = [] {
      QList<QString> l;
      l.reserve(int(sizeof(kRoleLabels) / sizeof(kRoleLabels[0])));
      for (const char *s : kRoleLabels)
        l.push_back(QString::fromUtf8(s));
      return l;
    }();
    return labels[role];
  }

  // Phase 238 (PREV-05): filament split categories in upstream column order
  // (GCodeViewer.cpp:4893-4903: Model / Support / Flushed / Tower).
  const char *kFilamentSplitKeys[4] = {"model", "support", "flushed", "tower"};
  const char *kFilamentSplitLabels[4] = {"Model", "Support", "Flushed", "Tower"};

  // Phase 238 (PREV-06): the legacy fixed 8-color extruder cycle, kept as the
  // fallback when the configured filament colors are unavailable.
  static const float kLegacyExtruderColors[][3] = {
      {0.95f, 0.55f, 0.22f},  // extruder 0 - orange
      {0.22f, 0.55f, 0.87f},  // extruder 1 - blue
      {0.30f, 0.69f, 0.46f},  // extruder 2 - green
      {0.61f, 0.35f, 0.71f},  // extruder 3 - purple
      {0.91f, 0.30f, 0.24f},  // extruder 4 - red
      {0.10f, 0.74f, 0.61f},  // extruder 5 - teal
      {0.95f, 0.77f, 0.06f},  // extruder 6 - yellow
      {0.91f, 0.12f, 0.55f},  // extruder 7 - pink
  };
  constexpr int kLegacyExtruderColorCount = 8;

  // Phase 238 (PREV-06): effective color for an extruder -- the CONFIGURED
  // filament color when present (upstream m_tool_colors sourced from the
  // plater extruder_colors config, GCodeViewer.cpp:1109-1127), else the
  // legacy cycle. Single source shared by extruderColor(), the Tool/Filament
  // recolor path, and the legend rows so the three can never diverge.
  ColorResult effectiveExtruderColor(int extruderId, const QStringList &configured)
  {
    if (extruderId >= 0 && extruderId < configured.size())
    {
      const QColor c(configured.at(extruderId));
      if (c.isValid())
        return {float(c.redF()), float(c.greenF()), float(c.blueF())};
    }
    const int idx = ((extruderId % kLegacyExtruderColorCount) + kLegacyExtruderColorCount) % kLegacyExtruderColorCount;
    const auto &tc = kLegacyExtruderColors[idx];
    return {tc[0], tc[1], tc[2]};
  }

  QString colorHex(const ColorResult &c)
  {
    return QStringLiteral("#%1%2%3")
        .arg(qBound(0, int(c.r * 255.f + 0.5f), 255), 2, 16, QLatin1Char('0'))
        .arg(qBound(0, int(c.g * 255.f + 0.5f), 255), 2, 16, QLatin1Char('0'))
        .arg(qBound(0, int(c.b * 255.f + 0.5f), 255), 2, 16, QLatin1Char('0'));
  }

  // Upstream EMoveType classification port (GCodeProcessor.cpp:2954-2968
  // move_type lambda). xyMoved/zMoved/moved follow the parsed axis deltas;
  // wiping mirrors the upstream "; WIPE_START"/"; WIPE_END" comment state.
  int classifyMoveKind(float dE, bool xyMoved, bool zMoved, bool wiping)
  {
    if (wiping)
      return 4;  // Wipe
    if (dE < 0.f)
      return xyMoved || zMoved ? 1 /*Travel*/ : 2 /*Retract*/;
    if (dE > 0.f)
    {
      if (!xyMoved)
        return zMoved ? 1 /*Travel (z lift)*/ : 3 /*Unretract*/;
      return 0;  // Extrude
    }
    return xyMoved || zMoved ? 1 /*Travel*/ : -1 /*Noop -- skip*/;
  }

  // Parse a "H:MM:SS" / "1h23m" / "MM:SS" duration from an upstream
  // "; estimated printing time (normal|silent mode) = <dhms>" comment value.
  float parseEstimatedTimeValue(const QString &value)
  {
    const QString v = value.trimmed();
    const auto parts = v.split(':');
    if (parts.size() == 3)
      return parts[0].toFloat() * 3600.f + parts[1].toFloat() * 60.f + parts[2].toFloat();
    if (parts.size() == 2)
      return parts[0].toFloat() * 60.f + parts[1].toFloat();
    const QRegularExpression re(QStringLiteral("(\\d+)h(\\d+)m"));
    const auto m = re.match(v);
    if (m.hasMatch())
      return m.captured(1).toFloat() * 3600.f + m.captured(2).toFloat() * 60.f;
    if (v.endsWith('s'))
      return v.left(v.size() - 1).toFloat();
    bool ok = false;
    const float secs = v.toFloat(&ok);
    return ok ? secs : 0.f;
  }

  QString formatFilamentLength(double lengthMm)
  {
    return QStringLiteral("%1 m").arg(lengthMm / 1000.0, 0, 'f', 2);
  }

  QString formatFilamentWeight(double grams)
  {
    return QStringLiteral("%1 g").arg(grams, 0, 'f', 1);
  }

  // PreviewViewModel::EViewType (declared in the header) is the single source
  // of truth for view-mode indices: rows 0..9 are the upstream view_type_items
  // order (GCodeViewer.cpp:892-901) and the tail keeps the internal
  // capabilities. Every mode-to-field mapping in recolorAndPackSegments() and
  // buildLegendItems() uses the named VT_ constants instead of raw integers so
  // a renumber cannot silently mislabel a gradient (55-RESEARCH Pitfall 2).

  // Availability gate for modes whose underlying field is missing from the
  // parsed payload. v5.11: every mode (ActualSpeed/Jerk/ActualFlow/PA included)
  // now parses real data from M220/M221/M205/M900, so nothing is gated. The
  // hook stays because the availability plumbing and the QML honesty pill are
  // part of the view-mode contract (QmlUiAuditTests pins the symbol).
  bool viewModeUsesUnavailableData(int mode)
  {
    Q_UNUSED(mode);
    return false;
  }

  bool parseAxis(const QString &line, QChar axis, float &value)
  {
    // PERF(PREV-PARSE): hand-rolled scan. The previous implementation built
    // and compiled a QRegularExpression per call and this runs 5x per parsed
    // G1 move (376k moves on the 400k bench sphere => ~70s of pure pattern
    // compilation). Semantics of the old pattern (?:^|\s)AXIS([+-]?number)
    // are preserved: word-start anchor, optional sign, digits with at most
    // one dot ("12", "12.5", "12.", ".5").
    const int n = line.size();
    for (int i = 0; i < n; ++i)
    {
      if (line.at(i) != axis)
        continue;
      if (i > 0 && !line.at(i - 1).isSpace())
        continue;
      int j = i + 1;
      if (j >= n)
        continue;
      const int numStart = j;
      if (line.at(j) == QLatin1Char('+') || line.at(j) == QLatin1Char('-'))
        ++j;
      bool sawDigit = false;
      bool sawDot = false;
      while (j < n)
      {
        const QChar c = line.at(j);
        if (c.isDigit())
          sawDigit = true;
        else if (c == QLatin1Char('.') && !sawDot)
          sawDot = true;
        else
          break;
        ++j;
      }
      if (!sawDigit)
        continue;
      bool ok = false;
      const float v = line.mid(numStart, j - numStart).toFloat(&ok);
      if (!ok)
        continue;
      value = v;
      return true;
    }
    return false;
  }

  // Shared scanner for the S/F word values (old pattern \bAXIS(-?\d+(?:\.\d+)?)\b).
  float parseWordFloatValue(const QString &line, QChar axis)
  {
    const int n = line.size();
    for (int i = 0; i < n; ++i)
    {
      if (line.at(i) != axis)
        continue;
      if (i > 0 && line.at(i - 1).isLetterOrNumber())
        continue;
      int j = i + 1;
      if (j < n && line.at(j) == QLatin1Char('-'))
        ++j;
      const int numStart = i + 1;
      bool sawDigit = false;
      while (j < n && line.at(j).isDigit())
      {
        sawDigit = true;
        ++j;
      }
      if (!sawDigit)
        continue;
      if (j < n && line.at(j) == QLatin1Char('.'))
      {
        int k = j + 1;
        while (k < n && line.at(k).isDigit())
          ++k;
        if (k > j + 1)
          j = k;
      }
      if (j < n && line.at(j).isLetterOrNumber())
        continue; // trailing word character: the old \b anchor rejected this
      bool ok = false;
      const float v = line.mid(numStart, j - numStart).toFloat(&ok);
      if (ok)
        return v;
    }
    return -1.f;
  }

  float parseSValue(const QString &line)
  {
    return parseWordFloatValue(line, QLatin1Char('S'));
  }

  float parseFValue(const QString &line)
  {
    return parseWordFloatValue(line, QLatin1Char('F'));
  }

  bool parseTaggedValue(const QString &line, const QString &tag, float &value)
  {
    // PERF(PREV-PARSE): the compiled pattern is cached per tag (the five call
    // sites pass constant tags; the old code recompiled per call on every
    // comment line).
    static QHash<QString, QRegularExpression> patternCache;
    auto it = patternCache.constFind(tag);
    if (it == patternCache.constEnd())
    {
      it = patternCache.insert(
          tag,
          QRegularExpression(QStringLiteral("%1\\s*[:=]\\s*([+-]?(?:\\d+(?:\\.\\d*)?|\\.\\d+))")
                                 .arg(QRegularExpression::escape(tag)),
                             QRegularExpression::CaseInsensitiveOption));
    }
    const auto m = it->match(line);
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

#ifdef HAS_LIBSLIC3R
  // P1 (preview direct hand-off): libslic3r ExtrusionRole (ExtrusionEntity.hpp)
  // -> canonical libvgcode EGCodeExtrusionRole index. The two enums DIVERGE
  // past index 6 (55-RESEARCH Pitfall 6), so the translation is an explicit
  // named switch -- never a static_cast of the libslic3r integer. Correctness
  // is guaranteed by construction against the text path: role_to_string
  // (ExtrusionEntity.cpp:583-608) produces exactly the strings kRoleMap keys
  // on, so table(role) == roleForTypeImpl(role_to_string(role)) for every
  // role the locked tree defines.
  int canonicalRoleFromExtrusionRole(Slic3r::ExtrusionRole role)
  {
    switch (role)
    {
      case Slic3r::erNone:                     return 0;   // None
      case Slic3r::erPerimeter:                return 1;   // Perimeter
      case Slic3r::erExternalPerimeter:        return 2;   // ExternalPerimeter
      case Slic3r::erOverhangPerimeter:        return 3;   // OverhangPerimeter
      case Slic3r::erInternalInfill:           return 4;   // InternalInfill
      case Slic3r::erSolidInfill:              return 5;   // SolidInfill
      case Slic3r::erTopSolidInfill:           return 6;   // TopSolidInfill
      case Slic3r::erBottomSurface:            return 15;  // BottomSurface (NOT 7)
      case Slic3r::erIroning:                  return 7;   // Ironing        (NOT 8)
      case Slic3r::erBridgeInfill:             return 8;   // BridgeInfill   (NOT 9)
      case Slic3r::erInternalBridgeInfill:     return 16;  // InternalBridgeInfill (NOT 10)
      case Slic3r::erGapFill:                  return 9;   // GapFill        (NOT 11)
      case Slic3r::erSkirt:                    return 10;  // Skirt          (NOT 12)
      case Slic3r::erBrim:                     return 17;  // Brim           (NOT 13)
      case Slic3r::erSupportMaterial:          return 11;  // SupportMaterial(NOT 14)
      case Slic3r::erSupportMaterialInterface: return 12;  // SupportMaterialInterface
      case Slic3r::erSupportTransition:        return 18;  // SupportTransition (NOT 16)
      case Slic3r::erWipeTower:                return 13;  // WipeTower      (NOT 17)
      case Slic3r::erCustom:                   return 14;  // Custom         (NOT 18)
      case Slic3r::erMixed:                    return 19;  // Mixed
      default:                                 return 0;
    }
  }
#endif

#ifdef HAS_LIBSLIC3R
  // Phase 118 (TICK-02/TICK-03): pure, side-effect-free conversion of the
  // project's OWzx::TickCode list into a libslic3r CustomGCode::Info so it can
  // be written to model->plates_custom_gcodes. Extracted from writeTicksToModel
  // (WB-05) so the mapping is unit-testable without a Model/SliceService, mirroring
  // the v4.5 Measure-engine unit-testable-boundary pattern.
  //
  // CRITICAL: TickType and CustomGCode::Type have DIFFERENT numeric orders, so an
  // explicit switch maps them -- never static_cast (CONTEXT.md enum table):
  //   PausePrint(0)->CustomGCode::PausePrint(1), CustomGcode(1)->Custom(4),
  //   Template(2)->Template(3),       ToolChange(3)->ToolChange(2),
  //   ColorChange(4)->ColorChange(0).
  // layer is a layer index; Item.print_z is a Z height (mm) -- converted via the
  // supplied layerZs (sourced from PreviewViewModel::m_layerZs / layerZAt). A
  // tick whose layer is out of range (or maps to 0 for a non-zero layer request)
  // is skipped so it does not land on layer 0. Items are sorted by print_z
  // (operator< exists on Item) and check_mode_for_custom_gcode_per_print_z is
  // applied so Info::mode is derived before write-back.
  Slic3r::CustomGCode::Info convertTicksToCustomGcodeInfo(const QList<OWzx::TickCode> &ticks, const QVector<float> &layerZs)
  {
    Slic3r::CustomGCode::Info info;
    info.gcodes.reserve(static_cast<size_t>(ticks.size()));
    for (const auto &tick : ticks) {
      // layer -> print_z via layerZs with out-of-range guard.
      if (tick.tick < 0 || tick.tick >= layerZs.size())
        continue;
      const float printZ = layerZs[tick.tick];
      // layerZAt returns 0.f for an invalid layer; skip so the tick does not
      // collapse onto layer 0.
      if (printZ <= 0.f)
        continue;

      Slic3r::CustomGCode::Type type = Slic3r::CustomGCode::Unknown;
      switch (tick.type) {
        case OWzx::TickType::PausePrint:  type = Slic3r::CustomGCode::PausePrint; break;
        case OWzx::TickType::CustomGcode: type = Slic3r::CustomGCode::Custom;     break;
        case OWzx::TickType::Template:    type = Slic3r::CustomGCode::Template;   break;
        case OWzx::TickType::ToolChange:  type = Slic3r::CustomGCode::ToolChange; break;
        case OWzx::TickType::ColorChange: type = Slic3r::CustomGCode::ColorChange;break;
        // No default static_cast -- unmapped ticks are dropped (defensive).
      }
      if (type == Slic3r::CustomGCode::Unknown)
        continue;

      Slic3r::CustomGCode::Item item;
      item.print_z  = static_cast<double>(printZ);
      item.type     = type;
      item.extruder = tick.extruder;
      item.color    = tick.color.toStdString();
      item.extra    = tick.extra.toStdString();
      info.gcodes.push_back(item);
    }
    std::sort(info.gcodes.begin(), info.gcodes.end());
    // Derive Info::mode from the assembled items (CustomGCode.hpp:121).
    Slic3r::CustomGCode::check_mode_for_custom_gcode_per_print_z(info);
    return info;
  }
#endif // HAS_LIBSLIC3R
}

PreviewViewModel::PreviewViewModel(ProjectService *projectService, SliceService *sliceService, QObject *parent)
    : QObject(parent), projectService_(projectService), sliceService_(sliceService)
{
  // All extrusion roles visible by default, matching upstream extrusion_roles_visibility
  // (libvgcode/src/Settings.hpp:49-71). showTravelMoves_ defaults to false in the header.
  m_roleVisibility.fill(true);
  // Phase 237 (VIEW-01): restore the persisted G-code window visibility
  // (upstream app_config show_gcode_window; default true).
  QSettings settings;
  showGcodeWindow_ = settings.value(QStringLiteral("preview/showGcodeWindow"), true).toBool();
  // P17.10: persist the option visibility flags like upstream
  // get/set_options_visibility_from_flags (GCodeViewer.cpp:1816-1838,
  // app_config options: travel/seams/retracts/unretracts/wipes visibility).
  showTravelMoves_ = settings.value(QStringLiteral("preview/showTravelMoves"), showTravelMoves_).toBool();
  showRetractMoves_ = settings.value(QStringLiteral("preview/showRetractMoves"), showRetractMoves_).toBool();
  showUnretractMoves_ = settings.value(QStringLiteral("preview/showUnretractMoves"), showUnretractMoves_).toBool();
  showWipeMoves_ = settings.value(QStringLiteral("preview/showWipeMoves"), showWipeMoves_).toBool();
  showSeamMarks_ = settings.value(QStringLiteral("preview/showSeamMarks"), showSeamMarks_).toBool();
  showToolChanges_ = settings.value(QStringLiteral("preview/showToolChanges"), showToolChanges_).toBool();
  playTimer_ = new QTimer(this);
  // 上游无时间轴播放，OWzx 增强：initial cadence derives from the default
  // playbackSpeed_ (1.0x -> 24ms), keeping the interval single-sourced in
  // applyPlaybackInterval().
  applyPlaybackInterval();
  connect(playTimer_, &QTimer::timeout, this, [this]()
          {
    if (moveCount_ <= 0)
      return;
    // PREV-PLAY: the stop bound follows the LIVE layer range (full range
    // degrades to the legacy moveCount_ end), so narrowing the rail
    // mid-playback halts the marker at the visible band's edge.
    const int end = playbackRangeEndExclusive();
    if (currentMove_ >= end)
    {
      playTimer_->stop();
      // PREV-PLAY: finishing a replay restores the colorful "everything
      // shown" pack (currentMove_ sits at the range end; a parity recolor
      // here would gray all but that layer's band).
      recolorAndPackSegments(true);
      // R-P1.B: notify the UI that playback finished -- without this emit
      // isPlaying stayed true and the QML play/pause button stuck on
      // "playing" after every completed playback.
      emit stateChanged();
      return;
    }
    currentMove_ = qMin(currentMove_ + 12, end);
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

// 上游无时间轴播放，OWzx 增强（see the playbackSpeed Q_PROPERTY）。
double PreviewViewModel::playbackSpeed() const
{
  return playbackSpeed_;
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
  // The 16 upstream dropdown rows, display strings verbatim from
  // get_view_type_string (GCodeViewer.cpp:66-104).
  //
  // ORDER NOTE: upstream update_by_mode (GCodeViewer.cpp:1119-1135) shows
  // these as Summary, Line Type, Filament, Speed, Actual Speed, Acceleration,
  // Jerk, Layer Height, Line Width, Flow, Actual Flow, Layer Time,
  // Layer Time (log), Fan Speed, Temperature, Pressure Advance. We keep
  // EViewType VALUE order instead of that visual order on purpose:
  // viewModeIndex_ doubles as the combo row AND the recolor switch key, so
  // mirroring the upstream sequence would force renumbering EViewType
  // (PreviewViewModel.h invariant: row index == EViewType value). Content is
  // the upstream 16 items item-for-item; only the sequence differs.
  //
  // Rows 0..9 are the historical GAP-3 prefix (unchanged, so every
  // modes.indexOf(...) consumer keeps its row). Rows 10..15 expose the six
  // modes whose recolor/legend/hover-detail branches already existed
  // (v5.11 parses real M220/M221/M205/M900 data for them). Tool stays hidden
  // upstream (appended only after the string table is built,
  // GCodeViewer.cpp:1145) and FilamentId stays internal-only, so neither
  // gets a row.
  return {
      QStringLiteral("Line Type"),         // FeatureType  (upstream row 1)
      QStringLiteral("Filament"),          // ColorPrint   (upstream row 2)
      QStringLiteral("Speed"),             // Feedrate     (upstream row 3)
      QStringLiteral("Layer Height"),      // Height       (upstream row 7)
      QStringLiteral("Line Width"),        // Width        (upstream row 8)
      QStringLiteral("Flow"),              // VolumetricRate (upstream row 9)
      QStringLiteral("Layer Time"),        // LayerTime    (upstream row 11)
      QStringLiteral("Layer Time (log)"),  // LayerTimeLog (upstream row 12)
      QStringLiteral("Fan Speed"),         // FanSpeed     (upstream row 13)
      QStringLiteral("Temperature"),       // Temperature  (upstream row 14)
      QStringLiteral("Summary"),           // Summary      (upstream row 0)
      QStringLiteral("Actual Speed"),      // ActualSpeed  (upstream row 4)
      QStringLiteral("Acceleration"),      // Acceleration (upstream row 5)
      QStringLiteral("Jerk"),              // Jerk         (upstream row 6)
      QStringLiteral("Actual Flow"),       // ActualVolumetricFlowRate (row 10)
      QStringLiteral("Pressure Advance")}; // PressureAdvance (upstream row 15)
}

bool PreviewViewModel::currentViewModeAvailable() const
{
  return viewModeAvailable(viewModeIndex_);
}

QString PreviewViewModel::currentViewModeStatus() const
{
  return viewModeStatusText(viewModeIndex_);
}

bool PreviewViewModel::roleVisibilityAvailable() const
{
  // Upstream renders per-extrusion-role rows only for FeatureType.
  return viewModeIndex_ == VT_LineType;
}

bool PreviewViewModel::extruderVisibilityAvailable() const
{
  // Upstream m_tool_visibles is exposed only by ColorPrint.
  return viewModeIndex_ == VT_Filament;
}

bool PreviewViewModel::moveVisibilityAvailable() const
{
  // Retract, unretract, wipe, seam, and ToolChanges options are
  // FeatureType-only (upstream m_options rows, GCodeViewer.cpp:3939-3957).
  return viewModeIndex_ == VT_LineType;
}

bool PreviewViewModel::travelVisibilityAvailable() const
{
  // Upstream exposes Travel in FeatureType and Feedrate views.
  return viewModeIndex_ == VT_LineType || viewModeIndex_ == VT_Speed;
}

bool PreviewViewModel::viewModeAvailable(int index) const
{
  if (index < 0 || index >= viewModes().size())
    return false;
  return !viewModeUsesUnavailableData(index);
}

QString PreviewViewModel::viewModeStatusText(int index) const
{
  const QStringList modes = viewModes();
  if (index < 0 || index >= modes.size())
    return tr("View mode unavailable");
  if (!viewModeUsesUnavailableData(index))
    return {};
  return tr("%1 data is not available in the current Preview payload").arg(modes.at(index));
}

bool PreviewViewModel::loadGCodeForPreview(const QString &filePath)
{
  const QFileInfo info(filePath);
  if (!info.exists() || !info.isFile())
  {
    lastLoadedPath_.clear();
    resetPreviewState();
    emit stateChanged();
    return false;
  }

  rebuildFromGCode(info.absoluteFilePath());
  lastLoadedPath_ = info.absoluteFilePath();
  emit stateChanged();
  return !gcodePreviewData_.isEmpty();
}

void PreviewViewModel::syncPreviewWithActiveResult()
{
  const QString activePath = sliceService_ ? sliceService_->outputPath() : QString{};
  if (activePath.isEmpty())
  {
    lastLoadedPath_.clear();
    resetPreviewState();
    return;
  }

#ifdef HAS_LIBSLIC3R
  // P1 (preview direct hand-off): a FRESH structured result from the last
  // slice / previous-G-code reuse takes the direct path -- the preview state
  // is built from the processor's own moves vector (upstream load_as_gcode
  // consumption, GCodeViewer.hpp:186) instead of re-parsing the just-written
  // text on the GUI thread. The delivery id makes the second sync of the same
  // completion (resultChanged AND sliceFinished both connect here) a no-op --
  // which also retires the pre-P1 behavior of running the full text parse
  // TWICE per slice.
  if (sliceService_)
  {
    quint64 resultId = 0;
    std::shared_ptr<const Slic3r::GCodeProcessorResult> processorResult =
        sliceService_->activeProcessorResult(&resultId);
    if (processorResult && resultId != lastConsumedResultId_)
    {
      lastConsumedResultId_ = resultId;
      lastLoadedPath_ = activePath;
      loadFromProcessorResult(*processorResult, activePath);
      // The ~40 MB moves vector is now duplicated inside the preview's own
      // segment store -- release the service copy immediately (the id keeps
      // the duplicate-sync guard working; a later plate-switch sync re-parses
      // that plate's file through the text path like before).
      sliceService_->releaseProcessorResult();
      return;
    }
    // Idempotence guard for the duplicate sync: same result id (or none) and
    // the same file means the state is already built -- skip the rebuild.
    if (activePath == lastLoadedPath_)
      return;
  }
#endif

  const QFileInfo info(activePath);
  if (!info.exists() || !info.isFile())
  {
    lastLoadedPath_.clear();
    resetPreviewState();
    return;
  }

  const QString activeTime = sliceService_->estimatedTimeLabel();
  estimatedTime_ = activeTime.isEmpty() ? QStringLiteral("--:--:--") : activeTime;
  totalTime_ = estimatedTime_;
  rebuildFromGCode(info.absoluteFilePath());
  lastLoadedPath_ = info.absoluteFilePath();
}

void PreviewViewModel::setLayerRange(int minLayer, int maxLayer)
{
  if (layerCount_ <= 0)
    return;
  const int lo = qBound(0, minLayer, layerCount_ - 1);
  const int hi = singleLayer_ ? lo : qBound(lo, maxLayer, layerCount_ - 1);
  if (lo == currentLayerMin_ && hi == currentLayerMax_)
    return;
  currentLayerMin_ = lo;
  currentLayerMax_ = hi;
  // P17.7: top_layer_only tints segments outside the current top layer, so a
  // range change re-runs the recolor while the flag is on.
  if (m_topLayerOnly && !segments_.empty())
    recolorAndPackSegments();
  emit stateChanged();
}

// P17.7: upstream top_layer_only — while the sequential view is scrubbed,
// segments outside the current top layer render in Neutral_Color
// (GV.cpp:3285: `else color = Neutral_Color`). The neutral dark-gray matches
// GCodeViewer.cpp:750. Re-pack applies it to the baked segment colors.
void PreviewViewModel::setTopLayerOnly(bool on)
{
  if (m_topLayerOnly == on)
    return;
  m_topLayerOnly = on;
  recolorAndPackSegments();
  emit stateChanged();
}

// P17.12: upstream preview_dim_previous_layers (Preferences.cpp:1908-1915,
// default false AppConfig.cpp:206-207) — layers below the viewed top layer
// render darkened while the sequential view is scrubbed (ViewerImpl.cpp
// PREVIOUS_LAYER_DARKEN_FACTOR). Re-pack applies it to the baked colors.
void PreviewViewModel::setDimPreviousLayers(bool on)
{
  if (m_dimPreviousLayers == on)
    return;
  m_dimPreviousLayers = on;
  recolorAndPackSegments();
  emit stateChanged();
}

// P17.9: full-config accessor — the header `; key = value` block parsed by
// rebuildFromGCode (upstream apply_config(DynamicPrintConfig) from the gcode
// file, GCodeProcessor.cpp:1602/1796).
QVariantMap PreviewViewModel::fullConfig() const
{
  return m_fullConfig;
}

void PreviewViewModel::setSingleLayer(bool enabled)
{
  if (singleLayer_ == enabled)
    return;
  singleLayer_ = enabled;
  if (singleLayer_)
    setLayerRange(currentLayerMax_, currentLayerMax_);
  else
    setLayerRange(0, layerCount_ - 1);
  emit stateChanged();
}

void PreviewViewModel::wheelLayer(int delta, bool accelerated, bool lowerHandle)
{
  if (layerCount_ <= 0 || delta == 0)
    return;
  const int step = delta * (accelerated ? 5 : 1);
  if (singleLayer_)
  {
    setLayerRange(currentLayerMax_ + step, currentLayerMax_ + step);
    return;
  }
  // IMSlider changes the selected handle; the higher handle is the default.
  if (lowerHandle)
    setLayerRange(currentLayerMin_ + step, currentLayerMax_);
  else
    setLayerRange(currentLayerMin_, currentLayerMax_ + step);
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
  // P17.7: top_layer_only grays out-of-top-layer segments as the scrub
  // position crosses layer boundaries — repack keeps the baked colors in
  // sync with the current position (the payload rebuild mirrors upstream
  // refresh_render_paths on sequential-view changes). PREV-PLAY: the
  // playback restart bypasses this (playback is a full-color replay, see
  // m_suppressTopLayerRecolor).
  if (m_topLayerOnly && !m_suppressTopLayerRecolor)
    recolorAndPackSegments();
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

  // Upstream property-table fields (GCodeViewer.cpp:430-475). is_extrusion
  // mirrors libvgcode PathVertex::is_extrusion (type == Extrude), which is
  // stricter than toolIsExtrusion (non-travel) and gates the N/A cells.
  toolKind_ = seg.kind;
  toolTypeText_ = moveTypeToString(seg.kind);
  const bool isExtrusion = seg.kind == KindExtrude;
  const bool isTravel = seg.kind == KindTravel;
  const bool isWipe = seg.kind == KindWipe;
  toolLineTypeText_ = isExtrusion ? moveRoleToString(seg.role) : QStringLiteral("N/A");
  toolHeight_ = seg.height;
  toolFlowRate_ = seg.volumetric_rate;
  toolJerk_ = seg.jerk;
  toolPressureAdvance_ = seg.pressure_advance;

  // Length of the move run ending at the current segment (upstream
  // GCodeViewer.cpp:442-457): arc moves are discretized into several segments
  // sharing the same gcode source line, so the whole run is summed. A
  // negative sentinel marks the upstream N/A case (first vertex or a
  // retract/unretract/seam move).
  toolLength_ = -1.0;
  int runFirst = idx;
  int runLast = idx;
  if (idx > 0 && (isExtrusion || isTravel || isWipe))
  {
    const int lineId = seg.gcodeLine;
    if (lineId >= 0)
    {
      while (runFirst > 0 && segments_[runFirst - 1].gcodeLine == lineId)
        --runFirst;
      while (runLast + 1 < static_cast<int>(segments_.size()) && segments_[runLast + 1].gcodeLine == lineId)
        ++runLast;
    }
    double length = 0.0;
    for (int i = qMax(runFirst, 1); i <= runLast; ++i)
    {
      const auto &prev = segments_[i - 1];
      const auto &curr = segments_[i];
      const double dx = double(curr.x2) - double(prev.x2);
      const double dy = double(curr.y2) - double(prev.y2);
      const double dz = double(curr.z2) - double(prev.z2);
      length += std::sqrt(dx * dx + dy * dy + dz * dz);
    }
    toolLength_ = length;
  }

  // Time row (upstream GCodeViewer.cpp:474-475): cumulative estimated time at
  // the vertex in get_time_dhms form + the move's own duration in %.3fs.
  const int accSize = static_cast<int>(m_moveAccumulatedTime.size());
  double cumulative = 0.0;
  double moveDuration = 0.0;
  if (accSize > 0)
  {
    const int accIdx = qMin(idx, accSize - 1);
    cumulative = m_moveAccumulatedTime[accIdx];
    moveDuration = (idx > 0 && idx - 1 < accSize) ? cumulative - m_moveAccumulatedTime[idx - 1]
                                                 : cumulative;
    if (moveDuration < 0.0)
      moveDuration = 0.0;
  }
  toolEstimatedTime_ = formatTimeDhms(float(cumulative));
  toolMoveTimeSecs_ = moveDuration;

  // Per-view-type detail appended after "Speed: N" on the second info line
  // (upstream detail switch, GCodeViewer.cpp:361-415). Empty string renders
  // nothing, like upstream detail_buf[0] == '\0'.
  const QString naText = QStringLiteral("N/A");
  QString detail;
  switch (viewModeIndex_)
  {
    case VT_LineType:
      // Upstream: extrusion -> role string, non-Noop -> move type string.
      detail = isExtrusion ? toolLineTypeText_ : toolTypeText_;
      break;
    case VT_Height:
      detail = tr("Height: ") + (isExtrusion ? QString::number(seg.height, 'f', 2) : naText);
      break;
    case VT_Width:
      detail = tr("Width: ") + (isExtrusion ? QString::number(seg.width, 'f', 2) : naText);
      break;
    case VT_Flow:
      detail = tr("Flow: ") + (isExtrusion ? QString::number(seg.volumetric_rate, 'f', 2) : naText);
      break;
    case VT_FanSpeed:
      detail = tr("Fan: ") + QString::number(seg.fan_speed, 'f', 0);
      break;
    case VT_Temperature:
      detail = tr("Temperature: ") + QString::number(seg.temperature, 'f', 0);
      break;
    case VT_LayerTime:
    case VT_LayerTimeLog:
      detail = tr("Layer Time: ") + QString::number(seg.layer_time, 'f', 1);
      break;
    case VT_Tool:
      detail = tr("Tool: ") + QString::number(seg.extruder_id + 1);
      break;
    case VT_Filament:
      // Upstream ColorPrint uses vertex.color_id; the Qt6 Filament view is the
      // per-extruder palette, so the extruder id stands in.
      detail = tr("Color: ") + QString::number(seg.extruder_id + 1);
      break;
    case VT_Acceleration:
      detail = tr("Acceleration: ") + QString::number(seg.acceleration, 'f', 0);
      break;
    case VT_Jerk:
      detail = tr("Jerk: ") + QString::number(seg.jerk, 'f', 1);
      break;
    case VT_PressureAdvance:
      detail = tr("PA: ") + QString::number(seg.pressure_advance, 'f', 4);
      break;
    default:
      break;
  }
  toolDetailText_ = detail;

  // ENABLE_ACTUAL_SPEED_DEBUG profile (upstream GCodeViewer.cpp:1735-1787):
  // segments sharing the current gcode line, cumulative length (mm) vs actual
  // feedrate, zero-delta points dropped, trailing seam dropped, consecutive
  // duplicate speeds (0.1 precision) compressed.
  toolActualSpeedExist_ = isExtrusion || isTravel || isWipe;
  toolActualSpeedProfile_.clear();
  // The parser feedrate is the mm/min F word; upstream actual_feedrate and
  // the ActualSpeed color range are mm/s, so the tooltip contract converts.
  toolActualSpeedYMin_ = m_actualSpeedMin <= m_actualSpeedMax ? m_actualSpeedMin / 60.0 : 0.0;
  toolActualSpeedYMax_ = m_actualSpeedMax / 60.0;
  if (toolActualSpeedExist_)
  {
    int endId = runLast;
    if (endId > runFirst && segments_[endId - 1].kind == KindSeam)
      --endId; // upstream drops a trailing seam vertex from the run
    struct ProfilePoint
    {
      float pos;
      float speed;
      bool internal;
    };
    std::vector<ProfilePoint> data;
    float totalLen = 0.f;
    for (int i = runFirst; i <= endId; ++i)
    {
      const auto &v = segments_[i];
      float len = 0.f;
      if (i > runFirst)
      {
        const auto &prev = segments_[i - 1];
        const float dx = v.x2 - prev.x2;
        const float dy = v.y2 - prev.y2;
        const float dz = v.z2 - prev.z2;
        len = std::sqrt(dx * dx + dy * dy + dz * dz);
      }
      totalLen += len;
      if (i == runFirst || len > 1e-6f)
      {
        // upstream "internal" flag = zero-duration vertex (v.times[0] == 0)
        const int accSize2 = static_cast<int>(m_moveAccumulatedTime.size());
        float duration = 0.f;
        if (accSize2 > 0)
        {
          const int ai = qMin(i, accSize2 - 1);
          duration = (i > 0 && i - 1 < accSize2) ? m_moveAccumulatedTime[ai] - m_moveAccumulatedTime[i - 1]
                                                : m_moveAccumulatedTime[ai];
        }
        data.push_back({totalLen, v.actual_speed, duration <= 0.f});
      }
    }
    // Upstream compression: drop middle points of runs of equal speeds.
    auto sameSpeed = [](float a, float b) {
      return int(std::round(a * 10.f)) == int(std::round(b * 10.f));
    };
    std::vector<ProfilePoint> compressed;
    if (!data.empty())
    {
      compressed.push_back(data[0]);
      for (int i = 1; i < static_cast<int>(data.size()); ++i)
      {
        const bool sameAsPrev = sameSpeed(data[i].speed, data[i - 1].speed);
        const bool sameAsNext = (i + 1 < static_cast<int>(data.size())) &&
                                sameSpeed(data[i].speed, data[i + 1].speed);
        if (!sameAsPrev)
        {
          if (!sameSpeed(compressed.back().speed, data[i - 1].speed))
            compressed.push_back(data[i - 1]);
          compressed.push_back(data[i]);
        }
        else if (!sameAsNext)
        {
          compressed.push_back(data[i]);
        }
      }
      if (compressed.back().pos != data.back().pos)
        compressed.push_back(data.back());
    }
    toolActualSpeedProfile_.reserve(static_cast<qsizetype>(compressed.size()));
    for (const ProfilePoint &p : compressed)
    {
      QVariantMap row;
      row.insert(QStringLiteral("pos"), double(p.pos));
      row.insert(QStringLiteral("speed"), double(p.speed / 60.f)); // mm/min -> mm/s
      row.insert(QStringLiteral("internal"), p.internal);
      toolActualSpeedProfile_.append(row);
    }
  }
}

void PreviewViewModel::rebuildGcodeLineWindow()
{
  gcodeLines_.clear();
  gcodeLineCount_ = m_gcodeSourceLines.size();
  currentGcodeLine_ = 0;
  currentGcodeRowIndex_ = -1;

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
  // gcode-window-follow: the row flagged `current` (i == anchor below) sits
  // at window-local index anchor - start. Derived straight from the anchor
  // the window was centered on, so it can never drift from the flag -- no
  // separate lineNumber->index lookup pass is needed. Every currentGcodeLine_
  // update flows through here or resetPreviewState, so the property stays
  // in sync with gcodeLines_ by construction.
  currentGcodeRowIndex_ = anchor - start;
  for (int i = start; i <= end; ++i) {
    const SourceGcodeLine &source = m_gcodeSourceLines[i];
    QVariantMap row;
    row.insert(QStringLiteral("line"), source.lineNumber);
    row.insert(QStringLiteral("move"), source.moveIndex);
    // PREVIEW-GCODE-SOURCE-TOKENS (upstream GCodeViewer.cpp:509-536):
    // lines longer than 55 characters truncate to 52 + "..."; the line then
    // splits at the first ';' (comment) and the first space (command vs
    // parameters) so the delegate can color the three parts separately.
    QString gline = source.text;
    if (gline.size() > 55)
      gline = gline.left(52) + QStringLiteral("...");
    QString command = gline;
    QString comment;
    const int semicolon = gline.indexOf(QLatin1Char(';'));
    if (semicolon >= 0) {
      command = gline.left(semicolon);
      comment = gline.mid(semicolon);
    }
    QString parameters;
    if (!command.isEmpty()) {
      const int space = command.indexOf(QLatin1Char(' '));
      if (space >= 0) {
        parameters = command.mid(space + 1);
        command = command.left(space);
      }
    }
    row.insert(QStringLiteral("text"), gline);
    row.insert(QStringLiteral("command"), command);
    row.insert(QStringLiteral("parameters"), parameters);
    row.insert(QStringLiteral("comment"), comment);
    row.insert(QStringLiteral("current"), i == anchor);
    gcodeLines_.append(row);
  }
}

void PreviewViewModel::playAnimation()
{
  if (moveCount_ <= 0)
    return;
  // PREV-PLAY: both parse paths park currentMove_ at moveCount_ ("everything
  // shown"), so a fresh preview used to tick once and stop without moving
  // the marker at all. Media-player semantics instead: pressing play with
  // the playhead at/past the visible range end (or stranded outside a
  // narrowed layer range) restarts from the range's first move so the
  // marker traces exactly the visible band.
  const int start = playbackRangeStart();
  const int end = playbackRangeEndExclusive();
  if (currentMove_ >= end || currentMove_ < start)
  {
    // Full-color replay: suppress the top_layer_only gray recolor that a
    // manual scrub would trigger for this position change (the timer path
    // already skips it).
    m_suppressTopLayerRecolor = true;
    setCurrentMove(start);
    m_suppressTopLayerRecolor = false;
    // Repack in full color even when the previous scrub left a gray pack.
    recolorAndPackSegments(true);
  }
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

// 上游无时间轴播放，OWzx 增强：clamp to [0.25, 8] and re-derive the play
// timer cadence (baseInterval/speed) immediately so the new speed takes
// effect even while a playback is running.
void PreviewViewModel::setPlaybackSpeed(double speed)
{
  const double clamped = qBound(kPlaybackSpeedMin, speed, kPlaybackSpeedMax);
  if (qFuzzyCompare(playbackSpeed_, clamped))
    return;
  playbackSpeed_ = clamped;
  applyPlaybackInterval();
  emit stateChanged();
}

// 上游无时间轴播放，OWzx 增强：cadence = baseInterval / speed, floored at 1ms
// so the extreme 8x tier never yields a zero/coarse Qt timer.
void PreviewViewModel::applyPlaybackInterval()
{
  if (!playTimer_)
    return;
  const int interval = qMax(1, qRound(kPlaybackBaseIntervalMs / playbackSpeed_));
  if (playTimer_->isActive())
    // Restart so the new cadence lands mid-playback instead of after the
    // remaining old-interval timeout.
    playTimer_->start(interval);
  else
    playTimer_->setInterval(interval);
}

// PREV-PLAY: one O(n) sweep per parse (n = move count; 40k on the gate
// models) building the per-layer first/last move indices. Segments carry
// their layer from both parse paths (seg.layer), and layers without any
// move keep -1 so the range helpers can skip them.
void PreviewViewModel::rebuildLayerMoveBounds()
{
  layerFirstMove_.clear();
  layerLastMove_.clear();
  if (layerCount_ <= 0 || segments_.empty())
    return;
  layerFirstMove_.resize(layerCount_, -1);
  layerLastMove_.resize(layerCount_, -1);
  const int count = static_cast<int>(segments_.size());
  for (int i = 0; i < count; ++i)
  {
    const int layer = segments_[i].layer;
    if (layer < 0 || layer >= layerCount_)
      continue;
    if (layerFirstMove_[layer] < 0)
      layerFirstMove_[layer] = i;
    layerLastMove_[layer] = i;
  }
}

// PREV-PLAY: first move inside the visible layer band (skipping empty
// layers whose tick never produced a segment); 0 when no bounds exist.
int PreviewViewModel::playbackRangeStart() const
{
  if (layerFirstMove_.isEmpty())
    return 0;
  const int lastLayer = qMin(currentLayerMax_, layerFirstMove_.size() - 1);
  for (int l = qMax(0, currentLayerMin_); l <= lastLayer; ++l)
  {
    if (layerFirstMove_[l] >= 0)
      return layerFirstMove_[l];
  }
  return 0;
}

// PREV-PLAY: exclusive end = last move inside the band + 1; moveCount_
// when no bounds exist (full-range playback identical to the legacy end).
int PreviewViewModel::playbackRangeEndExclusive() const
{
  if (layerLastMove_.isEmpty())
    return moveCount_;
  const int firstLayer = qMax(0, currentLayerMin_);
  for (int l = qMin(currentLayerMax_, layerLastMove_.size() - 1); l >= firstLayer; --l)
  {
    if (layerLastMove_[l] >= 0)
      return layerLastMove_[l] + 1;
  }
  return moveCount_;
}

void PreviewViewModel::setViewModeIndex(int index)
{
  // The QML combos offer the 16 upstream dropdown rows (viewModes()); the
  // hidden tail (VT_Tool/VT_FilamentId) stays addressable so the recolor
  // branches and the tests keep exercising it (GAP-3 kept capabilities).
  const int clamped = qBound(0, index, VT_ModeCount - 1);
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
  // Recalculate totalTime for stealth mode. Phase 238 (PREV-05): when the
  // gcode carries an "; estimated printing time (silent mode)" comment, use
  // the slicer's own stealth estimate (upstream PrintEstimatedStatistics
  // modes, GCodeProcessor TimeProcessor export); otherwise keep the x1.4
  // heuristic (~1.4x slower due to reduced accel/jerk) and flag it as an
  // estimate via stealthTimeEstimated().
  if (!totalTime_.contains("--"))
  {
    if (stealthMode_)
    {
      totalTime_ = m_stealthTimeSecs > 0.f
          ? formatTime(m_stealthTimeSecs)
          : formatTime(parseTimeSecs(estimatedTime_) * 1.4f);
    }
    else
    {
      totalTime_ = estimatedTime_;
    }
  }
  emit stateChanged();
}

void PreviewViewModel::setShowTravelMoves(bool enabled)
{
  if (showTravelMoves_ == enabled)
    return;
  showTravelMoves_ = enabled;
  // P17.10: persist like upstream set_options_visibility_from_flags.
  QSettings settings;
  settings.setValue(QStringLiteral("preview/showTravelMoves"), enabled);
  recolorAndPackSegments();
  emit stateChanged();
}

// Phase 237 (VIEW-01): G-code window visibility toggle. Upstream routes the
// View-menu "Show G-code Window" check item through
// wxGetApp::toggle_show_gcode_window() (MainFrame.cpp:2623-2629), which
// stores the app_config show_gcode_window flag; OWzx persists the same flag
// in QSettings and defaults it to true (the panel was always shown before).
void PreviewViewModel::setShowGcodeWindow(bool enabled)
{
  if (showGcodeWindow_ == enabled)
    return;
  showGcodeWindow_ = enabled;
  QSettings settings;
  settings.setValue(QStringLiteral("preview/showGcodeWindow"), enabled);
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

// Phase 238 (PREV-03): move-type visibility setters. All follow the
// setShowTravelMoves pattern: flip the flag, repack the GCV1 payload with the
// newly-visible/hidden move kinds, and emit stateChanged (upstream toggles
// m_buffers[buffer_id(type)].visible then refreshes render paths,
// GCodeViewer.cpp:4936-4946).
void PreviewViewModel::setShowRetractMoves(bool enabled)
{
  if (showRetractMoves_ == enabled)
    return;
  showRetractMoves_ = enabled;
  // P17.10: persist like upstream set_options_visibility_from_flags.
  QSettings settings;
  settings.setValue(QStringLiteral("preview/showRetractMoves"), enabled);
  recolorAndPackSegments();
  emit stateChanged();
}

void PreviewViewModel::setShowUnretractMoves(bool enabled)
{
  if (showUnretractMoves_ == enabled)
    return;
  showUnretractMoves_ = enabled;
  // P17.10: persist like upstream set_options_visibility_from_flags.
  QSettings settings;
  settings.setValue(QStringLiteral("preview/showUnretractMoves"), enabled);
  recolorAndPackSegments();
  emit stateChanged();
}

void PreviewViewModel::setShowWipeMoves(bool enabled)
{
  if (showWipeMoves_ == enabled)
    return;
  showWipeMoves_ = enabled;
  // P17.10: persist like upstream set_options_visibility_from_flags.
  QSettings settings;
  settings.setValue(QStringLiteral("preview/showWipeMoves"), enabled);
  recolorAndPackSegments();
  emit stateChanged();
}

void PreviewViewModel::setShowSeamMarks(bool enabled)
{
  if (showSeamMarks_ == enabled)
    return;
  showSeamMarks_ = enabled;
  // P17.10: persist like upstream set_options_visibility_from_flags.
  QSettings settings;
  settings.setValue(QStringLiteral("preview/showSeamMarks"), enabled);
  recolorAndPackSegments();
  emit stateChanged();
}

void PreviewViewModel::setShowToolChanges(bool enabled)
{
  if (showToolChanges_ == enabled)
    return;
  showToolChanges_ = enabled;
  // Persisted like the sibling option toggles (upstream
  // toggle_option_visibility(ToolChanges), GCodeViewer.cpp:3876-3879).
  // Render-side no-op today: the Qt6 line pipeline emits no tool-change
  // geometry (documented delta at appendMarkerSegment) so no segment kind is
  // gated -- recolorAndPackSegments() would be a wasted repack.
  QSettings settings;
  settings.setValue(QStringLiteral("preview/showToolChanges"), enabled);
  emit stateChanged();
}

int PreviewViewModel::moveCountOfKind(int kind) const
{
  if (kind < 0 || kind >= 6)
    return 0;
  return m_kindCounts[kind];
}

QVariantList PreviewViewModel::filamentSplit() const
{
  // Phase 238 (PREV-05): rows in upstream column order Model / Support /
  // Flushed / Tower (GCodeViewer.cpp:4893-4903), length in meters and weight
  // in grams using the same area/density conversion as the totals.
  QVariantList rows;
  const float area = filamentCrossAreaMm2(m_filamentDiameter);
  for (int i = 0; i < 4; ++i)
  {
    QVariantMap row;
    const double lengthMm = m_filamentSplitLength[i];
    const double grams = lengthMm * area * m_filamentDensity * 0.001;
    row.insert(QStringLiteral("key"), QString::fromLatin1(kFilamentSplitKeys[i]));
    row.insert(QStringLiteral("label"), QString::fromLatin1(kFilamentSplitLabels[i]));
    row.insert(QStringLiteral("lengthM"), lengthMm / 1000.0);
    row.insert(QStringLiteral("weightG"), grams);
    row.insert(QStringLiteral("lengthText"), formatFilamentLength(lengthMm));
    row.insert(QStringLiteral("weightText"), formatFilamentWeight(grams));
    rows.append(row);
  }
  return rows;
}

double PreviewViewModel::filamentUsedGrams() const
{
  return m_totalFilamentGrams;
}

bool PreviewViewModel::stealthTimeEstimated() const
{
  // True only while the heuristic (not a parsed silent-mode comment) drives
  // the stealth total.
  return stealthMode_ && m_stealthTimeSecs <= 0.f;
}

double PreviewViewModel::filamentPricePerKg() const
{
  if (!m_filamentPrices.isEmpty())
    return m_filamentPrices.first();
  return !segments_.empty() ? 29.99 : 0.0;
}

QVariantList PreviewViewModel::extruderVisibilities() const
{
  // Phase 238 (PREV-06): legend rows for the per-extruder visibility toggles
  // (upstream m_tool_visibles + m_tool_colors, GCodeViewer.cpp:5081-5093).
  QVariantList rows;
  QList<int> ids = m_extruderUsedLength.keys();
  std::sort(ids.begin(), ids.end());
  const QStringList configured = configuredExtruderColors();
  for (int id : ids)
  {
    QVariantMap row;
    row.insert(QStringLiteral("extruderId"), id);
    row.insert(QStringLiteral("label"), QStringLiteral("Extruder %1").arg(id));
    row.insert(QStringLiteral("color"), colorHex(effectiveExtruderColor(id, configured)));
    row.insert(QStringLiteral("visible"), isExtruderVisible(id));
    rows.append(row);
  }
  return rows;
}

bool PreviewViewModel::isExtruderVisible(int extruderId) const
{
  // Default visible, matching the upstream m_tool_visibles reload
  // (GCodeViewer.cpp:1109-1116 fills all-true on load).
  return m_extruderVisibility.value(extruderId, true);
}

void PreviewViewModel::toggleExtruderVisibility(int extruderId)
{
  // Applied at pack time in the Filament/ColorPrint view only (upstream
  // gates the skip on EViewType::ColorPrint, GCodeViewer.cpp:3337).
  m_extruderVisibility[extruderId] = !isExtruderVisible(extruderId);
  recolorAndPackSegments();
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
  //
  // Upstream only renders roles the loaded gcode actually contains
  // (ViewerImpl.cpp:1017-1018 collects Extrude vertices; get_roles returns the
  // ordered std::map contents, ExtrusionRoles.cpp:20-29; the legend loop walks
  // get_extrusion_roles(), GCodeViewer.cpp:3920-3921). With a parsed payload
  // the row set therefore follows m_roleFilamentLength's presence set. Without
  // a payload the full 18-row list is kept: the no-gcode UI shape is locked by
  // ViewModelSmokeTests (roleVisibilities().size() == 18).
  QVariantList rows;
  static const int kExcludedRoles[] = {0, 14};  // None, Custom
  const bool filterByContent = !segments_.empty();
  double roleTimeTotal = 0.0;
  for (auto it = m_roleTimeByIndex.cbegin(); it != m_roleTimeByIndex.cend(); ++it)
    roleTimeTotal += it.value();
  for (int role = 1; role < 20; ++role)
  {
    bool excluded = false;
    for (int ex : kExcludedRoles)
    {
      if (role == ex) { excluded = true; break; }
    }
    if (excluded)
      continue;
    if (filterByContent && !m_roleFilamentLength.contains(role))
      continue;  // role absent from the loaded gcode (upstream get_roles)
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
    // FeatureType row columns (GCodeViewer.cpp:3926-3931): Time / % / Usage
    // length / Usage weight. Percent keeps the role-time-sum denominator
    // convention already used by m_legendRoleColumns (upstream divides by
    // total_estimated_time). Weight converts the filament length with the
    // same area/density math as filamentSplit(). Empty text where the value
    // is 0, like upstream's "" cells (GCodeViewer.cpp:3852-3853).
    const double secs = m_roleTimeByIndex.value(role, 0.0);
    row.insert(QStringLiteral("time"),
               secs > 0.0 ? formatTime(float(secs)) : QString());
    row.insert(QStringLiteral("percent"),
               roleTimeTotal > 0.0
                   ? QString::number(secs / roleTimeTotal * 100.0, 'f', 1)
                   : QStringLiteral("0.0"));
    const double lengthMm = m_roleFilamentLength.value(role, 0.0);
    row.insert(QStringLiteral("usageLength"),
               lengthMm > 0.0 ? formatFilamentLength(lengthMm) : QString());
    const double radius = m_filamentDiameter * 0.5;
    const double area = 3.14159265 * radius * radius;
    const double grams = lengthMm * area * m_filamentDensity * 0.001;
    row.insert(QStringLiteral("usageWeight"),
               grams > 0.0 ? formatFilamentWeight(grams) : QString());
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

QVariantList PreviewViewModel::legendGradientStops() const
{
  return m_legendGradientStops;
}

QVariantList PreviewViewModel::legendRoleColumns() const
{
  return m_legendRoleColumns;
}

// P17.6: number of color-change ticks (upstream "Filament change times",
// GCodeViewer.cpp:5156-5159).
int PreviewViewModel::colorChangeCount() const
{
  int count = 0;
  for (const auto &tc : tickMarks_)
    if (tc.type == OWzx::TickType::ColorChange)
      ++count;
  return count;
}

// P17.6: custom g-code overview rows (upstream custom_gcode_times and the
// Custom g-code overview table, GCodeViewer.cpp:5479-5545) — one row per
// tick with its layer and the accumulated time at that layer.
QVariantList PreviewViewModel::customGcodeRows() const
{
  QVariantList rows;
  for (const auto &tc : tickMarks_)
  {
    QVariantMap row;
    row.insert(QStringLiteral("type"),
               [tc]() {
                 switch (tc.type)
                 {
                 case OWzx::TickType::PausePrint: return QStringLiteral("Pause");
                 case OWzx::TickType::CustomGcode: return QStringLiteral("Custom");
                 case OWzx::TickType::Template: return QStringLiteral("Template");
                 case OWzx::TickType::ToolChange: return QStringLiteral("Tool change");
                 case OWzx::TickType::ColorChange: return QStringLiteral("Color change");
                 default: return QStringLiteral("Custom");
                 }
               }());
    row.insert(QStringLiteral("layer"), tc.tick);
    row.insert(QStringLiteral("color"), tc.color);
    row.insert(QStringLiteral("extra"), tc.extra);
    row.insert(QStringLiteral("time"),
               tc.tick >= 0 && tc.tick < int(m_layerTimes.size())
                   ? formatTime(m_layerTimes.at(tc.tick))
                   : QStringLiteral("--"));
    rows.append(row);
  }
  return rows;
}

// P17.10: upstream Prepare time — elapsed time before the first extrusion
// (heating / priming moves, GCodeProcessor prepare_time).
QString PreviewViewModel::prepareTime() const
{
  return prepareTimeCaptured_ ? formatTime(prepareTimeSeconds_)
                              : QStringLiteral("--:--:--");
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
  m_roleFilamentLength.clear();
  m_roleTimeByIndex.clear();
  m_moveAccumulatedTime.clear();
  prepareTimeSeconds_ = 0.f;
  prepareTimeCaptured_ = false;
  m_fullConfig.clear();
  const bool hadTicks = !tickMarks_.isEmpty();
  tickMarks_.clear();
  m_maxLayerTime = 0.f;
  // Phase 238 (PREV-03/05/06): clear the parser-side move-kind counts, the
  // filament split, the estimated-time comments, and the per-extruder price /
  // visibility maps (visibility resets to all-visible like upstream
  // m_tool_visibles reload, GCodeViewer.cpp:1109-1116).
  for (int &c : m_kindCounts)
    c = 0;
  for (double &v : m_filamentSplitLength)
    v = 0.0;
  m_normalTimeSecs = 0.f;
  m_stealthTimeSecs = 0.f;
  m_filamentPrices.clear();
  m_extruderVisibility.clear();
  layerCount_ = 0;
  moveCount_ = 0;
  currentMove_ = 0;
  currentLayerMin_ = 0;
  currentLayerMax_ = 0;
  singleLayer_ = false;
  // PREV-PLAY: stale per-layer bounds must not outlive their segments.
  layerFirstMove_.clear();
  layerLastMove_.clear();
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
  currentGcodeRowIndex_ = -1;  // gcode-window-follow: cleared with the window.
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
  toolKind_ = 0;
  toolTypeText_.clear();
  toolLineTypeText_.clear();
  toolHeight_ = 0;
  toolLength_ = 0;
  toolFlowRate_ = 0;
  toolJerk_ = 0;
  toolPressureAdvance_ = 0;
  toolEstimatedTime_.clear();
  toolMoveTimeSecs_ = 0;
  toolDetailText_.clear();
  toolActualSpeedExist_ = false;
  toolActualSpeedProfile_.clear();
  toolActualSpeedYMin_ = 0;
  toolActualSpeedYMax_ = 0;
  m_actualSpeedMin = std::numeric_limits<float>::max();
  m_actualSpeedMax = 0.f;
  // P1: a reset invalidates any in-flight deferred source-index pass (the
  // queued lambda compares against pendingSourceIndexPath_ and no-ops) and
  // drops the transient direct-walk line index. lastLoadedPath_ /
  // lastConsumedResultId_ are NOT cleared here -- rebuildFromGCode /
  // loadFromProcessorResult set them at their END, so a reset followed by an
  // empty-path sync (resetPreviewState branch in syncPreviewWithActiveResult)
  // still needs the stale values to detect "nothing changed".
  pendingSourceIndexPath_.clear();
  directMoveLineIndex_.clear();
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
  // v5.11: M-code state for ActualSpeed/Jerk/ActualFlow/PressureAdvance view modes.
  float currentSpeedFactor = 1.f;   // M220 S<percent>/100
  float currentFlowFactor = 1.f;    // M221 S<percent>/100
  float currentJerk = 0.f;          // M205 X/Y/E value
  float currentPA = 0.f;            // M900 K value / SET_PRESSURE_ADVANCE ADVANCE
  int currentExtruder = 0;
  bool relativeExtrusion = false;
  // P17.9 (PARSER): G90/G91 absolute XYZ positioning state and G4 dwell
  // accumulation (pure time advance folded into the total).
  bool absolutePositioning = true;
  float dwellSeconds = 0.f;
  // P17.9: when the gcode carries ;TIME_ELAPSED tags, layer times come from
  // the tags (authoritative) and the accel-model pass is skipped.
  bool elapsedTagSeen = false;
  float currentWidth = 0.f;
  float currentHeight = 0.f;
  float elapsedTime = 0.f;
  float layerStartElapsed = 0.f;
  float currentLayerTime = 0.f;
  float printLayerZ = 0.f;
  bool hasPrintLayerZ = false;

  // Filament tracking
  float totalFilamentUsed = 0.f; // mm of filament extruded
  float filamentDiameter = kDefaultFilamentDiameter;
  float filamentDensity = kDefaultFilamentDensity;  // default PLA g/cm3
  int extrudeMoveCount = 0;
  int travelMoveCount = 0;
  int toolChangeCount = 0;
  double feedrateSum = 0.0;
  int feedrateCount = 0;

  // Phase 238 (PREV-03): wipe/flush comment state + seam detector, ported
  // from the upstream GCodeProcessor comment tags (" WIPE_START"/" WIPE_END"
  // and " FLUSH_START"/" FLUSH_END", GCodeProcessor.cpp:2261-2278 + :2283-2291)
  // and the outer-wall loop seam detector (GCodeProcessor.cpp:3307-3345).
  bool wiping = false;
  bool flushing = false;
  bool seamDetectorActive = false;
  bool seamHasFirstVertex = false;
  float seamFirstX = 0.f, seamFirstY = 0.f, seamFirstZ = 0.f;

  // Per-role time tracking aligned with upstream PrintEstimatedStatistics::roles_times.
  QHash<QString, double> roleTimeAccum; // TYPE label to accumulated time in seconds.
  auto accumulateRoleTime = [&](const QString &role, float dx, float dy, float dz, float feed) {
    if (feed <= 0.f) return;
    const float dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    roleTimeAccum[role] += dist / feed * 60.0; // feed is mm/min, dist is mm to seconds.
  };

  // Phase 238 (PREV-03): append a zero-displacement move marker (retract /
  // unretract / seam) as a short vertical tick segment. Upstream renders
  // these as GL_POINTS with the Options_Colors palette
  // (GCodeViewer.cpp:718-727); the Qt6 line pipeline has no glPointSize
  // equivalent, so a kMarkerTickHeight tick stands in (documented delta).
  // The marker advances moveIndex so playback reveals it at the right time.
  auto appendMarkerSegment = [&](int markerKind, float mx, float my, float mz, int lineNo) {
    StoredSegment seg;
    seg.x1 = mx;
    seg.y1 = my;
    seg.z1 = mz;
    seg.x2 = mx;
    seg.y2 = my;
    seg.z2 = mz + kMarkerTickHeight;
    const float *kc = kindBaseColor(markerKind);
    seg.baseR = kc[0];
    seg.baseG = kc[1];
    seg.baseB = kc[2];
    seg.feedrate = 0.f;
    seg.fan_speed = currentFanSpeed;
    seg.temperature = currentTemp;
    seg.width = currentWidth;
    seg.height = currentHeight;
    seg.layer_time = currentLayerTime;
    seg.acceleration = 0.f;
    seg.extruder_id = currentExtruder;
    seg.layer = layer;
    seg.move = moveIndex;
    seg.isTravel = true;
    seg.role = 0;
    seg.kind = markerKind;
    seg.gcodeLine = lineNo;
    segments_.push_back(seg);
    m_kindCounts[markerKind] += 1;
    featureCount_[kindFeatureLabel(markerKind)] += 1;
    // No elapsed time passes for a zero-displacement marker.
    const float prev = m_moveAccumulatedTime.empty() ? 0.f : m_moveAccumulatedTime.back();
    m_moveAccumulatedTime.push_back(prev);
    ++moveIndex;
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
      // P17.9: full-config header — OrcaSlicer writes `; key = value` lines
      // in the file header; upstream apply_config(DynamicPrintConfig) reads
      // exactly these (GCodeProcessor.cpp:1575-1602 pre-parse + :1796).
      if (raw.startsWith(QStringLiteral("; "))
          && raw.contains(QStringLiteral(" = ")))
      {
        const QString body = raw.mid(2);
        const int eq = body.indexOf(QStringLiteral(" = "));
        if (eq > 0)
          m_fullConfig.insert(body.left(eq).trimmed(),
                              body.mid(eq + 3).trimmed());
      }
      if (raw.startsWith(QStringLiteral(";TYPE:")))
        currentType = raw.mid(6).trimmed();
      else
      {
        // PERF(PREV-PARSE): constant pattern, compiled once (was rebuilt per
        // comment line).
        static const QRegularExpression featureRe(
            QStringLiteral("^;\\s*FEATURE\\s*:\\s*(.+)$"),
            QRegularExpression::CaseInsensitiveOption);
        const auto featureMatch = featureRe.match(raw);
        if (featureMatch.hasMatch())
          currentType = featureMatch.captured(1).trimmed();
      }

      const QString upperComment = raw.toUpper();
      // Phase 238 (PREV-03): wipe state tags. Upstream Reserved_Tags are
      // " WIPE_START"/" WIPE_END" (GCodeProcessor.cpp:55-75) -- matched here
      // contains-wise so both the BBS ("; WIPE_START") and plain forms work.
      // " WIPE_TOWER_START" does NOT contain "WIPE_START" so tower tags do
      // not leak into the wiping flag.
      if (upperComment.contains(QStringLiteral("WIPE_START")))
        wiping = true;
      else if (upperComment.contains(QStringLiteral("WIPE_END")))
        wiping = false;
      // Phase 238 (PREV-05): flush volume tags " FLUSH_START"/" FLUSH_END"
      // (GCodeProcessor.cpp:98-99). Unretract moves inside the region count
      // as flushed filament (upstream :3064-3074).
      if (upperComment.contains(QStringLiteral("FLUSH_START")))
        flushing = true;
      else if (upperComment.contains(QStringLiteral("FLUSH_END")))
        flushing = false;
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
        elapsedTagSeen = true;
      }
      // P1-parity: width tags follow the upstream tag grammar -- the tag must
      // OPEN the comment (GCodeProcessor.cpp:4707 pos==0). The previous
      // contains-match also swallowed free-text header prose like "; top
      // infill extrusion width = 0.40mm" and poisoned the width state before
      // the first real tag; the processor (whose moves vector the direct
      // preview path consumes) leaves m_width at its last computed value
      // there instead.
      if (raw.startsWith(QStringLiteral(";WIDTH"), Qt::CaseInsensitive)
          || raw.startsWith(QStringLiteral("; LINE_WIDTH"), Qt::CaseInsensitive))
      {
        if (parseTaggedValue(raw, QStringLiteral("LINE_WIDTH"), taggedValue)
            || parseTaggedValue(raw, QStringLiteral("WIDTH"), taggedValue))
        {
          if (taggedValue > 0.f)
            currentWidth = taggedValue;
        }
      }
      if (parseTaggedValue(raw, QStringLiteral("LAYER_HEIGHT"), taggedValue)
          || parseTaggedValue(raw, QStringLiteral("HEIGHT"), taggedValue))
      {
        if (taggedValue > 0.f)
          currentHeight = taggedValue;
      }
      if (raw.contains(QStringLiteral("filament_diameter")))
      {
        // Phase 238 (PREV-05): allow the "; key = value" spacing of the real
        // gcode config footer (the old [=:] without leading \s* never matched
        // "; filament_diameter = 1.75").
        const QRegularExpression re(QLatin1String("filament_diameter\\s*[=:]\\s*(\\d+(?:\\.\\d+)?)"), QRegularExpression::CaseInsensitiveOption);
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
        const QRegularExpression re(QLatin1String("filament_density\\s*[=:]\\s*(\\d+(?:\\.\\d+)?)"), QRegularExpression::CaseInsensitiveOption);
        const auto m = re.match(raw);
        if (m.hasMatch())
        {
          bool ok = false;
          const float v = m.captured(1).toFloat(&ok);
          if (ok && v > 0.f) filamentDensity = v;
        }
      }
      // Phase 238 (PREV-05): per-extruder filament price per kg from the
      // gcode config block ("; filament_cost = 29.99" / comma list). Upstream
      // reads the filament_cost config key (GCodeProcessor.cpp:1252-1260) and
      // defaults to DEFAULT_FILAMENT_COST 29.99 (GCodeProcessor.cpp:49).
      if (raw.contains(QStringLiteral("filament_cost")))
      {
        const QRegularExpression re(QLatin1String("filament_cost\\s*[=:]\\s*([\\d.,\\s]+)"), QRegularExpression::CaseInsensitiveOption);
        const auto m = re.match(raw);
        if (m.hasMatch())
        {
          const QStringList prices = m.captured(1).split(QLatin1Char(','), Qt::SkipEmptyParts);
          for (int i = 0; i < prices.size(); ++i)
          {
            bool ok = false;
            const double price = prices.at(i).trimmed().toDouble(&ok);
            if (ok && price > 0.0)
              m_filamentPrices[i] = price;
          }
        }
      }
      // Phase 238 (PREV-05): estimated printing time comments. Upstream
      // writes "; estimated printing time (normal mode) = X" and
      // "; estimated printing time (silent mode) = X" (GCodeProcessor.cpp
      // TimeProcessor post-process, Estimated_Printing_Time_Placeholder);
      // silent == the libvgcode Stealth time mode used by the stealth toggle.
      if (upperComment.contains(QStringLiteral("ESTIMATED PRINTING TIME")))
      {
        const int eq = raw.indexOf(QLatin1Char('='));
        if (eq >= 0)
        {
          const float secs = parseEstimatedTimeValue(raw.mid(eq + 1));
          if (secs > 0.f)
          {
            if (upperComment.contains(QStringLiteral("SILENT MODE")))
              m_stealthTimeSecs = secs;
            else if (upperComment.contains(QStringLiteral("NORMAL MODE")))
              m_normalTimeSecs = secs;
          }
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

    // P17.9 (PARSER): G90/G91 set the absolute/relative positioning mode for
    // XYZ (E keeps its own M82/M83 state). Upstream GCodeProcessor tracks the
    // same state (GCodeProcessor.cpp G90/G91 handling).
    if (upper == QStringLiteral("G90") || upper.startsWith(QStringLiteral("G90 ")))
    {
      absolutePositioning = true;
      continue;
    }

    if (upper == QStringLiteral("G91") || upper.startsWith(QStringLiteral("G91 ")))
    {
      absolutePositioning = false;
      continue;
    }

    // P17.9 (PARSER): G4 dwell — pure time advance (S seconds or P
    // milliseconds) folded into the dwell accumulator.
    if (upper == QStringLiteral("G4") || upper.startsWith(QStringLiteral("G4 ")))
    {
      float seconds = 0.f;
      const int sIdx = upper.indexOf(QLatin1Char('S'));
      if (sIdx >= 0)
        seconds = upper.mid(sIdx + 1).toFloat();
      if (seconds <= 0.f)
      {
        const int pIdx = upper.indexOf(QLatin1Char('P'));
        if (pIdx >= 0)
          seconds = upper.mid(pIdx + 1).toFloat() / 1000.f;
      }
      if (seconds > 0.f)
        dwellSeconds += seconds;
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

    // v5.11: Speed factor (M220 S<percent>) — drives ActualSpeed view mode.
    if (upper.startsWith(QStringLiteral("M220")))
    {
      float val = 0.f;
      if (parseAxis(upper, 'S', val) && val >= 0.f)
        currentSpeedFactor = val / 100.f;
    }
    // v5.11: Flow factor (M221 S<percent>) — drives ActualFlow view mode.
    if (upper.startsWith(QStringLiteral("M221")))
    {
      float val = 0.f;
      if (parseAxis(upper, 'S', val) && val >= 0.f)
        currentFlowFactor = val / 100.f;
    }
    // v5.11: Jerk (M205 X/Y/E<value>) — drives Jerk view mode.
    if (upper.startsWith(QStringLiteral("M205")))
    {
      float val = 0.f;
      if (parseAxis(upper, 'X', val) && val > 0.f) currentJerk = val;
      else if (parseAxis(upper, 'Y', val) && val > 0.f) currentJerk = val;
      else if (parseAxis(upper, 'E', val) && val > 0.f) currentJerk = val;
    }
    // v5.11: Pressure advance (M900 K<value>) — drives PressureAdvance view mode.
    if (upper.startsWith(QStringLiteral("M900")))
    {
      float val = 0.f;
      if (parseAxis(upper, 'K', val) && val >= 0.f) currentPA = val;
    }

    // Phase 238 (PREV-03): explicit firmware retract/unretract commands.
    // Upstream G10/G11 synthesize a G1 with E = -/+ retraction_length from
    // the printer config (GCodeProcessor.cpp:3818-3854). The Qt6 parser has
    // no config access here, so the event is recorded as a zero-displacement
    // retract/unretract tick at the current position (documented delta: no
    // E delta or retraction time is attributed).
    if (upper == QStringLiteral("G10") || upper.startsWith(QStringLiteral("G10 ")))
    {
      appendMarkerSegment(KindRetract, x, y, z, sourceLineNumber);
      continue;
    }
    if (upper == QStringLiteral("G11") || upper.startsWith(QStringLiteral("G11 ")))
    {
      appendMarkerSegment(KindUnretract, x, y, z, sourceLineNumber);
      continue;
    }

    // P17.9 (PARSER): the per-move processing shared by G0/G1 lines and
    // the G2/G3 arc chords (upstream processes arcs by segmenting them
    // into straight moves before classification).
    const auto processStraightMove = [&](float nx, float ny, float nz,
                                        float ne, bool hasE) {
        // P17.9: works for both E modes — the caller converts relative E
        // into an absolute ne before calling.
        const float extrusionDelta = hasE ? ne - e : 0.f;
        const bool xyMoved = (nx != x) || (ny != y);
    const bool zMoved = (nz != z);
    // Phase 238 (PREV-03): upstream move classification. E-only lines are
    // no longer dropped: dE<0 without movement = Retract, dE>0 without XY
    // movement = Unretract (GCodeProcessor.cpp:2954-2968). Lines with no
    // displacement at all stay skipped (upstream Noop).
    if (!xyMoved && !zMoved && !hasE)
    {
      x = nx;
      y = ny;
      z = nz;
      return;
    }
    const int kind = classifyMoveKind(extrusionDelta, xyMoved, zMoved, wiping);
    if (kind < 0)
    {
      x = nx;
      y = ny;
      z = nz;
      if (hasE)
        e = ne;
      return;
    }
    const bool extruding = (kind == KindExtrude) && extrusionDelta > 0.00001f;

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

    // Phase 238 (PREV-03): seam detection, ported from upstream
    // GCodeProcessor.cpp:3305-3345. The detector activates on the first
    // Outer wall (erExternalPerimeter, canonical index 2) extrusion and
    // records the loop's FIRST vertex ONCE (upstream has_first_vertex,
    // GCodeProcessor.cpp:3310-3311). Any move that is not an extrusion, or
    // an extrusion whose role is neither Outer wall nor Overhang wall
    // (indices 2/3), closes the detector: when that closing move's start
    // (= the previous move endpoint, upstream m_result.moves.back().position)
    // is within the upstream squared threshold 0.0625 (= 0.25mm) of the
    // first vertex, a Seam marker is stored at the midpoint.
    {
      const int currentRole = roleForTypeImpl(currentType);
      const bool wallLoopRole = (currentRole == 2 || currentRole == 3);
      if (seamDetectorActive)
      {
        if (kind == KindExtrude && currentRole == 2)
        {
          if (!seamHasFirstVertex)
          {
            seamFirstX = x;
            seamFirstY = y;
            seamFirstZ = z;
            seamHasFirstVertex = true;
          }
        }
        else if ((kind != KindExtrude || !wallLoopRole) && seamHasFirstVertex)
        {
          const float sdx = x - seamFirstX;
          const float sdy = y - seamFirstY;
          const float sdz = z - seamFirstZ;
          if (sdx * sdx + sdy * sdy + sdz * sdz < 0.0625f)
            appendMarkerSegment(KindSeam,
                                0.5f * (x + seamFirstX),
                                0.5f * (y + seamFirstY),
                                0.5f * (z + seamFirstZ),
                                sourceLineNumber);
          seamDetectorActive = false;
          seamHasFirstVertex = false;
        }
      }
      else if (kind == KindExtrude && currentRole == 2)
      {
        seamDetectorActive = true;
        seamFirstX = x;
        seamFirstY = y;
        seamFirstZ = z;
        seamHasFirstVertex = true;
      }
    }

    const float dx = nx - x;
    const float dy = ny - y;
    const float dz = nz - z;
    const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
    float volumetricRate = 0.f;
    if (extruding && dist > 0.f && currentFeedrate > 0.f && extrusionDelta > 0.f)
    {
      const float filamentArea = filamentCrossAreaMm2(filamentDiameter);
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
      // P17.4: per-role filament length for the FeatureType legend column
      // (upstream legend shows Used filament per role).
      const int parsedRole = roleForTypeImpl(currentType);
      m_roleFilamentLength[parsedRole] += extrusionDelta;
      // FeatureType legend Time/% columns (GCodeViewer.cpp:3728-3730 header
      // row, :3926-3931 role row columns): the same seconds accumulateRoleTime()
      // computes, keyed by canonical role so roleVisibilities() rows stay
      // index-aligned with the presence set (upstream ViewerImpl.cpp:1017-1018
      // collects only Extrude vertices).
      if (currentFeedrate > 0.f)
        m_roleTimeByIndex[parsedRole] +=
            std::sqrt(dx * dx + dy * dy + dz * dz) / currentFeedrate * 60.0;
      // P17.10: upstream Prepare time = elapsed before the first extrusion
      // (heating/priming, GCodeProcessor prepare_time).
      if (!prepareTimeCaptured_)
      {
        prepareTimeSeconds_ = m_moveAccumulatedTime.empty()
            ? 0.f
            : m_moveAccumulatedTime.back();
        prepareTimeCaptured_ = true;
      }
    }
    else if (kind == KindUnretract && flushing)
    {
      // Phase 238 (PREV-05): unretract inside a FLUSH_START..FLUSH_END region
      // counts as flushed filament and participates in the total (upstream
      // GCodeProcessor.cpp:3064-3074 update_flush_per_filament).
      totalFilamentUsed += extrusionDelta;
      m_extruderUsedLength[currentExtruder] += extrusionDelta;
    }

    if (kind == KindTravel)
      ++travelMoveCount;
    // Fine-grained role assignment: map the ;TYPE: display string DIRECTLY to
    // the canonical libvgcode index and bake the per-role base color from
    // kRoleColors (FeatureType mode). Both arrays use the same canonical index,
    // so every role -- including the divergent ones (Ironing->7, Bottom
    // surface->15) -- gets the correct color and visibility slot.
    const int role = extruding ? roleForTypeImpl(currentType) : 0;

    // Phase 238 (PREV-05): category split for the extruded volume -- the
    // shared filamentSplitBucketForRole() (upstream GCodeProcessor.cpp
    // :3002-3010 port), also used by the direct walk. Flushed is accumulated
    // at the unretract-while-flushing branch above (upstream
    // update_flush_per_filament).
    if (extruding)
    {
      m_filamentSplitLength[filamentSplitBucketForRole(role)] += extrusionDelta;
    }
    else if (kind == KindUnretract && flushing)
    {
      m_filamentSplitLength[2] += extrusionDelta;
    }

    StoredSegment seg;
    seg.x1 = x;
    seg.y1 = y;
    seg.z1 = z;
    seg.x2 = nx;
    seg.y2 = ny;
    seg.z2 = nz;
    if (extruding)
    {
      seg.baseR = kRoleColors[role][0];
      seg.baseG = kRoleColors[role][1];
      seg.baseB = kRoleColors[role][2];
    }
    else
    {
      // Phase 238 (PREV-03): travel/retract/unretract/wipe use the upstream
      // Travel_Colors/Options_Colors/Wipe_Color base colors
      // (GCodeViewer.cpp:718-751).
      const float *kc = kindBaseColor(kind);
      seg.baseR = kc[0];
      seg.baseG = kc[1];
      seg.baseB = kc[2];
      // P17.4: upstream travel_color(path) tri-state by delta_extruder
      // (GCodeViewer.cpp:3234-3236): <0 Retract / >0 Extrude / else Move.
      // P1 review follow-up: the tint constants are the file-level
      // kTravel{Extrude,Retract}Color shared with the direct walk.
      if (kind == KindTravel)
      {
        if (extrusionDelta < 0.f)
        {
          seg.baseR = kTravelRetractColor[0];
          seg.baseG = kTravelRetractColor[1];
          seg.baseB = kTravelRetractColor[2];
        }
        else if (extrusionDelta > 0.f)
        {
          seg.baseR = kTravelExtrudeColor[0];
          seg.baseG = kTravelExtrudeColor[1];
          seg.baseB = kTravelExtrudeColor[2];
        }
      }
    }
    seg.feedrate = currentFeedrate;
    seg.fan_speed = currentFanSpeed;
    seg.temperature = currentTemp;
    seg.width = currentWidth;
    seg.height = currentHeight > 0.f ? currentHeight : nz;
    seg.layer_time = currentLayerTime;
    seg.acceleration = currentAccel;
    seg.volumetric_rate = volumetricRate;
    seg.jerk = currentJerk;
    seg.pressure_advance = currentPA;
    seg.actual_speed = currentFeedrate * currentSpeedFactor;
    seg.actual_flow = currentFlowFactor;
    seg.extruder_id = currentExtruder;
    seg.layer = layer;
    seg.move = moveIndex;
    seg.isTravel = (kind == KindTravel || kind == KindWipe);
    seg.role = role;
    seg.kind = kind;
    seg.gcodeLine = sourceLineNumber;
    segments_.push_back(seg);
    // Global actual-speed range for the tooltip profile y axis (upstream
    // ActualSpeed color_range interval, GCodeViewer.cpp:1740-1741).
    if (seg.actual_speed < m_actualSpeedMin)
      m_actualSpeedMin = seg.actual_speed;
    if (seg.actual_speed > m_actualSpeedMax)
      m_actualSpeedMax = seg.actual_speed;

    // Accumulate elapsed time for the move slider, aligned with upstream IMSlider m_layers_times.
    {
      const float dt = currentFeedrate > 0.f ? dist / currentFeedrate * 60.f : 0.f;
      const float prev = m_moveAccumulatedTime.empty() ? 0.f : m_moveAccumulatedTime.back();
      m_moveAccumulatedTime.push_back(prev + dt);
    }

    if (extruding)
      featureCount_[roleLabel(role)] += 1;
    else
      featureCount_[kindFeatureLabel(kind)] += 1;
    m_kindCounts[kind] += 1;

    x = nx;
    y = ny;
    z = nz;
    if (hasE)
      e = ne;
    ++moveIndex;
    };

    // P1 (G28 parity with the direct path / upstream): GCodeProcessor::
    // process_G28 synthesizes a G1 move to the homed axes -- bare G28 -> X0
    // Y0 Z0, "G28 X0" -> X0 only -- so the preview renders the homing travel
    // (GCodeProcessor.cpp process_G28). The text path previously ignored G28
    // lines entirely, which left exactly the end-gcode "G28 X0" park move
    // missing next to the direct GCodeProcessorResult product. Relative
    // positioning (G91) targets 0 as a delta, i.e. no displacement, matching
    // upstream's extract_absolute_position_on_axis.
    if (upper == QStringLiteral("G28") || upper.startsWith(QStringLiteral("G28 ")))
    {
      const QStringList tokens = upper.mid(3).split(QLatin1Char(' '), Qt::SkipEmptyParts);
      bool homeX = false, homeY = false, homeZ = false;
      for (const QString &token : tokens)
      {
        if (token.startsWith(QLatin1Char('X')))
          homeX = true;
        else if (token.startsWith(QLatin1Char('Y')))
          homeY = true;
        else if (token.startsWith(QLatin1Char('Z')))
          homeZ = true;
      }
      if (!homeX && !homeY && !homeZ)
        homeX = homeY = homeZ = true;
      const float nx = homeX ? (absolutePositioning ? 0.f : x) : x;
      const float ny = homeY ? (absolutePositioning ? 0.f : y) : y;
      const float nz = homeZ ? (absolutePositioning ? 0.f : z) : z;
      processStraightMove(nx, ny, nz, e, false);
      continue;
    }

    // P17.9 (PARSER): G2/G3 arcs — segmented into straight chords that run
    // through the same per-move processing (upstream segments arcs during
    // parsing so every toolpath vertex is a straight move). E is distributed
    // proportionally to chord length; feedrate carries over. I/J form uses
    // the center offset; R form solves the center on the perpendicular
    // bisector (minor arc for the given direction).
    // R-P1.B: exact word match with a space guard -- bare startsWith("G2")
    // also matched G20/G21/G28-G29, so e.g. "G28 X0 Y0" was parsed as an
    // R=0 arc (upstream GCodeProcessor tokenizes the command word,
    // GCodeProcessor.cpp processG2_G3 dispatch).
    // G-10: also accept the leading-zero spellings (G02/G03) -- upstream
    // tokenizes the command number so "G02" dispatches with "G2"
    // (GCodeProcessor.cpp); slicer output is canonical, but post-processors
    // and firmware echoes sometimes zero-pad the word.
    const bool arcCW = upper == QStringLiteral("G2") || upper.startsWith(QStringLiteral("G2 "))
                    || upper == QStringLiteral("G02") || upper.startsWith(QStringLiteral("G02 "));
    const bool arcCCW = upper == QStringLiteral("G3") || upper.startsWith(QStringLiteral("G3 "))
                     || upper == QStringLiteral("G03") || upper.startsWith(QStringLiteral("G03 "));
    if (arcCW || arcCCW)
    {
      const bool clockwise = arcCW;
      float targetX = x, targetY = y;
      float iOffset = 0.f, jOffset = 0.f, radius = 0.f;
      parseAxis(upper, 'X', targetX);
      parseAxis(upper, 'Y', targetY);
      parseAxis(upper, 'I', iOffset);
      parseAxis(upper, 'J', jOffset);
      parseAxis(upper, 'R', radius);
      const float lineF = parseFValue(upper);
      if (lineF > 0.f)
        currentFeedrate = lineF;
      float parsedEArc = 0.f;
      const bool hasEArc = parseAxis(upper, 'E', parsedEArc);
      const float arcE = hasEArc ? (relativeExtrusion ? parsedEArc
                                                      : parsedEArc - e) : 0.f;

      float centerX = x + iOffset;
      float centerY = y + jOffset;
      float startAngle = std::atan2(y - centerY, x - centerX);
      float endAngle = std::atan2(targetY - centerY, targetX - centerX);
      if (iOffset == 0.f && jOffset == 0.f)
      {
        // R form: center on the perpendicular bisector at distance h from
        // the chord midpoint; pick the side that yields the minor arc.
        const float mx = (x + targetX) * 0.5f;
        const float my = (y + targetY) * 0.5f;
        const float chordDx = targetX - x;
        const float chordDy = targetY - y;
        const float chordLen = std::sqrt(chordDx * chordDx + chordDy * chordDy);
        if (chordLen < 1e-6f)
          continue;
        const float h2 = radius * radius - chordLen * chordLen * 0.25f;
        const float h = std::sqrt(std::max(0.f, h2));
        const float side = clockwise ? 1.f : -1.f;
        centerX = mx + side * h * chordDy / chordLen;
        centerY = my - side * h * chordDx / chordLen;
        startAngle = std::atan2(y - centerY, x - centerX);
        endAngle = std::atan2(targetY - centerY, targetX - centerX);
      }
      float sweep = endAngle - startAngle;
      if (clockwise && sweep > 0.f)
        sweep -= 2.f * float(M_PI);
      else if (!clockwise && sweep < 0.f)
        sweep += 2.f * float(M_PI);

      // Chords: ~0.5mm resolution, clamped to [8, 128] segments.
      const float arcRadius = std::sqrt((x - centerX) * (x - centerX)
                                        + (y - centerY) * (y - centerY));
      const int chordCount = qBound(8, int(std::abs(sweep) * arcRadius / 0.5f) + 1, 128);
      for (int chord = 1; chord <= chordCount; ++chord)
      {
        const float angle = startAngle + sweep * float(chord) / float(chordCount);
        const float chordX = centerX + arcRadius * std::cos(angle);
        const float chordY = centerY + arcRadius * std::sin(angle);
        const float chordE = e + arcE * float(chord) / float(chordCount);
        processStraightMove(chordX, chordY, z, chordE, hasEArc);
      }
      continue;
    }

    // G-10: leading-zero spellings (G00/G01) dispatch like G0/G1.
    const bool isG0 = upper == QStringLiteral("G0") || upper.startsWith(QStringLiteral("G0 "))
                   || upper == QStringLiteral("G00") || upper.startsWith(QStringLiteral("G00 "));
    const bool isG1 = upper == QStringLiteral("G1") || upper.startsWith(QStringLiteral("G1 "))
                   || upper == QStringLiteral("G01") || upper.startsWith(QStringLiteral("G01 "));
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
    // P17.9 (PARSER): G91 makes XYZ moves relative to the current position
    // (upstream GCodeProcessor G91 handling); E keeps the M82/M83 state.
    if (!absolutePositioning)
    {
      nx = x + nx;
      ny = y + ny;
      nz = z + nz;
    }

    float parsedE = 0.f;
    const bool hasE = parseAxis(upper, 'E', parsedE);
    if (hasE)
    {
      if (relativeExtrusion)
        ne = e + parsedE;
      else
        ne = parsedE;
    }


    processStraightMove(nx, ny, nz, ne, hasE);
  }

  moveCount_ = moveIndex;
  layerCount_ = qMax(1, hasPrintLayerZ ? layer + 1 : 1);

  // P17.9: acceleration-aware trapezoidal time model (upstream
  // GCodeProcessor TimeMachine, GCodeProcessor.cpp:2500-2700 — simplified
  // global planner). Backward pass bounds entry velocities from the 0-exit
  // end constraint; forward pass bounds by cruise feed + accel reach; the
  // trapezoid/triangle formula yields each move duration. Replaces the
  // dist/feed cruise estimate so accel-limited small moves take realistic
  // longer times (roles/layer/move accumulators rebuilt from the new dt).
  if (!segments_.empty() && !elapsedTagSeen)
  {
    const int n = int(segments_.size());
    QVector<float> dist(n), cruise(n), acc(n);
    for (int i = 0; i < n; ++i)
    {
      const auto &sg = segments_[i];
      const float dx = sg.x2 - sg.x1, dy = sg.y2 - sg.y1, dz = sg.z2 - sg.z1;
      dist[i] = std::sqrt(dx * dx + dy * dy + dz * dz);
      cruise[i] = sg.feedrate > 0.f ? sg.feedrate / 60.f : 0.f; // mm/min -> mm/s
      acc[i] = sg.acceleration > 0.f ? sg.acceleration : 1250.f;
    }
    QVector<float> ve(n), vx(n);
    float running = 0.f;
    for (int i = n - 1; i >= 0; --i)
    {
      running = std::min(cruise[i],
                         std::sqrt(running * running + 2.f * acc[i] * std::max(dist[i], 0.f)));
      ve[i] = running;
      vx[i] = running;
    }
    float ventry = 0.f;
    QVector<float> dt(n, 0.f);
    for (int i = 0; i < n; ++i)
    {
      const float vstart = std::min(ventry, cruise[i]);
      float vend = std::min(ve[i], std::sqrt(vstart * vstart + 2.f * acc[i] * dist[i]));
      if (dist[i] < 1e-6f)
      {
        dt[i] = 0.f;
        ventry = 0.f;
        continue;
      }
      if (vstart * vstart + 2.f * acc[i] * dist[i] < vend * vend)
        vend = std::sqrt(vstart * vstart + 2.f * acc[i] * dist[i]);
      if (vend > 1e-3f && vstart * vstart + 2.f * acc[i] * dist[i] >= vend * vend
          && vstart < cruise[i] + 1e-3f)
        dt[i] = (vend - vstart) / acc[i]
            + std::max(0.f, dist[i] - (vend * vend - vstart * vstart) / (2.f * acc[i])) / vend;
      else
        dt[i] = (vend + vstart) > 1e-3f ? 2.f * dist[i] / (vend + vstart) : 0.f;
      ventry = vend;
    }

    // Rebuild the accumulators from the accel-corrected durations.
    m_moveAccumulatedTime.clear();
    m_moveAccumulatedTime.reserve(n);
    float accElapsed = 0.f;
    for (int i = 0; i < n; ++i)
    {
      accElapsed += dt[i];
      m_moveAccumulatedTime.push_back(accElapsed);
    }
    // Per-layer durations, in first-appearance layer order.
    m_layerTimes.assign(size_t(qMax(1, layerCount_)), 0.f);
    QHash<int, float> layerDt;
    for (int i = 0; i < n; ++i)
    {
      const int li = qBound(0, segments_[i].layer, layerCount_ - 1);
      layerDt[li] += dt[i];
    }
    for (const float v : layerDt)
      Q_UNUSED(v);
    for (int li = 0; li < layerCount_; ++li)
      m_layerTimes[li] = layerDt.value(li, 0.f);
    m_maxLayerTime = 0.f;
    for (const auto &entry : layerDt)
      m_maxLayerTime = std::max(m_maxLayerTime, entry);
    // Role times: rebuild per-role durations from the accel-corrected dt
    // (the parse-time values used a cruise-only estimate).
    QHash<int, double> roleSeconds;
    for (int i = 0; i < n; ++i)
    {
      const auto &sg = segments_[i];
      if (sg.kind != KindExtrude)
        continue;
      roleSeconds[sg.role] += dt[i];
    }
    for (auto it = roleTimeAccum.begin(); it != roleTimeAccum.end(); ++it)
    {
      const int role = roleForType(it.key());
      it.value() = roleSeconds.value(role, 0.0);
    }
  } else {
    // Elapsed-tag files: the ;TIME_ELAPSED tags are authoritative — the
    // legacy final append (last layer's duration) closes the series.
    m_layerTimes.append(currentLayerTime);
    m_maxLayerTime = qMax(m_maxLayerTime, currentLayerTime);
  }

  currentLayerMin_ = 0;
  currentLayerMax_ = layerCount_ - 1;
  currentMove_ = moveCount_;
  // PREV-PLAY: bounds land with the finalized segments so the first play
  // press can restart from the visible range start.
  rebuildLayerMoveBounds();
  extrudeMoveCount_ = extrudeMoveCount;
  travelMoveCount_ = travelMoveCount;
  toolChangeCount_ = toolChangeCount;

  // Average speed
  // R-P1.B: F values are mm/min (parseFValue); the label is mm/s (upstream
  // MoveVertex::feedrate carries mm/s, GCodeProcessor.hpp:161). Convert like
  // the cruise computation above (feedrate / 60) instead of labeling
  // mm/min as mm/s (60x error).
  if (feedrateCount > 0)
    avgSpeed_ = QStringLiteral("%1 mm/s").arg(feedrateSum / feedrateCount / 60.0, 0, 'f', 1);
  else
    avgSpeed_ = QStringLiteral("--");

  applyFilamentTotals(totalFilamentUsed, filamentDiameter, filamentDensity);
  buildRoleTimeBreakdown(roleTimeAccum);

  updateToolPositionData();
  rebuildGcodeLineWindow();
  recolorAndPackSegments();
  // Phase 238 (PREV-05): honor the parsed "; estimated printing time"
  // comments. When the slice service did not supply a label, the normal-mode
  // comment becomes the total; the stealth total switches to the parsed
  // silent-mode value when present (else the documented x1.4 heuristic).
  if (m_normalTimeSecs > 0.f && estimatedTime_.contains(QStringLiteral("--")))
  {
    estimatedTime_ = formatTime(m_normalTimeSecs);
    totalTime_ = estimatedTime_;
  }
  // P17.9 (PARSER): G4 dwell adds pure time to the totals (upstream
  // GCodeProcessor accumulates dwell into the estimated print time).
  if (dwellSeconds > 0.f)
  {
    if (!estimatedTime_.contains(QStringLiteral("--")))
    {
      const float withDwell = parseTimeSecs(estimatedTime_) + dwellSeconds;
      estimatedTime_ = formatTime(withDwell);
    }
    totalTime_ = formatTime(parseTimeSecs(totalTime_) + dwellSeconds);
  }
  if (stealthMode_)
  {
    totalTime_ = m_stealthTimeSecs > 0.f
        ? formatTime(m_stealthTimeSecs)
        : formatTime(parseTimeSecs(estimatedTime_) * 1.4f);
  }
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

// P1 (preview direct hand-off): shared finalization tail -- the filament
// totals block of rebuildFromGCode, extracted verbatim so the text path and
// loadFromProcessorResult compute identical derived stats from their
// accumulators (单一结构双入口).
void PreviewViewModel::applyFilamentTotals(float totalFilamentUsedMm,
                                           float diameterMm, float densityGcm3)
{
  if (segments_.empty())
  {
    filamentUsed_ = QStringLiteral("--");
    filamentWeight_ = QStringLiteral("--");
    estimatedCost_ = QStringLiteral("--");
    m_totalFilamentGrams = 0.0;
    return;
  }
  filamentUsed_ = QStringLiteral("%1 m").arg(totalFilamentUsedMm / 1000.f, 0, 'f', 2);
  // Weight = volume_mm3 * density_g_per_cm3 * 1e-3.
  // volume_mm3 = length_mm * pi * (diameter / 2)^2.
  const float volume_mm3 = totalFilamentUsedMm * filamentCrossAreaMm2(diameterMm);
  const float weight_g = volume_mm3 * densityGcm3 * 0.001f;
  filamentWeight_ = QStringLiteral("%1 g").arg(weight_g, 0, 'f', 1);
  m_totalFilamentGrams = weight_g;
  // Phase 238 (PREV-05): keep the parsed diameter/density so the split
  // formatter can convert lengths to grams with the same formula.
  m_filamentDiameter = diameterMm;
  m_filamentDensity = densityGcm3;

  // Compute per-extruder filament weight, aligned with upstream PrintEstimatedStatistics.
  m_extruderUsedWeight.clear();
  for (auto it = m_extruderUsedLength.constBegin(); it != m_extruderUsedLength.constEnd(); ++it)
  {
    const float vol = it.value() * filamentCrossAreaMm2(diameterMm);
    m_extruderUsedWeight[it.key()] = vol * densityGcm3 * 0.001f;
  }

  // Phase 238 (PREV-05): estimated cost = sum over extruders of
  // grams * price_per_kg * 0.001 (upstream GCodeProcessor.cpp:4387-4398).
  // Price comes from the "; filament_cost" gcode config block when present;
  // the fallback default is the upstream DEFAULT_FILAMENT_COST 29.99/kg
  // (GCodeProcessor.cpp:49) -- no longer a hardcoded $20/kg.
  double totalCost = 0.0;
  for (auto it = m_extruderUsedWeight.constBegin(); it != m_extruderUsedWeight.constEnd(); ++it)
  {
    const double pricePerKg = m_filamentPrices.value(it.key(), 29.99);
    totalCost += it.value() * pricePerKg * 0.001;
  }
  estimatedCost_ = QStringLiteral("$%1").arg(totalCost, 0, 'f', 2);
}

// P1: shared finalization tail -- the role-time breakdown of rebuildFromGCode
// (upstream PrintEstimatedStatistics::roles_times), extracted verbatim. Keys
// are the upstream ;TYPE: display strings, identical between the text parse
// and the direct walk (the latter keys by kRoleLabels[canonicalRole], which
// equals role_to_string for every role).
void PreviewViewModel::buildRoleTimeBreakdown(const QHash<QString, double> &roleTimeAccum)
{
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
}

#ifdef HAS_LIBSLIC3R
bool PreviewViewModel::loadFromProcessorResult(const Slic3r::GCodeProcessorResult &result,
                                               const QString &gcodePath)
{
  const QString preservedEstimatedTime = estimatedTime_;
  const QString preservedTotalTime = totalTime_;
  resetPreviewState();
  estimatedTime_ = preservedEstimatedTime;
  totalTime_ = preservedTotalTime;

  segments_.reserve(result.moves.size());
  m_moveAccumulatedTime.reserve(result.moves.size());
  directMoveLineIndex_.reserve(result.moves.size());

  // Upstream EMoveType -> Qt MoveKind. Tool_change / Color_change /
  // Pause_Print / Custom_GCode lines never produce a text-path segment (the
  // parser skips them), and Noop vertices are never stored by the processor
  // (process_G1 early-returns on zero displacement), so those map to -1 and
  // only advance the tracked position below.
  auto kindForMoveType = [](Slic3r::EMoveType type) -> int {
    switch (type)
    {
      case Slic3r::EMoveType::Extrude:   return KindExtrude;
      case Slic3r::EMoveType::Travel:    return KindTravel;
      case Slic3r::EMoveType::Wipe:      return KindWipe;
      case Slic3r::EMoveType::Retract:   return KindRetract;
      case Slic3r::EMoveType::Unretract: return KindUnretract;
      case Slic3r::EMoveType::Seam:      return KindSeam;
      default:                           return -1;
    }
  };

  // Accumulators mirroring the text parser's loop locals one-for-one so the
  // shared finalization tail consumes the same shapes.
  int layer = 0;
  int moveIndex = 0;
  float prevX = 0.f, prevY = 0.f, prevZ = 0.f;
  float printLayerZ = 0.f;
  bool hasPrintLayerZ = false;
  int extrudeMoveCount = 0;
  int travelMoveCount = 0;
  int toolChangeCount = 0;
  double feedrateSum = 0.0;
  int feedrateCount = 0;
  float totalFilamentUsed = 0.f;
  QHash<QString, double> roleTimeAccum;
  int lastExtruderId = 0;
  float layerSeconds = 0.f;  // time[Normal] accumulated inside the OPEN layer

  // Config-sourced scalars (the text path parses the same values from the
  // file header/footer comments the slicer wrote from this config).
  const float filamentDiameter =
      result.filament_diameters.empty() ? kDefaultFilamentDiameter
                                         : result.filament_diameters.front();
  const float filamentDensity =
      result.filament_densities.empty() ? kDefaultFilamentDensity
                                        : result.filament_densities.front();
  for (size_t i = 0; i < result.filament_costs.size(); ++i)
    if (result.filament_costs[i] > 0.0)
      m_filamentPrices[int(i)] = result.filament_costs[i];
  // Authoritative estimated-printing-time comments (text: the
  // "; estimated printing time (normal|silent mode)" header lines) and the
  // upstream prepare time (text: elapsed before the first extrusion).
  m_normalTimeSecs = result.print_statistics.modes[0].time;
  m_stealthTimeSecs = result.print_statistics.modes[1].time;
  prepareTimeSeconds_ = result.print_statistics.modes[0].prepare_time;
  prepareTimeCaptured_ = prepareTimeSeconds_ > 0.f;

  const auto pushAccumulatedTime = [&](float dt) {
    const float prev = m_moveAccumulatedTime.empty() ? 0.f : m_moveAccumulatedTime.back();
    m_moveAccumulatedTime.push_back(prev + dt);
  };

  for (const Slic3r::GCodeProcessorResult::MoveVertex &mv : result.moves)
  {
    // calculate_time INSERTS cloned "actual speed" vertices (internal_only ==
    // true, GCodeProcessor.cpp:7474) into the moves vector -- planner
    // interpolation points that never exist in the exported file. The text
    // path cannot see them, so skip them entirely WITHOUT advancing the
    // tracked position (their interpolated position is not a file line's
    // endpoint).
    if (mv.internal_only)
      continue;

    const float x = mv.position.x();
    const float y = mv.position.y();
    const float z = mv.position.z();

    // Tool-change bookkeeping (text: T lines whose id differs from the
    // current extruder; upstream stores those transitions on the vertices).
    if (int(mv.extruder_id) != lastExtruderId)
    {
      ++toolChangeCount;
      m_toolChangePositions.push_back({moveIndex, int(mv.extruder_id)});
      lastExtruderId = int(mv.extruder_id);
    }

    const int kind = kindForMoveType(mv.type);
    if (kind >= 0)
    {
      const float dE = mv.delta_extruder;
      const bool extruding = kind == KindExtrude && dE > 0.00001f;
      const int role = extruding ? canonicalRoleFromExtrusionRole(mv.extrusion_role) : 0;
      const float dt = mv.time[0];                 // per-move duration, Normal mode
      const float feedrateMmMin = mv.feedrate * 60.f;  // StoredSegment unit is mm/min
      const float actualSpeed = mv.actual_feedrate > 0.f ? mv.actual_feedrate * 60.f
                                                         : feedrateMmMin;

      // Layer table rule, identical to the text parser: a preview layer opens
      // on the first extrusion at a new Z (travel Z-hops never create empty
      // selectable layers).
      if (extruding)
      {
        if (!hasPrintLayerZ)
        {
          printLayerZ = z;
          hasPrintLayerZ = true;
          m_layerZs.append(z);
        }
        else if (!isSameZ(z, printLayerZ))
        {
          m_layerTimes.append(layerSeconds);
          m_maxLayerTime = qMax(m_maxLayerTime, layerSeconds);
          ++layer;
          printLayerZ = z;
          m_layerZs.append(z);
          layerSeconds = 0.f;
        }
      }

      StoredSegment seg;
      if (kind == KindSeam)
      {
        // Seam marker: the same vertical tick the text parser emits for its
        // port of the upstream seam detector (GCodeProcessor.cpp store_seam).
        seg.x1 = x;
        seg.y1 = y;
        seg.z1 = z;
        seg.x2 = x;
        seg.y2 = y;
        seg.z2 = z + kMarkerTickHeight;
      }
      else
      {
        seg.x1 = prevX;
        seg.y1 = prevY;
        seg.z1 = prevZ;
        seg.x2 = x;
        seg.y2 = y;
        seg.z2 = z;
      }
      if (extruding)
      {
        seg.baseR = kRoleColors[role][0];
        seg.baseG = kRoleColors[role][1];
        seg.baseB = kRoleColors[role][2];
      }
      else
      {
        // Phase 238 (PREV-03) base colors + the travel tint by delta_extruder
        // sign (same constants as the text path).
        const float *kc = kindBaseColor(kind);
        seg.baseR = kc[0];
        seg.baseG = kc[1];
        seg.baseB = kc[2];
        // Same tri-state tint as the text path, from the shared
        // kTravel{Extrude,Retract}Color symbols (P1 review follow-up).
        if (kind == KindTravel)
        {
          if (dE < 0.f)
          {
            seg.baseR = kTravelRetractColor[0];
            seg.baseG = kTravelRetractColor[1];
            seg.baseB = kTravelRetractColor[2];
          }
          else if (dE > 0.f)
          {
            seg.baseR = kTravelExtrudeColor[0];
            seg.baseG = kTravelExtrudeColor[1];
            seg.baseB = kTravelExtrudeColor[2];
          }
        }
      }
      seg.feedrate = kind == KindSeam ? 0.f : feedrateMmMin;
      seg.fan_speed = mv.fan_speed;
      seg.temperature = mv.temperature;
      // Authoritative per-move width (the processor's own ;WIDTH handling /
      // geometry fallback) and height (layer thickness, upstream m_height --
      // the text path falls back to the absolute Z because no ;HEIGHT comment
      // exists in slicer output).
      seg.width = mv.width;
      seg.height = mv.height;
      seg.layer_time = 0.f;  // filled from m_layerTimes after the walk
      seg.acceleration = mv.acceleration;
      seg.volumetric_rate = extruding ? mv.feedrate * mv.mm3_per_mm : 0.f;
      seg.jerk = mv.jerk;
      seg.pressure_advance = mv.pressure_advance;
      seg.actual_speed = kind == KindSeam ? 0.f : actualSpeed;
      seg.actual_flow = extruding && mv.feedrate > 0.f && mv.actual_feedrate > 0.f
                            ? mv.actual_feedrate * mv.mm3_per_mm
                            : 0.f;
      seg.extruder_id = int(mv.extruder_id);
      seg.layer = layer;
      seg.move = moveIndex;
      seg.isTravel = (kind == KindTravel || kind == KindWipe);
      seg.role = role;
      seg.kind = kind;
      seg.gcodeLine = int(mv.gcode_id);
      segments_.push_back(seg);
      if (seg.actual_speed < m_actualSpeedMin)
        m_actualSpeedMin = seg.actual_speed;
      if (seg.actual_speed > m_actualSpeedMax)
        m_actualSpeedMax = seg.actual_speed;

      // Seam ticks advance the timeline like the text marker (no duration);
      // every other emitted move advances by the processor's duration.
      pushAccumulatedTime(kind == KindSeam ? 0.f : dt);
      layerSeconds += kind == KindSeam ? 0.f : dt;

      if (extruding)
      {
        featureCount_[roleLabel(role)] += 1;
        totalFilamentUsed += dE;
        m_extruderUsedLength[int(mv.extruder_id)] += dE;
        m_roleFilamentLength[role] += dE;
        // Phase 238 (PREV-05) category split -- the shared
        // filamentSplitBucketForRole() (single source with the text path,
        // P1 review follow-up).
        m_filamentSplitLength[filamentSplitBucketForRole(role)] += dE;
        if (feedrateMmMin > 0.f)
        {
          feedrateSum += feedrateMmMin;
          ++feedrateCount;
        }
        m_roleTimeByIndex[role] += dt;
        roleTimeAccum[QString::fromUtf8(kRoleLabels[role])] += dt;
        ++extrudeMoveCount;
      }
      else
      {
        featureCount_[kindFeatureLabel(kind)] += 1;
      }
      m_kindCounts[kind] += 1;
      if (kind == KindTravel)
        ++travelMoveCount;

      directMoveLineIndex_.push_back(quint32(mv.gcode_id));
      ++moveIndex;
    }

    prevX = x;
    prevY = y;
    prevZ = z;
  }

  moveCount_ = moveIndex;
  layerCount_ = qMax(1, hasPrintLayerZ ? layer + 1 : 1);
  // Close the open layer (the text path's legacy final append).
  m_layerTimes.append(layerSeconds);
  m_maxLayerTime = qMax(m_maxLayerTime, layerSeconds);

  currentLayerMin_ = 0;
  currentLayerMax_ = layerCount_ - 1;
  currentMove_ = moveCount_;
  // PREV-PLAY: bounds land with the finalized segments (text-parse path).
  rebuildLayerMoveBounds();
  extrudeMoveCount_ = extrudeMoveCount;
  travelMoveCount_ = travelMoveCount;
  toolChangeCount_ = toolChangeCount;

  // Per-segment layer time = the layer's processor total (VT_LayerTime
  // gradient + tooltip). The text path leaves this ~0 for files without
  // ;TIME_ELAPSED tags; the durations here are the authoritative ones.
  // PERF(P1-DIRECT): direct constData indexing -- QVector::value()
  // re-bounds-checks every one of the 620k+ lookups on the 400k tier.
  if (!segments_.empty() && layerCount_ > 0)
  {
    const float *layerTimesData = m_layerTimes.constData();
    const int lastLayer = layerCount_ - 1;
    for (auto &seg : segments_)
      seg.layer_time = layerTimesData[seg.layer < lastLayer ? seg.layer : lastLayer];
  }

  // Average speed: same formula as the text tail (F is mm/min, label mm/s).
  if (feedrateCount > 0)
    avgSpeed_ = QStringLiteral("%1 mm/s").arg(feedrateSum / feedrateCount / 60.0, 0, 'f', 1);
  else
    avgSpeed_ = QStringLiteral("--");

  // Phase 238 (PREV-05) flushed filament: the text path counts unretract
  // moves inside FLUSH_START/FLUSH_END regions; the per-move flushing state
  // does not survive into the moves vector, so take the processor's own
  // per-filament flush accounting (GCodeProcessor update_flush_per_filament)
  // converted from mm3 to filament mm.
  if (!result.print_statistics.flush_per_filament.empty())
  {
    const float area = filamentCrossAreaMm2(filamentDiameter);
    if (area > 0.f)
    {
      for (const auto &flush : result.print_statistics.flush_per_filament)
      {
        const float flushMm = float(flush.second) / area;
        m_filamentSplitLength[2] += flushMm;
        totalFilamentUsed += flushMm;
        m_extruderUsedLength[int(flush.first)] += flushMm;
      }
    }
  }

  applyFilamentTotals(totalFilamentUsed, filamentDiameter, filamentDensity);
  buildRoleTimeBreakdown(roleTimeAccum);

  // Ticks: the processor's custom_gcode_per_print_z carries what the slicer
  // wrote into the file (the text path re-derives the same list from the
  // ;COLOR_CHANGE / ;PAUSE_PRINT / ;CUSTOM_GCODE comment tags).
  for (const Slic3r::CustomGCode::Item &item : result.custom_gcode_per_print_z)
  {
    int layerIdx = -1;
    for (int i = 0; i < m_layerZs.size(); ++i)
    {
      if (isSameZ(m_layerZs[i], float(item.print_z)))
      {
        layerIdx = i;
        break;
      }
    }
    if (layerIdx < 0)
    {
      // print_z between layers (e.g. between the purge line and layer 1):
      // attribute to the first layer at or above it.
      for (int i = 0; i < m_layerZs.size(); ++i)
      {
        if (m_layerZs[i] >= float(item.print_z) - 0.0001f)
        {
          layerIdx = i;
          break;
        }
      }
    }
    if (layerIdx < 0)
      continue;
    OWzx::TickType type = OWzx::TickType::CustomGcode;
    switch (item.type)
    {
      case Slic3r::CustomGCode::ColorChange: type = OWzx::TickType::ColorChange; break;
      case Slic3r::CustomGCode::PausePrint:  type = OWzx::TickType::PausePrint;  break;
      case Slic3r::CustomGCode::Custom:      type = OWzx::TickType::CustomGcode; break;
      case Slic3r::CustomGCode::ToolChange:  type = OWzx::TickType::ToolChange;  break;
      case Slic3r::CustomGCode::Template:    type = OWzx::TickType::Template;    break;
      default: continue;
    }
    OWzx::TickCode tick;
    tick.tick = layerIdx;
    tick.type = type;
    tick.extruder = qMax(0, int(item.extruder) - 1);  // Item extruder is 1-based
    tick.color = QString::fromStdString(item.color);
    tick.extra = QString::fromStdString(item.extra);
    tickMarks_.append(tick);
  }

  updateToolPositionData();
  rebuildGcodeLineWindow();  // empty until the deferred source pass below
  recolorAndPackSegments();  // gcodePreviewData_ ready -> previewReady()

  // Estimated-time label tail (same conditions as the text tail): only when
  // no service label arrived yet.
  if (m_normalTimeSecs > 0.f && estimatedTime_.contains(QStringLiteral("--")))
  {
    estimatedTime_ = formatTime(m_normalTimeSecs);
    totalTime_ = estimatedTime_;
  }
  if (stealthMode_)
  {
    totalTime_ = m_stealthTimeSecs > 0.f
        ? formatTime(m_stealthTimeSecs)
        : formatTime(parseTimeSecs(estimatedTime_) * 1.4f);
  }
  qInfo("[PreviewViewModel] payload direct moves=%zu layers=%d segments=%zu travelVisible=%d",
        result.moves.size(),
        layerCount_,
        segments_.size(),
        showTravelMoves_ ? 1 : 0);

  // Deferred source pass: the G-code window / config footer need the file
  // TEXT, which is pure I/O -- keep it off the preview-ready critical path
  // (upstream equally sources its GCodeWindow from the file, not from the
  // result). The queued lambda no-ops when a reset superseded it.
  if (!gcodePath.isEmpty())
  {
    pendingSourceIndexPath_ = gcodePath;
    QMetaObject::invokeMethod(
        this, [this, path = gcodePath]() {
          if (pendingSourceIndexPath_ != path)
            return;  // superseded by a newer load/reset
          buildGcodeSourceIndex(path);
        },
        Qt::QueuedConnection);
  }

  if (!tickMarks_.isEmpty())
  {
    std::stable_sort(tickMarks_.begin(), tickMarks_.end());
    emit tickMarksChanged();
  }
  return !gcodePreviewData_.isEmpty();
}

// P1: deferred pass -- collect the source-line index (line number, move
// index, text) and the "; key = value" config block from the exported file.
// This is the ONLY part of the direct path that touches the file: it skips
// the parser's per-line state machine entirely (one linear walk), and it
// runs after previewReady() already flipped.
void PreviewViewModel::buildGcodeSourceIndex(const QString &filePath)
{
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return;

  m_gcodeSourceLines.clear();
  m_gcodeSourceLines.reserve(1024);
  m_fullConfig.clear();

  // Cursor over the ascending gcode line ids captured during the direct
  // walk (the move index IS the array position): the move index for file
  // line L is the number of emitted moves whose line id is < L -- exactly
  // the running moveIndex the text parser holds when it reaches line L.
  size_t moveCursor = 0;
  int moveIndexAtLine = 0;
  int sourceLineNumber = 0;
  while (!file.atEnd())
  {
    const QString raw = QString::fromUtf8(file.readLine()).trimmed();
    ++sourceLineNumber;
    while (moveCursor < directMoveLineIndex_.size()
           && directMoveLineIndex_[moveCursor] < quint32(sourceLineNumber))
    {
      moveIndexAtLine = int(moveCursor) + 1;
      ++moveCursor;
    }
    if (raw.isEmpty())
      continue;
    SourceGcodeLine line;
    line.lineNumber = sourceLineNumber;
    line.moveIndex = moveIndexAtLine;
    line.text = raw;
    m_gcodeSourceLines.append(line);
    // Same "; key = value" config rule as the text parser (header + footer
    // blocks; feeds fullConfigValue / PreviewLayerRail).
    if (raw.startsWith(QStringLiteral("; "))
        && raw.contains(QStringLiteral(" = ")))
    {
      const QString body = raw.mid(2);
      const int eq = body.indexOf(QStringLiteral(" = "));
      if (eq > 0)
        m_fullConfig.insert(body.left(eq).trimmed(),
                            body.mid(eq + 3).trimmed());
    }
  }
  directMoveLineIndex_.clear();
  directMoveLineIndex_.shrink_to_fit();
  pendingSourceIndexPath_.clear();
  rebuildGcodeLineWindow();
  emit stateChanged();
}
#endif // HAS_LIBSLIC3R

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
  // Phase 238 (PREV-06): configured filament colors first (upstream plater
  // extruder_colors / m_tool_colors, GCodeViewer.cpp:1109-1127), legacy
  // fixed-cycle fallback handled by effectiveExtruderColor.
  return colorHex(effectiveExtruderColor(extruderId, configuredExtruderColors()));
}

QStringList PreviewViewModel::configuredExtruderColors() const
{
  // The configured per-filament colors live on the project service
  // (PresetService default_filament_colour sync -> plateFilamentColours,
  // the Qt6 equivalent of upstream get_extruder_colors_from_plater_config).
  return projectService_ ? projectService_->plateFilamentColours() : QStringList{};
}

int PreviewViewModel::configuredExtruderCount() const
{
  // Phase 238 (PREV-04): the CONFIGURED extruder count (upstream gates the
  // IMSlider "Change Filament" submenu on m_extruder_colors.size() > 1,
  // IMSlider.cpp:1374). This counts configured filaments, not just the ones
  // used by the current slice.
  return projectService_ ? projectService_->filamentCount() : 1;
}

bool PreviewViewModel::sequentialPrint() const
{
  // Upstream GUI_Preview.cpp:582-583: the preview derives the slider draw
  // mode from the current plate's real print sequence; ByObject becomes
  // IMSlider dmSequentialFffPrint (IMSlider.cpp:320-325).
  if (!projectService_ || !projectService_->plateListConst())
    return false;
  const OWzx::PartPlate *plate = projectService_->plateListConst()->currentPlate();
  return plate
      && plate->printSequence() == static_cast<int>(OWzx::PlatePrintSequence::ByObject);
}

bool PreviewViewModel::canChangeColor() const
{
  // Upstream m_can_change_color truth chain, composed in three steps:
  // 1. GUI_Preview.cpp:462-506 (update_layers_slider_mode): true unless a
  //    multi-filament profile's plate uses a second extruder (support
  //    excluded -- get_extruders_without_support, PartPlate.cpp:1910).
  // 2. IMSlider.cpp:340 (SetModeAndOnlyExtruder): && !spiral vase.
  // 3. IMSlider.cpp:324 (SetDrawMode): && !dmSequentialFffPrint.
  if (sequentialPrint())
    return false;
  if (!projectService_ || !projectService_->plateListConst())
    return true;
  const OWzx::PartPlate *plate = projectService_->plateListConst()->currentPlate();
  if (plate && plate->spiralMode() == static_cast<int>(OWzx::PlateSpiralMode::On))
    return false;
  if (projectService_->filamentCount() <= 1)
    return true;
  // Non-support extruder ids seen by the preview parse (Qt6 equivalent of
  // get_extruders_without_support: canonical libvgcode role indices 11/12/18
  // are the support material / interface / transition roles). Any extruder
  // beyond the first disables the per-layer filament change upstream.
  QSet<int> plateExtruders;
  for (const auto &s : segments_) {
    if (s.role == 11 || s.role == 12 || s.role == 18)
      continue;
    plateExtruders.insert(s.extruder_id);
  }
  return plateExtruders.size() <= 1;
}

QStringList PreviewViewModel::defaultColorChangePalette() const
{
  // Phase 238 (PREV-04): upstream GCodeProcessor Default_Colors
  // (GCodeProcessor.cpp:2305-2312), used when a color change is decoded
  // without an explicit color.
  return {
      QStringLiteral("#0B2C7A"),
      QStringLiteral("#1C8891"),
      QStringLiteral("#AAF200"),
      QStringLiteral("#F5CE0A"),
      QStringLiteral("#D16830"),
      QStringLiteral("#942616"),
  };
}

float PreviewViewModel::layerTimeCumulative(int layer) const
{
  // Upstream IMSlider m_layers_times is CUMULATIVE (IMSlider.cpp:307-308
  // adds the previous entry); m_layerTimes stores per-layer durations, so
  // sum 0..layer here. Out-of-range -> 0.
  if (layer < 0)
    return 0.f;
  double total = 0.0;
  const int last = qMin(layer, m_layerTimes.size() - 1);
  for (int i = 0; i <= last; ++i)
    total += m_layerTimes[i];
  return float(total);
}

QString PreviewViewModel::layerTimeLabel(int layer) const
{
  return formatTime(layerTimeCumulative(layer));
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

void PreviewViewModel::recolorAndPackSegments(bool fullColor)
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
    // Phase 238 (PREV-03): per-move-kind visibility (upstream options_items
    // checkbox filters, GCodeViewer.cpp:4936-4950): Travel / Retract /
    // Unretract / Wipe / Seam each keep their own flag; extrusions always
    // pass. Phase 238 (PREV-06): in the Filament/ColorPrint view the
    // per-extruder m_tool_visibles mask also applies (GCodeViewer.cpp:3337).
    const auto &s = segments_[i];
    bool visible = true;
    switch (s.kind)
    {
      case KindTravel:     visible = showTravelMoves_;      break;
      case KindRetract:    visible = showRetractMoves_;     break;
      case KindUnretract:  visible = showUnretractMoves_;   break;
      case KindWipe:       visible = showWipeMoves_;        break;
      case KindSeam:       visible = showSeamMarks_;        break;
      default:             visible = true;                  break;
    }
    if (visible && mode == VT_Filament)
      visible = isExtruderVisible(s.extruder_id);
    if (visible)
      visibleIndices.push_back(i);
  }

  if (visibleIndices.empty())
    return;

  const int count = int(visibleIndices.size());

  // Determine value range for gradient modes. Uses the PreviewViewModel::EViewType
  // named constants so a renumber cannot silently mislabel a gradient
  // (55-RESEARCH Pitfall 2). Summary / LineType / Filament / Tool / FilamentId
  // do not contribute to the gradient range.
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
    // v5.11: the 4 previously-unavailable modes now have real data.
    case VT_ActualSpeed:      v = s.actual_speed; break;
    case VT_Jerk:             v = s.jerk; break;
    case VT_ActualFlow:       v = s.actual_flow; break;
    case VT_PressureAdvance:  v = s.pressure_advance; break;
    default: continue;  // VT_Summary/VT_LineType/VT_Filament/VT_Tool/
                        // VT_FilamentId do not compute a gradient range.
    }
    if (v < minV) minV = v;
    if (v > maxV) maxV = v;
  }

  // Build packed segments with view-mode colors
  std::vector<PackedSegment> packed;
  packed.resize(count);

  // Phase 238 (PREV-06): Tool/Filament modes color by the CONFIGURED
  // extruder colors (upstream m_tool_colors from the plater
  // extruder_colors config, GCodeViewer.cpp:1109-1127), with the legacy
  // 8-color cycle as fallback (effectiveExtruderColor).
  const QStringList configuredColors = configuredExtruderColors();

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
    p.height = s.height;  // P17.2: carried for the solid-prism preview render
    p.layer_time = s.layer_time;
    p.acceleration = s.acceleration;
    p.jerk = s.jerk;
    p.pressure_advance = s.pressure_advance;
    p.actual_speed = s.actual_speed;
    p.actual_flow = s.actual_flow;
    p.extruder_id = s.extruder_id;
    p.layer = s.layer;
    p.move = s.move;
    p.role = s.role;

    // P17.7: top_layer_only — while the sequential view is scrubbed
    // (moveEnd short of the full toolpath), segments outside the top layer
    // render in the upstream Neutral_Color dark gray
    // (GV.cpp:3285 + :750 Neutral_Color = 0.18 gray). The "current top
    // layer" is the LAYER OF THE SCRUB POSITION (toolLayer_), NOT the
    // layer-rail max handle -- with the rail parked at full range the old
    // proxy grayed every segment except the last layer, so pressing play
    // (which restarts at the range start) turned the whole model gray.
    const int currentTopLayer = hasToolPosition_ ? toolLayer_ : currentLayerMax_;
    const bool topLayerGray = !fullColor
        && m_topLayerOnly && currentMove_ < moveCount_ && s.layer != currentTopLayer;
    // P17.12: dim-lower-layers — upstream darkens the TRUE color of layers
    // strictly below the viewed top layer while top-layer-only is scrubbing
    // (ViewerImpl.cpp:1225-1290: dim > gray precedence, so dimmed layers keep
    // their hue instead of collapsing to Neutral_Color gray; layers above the
    // current one stay gray).
    const bool dimmed =
        !fullColor && m_dimPreviousLayers && topLayerGray && s.layer < currentTopLayer;

    if (mode == VT_LineType)
    {
      // FeatureType: use the baked per-role base color (kRoleColors).
      if (topLayerGray && !dimmed)
      {
        p.r = 0.18f;
        p.g = 0.18f;
        p.b = 0.18f;
      } else {
        p.r = s.baseR;
        p.g = s.baseG;
        p.b = s.baseB;
      }
    }
    else if (mode == VT_Filament || mode == VT_Tool)
    {
      // Filament (ColorPrint) / Tool: per-extruder palette from the
      // CONFIGURED filament colors (Phase 238 PREV-06).
      ColorResult tc = effectiveExtruderColor(s.extruder_id, configuredColors);
      if (topLayerGray && !dimmed)
      {
        tc.r = 0.18f;
        tc.g = 0.18f;
        tc.b = 0.18f;
      }
      p.r = tc.r; p.g = tc.g; p.b = tc.b;
    }
    else if (mode == VT_ActualSpeed || mode == VT_Jerk || mode == VT_ActualFlow || mode == VT_PressureAdvance)
    {
      // v5.11: these modes now have real data (parsed from M220/M221/M205/M900).
      // Map the segment's value to the gradient like the other scalar modes.
      float value = 0.f;
      switch (mode) {
      case VT_ActualSpeed:     value = s.actual_speed; break;
      case VT_Jerk:            value = s.jerk; break;
      case VT_ActualFlow:      value = s.actual_flow; break;
      case VT_PressureAdvance: value = s.pressure_advance; break;
      default: break;
      }
      ColorResult c = valueToGradient(value, minV, maxV);
      if (topLayerGray && !dimmed)
      {
        c.r = 0.18f;
        c.g = 0.18f;
        c.b = 0.18f;
      }
      p.r = c.r; p.g = c.g; p.b = c.b;
    }
    else if (mode == VT_FilamentId)
    {
      // P17.1: upstream EViewType::FilamentId pseudo-color {id, role, id}
      // (GCodeViewer.cpp:3221-3226) — extruder id on R and B, role on G.
      // Hidden diagnostic mode (no legend entry).
      p.r = qBound(0, s.extruder_id * 32, 255) / 255.0f;
      p.g = qBound(0, s.role * 12, 255) / 255.0f;
      p.b = p.r;
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

    if (dimmed)
    {
      // ViewerImpl.cpp:1227 encode_color_darkened — scale toward black by the
      // keep factor (1 - PREVIOUS_LAYER_DARKEN_FACTOR 0.60), hue preserved.
      p.r *= 0.4f;
      p.g *= 0.4f;
      p.b *= 0.4f;
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
  m_legendGradientStops.clear();

  if (mode == VT_Summary)
  {
    // Summary: statistics only -- no gradient legend (upstream EViewType::Summary).
    // m_legendType stays 0 (discrete) and legendItems_ stays empty.
    return;
  }

  if (mode == VT_LineType)
  {
    // P17.4: per-role legend columns — Time / Percent / Used filament —
    // aligned with the upstream FeatureType legend rows
    // (GCodeViewer.cpp:4808-4845, roles_times + filaments per role).
    m_legendRoleColumns.clear();
    auto roleIndexFromLabel = [](const QString &label) {
      for (int r = 0; r < 20; ++r)
        if (QString::fromUtf8(kRoleLabels[r]) == label)
          return r;
      return -1;
    };
    double roleTimeTotal = 0.0;
    for (const auto &entry : m_roleTimes)
      roleTimeTotal += entry.timeSecs;
    for (const auto &entry : m_roleTimes)
    {
      QVariantMap row;
      row.insert(QStringLiteral("label"), entry.name);
      row.insert(QStringLiteral("time"), formatTime(entry.timeSecs));
      row.insert(QStringLiteral("percent"),
                 roleTimeTotal > 0.0
                     ? QString::number(entry.timeSecs / roleTimeTotal * 100.0, 'f', 1)
                     : QStringLiteral("0.0"));
      const double lengthMm = m_roleFilamentLength.value(
          roleIndexFromLabel(entry.name), 0.0);
      row.insert(QStringLiteral("filament"),
                 lengthMm > 0.0 ? QStringLiteral("%1 mm").arg(lengthMm, 0, 'f', 0)
                                : QStringLiteral("--"));
      m_legendRoleColumns.append(row);
    }

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
      // Phase 238 (PREV-03): non-extrusion move kinds (Travel / Retract /
      // Unretract / Wipe / Seam) match their own Options_Colors-based palette
      // (upstream shows them in the FeatureType legend, GCodeViewer.cpp:4948-4957).
      for (int k = 1; k <= 5; ++k)
      {
        if (it.key() == kindFeatureLabel(k))
        {
          color = colorHex({kindBaseColor(k)[0], kindBaseColor(k)[1], kindBaseColor(k)[2]});
          break;
        }
      }
      legendItems_.append(legendItem(it.key(), color, it.value()));
    }
  }
  else if (mode == VT_Filament || mode == VT_Tool)
  {
    // Filament (ColorPrint) / Tool legend: extruder palette from the
    // CONFIGURED filament colors (Phase 238 PREV-06). Rows carry extruderId
    // and visible so the QML legend can host the per-extruder visibility
    // checkbox (upstream m_tool_visibles, GCodeViewer.cpp:5081-5093).
    m_legendType = 2; // extruder palette
    const QStringList configuredColors = configuredExtruderColors();
    QSet<int> usedIds;
    for (const auto &s : segments_)
      usedIds.insert(s.extruder_id);
    QList<int> sortedIds = usedIds.values();
    std::sort(sortedIds.begin(), sortedIds.end());
    // P17.6: filament preset names from the gcode full config
    // (filament_settings_id, upstream legend shows the preset per extruder,
    // GCodeViewer.cpp:5470-5477).
    const QStringList presetNames =
        m_fullConfig.value(QStringLiteral("filament_settings_id"))
            .toString()
            .split(QStringLiteral(", "), Qt::SkipEmptyParts);

    for (int id : sortedIds)
    {
      const QString col = colorHex(effectiveExtruderColor(id, configuredColors));
      QString label = QStringLiteral("Extruder %1").arg(id);
      if (id >= 1 && id <= presetNames.size())
        label += QStringLiteral(" (%1)").arg(presetNames.at(id - 1));
      int cnt = 0;
      for (const auto &s : segments_)
        if (s.extruder_id == id) ++cnt;
      QVariantMap row = legendItem(label, col, cnt);
      row.insert(QStringLiteral("extruderId"), id);
      row.insert(QStringLiteral("visible"), isExtruderVisible(id));
      legendItems_.append(row);
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
    // Internal gradient capabilities (Actual Speed / Acceleration / Jerk /
    // Actual Flow / Pressure Advance) label by mode name; any other
    // unlisted mode (VT_FilamentId diagnostic) falls back to viewModes().
    case VT_ActualSpeed:      label = QStringLiteral("Actual Speed"); break;
    case VT_Jerk:             label = QStringLiteral("Jerk"); break;
    case VT_ActualFlow:       label = QStringLiteral("Actual Flow"); break;
    case VT_PressureAdvance:  label = QStringLiteral("Pressure Advance"); break;
    default: label = viewModes().value(mode); break;
    }

    // P17.3: per-mode decimals — Height/Width/Flow use 2 decimals (upstream
    // GCodeViewer.cpp:4847-4853, :4861), the rest use 0.
    int decimals = 0;
    switch (mode) {
    case VT_Height:
    case VT_Width:
    case VT_Flow:
      decimals = 2;
      break;
    default:
      decimals = 0;
      break;
    }

    const QString minStr = (minV <= FLT_MAX) ? QString::number(minV, 'f', decimals) : QStringLiteral("--");
    const QString maxStr = (maxV >= -FLT_MAX) ? QString::number(maxV, 'f', decimals) : QStringLiteral("--");

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

    // P17.3: the 10-step legend value list (upstream append_range,
    // GCodeViewer.cpp:4498-4518) — one row per Range_Color with the
    // interpolated value at that step, rendered as the gradient bar stops.
    m_legendGradientStops.clear();
    for (int step = 0; step < kRangeColorCount; ++step)
    {
      const float t = float(step) / float(kRangeColorCount - 1);
      const float value = minV + (maxV - minV) * t;
      QVariantMap stop;
      stop.insert(QStringLiteral("position"), t);
      stop.insert(QStringLiteral("color"), colorHex({kRangeColors[step][0],
                                                     kRangeColors[step][1],
                                                     kRangeColors[step][2]}));
      stop.insert(QStringLiteral("value"), QString::number(value, 'f', decimals));
      m_legendGradientStops.append(stop);
    }

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
  // Phase 118 (TICK-02/TICK-03): persist into libslic3r custom-g-code store + re-slice.
  writeTicksToModel();
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
  // Phase 118 (TICK-02/TICK-03): persist into libslic3r custom-g-code store + re-slice.
  writeTicksToModel();
}

void PreviewViewModel::removeTickAtLayer(int layer)
{
  for (int i = 0; i < tickMarks_.size(); ++i) {
    if (tickMarks_[i].tick == layer) {
      tickMarks_.removeAt(i);
      emit tickMarksChanged();
      // Phase 118 (TICK-02/TICK-03): persist removal into libslic3r store + re-slice.
      writeTicksToModel();
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
      // Phase 118 (TICK-02/TICK-03): persist the edited extra into the libslic3r store + re-slice.
      writeTicksToModel();
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
  // Phase 118 (TICK-02/TICK-03): persist into libslic3r custom-g-code store + re-slice.
  writeTicksToModel();
}

void PreviewViewModel::editFilamentChangeAtLayer(int layer, int extruderId)
{
  // Phase 238 (PREV-04): in-place extruder update for an existing ToolChange
  // tick (upstream edit menu, IMSlider.cpp:1414-1424). Follows the
  // editCustomGcodeAtLayer pattern: mutate + emit + writeTicksToModel.
  for (auto& tc : tickMarks_) {
    if (tc.tick == layer && tc.type == OWzx::TickType::ToolChange) {
      if (tc.extruder == extruderId)
        return;
      tc.extruder = extruderId;
      emit tickMarksChanged();
      writeTicksToModel();
      return;
    }
  }
  qWarning("editFilamentChangeAtLayer: no ToolChange tick at layer %d", layer);
}

void PreviewViewModel::addColorChangeAtLayer(int layer, int extruder, const QString& color)
{
  // Phase 119 (TICK-04): the ColorChange type was parsed + written back but had
  // no Q_INVOKABLE add path. This closes the 5-type coverage gap. Dedup + sort
  // + emit + writeTicksToModel mirrors the other add*AtLayer methods.
  for (const auto& tc : tickMarks_) {
    if (tc.tick == layer) {
      qWarning("addColorChangeAtLayer: tick already exists at layer %d", layer);
      return;
    }
  }
  OWzx::TickCode tc;
  tc.tick = layer;
  tc.type = OWzx::TickType::ColorChange;
  tc.extruder = extruder;
  tc.color = color;
  tickMarks_.append(tc);
  std::sort(tickMarks_.begin(), tickMarks_.end());
  emit tickMarksChanged();
  // Phase 118 (TICK-02/TICK-03): persist into libslic3r custom-g-code store + re-slice.
  writeTicksToModel();
}

void PreviewViewModel::addTemplateAtLayer(int layer)
{
  // Phase 119 (TICK-04): Template (upstream "save current state" anchor, type=2)
  // round-trips through convertTicksToCustomGcodeInfo but had no add path. Full
  // template UI is future; this wires the add + round-trip.
  for (const auto& tc : tickMarks_) {
    if (tc.tick == layer) {
      qWarning("addTemplateAtLayer: tick already exists at layer %d", layer);
      return;
    }
  }
  OWzx::TickCode tc;
  tc.tick = layer;
  tc.type = OWzx::TickType::Template;
  tickMarks_.append(tc);
  std::sort(tickMarks_.begin(), tickMarks_.end());
  emit tickMarksChanged();
  // Phase 118 (TICK-02/TICK-03): persist into libslic3r custom-g-code store + re-slice.
  writeTicksToModel();
}

bool PreviewViewModel::moveTick(int fromLayer, int toLayer)
{
  // Phase 119 (TICK-05): drag-to-relocate, aligned with upstream IMSlider tick
  // drag (on_mouse_drag / render_tick_on_mouse_pos). Returns false when the
  // source tick is missing or the target layer is occupied, so the QML delegate
  // can snap back. On success, the moved tick is re-keyed to toLayer, the vector
  // is re-sorted, and Phase 118's writeTicksToModel persists the new ordering +
  // triggers a re-slice so the emitted G-code carries the relocated marker.
  if (fromLayer == toLayer)
    return true;
  int fromIndex = -1;
  for (int i = 0; i < tickMarks_.size(); ++i) {
    if (tickMarks_[i].tick == fromLayer) {
      fromIndex = i;
      break;
    }
  }
  if (fromIndex < 0) {
    qWarning("moveTick: no tick at source layer %d", fromLayer);
    return false;
  }
  for (const auto& tc : tickMarks_) {
    if (tc.tick == toLayer) {
      qWarning("moveTick: target layer %d already occupied", toLayer);
      return false;
    }
  }
  tickMarks_[fromIndex].tick = toLayer;
  std::sort(tickMarks_.begin(), tickMarks_.end());
  emit tickMarksChanged();
  // Phase 118 (TICK-02/TICK-03): persist the relocated tick into the libslic3r store + re-slice.
  writeTicksToModel();
  return true;
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
  // Phase 118 (TICK-02/TICK-03): write the now-empty Info back to the libslic3r
  // store so the next G-code no longer carries the cleared markers, then re-slice.
  writeTicksToModel();
}

void PreviewViewModel::writeTicksToModel()
{
  // Phase 118 (TICK-02/TICK-03): close the loop Phase 117 left open. The in-memory
  // tickMarks_ is converted to a Slic3r::CustomGCode::Info and written to the
  // current plate's entry in model->plates_custom_gcodes (direct field
  // assignment -- BBS deprecated Model::set_custom_gcode_per_print_z, see
  // Model.hpp:1559-1570). curr_plate_index is set so Print::apply reads the
  // correct plate. A re-slice is then triggered so the emitted G-code contains
  // the markers; cloneCurrentPlateModel() copies plates_custom_gcodes
  // (Model.cpp:82-83) so startSlice() sees the written codes.
  //
  // Guard chain (WB-03): skip silently if any dependency is null, if a slice is
  // already in flight (the tick edit is still persisted for the next slice), or
  // when libslic3r is absent (the non-lib fallback just emits tickMarksChanged,
  // preserving Phase 117 in-memory behavior).
#ifdef HAS_LIBSLIC3R
  if (!projectService_ || !sliceService_)
    return;
  if (sliceService_->slicing())
    return;
  Slic3r::Model *model = projectService_->rawModel();
  if (!model)
    return;

  const int plate = projectService_->currentPlateIndex();
  const Slic3r::CustomGCode::Info info = convertTicksToCustomGcodeInfo(tickMarks_, m_layerZs);

  model->curr_plate_index = plate;
  model->plates_custom_gcodes[plate] = info;

  // WB-04: idempotent re-slice. startSlice() clears the old result and re-clones
  // the model (with the written custom codes) before re-applying (SliceService
  // .cpp:325-352). The re-slice runs async; tickMarks_ is repopulated from the
  // new G-code when rebuildFromGCode fires (read-side parse at :993-1021).
  sliceService_->startSlice(projectService_->projectName());
#else
  // Non-lib build: no Model to write to. Preserve Phase 117 in-memory behavior.
  emit tickMarksChanged();
#endif
}
