#pragma once
#include <QQuickFramebufferObject>
#include <QMutex>
#include <QList>
#include <QByteArray>

/**
 * GLViewport — QML-exposed OpenGL viewport.
 *
 * Registered in QML as: import CrealityGL 1.0 → GLViewport {}
 *
 * Supports two roles:
 *   canvasType: GLViewport.CanvasView3D   – 3-D prepare scene
 *   canvasType: GLViewport.CanvasPreview  – G-code preview scene (Phase F)
 */
class GLViewport : public QQuickFramebufferObject
{
  Q_OBJECT
  friend class GLViewportRenderer;
  Q_PROPERTY(int canvasType READ canvasType WRITE setCanvasType NOTIFY canvasTypeChanged)
  /// 写入属性: QML 侧绑定 editorVm.meshData，触发网格上传 GPU
  Q_PROPERTY(QByteArray meshData READ meshData WRITE setMeshData)
  Q_PROPERTY(QByteArray previewData READ previewData WRITE setPreviewData)
  Q_PROPERTY(int layerMin READ layerMin WRITE setLayerMin)
  Q_PROPERTY(int layerMax READ layerMax WRITE setLayerMax)
  Q_PROPERTY(int moveEnd READ moveEnd WRITE setMoveEnd)
  /// 对齐上游 GCodeViewer m_travel_visibility
  Q_PROPERTY(bool showTravelMoves READ showTravelMoves WRITE setShowTravelMoves)
  /// 对齐上游 GCodeViewer show_bed
  Q_PROPERTY(bool showBed READ showBed WRITE setShowBed)
  /// Wipe tower visualization (P2.8.3)
  Q_PROPERTY(bool showWipeTower READ showWipeTower WRITE setShowWipeTower)
  Q_PROPERTY(float wipeTowerWidth READ wipeTowerWidth WRITE setWipeTowerWidth)
  Q_PROPERTY(float wipeTowerDepth READ wipeTowerDepth WRITE setWipeTowerDepth)
  Q_PROPERTY(float wipeTowerHeight READ wipeTowerHeight WRITE setWipeTowerHeight)
  Q_PROPERTY(float wipeTowerX READ wipeTowerX WRITE setWipeTowerX)
  Q_PROPERTY(float wipeTowerZ READ wipeTowerZ WRITE setWipeTowerZ)
  /// 对齐上游 GCodeViewer::Marker
  Q_PROPERTY(float markerX READ markerX WRITE setMarkerX)
  Q_PROPERTY(float markerY READ markerY WRITE setMarkerY)
  Q_PROPERTY(float markerZ READ markerZ WRITE setMarkerZ)
  Q_PROPERTY(bool showMarker READ showMarker WRITE setShowMarker)
  Q_PROPERTY(int gizmoMode READ gizmoMode WRITE setGizmoMode NOTIFY gizmoModeChanged)
  Q_PROPERTY(bool wireframeMode READ wireframeMode WRITE setWireframeMode NOTIFY wireframeModeChanged)
  /// 最近一次 FBO 缩略图捕获结果（base64 PNG，供 QML Image 使用）
  Q_PROPERTY(QString lastThumbnailData READ lastThumbnailData NOTIFY thumbnailCaptured)

  // GizmoMode constants exposed to QML
  enum GizmoMode {
    GizmoMove = 0,
    GizmoRotate = 1,
    GizmoScale = 2,
    GizmoMeasure = 3,
    GizmoFlatten = 4,   ///< 对齐上游 GLGizmoFlatten — 选择面平放
    GizmoCut = 5,       ///< 对齐上游 GLGizmoCut — 切割对象
    GizmoSupportPaint = 6,  ///< 对齐上游 GLGizmoFdmSupports — 支撑绘制
    GizmoSeamPaint = 7,    ///< 对齐上游 GLGizmoSeam — 缝线绘制
    GizmoHollow = 8,       ///< 对齐上游 GLGizmoHollow — SLA 空洞标记
    GizmoSimplify = 9,     ///< 对齐上游 GLGizmoSimplify — 网格简化
    GizmoMmuSegmentation = 10,  ///< 对齐上游 GLGizmoMmuSegmentation — MMU 多耗材分段
    GizmoDrill = 11,            ///< 对齐上游 GLGizmoDrill — 钻孔
    GizmoEmboss = 12,           ///< 对齐上游 GLGizmoEmboss — 浮雕文字
    GizmoMeshBoolean = 13,      ///< 对齐上游 GLGizmoMeshBoolean — 布尔运算
    GizmoAdvancedCut = 14,      ///< 对齐上游 GLGizmoAdvancedCut — 高级切割
    GizmoFaceDetector = 15,     ///< 对齐上游 GLGizmoFaceDetector — 平面检测
    GizmoText = 16,             ///< 对齐上游 GLGizmoText — 文字
    GizmoSVG = 17,              ///< 对齐上游 GLGizmoSVG — SVG 导入
    GizmoSlaSupports = 18       ///< 对齐上游 GLGizmoSlaSupports — SLA 支撑
  };
  Q_ENUM(GizmoMode)

public:
  enum CanvasType
  {
    CanvasView3D = 0,
    CanvasPreview = 1
  };
  Q_ENUM(CanvasType)

  explicit GLViewport(QQuickItem *parent = nullptr);

  // QQuickFramebufferObject interface
  Renderer *createRenderer() const override;

  // Property accessors
  int canvasType() const { return m_canvasType; }
  void setCanvasType(int t);

  int gizmoMode() const { return m_gizmoMode; }
  void setGizmoMode(int mode);

  bool wireframeMode() const { return m_wireframeMode; }
  void setWireframeMode(bool on);

  // --- 网格数据 (GUI 线程写，渲染线程通过 takeMesh 读) ---
  QByteArray meshData() const
  {
    QMutexLocker lk(&m_eventMutex);
    return m_meshData;
  }
  void setMeshData(const QByteArray &data);
  /** 渲染线程调用: 取走待上传的数据。若版本相同则返回 false。 */
  bool takeMesh(QByteArray &out, int &version);

  QByteArray previewData() const
  {
    QMutexLocker lk(&m_eventMutex);
    return m_previewData;
  }
  void setPreviewData(const QByteArray &data);
  bool takePreviewData(QByteArray &out, int &version);

  int layerMin() const { return m_layerMin; }
  int layerMax() const { return m_layerMax; }
  int moveEnd() const { return m_moveEnd; }
  bool showTravelMoves() const { return m_showTravelMoves; }
  bool showBed() const { return m_showBed; }
  bool showWipeTower() const { return m_showWipeTower; }
  float wipeTowerWidth() const { return m_wipeTowerWidth; }
  float wipeTowerDepth() const { return m_wipeTowerDepth; }
  float wipeTowerHeight() const { return m_wipeTowerHeight; }
  float wipeTowerX() const { return m_wipeTowerX; }
  float wipeTowerZ() const { return m_wipeTowerZ; }
  float markerX() const { return m_markerX; }
  float markerY() const { return m_markerY; }
  float markerZ() const { return m_markerZ; }
  bool showMarker() const { return m_showMarker; }
  void setLayerMin(int v);
  void setLayerMax(int v);
  void setMoveEnd(int v);
  void setShowTravelMoves(bool on);
  void setShowBed(bool on);
  void setShowWipeTower(bool on);
  void setWipeTowerWidth(float v);
  void setWipeTowerDepth(float v);
  void setWipeTowerHeight(float v);
  void setWipeTowerX(float v);
  void setWipeTowerZ(float v);
  void setMarkerX(float v) { m_markerX = v; }
  void setMarkerY(float v) { m_markerY = v; }
  void setMarkerZ(float v) { m_markerZ = v; }
  void setShowMarker(bool v) { m_showMarker = v; }

  // --- Input event queue (consumed by GLViewportRenderer::synchronize) ---
  struct InputEvent
  {
    enum Type
    {
      Press,
      Move,
      Release,
      Wheel,
      FitView,     ///< 相机自适应 bbox
      ViewPreset, ///< 相机预设视角 (x=0:top 1:front 2:right 3:iso)
      Undo,
      Redo,
      ClearHistory,
      SetGizmoMode,
      Mirror,  ///< Mirror selected object along axis (mirrorAxis: 0=X 1=Y 2=Z)
      ArrangeSelected, ///< 自动排列选中对象
      CaptureThumbnail ///< FBO 缩略图捕获（对齐上游 PartPlate::thumbnail_data）
    } type;
    Qt::MouseButton button = Qt::NoButton;
    Qt::MouseButtons buttons = Qt::NoButton;
    float x = 0.f;
    float y = 0.f;
    float wheelDelta = 0.f;
    // FitView payload
    float fitCX = 0.f, fitCY = 0.f, fitCZ = 0.f, fitRadius = 100.f;
    // Mirror payload
    int mirrorAxis = 0;
    // ArrangeSelected payload (对齐上游 ArrangeSettings)
    float arrangeSpacing = 6.0f;      ///< 0 = auto spacing
    bool  arrangeRotation = false;    ///< 自动旋转
    bool  arrangeAlignY = false;      ///< 对齐 Y 轴
    // CaptureThumbnail payload (对齐上游 PartPlate::thumbnail_data)
    int capturePlateIndex = 0;        ///< 目标平板索引
    int captureSize = 128;            ///< 缩略图尺寸（宽高相同）
  };

  /** Atomically swap out & return all pending events. */
  QList<InputEvent> takeEvents();

  /** QML 调用: 令相机自适应包含中心 (cx, cy, cz)、半径 r 的球体 */
  Q_INVOKABLE void requestFitView(float cx, float cy, float cz, float r);
  /** QML 调用: 切换相机预设视角 (preset: 0=top, 1=front, 2=right, 3=iso) */
  Q_INVOKABLE void requestViewPreset(int preset);
  Q_INVOKABLE void undo();
  Q_INVOKABLE void redo();
  Q_INVOKABLE void clearHistory();
  /** QML 调用: 镜像选中对象 (axis: 0=X 1=Y 2=Z) */
  Q_INVOKABLE void mirrorSelection(int axis);
  /** QML 调用: 自动排列选中对象（对齐上游 Plater::priv::on_arrange） */
  Q_INVOKABLE void arrangeSelected(float spacing = 0.f, bool rotation = false, bool alignY = false);
  /** QML 调用: 请求 FBO 缩略图捕获（对齐上游 PartPlate::thumbnail_data） */
  Q_INVOKABLE void requestThumbnailCapture(int plateIndex, int size = 128);

  /// 返回最近一次 FBO 缩略图捕获结果
  QString lastThumbnailData() const { return m_lastThumbnailData; }

signals:
  void canvasTypeChanged();
  void gizmoModeChanged();
  void wireframeModeChanged();
  /// FBO 缩略图捕获完成，携带 base64 PNG 数据
  void thumbnailCaptured();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  int m_canvasType = CanvasView3D;
  int m_gizmoMode = GizmoMove;
  bool m_wireframeMode = false;
  mutable QMutex m_eventMutex;
  QList<InputEvent> m_events;
  // 网格数据双缓冲
  QByteArray m_meshData;
  int m_meshVersion = 0;
  QByteArray m_previewData;
  int m_previewVersion = 0;
  int m_layerMin = 0;
  int m_layerMax = 0;
  int m_moveEnd = 0;
  bool m_showTravelMoves = true;  ///< 对齐上游 GCodeViewer m_travel_visibility
  bool m_showBed = true;          ///< 对齐上游 GCodeViewer show_bed
  bool m_showWipeTower = false;   ///< Wipe tower visibility (P2.8.3)
  float m_wipeTowerWidth = 10.f;  ///< Wipe tower width in mm (default 10)
  float m_wipeTowerDepth = 10.f;  ///< Wipe tower depth in mm (default 10)
  float m_wipeTowerHeight = 50.f; ///< Wipe tower height in mm (default 50)
  float m_wipeTowerX = 100.f;     ///< Wipe tower X position on bed
  float m_wipeTowerZ = 25.f;      ///< Wipe tower Z position on bed
  float m_markerX = 0.f;
  float m_markerY = 0.f;
  float m_markerZ = 0.f;
  bool m_showMarker = true;       ///< 对齐上游 GCodeViewer m_show_marker
  QString m_lastThumbnailData; ///< 最近一次 FBO 缩略图（base64 PNG）
};
