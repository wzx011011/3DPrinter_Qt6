#pragma once

#include <QByteArray>
#include <QImage>
#include <QList>
#include <QPointF>
#include <QQuickPaintedItem>
#include <QVariant>
#include <QVector>
#include <QVector3D>

class SoftwareViewport : public QQuickPaintedItem
{
  Q_OBJECT
  Q_PROPERTY(int canvasType READ canvasType WRITE setCanvasType NOTIFY canvasTypeChanged)
  Q_PROPERTY(QByteArray meshData READ meshData WRITE setMeshData)
  Q_PROPERTY(QByteArray previewData READ previewData WRITE setPreviewData)
  Q_PROPERTY(int layerMin READ layerMin WRITE setLayerMin)
  Q_PROPERTY(int layerMax READ layerMax WRITE setLayerMax)
  Q_PROPERTY(int moveEnd READ moveEnd WRITE setMoveEnd)
  Q_PROPERTY(bool showTravelMoves READ showTravelMoves WRITE setShowTravelMoves)
  Q_PROPERTY(bool showBed READ showBed WRITE setShowBed)
  Q_PROPERTY(float bedWidth READ bedWidth WRITE setBedWidth)
  Q_PROPERTY(float bedDepth READ bedDepth WRITE setBedDepth)
  Q_PROPERTY(float bedOriginX READ bedOriginX WRITE setBedOriginX)
  Q_PROPERTY(float bedOriginY READ bedOriginY WRITE setBedOriginY)
  Q_PROPERTY(int bedShapeType READ bedShapeType WRITE setBedShapeType)
  Q_PROPERTY(float bedDiameter READ bedDiameter WRITE setBedDiameter)
  Q_PROPERTY(int currentPlateIndex READ currentPlateIndex WRITE setCurrentPlateIndex)
  Q_PROPERTY(int plateCount READ plateCount WRITE setPlateCount)
  Q_PROPERTY(QVariantList activePlateObjectIndices READ activePlateObjectIndices WRITE setActivePlateObjectIndices)
  Q_PROPERTY(bool showWipeTower READ showWipeTower WRITE setShowWipeTower)
  Q_PROPERTY(float wipeTowerWidth READ wipeTowerWidth WRITE setWipeTowerWidth)
  Q_PROPERTY(float wipeTowerDepth READ wipeTowerDepth WRITE setWipeTowerDepth)
  Q_PROPERTY(float wipeTowerHeight READ wipeTowerHeight WRITE setWipeTowerHeight)
  Q_PROPERTY(float wipeTowerX READ wipeTowerX WRITE setWipeTowerX)
  Q_PROPERTY(float wipeTowerZ READ wipeTowerZ WRITE setWipeTowerZ)
  Q_PROPERTY(int gcodeViewMode READ gcodeViewMode WRITE setGcodeViewMode NOTIFY gcodeViewModeChanged)
  Q_PROPERTY(float markerX READ markerX WRITE setMarkerX)
  Q_PROPERTY(float markerY READ markerY WRITE setMarkerY)
  Q_PROPERTY(float markerZ READ markerZ WRITE setMarkerZ)
  Q_PROPERTY(bool showMarker READ showMarker WRITE setShowMarker)
  Q_PROPERTY(int gizmoMode READ gizmoMode WRITE setGizmoMode NOTIFY gizmoModeChanged)
  Q_PROPERTY(bool wireframeMode READ wireframeMode WRITE setWireframeMode NOTIFY wireframeModeChanged)
  Q_PROPERTY(int cutAxis READ cutAxis WRITE setCutAxis)
  Q_PROPERTY(float cutPosition READ cutPosition WRITE setCutPosition)
  Q_PROPERTY(QString lastThumbnailData READ lastThumbnailData NOTIFY thumbnailCaptured)

public:
  enum CanvasType { CanvasView3D = 0, CanvasPreview = 1 };
  Q_ENUM(CanvasType)

  enum GizmoMode {
    GizmoMove = 0,
    GizmoRotate = 1,
    GizmoScale = 2,
    GizmoMeasure = 3,
    GizmoFlatten = 4,
    GizmoCut = 5,
    GizmoSupportPaint = 6,
    GizmoSeamPaint = 7,
    GizmoHollow = 8,
    GizmoSimplify = 9,
    GizmoMmuSegmentation = 10,
    GizmoDrill = 11,
    GizmoEmboss = 12,
    GizmoMeshBoolean = 13,
    GizmoAdvancedCut = 14,
    GizmoFaceDetector = 15,
    GizmoText = 16,
    GizmoSVG = 17,
    GizmoSlaSupports = 18
  };
  Q_ENUM(GizmoMode)

  explicit SoftwareViewport(QQuickItem *parent = nullptr);

  void paint(QPainter *painter) override;

  int canvasType() const { return m_canvasType; }
  void setCanvasType(int value);

  QByteArray meshData() const { return m_meshData; }
  void setMeshData(const QByteArray &data);

  QByteArray previewData() const { return m_previewData; }
  void setPreviewData(const QByteArray &data);

  int layerMin() const { return m_layerMin; }
  void setLayerMin(int value);
  int layerMax() const { return m_layerMax; }
  void setLayerMax(int value);
  int moveEnd() const { return m_moveEnd; }
  void setMoveEnd(int value);
  bool showTravelMoves() const { return m_showTravelMoves; }
  void setShowTravelMoves(bool value);
  bool showBed() const { return m_showBed; }
  void setShowBed(bool value);
  float bedWidth() const { return m_bedWidth; }
  void setBedWidth(float value);
  float bedDepth() const { return m_bedDepth; }
  void setBedDepth(float value);
  float bedOriginX() const { return m_bedOriginX; }
  void setBedOriginX(float value);
  float bedOriginY() const { return m_bedOriginY; }
  void setBedOriginY(float value);
  int bedShapeType() const { return m_bedShapeType; }
  void setBedShapeType(int value);
  float bedDiameter() const { return m_bedDiameter; }
  void setBedDiameter(float value);
  int currentPlateIndex() const { return m_currentPlateIndex; }
  void setCurrentPlateIndex(int value);
  int plateCount() const { return m_plateCount; }
  void setPlateCount(int value);
  QVariantList activePlateObjectIndices() const { return m_activePlateObjectIndices; }
  void setActivePlateObjectIndices(const QVariantList &value);
  bool showWipeTower() const { return m_showWipeTower; }
  void setShowWipeTower(bool value);
  float wipeTowerWidth() const { return m_wipeTowerWidth; }
  void setWipeTowerWidth(float value);
  float wipeTowerDepth() const { return m_wipeTowerDepth; }
  void setWipeTowerDepth(float value);
  float wipeTowerHeight() const { return m_wipeTowerHeight; }
  void setWipeTowerHeight(float value);
  float wipeTowerX() const { return m_wipeTowerX; }
  void setWipeTowerX(float value);
  float wipeTowerZ() const { return m_wipeTowerZ; }
  void setWipeTowerZ(float value);
  int gcodeViewMode() const { return m_gcodeViewMode; }
  void setGcodeViewMode(int value);
  float markerX() const { return m_markerX; }
  void setMarkerX(float value);
  float markerY() const { return m_markerY; }
  void setMarkerY(float value);
  float markerZ() const { return m_markerZ; }
  void setMarkerZ(float value);
  bool showMarker() const { return m_showMarker; }
  void setShowMarker(bool value);

  int gizmoMode() const { return m_gizmoMode; }
  void setGizmoMode(int value);

  bool wireframeMode() const { return m_wireframeMode; }
  void setWireframeMode(bool value);

  int cutAxis() const { return m_cutAxis; }
  void setCutAxis(int value);

  float cutPosition() const { return m_cutPosition; }
  void setCutPosition(float value);

  QString lastThumbnailData() const { return m_lastThumbnailData; }

  Q_INVOKABLE void requestFitView(float cx, float cy, float cz, float r);
  Q_INVOKABLE void requestViewPreset(int preset);
  Q_INVOKABLE void undo() {}
  Q_INVOKABLE void redo() {}
  Q_INVOKABLE void clearHistory() {}
  Q_INVOKABLE void mirrorSelection(int axis) { Q_UNUSED(axis); }
  Q_INVOKABLE void arrangeSelected(float spacing = 0.f, bool rotation = false, bool alignY = false)
  {
    Q_UNUSED(spacing);
    Q_UNUSED(rotation);
    Q_UNUSED(alignY);
  }
  Q_INVOKABLE void requestThumbnailCapture(int plateIndex, int size = 128);

signals:
  void canvasTypeChanged();
  void gizmoModeChanged();
  void wireframeModeChanged();
  void gcodeViewModeChanged();
  void thumbnailCaptured();

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  struct MeshObject
  {
    int objectId = -1;
    QVector<QVector3D> vertices;
    QVector3D min;
    QVector3D max;
  };

  void parseMeshData();
  void fitToMeshes();
  QPointF project(const QVector3D &point) const;
  QVector3D rotatePoint(const QVector3D &point) const;
  QRectF contentRect() const;
  void paintScene(QPainter *painter, const QRectF &target);

  int m_canvasType = CanvasView3D;
  int m_gizmoMode = GizmoMove;
  bool m_wireframeMode = false;
  int m_cutAxis = 2;
  float m_cutPosition = 0.f;
  QByteArray m_meshData;
  QByteArray m_previewData;
  int m_layerMin = 0;
  int m_layerMax = 0;
  int m_moveEnd = 0;
  bool m_showTravelMoves = true;
  bool m_showBed = true;
  float m_bedWidth = 220.f;
  float m_bedDepth = 220.f;
  float m_bedOriginX = 0.f;
  float m_bedOriginY = 0.f;
  int m_bedShapeType = 0;
  float m_bedDiameter = 220.f;
  int m_currentPlateIndex = 0;
  int m_plateCount = 1;
  QVariantList m_activePlateObjectIndices;
  bool m_showWipeTower = true;
  float m_wipeTowerWidth = 0.f;
  float m_wipeTowerDepth = 0.f;
  float m_wipeTowerHeight = 0.f;
  float m_wipeTowerX = 0.f;
  float m_wipeTowerZ = 0.f;
  int m_gcodeViewMode = 0;
  float m_markerX = 0.f;
  float m_markerY = 0.f;
  float m_markerZ = 0.f;
  bool m_showMarker = true;
  QVector<MeshObject> m_meshes;
  QVector3D m_center = QVector3D(110.f, 0.f, 110.f);
  float m_radius = 180.f;
  float m_yaw = -35.f;
  float m_pitch = 55.f;
  float m_zoom = 1.f;
  QPointF m_pan;
  QPointF m_lastMouse;
  Qt::MouseButton m_dragButton = Qt::NoButton;
  QString m_lastThumbnailData;
};
