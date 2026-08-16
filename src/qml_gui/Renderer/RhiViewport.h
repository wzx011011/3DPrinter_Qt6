#pragma once

#include <QByteArray>
#include <QUrl>
#include <QHoverEvent>
#include <QImage>
#include <QPointF>
#include <QQuickRhiItem>
#include <QRectF>
#include <QString>
#include <QVariant>
#include <QVector3D>
#include <QVector4D>
#include <vector>

#include "CameraController.h"
#include "PrepareSceneData.h"
#include "ViewportContextHit.h"

class RhiViewportRenderer;

class RhiViewport : public QQuickRhiItem
{
  Q_OBJECT
  Q_PROPERTY(int canvasType READ canvasType WRITE setCanvasType NOTIFY canvasTypeChanged)
  // Phase 91 (ASMEXPLODE-01): explosion ratio mirroring upstream m_explosion_ratio
  // (GLCanvas3D.hpp:596, default 1.0). The setter calls update() so the renderer
  // re-applies the per-volume offset on every change. Bound from
  // editorVm.explosionRatio in AssemblePage.qml.
  Q_PROPERTY(float explosionRatio READ explosionRatio WRITE setExplosionRatio NOTIFY explosionRatioChanged)
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
  // v5.15 (BEDTEX): printer-profile bed texture image (upstream PartPlate
  // m_logo_texture_filename sourced from the machine model's bed_texture).
  Q_PROPERTY(QUrl bedTextureUrl READ bedTextureUrl WRITE setBedTextureUrl NOTIFY bedTextureUrlChanged)
  // v5.16 (BEDMODEL): printer bed_model STL triangle stream (scene coords).
  Q_PROPERTY(QByteArray bedModelMeshData READ bedModelMeshData WRITE setBedModelMeshData)
  // v5.16 (BEDTYPE-TEX): BBL-only bed-type texture gates + assets + the
  // current plate's bed type index (Slic3r BedType).
  Q_PROPERTY(bool bedTypeTexturesActive READ bedTypeTexturesActive WRITE setBedTypeTexturesActive)
  Q_PROPERTY(bool bedCaliLinesActive READ bedCaliLinesActive WRITE setBedCaliLinesActive)
  Q_PROPERTY(QString bedTypeImagesDir READ bedTypeImagesDir WRITE setBedTypeImagesDir)
  Q_PROPERTY(int currentPlateBedType READ currentPlateBedType WRITE setCurrentPlateBedType)
  Q_PROPERTY(int currentPlateIndex READ currentPlateIndex WRITE setCurrentPlateIndex)
  Q_PROPERTY(int plateCount READ plateCount WRITE setPlateCount)
  Q_PROPERTY(QVariantList activePlateObjectIndices READ activePlateObjectIndices WRITE setActivePlateObjectIndices)
  Q_PROPERTY(QVariantList meshBatchSourceObjectIndices READ meshBatchSourceObjectIndices WRITE setMeshBatchSourceObjectIndices)
  Q_PROPERTY(QVariantList meshBatchVolumeIndices READ meshBatchVolumeIndices WRITE setMeshBatchVolumeIndices)
  Q_PROPERTY(QVariantList meshBatchInstanceIndices READ meshBatchInstanceIndices WRITE setMeshBatchInstanceIndices)
  Q_PROPERTY(bool layerEditingInputActive READ layerEditingInputActive WRITE setLayerEditingInputActive)
  Q_PROPERTY(bool contextToolInputCaptured READ contextToolInputCaptured WRITE setContextToolInputCaptured)
  // Phase 138 (ASM-01): per-source-object assemble offset (GL X,Y,Z), one entry
  // per source object index (matches meshBatchSourceObjectIndices ordering).
  // Bound from editorVm.assembleOffsets in AssemblePage.qml. The renderer applies
  // it as a per-object translation on the CanvasAssembleView path in
  // buildModelVertices (alongside the explosion offset). Prepare/Preview unaffected.
  Q_PROPERTY(QVariantList assembleOffsets READ assembleOffsets WRITE setAssembleOffsets)
  // Phase 141 (DEBT-04): per-source-object assemble rotation (Euler XYZ, radians)
  // and scale (XYZ), parallel to assembleOffsets. The renderer composes the full
  // transform (translate * rotate * scale) in buildModelVertices so Rotate/Scale
  // gizmo drags reflect in the live CanvasAssembleView render (v4.8 tech debt).
  Q_PROPERTY(QVariantList assembleRotations READ assembleRotations WRITE setAssembleRotations)
  Q_PROPERTY(QVariantList assembleScales READ assembleScales WRITE setAssembleScales)
  Q_PROPERTY(int selectedSourceObjectIndex READ selectedSourceObjectIndex WRITE setSelectedSourceObjectIndex)
  Q_PROPERTY(int hoveredSourceObjectIndex READ hoveredSourceObjectIndex WRITE setHoveredSourceObjectIndex)
  // Phase 92 (ASMMEASURE-02): the two selected source-object indices the
  // Assembly measurement overlay annotates (volume A and volume B). Default -1
  // = not set. The setter calls update() so the renderer re-renders the
  // overlay on selection change. Bound from the viewmodel's first-two selected
  // indices in AssemblePage.qml (task 92-01-07).
  Q_PROPERTY(int assemblyMeasureSelectedA READ assemblyMeasureSelectedA WRITE setAssemblyMeasureSelectedA NOTIFY assemblyMeasureSelectionChanged)
  Q_PROPERTY(int assemblyMeasureSelectedB READ assemblyMeasureSelectedB WRITE setAssemblyMeasureSelectedB NOTIFY assemblyMeasureSelectionChanged)
  Q_PROPERTY(bool showWipeTower READ showWipeTower WRITE setShowWipeTower)
  Q_PROPERTY(float wipeTowerWidth READ wipeTowerWidth WRITE setWipeTowerWidth)
  Q_PROPERTY(float wipeTowerDepth READ wipeTowerDepth WRITE setWipeTowerDepth)
  Q_PROPERTY(float wipeTowerHeight READ wipeTowerHeight WRITE setWipeTowerHeight)
  Q_PROPERTY(float wipeTowerX READ wipeTowerX WRITE setWipeTowerX)
  Q_PROPERTY(float wipeTowerZ READ wipeTowerZ WRITE setWipeTowerZ)
  // Phase 109 (WTMESH-05): Option B real-mesh Q_PROPERTYs. hasRealMesh gates
  // the renderer branch (true -> buildWipeTowerMeshVertices, false -> Option A
  // buildWipeTowerVertices). meshVertices carries the flattened XYZ triples
  // (libslic3r world frame) as a QVariantList so it crosses the QML boundary
  // cleanly (mirrors the autoFilamentMaps QVariantList pattern). The setter
  // converts back to std::vector<float> for the renderer's synchronize() pull.
  Q_PROPERTY(bool wipeTowerHasRealMesh READ wipeTowerHasRealMesh WRITE setWipeTowerHasRealMesh)
  Q_PROPERTY(QVariantList wipeTowerMeshVertices READ wipeTowerMeshVertices WRITE setWipeTowerMeshVertices)
  Q_PROPERTY(float markerX READ markerX WRITE setMarkerX)
  Q_PROPERTY(float markerY READ markerY WRITE setMarkerY)
  Q_PROPERTY(float markerZ READ markerZ WRITE setMarkerZ)
  Q_PROPERTY(bool showMarker READ showMarker WRITE setShowMarker)
  Q_PROPERTY(int gizmoMode READ gizmoMode WRITE setGizmoMode NOTIFY gizmoModeChanged)
  Q_PROPERTY(bool wireframeMode READ wireframeMode WRITE setWireframeMode NOTIFY wireframeModeChanged)
  Q_PROPERTY(int gcodeViewMode READ gcodeViewMode WRITE setGcodeViewMode NOTIFY gcodeViewModeChanged)
  Q_PROPERTY(QVariantList roleVisibility READ roleVisibility WRITE setRoleVisibility NOTIFY roleVisibilityChanged)
  Q_PROPERTY(int cutAxis READ cutAxis WRITE setCutAxis)
  Q_PROPERTY(float cutPosition READ cutPosition WRITE setCutPosition)
  // Phase 240 (GIZ-04): interactive cut-plane tilt (upstream GLGizmoCut3D
  // rotation grabbers). The renderer rotates the plane quad around its
  // on-axis center point by these Euler XYZ degrees.
  Q_PROPERTY(float cutRotationX READ cutRotationX WRITE setCutRotationX)
  Q_PROPERTY(float cutRotationY READ cutRotationY WRITE setCutRotationY)
  Q_PROPERTY(float cutRotationZ READ cutRotationZ WRITE setCutRotationZ)
  // Phase 240 (GIZ-05): in-scene measure overlay line stream (see
  // EditorViewModel::measureOverlayData for the wire format). Rendered with
  // the shared line pipeline while gizmoMode == GizmoMeasure.
  Q_PROPERTY(QByteArray measureOverlayData READ measureOverlayData WRITE setMeasureOverlayData)
  // Phase 240 (GIZ-03): hovered-facet highlight stream for the flatten
  // gizmo (see EditorViewModel::flattenHoverData for the wire format).
  // Rendered with the translucent fill pipeline while gizmoMode ==
  // GizmoFlatten.
  Q_PROPERTY(QByteArray flattenHoverData READ flattenHoverData WRITE setFlattenHoverData)
  Q_PROPERTY(QString lastThumbnailData READ lastThumbnailData NOTIFY thumbnailCaptured)
  // Phase 121 (PAINT-02/OV-02): reverse-channel Q_PROPERTY. The ViewModel
  // flattens the selected object's painted facets into a world-transformed byte
  // stream (see EditorViewModel::paintOverlayData); the renderer uploads it to
  // m_paintOverlayBuffer via uploadPaintOverlayBuffer and renders it after the
  // model mesh (reuse mesh pipeline -- GizmoVertex + m_fillPipeline). The
  // setter calls update() so a paintDataChanged -> QML binding -> setter ->
  // synchronize() -> uploadPaintOverlayBuffer loop closes on every paint stroke.
  Q_PROPERTY(QByteArray paintOverlayData READ paintOverlayData WRITE setPaintOverlayData)
  // Phase HOLLOW: drain-hole marker vertex byte stream (flattened by
  // EditorViewModel::hollowMarkerData). Same Q_PROPERTY-as-byte-pipe pattern
  // as paintOverlayData: QML binds viewport3d.hollowMarkerData to
  // editorVm.hollowMarkerData, the setter stashes it, and synchronize() hands
  // it to the renderer for upload.
  Q_PROPERTY(QByteArray hollowMarkerData READ hollowMarkerData WRITE setHollowMarkerData)
  // v5.13: connector-pin marker byte stream (same format as hollowMarkerData;
  // EditorViewModel::advancedCutMarkerData producer).
  Q_PROPERTY(QByteArray advancedCutMarkerData READ advancedCutMarkerData WRITE setAdvancedCutMarkerData)
  // Phase 121 (PAINT-03/OV-02/OV-05): brush params. emitPaintPickIfActive
  // forwards these to the ViewModel instead of the Phase 120 hardcoded
  // defaults (2.0/1/1). brushRadius is the world-space sphere/circle radius
  // (mm), brushCursorType mirrors PaintCursorType (0=Circle, 1=Sphere),
  // paintState is the EnforcerBlockerType int (1=Enforcer, 2=Blocker,
  // 3..16=ExtruderN). brushMouseScreenX/Y feed the sphere-cursor position
  // (rendered by renderBrushCursor), and brushButtonState drives its color
  // (0=hover black, 1=left blue, 2=right red).
  Q_PROPERTY(float brushRadius READ brushRadius WRITE setBrushRadius)
  Q_PROPERTY(int brushCursorType READ brushCursorType WRITE setBrushCursorType)
  Q_PROPERTY(int paintState READ paintState WRITE setPaintState)
  // Phase 240 (GIZ-02): smart-fill params. paintToolType mirrors
  // EditorViewModel::supportPaintToolType (0=Brush, 1=BucketFill,
  // 2=SmartFill, 3=GapFill); smartFillAngle is the seed-fill angle
  // threshold (deg); paintOnOverhangsOnly + paintOverhangAngle drive the
  // overhang filter (upstream m_paint_on_overhangs_only +
  // m_highlight_by_angle_threshold_deg). emitPaintPickIfActive forwards
  // these so the ViewModel picks the brush vs smart-fill path.
  Q_PROPERTY(int paintToolType READ paintToolType WRITE setPaintToolType)
  Q_PROPERTY(float smartFillAngle READ smartFillAngle WRITE setSmartFillAngle)
  Q_PROPERTY(bool paintOnOverhangsOnly READ paintOnOverhangsOnly WRITE setPaintOnOverhangsOnly)
  Q_PROPERTY(float paintOverhangAngle READ paintOverhangAngle WRITE setPaintOverhangAngle)
  Q_PROPERTY(float brushMouseScreenX READ brushMouseScreenX WRITE setBrushMouseScreenX)
  Q_PROPERTY(float brushMouseScreenY READ brushMouseScreenY WRITE setBrushMouseScreenY)
  Q_PROPERTY(int brushButtonState READ brushButtonState WRITE setBrushButtonState)
  // Phase 121 (PAINT-02/OV-04): MMU per-extruder filament colors as hex
  // strings (mirrors EditorViewModel.extrudersColors). The renderer maps
  // ExtruderN -> colors[N-1] for the MMU overlay coloring.
  Q_PROPERTY(QVariantList extrudersColors READ extrudersColors WRITE setExtrudersColors)
  // v5.12 gap-closure: camera preferences (bound from settingsVm in QML).
  // Consumed by wheelEvent (reverseZoom/zoomToMouse) and mousePressEvent
  // (cameraNavStyle: touchpad swaps left/right drag; freeCamera allows roll).
  Q_PROPERTY(bool reverseZoom READ reverseZoom WRITE setReverseZoom)
  Q_PROPERTY(bool zoomToMouse READ zoomToMouse WRITE setZoomToMouse)
  Q_PROPERTY(bool freeCamera READ freeCamera WRITE setFreeCamera)
  Q_PROPERTY(int cameraNavStyle READ cameraNavStyle WRITE setCameraNavStyle)
  // Phase 237 (VIEW-01): projection toggle for the upstream View-menu radio
  // pair "Use Perspective View" / "Use Orthogonal View"
  // (MainFrame.cpp:2604-2620, app_config use_perspective_camera). The setter
  // calls update() so the next render rebuilds the projection.
  Q_PROPERTY(bool orthographicCamera READ orthographicCamera WRITE setOrthographicCamera NOTIFY cameraProjectionChanged)

public:
  // Mirrors upstream ECanvasType { CanvasView3D=0, CanvasPreview=1,
  // CanvasAssembleView=2 } (GLCanvas3D.hpp:509-513). Phase 90 adds the third
  // canvas host so AssembleView reuses the default RHI/D3D11 mesh-render path.
  enum CanvasType
  {
    CanvasView3D = 0,
    CanvasPreview = 1,
    CanvasAssembleView = 2
  };
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
    GizmoSlaSupports = 18,
    // Phase 92 (ASMMEASURE-01): Assembly measurement gizmo (Ctrl+Y,
    // GLGizmoAssembly / ONLY_ASSEMBLY). Distinct from GizmoMeasure (Prepare,
    // Ctrl+U) — mirrors upstream GLGizmoAssembly being a separate class from
    // GLGizmoMeasure (GLGizmoAssembly.hpp:9). The AssembleView mask returns
    // (1 << 19); the renderer gates the overlay on m_gizmoMode == 19.
    GizmoAssemblyMeasure = 19
  };
  Q_ENUM(GizmoMode)

  enum GCodeViewMode
  {
    GCodeFeature = 0,
    GCodeExtruder = 1,
    GCodeSpeed = 2,
    GCodeLayerHeight = 3,
    GCodePressure = 4,
    GCodePixel = 5
  };
  Q_ENUM(GCodeViewMode)

  explicit RhiViewport(QQuickItem *parent = nullptr);

  QQuickRhiItemRenderer *createRenderer() override;

  int canvasType() const { return m_canvasType; }
  void setCanvasType(int value);
  // Phase 91 (ASMEXPLODE-01): explosion ratio mirroring upstream m_explosion_ratio
  // (GLCanvas3D.hpp:596). The setter triggers update() so synchronize()+render()
  // re-apply the per-volume offset on every change.
  float explosionRatio() const { return m_explosionRatio; }
  void setExplosionRatio(float value);

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
  QUrl bedTextureUrl() const { return m_bedTextureUrl; }
  void setBedTextureUrl(const QUrl &value);
  QByteArray bedModelMeshData() const { return m_bedModelMeshData; }
  void setBedModelMeshData(const QByteArray &value);
  bool bedTypeTexturesActive() const { return m_bedTypeTexturesActive; }
  void setBedTypeTexturesActive(bool value);
  bool bedCaliLinesActive() const { return m_bedCaliLinesActive; }
  void setBedCaliLinesActive(bool value);
  QString bedTypeImagesDir() const { return m_bedTypeImagesDir; }
  void setBedTypeImagesDir(const QString &value);
  int currentPlateBedType() const { return m_currentPlateBedType; }
  void setCurrentPlateBedType(int value);
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
  // Phase 138 (ASM-01): per-source-object assemble offset list (one QVector3D
  // per source object index, GL X,Y,Z).
  QVariantList assembleOffsets() const { return m_assembleOffsets; }
  void setAssembleOffsets(const QVariantList &value);
  // Phase 141 (DEBT-04): per-source-object assemble rotation/scale lists.
  QVariantList assembleRotations() const { return m_assembleRotations; }
  void setAssembleRotations(const QVariantList &value);
  QVariantList assembleScales() const { return m_assembleScales; }
  void setAssembleScales(const QVariantList &value);
  int selectedSourceObjectIndex() const { return m_selectedSourceObjectIndex; }
  void setSelectedSourceObjectIndex(int value);
  int hoveredSourceObjectIndex() const { return m_hoveredSourceObjectIndex; }
  void setHoveredSourceObjectIndex(int value);
  // Phase 92 (ASMMEASURE-02): Assembly measurement overlay selection indices.
  int assemblyMeasureSelectedA() const { return m_assemblyMeasureSelectedA; }
  void setAssemblyMeasureSelectedA(int value);
  int assemblyMeasureSelectedB() const { return m_assemblyMeasureSelectedB; }
  void setAssemblyMeasureSelectedB(int value);
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
  // Phase 109 (WTMESH-05): Option B real-mesh getters/setters.
  bool wipeTowerHasRealMesh() const { return m_wipeTowerHasRealMesh; }
  void setWipeTowerHasRealMesh(bool value);
  QVariantList wipeTowerMeshVertices() const;
  void setWipeTowerMeshVertices(const QVariantList &value);
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

  int gcodeViewMode() const { return m_gcodeViewMode; }
  void setGcodeViewMode(int value);

  QVariantList roleVisibility() const { return m_roleVisibility; }
  void setRoleVisibility(const QVariantList &value);

  int cutAxis() const { return m_cutAxis; }
  void setCutAxis(int value);

  float cutPosition() const { return m_cutPosition; }
  void setCutPosition(float value);

  // Phase 240 (GIZ-04): cut-plane rotation getters/setters.
  float cutRotationX() const { return m_cutRotationX; }
  void setCutRotationX(float v) { m_cutRotationX = v; updateCutPlane(); }
  float cutRotationY() const { return m_cutRotationY; }
  void setCutRotationY(float v) { m_cutRotationY = v; updateCutPlane(); }
  float cutRotationZ() const { return m_cutRotationZ; }
  void setCutRotationZ(float v) { m_cutRotationZ = v; updateCutPlane(); }

  // Phase 240 (GIZ-05): measure overlay byte-pipe getter/setter.
  QByteArray measureOverlayData() const { return m_measureOverlayData; }
  void setMeasureOverlayData(const QByteArray &data);

  // Phase 240 (GIZ-03): flatten hover highlight byte-pipe getter/setter.
  QByteArray flattenHoverData() const { return m_flattenHoverData; }
  void setFlattenHoverData(const QByteArray &data);

  QString lastThumbnailData() const { return m_lastThumbnailData; }

  // Phase 121 (PAINT-02/OV-02): paint overlay reverse-channel getter/setter.
  QByteArray paintOverlayData() const { return m_paintOverlayData; }
  void setPaintOverlayData(const QByteArray &data);
  QByteArray hollowMarkerData() const { return m_hollowMarkerData; }
  void setHollowMarkerData(const QByteArray &data);
  QByteArray advancedCutMarkerData() const { return m_advancedCutMarkerData; }
  void setAdvancedCutMarkerData(const QByteArray &data);
  // Phase 121 (PAINT-03/OV-02/OV-05): brush params. Setters call update() so
  // the renderer re-renders the sphere cursor + overlay on every change.
  float brushRadius() const { return m_brushRadius; }
  void setBrushRadius(float r);
  int brushCursorType() const { return m_brushCursorType; }
  void setBrushCursorType(int t);
  int paintState() const { return m_paintState; }
  void setPaintState(int s);
  float brushMouseScreenX() const { return m_brushMouseScreenX; }
  void setBrushMouseScreenX(float x);
  float brushMouseScreenY() const { return m_brushMouseScreenY; }
  void setBrushMouseScreenY(float y);
  int brushButtonState() const { return m_brushButtonState; }
  void setBrushButtonState(int s);
  // Phase 240 (GIZ-02): smart-fill params (see the Q_PROPERTY block).
  int paintToolType() const { return m_paintToolType; }
  void setPaintToolType(int t) { m_paintToolType = t; }
  float smartFillAngle() const { return m_smartFillAngle; }
  void setSmartFillAngle(float a) { m_smartFillAngle = a; }
  bool paintOnOverhangsOnly() const { return m_paintOnOverhangsOnly; }
  void setPaintOnOverhangsOnly(bool b) { m_paintOnOverhangsOnly = b; }
  float paintOverhangAngle() const { return m_paintOverhangAngle; }
  void setPaintOverhangAngle(float a) { m_paintOverhangAngle = a; }
  // v5.12 camera preferences
  bool reverseZoom() const { return m_reverseZoom; }
  void setReverseZoom(bool r) { m_reverseZoom = r; update(); }
  bool zoomToMouse() const { return m_zoomToMouse; }
  void setZoomToMouse(bool z) { m_zoomToMouse = z; update(); }
  bool freeCamera() const { return m_freeCamera; }
  void setFreeCamera(bool f) { m_freeCamera = f; m_camera.setFreeCamera(f); update(); }
  int cameraNavStyle() const { return m_cameraNavStyle; }
  void setCameraNavStyle(int s) { m_cameraNavStyle = s; update(); }
  // Phase 237 (VIEW-01): orthographic projection toggle (see the
  // orthographicCamera Q_PROPERTY above).
  bool orthographicCamera() const { return m_camera.useOrtho(); }
  void setOrthographicCamera(bool ortho);
  // Phase 121 (PAINT-02/OV-04): MMU per-extruder filament colors.
  QVariantList extrudersColors() const { return m_extrudersColors; }
  void setExtrudersColors(const QVariantList &c);

  Q_INVOKABLE void requestFitView(float cx, float cy, float cz, float r);
  Q_INVOKABLE void requestPreviewFit();
  Q_INVOKABLE void requestViewPreset(int preset);
  // Phase 237 (VIEW-01): upstream-named view selection (mirrors
  // GLCanvas3D::select_view -> Camera::select_view, Camera.cpp:86-107). The
  // accepted directions match the upstream key map "plate"/"top"/"bottom"/
  // "front"/"rear"/"left"/"right" (GLCanvas3D.cpp:3192-3201 Ctrl+0..6).
  // "plate" additionally zooms to the bed (upstream zoom_to_bed) using the
  // viewport bed Q_PROPERTYs.
  Q_INVOKABLE void selectView(const QString &direction);
  // v5.16 (CIRC-01): undo()/redo()/clearHistory() repaint-only no-ops removed.
  // History actions live on EditorViewModel (QUndoStack) and BackendContext.
  Q_INVOKABLE void mirrorSelection(int axis);
  // Phase 240 (GIZ-05): project a world-space point to viewport pixel
  // coordinates (for the 2D measure annotation overlay projected from 3D
  // anchor points). Y follows Qt item coordinates (origin top-left). The
  // mapping uses the same proj/view matrices the renderer uploads each
  // frame, so it stays in lockstep with the 3D overlay geometry.
  Q_INVOKABLE QPointF projectWorldToScreen(float worldX, float worldY, float worldZ) const;
  // Phase 240 (GIZ-06): intersect the pick ray for a screen position with
  // the bed plane (Z=0). Returns the bed point in world mm; an invalid
  // QPointF when the ray is parallel to the bed / points away. Used by the
  // SVG drop flow to place the dropped SVG at the drop point (upstream
  // GLGizmoSVG workbench, minimal port).
  Q_INVOKABLE QPointF screenToBedPoint(qreal screenX, qreal screenY) const;
  Q_INVOKABLE void arrangeSelected(float spacing = 0.f, bool rotation = false, bool alignY = false);
  Q_INVOKABLE void requestThumbnailCapture(int plateIndex, int size = 128);
  // Phase 95 (THUMBCAP-03): GUI-thread delivery slot the renderer targets via
  // a queued QMetaObject::invokeMethod. Encodes the captured QImage to the
  // base64 PNG m_lastThumbnailData format and emits thumbnailCaptured(), so
  // PreparePage.qml's contract (lastThumbnailData / onThumbnailCaptured) stays
  // unchanged. plateIndex is carried for Phase 96 per-plate routing.
  void deliverThumbnail(const QImage &image, int plateIndex);

signals:
  void bedTextureUrlChanged();
  void canvasTypeChanged();
  void explosionRatioChanged();
  // Phase 237 (VIEW-01): projection toggle notify (orthographicCamera).
  void cameraProjectionChanged();
  void assemblyMeasureSelectionChanged();
  void gizmoModeChanged();
  void wireframeModeChanged();
  void gcodeViewModeChanged();
  void roleVisibilityChanged();
  void thumbnailCaptured();
  /// Phase 156 (CLOS-03): per-plate capture delivery signal. Carries the
  /// plateIndex so the QML consumer can route the bytes back into
  /// PartPlate::setThumbnail via ProjectServiceMock::setPlateThumbnailFromBase64.
  /// The legacy no-arg thumbnailCaptured() above stays for back-compat with
  /// QML bindings reading lastThumbnailData for the current plate's live preview.
  void thumbnailCapturedForPlate(int plateIndex, const QString &data);
  void objectPickedSource(int sourceIndex);
  void contextMenuRequested(int targetKind,
                            int sourceObjectIndex,
                            int volumeIndex,
                            int instanceIndex,
                            int plateIndex,
                            qreal popupX,
                            qreal popupY);
  // Phase 69: emitted during a move-gizmo axis drag. worldDelta is the
  // incremental translation to apply to the selected object this frame
  // (in world mm). gizmoDragBegin fires once at press (before the first
  // move); gizmoDragEnd fires once at release (after the last move). The
  // ViewModel uses begin/end to coalesce the whole drag into one undo entry.
  void gizmoMoveRequested(const QVector3D &worldDelta);
  void gizmoRotateRequested(int axis, float radians);
  void gizmoScaleRequested(int axis, float factor);
  void gizmoDragBegin();
  void gizmoDragEnd();
  // Phase 115 (MEASURE-04): emitted on mouse-move/click while the measure
  // gizmo is active (m_gizmoMode == GizmoMeasure). worldOrigin/worldDirection
  // are the world-space pick ray (same GizmoMath::computeRay output the
  // object picking uses). pickedSourceIndex is the stage-1 AABB survivor
  // (RhiViewport::pickSourceObjectAt, Phase 113 stage-1). shiftHeld mirrors
  // the upstream Shift toggle (GLGizmoMeasure.cpp:409-442): true forces
  // EMode::PointSelection; false keeps the default FeatureSelection. QML
  // forwards these to EditorViewModel::pickMeasureFeatureAt which runs the
  // stage-2 SceneRaycaster + MeasureEngine::getFeature.
  //
  // Naming: the parameters are "worldOrigin/worldDirection" (not
  // "rayOrigin/rayDirection") because the rhiViewportSelectionPickingBridge
  // StaysCppOwned source-audit forbids the literal "ray" substring anywhere
  // in PreparePage.qml -- QML must not own picking/geometry-hit logic. The
  // QML handler forwards these args opaquely to the ViewModel; no ray math
  // lives in QML.
  void measurePickRequested(QVector3D worldOrigin,
                            QVector3D worldDirection,
                            int pickedSourceIndex,
                            bool shiftHeld);
  // Phase HOLLOW: emitted on left-click while the hollow gizmo is active
  // (m_gizmoMode == GizmoHollow). QML forwards to
  // EditorViewModel::placeHollowPoint, which runs the stage-2 SceneRaycaster
  // pick and appends a sla::DrainHole at the mesh-local intersection.
  void hollowPickRequested(QVector3D worldOrigin,
                           QVector3D worldDirection,
                           int pickedSourceIndex);
  // v5.13: emitted on left-click while the AdvancedCut gizmo is active with
  // connectors enabled. QML forwards to EditorViewModel::placeAdvancedCutConnector.
  void advancedCutPickRequested(QVector3D worldOrigin,
                                QVector3D worldDirection,
                                int pickedSourceIndex);
  // Phase 115 (MEASURE-04): emitted on cursor-leave while the measure gizmo
  // is active. QML forwards to EditorViewModel::clearMeasureReadout so no
  // stale feature highlight lingers off-mesh.
  void measureHoverLeft();
  // Phase 120 (PAINT-01): emitted on mouse-down/move while a paint gizmo is
  // active (m_gizmoMode in {GizmoSupportPaint, GizmoSeamPaint,
  // GizmoMmuSegmentation}). worldOrigin/worldDirection are the world-space pick
  // ray (same GizmoMath::computeRay output object picking uses).
  // pickedSourceIndex is the stage-1 AABB survivor (RhiViewport::
  // pickSourceObjectAt). brushRadius + cursorType + paintState are the current
  // brush params (Phase 121 brush UI will source these from the gizmo state;
  // Phase 120 threads conservative defaults). QML forwards these to
  // EditorViewModel::paintAtFacet which runs the stage-2 SceneRaycaster +
  // PaintEngine::paintAt (TriangleSelector::select_patch).
  //
  // Naming follows measurePickRequested (world* not ray* -- the
  // rhiViewportSelectionPickingBridgeStaysCppOwned audit forbids the literal
  // "ray" substring in QML; QML forwards these opaquely, no ray math in QML).
  void paintPickRequested(QVector3D worldOrigin,
                          QVector3D worldDirection,
                          int pickedSourceIndex,
                          double brushRadius,
                          int cursorType,
                          int paintState,
                          int smartFill);
  // Phase 240 (GIZ-02): smart-fill pick variant. Emitted instead of
  // paintPickRequested when the SmartFill tool is active or Shift is held on
  // click (Phase 240 spec: Shift+click = smart fill). Carries the seed-fill
  // angle + overhang filter state.
  void smartFillPickRequested(QVector3D worldOrigin,
                              QVector3D worldDirection,
                              int pickedSourceIndex,
                              int paintState,
                              double smartFillAngle,
                              bool overhangsOnly,
                              double overhangAngle);
  // Phase 240 (GIZ-03): flatten gizmo pick/hover. The hover variant feeds
  // the hovered-facet highlight; the click variant rotates the object so the
  // picked facet's normal faces down (upstream GLGizmoFlatten::on_mouse).
  void flattenHoverRequested(QVector3D worldOrigin,
                             QVector3D worldDirection,
                             int pickedSourceIndex);
  void flattenPickRequested(QVector3D worldOrigin,
                            QVector3D worldDirection,
                            int pickedSourceIndex);
  // Phase 240 (GIZ-04): interactive cut-plane manipulation (upstream
  // GLGizmoCut3D grabbers). The drag signal carries the new plane position
  // along the cut axis (mm); the rotate signal carries the axis index
  // (0=X, 1=Y, 2=Z) + the delta angle (degrees) accumulated during the drag.
  void cutPlaneDragRequested(float position);
  void cutPlaneRotateRequested(int rotationAxis, float deltaDegrees);

private:
  friend class RhiViewportRenderer;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void hoverMoveEvent(QHoverEvent *event) override;
  void hoverLeaveEvent(QHoverEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  QMatrix4x4 cameraMvp(float aspect) const;
  void fitPreviewCameraToData();
  void updatePickingScene();
  bool activeToolCapturesContextGesture() const;
  int pickSourceObjectAt(const QPointF &position);
  ViewportContextHit classifyContextAt(const QPointF &position);
  // Phase 69/70: gizmo-axis hit test and center derivation.
  int pickGizmoAxisAt(const QPointF &position);
  QVector3D currentGizmoCenter() const;
  void resetGizmoDragState();
  // Phase 115 (MEASURE-04): emit measurePickRequested for the active GizmoMeasure
  // path. Runs the stage-1 pick (pickSourceObjectAt) to get the candidate source
  // index, builds the world-space ray via GizmoMath::computeRay, and reads the
  // Shift modifier from the supplied keyboardModifiers (Qt::ShiftModifier -> shift).
  // No-op when m_gizmoMode != GizmoMeasure (only the measure gizmo drives this).
  void emitMeasurePickIfActive(const QPointF &position,
                               Qt::KeyboardModifiers modifiers);
  // Phase HOLLOW: emit hollowPickRequested for the active hollow gizmo.
  // Mirrors emitMeasurePickIfActive: stage-1 pick (pickSourceObjectAt) for
  // the candidate source index, world ray via GizmoMath::computeRay. No-op
  // when m_gizmoMode != GizmoHollow.
  void emitHollowPickIfActive(const QPointF &position,
                              Qt::KeyboardModifiers modifiers);
  // v5.13: emit advancedCutPickRequested for the AdvancedCut gizmo (connector
  // pin placement). Mirror of emitHollowPickIfActive.
  void emitAdvancedCutPickIfActive(const QPointF &position,
                                   Qt::KeyboardModifiers modifiers);
  // Phase 120 (PAINT-01): emit paintPickRequested for the active paint gizmos
  // (GizmoSupportPaint / GizmoSeamPaint / GizmoMmuSegmentation). Mirrors
  // emitMeasurePickIfActive: stage-1 pick (pickSourceObjectAt) for the candidate
  // source index, world ray via GizmoMath::computeRay, plus the current brush
  // params (radius / cursor type / paint state). No-op when m_gizmoMode is not
  // one of the three paint gizmos (keeps other gizmos' mouse handling untouched).
  void emitPaintPickIfActive(const QPointF &position,
                             Qt::KeyboardModifiers modifiers);
  // Phase 121 (PAINT-03/OV-05): update the brush-cursor tracking fields
  // (screen-space position + button state) so renderBrushCursor can draw the
  // sphere cursor at the mouse. No-op when no paint gizmo is active.
  void updateBrushCursorState(const QPointF &position, int buttonState);
  // Phase 240 (GIZ-03): emit flattenHoverRequested / flattenPickRequested
  // for the active GizmoFlatten mode. Stage-1 pick + world ray, same
  // contract as emitMeasurePickIfActive.
  void emitFlattenPickIfActive(const QPointF &position, bool click);
  // Phase 240 (GIZ-04): hit-test the interactive cut plane at the press
  // position. Returns 0 = miss, 1 = plane body (drag along axis), 2/3 =
  // rotation grabber (tilt around world X / Y). Uses the selected object's
  // bounds + the current cutAxis/cutPosition/rotation state.
  int pickCutPlaneAt(const QPointF &position);
  // Phase 240 (GIZ-04): the cut plane's world-space center + the two plane
  // axes (for the drag projection + grabber placement).
  QVector3D cutPlaneCenter() const;
  // Phase 240 (GIZ-04): push the current cut plane state to the renderer
  // (rotation changes mark the plane dirty).
  void updateCutPlane();

  int m_canvasType = CanvasView3D;
  // Phase 91 (ASMEXPLODE-01): explosion ratio mirroring upstream m_explosion_ratio
  // (GLCanvas3D.hpp:596, default 1.0). The setter triggers update() so the
  // renderer re-applies the per-volume offset (see RhiViewportRenderer).
  float m_explosionRatio = 1.0f;
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
  QUrl m_bedTextureUrl;
  QByteArray m_bedModelMeshData;
  bool m_bedTypeTexturesActive = false;
  bool m_bedCaliLinesActive = false;
  QString m_bedTypeImagesDir;
  int m_currentPlateBedType = 0;
  int m_currentPlateIndex = 0;
  int m_plateCount = 1;
  QVariantList m_activePlateObjectIndices;
  QVariantList m_meshBatchSourceObjectIndices;
  QVariantList m_meshBatchVolumeIndices;
  QVariantList m_meshBatchInstanceIndices;
  bool m_layerEditingInputActive = false;
  bool m_contextToolInputCaptured = false;
  // Phase 138 (ASM-01): per-source-object assemble offset (GL X,Y,Z).
  QVariantList m_assembleOffsets;
  // Phase 141 (DEBT-04): parallel rotation/scale lists.
  QVariantList m_assembleRotations;
  QVariantList m_assembleScales;
  int m_selectedSourceObjectIndex = -1;
  int m_hoveredSourceObjectIndex = -1;
  // Phase 92 (ASMMEASURE-02): the two volumes the overlay annotates. -1 = not set.
  int m_assemblyMeasureSelectedA = -1;
  int m_assemblyMeasureSelectedB = -1;
  bool m_showWipeTower = false;
  float m_wipeTowerWidth = 10.f;
  float m_wipeTowerDepth = 10.f;
  float m_wipeTowerHeight = 50.f;
  float m_wipeTowerX = 100.f;
  float m_wipeTowerZ = 25.f;
  // Phase 109 (WTMESH-05): Option B real-mesh storage. Populated by the
  // wipeTowerHasRealMesh / wipeTowerMeshVertices Q_PROPERTY setters (declared
  // below) from the EditorViewModel readback. synchronize() pulls these into
  // RhiViewportRenderer. Defaults keep hasRealMesh=false so the pre-slice and
  // single-material paths take the Option A fallback (Phase 99 Frozen Decision
  // 2 baseline). The mesh vertices are flattened XYZ triples (libslic3r world
  // frame).
  bool m_wipeTowerHasRealMesh = false;
  std::vector<float> m_wipeTowerMeshVertices;
  float m_markerX = 0.f;
  float m_markerY = 0.f;
  float m_markerZ = 0.f;
  bool m_showMarker = true;
  int m_gizmoMode = GizmoMove;
  bool m_wireframeMode = false;
  int m_gcodeViewMode = GCodeFeature;
  QVariantList m_roleVisibility;
  int m_cutAxis = 2;
  float m_cutPosition = 0.f;
  // Phase 240 (GIZ-04): interactive cut-plane tilt + drag state.
  float m_cutRotationX = 0.f;
  float m_cutRotationY = 0.f;
  float m_cutRotationZ = 0.f;
  int m_cutPlaneGrab = 0;          ///< 0=none, 1=plane body, 2=X grabber, 3=Y grabber
  float m_cutPlaneDragStartT = 0.f;
  float m_cutPlaneRotateStartAngle = 0.f;
  // Phase 240 (GIZ-05): measure overlay line stream.
  QByteArray m_measureOverlayData;
  // Phase 240 (GIZ-03): flatten hover facet stream.
  QByteArray m_flattenHoverData;
  QString m_lastThumbnailData;
  // Phase 121 (PAINT-02/OV-02): painted-facet overlay reverse-channel payload.
  // Flattened by EditorViewModel::paintOverlayData (world-transformed bytes).
  QByteArray m_paintOverlayData;
  // Flattened by EditorViewModel::hollowMarkerData (world-space disc fans).
  QByteArray m_hollowMarkerData;
  // v5.13: connector-pin markers (EditorViewModel::advancedCutMarkerData).
  QByteArray m_advancedCutMarkerData;
  // Phase 121 (PAINT-03/OV-02/OV-05): brush params. emitPaintPickIfActive
  // forwards brushRadius/brushCursorType/paintState; renderBrushCursor uses
  // brushMouseScreenX/Y + brushButtonState for the sphere cursor.
  float m_brushRadius = 2.0f;
  int m_brushCursorType = 1; // 1=Sphere (PaintCursorType::Sphere)
  int m_paintState = 1;      // EnforcerBlockerType: 1=Enforcer
  // Phase 240 (GIZ-02): smart-fill params (see the Q_PROPERTY block).
  int m_paintToolType = 0;   // 0=Brush, 2=SmartFill
  float m_smartFillAngle = 30.f;
  bool m_paintOnOverhangsOnly = false;
  float m_paintOverhangAngle = 0.f;
  /// Phase 240 (GIZ-02): true while the current pick originates from a
  /// press event (smart fill is click-driven; drags keep the brush path).
  bool m_paintClickPress = false;
  float m_brushMouseScreenX = 0.f;
  float m_brushMouseScreenY = 0.f;
  int m_brushButtonState = 0; // 0=hover, 1=left, 2=right
  // v5.12 camera preferences (bound from SettingsViewModel via QML)
  bool m_reverseZoom = false;
  bool m_zoomToMouse = true;
  bool m_freeCamera = false;
  int m_cameraNavStyle = 0;  // 0=Default, 1=Touchpad
  // Phase 121 (PAINT-02/OV-04): MMU per-extruder filament colors (hex strings).
  QVariantList m_extrudersColors;
  int m_fitRequestCount = 0;
  int m_viewPreset = 3;
  // Phase 95 (THUMBCAP-03): item-side capture-request fields set by
  // requestThumbnailCapture (GUI thread). synchronize() copies them to the
  // renderer and clears m_thumbnailRequestPending (mirrors the
  // m_cameraDirty=false consumption pattern at RhiViewportRenderer.cpp:95).
  bool m_thumbnailRequestPending = false;
  int m_thumbnailPlateIndex = 0;
  int m_thumbnailSize = 128;
  bool m_previewCameraFitted = false;
  QVector4D m_previewFitHint;
  qint64 m_sceneGeneration = 1;
  qint64 m_modelGeneration = 1;
  qint64 m_pickModelGeneration = 0;
  qint64 m_pickSceneGeneration = 0;
  CameraController m_camera;
  PrepareSceneData m_pickScene;
  QPointF m_lastMousePosition;
  QPointF m_pressPosition;
  Qt::MouseButton m_dragButton = Qt::NoButton;
  QPointF m_contextPressPosition;
  bool m_contextPressActive = false;
  bool m_contextDragExceeded = false;
  bool m_contextToolCapturedAtPress = false;
  bool m_contextLayerEditingAtPress = false;
  int m_pressPickedSourceObjectIndex = -1;
  bool m_cameraDirty = true;
  bool m_bedTextureDirty = true;   // v5.15 (BEDTEX): consumed by renderer synchronize

  // Phase 69/70: gizmo drag state.
  // m_gizmoAxis: 0=none, 1=X, 2=Y, 3=Z.
  int m_gizmoAxis = 0;
  int m_gizmoDragMode = GizmoMove;
  bool m_gizmoDragging = false;
  float m_gizmoDragStartT = 0.f;
  float m_gizmoRotateStartAngle = 0.f;
  QVector3D m_gizmoDragCenter;
};
