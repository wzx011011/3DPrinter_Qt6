#pragma once

#include <QObject>
#include <QByteArray>
#include <QVariantList>
#include <QHash>
#include <vector>

class QTimer;

class SliceService;

class PreviewViewModel final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(int progress READ progress NOTIFY stateChanged)
  Q_PROPERTY(bool slicing READ slicing NOTIFY stateChanged)
  Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY stateChanged)
  Q_PROPERTY(QString estimatedTime READ estimatedTime NOTIFY stateChanged)
  Q_PROPERTY(int layerCount READ layerCount NOTIFY stateChanged)
  Q_PROPERTY(int currentLayerMin READ currentLayerMin NOTIFY stateChanged)
  Q_PROPERTY(int currentLayerMax READ currentLayerMax NOTIFY stateChanged)
  Q_PROPERTY(int moveCount READ moveCount NOTIFY stateChanged)
  Q_PROPERTY(int currentMove READ currentMove NOTIFY stateChanged)
  Q_PROPERTY(QString currentTime READ currentTime NOTIFY stateChanged)
  Q_PROPERTY(QByteArray gcodePreviewData READ gcodePreviewData NOTIFY stateChanged)
  Q_PROPERTY(QVariantList legendItems READ legendItems NOTIFY stateChanged)
  /// Legend rendering type (对齐上游 GCodeViewer legend): 0=discrete, 1=gradient, 2=extruder
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
  /// Normal/Stealth 双模式（对齐上游 PrintEstimatedStatistics modes[0]/modes[1]）
  Q_PROPERTY(bool stealthMode READ stealthMode WRITE setStealthMode NOTIFY stateChanged)
  /// 显示/隐藏空驶移动（对齐上游 GCodeViewer travel visibility toggle）
  Q_PROPERTY(bool showTravelMoves READ showTravelMoves WRITE setShowTravelMoves NOTIFY stateChanged)
  /// 显示/隐藏热床网格（对齐上游 GCodeViewer show_bed）
  Q_PROPERTY(bool showBed READ showBed WRITE setShowBed NOTIFY stateChanged)
  /// 显示/隐藏工具位置标记（对齐上游 GCodeViewer show_marker）
  Q_PROPERTY(bool showMarker READ showMarker WRITE setShowMarker NOTIFY stateChanged)
  /// 工具位置提示框数据（对齐上游 GCodeViewer::Marker::render）
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

public:
  explicit PreviewViewModel(SliceService *sliceService, QObject *parent = nullptr);

  int progress() const;
  bool slicing() const;
  bool isPlaying() const;
  QString estimatedTime() const;
  int layerCount() const { return layerCount_; }
  int currentLayerMin() const { return currentLayerMin_; }
  int currentLayerMax() const { return currentLayerMax_; }
  int moveCount() const { return moveCount_; }
  int currentMove() const { return currentMove_; }
  /// Elapsed time at current move position (对齐上游 IMSlider get_label)
  QString currentTime() const;
  /// Elapsed time at an arbitrary move position (对齐上游 IMSlider hover 时间提示)
  Q_INVOKABLE QString timeAtMove(int moveIndex) const;
  const QByteArray &gcodePreviewData() const { return gcodePreviewData_; }
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
  /// 按角色时间分解（对齐上游 PrintEstimatedStatistics::roles_times）
  Q_INVOKABLE int roleTimeCount() const;
  Q_INVOKABLE QString roleTimeName(int i) const;
  Q_INVOKABLE QString roleTimeValue(int i) const;
  Q_INVOKABLE double roleTimePercent(int i) const;
  /// Per-layer time chart data (对齐上游 IMSlider m_layers_times)
  Q_INVOKABLE int layerTimeCount() const;
  Q_INVOKABLE float layerTimeAt(int layer) const;  // seconds
  Q_INVOKABLE float maxLayerTime() const;
  Q_INVOKABLE float minLayerTime() const;
  Q_INVOKABLE float avgLayerTime() const;
  /// Per-layer Z height（对齐上游 IMSlider hover tooltip 显示层高度）
  Q_INVOKABLE float layerZAt(int layer) const;
  /// 工具切换位置（对齐上游 IMSlider colored band / extruder_colors）
  Q_INVOKABLE int toolChangePositionCount() const;
  Q_INVOKABLE int toolChangePositionAt(int i) const;  // move index
  Q_INVOKABLE int toolChangeExtruderIdAt(int i) const; // extruder ID at that point
  /// 挤出机颜色（对齐上游 extruder_colors）
  Q_INVOKABLE QString extruderColor(int extruderId) const;
  /// 每挤出机耗材用量（对齐上游 PrintEstimatedStatistics render_all_plates_stats）
  Q_INVOKABLE int extruderCount() const;
  Q_INVOKABLE double extruderUsedLength(int extruderId) const;  // meters
  Q_INVOKABLE double extruderUsedWeight(int extruderId) const;  // grams
  QStringList viewModes() const;
  int viewModeIndex() const { return viewModeIndex_; }
  bool stealthMode() const { return stealthMode_; }
  void setStealthMode(bool enabled);
  bool showTravelMoves() const { return showTravelMoves_; }
  void setShowTravelMoves(bool enabled);
  bool showBed() const { return showBed_; }
  void setShowBed(bool enabled);
  bool showMarker() const { return showMarker_; }
  void setShowMarker(bool enabled);
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
  /// 跳转到指定层（1-indexed，对齐上游 IMSlider::do_go_to_layer）
  Q_INVOKABLE void jumpToLayer(int oneIndexedLayer);
  /// 整体平移层范围（对齐上游 IMSlider 鼠标滚轮行为）
  Q_INVOKABLE void moveLayerRange(int delta);
  Q_INVOKABLE void setCurrentMove(int move);
  Q_INVOKABLE void playAnimation();
  Q_INVOKABLE void pauseAnimation();
  Q_INVOKABLE void togglePlayPause();
  Q_INVOKABLE void setViewModeIndex(int index);

signals:
  void stateChanged();

private:
  void rebuildFromGCode(const QString &filePath);
  void recolorAndPackSegments();
  void buildLegendItems(int mode, float minV, float maxV);
  QVariantMap legendItem(const QString &label, const QString &color, int count) const;
  void updateToolPositionData();

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

  /// Per-role time breakdown (对齐上游 ExtrusionRole)
  struct RoleTimeEntry
  {
    QString name;     ///< Display name (e.g., "内壁", "外壁", "填充")
    double timeSecs = 0.0;  ///< Total time in seconds
  };
  QList<RoleTimeEntry> m_roleTimes;
  QVector<float> m_layerTimes;  ///< Per-layer elapsed time in seconds (对齐上游 IMSlider m_layers_times)
  QVector<float> m_layerZs;     ///< Per-layer Z height in mm (对齐上游 IMSlider hover tooltip)
  struct ToolChangePos { int moveIndex; int extruderId; };
  QVector<ToolChangePos> m_toolChangePositions; ///< Tool change positions for colored bands
  /// Per-extruder filament usage (对齐上游 PrintEstimatedStatistics volumes_per_extruder)
  QMap<int, double> m_extruderUsedLength;  ///< extruder_id → total extrusion length in mm
  QMap<int, double> m_extruderUsedWeight;  ///< extruder_id → total extrusion weight in g
  float m_maxLayerTime = 0.f;
  int viewModeIndex_ = 0;
  bool stealthMode_ = false;
  bool showTravelMoves_ = true;  ///< 显示空驶移动（对齐上游 GCodeViewer travel toggle）
  bool showBed_ = true;           ///< 显示热床网格（对齐上游 GCodeViewer show_bed）
  bool showMarker_ = true;        ///< 显示工具位置标记（对齐上游 GCodeViewer show_marker）
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
    float layer_time;
    float acceleration;
    int extruder_id;
    int layer;
    int move;
  };
  std::vector<StoredSegment> segments_;
  QHash<QString, int> featureCount_;
  /// Accumulated elapsed time per move (对齐上游 IMSlider m_layers_times)
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
