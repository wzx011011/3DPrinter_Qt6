#pragma once

#include <QByteArray>
#include <QImage>
#include <QList>
#include <QPointF>
#include <QQuickPaintedItem>
#include <QRectF>
#include <QUrl>
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
  // SEL-MODIFIERS-RECT: inert rubber-band mirrors of RhiViewport. The
  // software fallback performs no object picking (never emits
  // objectPickedSource*), so the band stays inactive; the properties exist so
  // the shared PreparePage.qml bindings resolve on both viewport paths.
  Q_PROPERTY(QRectF selectionRubberBand READ selectionRubberBand NOTIFY selectionRubberBandChanged)
  Q_PROPERTY(bool selectionRubberBandActive READ selectionRubberBandActive NOTIFY selectionRubberBandChanged)
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
  // Phase 121 (PAINT-02/OV-06): paint overlay reverse-channel (mirrors RhiViewport).
  // The Software mirror parses the same byte stream and appends the painted
  // facets to the depth-sorted `faces` vector in paintScene.
  Q_PROPERTY(QByteArray paintOverlayData READ paintOverlayData WRITE setPaintOverlayData)
  // Phase 121 (PAINT-02/OV-04): MMU per-extruder filament colors (hex strings).
  Q_PROPERTY(QVariantList extrudersColors READ extrudersColors WRITE setExtrudersColors)
  // Phase 121 (PAINT-03/OV-06): brush params (mirror RhiViewport) so the
  // software path can draw a 2D brush cursor circle when a paint gizmo is on.
  Q_PROPERTY(float brushRadius READ brushRadius WRITE setBrushRadius)
  // G3 debt ② (fixed 2026-10-19): inert contract mirrors of RhiViewport.
  // OWZX_RHI_RENDERER=software swaps the GLViewport registration to this
  // class, and every PreparePage binding below must resolve here or the whole
  // page aborts with "Cannot assign to non-existent property" (startup
  // exit(-1)). The values are stored but never rendered — the fallback paints
  // only the bed/preview pass. The mirror set is locked by
  // QmlSmokeTests::softwareViewportMirrorsPreparePageBindings.
  Q_PROPERTY(QByteArray advancedCutMarkerData READ advancedCutMarkerData WRITE setAdvancedCutMarkerData)
  Q_PROPERTY(bool bedCaliLinesActive READ bedCaliLinesActive WRITE setBedCaliLinesActive)
  Q_PROPERTY(QVariantList bedExcludeAreas READ bedExcludeAreas WRITE setBedExcludeAreas)
  Q_PROPERTY(bool bedHeightLimitActive READ bedHeightLimitActive WRITE setBedHeightLimitActive)
  Q_PROPERTY(float bedHeightToLid READ bedHeightToLid WRITE setBedHeightToLid)
  Q_PROPERTY(float bedHeightToRod READ bedHeightToRod WRITE setBedHeightToRod)
  Q_PROPERTY(QByteArray bedModelMeshData READ bedModelMeshData WRITE setBedModelMeshData)
  Q_PROPERTY(float bedPrintableHeight READ bedPrintableHeight WRITE setBedPrintableHeight)
  Q_PROPERTY(QUrl bedTextureUrl READ bedTextureUrl WRITE setBedTextureUrl)
  Q_PROPERTY(QString bedTypeImagesDir READ bedTypeImagesDir WRITE setBedTypeImagesDir)
  Q_PROPERTY(bool bedTypeTexturesActive READ bedTypeTexturesActive WRITE setBedTypeTexturesActive)
  Q_PROPERTY(int brushCursorType READ brushCursorType WRITE setBrushCursorType)
  Q_PROPERTY(double brushHeightRange READ brushHeightRange WRITE setBrushHeightRange)
  Q_PROPERTY(int cameraNavStyle READ cameraNavStyle WRITE setCameraNavStyle)
  Q_PROPERTY(int currentPlateBedType READ currentPlateBedType WRITE setCurrentPlateBedType)
  Q_PROPERTY(float cutRotationX READ cutRotationX WRITE setCutRotationX)
  Q_PROPERTY(float cutRotationY READ cutRotationY WRITE setCutRotationY)
  Q_PROPERTY(float cutRotationZ READ cutRotationZ WRITE setCutRotationZ)
  Q_PROPERTY(QByteArray flattenHoverData READ flattenHoverData WRITE setFlattenHoverData)
  Q_PROPERTY(bool freeCamera READ freeCamera WRITE setFreeCamera)
  Q_PROPERTY(float gapArea READ gapArea WRITE setGapArea)
  Q_PROPERTY(QVariantList hintLocalRotation READ hintLocalRotation WRITE setHintLocalRotation)
  Q_PROPERTY(QByteArray hollowMarkerData READ hollowMarkerData WRITE setHollowMarkerData)
  Q_PROPERTY(QByteArray measureOverlayData READ measureOverlayData WRITE setMeasureOverlayData)
  Q_PROPERTY(QVariantList meshBatchExtruderIds READ meshBatchExtruderIds WRITE setMeshBatchExtruderIds)
  Q_PROPERTY(QVariantList meshBatchInstanceTransforms READ meshBatchInstanceTransforms WRITE setMeshBatchInstanceTransforms)
  Q_PROPERTY(QVariantList meshBatchPrintableFlags READ meshBatchPrintableFlags WRITE setMeshBatchPrintableFlags)
  Q_PROPERTY(QVariantList meshBatchVolumeTypes READ meshBatchVolumeTypes WRITE setMeshBatchVolumeTypes)
  Q_PROPERTY(bool mmuBucketEdgeDetection READ mmuBucketEdgeDetection WRITE setMmuBucketEdgeDetection)
  Q_PROPERTY(int mmuPaintTool READ mmuPaintTool WRITE setMmuPaintTool)
  Q_PROPERTY(int navigatorBottomOffset READ navigatorBottomOffset WRITE setNavigatorBottomOffset)
  Q_PROPERTY(bool navigatorEnabled READ navigatorEnabled WRITE setNavigatorEnabled)
  Q_PROPERTY(double paintClippingPosition READ paintClippingPosition WRITE setPaintClippingPosition)
  Q_PROPERTY(bool paintOnOverhangsOnly READ paintOnOverhangsOnly WRITE setPaintOnOverhangsOnly)
  Q_PROPERTY(float paintOverhangAngle READ paintOverhangAngle WRITE setPaintOverhangAngle)
  Q_PROPERTY(int paintState READ paintState WRITE setPaintState)
  Q_PROPERTY(int paintToolType READ paintToolType WRITE setPaintToolType)
  Q_PROPERTY(QVariantList plateIconStates READ plateIconStates WRITE setPlateIconStates)
  Q_PROPERTY(bool reverseZoom READ reverseZoom WRITE setReverseZoom)
  Q_PROPERTY(QVariantList selectedSourceObjectIndices READ selectedSourceObjectIndices WRITE setSelectedSourceObjectIndices)
  Q_PROPERTY(bool sequentialClearanceActive READ sequentialClearanceActive WRITE setSequentialClearanceActive)
  Q_PROPERTY(QByteArray sequentialClearanceFill READ sequentialClearanceFill WRITE setSequentialClearanceFill)
  Q_PROPERTY(QByteArray sequentialClearanceOutline READ sequentialClearanceOutline WRITE setSequentialClearanceOutline)
  Q_PROPERTY(bool sequentialClearancePreviewMode READ sequentialClearancePreviewMode WRITE setSequentialClearancePreviewMode)
  Q_PROPERTY(QByteArray sequentialHeightFill READ sequentialHeightFill WRITE setSequentialHeightFill)
  Q_PROPERTY(float smartFillAngle READ smartFillAngle WRITE setSmartFillAngle)
  Q_PROPERTY(bool uniformScale READ uniformScale WRITE setUniformScale)
  Q_PROPERTY(bool zoomToMouse READ zoomToMouse WRITE setZoomToMouse)
  Q_PROPERTY(QVariantList roleVisibility READ roleVisibility WRITE setRoleVisibility)
  Q_PROPERTY(QVariantList assembleOffsets READ assembleOffsets WRITE setAssembleOffsets)
  Q_PROPERTY(QVariantList assembleRotations READ assembleRotations WRITE setAssembleRotations)
  Q_PROPERTY(QVariantList assembleScales READ assembleScales WRITE setAssembleScales)
  Q_PROPERTY(int assemblyMeasureSelectedA READ assemblyMeasureSelectedA WRITE setAssemblyMeasureSelectedA)
  Q_PROPERTY(int assemblyMeasureSelectedB READ assemblyMeasureSelectedB WRITE setAssemblyMeasureSelectedB)
  Q_PROPERTY(float explosionRatio READ explosionRatio WRITE setExplosionRatio)

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
  void setHoveredSourceObjectIndex(int value);
  // SEL-MODIFIERS-RECT: inert rubber-band mirrors (see the Q_PROPERTY note).
  QRectF selectionRubberBand() const { return QRectF(); }
  bool selectionRubberBandActive() const { return false; }
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
  // G3 ②: inert accessors for the mirror properties above (store only).
  QByteArray advancedCutMarkerData() const { return m_advancedCutMarkerData; }
  void setAdvancedCutMarkerData(QByteArray v) { m_advancedCutMarkerData = v; }
  bool bedCaliLinesActive() const { return m_bedCaliLinesActive; }
  void setBedCaliLinesActive(bool v) { m_bedCaliLinesActive = v; }
  QVariantList bedExcludeAreas() const { return m_bedExcludeAreas; }
  void setBedExcludeAreas(const QVariantList &v) { m_bedExcludeAreas = v; }
  bool bedHeightLimitActive() const { return m_bedHeightLimitActive; }
  void setBedHeightLimitActive(bool v) { m_bedHeightLimitActive = v; }
  float bedHeightToLid() const { return m_bedHeightToLid; }
  void setBedHeightToLid(float v) { m_bedHeightToLid = v; }
  float bedHeightToRod() const { return m_bedHeightToRod; }
  void setBedHeightToRod(float v) { m_bedHeightToRod = v; }
  QByteArray bedModelMeshData() const { return m_bedModelMeshData; }
  void setBedModelMeshData(QByteArray v) { m_bedModelMeshData = v; }
  float bedPrintableHeight() const { return m_bedPrintableHeight; }
  void setBedPrintableHeight(float v) { m_bedPrintableHeight = v; }
  QUrl bedTextureUrl() const { return m_bedTextureUrl; }
  void setBedTextureUrl(const QUrl &v) { m_bedTextureUrl = v; }
  QString bedTypeImagesDir() const { return m_bedTypeImagesDir; }
  void setBedTypeImagesDir(const QString &v) { m_bedTypeImagesDir = v; }
  bool bedTypeTexturesActive() const { return m_bedTypeTexturesActive; }
  void setBedTypeTexturesActive(bool v) { m_bedTypeTexturesActive = v; }
  int brushCursorType() const { return m_brushCursorType; }
  void setBrushCursorType(int v) { m_brushCursorType = v; }
  double brushHeightRange() const { return m_brushHeightRange; }
  void setBrushHeightRange(double v) { m_brushHeightRange = v; }
  int cameraNavStyle() const { return m_cameraNavStyle; }
  void setCameraNavStyle(int v) { m_cameraNavStyle = v; }
  int currentPlateBedType() const { return m_currentPlateBedType; }
  void setCurrentPlateBedType(int v) { m_currentPlateBedType = v; }
  float cutRotationX() const { return m_cutRotationX; }
  void setCutRotationX(float v) { m_cutRotationX = v; }
  float cutRotationY() const { return m_cutRotationY; }
  void setCutRotationY(float v) { m_cutRotationY = v; }
  float cutRotationZ() const { return m_cutRotationZ; }
  void setCutRotationZ(float v) { m_cutRotationZ = v; }
  QByteArray flattenHoverData() const { return m_flattenHoverData; }
  void setFlattenHoverData(QByteArray v) { m_flattenHoverData = v; }
  bool freeCamera() const { return m_freeCamera; }
  void setFreeCamera(bool v) { m_freeCamera = v; }
  float gapArea() const { return m_gapArea; }
  void setGapArea(float v) { m_gapArea = v; }
  QVariantList hintLocalRotation() const { return m_hintLocalRotation; }
  void setHintLocalRotation(const QVariantList &v) { m_hintLocalRotation = v; }
  QByteArray hollowMarkerData() const { return m_hollowMarkerData; }
  void setHollowMarkerData(QByteArray v) { m_hollowMarkerData = v; }
  QByteArray measureOverlayData() const { return m_measureOverlayData; }
  void setMeasureOverlayData(QByteArray v) { m_measureOverlayData = v; }
  QVariantList meshBatchExtruderIds() const { return m_meshBatchExtruderIds; }
  void setMeshBatchExtruderIds(const QVariantList &v) { m_meshBatchExtruderIds = v; }
  QVariantList meshBatchInstanceTransforms() const { return m_meshBatchInstanceTransforms; }
  void setMeshBatchInstanceTransforms(const QVariantList &v) { m_meshBatchInstanceTransforms = v; }
  QVariantList meshBatchPrintableFlags() const { return m_meshBatchPrintableFlags; }
  void setMeshBatchPrintableFlags(const QVariantList &v) { m_meshBatchPrintableFlags = v; }
  QVariantList meshBatchVolumeTypes() const { return m_meshBatchVolumeTypes; }
  void setMeshBatchVolumeTypes(const QVariantList &v) { m_meshBatchVolumeTypes = v; }
  bool mmuBucketEdgeDetection() const { return m_mmuBucketEdgeDetection; }
  void setMmuBucketEdgeDetection(bool v) { m_mmuBucketEdgeDetection = v; }
  int mmuPaintTool() const { return m_mmuPaintTool; }
  void setMmuPaintTool(int v) { m_mmuPaintTool = v; }
  int navigatorBottomOffset() const { return m_navigatorBottomOffset; }
  void setNavigatorBottomOffset(int v) { m_navigatorBottomOffset = v; }
  bool navigatorEnabled() const { return m_navigatorEnabled; }
  void setNavigatorEnabled(bool v) { m_navigatorEnabled = v; }
  double paintClippingPosition() const { return m_paintClippingPosition; }
  void setPaintClippingPosition(double v) { m_paintClippingPosition = v; }
  bool paintOnOverhangsOnly() const { return m_paintOnOverhangsOnly; }
  void setPaintOnOverhangsOnly(bool v) { m_paintOnOverhangsOnly = v; }
  float paintOverhangAngle() const { return m_paintOverhangAngle; }
  void setPaintOverhangAngle(float v) { m_paintOverhangAngle = v; }
  int paintState() const { return m_paintState; }
  void setPaintState(int v) { m_paintState = v; }
  int paintToolType() const { return m_paintToolType; }
  void setPaintToolType(int v) { m_paintToolType = v; }
  QVariantList plateIconStates() const { return m_plateIconStates; }
  void setPlateIconStates(const QVariantList &v) { m_plateIconStates = v; }
  bool reverseZoom() const { return m_reverseZoom; }
  void setReverseZoom(bool v) { m_reverseZoom = v; }
  QVariantList selectedSourceObjectIndices() const { return m_selectedSourceObjectIndices; }
  void setSelectedSourceObjectIndices(const QVariantList &v) { m_selectedSourceObjectIndices = v; }
  bool sequentialClearanceActive() const { return m_sequentialClearanceActive; }
  void setSequentialClearanceActive(bool v) { m_sequentialClearanceActive = v; }
  QByteArray sequentialClearanceFill() const { return m_sequentialClearanceFill; }
  void setSequentialClearanceFill(QByteArray v) { m_sequentialClearanceFill = v; }
  QByteArray sequentialClearanceOutline() const { return m_sequentialClearanceOutline; }
  void setSequentialClearanceOutline(QByteArray v) { m_sequentialClearanceOutline = v; }
  bool sequentialClearancePreviewMode() const { return m_sequentialClearancePreviewMode; }
  void setSequentialClearancePreviewMode(bool v) { m_sequentialClearancePreviewMode = v; }
  QByteArray sequentialHeightFill() const { return m_sequentialHeightFill; }
  void setSequentialHeightFill(QByteArray v) { m_sequentialHeightFill = v; }
  float smartFillAngle() const { return m_smartFillAngle; }
  void setSmartFillAngle(float v) { m_smartFillAngle = v; }
  bool uniformScale() const { return m_uniformScale; }
  void setUniformScale(bool v) { m_uniformScale = v; }
  bool zoomToMouse() const { return m_zoomToMouse; }
  void setZoomToMouse(bool v) { m_zoomToMouse = v; }
  QVariantList roleVisibility() const { return m_roleVisibility; }
  void setRoleVisibility(const QVariantList &v) { m_roleVisibility = v; }
  QVariantList assembleOffsets() const { return m_assembleOffsets; }
  void setAssembleOffsets(const QVariantList &v) { m_assembleOffsets = v; }
  QVariantList assembleRotations() const { return m_assembleRotations; }
  void setAssembleRotations(const QVariantList &v) { m_assembleRotations = v; }
  QVariantList assembleScales() const { return m_assembleScales; }
  void setAssembleScales(const QVariantList &v) { m_assembleScales = v; }
  int assemblyMeasureSelectedA() const { return m_assemblyMeasureSelectedA; }
  void setAssemblyMeasureSelectedA(int v) { m_assemblyMeasureSelectedA = v; }
  int assemblyMeasureSelectedB() const { return m_assemblyMeasureSelectedB; }
  void setAssemblyMeasureSelectedB(int v) { m_assemblyMeasureSelectedB = v; }
  float explosionRatio() const { return m_explosionRatio; }
  void setExplosionRatio(float v) { m_explosionRatio = v; }

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
  void objectPickedSource(int sourceIndex);
  // SEL-MODIFIERS-RECT: signal-compatible mirrors of RhiViewport. The
  // software fallback never emits them (no object picking); they exist only
  // so the shared PreparePage.qml signal handlers compile on this path too.
  void objectPickedSourceWithModifiers(int sourceIndex, int volumeIndex,
                                       int modifiers);
  void rectangleSelectionFinished(int modifiers, QRectF rect,
                                  QVariantList containedSourceIndices);
  void selectionRubberBandChanged();
  void contextMenuRequested(int targetKind,
                            int sourceObjectIndex,
                            int volumeIndex,
                            int instanceIndex,
                            int plateIndex,
                            qreal popupX,
                            qreal popupY);
  // G3 ②: further signal-compatible mirrors of RhiViewport (same rationale
  // as the block above — never emitted, they exist so PreparePage's on*
  // handlers resolve on the software path).
  void advancedCutPickRequested(QVector3D worldOrigin,
                                QVector3D worldDirection,
                                int pickedSourceIndex);
  void bedDragMoveBegin();
  void bedDragMoveEnd();
  void bedDragMoveRequested(const QVector3D &worldDelta);
  void bucketFillPickRequested(QVector3D worldOrigin,
                               QVector3D worldDirection,
                               QVector3D cameraForward,
                               int pickedSourceIndex,
                               int paintState,
                               double seedFillAngle,
                               bool propagate);
  void cutPlaneDragRequested(float position);
  void cutPlaneRotateRequested(int rotationAxis, float deltaDegrees);
  void flattenHoverRequested(QVector3D worldOrigin,
                             QVector3D worldDirection,
                             int pickedSourceIndex);
  void flattenPickRequested(QVector3D worldOrigin,
                            QVector3D worldDirection,
                            int pickedSourceIndex);
  void gapAreaTuned(double gapArea);
  void gizmoDragBegin();
  void gizmoDragEnd();
  void gizmoMoveRequested(const QVector3D &worldDelta);
  void gizmoRotateRequested(int axis, float radians);
  void gizmoScaleRequested(int axis, float factor);
  void heightRangePickRequested(QVector3D worldOrigin,
                                QVector3D worldDirection,
                                QVector3D cameraPosition,
                                QVector3D cameraForward,
                                int pickedSourceIndex,
                                int paintState,
                                double height);
  void heightRangeTuned(double height);
  void hollowPickRequested(QVector3D worldOrigin,
                           QVector3D worldDirection,
                           int pickedSourceIndex);
  void measureHoverLeft();
  void measurePickRequested(QVector3D worldOrigin,
                            QVector3D worldDirection,
                            int pickedSourceIndex,
                            bool shiftHeld);
  void paintClippingTuned(double position);
  void paintPickRequested(QVector3D worldOrigin,
                          QVector3D worldDirection,
                          QVector3D cameraPosition,
                          QVector3D cameraForward,
                          int pickedSourceIndex,
                          double brushRadius,
                          int cursorType,
                          int paintState,
                          int smartFill);
  void plateIconActivated(int plateIndex, int iconId);
  // MP4 (dev plan 多盘 2026-10-20): RhiViewport signal mirror -- the page
  // binds onPlateBodyPicked on whichever viewport hosts it, and the
  // SoftwareViewport fallback must not abort the page at startup.
  void plateBodyPicked(int plateIndex);
  void smartFillPickRequested(QVector3D worldOrigin,
                              QVector3D worldDirection,
                              QVector3D cameraForward,
                              int pickedSourceIndex,
                              int paintState,
                              double smartFillAngle,
                              bool overhangsOnly,
                              double overhangAngle);

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
  QVector3D m_center = QVector3D(110.f, 0.f, -110.f);  // AXES-ROT: bed center
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
  // G3 ②: storage for the inert mirror properties.
  QByteArray m_advancedCutMarkerData;
  bool m_bedCaliLinesActive = false;
  QVariantList m_bedExcludeAreas;
  bool m_bedHeightLimitActive = false;
  float m_bedHeightToLid = 0.f;
  float m_bedHeightToRod = 0.f;
  QByteArray m_bedModelMeshData;
  float m_bedPrintableHeight = 0.f;
  QUrl m_bedTextureUrl;
  QString m_bedTypeImagesDir;
  bool m_bedTypeTexturesActive = false;
  int m_brushCursorType = 0;
  double m_brushHeightRange = 0.;
  int m_cameraNavStyle = 0;
  int m_currentPlateBedType = 0;
  float m_cutRotationX = 0.f;
  float m_cutRotationY = 0.f;
  float m_cutRotationZ = 0.f;
  QByteArray m_flattenHoverData;
  bool m_freeCamera = false;
  float m_gapArea = 0.f;
  QVariantList m_hintLocalRotation;
  QByteArray m_hollowMarkerData;
  QByteArray m_measureOverlayData;
  QVariantList m_meshBatchExtruderIds;
  QVariantList m_meshBatchInstanceTransforms;
  QVariantList m_meshBatchPrintableFlags;
  QVariantList m_meshBatchVolumeTypes;
  bool m_mmuBucketEdgeDetection = false;
  int m_mmuPaintTool = 0;
  int m_navigatorBottomOffset = 0;
  bool m_navigatorEnabled = true;
  double m_paintClippingPosition = 0.;
  bool m_paintOnOverhangsOnly = false;
  float m_paintOverhangAngle = 0.f;
  int m_paintState = 0;
  int m_paintToolType = 0;
  QVariantList m_plateIconStates;
  bool m_reverseZoom = false;
  QVariantList m_selectedSourceObjectIndices;
  bool m_sequentialClearanceActive = false;
  QByteArray m_sequentialClearanceFill;
  QByteArray m_sequentialClearanceOutline;
  bool m_sequentialClearancePreviewMode = false;
  QByteArray m_sequentialHeightFill;
  float m_smartFillAngle = 0.f;
  bool m_uniformScale = true;
  bool m_zoomToMouse = false;
  QVariantList m_roleVisibility;
  QVariantList m_assembleOffsets;
  QVariantList m_assembleRotations;
  QVariantList m_assembleScales;
  int m_assemblyMeasureSelectedA = -1;
  int m_assemblyMeasureSelectedB = -1;
  float m_explosionRatio = 1.f;
};
