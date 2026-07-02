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
  /// Normal/Stealth mode aligned with upstream PrintEstimatedStatistics modes.
  Q_PROPERTY(bool stealthMode READ stealthMode WRITE setStealthMode NOTIFY stateChanged)
  /// Travel visibility toggle aligned with upstream GCodeViewer.
  Q_PROPERTY(bool showTravelMoves READ showTravelMoves WRITE setShowTravelMoves NOTIFY stateChanged)
  /// Bed-grid visibility aligned with upstream GCodeViewer show_bed.
  Q_PROPERTY(bool showBed READ showBed WRITE setShowBed NOTIFY stateChanged)
  /// Tool marker visibility aligned with upstream GCodeViewer show_marker.
  Q_PROPERTY(bool showMarker READ showMarker WRITE setShowMarker NOTIFY stateChanged)
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

public:
  explicit PreviewViewModel(SliceService *sliceService, QObject *parent = nullptr);

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
  Q_INVOKABLE float maxLayerTime() const;
  Q_INVOKABLE float minLayerTime() const;
  Q_INVOKABLE float avgLayerTime() const;
  /// Per-layer Z height for the upstream IMSlider hover tooltip.
  Q_INVOKABLE float layerZAt(int layer) const;
  /// Tool-change positions aligned with upstream IMSlider colored bands.
  Q_INVOKABLE int toolChangePositionCount() const;
  Q_INVOKABLE int toolChangePositionAt(int i) const;  // move index
  Q_INVOKABLE int toolChangeExtruderIdAt(int i) const; // extruder ID at that point
  /// Extruder colors aligned with upstream extruder_colors.
  Q_INVOKABLE QString extruderColor(int extruderId) const;
  /// Per-extruder filament usage aligned with upstream all-plate statistics.
  Q_INVOKABLE int extruderCount() const;
  Q_INVOKABLE double extruderUsedLength(int extruderId) const;  // meters
  Q_INVOKABLE double extruderUsedWeight(int extruderId) const;  // grams
  Q_INVOKABLE bool loadGCodeForPreview(const QString &filePath);
  QStringList viewModes() const;
  int viewModeIndex() const { return viewModeIndex_; }
  bool stealthMode() const { return stealthMode_; }
  Q_INVOKABLE void setStealthMode(bool enabled);
  bool showTravelMoves() const { return showTravelMoves_; }
  Q_INVOKABLE void setShowTravelMoves(bool enabled);
  bool showBed() const { return showBed_; }
  Q_INVOKABLE void setShowBed(bool enabled);
  bool showMarker() const { return showMarker_; }
  Q_INVOKABLE void setShowMarker(bool enabled);
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

  /// Tick code management (aligned with upstream TickCode/TickCodeInfo)
  Q_INVOKABLE void addPauseAtLayer(int layer);
  Q_INVOKABLE void addCustomGcodeAtLayer(int layer, const QString& gcode);
  Q_INVOKABLE void removeTickAtLayer(int layer);
  Q_INVOKABLE void editCustomGcodeAtLayer(int layer, const QString& newGcode);
  Q_INVOKABLE void addFilamentChangeAtLayer(int layer, int extruderId);
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
  bool showBed_ = true;          ///< Bed-grid visibility aligned with upstream GCodeViewer.
  bool showMarker_ = true;       ///< Tool-marker visibility aligned with upstream GCodeViewer.
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
    int extruder_id;
    int layer;
    int move;
    bool isTravel;
    int role = 0;  ///< Canonical libvgcode EGCodeExtrusionRole index (0=None..19=Mixed).
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
