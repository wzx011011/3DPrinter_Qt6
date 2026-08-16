#pragma once

#include <QObject>
#include <QByteArray>
#include <QVariantMap>
#include <QVariantList>
#include <QHash>
#include <QColor>
#include <QVector>
#include <array>
#include <vector>
#include "core/rendering/TickCodeTypes.h"

class QTimer;

class SliceService;
class ProjectServiceMock;

class PreviewViewModel final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int progress READ progress NOTIFY stateChanged)
  Q_PROPERTY(bool slicing READ slicing NOTIFY stateChanged)
  Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY stateChanged)
  Q_PROPERTY(QString estimatedTime READ estimatedTime NOTIFY stateChanged)
  Q_PROPERTY(bool previewReady READ previewReady NOTIFY stateChanged)
  Q_PROPERTY(QString previewStatusText READ previewStatusText NOTIFY stateChanged)
  Q_PROPERTY(QString currentLayerLabel READ currentLayerLabel NOTIFY stateChanged)
  Q_PROPERTY(QString currentMoveLabel READ currentMoveLabel NOTIFY stateChanged)
  Q_PROPERTY(QString plateSummary READ plateSummary NOTIFY stateChanged)
  Q_PROPERTY(QString warningSummary READ warningSummary NOTIFY stateChanged)
  Q_PROPERTY(int layerCount READ layerCount NOTIFY stateChanged)
  Q_PROPERTY(int currentLayerMin READ currentLayerMin NOTIFY stateChanged)
  Q_PROPERTY(int currentLayerMax READ currentLayerMax NOTIFY stateChanged)
  Q_PROPERTY(int moveCount READ moveCount NOTIFY stateChanged)
  Q_PROPERTY(int currentMove READ currentMove NOTIFY stateChanged)
  Q_PROPERTY(QString currentTime READ currentTime NOTIFY stateChanged)
  Q_PROPERTY(QByteArray gcodePreviewData READ gcodePreviewData NOTIFY stateChanged)
  Q_PROPERTY(int gcodeLineCount READ gcodeLineCount NOTIFY stateChanged)
  Q_PROPERTY(int currentGcodeLine READ currentGcodeLine NOTIFY stateChanged)
  Q_PROPERTY(QVariantList gcodeLines READ gcodeLines NOTIFY stateChanged)
  Q_PROPERTY(QVariantList legendItems READ legendItems NOTIFY stateChanged)
  /// Legend rendering type aligned with upstream GCodeViewer legend: 0=discrete, 1=gradient, 2=extruder.
  Q_PROPERTY(int legendType READ legendType NOTIFY stateChanged)
  Q_PROPERTY(QString legendGradientMinLabel READ legendGradientMinLabel NOTIFY stateChanged)
  Q_PROPERTY(QString legendGradientMaxLabel READ legendGradientMaxLabel NOTIFY stateChanged)
  Q_PROPERTY(QString legendGradientMinColor READ legendGradientMinColor NOTIFY stateChanged)
  Q_PROPERTY(QString legendGradientMaxColor READ legendGradientMaxColor NOTIFY stateChanged)
  Q_PROPERTY(QString totalTime READ totalTime NOTIFY stateChanged)
  Q_PROPERTY(QString filamentUsed READ filamentUsed NOTIFY stateChanged)
  Q_PROPERTY(QString filamentWeight READ filamentWeight NOTIFY stateChanged)
  Q_PROPERTY(int extrudeMoveCount READ extrudeMoveCount NOTIFY stateChanged)
  Q_PROPERTY(int travelMoveCount READ travelMoveCount NOTIFY stateChanged)
  Q_PROPERTY(int toolChangeCount READ toolChangeCount NOTIFY stateChanged)
  Q_PROPERTY(QString avgSpeed READ avgSpeed NOTIFY stateChanged)
  Q_PROPERTY(QString estimatedCost READ estimatedCost NOTIFY stateChanged)
  Q_PROPERTY(QStringList viewModes READ viewModes CONSTANT)
  Q_PROPERTY(int viewModeIndex READ viewModeIndex WRITE setViewModeIndex NOTIFY stateChanged)
  Q_PROPERTY(bool currentViewModeAvailable READ currentViewModeAvailable NOTIFY stateChanged)
  Q_PROPERTY(QString currentViewModeStatus READ currentViewModeStatus NOTIFY stateChanged)
  /// Normal/Stealth mode aligned with upstream PrintEstimatedStatistics modes.
  Q_PROPERTY(bool stealthMode READ stealthMode WRITE setStealthMode NOTIFY stateChanged)
  /// Travel visibility toggle aligned with upstream GCodeViewer.
  Q_PROPERTY(bool showTravelMoves READ showTravelMoves WRITE setShowTravelMoves NOTIFY stateChanged)
  /// Phase 237 (VIEW-01): G-code window visibility toggle (upstream View
  /// menu "Show G-code Window" / key C, MainFrame.cpp:2623-2629; upstream
  /// persists it as the app_config show_gcode_window flag). Drives the
  /// PreviewPage right-panel G-code source view.
  Q_PROPERTY(bool showGcodeWindow READ showGcodeWindow WRITE setShowGcodeWindow NOTIFY stateChanged)
  /// Bed-grid visibility aligned with upstream GCodeViewer show_bed.
  Q_PROPERTY(bool showBed READ showBed WRITE setShowBed NOTIFY stateChanged)
  /// Tool marker visibility aligned with upstream GCodeViewer show_marker.
  Q_PROPERTY(bool showMarker READ showMarker WRITE setShowMarker NOTIFY stateChanged)
  /// Phase 238 (PREV-03): per-move-type visibility toggles aligned with the
  /// upstream GCodeViewer options_items legend checkboxes (GCodeViewer.cpp:
  /// 913-921: Travel, Retract, Unretract, Wipe, Seam). Wipe defaults to false
  /// matching the existing travel-after-first-view behavior note above
  /// (upstream Travels/Wipes default hidden); retract/unretract/seam default
  /// visible (upstream Retractions/Unretractions/Seams buffers).
  Q_PROPERTY(bool showRetractMoves READ showRetractMoves WRITE setShowRetractMoves NOTIFY stateChanged)
  Q_PROPERTY(bool showUnretractMoves READ showUnretractMoves WRITE setShowUnretractMoves NOTIFY stateChanged)
  Q_PROPERTY(bool showWipeMoves READ showWipeMoves WRITE setShowWipeMoves NOTIFY stateChanged)
  Q_PROPERTY(bool showSeamMarks READ showSeamMarks WRITE setShowSeamMarks NOTIFY stateChanged)
  /// Phase 238 (PREV-05): filament usage split aligned with the upstream
  /// statistics columns Model/Support/Flushed/Tower
  /// (GCodeViewer.cpp:5161-5277 append_headers/columns; volume accounting in
  /// GCodeProcessor.cpp:3002-3010). Rows are {key,label,lengthText,weightText,
  /// lengthM,weightG} in upstream column order.
  Q_PROPERTY(QVariantList filamentSplit READ filamentSplit NOTIFY stateChanged)
  /// Phase 238 (PREV-05): true while the stealth total time is the x1.4
  /// heuristic instead of a parsed "; estimated printing time (silent mode)"
  /// comment (upstream writes normal/silent mode headers, GCodeProcessor.cpp
  /// TimeProcessor "; estimated printing time (normal mode) = ").
  Q_PROPERTY(bool stealthTimeEstimated READ stealthTimeEstimated NOTIFY stateChanged)
  /// Phase 238 (PREV-05): filament price per kg actually used for the cost
  /// estimate. Sourced from the "; filament_cost = " gcode config block
  /// (upstream filament_cost config key, GCodeProcessor.cpp:1252-1260; default
  /// DEFAULT_FILAMENT_COST 29.99, GCodeProcessor.cpp:49). 0 when no price is
  /// known at all.
  Q_PROPERTY(double filamentPricePerKg READ filamentPricePerKg NOTIFY stateChanged)
  /// Phase 238 (PREV-06): per-extruder legend rows {extruderId,label,color,
  /// visible} using the CONFIGURED extruder colors (upstream
  /// m_tools.m_tool_colors from plater extruder_colors config,
  /// GCodeViewer.cpp:1109-1127) instead of a fixed palette. Visibility toggles
  /// mirror upstream m_tool_visibles (GCodeViewer.cpp:5088, ColorPrint view).
  Q_PROPERTY(QVariantList extruderVisibilities READ extruderVisibilities NOTIFY stateChanged)
  /// Tool-position tooltip data aligned with upstream GCodeViewer::Marker::render.
  Q_PROPERTY(bool hasToolPosition READ hasToolPosition NOTIFY stateChanged)
  Q_PROPERTY(double toolX READ toolX NOTIFY stateChanged)
  Q_PROPERTY(double toolY READ toolY NOTIFY stateChanged)
  Q_PROPERTY(double toolZ READ toolZ NOTIFY stateChanged)
  Q_PROPERTY(double toolFeedrate READ toolFeedrate NOTIFY stateChanged)
  Q_PROPERTY(double toolFanSpeed READ toolFanSpeed NOTIFY stateChanged)
  Q_PROPERTY(double toolTemperature READ toolTemperature NOTIFY stateChanged)
  Q_PROPERTY(double toolWidth READ toolWidth NOTIFY stateChanged)
  Q_PROPERTY(double toolLayerTime READ toolLayerTime NOTIFY stateChanged)
  Q_PROPERTY(double toolAcceleration READ toolAcceleration NOTIFY stateChanged)
  Q_PROPERTY(int toolExtruderId READ toolExtruderId NOTIFY stateChanged)
  Q_PROPERTY(int toolLayer READ toolLayer NOTIFY stateChanged)
  Q_PROPERTY(int toolMoveIndex READ toolMoveIndex NOTIFY stateChanged)
  Q_PROPERTY(bool toolIsExtrusion READ toolIsExtrusion NOTIFY stateChanged)
  Q_PROPERTY(QVariantList tickMarks READ tickMarks NOTIFY tickMarksChanged)
  Q_PROPERTY(int tickMarkCount READ tickMarkCount NOTIFY tickMarksChanged)
  /// Per-role extrusion visibility (render-side filter, no repack).
  /// Rows are emitted in ascending canonical libvgcode EGCodeExtrusionRole
  /// index order (1..19 except 0 None and 14 Custom).
  Q_PROPERTY(QVariantList roleVisibilities READ roleVisibilities NOTIFY stateChanged)
  /// Dense 20-bool mask for the renderer (canonical libvgcode index 0..19).
  /// Distinct from roleVisibilities (18 QVariantMap rows for the UI Repeater):
  /// the renderer's synchronize expects a flat 20-element bool list indexed
  /// by canonical role. Binding roleVisibilities (18 maps) here would defeat
  /// the render-side filter (Phase 55 code-review Critical fix).
  Q_PROPERTY(QVariantList roleVisibilityMask READ roleVisibilityMask NOTIFY stateChanged)

public:
  // Phase 118 (TICK-02/TICK-03): projectService_ injected so tick CRUD can
  // write back into libslic3r's model->plates_custom_gcodes (BBS path) and
  // trigger a re-slice. Mirrors EditorViewModel's injection.
  explicit PreviewViewModel(ProjectServiceMock *projectService, SliceService *sliceService, QObject *parent = nullptr);

  int progress() const;
  bool slicing() const;
  bool isPlaying() const;
  QString estimatedTime() const;
  bool previewReady() const;
  QString previewStatusText() const;
  QString currentLayerLabel() const;
  QString currentMoveLabel() const;
  QString plateSummary() const;
  QString warningSummary() const;
  int layerCount() const { return layerCount_; }
  int currentLayerMin() const { return currentLayerMin_; }
  int currentLayerMax() const { return currentLayerMax_; }
  int moveCount() const { return moveCount_; }
  int currentMove() const { return currentMove_; }
  /// Elapsed time at the current move position, aligned with upstream IMSlider get_label.
  QString currentTime() const;
  /// Elapsed time at an arbitrary move position, aligned with upstream IMSlider hover labels.
  Q_INVOKABLE QString timeAtMove(int moveIndex) const;
  const QByteArray &gcodePreviewData() const { return gcodePreviewData_; }
  int gcodeLineCount() const { return gcodeLineCount_; }
  int currentGcodeLine() const { return currentGcodeLine_; }
  QVariantList gcodeLines() const { return gcodeLines_; }
  QVariantList legendItems() const { return legendItems_; }
  int legendType() const { return m_legendType; }
  QString legendGradientMinLabel() const { return m_legendGradMinLabel; }
  QString legendGradientMaxLabel() const { return m_legendGradMaxLabel; }
  QString legendGradientMinColor() const { return m_legendGradMinColor; }
  QString legendGradientMaxColor() const { return m_legendGradMaxColor; }
  QString totalTime() const { return totalTime_; }
  QString filamentUsed() const { return filamentUsed_; }
  QString filamentWeight() const { return filamentWeight_; }
  int extrudeMoveCount() const { return extrudeMoveCount_; }
  int travelMoveCount() const { return travelMoveCount_; }
  int toolChangeCount() const { return toolChangeCount_; }
  QString avgSpeed() const { return avgSpeed_; }
  QString estimatedCost() const { return estimatedCost_; }
  /// Per-role time breakdown aligned with upstream PrintEstimatedStatistics::roles_times.
  Q_INVOKABLE int roleTimeCount() const;
  Q_INVOKABLE QString roleTimeName(int i) const;
  Q_INVOKABLE QString roleTimeValue(int i) const;
  Q_INVOKABLE double roleTimePercent(int i) const;
  /// Per-layer time chart data aligned with upstream IMSlider m_layers_times.
  Q_INVOKABLE int layerTimeCount() const;
  Q_INVOKABLE float layerTimeAt(int layer) const;  // seconds
  /// Phase 238 (PREV-04): cumulative elapsed time at the END of a layer
  /// (upstream IMSlider m_layers_times is cumulative, IMSlider.cpp:307-308;
  /// the tick hover tooltip reads the layer BELOW the tick, IMSlider.cpp:774-781).
  Q_INVOKABLE float layerTimeCumulative(int layer) const;  // seconds
  /// Phase 238 (PREV-04): formatted cumulative layer time for the tick tooltip.
  Q_INVOKABLE QString layerTimeLabel(int layer) const;
  Q_INVOKABLE float maxLayerTime() const;
  Q_INVOKABLE float minLayerTime() const;
  Q_INVOKABLE float avgLayerTime() const;
  /// Per-layer Z height for the upstream IMSlider hover tooltip.
  Q_INVOKABLE float layerZAt(int layer) const;
  /// Tool-change positions aligned with upstream IMSlider colored bands.
  Q_INVOKABLE int toolChangePositionCount() const;
  Q_INVOKABLE int toolChangePositionAt(int i) const;  // move index
  Q_INVOKABLE int toolChangeExtruderIdAt(int i) const; // extruder ID at that point
  /// Extruder colors aligned with upstream extruder_colors. Phase 238
  /// (PREV-06): sourced from the CONFIGURED filament colors
  /// (ProjectServiceMock::plateFilamentColours, i.e. the filament_colour /
  /// default_filament_colour presets -- upstream plater
  /// get_extruder_colors_from_plater_config, GCodeViewer.cpp:1109-1127).
  /// Falls back to the legacy fixed 8-color cycle only when the config has
  /// no colors.
  Q_INVOKABLE QString extruderColor(int extruderId) const;
  /// Phase 238 (PREV-06): configured color list for the Tool/Filament view
  /// modes (may be empty -> caller uses the fixed-cycle fallback).
  QStringList configuredExtruderColors() const;
  /// Phase 238 (PREV-04): CONFIGURED extruder count (gates the rail's
  /// "Change Filament" submenu, upstream m_extruder_colors.size(),
  /// IMSlider.cpp:1374).
  Q_INVOKABLE int configuredExtruderCount() const;
  /// Phase 238 (PREV-04): the default color-change palette shown in the
  /// ColorChange picker (upstream GCodeProcessor Default_Colors,
  /// GCodeProcessor.cpp:2305-2312).
  Q_INVOKABLE QStringList defaultColorChangePalette() const;
  /// Per-extruder filament usage aligned with upstream all-plate statistics.
  Q_INVOKABLE int extruderCount() const;
  Q_INVOKABLE double extruderUsedLength(int extruderId) const;  // meters
  Q_INVOKABLE double extruderUsedWeight(int extruderId) const;  // grams
  Q_INVOKABLE bool loadGCodeForPreview(const QString &filePath);
  QStringList viewModes() const;
  int viewModeIndex() const { return viewModeIndex_; }
  bool currentViewModeAvailable() const;
  QString currentViewModeStatus() const;
  Q_INVOKABLE bool viewModeAvailable(int index) const;
  Q_INVOKABLE QString viewModeStatusText(int index) const;
  bool stealthMode() const { return stealthMode_; }
  Q_INVOKABLE void setStealthMode(bool enabled);
  bool showTravelMoves() const { return showTravelMoves_; }
  Q_INVOKABLE void setShowTravelMoves(bool enabled);
  /// Phase 237 (VIEW-01): G-code window visibility (see the Q_PROPERTY).
  bool showGcodeWindow() const { return showGcodeWindow_; }
  Q_INVOKABLE void setShowGcodeWindow(bool enabled);
  bool showBed() const { return showBed_; }
  Q_INVOKABLE void setShowBed(bool enabled);
  bool showMarker() const { return showMarker_; }
  Q_INVOKABLE void setShowMarker(bool enabled);
  // Phase 238 (PREV-03): move-type visibility (see the Q_PROPERTY block).
  bool showRetractMoves() const { return showRetractMoves_; }
  Q_INVOKABLE void setShowRetractMoves(bool enabled);
  bool showUnretractMoves() const { return showUnretractMoves_; }
  Q_INVOKABLE void setShowUnretractMoves(bool enabled);
  bool showWipeMoves() const { return showWipeMoves_; }
  Q_INVOKABLE void setShowWipeMoves(bool enabled);
  bool showSeamMarks() const { return showSeamMarks_; }
  Q_INVOKABLE void setShowSeamMarks(bool enabled);
  /// Phase 238 (PREV-03): canonical move kinds aligned with the upstream
  /// GCodeProcessor::EMoveType classification relevant to preview
  /// (GCodeProcessor.cpp:2954-2968): 0=Extrude, 1=Travel, 2=Retract,
  /// 3=Unretract, 4=Wipe, 5=Seam.
  enum MoveKind { KindExtrude = 0, KindTravel = 1, KindRetract = 2, KindUnretract = 3, KindWipe = 4, KindSeam = 5 };
  Q_ENUM(MoveKind)
  /// Phase 238 (PREV-03): parsed segment count of a MoveKind (regression
  /// anchor for the retract/unretract/wipe/seam parser paths).
  Q_INVOKABLE int moveCountOfKind(int kind) const;
  /// Phase 238 (PREV-05): filament split rows for the stats panel QML.
  QVariantList filamentSplit() const;
  /// Phase 238 (PREV-05): total extruded grams (cost test anchor).
  Q_INVOKABLE double filamentUsedGrams() const;
  /// Phase 238 (PREV-05): stealth heuristic flag (see the Q_PROPERTY).
  bool stealthTimeEstimated() const;
  /// Phase 238 (PREV-05): filament price per kg in use (see the Q_PROPERTY).
  double filamentPricePerKg() const;
  /// Phase 238 (PREV-06): per-extruder legend rows (see the Q_PROPERTY).
  QVariantList extruderVisibilities() const;
  /// Phase 238 (PREV-06): per-extruder visibility state (upstream
  /// m_tool_visibles, applied in the Filament/ColorPrint view only --
  /// GCodeViewer.cpp:3337).
  Q_INVOKABLE bool isExtruderVisible(int extruderId) const;
  Q_INVOKABLE void toggleExtruderVisibility(int extruderId);
  bool hasToolPosition() const { return hasToolPosition_; }
  double toolX() const { return toolX_; }
  double toolY() const { return toolY_; }
  double toolZ() const { return toolZ_; }
  double toolFeedrate() const { return toolFeedrate_; }
  double toolFanSpeed() const { return toolFanSpeed_; }
  double toolTemperature() const { return toolTemperature_; }
  double toolWidth() const { return toolWidth_; }
  double toolLayerTime() const { return toolLayerTime_; }
  double toolAcceleration() const { return toolAcceleration_; }
  int toolExtruderId() const { return toolExtruderId_; }
  int toolLayer() const { return toolLayer_; }
  int toolMoveIndex() const { return toolMoveIndex_; }
  bool toolIsExtrusion() const { return toolIsExtrusion_; }

  Q_INVOKABLE void setLayerRange(int minLayer, int maxLayer);
  /// Jump to a 1-indexed layer, aligned with upstream IMSlider::do_go_to_layer.
  Q_INVOKABLE void jumpToLayer(int oneIndexedLayer);
  /// Move the full layer range, aligned with upstream IMSlider mouse-wheel behavior.
  Q_INVOKABLE void moveLayerRange(int delta);
  Q_INVOKABLE void setCurrentMove(int move);
  Q_INVOKABLE void stepCurrentMove(int delta);
  Q_INVOKABLE void playAnimation();
  Q_INVOKABLE void pauseAnimation();
  Q_INVOKABLE void togglePlayPause();
  Q_INVOKABLE void setViewModeIndex(int index);

  /// Map an upstream ;TYPE: display string to its canonical libvgcode
  /// EGCodeExtrusionRole index (0..19). Travel/unrecognized -> 0 (None).
  /// Source strings: libslic3r/ExtrusionEntity.cpp:583-608 (role_to_string).
  /// Target indices:  libvgcode/include/Types.hpp:131-157 (EGCodeExtrusionRole).
  /// The two enums DIVERGE past index 6 -- this maps the string DIRECTLY to the
  /// libvgcode index, never via the libslic3r integer (55-RESEARCH Pitfall 6).
  Q_INVOKABLE int roleForType(const QString &type) const;
  /// Return the canonical libvgcode color for a role index (normalized RGB),
  /// sourced from upstream DEFAULT_EXTRUSION_ROLES_COLORS @ ViewerImpl.cpp:283.
  Q_INVOKABLE QColor roleColor(int roleIndex) const;
  /// Per-role extrusion visibility state (canonical libvgcode index 0..19).
  Q_INVOKABLE bool isRoleVisible(int roleIndex) const;
  /// Toggle a role's visibility. Render-side only: emits stateChanged() and does
  /// NOT call recolorAndPackSegments() (Phase 41 interaction-stability invariant).
  Q_INVOKABLE void toggleRoleVisibility(int roleIndex);
  /// QML-facing list of {roleIndex,label,color,visible} rows for the visibility
  /// filter Repeater (ascending canonical index; None(0) and Custom(14) hidden).
  QVariantList roleVisibilities() const;
  /// Dense 20-bool mask (canonical libvgcode index 0..19) for the renderer.
  QVariantList roleVisibilityMask() const;

  /// Tick code management (aligned with upstream TickCode/TickCodeInfo)
  Q_INVOKABLE void addPauseAtLayer(int layer);
  Q_INVOKABLE void addCustomGcodeAtLayer(int layer, const QString& gcode);
  Q_INVOKABLE void removeTickAtLayer(int layer);
  Q_INVOKABLE void editCustomGcodeAtLayer(int layer, const QString& newGcode);
  Q_INVOKABLE void addFilamentChangeAtLayer(int layer, int extruderId);
  /// Phase 238 (PREV-04): edit the extruder of an EXISTING ToolChange tick
  /// in place (upstream edit menu re-picks the filament of a ToolChange tick,
  /// IMSlider.cpp:1414-1424) instead of delete+re-add.
  Q_INVOKABLE void editFilamentChangeAtLayer(int layer, int extruderId);
  // Phase 119 (TICK-04): close the 5-type coverage gap. addColorChangeAtLayer
  // stores type=ColorChange with the user's extruder + color; addTemplateAtLayer
  // stores type=Template (an upstream "save current state" anchor). Both follow
  // the Phase 118 dedup + sort + emit + writeTicksToModel pattern.
  Q_INVOKABLE void addColorChangeAtLayer(int layer, int extruder, const QString& color);
  Q_INVOKABLE void addTemplateAtLayer(int layer);
  // Phase 119 (TICK-05): drag-to-relocate. Moves the tick at fromLayer to
  // toLayer (re-sorts + re-emits + writeTicksToModel). Returns false when the
  // source tick is missing or the target layer is already occupied (caller --
  // PreviewLayerRail.qml -- snaps the delegate back on false). Mirrors upstream
  // IMSlider tick drag (on_mouse_drag / render_tick_on_mouse_pos).
  Q_INVOKABLE bool moveTick(int fromLayer, int toLayer);
  Q_INVOKABLE QVariantMap tickAtLayer(int layer) const;
  Q_INVOKABLE void clearAllTicks();
  QVariantList tickMarks() const;
  int tickMarkCount() const;

signals:
  void stateChanged();
  void tickMarksChanged();

private:
  void resetPreviewState();
  void rebuildFromGCode(const QString &filePath);
  void syncPreviewWithActiveResult();
  void recolorAndPackSegments();
  void buildLegendItems(int mode, float minV, float maxV);
  QVariantMap legendItem(const QString &label, const QString &color, int count) const;
  void updateToolPositionData();
  void rebuildGcodeLineWindow();
  // Phase 118 (TICK-02/TICK-03): write tickMarks_ back into libslic3r's
  // model->plates_custom_gcodes (direct field assignment -- BBS deprecated
  // set_custom_gcode_per_print_z, see Model.hpp:1559-1570) and trigger a
  // re-slice so the resulting G-code contains the user's markers. Guarded by
  // HAS_LIBSLIC3R; the non-lib fallback just emits tickMarksChanged (preserves
  // Phase 117 in-memory behavior when libslic3r is absent).
  void writeTicksToModel();

  ProjectServiceMock *projectService_ = nullptr;
  SliceService *sliceService_ = nullptr;
  QString estimatedTime_ = QStringLiteral("--:--:--");
  int layerCount_ = 0;
  int currentLayerMin_ = 0;
  int currentLayerMax_ = 0;
  int moveCount_ = 0;
  int currentMove_ = 0;
  QByteArray gcodePreviewData_;
  QVariantList legendItems_;
  int m_legendType = 0;  ///< 0=discrete, 1=gradient, 2=extruder
  QString m_legendGradMinLabel;
  QString m_legendGradMaxLabel;
  QString m_legendGradMinColor;
  QString m_legendGradMaxColor;
  QString totalTime_ = QStringLiteral("--:--:--");
  QString filamentUsed_ = QStringLiteral("--");
  QString filamentWeight_ = QStringLiteral("--");
  int extrudeMoveCount_ = 0;
  int travelMoveCount_ = 0;
  int toolChangeCount_ = 0;
  QString avgSpeed_ = QStringLiteral("--");
  QString estimatedCost_ = QStringLiteral("--");
  int gcodeLineCount_ = 0;
  int currentGcodeLine_ = 0;
  QVariantList gcodeLines_;

  /// Per-role time breakdown aligned with upstream ExtrusionRole.
  struct RoleTimeEntry
  {
    QString name;     ///< Display name.
    double timeSecs = 0.0;  ///< Total time in seconds
  };
  QList<RoleTimeEntry> m_roleTimes;
  QVector<float> m_layerTimes;  ///< Per-layer elapsed time in seconds, aligned with IMSlider m_layers_times.
  QVector<float> m_layerZs;     ///< Per-layer Z height in mm for the IMSlider hover tooltip.
  struct ToolChangePos { int moveIndex; int extruderId; };
  QVector<ToolChangePos> m_toolChangePositions; ///< Tool change positions for colored bands
  /// Per-extruder filament usage aligned with upstream PrintEstimatedStatistics volumes_per_extruder.
  QMap<int, double> m_extruderUsedLength;  ///< extruder_id to total extrusion length in mm.
  QMap<int, double> m_extruderUsedWeight;  ///< extruder_id to total extrusion weight in g.
  float m_maxLayerTime = 0.f;
  int viewModeIndex_ = 0;
  QList<OWzx::TickCode> tickMarks_;
  bool stealthMode_ = false;
  // Travel hidden after first view, matching upstream Travels/Wipes=false defaults
  // and CONTEXT.md "travel and wipe hidden after first view" (55-RESEARCH Pitfall 3).
  bool showTravelMoves_ = false;  ///< Travel-move visibility aligned with upstream GCodeViewer.
  bool showGcodeWindow_ = true;   ///< Phase 237 (VIEW-01): G-code window visibility (upstream show_gcode_window).
  bool showBed_ = true;          ///< Bed-grid visibility aligned with upstream GCodeViewer.
  bool showMarker_ = true;       ///< Tool-marker visibility aligned with upstream GCodeViewer.
  // Phase 238 (PREV-03): move-type visibility flags (see the Q_PROPERTY
  // block). Wipe defaults false to match the upstream Travels/Wipes
  // hidden-after-first-view default; retract/unretract/seam default true.
  bool showRetractMoves_ = true;
  bool showUnretractMoves_ = true;
  bool showWipeMoves_ = false;
  bool showSeamMarks_ = true;
  QTimer *playTimer_ = nullptr;

  // Stored parsed segments for view-mode recoloring
  struct StoredSegment
  {
    float x1, y1, z1, x2, y2, z2;
    float baseR, baseG, baseB; // FeatureType colors
    float feedrate;
    float fan_speed;
    float temperature;
    float width;
    float height;
    float layer_time;
    float acceleration;
    float volumetric_rate;
    float jerk = 0.f;             // v5.11: M205 jerk
    float pressure_advance = 0.f; // v5.11: M900 PA
    float actual_speed = 0.f;     // v5.11: feedrate * M220 speed factor
    float actual_flow = 0.f;      // v5.11: M221 flow factor percent
    int extruder_id;
    int layer;
    int move;
    bool isTravel;
    int role = 0;  ///< Canonical libvgcode EGCodeExtrusionRole index (0=None..19=Mixed).
    int kind = 0;  ///< Phase 238 (PREV-03): MoveKind (upstream GCodeProcessor::EMoveType classification).
  };
  std::vector<StoredSegment> segments_;
  /// Per-role extrusion visibility mask, indexed by canonical libvgcode
  /// EGCodeExtrusionRole. All true by default (matches upstream extrusion_roles_visibility).
  std::array<bool, 20> m_roleVisibility{};
  struct SourceGcodeLine
  {
    int lineNumber = 0;
    int moveIndex = -1;
    QString text;
  };
  QVector<SourceGcodeLine> m_gcodeSourceLines;
  QHash<QString, int> featureCount_;
  // ── Phase 238 (PREV-03/05/06) parser state results ──
  int m_kindCounts[6] = {0, 0, 0, 0, 0, 0};  ///< Parsed segment count per MoveKind.
  /// Filament length (mm) split by category, aligned with the upstream
  /// UsedFilaments caches (GCodeProcessor.cpp:783-803): 0=Model, 1=Support,
  /// 2=Flushed, 3=Tower (WipeTower).
  double m_filamentSplitLength[4] = {0.0, 0.0, 0.0, 0.0};
  /// Parsed filament_diameter / filament_density gcode config values (mm / g
  /// per cm3) kept so filamentSplit() can convert lengths to weights.
  float m_filamentDiameter = 1.75f;
  float m_filamentDensity = 1.24f;
  /// Total extruded grams (model+support+tower+flushed), set at parse end.
  double m_totalFilamentGrams = 0.0;
  /// Parsed "; estimated printing time (normal mode) = " seconds (<=0 absent).
  float m_normalTimeSecs = 0.f;
  /// Parsed "; estimated printing time (silent mode) = " seconds (<=0 absent).
  /// Upstream silent == libvgcode Stealth time mode.
  float m_stealthTimeSecs = 0.f;
  /// Per-extruder "; filament_cost = " price per kg (upstream DEFAULT_FILAMENT_COST
  /// 29.99 fallback, GCodeProcessor.cpp:49).
  QMap<int, double> m_filamentPrices;
  /// Per-extruder visibility, upstream m_tool_visibles (ColorPrint view only).
  QMap<int, bool> m_extruderVisibility;
  /// Accumulated elapsed time per move aligned with upstream IMSlider m_layers_times.
  std::vector<float> m_moveAccumulatedTime;
  // Tool position data (updated when currentMove changes)
  bool hasToolPosition_ = false;
  double toolX_ = 0;
  double toolY_ = 0;
  double toolZ_ = 0;
  double toolFeedrate_ = 0;
  double toolFanSpeed_ = 0;
  double toolTemperature_ = 0;
  double toolWidth_ = 0;
  double toolLayerTime_ = 0;
  double toolAcceleration_ = 0;
  int toolExtruderId_ = 0;
  int toolLayer_ = 0;
  int toolMoveIndex_ = 0;
  bool toolIsExtrusion_ = false;
};
