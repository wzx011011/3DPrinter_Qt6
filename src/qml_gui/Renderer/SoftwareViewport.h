#pragma once

#include <QByteArray>
#include <QImage>
#include <QList>
#include <QPointF>
#include <QQuickPaintedItem>
#include <QVariant>
#include <QVector>
#include <QVector3D>
#include <vector>

#include "PrepareSceneData.h"
#include "ViewportContextHit.h"

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
  Q_PROPERTY(QVariantList meshBatchSourceObjectIndices READ meshBatchSourceObjectIndices WRITE setMeshBatchSourceObjectIndices)
  Q_PROPERTY(QVariantList meshBatchVolumeIndices READ meshBatchVolumeIndices WRITE setMeshBatchVolumeIndices)
  Q_PROPERTY(QVariantList meshBatchInstanceIndices READ meshBatchInstanceIndices WRITE setMeshBatchInstanceIndices)
  Q_PROPERTY(bool layerEditingInputActive READ layerEditingInputActive WRITE setLayerEditingInputActive)
  Q_PROPERTY(bool contextToolInputCaptured READ contextToolInputCaptured WRITE setContextToolInputCaptured)
  Q_PROPERTY(int selectedSourceObjectIndex READ selectedSourceObjectIndex WRITE setSelectedSourceObjectIndex)
  Q_PROPERTY(int hoveredSourceObjectIndex READ hoveredSourceObjectIndex WRITE setHoveredSourceObjectIndex)
  // v5.17 (FPS overlay): upstream render-statistics parity (RhiViewport
  // carries the same pair; PreparePage draws one overlay bound to it).
  Q_PROPERTY(int fps READ fps NOTIFY fpsChanged)
  Q_PROPERTY(bool showFps READ showFps WRITE setShowFps NOTIFY showFpsChanged)
  Q_PROPERTY(bool showWipeTower READ showWipeTower WRITE setShowWipeTower)
  Q_PROPERTY(float wipeTowerWidth READ wipeTowerWidth WRITE setWipeTowerWidth)
  Q_PROPERTY(float wipeTowerDepth READ wipeTowerDepth WRITE setWipeTowerDepth)
  Q_PROPERTY(float wipeTowerHeight READ wipeTowerHeight WRITE setWipeTowerHeight)
  Q_PROPERTY(float wipeTowerX READ wipeTowerX WRITE setWipeTowerX)
  Q_PROPERTY(float wipeTowerZ READ wipeTowerZ WRITE setWipeTowerZ)
  // Phase 109 (WTMESH-07): Option B real-mesh Q_PROPERTYs mirror RhiViewport so
  // the software fallback path can render the real mesh when hasRealMesh is
  // true. Same QVariantList-over-QML convention as RhiViewport.
  Q_PROPERTY(bool wipeTowerHasRealMesh READ wipeTowerHasRealMesh WRITE setWipeTowerHasRealMesh)
  Q_PROPERTY(QVariantList wipeTowerMeshVertices READ wipeTowerMeshVertices WRITE setWipeTowerMeshVertices)
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
  // ── P7.1 remote-access parity surface ────────────────────────────────
  // SoftwareViewport is registered AS "GLViewport" in software/remote mode
  // (main_qml.cpp remoteSession branch). The QML pages assign paint/measure/
  // cut properties unconditionally; a missing property fails the WHOLE page
  // load (PreparePage.qml "Cannot assign to non-existent property"). These
  // stash properties keep one QML source working for both backends: values
  // are stored and trigger update(); the specific stream rendering for these
  // markers stays on the RHI backend.
  Q_PROPERTY(QByteArray supportPreviewData READ supportPreviewData WRITE setSupportPreviewData)
  Q_PROPERTY(QByteArray hollowMarkerData READ hollowMarkerData WRITE setHollowMarkerData)
  Q_PROPERTY(QByteArray advancedCutMarkerData READ advancedCutMarkerData WRITE setAdvancedCutMarkerData)
  Q_PROPERTY(QByteArray measureOverlayData READ measureOverlayData WRITE setMeasureOverlayData)
  Q_PROPERTY(QByteArray flattenHoverData READ flattenHoverData WRITE setFlattenHoverData)
  Q_PROPERTY(int paintToolType READ paintToolType WRITE setPaintToolType)
  Q_PROPERTY(int paintState READ paintState WRITE setPaintState)
  Q_PROPERTY(int brushCursorType READ brushCursorType WRITE setBrushCursorType)
  Q_PROPERTY(double brushHeightRange READ brushHeightRange WRITE setBrushHeightRange)
  Q_PROPERTY(double smartFillAngle READ smartFillAngle WRITE setSmartFillAngle)
  Q_PROPERTY(bool paintOnOverhangsOnly READ paintOnOverhangsOnly WRITE setPaintOnOverhangsOnly)
  Q_PROPERTY(double paintOverhangAngle READ paintOverhangAngle WRITE setPaintOverhangAngle)
  Q_PROPERTY(float cutRotationX READ cutRotationX WRITE setCutRotationX)
  Q_PROPERTY(float cutRotationY READ cutRotationY WRITE setCutRotationY)
  Q_PROPERTY(float cutRotationZ READ cutRotationZ WRITE setCutRotationZ)
  Q_PROPERTY(QVariantList plateAnchors READ plateAnchors CONSTANT)
  // Phase 121 (PAINT-02/OV-06): paint overlay reverse-channel (mirrors RhiViewport).
  // The Software mirror parses the same byte stream and appends the painted
  // facets to the depth-sorted `faces` vector in paintScene.
  Q_PROPERTY(QByteArray paintOverlayData READ paintOverlayData WRITE setPaintOverlayData)
  // Phase 121 (PAINT-02/OV-04): MMU per-extruder filament colors (hex strings).
  Q_PROPERTY(QVariantList extrudersColors READ extrudersColors WRITE setExtrudersColors)
  // Phase 121 (PAINT-03/OV-06): brush params (mirror RhiViewport) so the
  // software path can draw a 2D brush cursor circle when a paint gizmo is on.
  Q_PROPERTY(float brushRadius READ brushRadius WRITE setBrushRadius)
  // ── P7.1 remote-access parity properties (mirrored from RhiViewport) ──
  // The software/remote fallback must accept every property the pages assign
  // to the viewport item; QML fails the whole page load on a missing property.
  Q_PROPERTY(bool bedCaliLinesActive READ bedCaliLinesActive WRITE setBedCaliLinesActive)
  Q_PROPERTY(QVariantList bedExcludeAreas READ bedExcludeAreas WRITE setBedExcludeAreas)
  Q_PROPERTY(bool bedHeightLimitActive READ bedHeightLimitActive WRITE setBedHeightLimitActive)
  Q_PROPERTY(float bedHeightToLid READ bedHeightToLid WRITE setBedHeightToLid)
  Q_PROPERTY(float bedHeightToRod READ bedHeightToRod WRITE setBedHeightToRod)
  Q_PROPERTY(QByteArray bedModelMeshData READ bedModelMeshData WRITE setBedModelMeshData)
  Q_PROPERTY(QString bedTypeImagesDir READ bedTypeImagesDir WRITE setBedTypeImagesDir)
  Q_PROPERTY(bool bedTypeTexturesActive READ bedTypeTexturesActive WRITE setBedTypeTexturesActive)
  Q_PROPERTY(int cameraNavStyle READ cameraNavStyle WRITE setCameraNavStyle)
  Q_PROPERTY(int currentPlateBedType READ currentPlateBedType WRITE setCurrentPlateBedType)
  Q_PROPERTY(bool freeCamera READ freeCamera WRITE setFreeCamera)
  Q_PROPERTY(QVariantList meshBatchExtruderIds READ meshBatchExtruderIds WRITE setMeshBatchExtruderIds)
  Q_PROPERTY(QVariantList meshBatchPrintableFlags READ meshBatchPrintableFlags WRITE setMeshBatchPrintableFlags)
  Q_PROPERTY(QVariantList meshBatchVolumeTypes READ meshBatchVolumeTypes WRITE setMeshBatchVolumeTypes)
  Q_PROPERTY(int navigatorBottomOffset READ navigatorBottomOffset WRITE setNavigatorBottomOffset)
  Q_PROPERTY(bool paintHorizontalOnly READ paintHorizontalOnly WRITE setPaintHorizontalOnly)
  Q_PROPERTY(bool paintVerticalOnly READ paintVerticalOnly WRITE setPaintVerticalOnly)
  Q_PROPERTY(bool reverseZoom READ reverseZoom WRITE setReverseZoom)
  Q_PROPERTY(QVariant rotationCenterHint READ rotationCenterHint WRITE setRotationCenterHint)
  Q_PROPERTY(QVariantList selectedSourceObjectIndices READ selectedSourceObjectIndices WRITE setSelectedSourceObjectIndices)
  Q_PROPERTY(bool sequentialClearanceActive READ sequentialClearanceActive WRITE setSequentialClearanceActive)
  Q_PROPERTY(QByteArray sequentialClearanceFill READ sequentialClearanceFill WRITE setSequentialClearanceFill)
  Q_PROPERTY(QByteArray sequentialClearanceOutline READ sequentialClearanceOutline WRITE setSequentialClearanceOutline)
  Q_PROPERTY(bool sequentialClearancePreviewMode READ sequentialClearancePreviewMode WRITE setSequentialClearancePreviewMode)
  Q_PROPERTY(QByteArray sequentialHeightFill READ sequentialHeightFill WRITE setSequentialHeightFill)
  Q_PROPERTY(bool zoomToMouse READ zoomToMouse WRITE setZoomToMouse)
  Q_PROPERTY(QUrl bedTextureUrl READ bedTextureUrl WRITE setBedTextureUrl)
  Q_PROPERTY(QVariantList hintLocalRotation READ hintLocalRotation WRITE setHintLocalRotation)
  Q_PROPERTY(bool navigatorEnabled READ navigatorEnabled WRITE setNavigatorEnabled)
  Q_PROPERTY(bool uniformScale READ uniformScale WRITE setUniformScale)
  Q_PROPERTY(QVariantList roleVisibility READ roleVisibility WRITE setRoleVisibility)
  // AssemblePage surface (upstream GLGizmoAssembly / AssembleView canvas).
  Q_PROPERTY(float explosionRatio READ explosionRatio WRITE setExplosionRatio)
  Q_PROPERTY(int assemblyMeasureSelectedA READ assemblyMeasureSelectedA WRITE setAssemblyMeasureSelectedA)
  Q_PROPERTY(int assemblyMeasureSelectedB READ assemblyMeasureSelectedB WRITE setAssemblyMeasureSelectedB)
  Q_PROPERTY(QVariantList assembleOffsets READ assembleOffsets WRITE setAssembleOffsets)
  Q_PROPERTY(QVariantList assembleRotations READ assembleRotations WRITE setAssembleRotations)
  Q_PROPERTY(QVariantList assembleScales READ assembleScales WRITE setAssembleScales)

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
  QVariantList meshBatchSourceObjectIndices() const { return m_meshBatchSourceObjectIndices; }
  void setMeshBatchSourceObjectIndices(const QVariantList &value);
  QVariantList meshBatchVolumeIndices() const { return m_meshBatchVolumeIndices; }
  void setMeshBatchVolumeIndices(const QVariantList &value);
  QVariantList meshBatchInstanceIndices() const { return m_meshBatchInstanceIndices; }
  void setMeshBatchInstanceIndices(const QVariantList &value);
  bool layerEditingInputActive() const { return m_layerEditingInputActive; }
  void setLayerEditingInputActive(bool value);
  bool contextToolInputCaptured() const { return m_contextToolInputCaptured; }
  void setContextToolInputCaptured(bool value);
  int selectedSourceObjectIndex() const { return m_selectedSourceObjectIndex; }
  void setSelectedSourceObjectIndex(int value);
  int hoveredSourceObjectIndex() const { return m_hoveredSourceObjectIndex; }
  // v5.17 (FPS overlay): paint() updates the counter on the GUI thread.
  int fps() const { return m_fps; }
  bool showFps() const { return m_showFps; }
  void setShowFps(bool on);
  void setHoveredSourceObjectIndex(int value);
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
  // Phase 109 (WTMESH-07): Option B real-mesh getters/setters.
  bool wipeTowerHasRealMesh() const { return m_wipeTowerHasRealMesh; }
  void setWipeTowerHasRealMesh(bool value);
  QVariantList wipeTowerMeshVertices() const;
  void setWipeTowerMeshVertices(const QVariantList &value);
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
  // P7.1 parity stash getters/setters (see the Q_PROPERTY block above).
  QByteArray supportPreviewData() const { return m_supportPreviewData; }
  void setSupportPreviewData(const QByteArray &data);
  QByteArray hollowMarkerData() const { return m_hollowMarkerData; }
  void setHollowMarkerData(const QByteArray &data);
  QByteArray advancedCutMarkerData() const { return m_advancedCutMarkerData; }
  void setAdvancedCutMarkerData(const QByteArray &data);
  QByteArray measureOverlayData() const { return m_measureOverlayData; }
  void setMeasureOverlayData(const QByteArray &data);
  QByteArray flattenHoverData() const { return m_flattenHoverData; }
  void setFlattenHoverData(const QByteArray &data);
  int paintToolType() const { return m_paintToolType; }
  void setPaintToolType(int t);
  int paintState() const { return m_paintState; }
  void setPaintState(int s);
  int brushCursorType() const { return m_brushCursorType; }
  void setBrushCursorType(int t);
  double brushHeightRange() const { return m_brushHeightRange; }
  void setBrushHeightRange(double h);
  double smartFillAngle() const { return m_smartFillAngle; }
  void setSmartFillAngle(double a);
  bool paintOnOverhangsOnly() const { return m_paintOnOverhangsOnly; }
  void setPaintOnOverhangsOnly(bool b);
  double paintOverhangAngle() const { return m_paintOverhangAngle; }
  void setPaintOverhangAngle(double a);
  float cutRotationX() const { return m_cutRotationX; }
  void setCutRotationX(float v);
  float cutRotationY() const { return m_cutRotationY; }
  void setCutRotationY(float v);
  float cutRotationZ() const { return m_cutRotationZ; }
  void setCutRotationZ(float v);
  QVariantList plateAnchors() const { return {}; }
  // P7.1 mirrored-property accessors (stash pattern; see the Q_PROPERTY block).
  bool bedCaliLinesActive() const { return m_bedCaliLinesActive; }
  void setBedCaliLinesActive(bool b);
  QVariantList bedExcludeAreas() const { return m_bedExcludeAreas; }
  void setBedExcludeAreas(const QVariantList &list);
  bool bedHeightLimitActive() const { return m_bedHeightLimitActive; }
  void setBedHeightLimitActive(bool b);
  float bedHeightToLid() const { return m_bedHeightToLid; }
  void setBedHeightToLid(float v);
  float bedHeightToRod() const { return m_bedHeightToRod; }
  void setBedHeightToRod(float v);
  QByteArray bedModelMeshData() const { return m_bedModelMeshData; }
  void setBedModelMeshData(const QByteArray &data);
  QString bedTypeImagesDir() const { return m_bedTypeImagesDir; }
  void setBedTypeImagesDir(const QString &dir);
  bool bedTypeTexturesActive() const { return m_bedTypeTexturesActive; }
  void setBedTypeTexturesActive(bool b);
  int cameraNavStyle() const { return m_cameraNavStyle; }
  void setCameraNavStyle(int s);
  int currentPlateBedType() const { return m_currentPlateBedType; }
  void setCurrentPlateBedType(int t);
  bool freeCamera() const { return m_freeCamera; }
  void setFreeCamera(bool b);
  QVariantList meshBatchExtruderIds() const { return m_meshBatchExtruderIds; }
  void setMeshBatchExtruderIds(const QVariantList &list);
  QVariantList meshBatchPrintableFlags() const { return m_meshBatchPrintableFlags; }
  void setMeshBatchPrintableFlags(const QVariantList &list);
  QVariantList meshBatchVolumeTypes() const { return m_meshBatchVolumeTypes; }
  void setMeshBatchVolumeTypes(const QVariantList &list);
  int navigatorBottomOffset() const { return m_navigatorBottomOffset; }
  void setNavigatorBottomOffset(int v);
  bool paintHorizontalOnly() const { return m_paintHorizontalOnly; }
  void setPaintHorizontalOnly(bool b);
  bool paintVerticalOnly() const { return m_paintVerticalOnly; }
  void setPaintVerticalOnly(bool b);
  bool reverseZoom() const { return m_reverseZoom; }
  void setReverseZoom(bool b);
  QVariant rotationCenterHint() const { return m_rotationCenterHint; }
  void setRotationCenterHint(const QVariant &hint);
  QVariantList selectedSourceObjectIndices() const { return m_selectedSourceObjectIndices; }
  void setSelectedSourceObjectIndices(const QVariantList &list);
  bool sequentialClearanceActive() const { return m_sequentialClearanceActive; }
  void setSequentialClearanceActive(bool b);
  QByteArray sequentialClearanceFill() const { return m_sequentialClearanceFill; }
  void setSequentialClearanceFill(const QByteArray &data);
  QByteArray sequentialClearanceOutline() const { return m_sequentialClearanceOutline; }
  void setSequentialClearanceOutline(const QByteArray &data);
  bool sequentialClearancePreviewMode() const { return m_sequentialClearancePreviewMode; }
  void setSequentialClearancePreviewMode(bool b);
  QByteArray sequentialHeightFill() const { return m_sequentialHeightFill; }
  void setSequentialHeightFill(const QByteArray &data);
  bool zoomToMouse() const { return m_zoomToMouse; }
  void setZoomToMouse(bool b);
  QUrl bedTextureUrl() const { return m_bedTextureUrl; }
  void setBedTextureUrl(const QUrl &url);
  QVariantList hintLocalRotation() const { return m_hintLocalRotation; }
  void setHintLocalRotation(const QVariantList &list);
  bool navigatorEnabled() const { return m_navigatorEnabled; }
  void setNavigatorEnabled(bool b);
  bool uniformScale() const { return m_uniformScale; }
  void setUniformScale(bool b);
  QVariantList roleVisibility() const { return m_roleVisibility; }
  void setRoleVisibility(const QVariantList &list);
  float explosionRatio() const { return m_explosionRatio; }
  void setExplosionRatio(float v);
  int assemblyMeasureSelectedA() const { return m_assemblyMeasureSelectedA; }
  void setAssemblyMeasureSelectedA(int v);
  int assemblyMeasureSelectedB() const { return m_assemblyMeasureSelectedB; }
  void setAssemblyMeasureSelectedB(int v);
  QVariantList assembleOffsets() const { return m_assembleOffsets; }
  void setAssembleOffsets(const QVariantList &list);
  QVariantList assembleRotations() const { return m_assembleRotations; }
  void setAssembleRotations(const QVariantList &list);
  QVariantList assembleScales() const { return m_assembleScales; }
  void setAssembleScales(const QVariantList &list);

  // Phase 121 (PAINT-02/OV-06): paint overlay + brush accessors (mirror RhiViewport).
  QByteArray paintOverlayData() const { return m_paintOverlayData; }
  void setPaintOverlayData(const QByteArray &data) { m_paintOverlayData = data; update(); }
  QVariantList extrudersColors() const { return m_extrudersColors; }
  void setExtrudersColors(const QVariantList &c) { m_extrudersColors = c; update(); }
  float brushRadius() const { return m_brushRadius; }
  void setBrushRadius(float r) { m_brushRadius = r; update(); }

  Q_INVOKABLE void requestFitView(float cx, float cy, float cz, float r);
  Q_INVOKABLE void requestViewPreset(int preset);
  // v5.16 (CIRC-01): history no-ops removed with the RhiViewport ones.
  Q_INVOKABLE void mirrorSelection(int axis) { Q_UNUSED(axis); }
  Q_INVOKABLE void arrangeSelected(float spacing = 0.f, bool rotation = false, bool alignY = false)
  {
    Q_UNUSED(spacing);
    Q_UNUSED(rotation);
    Q_UNUSED(alignY);
  }
  Q_INVOKABLE void requestThumbnailCapture(int plateIndex, int size = 128);
  // P7.1 parity stubs: PreparePage binds these unconditionally. The software
  // viewport projects through its own paint math; screen->bed returns the
  // nearest bed-plane point (Y=0) using the same project() helper.
  Q_INVOKABLE QPointF projectWorldToScreen(float worldX, float worldY, float worldZ) const;
  Q_INVOKABLE QPointF screenToBedPoint(qreal screenX, qreal screenY) const;

signals:
  void fpsChanged();
  void showFpsChanged();
  void canvasTypeChanged();
  void gizmoModeChanged();
  void wireframeModeChanged();
  void gcodeViewModeChanged();
  void thumbnailCaptured();
  void objectPickedSource(int sourceIndex);
  void contextMenuRequested(int targetKind,
                            int sourceObjectIndex,
                            int volumeIndex,
                            int instanceIndex,
                            int plateIndex,
                            qreal popupX,
                            qreal popupY);
  // ── P7.1 remote-access parity signals ────────────────────────────────
  // PreparePage/Plater declare on* handlers for these RhiViewport signals
  // directly on the item; QML hard-fails a component when a signal is
  // missing, so the software fallback must declare the same surface. They
  // are emitted by the RHI backend only — the software viewport keeps them
  // as inert declarations for load compatibility.
  void bedTextureUrlChanged();
  void explosionRatioChanged();
  void cameraProjectionChanged();
  void showSelectedOutlineChanged();
  void navigatorEnabledChanged();
  void navigatorLabelsChanged();
  void plateAnchorsChanged();
  void assemblyMeasureSelectionChanged();
  void sidebarFieldChanged();
  void uniformScaleChanged();
  void hintLocalRotationChanged();
  void roleVisibilityChanged();
  void thumbnailCapturedForPlate(int plateIndex, const QString &data);
  void gizmoMoveRequested(const QVector3D &worldDelta);
  void gizmoRotateRequested(int axis, float radians);
  void gizmoScaleRequested(int axis, float factor);
  void gizmoDragBegin();
  void gizmoDragEnd();
  void measurePickRequested(QVector3D worldOrigin,
                            QVector3D worldDirection,
                            int pickedSourceIndex,
                            bool shiftHeld);
  void hollowPickRequested(QVector3D worldOrigin,
                           QVector3D worldDirection,
                           int pickedSourceIndex);
  void advancedCutPickRequested(QVector3D worldOrigin,
                                QVector3D worldDirection,
                                int pickedSourceIndex);
  void measureHoverLeft();
  void paintPickRequested(QVector3D worldOrigin,
                          QVector3D worldDirection,
                          QVector3D cameraPosition,
                          QVector3D cameraForward,
                          int pickedSourceIndex,
                          double brushRadius,
                          int cursorType,
                          int paintState,
                          int smartFill);
  void smartFillPickRequested(QVector3D worldOrigin,
                              QVector3D worldDirection,
                              QVector3D cameraForward,
                              int pickedSourceIndex,
                              int paintState,
                              double smartFillAngle,
                              bool overhangsOnly,
                              double overhangAngle);
  void heightRangePickRequested(QVector3D worldOrigin,
                                QVector3D worldDirection,
                                QVector3D cameraPosition,
                                QVector3D cameraForward,
                                int pickedSourceIndex,
                                int paintState,
                                double height);
  void flattenHoverRequested(QVector3D worldOrigin,
                             QVector3D worldDirection,
                             int pickedSourceIndex);
  void flattenPickRequested(QVector3D worldOrigin,
                            QVector3D worldDirection,
                            int pickedSourceIndex);
  void cutPlaneDragRequested(float position);
  void cutPlaneRotateRequested(int rotationAxis, float deltaDegrees);

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

  // Phase 238 (PREV-07): one parsed GCV1 preview segment (wire-compatible
  // with PreviewViewModel::PackedSegment / RhiViewportRenderer::
  // GcvPackedSegment -- 21 floats + 4 ints = 96 bytes (P17.2 height); the layout is
  // static_asserted in SoftwareViewport.cpp).
  struct PreviewSegment
  {
    float x1, y1, z1, x2, y2, z2;
    float r, g, b;
    float feedrate, fan_speed, temperature, width, layer_time, acceleration;
    float jerk, pressure_advance, actual_speed, actual_flow;
    int extruder_id, layer, move;
    int role;
    bool visible = true;  // layer/move filter result (recomputed per paint)
  };

  void parseMeshData();
  // Phase 238 (PREV-07): parse the GCV1 blob from setPreviewData.
  void parsePreviewData();
  // Phase 238 (PREV-07): QPainter draw of the layer-clipped preview
  // segments (color per segment, orthographic projection via project()).
  void drawPreviewSegments(QPainter *painter);
  void rebuildPickingScene();
  bool activeToolCapturesContextGesture() const;
  ViewportContextHit classifyContextAt(const QPointF &position);
  QVector3D inverseRotatePoint(const QVector3D &point) const;
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
  // Phase 121 (PAINT-02/OV-06): paint overlay payload (mirror RhiViewport).
  QByteArray m_paintOverlayData;
  QVariantList m_extrudersColors;
  float m_brushRadius = 2.0f;
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
  QVariantList m_meshBatchSourceObjectIndices;
  QVariantList m_meshBatchVolumeIndices;
  QVariantList m_meshBatchInstanceIndices;
  bool m_layerEditingInputActive = false;
  bool m_contextToolInputCaptured = false;
  int m_selectedSourceObjectIndex = -1;
  int m_hoveredSourceObjectIndex = -1;
  // v5.17 (FPS overlay).
  int m_fps = 0;
  bool m_showFps = false;
  // Phase 100 (WTREAD-02): aligned to false so the software path gates on real
  // data like the RHI path (RhiViewport.h:304 default false), not default-true.
  // SoftwareViewport is not instantiated in any page today (only registered as
  // an alternate GLViewport QML type at main_qml.cpp:305); this is defensive so
  // a future instantiation cannot leak a placeholder box before readback
  // arrives. The width/depth/height/x/z defaults stay 0.f (benign while
  // show=false).
  bool m_showWipeTower = false;
  float m_wipeTowerWidth = 0.f;
  float m_wipeTowerDepth = 0.f;
  float m_wipeTowerHeight = 0.f;
  float m_wipeTowerX = 0.f;
  float m_wipeTowerZ = 0.f;
  // Phase 109 (WTMESH-07): Option B real-mesh storage (mirrors RhiViewport).
  // Defaults keep hasRealMesh=false so the software path takes the Option A
  // fallback (Phase 99 Frozen Decision 2 baseline) until a real readback
  // arrives.
  bool m_wipeTowerHasRealMesh = false;
  std::vector<float> m_wipeTowerMeshVertices;
  int m_gcodeViewMode = 0;
  float m_markerX = 0.f;
  float m_markerY = 0.f;
  float m_markerZ = 0.f;
  bool m_showMarker = true;
  QVector<MeshObject> m_meshes;
  // Phase 238 (PREV-07): parsed GCV1 preview segments (CPU staging for the
  // software-fallback preview pass).
  QVector<PreviewSegment> m_previewSegments;
  QVector3D m_center = QVector3D(110.f, 0.f, 110.f);
  float m_radius = 180.f;
  float m_yaw = -35.f;
  float m_pitch = 55.f;
  float m_zoom = 1.f;
  QPointF m_pan;
  QPointF m_lastMouse;
  Qt::MouseButton m_dragButton = Qt::NoButton;
  QPointF m_contextPressPosition;
  bool m_contextPressActive = false;
  bool m_contextDragExceeded = false;
  bool m_contextToolCapturedAtPress = false;
  bool m_contextLayerEditingAtPress = false;
  PrepareSceneData m_pickScene;
  QString m_lastThumbnailData;
  // P7.1 parity stash members.
  QByteArray m_supportPreviewData;
  QByteArray m_hollowMarkerData;
  QByteArray m_advancedCutMarkerData;
  QByteArray m_measureOverlayData;
  QByteArray m_flattenHoverData;
  int m_paintToolType = 0;
  int m_paintState = 1;
  int m_brushCursorType = 1;
  double m_brushHeightRange = 0.2;
  double m_smartFillAngle = 30.0;
  bool m_paintOnOverhangsOnly = false;
  double m_paintOverhangAngle = 0.0;
  float m_cutRotationX = 0.f;
  float m_cutRotationY = 0.f;
  float m_cutRotationZ = 0.f;
  bool m_bedCaliLinesActive = false;
  QVariantList m_bedExcludeAreas;
  bool m_bedHeightLimitActive = false;
  float m_bedHeightToLid = 0.f;
  float m_bedHeightToRod = 0.f;
  QByteArray m_bedModelMeshData;
  QString m_bedTypeImagesDir;
  bool m_bedTypeTexturesActive = false;
  int m_cameraNavStyle = 0;
  int m_currentPlateBedType = 0;
  bool m_freeCamera = false;
  QVariantList m_meshBatchExtruderIds;
  QVariantList m_meshBatchPrintableFlags;
  QVariantList m_meshBatchVolumeTypes;
  int m_navigatorBottomOffset = 0;
  bool m_paintHorizontalOnly = false;
  bool m_paintVerticalOnly = false;
  bool m_reverseZoom = false;
  QVariant m_rotationCenterHint;
  QVariantList m_selectedSourceObjectIndices;
  bool m_sequentialClearanceActive = false;
  QByteArray m_sequentialClearanceFill;
  QByteArray m_sequentialClearanceOutline;
  bool m_sequentialClearancePreviewMode = false;
  QByteArray m_sequentialHeightFill;
  bool m_zoomToMouse = false;
  QUrl m_bedTextureUrl;
  QVariantList m_hintLocalRotation;
  bool m_navigatorEnabled = true;
  bool m_uniformScale = false;
  QVariantList m_roleVisibility;
  float m_explosionRatio = 1.f;
  int m_assemblyMeasureSelectedA = -1;
  int m_assemblyMeasureSelectedB = -1;
  QVariantList m_assembleOffsets;
  QVariantList m_assembleRotations;
  QVariantList m_assembleScales;
};
