#include "PreviewViewModel.h"

#include "core/services/SliceService.h"
#include "core/services/ProjectServiceMock.h"
#ifdef HAS_LIBSLIC3R
// Phase 118 (TICK-02/TICK-03): libslic3r custom-g-code write-back path.
#include <libslic3r/CustomGCode.hpp>
#include <libslic3r/Model.hpp>
#endif
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QSettings>
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
    VT_Tool = 16,            // per-extruder palette
    // P17.1: upstream EViewType::FilamentId (GCodeViewer.hpp:711-726, 12th
    // entry). Hidden diagnostic mode — pseudo-color {id, role, id}, no
    // legend (GCodeViewer.cpp:910-911 gates it out of the dropdown).
    VT_FilamentId = 17
  };

  // One-time log guard for the modes whose underlying field is unavailable in
  // the fixture-driven path (Jerk/PA/ActualSpeed/ActualFlow). Logs once per mode
  // so users are informed without spamming on every recolor.
  bool viewModeUsesUnavailableData(int mode)
  {
    // v5.11: all 4 previously-unavailable modes (ActualSpeed/Jerk/ActualFlow/
    // PressureAdvance) now parse real data from M220/M221/M205/M900. None are
    // unavailable anymore.
    Q_UNUSED(mode);
    return false;
  }

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

PreviewViewModel::PreviewViewModel(ProjectServiceMock *projectService, SliceService *sliceService, QObject *parent)
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

bool PreviewViewModel::currentViewModeAvailable() const
{
  return viewModeAvailable(viewModeIndex_);
}

QString PreviewViewModel::currentViewModeStatus() const
{
  return viewModeStatusText(viewModeIndex_);
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
  const float radius = m_filamentDiameter * 0.5f;
  const float area = 3.14159265f * radius * radius;
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
  m_moveAccumulatedTime.clear();
  prepareTimeSeconds_ = 0.f;
  prepareTimeCaptured_ = false;
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
  auto appendMarkerSegment = [&](int markerKind, float mx, float my, float mz) {
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
      appendMarkerSegment(KindRetract, x, y, z);
      continue;
    }
    if (upper == QStringLiteral("G11") || upper.startsWith(QStringLiteral("G11 ")))
    {
      appendMarkerSegment(KindUnretract, x, y, z);
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
                                0.5f * (z + seamFirstZ));
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
      // P17.4: per-role filament length for the FeatureType legend column
      // (upstream legend shows Used filament per role).
      m_roleFilamentLength[roleForTypeImpl(currentType)] += extrusionDelta;
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

    // Phase 238 (PREV-05): category split for the extruded volume, ported
    // from upstream GCodeProcessor.cpp:3002-3010 -- support roles
    // (SupportMaterial 11 / SupportMaterialInterface 12 / SupportTransition
    // 18) -> support, WipeTower (Prime tower, 13) -> tower, everything else
    // -> model. Flushed is accumulated at the unretract-while-flushing branch
    // above (upstream update_flush_per_filament).
    if (extruding)
    {
      if (role == 11 || role == 12 || role == 18)
        m_filamentSplitLength[1] += extrusionDelta;
      else if (role == 13)
        m_filamentSplitLength[3] += extrusionDelta;
      else
        m_filamentSplitLength[0] += extrusionDelta;
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
      if (kind == KindTravel)
      {
        static const float kTravelExtrude[3] = {0.112f, 0.422f, 0.103f};
        static const float kTravelRetract[3] = {0.505f, 0.064f, 0.028f};
        if (extrusionDelta < 0.f)
        {
          seg.baseR = kTravelRetract[0];
          seg.baseG = kTravelRetract[1];
          seg.baseB = kTravelRetract[2];
        }
        else if (extrusionDelta > 0.f)
        {
          seg.baseR = kTravelExtrude[0];
          seg.baseG = kTravelExtrude[1];
          seg.baseB = kTravelExtrude[2];
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
    segments_.push_back(seg);

    // Accumulate elapsed time for the move slider, aligned with upstream IMSlider m_layers_times.
    {
      const float dt = currentFeedrate > 0.f ? dist / currentFeedrate * 60.f : 0.f;
      const float prev = m_moveAccumulatedTime.empty() ? 0.f : m_moveAccumulatedTime.back();
      m_moveAccumulatedTime.push_back(prev + dt);
    }

    if (extruding)
      featureCount_[QString::fromUtf8(kRoleLabels[role])] += 1;
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

    // P17.9 (PARSER): G2/G3 arcs — segmented into straight chords that run
    // through the same per-move processing (upstream segments arcs during
    // parsing so every toolpath vertex is a straight move). E is distributed
    // proportionally to chord length; feedrate carries over. I/J form uses
    // the center offset; R form solves the center on the perpendicular
    // bisector (minor arc for the given direction).
    if (upper.startsWith(QStringLiteral("G2")) || upper.startsWith(QStringLiteral("G3")))
    {
      const bool clockwise = upper.startsWith(QStringLiteral("G2"));
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
    m_totalFilamentGrams = 0.0;
  }
  else
  {
    filamentUsed_ = QStringLiteral("%1 m").arg(totalFilamentUsed / 1000.f, 0, 'f', 2);
    // Weight = volume_mm3 * density_g_per_cm3 * 1e-3.
    // volume_mm3 = length_mm * pi * (diameter / 2)^2.
    const float volume_mm3 = totalFilamentUsed * 3.14159265f * (filamentDiameter * 0.5f) * (filamentDiameter * 0.5f);
    const float weight_g = volume_mm3 * filamentDensity * 0.001f;
    filamentWeight_ = QStringLiteral("%1 g").arg(weight_g, 0, 'f', 1);
    m_totalFilamentGrams = weight_g;
    // Phase 238 (PREV-05): keep the parsed diameter/density so the split
    // formatter can convert lengths to grams with the same formula.
    m_filamentDiameter = filamentDiameter;
    m_filamentDensity = filamentDensity;

    // Compute per-extruder filament weight, aligned with upstream PrintEstimatedStatistics.
    m_extruderUsedWeight.clear();
    for (auto it = m_extruderUsedLength.constBegin(); it != m_extruderUsedLength.constEnd(); ++it)
    {
      const float vol = it.value() * 3.14159265f * (filamentDiameter * 0.5f) * (filamentDiameter * 0.5f);
      m_extruderUsedWeight[it.key()] = vol * filamentDensity * 0.001f;
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
  // (PresetServiceMock default_filament_colour sync -> plateFilamentColours,
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
    // v5.11: the 4 previously-unavailable modes now have real data.
    case VT_ActualSpeed:      v = s.actual_speed; break;
    case VT_Jerk:             v = s.jerk; break;
    case VT_ActualFlow:       v = s.actual_flow; break;
    case VT_PressureAdvance:  v = s.pressure_advance; break;
    default: continue;  // VT_Summary, VT_LineType, VT_Filament, VT_Tool do not
                        // data-unavailable modes (ActualSpeed/Jerk/ActualFlow/PA)
                        // do not compute a gradient range.
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

    if (mode == VT_LineType)
    {
      // FeatureType: use the baked per-role base color (kRoleColors).
      p.r = s.baseR;
      p.g = s.baseG;
      p.b = s.baseB;
    }
    else if (mode == VT_Filament || mode == VT_Tool)
    {
      // Filament (ColorPrint) / Tool: per-extruder palette from the
      // CONFIGURED filament colors (Phase 238 PREV-06).
      const ColorResult tc = effectiveExtruderColor(s.extruder_id, configuredColors);
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
      const ColorResult c = valueToGradient(value, minV, maxV);
      p.r = c.r; p.g = c.g; p.b = c.b;
    }
    else if (mode == VT_Summary)
    {
      // Summary: statistics only; segments still draw in their baked role color.
      p.r = s.baseR; p.g = s.baseG; p.b = s.baseB;
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
    for (int id : sortedIds)
    {
      const QString col = colorHex(effectiveExtruderColor(id, configuredColors));
      const QString label = QStringLiteral("Extruder %1").arg(id);
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
    // Data-unavailable modes render a uniform gradient; label by mode name.
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
