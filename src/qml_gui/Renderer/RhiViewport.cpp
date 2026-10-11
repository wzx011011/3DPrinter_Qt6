#include "RhiViewport.h"
#include "RhiViewportRenderer.h"
#include "core/model/PartPlateList.h"
#include "core/rendering/GizmoCenter.h"
#include "core/rendering/GizmoMath.h"
#include "core/rendering/NavigatorCube.h"
#include "core/rendering/ObjectPicking.h"
#include "core/rendering/PickingRaycaster.h"

#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QCursor>
#include <QHoverEvent>
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QSvgRenderer>
#include <QTimer>
#include <QVector4D>
#include <QWheelEvent>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace {
struct GcvPackedSegment
{
  float x1, y1, z1, x2, y2, z2;
  float r, g, b;
  float feedrate, fan_speed, temperature, width, height, layer_time, acceleration;
  float jerk, pressure_advance, actual_speed, actual_flow;
  int extruder_id, layer, move;
  int role;  // must match PackedSegment layout exactly (canonical libvgcode index).
};
static_assert(sizeof(GcvPackedSegment) == 96,
              "GcvPackedSegment must be 96 bytes (20 floats + 4 ints)");

static const QVector3D kGizmoAxes[3] = {
    QVector3D(1, 0, 0), QVector3D(0, 1, 0), QVector3D(0, 0, 1)};
}

RhiViewport::RhiViewport(QQuickItem *parent)
    : QQuickRhiItem(parent)
{
  setAcceptedMouseButtons(Qt::AllButtons);
  setAcceptHoverEvents(true);
  setMirrorVertically(true);
  // EXPERIMENT: MSAA sample count > 1 to trigger QQuickRhiItem internal
  // depth-stencil buffer creation (QQuickRhiItem has no public depth buffer
  // API; this is the only known trigger). Remove if depth test proves
  // unworkable or MSAA visual/perf cost is unacceptable.
  setSampleCount(4);
  // Default: all 20 canonical libvgcode extrusion roles visible so renderer-side
  // filtering is a no-op until QML binds Plan 03's UI (matches upstream defaults).
  m_roleVisibility.reserve(20);
  for (int i = 0; i < 20; ++i)
    m_roleVisibility.append(true);
  // P14 (CAM-PARITY): seed the camera scene box from the member defaults so
  // the zoom bounds and target validation hold before QML binds the bed.
  updateSceneExtent();
}

QQuickRhiItemRenderer *RhiViewport::createRenderer()
{
  return new RhiViewportRenderer();
}

void RhiViewport::setCanvasType(int value)
{
  if (m_canvasType == value)
    return;
  m_canvasType = value;
  if (m_canvasType == CanvasPreview)
    fitPreviewCameraToData();
  emit canvasTypeChanged();
  update();
}

void RhiViewport::setExplosionRatio(float value)
{
  // Phase 91 (ASMEXPLODE-01): mirror upstream m_explosion_ratio
  // (GLCanvas3D.hpp:596). update() triggers synchronize()+render() so the
  // renderer re-applies the per-volume offset on every change. Guarded against
  // no-op and non-finite values so Prepare/Preview-equivalent rendering (ratio
  // == 1.0) produces zero offset.
  if (qFuzzyCompare(m_explosionRatio, value) || !std::isfinite(value))
    return;
  m_explosionRatio = value;
  emit explosionRatioChanged();
  update();
}

void RhiViewport::setMeshData(const QByteArray &data)
{
  if (m_meshData == data)
    return;
  m_meshData = data;
  ++m_sceneGeneration;
  ++m_modelGeneration;
  update();
}

void RhiViewport::setSequentialClearanceActive(bool active)
{
  if (m_sequentialClearanceActive == active) return;
  m_sequentialClearanceActive = active;
  update();
}

// P15.11: drag-time preview flag. A flip changes the baked per-vertex color
// (failure blue vs preview gray), so update() re-synchronizes and the renderer
// marks the clearance buffers dirty.
void RhiViewport::setSequentialClearancePreviewMode(bool preview)
{
  if (m_sequentialClearancePreviewMode == preview) return;
  m_sequentialClearancePreviewMode = preview;
  update();
}

void RhiViewport::setSequentialClearanceOutline(const QByteArray &data)
{
  if (m_sequentialClearanceOutline == data) return;
  m_sequentialClearanceOutline = data;
  update();
}

void RhiViewport::setSequentialClearanceFill(const QByteArray &data)
{
  if (m_sequentialClearanceFill == data) return;
  m_sequentialClearanceFill = data;
  update();
}

void RhiViewport::setSequentialHeightFill(const QByteArray &data)
{
  if (m_sequentialHeightFill == data) return;
  m_sequentialHeightFill = data;
  update();
}

void RhiViewport::setPreviewData(const QByteArray &data)
{
  if (m_previewData == data)
    return;
  m_previewData = data;
  fitPreviewCameraToData();
  update();
}

void RhiViewport::setLayerMin(int value)
{
  if (m_layerMin == value)
    return;
  m_layerMin = value;
  update();
}

void RhiViewport::setLayerMax(int value)
{
  if (m_layerMax == value)
    return;
  m_layerMax = value;
  update();
}

void RhiViewport::setMoveEnd(int value)
{
  if (m_moveEnd == value)
    return;
  m_moveEnd = value;
  update();
}

void RhiViewport::setShowTravelMoves(bool value)
{
  if (m_showTravelMoves == value)
    return;
  m_showTravelMoves = value;
  update();
}

void RhiViewport::setShowSelectedOutline(bool value)
{
  if (m_showSelectedOutline == value)
    return;
  m_showSelectedOutline = value;
  emit showSelectedOutlineChanged();
  update();
}

void RhiViewport::setShowBed(bool value)
{
  if (m_showBed == value)
    return;
  m_showBed = value;
  ++m_sceneGeneration;
  update();
}

void RhiViewport::setBedWidth(float value)
{
  if (qFuzzyCompare(m_bedWidth, value))
    return;
  m_bedWidth = value;
  ++m_sceneGeneration;
  update();
  updateSceneExtent();
  // P15.9 (PLATEANCHOR): the bed footprint drives the plate grid stride.
  // PLATEICONS-GL: the icon batch re-anchors with the bed layout/count.
  rebuildPlateIcons();
  emit plateAnchorsChanged();
}

void RhiViewport::setBedDepth(float value)
{
  if (qFuzzyCompare(m_bedDepth, value))
    return;
  m_bedDepth = value;
  ++m_sceneGeneration;
  update();
  updateSceneExtent();
  // P15.9 (PLATEANCHOR): the bed footprint drives the plate grid stride.
  // PLATEICONS-GL: the icon batch re-anchors with the bed layout/count.
  rebuildPlateIcons();
  emit plateAnchorsChanged();
}

void RhiViewport::setBedOriginX(float value)
{
  if (qFuzzyCompare(m_bedOriginX, value))
    return;
  m_bedOriginX = value;
  ++m_sceneGeneration;
  update();
  // P15.9 (PLATEANCHOR): plate anchors offset from the bed origin.
  // PLATEICONS-GL: the icon batch re-anchors with the bed layout/count.
  rebuildPlateIcons();
  emit plateAnchorsChanged();
}

void RhiViewport::setBedOriginY(float value)
{
  if (qFuzzyCompare(m_bedOriginY, value))
    return;
  m_bedOriginY = value;
  ++m_sceneGeneration;
  update();
  // P15.9 (PLATEANCHOR): plate anchors offset from the bed origin.
  // PLATEICONS-GL: the icon batch re-anchors with the bed layout/count.
  rebuildPlateIcons();
  emit plateAnchorsChanged();
}

void RhiViewport::setBedShapeType(int value)
{
  if (m_bedShapeType == value)
    return;
  m_bedShapeType = value;
  ++m_sceneGeneration;
  update();
  updateSceneExtent();
}

void RhiViewport::setBedDiameter(float value)
{
  if (qFuzzyCompare(m_bedDiameter, value))
    return;
  m_bedDiameter = value;
  ++m_sceneGeneration;
  update();
  updateSceneExtent();
}

void RhiViewport::setBedTextureUrl(const QUrl &value)
{
  if (m_bedTextureUrl == value)
    return;
  m_bedTextureUrl = value;
  m_bedTextureDirty = true;
  emit bedTextureUrlChanged();
  update();
}

// v5.16 setters: plain value + update(); the renderer diff-checks the
// members each synchronize (same pattern as meshData).
void RhiViewport::setBedModelMeshData(const QByteArray &value)
{
  if (m_bedModelMeshData == value)
    return;
  m_bedModelMeshData = value;
  update();
}

// v5.16 (EXCLAREA): exclude polygons reshape the bed fill geometry, so they
// bump the scene generation like the other bed-shape properties.
void RhiViewport::setBedExcludeAreas(const QVariantList &value)
{
  if (m_bedExcludeAreas == value)
    return;
  m_bedExcludeAreas = value;
  ++m_sceneGeneration;
  update();
}

// v5.16 (HTLIMIT): plain value + update(); the renderer diff-checks the
// members each synchronize (same pattern as bedTypeTexturesActive).
void RhiViewport::setBedHeightToRod(float value)
{
  if (qFuzzyCompare(m_bedHeightToRod, value))
    return;
  m_bedHeightToRod = value;
  update();
}

void RhiViewport::setBedHeightToLid(float value)
{
  if (qFuzzyCompare(m_bedHeightToLid, value))
    return;
  m_bedHeightToLid = value;
  update();
  updateSceneExtent();
}

void RhiViewport::setBedHeightLimitActive(bool value)
{
  if (m_bedHeightLimitActive == value)
    return;
  m_bedHeightLimitActive = value;
  update();
}

// Q9 (OUTOFBED): plain value + update() like the HTLIMIT setters; the
// renderer forwards it into PrepareSceneData::setPrintableHeight each
// synchronize (that setter is nearlyEqual-guarded, so repeat values cause no
// scene churn) and 0 keeps the print-volume top clamp unbounded.
void RhiViewport::setBedPrintableHeight(float value)
{
  if (qFuzzyCompare(m_bedPrintableHeight, value))
    return;
  m_bedPrintableHeight = value;
  update();
}

void RhiViewport::setBedTypeTexturesActive(bool value)
{
  if (m_bedTypeTexturesActive == value)
    return;
  m_bedTypeTexturesActive = value;
  update();
}

void RhiViewport::setBedCaliLinesActive(bool value)
{
  if (m_bedCaliLinesActive == value)
    return;
  m_bedCaliLinesActive = value;
  update();
}

void RhiViewport::setBedTypeImagesDir(const QString &value)
{
  if (m_bedTypeImagesDir == value)
    return;
  m_bedTypeImagesDir = value;
  update();
}

void RhiViewport::setCurrentPlateBedType(int value)
{
  if (m_currentPlateBedType == value)
    return;
  m_currentPlateBedType = value;
  update();
}

void RhiViewport::setCurrentPlateIndex(int value)
{
  if (m_currentPlateIndex == value)
    return;
  m_currentPlateIndex = value;
  ++m_sceneGeneration;
  ++m_modelGeneration;
  update();
}

void RhiViewport::setPlateCount(int value)
{
  if (m_plateCount == value)
    return;
  m_plateCount = value;
  ++m_sceneGeneration;
  ++m_modelGeneration;
  update();
  // P15.9 (PLATEANCHOR): the anchor list carries one entry per plate.
  emit plateAnchorsChanged();
  // PLATEICONS-GL: the icon batch re-anchors with the bed layout/count.
  rebuildPlateIcons();
}

void RhiViewport::setActivePlateObjectIndices(const QVariantList &value)
{
  if (m_activePlateObjectIndices == value)
    return;
  m_activePlateObjectIndices = value;
  ++m_sceneGeneration;
  ++m_modelGeneration;
  update();
}

void RhiViewport::setMeshBatchSourceObjectIndices(const QVariantList &value)
{
  if (m_meshBatchSourceObjectIndices == value)
    return;
  m_meshBatchSourceObjectIndices = value;
  ++m_sceneGeneration;
  ++m_modelGeneration;
  update();
}

void RhiViewport::setMeshBatchVolumeIndices(const QVariantList &value)
{
  if (m_meshBatchVolumeIndices == value)
    return;
  m_meshBatchVolumeIndices = value;
  ++m_sceneGeneration;
  ++m_modelGeneration;
  update();
}

void RhiViewport::setMeshBatchInstanceIndices(const QVariantList &value)
{
  if (m_meshBatchInstanceIndices == value)
    return;
  m_meshBatchInstanceIndices = value;
  ++m_sceneGeneration;
  ++m_modelGeneration;
  update();
}

// P15.1/15.2 (COLOR): per-batch render-channel metadata.
void RhiViewport::setMeshBatchVolumeTypes(const QVariantList &value)
{
  if (m_meshBatchVolumeTypes == value)
    return;
  m_meshBatchVolumeTypes = value;
  ++m_sceneGeneration;
  ++m_modelGeneration;
  update();
}

void RhiViewport::setMeshBatchExtruderIds(const QVariantList &value)
{
  if (m_meshBatchExtruderIds == value)
    return;
  m_meshBatchExtruderIds = value;
  ++m_sceneGeneration;
  ++m_modelGeneration;
  update();
}

void RhiViewport::setMeshBatchPrintableFlags(const QVariantList &value)
{
  if (m_meshBatchPrintableFlags == value)
    return;
  m_meshBatchPrintableFlags = value;
  ++m_sceneGeneration;
  ++m_modelGeneration;
  update();
}

void RhiViewport::setLayerEditingInputActive(bool value)
{
  m_layerEditingInputActive = value;
}

void RhiViewport::setContextToolInputCaptured(bool value)
{
  m_contextToolInputCaptured = value;
}

// Phase 138 (ASM-01): per-source-object assemble offset. Force a model re-upload
// so buildModelVertices re-applies the per-object translation on the
// CanvasAssembleView branch. Gated to CanvasAssembleView in buildModelVertices,
// so Prepare/Preview are unaffected.
void RhiViewport::setAssembleOffsets(const QVariantList &value)
{
  if (m_assembleOffsets == value)
    return;
  m_assembleOffsets = value;
  ++m_sceneGeneration;
  ++m_modelGeneration;
  update();
}
// Phase 141 (DEBT-04): parallel rotation/scale setters. Same invalidation pattern
// as setAssembleOffsets — bump generations + update() so buildModelVertices re-applies
// the full transform on the CanvasAssembleView branch.
void RhiViewport::setAssembleRotations(const QVariantList &value)
{
  if (m_assembleRotations == value)
    return;
  m_assembleRotations = value;
  ++m_sceneGeneration;
  ++m_modelGeneration;
  update();
}
void RhiViewport::setAssembleScales(const QVariantList &value)
{
  if (m_assembleScales == value)
    return;
  m_assembleScales = value;
  ++m_sceneGeneration;
  ++m_modelGeneration;
  update();
}

void RhiViewport::setSelectedSourceObjectIndex(int value)
{
  if (m_selectedSourceObjectIndex == value)
    return;
  m_selectedSourceObjectIndex = value;
  update();
}

// P15.11 (MULTICENTER): full multi-selection setter. update() triggers
// synchronize() so the renderer re-unions the selection AABBs (gizmo pivot,
// selection-center sphere, corner-tick highlight) on the next frame.
void RhiViewport::setSelectedSourceObjectIndices(const QVariantList &value)
{
  if (m_selectedSourceObjectIndices == value)
    return;
  m_selectedSourceObjectIndices = value;
  update();
}

void RhiViewport::setHoveredSourceObjectIndex(int value)
{
  if (m_hoveredSourceObjectIndex == value)
    return;
  m_hoveredSourceObjectIndex = value;
  update();
}

void RhiViewport::setAssemblyMeasureSelectedA(int value)
{
  // Phase 92 (ASMMEASURE-02): overlay selection index A. update() triggers
  // synchronize()+render() so the renderer re-uploads and re-draws the
  // Assembly measurement overlay anchored to the new selection.
  if (m_assemblyMeasureSelectedA == value)
    return;
  m_assemblyMeasureSelectedA = value;
  emit assemblyMeasureSelectionChanged();
  update();
}

void RhiViewport::setAssemblyMeasureSelectedB(int value)
{
  // Phase 92 (ASMMEASURE-02): overlay selection index B (see setAssemblyMeasureSelectedA).
  if (m_assemblyMeasureSelectedB == value)
    return;
  m_assemblyMeasureSelectedB = value;
  emit assemblyMeasureSelectionChanged();
  update();
}

void RhiViewport::setShowWipeTower(bool value)
{
  if (m_showWipeTower == value)
    return;
  m_showWipeTower = value;
  update();
}

void RhiViewport::setWipeTowerWidth(float value)
{
  if (qFuzzyCompare(m_wipeTowerWidth, value))
    return;
  m_wipeTowerWidth = value;
  update();
}

void RhiViewport::setWipeTowerDepth(float value)
{
  if (qFuzzyCompare(m_wipeTowerDepth, value))
    return;
  m_wipeTowerDepth = value;
  update();
}

void RhiViewport::setWipeTowerHeight(float value)
{
  if (qFuzzyCompare(m_wipeTowerHeight, value))
    return;
  m_wipeTowerHeight = value;
  update();
}

void RhiViewport::setWipeTowerX(float value)
{
  if (qFuzzyCompare(m_wipeTowerX, value))
    return;
  m_wipeTowerX = value;
  update();
}

void RhiViewport::setWipeTowerZ(float value)
{
  if (qFuzzyCompare(m_wipeTowerZ, value))
    return;
  m_wipeTowerZ = value;
  update();
}

// Phase 109 (WTMESH-05): Option B real-mesh setters. The mesh vertices cross
// the QML boundary as a QVariantList of floats (mirrors the autoFilamentMaps
// pattern); the setter converts back to std::vector<float> for the renderer's
// synchronize() pull. Each setter calls update() so synchronize() + render()
// re-run on every change (the renderer's dirty-flag comparison handles the
// no-op-skip when the value did not actually change).
void RhiViewport::setWipeTowerHasRealMesh(bool value)
{
  if (m_wipeTowerHasRealMesh == value)
    return;
  m_wipeTowerHasRealMesh = value;
  update();
}

QVariantList RhiViewport::wipeTowerMeshVertices() const
{
  QVariantList out;
  out.reserve(int(m_wipeTowerMeshVertices.size()));
  for (float v : m_wipeTowerMeshVertices)
    out.append(v);
  return out;
}

void RhiViewport::setWipeTowerMeshVertices(const QVariantList &value)
{
  std::vector<float> converted;
  converted.reserve(size_t(value.size()));
  bool ok = false;
  for (const QVariant &entry : value)
  {
    const float f = entry.toFloat(&ok);
    if (!ok)
      return; // Malformed entry -- keep the prior mesh (defensive).
    converted.push_back(f);
  }
  if (m_wipeTowerMeshVertices == converted)
    return;
  m_wipeTowerMeshVertices = std::move(converted);
  update();
}

void RhiViewport::setMarkerX(float value)
{
  if (qFuzzyCompare(m_markerX, value))
    return;
  m_markerX = value;
  update();
}

void RhiViewport::setMarkerY(float value)
{
  if (qFuzzyCompare(m_markerY, value))
    return;
  m_markerY = value;
  update();
}

void RhiViewport::setMarkerZ(float value)
{
  if (qFuzzyCompare(m_markerZ, value))
    return;
  m_markerZ = value;
  update();
}

void RhiViewport::setShowMarker(bool value)
{
  if (m_showMarker == value)
    return;
  m_showMarker = value;
  update();
}

void RhiViewport::setGizmoMode(int value)
{
  if (m_gizmoMode == value)
    return;
  m_gizmoMode = value;
  emit gizmoModeChanged();
  update();
}

void RhiViewport::setSidebarField(const QString &value)
{
  if (m_sidebarField == value)
    return;
  m_sidebarField = value;
  emit sidebarFieldChanged();
  update();
}

void RhiViewport::setUniformScale(bool value)
{
  if (m_uniformScale == value)
    return;
  m_uniformScale = value;
  emit uniformScaleChanged();
  update();
}

// P15.11 (SIDEBAR-LOCAL-AXES): reverse-channel pipe for the sidebar hint
// orientation. Selection change -> stateChanged -> QML binding -> setter ->
// update() -> synchronize() -> gizmo buffer rebuild, mirroring the
// sidebarField/uniformScale loop.
void RhiViewport::setHintLocalRotation(const QVariantList &value)
{
  if (m_hintLocalRotation == value)
    return;
  m_hintLocalRotation = value;
  emit hintLocalRotationChanged();
  update();
}

void RhiViewport::setWireframeMode(bool value)
{
  if (m_wireframeMode == value)
    return;
  m_wireframeMode = value;
  emit wireframeModeChanged();
  update();
}

void RhiViewport::setGcodeViewMode(int value)
{
  if (m_gcodeViewMode == value)
    return;
  m_gcodeViewMode = value;
  emit gcodeViewModeChanged();
  update();
}

void RhiViewport::setRoleVisibility(const QVariantList &value)
{
  // Render-side filter only: store + update(). Does NOT mutate m_previewData
  // (Phase 41 interaction-stability invariant; the renderer skips masked spans).
  if (m_roleVisibility == value)
    return;
  m_roleVisibility = value;
  emit roleVisibilityChanged();
  update();
}

void RhiViewport::setCutAxis(int value)
{
  if (m_cutAxis == value)
    return;
  m_cutAxis = value;
  update();
}

void RhiViewport::setCutPosition(float value)
{
  if (qFuzzyCompare(m_cutPosition, value))
    return;
  m_cutPosition = value;
  update();
}

// Phase 240 (GIZ-05): measure overlay byte-pipe setter (same pattern as
// setPaintOverlayData).
void RhiViewport::setMeasureOverlayData(const QByteArray &data)
{
  m_measureOverlayData = data;
  update();
}

// Phase 240 (GIZ-03): flatten hover highlight byte-pipe setter.
void RhiViewport::setFlattenHoverData(const QByteArray &data)
{
  m_flattenHoverData = data;
  update();
}

// Phase 121 (PAINT-02/OV-02): reverse-channel setter. Stores the flattened
// paint-facet byte stream (from EditorViewModel::paintOverlayData) and triggers
// update() so synchronize() pulls it into the renderer. No equality short-circuit:
// the byte stream may differ each paint stroke even at the same size, and a
// redundant update() is cheap (one dirty check).
void RhiViewport::setPaintOverlayData(const QByteArray &data)
{
  m_paintOverlayData = data;
  update();
}

void RhiViewport::setHollowMarkerData(const QByteArray &data)
{
  m_hollowMarkerData = data;
  update();
}

void RhiViewport::setAdvancedCutMarkerData(const QByteArray &data)
{
  m_advancedCutMarkerData = data;
  update();
}

// Phase 121 (PAINT-03/OV-02/OV-05): brush param setters. Each calls update()
// so renderBrushCursor + the overlay stay in sync with the UI controls.
void RhiViewport::setBrushRadius(float r)
{
  if (qFuzzyCompare(m_brushRadius, r))
    return;
  m_brushRadius = r;
  update();
}

void RhiViewport::setBrushCursorType(int t)
{
  if (m_brushCursorType == t)
    return;
  m_brushCursorType = t;
  update();
}

void RhiViewport::setPaintState(int s)
{
  if (m_paintState == s)
    return;
  m_paintState = s;
  update();
}

void RhiViewport::setBrushMouseScreenX(float x)
{
  if (qFuzzyCompare(m_brushMouseScreenX, x))
    return;
  m_brushMouseScreenX = x;
  update();
}

void RhiViewport::setBrushMouseScreenY(float y)
{
  if (qFuzzyCompare(m_brushMouseScreenY, y))
    return;
  m_brushMouseScreenY = y;
  update();
}

void RhiViewport::setBrushButtonState(int s)
{
  if (m_brushButtonState == s)
    return;
  m_brushButtonState = s;
  update();
}

void RhiViewport::setExtrudersColors(const QVariantList &c)
{
  m_extrudersColors = c;
  update();
}

void RhiViewport::requestFitView(float cx, float cy, float cz, float r)
{
  m_camera.fitView(cx, cy, cz, r);
  m_cameraDirty = true;
  ++m_fitRequestCount;
  update();
  emit navigatorLabelsChanged();
  emit plateAnchorsChanged();
}

void RhiViewport::requestPreviewFit()
{
  if (m_canvasType != CanvasPreview) {
    update();
    return;
  }

  if (!m_previewCameraFitted)
    fitPreviewCameraToData();

  if (m_previewCameraFitted) {
    m_camera.fitView(m_previewFitHint.x(),
                     m_previewFitHint.y(),
                     m_previewFitHint.z(),
                     m_previewFitHint.w());
    m_cameraDirty = true;
    ++m_fitRequestCount;
  }
  update();
}

void RhiViewport::requestZoom(float delta)
{
  // GAP-4: centered keyboard zoom (upstream GLCanvas3D.cpp on_char
  // 'I'/'O' -> zoom delta +-1 through update_zoom, Camera.cpp:83), the same
  // math the plain wheel path uses (RhiViewport.cpp wheelEvent -> m_camera.zoom).
  m_camera.zoom(delta);
  update();
}

void RhiViewport::requestViewPreset(int preset)
{
  m_viewPreset = preset;
  switch (preset)
  {
  case 0:
    m_camera.viewTop();
    break;
  case 1:
    m_camera.viewFront();
    break;
  case 2:
    m_camera.viewRight();
    break;
  default:
    m_camera.viewIso();
    break;
  }
  m_cameraDirty = true;
  update();
  emit navigatorLabelsChanged();
  emit plateAnchorsChanged();
}

// Phase 237 (VIEW-01): upstream-named view selection (upstream
// GLCanvas3D::select_view -> Camera::select_view, Camera.cpp:86-107). The
// direction strings match the upstream Ctrl+0..6 key map
// (GLCanvas3D.cpp:3192-3201) and the View-menu items
// (MainFrame.cpp:2213-2235 add_common_view_menu_items):
//   "plate"  -> select_view("plate") + zoom_to_bed()
//   "top" / "bottom" / "front" / "rear" / "left" / "right" -> orientation only
// "plate" zooms to the bed using the same bed Q_PROPERTYs the renderer draws
// with (upstream MainFrame.cpp:2216-2219 pairs the two calls).
void RhiViewport::selectView(const QString &direction)
{
  const QString dir = direction.toLower();
  if (dir == QLatin1String("plate"))
    m_camera.viewPlate();
  else if (dir == QLatin1String("top"))
    m_camera.viewTop();
  else if (dir == QLatin1String("bottom"))
    m_camera.viewBottom();
  else if (dir == QLatin1String("front"))
    m_camera.viewFront();
  else if (dir == QLatin1String("rear"))
    m_camera.viewRear();
  else if (dir == QLatin1String("left"))
    m_camera.viewLeft();
  else if (dir == QLatin1String("right"))
    m_camera.viewRight();
  else // "plate" / "iso" / unknown: default orientation
    m_camera.viewIso();

  if (dir == QLatin1String("plate"))
  {
    // zoom_to_bed equivalent: fit the camera to the bed rectangle. The bed
    // lives in the GL XZ plane (Y up); bedOriginX/bedOriginY are raw slic3r
    // X/Y, and scene Z negates slic3r Y (AXES-ROT rotation mapping). The
    // fit includes the bed icon column overhang (~2.2% of the width) so the
    // upstream plate icon strip stays in frame (icons hang just outside the
    // bed edge, PartPlate.cpp:641-655).
    const float w = qMax(1.0f, m_bedWidth) * 1.045f;
    const float d = qMax(1.0f, m_bedDepth);
    if (m_bedShapeType == 1)
    {
      const float radius = qMax(1.0f, m_bedDiameter);
      m_camera.fitView(m_bedOriginX, 0.f, -m_bedOriginY, radius * 0.5f);
    }
    else
    {
      m_camera.fitView(m_bedOriginX + w * 0.5f, 0.f,
                       -(m_bedOriginY + d * 0.5f),
                       0.5f * std::sqrt(w * w + d * d));
    }
  }
  m_viewPreset = 3; // isometric family; legacy int preset stays for GLToolbars
  m_cameraDirty = true;
  update();
  emit navigatorLabelsChanged();
  emit plateAnchorsChanged();
}

// Phase 237 (VIEW-01): orthographic projection toggle (upstream View-menu
// radio pair, MainFrame.cpp:2604-2620). Render MVP and picking both derive
// from CameraController::projMatrix, so flipping the flag re-renders
// consistently.
void RhiViewport::setOrthographicCamera(bool ortho)
{
  if (m_camera.useOrtho() == ortho)
    return;
  m_camera.setUseOrtho(ortho);
  m_cameraDirty = true;
  emit cameraProjectionChanged();
  update();
  // P15.9 (PLATEANCHOR): the projection flip moves every projected anchor.
  emit plateAnchorsChanged();
}

void RhiViewport::mirrorSelection(int axis)
{
  Q_UNUSED(axis);
  update();
}

void RhiViewport::arrangeSelected(float spacing, bool rotation, bool alignY)
{
  Q_UNUSED(spacing);
  Q_UNUSED(rotation);
  Q_UNUSED(alignY);
  update();
}

void RhiViewport::requestThumbnailCapture(int plateIndex, int size, int variant)
{
  // Phase 95 (THUMBCAP-01/03): real QRhi texture readback dispatch. The
  // previous solid-color stub (flat dark PNG fabricated on the GUI thread) is
  // removed. This sets the capture-request fields and schedules a render so
  // synchronize() copies the request to the renderer, which performs the
  // offscreen render + async readback inside render() and delivers the QImage
  // back via deliverThumbnail (queued). Mirrors the requestFitView/
  // requestViewPreset pattern (set state + update()).
  m_thumbnailPlateIndex = plateIndex;
  m_thumbnailSize = qMax(32, size);
  m_thumbnailVariant = variant;
  m_thumbnailRequestPending = true;
  update();  // schedule synchronize()+render() on the render thread
}

void RhiViewport::deliverThumbnail(const QImage &image, int plateIndex, int variant)
{
  // Phase 95 (THUMBCAP-03): GUI-thread delivery slot targeted by the
  // renderer's queued QMetaObject::invokeMethod. Encodes the captured QImage
  // to the base64 PNG m_lastThumbnailData format (preserving the exact format
  // the previous stub produced) so PreparePage.qml:3154 (lastThumbnailData)
  // and onThumbnailCaptured (PreparePage.qml:3081) keep working unchanged.
  // Phase 156 (CLOS-03): plateIndex is now forwarded through
  // thumbnailCapturedForPlate so the QML consumer can route captured bytes
  // back into PartPlate::setThumbnail for non-current plates (the gap that
  // forced Phase 151 to ship persisted-only). The legacy no-arg signal stays
  // for back-compat with existing lastThumbnailData bindings.
  if (image.isNull())
    return;
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  m_lastThumbnailData = QStringLiteral("data:image/png;base64,")
                        + QString::fromLatin1(bytes.toBase64());
  emit thumbnailCaptured();
  emit thumbnailCapturedForPlate(plateIndex, m_lastThumbnailData, variant);
}

// ── v5.16 (NAVIGATOR): bottom-left 3D navigator cube ──
// Upstream GLCanvas3D::_render_3d_navigator (GLCanvas3D.cpp:5669-5733) hosts
// ImGuizmo::ViewManipulate (ImGuizmo.cpp:2779-3140): a real 3D cube pinned to
// the canvas bottom-left corner whose orientation tracks the camera. Presses
// and drags on the cube are consumed before any scene interaction (upstream
// ImGui handles the ImGuizmo hit before GLCanvas3D::on_mouse), face/edge/
// corner clicks snap the view through a 40-frame interpolation, and cube
// drags rotate the camera with a horizon clamp.

// v5.16 (NAVIGATOR): label anchors are computed from navigatorRect(), which
// depends on the laid-out item height. QML binds them before layout completes
// (height is still the pre-layout value); the label model binding in
// NavigatorLabels.qml reads the viewport size first, so the QQuickItem
// width/height notify refreshes the anchors when layout settles. Camera
// rotation and hover re-emit navigatorLabelsChanged() directly below.

void RhiViewport::setNavigatorEnabled(bool enabled)
{
  if (m_navigatorEnabled == enabled)
    return;
  m_navigatorEnabled = enabled;
  if (!enabled) {
    m_navigatorHoverBox = -1;
    m_navigatorHoverFaceNormal = {};
    if (m_navigatorSnapTimer)
      m_navigatorSnapTimer->stop();
    m_navigatorSnapFramesLeft = 0;
    m_navigatorPressActive = false;
    m_navigatorDragging = false;
  }
  emit navigatorEnabledChanged();
  emit navigatorLabelsChanged();
  update();
}

NavigatorCube::RectF RhiViewport::navigatorRect() const
{
  const float size = NavigatorCube::kRectSizePx;
  return NavigatorCube::RectF{0.0f, float(height()) - size - float(m_navigatorBottomOffset),
                              size, size};
}

QString RhiViewport::navigatorFaceName(const QVector3D &normal)
{
  // ImGuizmo face labels (ImGuizmo.cpp:5686-5691 upstream in
  // GLCanvas3D::_render_3d_navigator): Front/Back/Top/Bottom/Left/Right in the
  // cube frame (X right, Y up, Z front). Returned untranslated so QML owns
  // qsTr().
  if (normal.y() > 0.5f) return QStringLiteral("top");
  if (normal.y() < -0.5f) return QStringLiteral("bottom");
  if (normal.x() > 0.5f) return QStringLiteral("right");
  if (normal.x() < -0.5f) return QStringLiteral("left");
  if (normal.z() > 0.5f) return QStringLiteral("front");
  if (normal.z() < -0.5f) return QStringLiteral("back");
  return QString();
}

QVariantList RhiViewport::navigatorLabels() const
{
  QVariantList labels;
  if (!m_navigatorEnabled)
    return labels;
  const NavigatorCube::CameraBasis basis =
      NavigatorCube::cameraBasis(m_camera.viewMatrix());
  const NavigatorCube::RectF rect = navigatorRect();
  const QVector3D origin(-0.5f, -0.5f, -0.5f);
  // vp-6 (AXISLABELS): uppercase slic3r axis names matching the transform
  // widgets (upstream strcpy AxisLabels "Y"/"Z"/"X", GLCanvas3D.cpp:6144-6146).
  // The vectors are Qt scene axes: Qt X = slic3r X, Qt +Y (height) = slic3r Z,
  // Qt -Z = slic3r +Y (AXES-ROT rotation mapping) -- same mapping
  // PrepareSceneData documents for the world axes.
  const struct
  {
    const char *text;
    QVector3D axis;
  } axes[3] = {
      {"X", QVector3D(1, 0, 0)},
      {"Z", QVector3D(0, 1, 0)},
      // AXES-ROT: slic3r +Y points along scene -Z (rotation mapping).
      {"Y", QVector3D(0, 0, -1)},
  };
  for (const auto &axis : axes) {
    // Axis label at 1.3x the axis direction (ImGuizmo.cpp:3037).
    const QPointF p = NavigatorCube::projectToRect(origin + axis.axis * 1.3f,
                                                   basis, rect);
    labels.append(QVariantMap{
        {"kind", QStringLiteral("axis")},
        {"text", QString::fromLatin1(axis.text)},
        {"x", p.x()},
        {"y", p.y()}});
  }
  // Upstream draws the face label on EVERY visible face (unconditionally in
  // the ImGuizmo panel pass, ImGuizmo.cpp:2941-2990 -- the text vertices are
  // warped onto the face quad), not only on hover.
  const QVector3D normals[6] = {
      QVector3D(1, 0, 0), QVector3D(-1, 0, 0), QVector3D(0, 1, 0),
      QVector3D(0, -1, 0), QVector3D(0, 0, 1), QVector3D(0, 0, -1)};
  for (const QVector3D &n : normals) {
    if (QVector3D::dotProduct(n, basis.forward) >= 0.0f)
      continue;
    const QString face = navigatorFaceName(n);
    if (face.isEmpty())
      continue;
    const QPointF p = NavigatorCube::projectToRect(n * 0.5f, basis, rect);
    labels.append(QVariantMap{
        {"kind", QStringLiteral("face")},
        {"text", face},
        {"x", p.x()},
        {"y", p.y()}});
  }
  return labels;
}

// P15.9 (PLATEANCHOR): project every plate's top-right corner to item pixels
// so QML can anchor the per-plate identity clusters (upstream render_icons
// draws each plate's icon stack at its own bed position, PartPlate.cpp:972).
// The projection mirrors projectWorldToScreen: cameraMvp(aspect) built from
// the same proj/view matrices the renderer uploads, mapped to item size.
QVariantList RhiViewport::plateAnchors() const
{
  QVariantList anchors;
  const int plateCount = m_plateCount > 0 ? m_plateCount : 0;
  if (plateCount <= 0)
    return anchors;
  const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
  const float aspect = float(viewSize.width()) / float(viewSize.height());
  const QMatrix4x4 mvp = cameraMvp(aspect);
  for (int i = 0; i < plateCount; ++i) {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    PrepareSceneData::plateGridOffset(i, plateCount, m_bedWidth, m_bedDepth,
                                      offsetX, offsetY);
    // Anchor at the plate's top-right corner (slic3r frame): the upstream
    // icon column sits at the bed extents max X just below the plate top
    // edge (calc_vertex_for_icons, PartPlate.cpp:633-641). Qt scene frame:
    // X = bed X, Y = height (ground), Z = bed Y.
    // AXES-ROT: scene Z negates slic3r Y — the plate's slic3r-far (max-Y)
    // edge is the scene min-Z edge, where both the icon column (bed extents
    // max X, far edge) and the plate-name anchor (min X, far edge) live,
    // matching upstream calc_vertex_for_icons / plate_name (PartPlate.cpp:
    // 633-676). m_bedOriginY/m_bedDepth are raw slic3r here.
    const float worldX = m_bedOriginX + offsetX + m_bedWidth;
    const float worldZ = -(m_bedOriginY + offsetY + m_bedDepth);
    // vp-2 (PLATEICONS): second anchor at the plate's top-LEFT corner for
    // the plate name + pencil rename cluster (upstream calc_vertex_for_
    // plate_name_edit_icon pins the edit icon next to the name at the bed
    // extents min corner, PartPlate.cpp:646-676).
    const float nameWorldX = m_bedOriginX + offsetX;
    const QVector4D clip = mvp * QVector4D(worldX, 0.0f, worldZ, 1.0f);
    const QVector4D nameClip = mvp * QVector4D(nameWorldX, 0.0f, worldZ, 1.0f);
    // Behind the camera (or degenerate w) -> QML hides the cluster.
    const bool visible = clip.w() > 0.0f && !qFuzzyIsNull(clip.w())
        && nameClip.w() > 0.0f && !qFuzzyIsNull(nameClip.w());
    double px = 0.0;
    double py = 0.0;
    double namePx = 0.0;
    double namePy = 0.0;
    if (visible) {
      const QVector3D ndc = clip.toVector3D() / clip.w();
      px = double((ndc.x() * 0.5f + 0.5f) * float(viewSize.width()));
      py = double((1.f - (ndc.y() * 0.5f + 0.5f)) * float(viewSize.height()));
      const QVector3D nameNdc = nameClip.toVector3D() / nameClip.w();
      namePx = double((nameNdc.x() * 0.5f + 0.5f) * float(viewSize.width()));
      namePy = double((1.f - (nameNdc.y() * 0.5f + 0.5f)) * float(viewSize.height()));
    }
    anchors.append(QVariantMap{
        {"plateIndex", i},
        {"x", px},
        {"y", py},
        {"nameX", namePx},
        {"nameY", namePy},
        {"visible", visible}});
  }
  return anchors;
}

// ── PLATEICONS-GL: bed icon atlas + picking (upstream render_icons) ─────────
namespace {
constexpr int kIconAtlasSize = 2048;
constexpr int kIconCell = 128;
// World-space heights (mm at the 200mm reference bed, PartPlate.cpp:61-70):
// icon cells render 16 units tall, the pencil 9, the name text ~10
// (PARTPLATE_PLATENAME_OFFSET_Y).
constexpr float kNameWorldHeight = 11.0f;
// Name text inset from the plate's far-left corner (gap-left, PartPlate.cpp:64).
constexpr float kNameTextInset = 3.0f;
// Pixel size of the name/badge font rasterized into the atlas; the name
// UV window and the name-width metrics both derive from it.
constexpr int kNameFontPixelSize = 72;
// AXES-ROT note: PrepareSceneData::setBed normalizes the QML bed origin into
// the scene frame (m_bedOriginY ends up as the plate's far/min-Z edge,
// PrepareSceneData.cpp:160-169/1250); every plateIconRect call below feeds
// -(m_bedOriginY + m_bedDepth) so the icon math shares that frame.

// ── Q2: atlas row layout ────────────────────────────────────────────────────
// Rows 0-1: the 6 column icon slots (rest/hover). Row 2: two-state extras
// (unlocked lock, changed settings, pencil). Rows kBadgeRow0..: index
// badges, kAtlasCellsPerRow per row. Rows kNameRow0..: plate names, one
// kNameCellSpan-wide block each. The badge cells used to sit in the single
// row 3 with the plate index as the column (IconCellRef{p, 3}) -- the atlas
// is only 2048/128 = 16 cells wide while kMaxPlateCount = 36
// (PartPlateList.h:32), so plate 17+ sampled outside the atlas. Badges now
// flow onto extra rows, and the row budget is a compile-time contract
// between the atlas geometry and kMaxPlateCount.
constexpr int kAtlasCellsPerRow = kIconAtlasSize / kIconCell;
constexpr int kBadgeRow0 = 3;
constexpr int kBadgeRowCount =
    (OWzx::kMaxPlateCount + kAtlasCellsPerRow - 1) / kAtlasCellsPerRow;
constexpr int kNameCellSpan = 4;  // 512px block per plate name
constexpr int kNamesPerRow = kAtlasCellsPerRow / kNameCellSpan;
constexpr int kNameRow0 = kBadgeRow0 + kBadgeRowCount;
constexpr int kNameRowCount =
    (OWzx::kMaxPlateCount + kNamesPerRow - 1) / kNamesPerRow;
// Text inset (px) applied identically when rasterizing a name block and
// when mapping its UV window, so the quad samples exactly the drawn text.
constexpr int kNameCellTextInset = 8;
static_assert(kIconAtlasSize % kIconCell == 0,
              "atlas cells must tile the atlas exactly");
static_assert(kNamesPerRow * kNameCellSpan == kAtlasCellsPerRow,
              "name blocks must tile an atlas row exactly");
static_assert(kNameRow0 + kNameRowCount <= kAtlasCellsPerRow,
              "plate icon atlas row budget overflows for kMaxPlateCount plates "
              "(grow the atlas or shrink the name cells)");

struct IconCellRef
{
  int col = 0;
  int row = 0;
};

// Badge cell of plate `p` (multi-row flow; see the layout block above).
IconCellRef badgeCellRef(int plateIndex)
{
  return IconCellRef{plateIndex % kAtlasCellsPerRow,
                     kBadgeRow0 + plateIndex / kAtlasCellsPerRow};
}

// Name block (row, column) of plate `p` -- the raster painter and the quad
// UV window both resolve through these so the two can never drift.
int nameCellRow(int plateIndex) { return kNameRow0 + plateIndex / kNamesPerRow; }
int nameCellCol(int plateIndex)
{
  return (plateIndex % kNamesPerRow) * kNameCellSpan;
}

// Q2: single source for the plate-name width formula. refreshPlateIconQuads
// (quad build + pencil rect) and plateIconAt (pencil hit test) used to
// duplicate it; a drift would have made the hover rect disagree with the
// rendered pencil. `textPixels` is the raster-font advance, `aspect` its
// ratio to the font size (texture space), `worldWidth` the clamped world
// width actually used on the bed.
struct PlateNameMetrics
{
  int textPixels = 0;
  float aspect = 0.0f;
  float worldWidth = 0.0f;
};

// Display name shared by the atlas raster and the width metrics (upstream
// falls back to "Untitled" for unnamed plates).
QString plateDisplayName(const QVariantMap &plateState)
{
  const QString name = plateState.value(QStringLiteral("name")).toString();
  return name.isEmpty() ? QStringLiteral("Untitled") : name;
}

PlateNameMetrics plateNameMetrics(const QVariantMap &plateState, float bedWidth)
{
  const QString displayName = plateDisplayName(plateState);
  QFont font;
  font.setPixelSize(kNameFontPixelSize);
  const int textPixels = QFontMetrics(font).horizontalAdvance(displayName);
  const float aspect = float(textPixels) / float(kNameFontPixelSize);
  return PlateNameMetrics{textPixels, aspect,
                          qMin(aspect * kNameWorldHeight, bedWidth * 0.35f)};
}

QString iconSvgPath(const char *base)
{
  return QStringLiteral(":/qml/assets/icons/%1.svg").arg(QLatin1String(base));
}

void rasterSvgCell(QImage &atlas, int col, int row, const QString &path)
{
  QSvgRenderer svg(path);
  const QRect cell(col * kIconCell, row * kIconCell, kIconCell, kIconCell);
  if (!svg.isValid()) {
    qWarning("[PLATEICONS] missing icon asset: %s", qPrintable(path));
    return;
  }
  // 6px inset so the rounded frame breathes inside the cell.
  const QRect target = cell.adjusted(6, 6, -6, -6);
  QPainter painter(&atlas);
  svg.render(&painter, target);
  painter.end();
}
}  // namespace

void RhiViewport::setPlateIconStates(const QVariantList &states)
{
  if (states == m_plateIconStates)
    return;
  m_plateIconStates = states;
  rebuildPlateIcons();
  emit plateIconStatesChanged();
}

void RhiViewport::rebuildPlateIcons()
{
  m_plateIconAtlasDirty = true;
  refreshPlateIconQuads();
  // refreshPlateIconQuads early-returns (before buildPlateIconAtlas) when
  // the canvas/plate context is not ready yet; building here as well keeps
  // the atlas non-null so the renderer's first sync pull never latches an
  // empty texture (that empty-first-pull left the column invisible until
  // an unrelated rebuild happened to fire again).
  buildPlateIconAtlas();
}

void RhiViewport::refreshPlateIconQuads()
{
  const int plateCount = m_plateCount;
  m_plateIconQuadBytes.clear();
  m_plateIconQuadCount = 0;
  // Q2: quad-only latch -- a hover swap re-uploads just this batch; the
  // atlas sync latch is set by buildPlateIconAtlas only when it re-rasters.
  m_plateIconQuadSyncDirty = true;
  if (m_canvasType != CanvasView3D || !m_showBed || plateCount <= 0) {
    update();
    return;
  }

  // Atlas cell per visual state (see the raster layout above refreshPlateIconQuads).
  const auto columnCell = [&](int slot, bool hovered) -> IconCellRef {
    return IconCellRef{slot, hovered ? 1 : 0};
  };
  const auto lockCell = [&](bool locked, bool hovered) -> IconCellRef {
    if (locked)
      return IconCellRef{3, hovered ? 1 : 0};   // row1 col3 hover / row0 col3
    return IconCellRef{hovered ? 1 : 0, 2};     // row2: unlocked / unlocked_hover
  };
  const auto settingsCell = [&](bool changed, bool hovered) -> IconCellRef {
    if (changed)
      return IconCellRef{hovered ? 3 : 2, 2};   // row2: changed / changed_hover
    return columnCell(4, hovered);              // plain settings slot
  };

  struct Vert
  {
    float x, y, z, u, v;
  };
  QVector<Vert> verts;
  const float f = PrepareSceneData::plateIconScale(m_bedDepth);
  const float uvStep = float(kIconCell) / float(kIconAtlasSize);

  const auto appendQuad = [&](const PrepareSceneData::PlateIconRect &rect,
                              const IconCellRef &cell) {
    const float u0 = float(cell.col) * uvStep;
    const float v0 = float(cell.row) * uvStep;
    const float u1 = u0 + uvStep;
    const float v1 = v0 + uvStep;
    const float lift = 0.03f;  // above the 0.02 bed texture quad
    verts.append(Vert{rect.x0, lift, rect.z0, u0, v0});
    verts.append(Vert{rect.x1, lift, rect.z0, u1, v0});
    verts.append(Vert{rect.x1, lift, rect.z1, u1, v1});
    verts.append(Vert{rect.x0, lift, rect.z0, u0, v0});
    verts.append(Vert{rect.x1, lift, rect.z1, u1, v1});
    verts.append(Vert{rect.x0, lift, rect.z1, u0, v1});
  };

  for (int p = 0; p < plateCount; ++p) {
    const QVariantMap state = m_plateIconStates.value(p).toMap();
    const bool locked = state.value(QStringLiteral("locked")).toBool();
    const bool changed = state.value(QStringLiteral("custom")).toBool();

    for (int icon = 0; icon <= int(PrepareSceneData::PlateIconId::MoveFront); ++icon) {
      IconCellRef cell;
      switch (icon) {
      case int(PrepareSceneData::PlateIconId::Lock):
        cell = lockCell(locked, p == m_hoverPlateIndex && m_hoverPlateIconId == icon);
        break;
      case int(PrepareSceneData::PlateIconId::Settings):
        cell = settingsCell(changed, p == m_hoverPlateIndex && m_hoverPlateIconId == icon);
        break;
      default:
        cell = columnCell(icon, p == m_hoverPlateIndex && m_hoverPlateIconId == icon);
        break;
      }
      appendQuad(PrepareSceneData::plateIconRect(p, plateCount, icon,
                                                 m_bedWidth, m_bedDepth,
                                                 m_bedOriginX,
                                                 -(m_bedOriginY + m_bedDepth)),
                 cell);
    }

    // Rename pencil (hoverable) sits right of the plate-name text, so the
    // name geometry is resolved first (PartPlate.cpp:646-663). The width
    // formula is single-sourced in plateNameMetrics (shared with the
    // plateIconAt pencil hit test).
    const PlateNameMetrics nameMetrics = plateNameMetrics(state, m_bedWidth);

    float plateOffX = 0.0f;
    float plateOffY = 0.0f;
    PrepareSceneData::plateGridOffset(p, plateCount, m_bedWidth, m_bedDepth,
                                      plateOffX, plateOffY);
    const float farZ = -(m_bedOriginY + m_bedDepth) + plateOffY;
    const float minX = m_bedOriginX + plateOffX;

    const bool pencilHover = p == m_hoverPlateIndex
                             && m_hoverPlateIconId == int(PrepareSceneData::PlateIconId::NameEdit);
    appendQuad(PrepareSceneData::plateNameEditRect(p, plateCount,
                                                   nameMetrics.worldWidth,
                                                   m_bedWidth, m_bedDepth,
                                                   m_bedOriginX, -(m_bedOriginY + m_bedDepth)),
               IconCellRef{pencilHover ? 5 : 4, 2});

    // Plate-name text quad at the far-left corner; the uv window maps the
    // exact pixel bounds of the drawn text so the aspect holds.
    {
      const float pxU = float(nameCellCol(p) * kIconCell + kNameCellTextInset);
      const float pxV = float(nameCellRow(p) * kIconCell + kNameCellTextInset);
      const float u0 = pxU / float(kIconAtlasSize);
      const float v0 = pxV / float(kIconAtlasSize);
      const float u1 = u0 + float(nameMetrics.textPixels) / float(kIconAtlasSize);
      const float v1 = v0 + float(kNameFontPixelSize) / float(kIconAtlasSize);
      const float worldH = kNameWorldHeight;
      const float worldW = nameMetrics.aspect * worldH;
      const float lift = 0.03f;
      const float x0 = minX + kNameTextInset * f;
      const float z0 = farZ + kNameTextInset * f;
      verts.append(Vert{x0, lift, z0, u0, v0});
      verts.append(Vert{x0 + worldW, lift, z0, u1, v0});
      verts.append(Vert{x0 + worldW, lift, z0 + worldH, u1, v1});
      verts.append(Vert{x0, lift, z0, u0, v0});
      verts.append(Vert{x0 + worldW, lift, z0 + worldH, u1, v1});
      verts.append(Vert{x0, lift, z0 + worldH, u0, v1});
    }

    // Index badge at the near-right corner (not pickable upstream). Badge
    // cells flow multi-row (Q2): kMaxPlateCount exceeds one 16-cell atlas
    // row, and badgeCellRef keeps the quad UV inside the rasterized cells.
    appendQuad(PrepareSceneData::plateIndexBadgeRect(p, plateCount, m_bedWidth,
                                                     m_bedDepth, m_bedOriginX,
                                                     -(m_bedOriginY + m_bedDepth)),
               badgeCellRef(p));
  }

  m_plateIconQuadBytes.resize(verts.size() * int(sizeof(Vert)));
  if (!verts.isEmpty())
    std::memcpy(m_plateIconQuadBytes.data(), verts.constData(),
                m_plateIconQuadBytes.size());
  m_plateIconQuadCount = quint32(verts.size());
  buildPlateIconAtlas();
  update();
}

void RhiViewport::buildPlateIconAtlas()
{
  if (!m_plateIconAtlasDirty && !m_plateIconAtlas.isNull())
    return;
  m_plateIconAtlasDirty = false;
  // Q2: only a real re-raster latches the atlas sync pull -- a quad-only
  // hover refresh never re-stages the 2048x2048 image for synchronize().
  m_plateIconAtlasSyncDirty = true;
  m_plateIconAtlas = QImage(kIconAtlasSize, kIconAtlasSize,
                            QImage::Format_RGBA8888);
  m_plateIconAtlas.fill(Qt::transparent);

  // Row 0: column icons, rest state; row 1: the same slots hovered.
  const char *columnIcons[6] = {
      "plate_close_dark",       "plate_orient_dark",   "plate_arrange_dark",
      "plate_locked_dark",      "plate_settings_dark", "plate_move_front_dark"};
  const char *columnIconsHover[6] = {
      "plate_close_hover_dark", "plate_orient_hover_dark",
      "plate_arrange_hover_dark", "plate_locked_hover_dark",
      "plate_settings_hover_dark", "plate_move_front_hover_dark"};
  for (int slot = 0; slot < 6; ++slot) {
    rasterSvgCell(m_plateIconAtlas, slot, 0, iconSvgPath(columnIcons[slot]));
    rasterSvgCell(m_plateIconAtlas, slot, 1, iconSvgPath(columnIconsHover[slot]));
  }
  // Row 2: two-state extras (unlocked lock, changed settings, pencil).
  rasterSvgCell(m_plateIconAtlas, 0, 2, iconSvgPath("plate_unlocked_dark"));
  rasterSvgCell(m_plateIconAtlas, 1, 2, iconSvgPath("plate_unlocked_hover_dark"));
  rasterSvgCell(m_plateIconAtlas, 2, 2, iconSvgPath("plate_settings_changed_dark"));
  rasterSvgCell(m_plateIconAtlas, 3, 2, iconSvgPath("plate_settings_changed_hover_dark"));
  rasterSvgCell(m_plateIconAtlas, 4, 2, iconSvgPath("plate_name_edit_dark"));
  rasterSvgCell(m_plateIconAtlas, 5, 2, iconSvgPath("plate_name_edit_hover_dark"));

  for (int p = 0; p < m_plateCount; ++p) {
    const QVariantMap state = m_plateIconStates.value(p).toMap();
    const QString displayName = plateDisplayName(state);

    // Index badge "01" (multi-row flow -- see the atlas layout block for
    // why one cell per plate cannot stay in a single row).
    const IconCellRef badge = badgeCellRef(p);
    const QString idx = QStringLiteral("%1").arg(p + 1, 2, 10, QLatin1Char('0'));
    QPainter idxPainter(&m_plateIconAtlas);
    QFont idxFont;
    idxFont.setBold(true);
    idxFont.setPixelSize(kNameFontPixelSize);
    idxPainter.setFont(idxFont);
    idxPainter.setPen(QColor(QStringLiteral("#18c75e")));
    idxPainter.drawText(QRect(badge.col * kIconCell, badge.row * kIconCell,
                              kIconCell, kIconCell),
                        Qt::AlignCenter, idx);
    idxPainter.end();

    // Plate name (kNameCellSpan-wide block per plate, rows below the
    // badges; same cell helpers the quad UV window resolves through).
    QPainter namePainter(&m_plateIconAtlas);
    QFont nameFont;
    nameFont.setPixelSize(kNameFontPixelSize);
    namePainter.setFont(nameFont);
    namePainter.setPen(QColor(QStringLiteral("#18c75e")));
    namePainter.drawText(QRect(nameCellCol(p) * kIconCell + kNameCellTextInset,
                               nameCellRow(p) * kIconCell + kNameCellTextInset,
                               kNameCellSpan * kIconCell - 2 * kNameCellTextInset,
                               kIconCell - 2 * kNameCellTextInset),
                         Qt::AlignLeft | Qt::AlignVCenter, displayName);
    namePainter.end();
  }
}

bool RhiViewport::plateIconAt(const QPointF &position, int &plateIndex,
                              int &iconId) const
{
  plateIndex = -1;
  iconId = -1;
  if (m_canvasType != CanvasView3D || !m_showBed || m_plateCount <= 0)
    return false;
  const float aspect = height() > 0 ? float(width()) / float(height()) : 1.0f;
  QVector3D hit;
  if (!CameraController::groundPointOnPlane(cameraMvp(aspect),
                                            QSizeF(width(), height()),
                                            position, &hit))
    return false;
  for (int p = 0; p < m_plateCount; ++p) {
    for (int icon = 0; icon <= int(PrepareSceneData::PlateIconId::NameEdit); ++icon) {
      if (icon == int(PrepareSceneData::PlateIconId::NameEdit)) {
        // The pencil rect depends on the rendered name width; resolve the
        // same single-source metrics used when building the quads.
        const QVariantMap state = m_plateIconStates.value(p).toMap();
        const PlateNameMetrics nameMetrics = plateNameMetrics(state, m_bedWidth);
        const PrepareSceneData::PlateIconRect pencil =
            PrepareSceneData::plateNameEditRect(p, m_plateCount,
                                                nameMetrics.worldWidth,
                                                m_bedWidth, m_bedDepth,
                                                m_bedOriginX, -(m_bedOriginY + m_bedDepth));
        if (hit.x() >= pencil.x0 && hit.x() <= pencil.x1
            && hit.z() >= pencil.z0 && hit.z() <= pencil.z1) {
          plateIndex = p;
          iconId = icon;
          return true;
        }
        continue;
      }
      const PrepareSceneData::PlateIconRect rect = PrepareSceneData::plateIconRect(
          p, m_plateCount, icon, m_bedWidth, m_bedDepth, m_bedOriginX, -(m_bedOriginY + m_bedDepth));
      if (hit.x() >= rect.x0 && hit.x() <= rect.x1
          && hit.z() >= rect.z0 && hit.z() <= rect.z1) {
        plateIndex = p;
        iconId = icon;
        return true;
      }
    }
  }
  return false;
}

void RhiViewport::updatePlateIconHover(const QPointF &position)
{
  int plate = -1;
  int icon = -1;
  const bool hit = plateIconAt(position, plate, icon);
  // Q2: early-exit on "hover TARGET unchanged" (not "still hitting"): a
  // miss normalizes to (-1, -1), so moving the mouse across empty bed no
  // longer rebuilds the quad batch / latches sync-dirty / re-emits hover.
  // Contract locked by PrepareSceneDataTests.
  if (!PrepareSceneData::plateIconHoverTargetChanged(m_hoverPlateIndex,
                                                     m_hoverPlateIconId,
                                                     hit, plate, icon))
    return;
  m_hoverPlateIndex = hit ? plate : -1;
  m_hoverPlateIconId = hit ? icon : -1;
  if (hit) {
    m_hoverPlateIconPx = position;
    m_plateIconHover = QVariantMap{
        {QStringLiteral("plate"), plate},
        {QStringLiteral("icon"), icon},
        {QStringLiteral("x"), position.x()},
        {QStringLiteral("y"), position.y()}};
    setCursor(QCursor(Qt::PointingHandCursor));
  } else {
    m_plateIconHover.clear();
    unsetCursor();
  }
  emit plateIconHoverChanged();
  refreshPlateIconQuads();
}

bool RhiViewport::startNavigatorPress(const QPointF &position)
{
  if (!m_navigatorEnabled)
    return false;
  const NavigatorCube::CameraBasis basis =
      NavigatorCube::cameraBasis(m_camera.viewMatrix());
  const NavigatorCube::Hit hit =
      NavigatorCube::hitTest(position, basis, navigatorRect());
  if (!hit.isValid())
    return false;
  m_navigatorPressActive = true;
  m_navigatorDragging = false;
  m_navigatorPressPos = position;
  m_navigatorLastPos = position;
  return true;
}

void RhiViewport::navigatorDragMove(const QPointF &position)
{
  const QPointF delta = position - m_navigatorLastPos;
  // ImGuizmo cancels the click once the press moves (ImGuizmo.cpp:3060-3069).
  if (!m_navigatorDragging
      && std::hypot(position.x() - m_navigatorPressPos.x(),
                    position.y() - m_navigatorPressPos.y()) > 4.0) {
    m_navigatorDragging = true;
    m_navigatorSnapFramesLeft = 0;
    if (m_navigatorSnapTimer)
      m_navigatorSnapTimer->stop();
  }
  if (m_navigatorDragging) {
    const NavigatorCube::CameraBasis basis =
        NavigatorCube::cameraBasis(m_camera.viewMatrix());
    const QVector3D dir = NavigatorCube::dragRotateClamped(
        basis.forward, basis.right,
        float(delta.x()) * NavigatorCube::kDragRadiansPerPixel,
        float(delta.y()) * NavigatorCube::kDragRadiansPerPixel);
    float azimuth = 0.0f;
    float elevation = 0.0f;
    NavigatorCube::orientationForDirection(dir, azimuth, elevation);
    if (std::isnan(azimuth))
      azimuth = m_camera.azimuth();
    m_camera.setOrientation(azimuth, elevation);
    m_cameraDirty = true;
    update();
    emit navigatorLabelsChanged();
    emit plateAnchorsChanged();
  }
  m_navigatorLastPos = position;
  updateNavigatorHover(position, false);
}

void RhiViewport::finishNavigatorPress()
{
  const bool wasClick = !m_navigatorDragging;
  m_navigatorPressActive = false;
  m_navigatorDragging = false;
  if (!wasClick)
    return;
  // Snap on click release: ImGuizmo applies overBox at release
  // (ImGuizmo.cpp:3070-3078).
  const NavigatorCube::CameraBasis basis =
      NavigatorCube::cameraBasis(m_camera.viewMatrix());
  const NavigatorCube::Hit hit =
      NavigatorCube::hitTest(m_navigatorLastPos, basis, navigatorRect());
  if (hit.isValid())
    startNavigatorSnap(hit.box);
}

void RhiViewport::updateNavigatorHover(const QPointF &position, bool leave)
{
  int box = -1;
  QVector3D faceNormal;
  if (m_navigatorEnabled && !leave) {
    const NavigatorCube::CameraBasis basis =
        NavigatorCube::cameraBasis(m_camera.viewMatrix());
    const NavigatorCube::Hit hit =
        NavigatorCube::hitTest(position, basis, navigatorRect());
    box = hit.box;
    faceNormal = hit.faceNormal;
  }
  if (box != m_navigatorHoverBox) {
    m_navigatorHoverBox = box;
    m_navigatorHoverFaceNormal = faceNormal;
    emit navigatorLabelsChanged();
    update();
  }
}

void RhiViewport::startNavigatorSnap(int box)
{
  const QVector3D target = NavigatorCube::snapDirectionForBox(box);
  if (target.isNull())
    return;
  m_navigatorSnapTarget = target;
  m_navigatorSnapDir =
      NavigatorCube::cameraBasis(m_camera.viewMatrix()).forward;
  m_navigatorSnapFramesLeft = NavigatorCube::kSnapFrameCount;
  if (m_navigatorSnapTimer == nullptr) {
    m_navigatorSnapTimer = new QTimer(this);
    m_navigatorSnapTimer->setInterval(16);
    connect(m_navigatorSnapTimer, &QTimer::timeout, this,
            &RhiViewport::advanceNavigatorSnap);
  }
  m_navigatorSnapTimer->start();
}

void RhiViewport::advanceNavigatorSnap()
{
  if (m_navigatorSnapFramesLeft <= 0) {
    if (m_navigatorSnapTimer)
      m_navigatorSnapTimer->stop();
    return;
  }
  m_navigatorSnapFramesLeft--;
  // ImGuizmo.cpp:3042-3048: the direction lerps 0.2 toward the target each
  // frame (their up-vector lerp result is immediately overwritten, so the
  // up snaps -- mirrored by writing az/el directly).
  m_navigatorSnapDir += (m_navigatorSnapTarget - m_navigatorSnapDir)
                        * NavigatorCube::kSnapDirLerp;
  m_navigatorSnapDir.normalize();
  float azimuth = 0.0f;
  float elevation = 0.0f;
  NavigatorCube::orientationForDirection(m_navigatorSnapDir, azimuth,
                                         elevation);
  if (std::isnan(azimuth))
    azimuth = m_camera.azimuth();
  m_camera.setOrientation(azimuth, elevation);
  m_cameraDirty = true;
  update();
  emit navigatorLabelsChanged();
  emit plateAnchorsChanged();
}

void RhiViewport::mousePressEvent(QMouseEvent *event)
{
  // CANVAS-FOCUS (upstream GLCanvas mouse-down grabs focus): a press on the
  // canvas makes it the focused item again so the page key table revives
  // after a sidebar control took focus. QQuickItem does not do this by
  // default, unlike the wx canvas.
  forceActiveFocus();
  // v5.16 (NAVIGATOR): the bottom-left cube consumes left presses before any
  // scene interaction (upstream ImGui processes the ImGuizmo hit first).
  if (event->button() == Qt::LeftButton
      && startNavigatorPress(event->position())) {
    event->accept();
    return;
  }
  // PLATEICONS-GL: a left click over a bed icon activates it before any
  // scene interaction (upstream picking dispatches the icon action on
  // LeftDown, render_icons picking ids).
  if (event->button() == Qt::LeftButton) {
    int iconPlate = -1;
    int iconId = -1;
    if (plateIconAt(event->position(), iconPlate, iconId)) {
      event->accept();
      emit plateIconActivated(iconPlate, iconId);
      return;
    }
  }
  if (event->button() == Qt::RightButton) {
    // P11 (upstream GLGizmoPainterBase.cpp:649 RightDown): with a paint gizmo
    // active the right button paints instead of opening the context menu.
    // MMU disables the right button entirely (upstream
    // GLGizmoMmuSegmentation.hpp:94 returns -1 and the event is dropped).
    if ((m_gizmoMode == GizmoSupportPaint ||
         m_gizmoMode == GizmoSeamPaint ||
         m_gizmoMode == GizmoMmuSegmentation) &&
        m_gizmoMode != GizmoMmuSegmentation) {
      m_dragButton = Qt::RightButton;
      m_paintButton = 2;
      m_paintPressPosition = event->position();
      updateBrushCursorState(event->position(), 2 /*right*/);
      m_paintClickPress = true;
      emitPaintPickIfActive(event->position(), event->modifiers());
      m_paintClickPress = false;
      m_contextPressActive = false;
      event->accept();
      return;
    }
    m_contextPressPosition = event->position();
    m_contextPressActive = true;
    m_contextDragExceeded = false;
    m_contextToolCapturedAtPress = activeToolCapturesContextGesture();
    m_contextLayerEditingAtPress = m_layerEditingInputActive;
    // CAM-PARITY: right drag pans the camera (upstream default
    // right_mouse_drag_action = Pan, AppConfig.cpp:248-249); the camera move
    // latches m_contextDragExceeded so the menu stays closed, mirroring the
    // upstream ignore_right_up latch. Sub-threshold releases keep the menu.
    // Tools that capture the right button (layer editing / gizmo context
    // capture) keep their gesture instead.
    m_rightDragPan = !m_contextToolCapturedAtPress && !m_contextLayerEditingAtPress;
    m_pressPosition = event->position();
    m_lastMousePosition = event->position();
    event->accept();
    return;
  }

  m_lastMousePosition = event->position();
  m_pressPosition = event->position();
  m_dragButton = event->button();
  m_pressPickedSourceObjectIndex = -1;
  // SEL-MODIFIERS-RECT: press-time modifiers drive the release routing
  // (upstream reads the live key state at LeftDown, GLCanvas3D.cpp:4135-4142).
  m_pressModifiers = event->modifiers();
  m_pressPickedVolumeIndex = -1;
  m_rectSelectActive = false;
  // Upstream GLCanvas3D.cpp:4310-4317: Ctrl+drag orbits around the canvas
  // center unprojected onto the bed plane (m_rotation_center), cached once
  // per press. Shift is excluded: Shift+left belongs to the rectangle
  // selection drag (GLCanvas3D.cpp:4136-4145), not the orbit pivot.
  m_ctrlRotationCenterActive = false;
  if (event->button() == Qt::LeftButton
      && (event->modifiers() & Qt::ControlModifier)
      && !(event->modifiers() & Qt::ShiftModifier)) {
    QVector3D hit;
    const float aspect = height() > 0 ? float(width()) / float(height()) : 1.0f;
    if (CameraController::groundPointOnPlane(
            cameraMvp(aspect), QSizeF(width(), height()),
            QPointF(width() * 0.5, height() * 0.5), &hit)) {
      // Upstream pivots on _mouse_to_3d(screen_center)
      // (GLCanvas3D.cpp:4583, raycast/bed fallback). The Qt6 bed-plane pick
      // lands far outside the scene at low elevation and an unbounded pivot
      // sweeps the orbit target away, so the pivot is bounded to the same 3x
      // scene box the camera target uses (Camera.cpp validate_target).
      m_ctrlRotationCenterActive = true;
      m_ctrlRotationCenter = m_camera.clampTarget(hit);
    }
  }
  if (m_gizmoDragging)
    emit gizmoDragEnd();
  resetGizmoDragState();

  // Phase 115 (MEASURE-04): the measure gizmo does not drag the object -- a
  // left click drives the two-click measure flow (first click sets A, second
  // sets B). Emit measurePickRequested so the ViewModel runs the stage-2
  // pick + getFeature + the readout math. Mirrors upstream GLGizmoMeasure
  // gizmo_event handling LeftDown (the upstream two-click measure flow).
  if (event->button() == Qt::LeftButton && m_gizmoMode == GizmoMeasure) {
    emitMeasurePickIfActive(event->position(), event->modifiers());
    event->accept();
    return;
  }

  // Phase HOLLOW: a left click in the hollow gizmo places a drain hole at
  // the mesh surface under the cursor (对齐上游 GLGizmoHollow::on_mouse
  // LeftDown → unproject_on_mesh). Emit hollowPickRequested so the ViewModel
  // runs the stage-2 SceneRaycaster pick + appendObjectDrainHole.
  if (event->button() == Qt::LeftButton && m_gizmoMode == GizmoHollow) {
    emitHollowPickIfActive(event->position(), event->modifiers());
    event->accept();
    return;
  }

  // v5.13: AdvancedCut connector-pin placement (VM guards the toggle).
  if (event->button() == Qt::LeftButton && m_gizmoMode == GizmoAdvancedCut) {
    emitAdvancedCutPickIfActive(event->position(), event->modifiers());
    event->accept();
    return;
  }

  // Phase 240 (GIZ-04): interactive cut-plane grab. The plane body drags the
  // cut position along the axis; the border grabbers tilt the plane normal
  // (upstream GLGizmoCut3D use_grabbers, GLGizmoCut.cpp:271-345).
  if (event->button() == Qt::LeftButton &&
      (m_gizmoMode == GizmoCut || m_gizmoMode == GizmoAdvancedCut))
  {
    const int grab = pickCutPlaneAt(event->position());
    if (grab != 0)
    {
      m_cutPlaneGrab = grab;
      m_dragButton = Qt::LeftButton;
      const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
      const float aspect = float(viewSize.width()) / float(viewSize.height());
      const QVector3D center = cutPlaneCenter();
      const QVector3D axisDir = (m_cutAxis == 0) ? QVector3D(1.f, 0.f, 0.f)
                                 : (m_cutAxis == 1) ? QVector3D(0.f, 1.f, 0.f)
                                                    : QVector3D(0.f, 0.f, 1.f);
      auto [orig, dir] = GizmoMath::computeRay(
          float(event->position().x()), float(event->position().y()),
          viewSize, m_camera.projMatrix(aspect), m_camera.viewMatrix());
      if (grab == 1)
        m_cutPlaneDragStartT = GizmoMath::rayToAxisT(orig, dir, axisDir, center);
      else
        m_cutPlaneRotateStartAngle = GizmoMath::computeRotateAngle(
            float(event->position().x()), float(event->position().y()),
            /*axis=*/grab - 1 /* 2->X(1), 3->Y(2) */, viewSize,
            m_camera.projMatrix(aspect), m_camera.viewMatrix(), center,
            m_cutPlaneRotateStartAngle);
      event->accept();
      return;
    }
  }

  // Phase 240 (GIZ-03): flatten face pick. A left click on the mesh rotates
  // the object so the picked facet's normal faces down (upstream
  // GLGizmoFlatten::on_mouse LeftDown, GLGizmoFlatten.cpp:22-38).
  if (event->button() == Qt::LeftButton && m_gizmoMode == GizmoFlatten) {
    emitFlattenPickIfActive(event->position(), /*click=*/true);
    event->accept();
    return;
  }

  // Phase 120 (PAINT-01): a paint-gizmo left click drives the TriangleSelector
  // brush. Emit paintPickRequested so the ViewModel runs the stage-2 pick +
  // PaintEngine::paintAt (select_patch). Mirrors upstream
  // GLGizmoPainterBase gizmo_event handling LeftDown. Phase 121 (PAINT-03) also
  // sets the brush-cursor button state to left so renderBrushCursor paints the
  // cursor blue while painting.
  if (event->button() == Qt::LeftButton &&
      (m_gizmoMode == GizmoSupportPaint ||
       m_gizmoMode == GizmoSeamPaint ||
       m_gizmoMode == GizmoMmuSegmentation)) {
    updateBrushCursorState(event->position(), 1 /*left*/);
    // P11.B2.3 (upstream m_last_mouse_click, GLGizmoPainterBase.cpp:715):
    // remember the press point so an axis-locked drag can clamp to it.
    m_paintPressPosition = event->position();
    m_paintButton = 1;
    // Phase 240 (GIZ-02): smart fill fires on PRESS only (a shift-drag keeps
    // the erase-brush path).
    m_paintClickPress = true;
    emitPaintPickIfActive(event->position(), event->modifiers());
    m_paintClickPress = false;
    event->accept();
    return;
  }

  // Phase 69/70: active gizmo hit tests take priority over object picking.
  // P15.11 (MULTICENTER): a non-empty multi-selection list also hosts the
  // gizmo (the drawn gizmo sits at the union center in that case).
  if (event->button() == Qt::LeftButton &&
      (m_gizmoMode == GizmoMove || m_gizmoMode == GizmoRotate || m_gizmoMode == GizmoScale) &&
      (m_selectedSourceObjectIndex >= 0 || !m_selectedSourceObjectIndices.isEmpty()))
  {
    const int axis = pickGizmoAxisAt(event->position());
    if (axis != 0)
    {
      m_gizmoAxis = axis;
      m_gizmoDragMode = m_gizmoMode;
      m_gizmoDragging = true;
      m_gizmoDragCenter = currentGizmoCenter();
      const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
      const float aspect = float(viewSize.width()) / float(viewSize.height());
      auto [orig, dir] = GizmoMath::computeRay(
          float(event->position().x()), float(event->position().y()),
          viewSize,
          m_camera.projMatrix(aspect), m_camera.viewMatrix());
      if (m_gizmoDragMode == GizmoRotate)
      {
        m_gizmoRotateStartAngle = GizmoMath::computeRotateAngle(
            float(event->position().x()), float(event->position().y()), axis,
            viewSize, m_camera.projMatrix(aspect), m_camera.viewMatrix(),
            m_gizmoDragCenter, m_gizmoRotateStartAngle);
      }
      else
      {
        m_gizmoDragStartT = GizmoMath::rayToAxisT(orig, dir, kGizmoAxes[axis - 1],
                                                  m_gizmoDragCenter);
      }
      emit gizmoDragBegin();
      event->accept();
      return;
    }
  }

  if (event->button() == Qt::LeftButton) {
    // SEL-MODIFIERS-RECT (upstream GLCanvas3D.cpp:4136-4145): Shift+left
    // starts the rectangle selection drag instead of a single pick. Alt turns
    // the stroke into a deselect rectangle (GLCanvas3D.cpp:8676 "alt_pressed
    // = deselect by rectangle"; applied through GLSelectionRectangle::EState::
    // Deselect, GLCanvas3D.cpp:9588-9589). Active gizmos never reach here
    // (their branches above accept the press first), mirroring the upstream
    // gizmo-consumed guard.
    if (event->modifiers() & Qt::ShiftModifier) {
      m_rectSelectActive = true;
      m_rectSelectStart = event->position();
      updateSelectionRubberBand(event->position());
      event->accept();
      return;
    }
    m_pressPickedSourceObjectIndex = pickSourceObjectAt(event->position());
    // SEL-MODIFIERS-RECT (GLCanvas3D.cpp:4135/4152-4168): Ctrl+click toggles
    // the object, Alt+click picks the volume (part) under the cursor -- both
    // need the full hit with the volume index, not just the source index.
    if (m_pressPickedSourceObjectIndex >= 0
        && (event->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
      const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
      const float aspect = float(viewSize.width()) / float(viewSize.height());
      auto [rayOrigin, rayDirection] = GizmoMath::computeRay(
          float(event->position().x()), float(event->position().y()),
          viewSize,
          m_camera.projMatrix(aspect),
          m_camera.viewMatrix());
      // PICK-BVH: BVH-backed pick over the picking scene
      // (PrepareSceneData::pickingRaycaster).
      const ObjectPicking::Hit hit = m_pickScene.pickingRaycaster()->pick(
          rayOrigin, rayDirection,
          m_pickScene.modelVertices(), m_pickScene.modelBatches());
      if (hit.isValid())
        m_pressPickedVolumeIndex = hit.volumeIndex;
    }
    // CAM-PARITY: a plain left press on an object arms the bed drag-move
    // (upstream GLCanvas3D.cpp:4508-4526 arms m_mouse.drag.move_volume_idx
    // when m_moving_enabled and the press lands inside a volume bbox with no
    // Ctrl; the Dragging branch :4531 then moves the object instead of
    // orbiting). Ctrl keeps the canvas-center orbit pivot, Alt keeps the
    // part-pick gesture, and Shift never reaches here (rectangle selection).
    // Layer editing owns the left drag while active, mirroring the upstream
    // layers-editing branch ordering.
    m_bedDragCandidate = m_pressPickedSourceObjectIndex >= 0
        && !m_ctrlRotationCenterActive
        && !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier))
        && !m_layerEditingInputActive;
    m_bedDragActive = false;
    m_bedDragSourceIndex = m_bedDragCandidate ? m_pressPickedSourceObjectIndex : -1;
    m_bedDragLastWorldValid = false;
    setHoveredSourceObjectIndex(m_pressPickedSourceObjectIndex);
  }
  event->accept();
}

void RhiViewport::mouseMoveEvent(QMouseEvent *event)
{
  // v5.16 (NAVIGATOR): an active cube press drags/rotates the camera through
  // the navigator and never reaches the scene handlers.
  if (m_navigatorPressActive) {
    navigatorDragMove(event->position());
    event->accept();
    return;
  }
  if (m_contextPressActive) {
    const QPointF delta = event->position() - m_contextPressPosition;
    m_contextDragExceeded = m_contextDragExceeded || std::hypot(delta.x(), delta.y()) > 4.0;
    // CAM-PARITY: right-drag pans (upstream is_camera_pan branch,
    // GLCanvas3D.cpp:4692-4721; right_mouse_drag_action defaults to Pan).
    // The pick-based pan unprojects the previous and current cursor at one
    // shared view-plane depth, so it works for any armed press position.
    if (m_rightDragPan)
      applyCameraPan(event->position());
    m_lastMousePosition = event->position();
    event->accept();
    return;
  }
  // Phase 120 (PAINT-01): continuous-paint-on-drag. While a paint gizmo is
  // active and the left button is held, every mouse-move drives the
  // TriangleSelector brush (mirrors upstream GLGizmoPainterBase on_mouse
  // move-while-LeftDown). mousePressEvent already accept()-ed the initial
  // click for paint gizmos, so m_gizmoDragging stays false here.
  if ((m_dragButton == Qt::LeftButton || m_dragButton == Qt::RightButton) &&
      (m_gizmoMode == GizmoSupportPaint ||
       m_gizmoMode == GizmoSeamPaint ||
       m_gizmoMode == GizmoMmuSegmentation)) {
    // P11.B2.3 (upstream GLGizmoPainterBase.cpp:738-742): during a brush
    // drag, Vertical lock clamps the screen X to the press column and
    // Horizontal lock clamps the screen Y to the press row, so the stroke
    // stays a straight line through the click point. The clamp applies to
    // the whole downstream path (cursor + pick ray).
    QPointF paintPos = event->position();
    if (m_paintLockAxis == 1)
      paintPos.setX(m_paintPressPosition.x());
    else if (m_paintLockAxis == 2)
      paintPos.setY(m_paintPressPosition.y());
    updateBrushCursorState(paintPos, m_dragButton == Qt::RightButton ? 2 : 1);
    emitPaintPickIfActive(paintPos, event->modifiers());
    event->accept();
    return;
  }
  // Phase 240 (GIZ-04): active cut-plane drag. Grab 1 moves the plane along
  // the cut axis (position delta from the ray-to-axis projection); grabs 2/3
  // tilt the plane normal around the world X/Y axes (circular drag angle
  // delta, same computeRotateAngle the rotate gizmo uses).
  if (m_cutPlaneGrab != 0 && m_dragButton == Qt::LeftButton)
  {
    const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
    const float aspect = float(std::max(1, viewSize.width())) / float(std::max(1, viewSize.height()));
    const QVector3D center = cutPlaneCenter();
    if (center.isNull())
    {
      m_cutPlaneGrab = 0;
      event->accept();
      return;
    }
    const QVector3D axisDir = (m_cutAxis == 0) ? QVector3D(1.f, 0.f, 0.f)
                           : (m_cutAxis == 1) ? QVector3D(0.f, 1.f, 0.f)
                                              : QVector3D(0.f, 0.f, 1.f);
    auto [orig, dir] = GizmoMath::computeRay(
        float(event->position().x()), float(event->position().y()), viewSize,
        m_camera.projMatrix(aspect), m_camera.viewMatrix());
    if (m_cutPlaneGrab == 1)
    {
      const float curT = GizmoMath::rayToAxisT(orig, dir, axisDir, center);
      const float deltaT = curT - m_cutPlaneDragStartT;
      m_cutPlaneDragStartT = curT;
      emit cutPlaneDragRequested(m_cutPosition + deltaT);
    }
    else
    {
      const int rotateAxis = m_cutPlaneGrab - 1; // 2->X(1), 3->Y(2)
      const float currentAngle = GizmoMath::computeRotateAngle(
          float(event->position().x()), float(event->position().y()),
          rotateAxis, viewSize, m_camera.projMatrix(aspect),
          m_camera.viewMatrix(), center, m_cutPlaneRotateStartAngle);
      const float deltaAngle = currentAngle - m_cutPlaneRotateStartAngle;
      m_cutPlaneRotateStartAngle = currentAngle;
      emit cutPlaneRotateRequested(
          rotateAxis,
          float(deltaAngle * 180.0 / M_PI));
    }
    m_lastMousePosition = event->position();
    event->accept();
    return;
  }

  // Phase 69: active gizmo drag translates the selected object along the
  // picked axis. Phase 70 extends the same consumed drag path to rotate/scale.
  if (m_gizmoDragging && m_dragButton == Qt::LeftButton)
  {
    if (m_gizmoAxis < 1 || m_gizmoAxis > 3)
    {
      resetGizmoDragState();
      emit gizmoDragEnd();
      event->accept();
      return;
    }

    const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
    const float aspect = float(std::max(1, viewSize.width())) / float(std::max(1, viewSize.height()));
    auto [orig, dir] = GizmoMath::computeRay(
        float(event->position().x()), float(event->position().y()), viewSize,
        m_camera.projMatrix(aspect), m_camera.viewMatrix());
    if (m_gizmoDragMode == GizmoRotate)
    {
      const float currentAngle = GizmoMath::computeRotateAngle(
          float(event->position().x()), float(event->position().y()),
          m_gizmoAxis, viewSize, m_camera.projMatrix(aspect),
          m_camera.viewMatrix(), m_gizmoDragCenter, m_gizmoRotateStartAngle);
      const float deltaAngle = currentAngle - m_gizmoRotateStartAngle;
      m_gizmoRotateStartAngle = currentAngle;
      emit gizmoRotateRequested(m_gizmoAxis, deltaAngle);
    }
    else
    {
      const QVector3D axisDir = kGizmoAxes[m_gizmoAxis - 1];
      const float curT = GizmoMath::rayToAxisT(orig, dir, axisDir, m_gizmoDragCenter);
      const float worldDeltaT = curT - m_gizmoDragStartT;
      m_gizmoDragStartT = curT;
      if (m_gizmoDragMode == GizmoScale)
      {
        const float factor = std::max(1.0f + worldDeltaT * 0.01f, 0.01f);
        emit gizmoScaleRequested(m_gizmoAxis, factor);
      }
      else
      {
        const QVector3D frameDelta = axisDir * worldDeltaT;
        emit gizmoMoveRequested(frameDelta);
      }
    }
    m_lastMousePosition = event->position();
    event->accept();
    return;
  }

  // SEL-MODIFIERS-RECT (upstream GLCanvas3D.cpp:4255-4260): while the
  // rectangle-selection drag is active, a left-drag only grows the rubber
  // band -- camera orbit and hover picking stay suppressed.
  if (m_rectSelectActive && m_dragButton == Qt::LeftButton) {
    updateSelectionRubberBand(event->position());
    m_lastMousePosition = event->position();
    event->accept();
    return;
  }

  const QPointF delta = event->position() - m_lastMousePosition;
  // v5.12 gap-closure: cameraNavStyle Touchpad (1) routes navigation through
  // the keyboard modifiers exactly like upstream (is_camera_rotate /
  // is_camera_pan, GLCanvas3D.cpp:4940-4957): Alt+move rotates, Shift+move
  // pans, unmapped buttons do nothing. Shift+left keeps the rectangle
  // selection, whose own branch above already consumed the stroke. Default
  // (0): Left=orbit, Middle=pan; right pans inside the context-gesture branch
  // above (upstream AppConfig.cpp:242-249 mapping).
  const bool touchpad = (m_cameraNavStyle == 1);
  const bool touchpadRotate = touchpad && (event->modifiers() & Qt::AltModifier);
  const bool touchpadPan = touchpad && (event->modifiers() & Qt::ShiftModifier);
  if (m_dragButton == Qt::LeftButton && m_bedDragCandidate) {
    // CAM-PARITY: the press landed on an object without Ctrl/Alt/Shift -- a
    // drag moves the object across the bed instead of orbiting the camera
    // (upstream GLCanvas3D.cpp:4531, the move branch precedes the camera
    // branch; :4613 also forbids camera rotation while a volume is hovered).
    const QPointF pressDelta = event->position() - m_pressPosition;
    const bool becameDrag = std::hypot(pressDelta.x(), pressDelta.y()) > 4.0;
    if (m_bedDragActive || becameDrag) {
      if (!m_bedDragActive) {
        // Select the grabbed object first so the move chain (which operates
        // on the primary selection) drives it; a drag past the threshold is
        // not a click, so the release-side click-select stays suppressed.
        if (m_bedDragSourceIndex != m_selectedSourceObjectIndex)
          emit objectPickedSource(m_bedDragSourceIndex);
        m_bedDragActive = true;
        emit bedDragMoveBegin();
      }
      QVector3D groundPoint;
      const float aspect = height() > 0 ? float(width()) / float(height()) : 1.0f;
      if (CameraController::groundPointOnPlane(cameraMvp(aspect),
                                               QSizeF(width(), height()),
                                               event->position(),
                                               &groundPoint)) {
        // Bed-plane (y=0) pick per frame: the delta slides the object
        // horizontally and never lifts it (upstream _mouse_to_3d z=0 pick,
        // GLCanvas3D.cpp:4540-4560).
        if (m_bedDragLastWorldValid)
          emit bedDragMoveRequested(groundPoint - m_bedDragLastWorld);
        m_bedDragLastWorld = groundPoint;
        m_bedDragLastWorldValid = true;
      }
    } else {
      setHoveredSourceObjectIndex(m_pressPickedSourceObjectIndex);
    }
  } else if (m_dragButton == Qt::LeftButton) {
    if (touchpad) {
      if (touchpadRotate)
        applyCameraOrbit(float(delta.x()), float(delta.y()));
      else if (touchpadPan)
        applyCameraPan(event->position());
    } else if (m_pressPickedSourceObjectIndex >= 0) {
      // Modifier presses (Ctrl pivot / Alt part-pick) keep the camera drag:
      // hover tracks until the drag threshold, then the orbit takes over.
      const QPointF pressDelta = event->position() - m_pressPosition;
      const bool becameDrag = std::hypot(pressDelta.x(), pressDelta.y()) > 4.0;
      if (!becameDrag) {
        setHoveredSourceObjectIndex(m_pressPickedSourceObjectIndex);
      } else {
        m_pressPickedSourceObjectIndex = -1;
        setHoveredSourceObjectIndex(pickSourceObjectAt(event->position()));
        // Raw screen deltas, matching upstream (GLCanvas3D.cpp:4622: rot =
        // pos - drag.start, y-DOWN positive; Camera's positive delta_zenit /
        // dElevation raises the eye). Negating y inverted the vertical orbit
        // so dragging "up" dived the camera under the bed, where the +-90
        // zenit clamp parked it -- the bed top face was unreachable.
        applyCameraOrbit(float(delta.x()), float(delta.y()));
      }
    } else {
      // Raw screen deltas, matching upstream (GLCanvas3D.cpp:4622): see the
      // note above on the y-DOWN convention.
      applyCameraOrbit(float(delta.x()), float(delta.y()));
    }
  } else if (m_dragButton == Qt::MiddleButton) {
    if (touchpad) {
      if (touchpadRotate)
        applyCameraOrbit(float(delta.x()), float(delta.y()));
      else if (touchpadPan)
        applyCameraPan(event->position());
    } else {
      applyCameraPan(event->position());
    }
  }
  m_lastMousePosition = event->position();
  event->accept();
}

// P14 (CAM-PARITY): shared constrained/free orbit routing mirroring the
// upstream left-drag block (GLCanvas3D.cpp:4275-4332).
void RhiViewport::applyCameraOrbit(float dxPx, float dyPx)
{
  // Upstream sensitivity (GLCanvas3D.cpp:4276, TRACKBALLSIZE=0.8 at :80):
  // both the constrained sphere orbit and the free trackball use 0.8
  // degrees per pixel.
  if (m_freeCamera) {
    // Upstream GLCanvas3D.cpp:4288-4290: free camera = virtual trackball
    // (rotate_local_around_target), no angle limits.
    const float radiansPerPixel = float(M_PI) * 0.8f / 180.0f;
    m_camera.rotateLocalAroundTarget(dyPx * radiansPerPixel,
                                     dxPx * radiansPerPixel);
  } else {
    // Upstream GLCanvas3D.cpp:4308: recover_from_free_camera before every
    // constrained rotation so the horizon stays level.
    m_camera.recoverFromFreeCamera();
    QVector3D pivot;
    if (m_ctrlRotationCenterActive) {
      // Upstream GLCanvas3D.cpp:4310-4317: Ctrl pivots on the cached
      // canvas-center ground pick.
      pivot = m_ctrlRotationCenter;
    } else {
      // USER-RULING (2026-10-10): the plain orbit always pivots on the
      // CURRENT PLATE CENTER, replacing the upstream selection-pivot
      // (GLCanvas3D.cpp:4661-4662 rotates on the selection bbox center).
      // Selection-following made the bed sweep around a model dragged
      // off-center, which read as a broken rotation axis.
      pivot = m_camera.bedCenter();
    }
    m_camera.orbitAround(dxPx * 0.8f, dyPx * 0.8f, pivot);
  }
  m_cameraDirty = true;
  update();
  emit navigatorLabelsChanged();
  emit plateAnchorsChanged();
}

// P14 (CAM-PARITY): pick-based pan (upstream is_camera_pan block,
// GLCanvas3D.cpp:4692-4721): unproject the PREVIOUS and CURRENT cursor
// positions at ONE shared depth plane and translate the target by the world
// delta so the scene follows the cursor. Upstream unprojects both points at
// window z=0 -- the tight-frustum near plane (Camera.cpp
// calc_tight_frustrum_zs_around), i.e. a plane PERPENDICULAR to the view
// direction at the scene front -- and, crucially, REPLACES
// drag.start_position_2D with the cursor after every pan event
// (GLCanvas3D.cpp:4721): the translation is a per-frame delta. The previous
// Qt6 code anchored the "orig" pick at the press position and never advanced
// it, re-applying the cumulative press->cursor displacement on EVERY move --
// the total translation grew quadratically with the frame count, which read
// as "one small drag sweeps the whole scene". The plane choice sets the pan
// gain: the scene-front plane keeps the delta damped and scene-bounded
// exactly like upstream (the earlier bed-plane y=0 pick exploded at low
// elevation and flew the camera).
void RhiViewport::applyCameraPan(const QPointF &currentPosition)
{
  const float aspect = height() > 0 ? float(width()) / float(height()) : 1.0f;
  const QMatrix4x4 mvp = cameraMvp(aspect);
  const QSizeF vp(width(), height());
  const QVector3D referencePoint = m_camera.viewFrontPoint();
  QVector3D previousPoint;
  QVector3D currentPoint;
  if (CameraController::viewPlanePoint(mvp, vp, m_lastMousePosition,
                                       referencePoint, &previousPoint)
      && CameraController::viewPlanePoint(mvp, vp, currentPosition,
                                          referencePoint, &currentPoint)) {
    m_camera.translateWorld(previousPoint - currentPoint);
    m_cameraDirty = true;
    update();
    // P15.9 (PLATEANCHOR): pan translates the camera target, so the
    // world-anchored plate clusters move with the scene (the navigator cube
    // labels are orientation-only and skip this emit).
    emit plateAnchorsChanged();
  }
}

// P14 (CAM-PARITY): the camera scene box follows the live bed footprint and
// height (upstream Camera::set_scene_box; drives the zoom bounds and the
// target validation in CameraController).
void RhiViewport::updateSceneExtent()
{
  const bool roundBed = m_bedShapeType == 1;
  const float width = roundBed ? m_bedDiameter : m_bedWidth;
  const float depth = roundBed ? m_bedDiameter : m_bedDepth;
  const float height = m_bedHeightToLid > 0.0f ? m_bedHeightToLid : 100.0f;
  // AXES-ROT: the bed rect occupies scene Z in [-(originY+depth), -originY]
  // (raw slic3r originY negates into scene Z).
  m_camera.setSceneExtent(width, depth, height, -(m_bedOriginY + depth));
  m_cameraDirty = true;
}

void RhiViewport::mouseReleaseEvent(QMouseEvent *event)
{
  m_ctrlRotationCenterActive = false;
  // v5.16 (NAVIGATOR): cube release either starts the snap interpolation
  // (click) or ends the drag; the scene handlers never see it.
  if (event->button() == Qt::LeftButton && m_navigatorPressActive) {
    finishNavigatorPress();
    event->accept();
    return;
  }
  if (event->button() == Qt::RightButton) {
    // P11: a right-button paint stroke ends here; it suppresses the context
    // menu (m_contextPressActive was cleared at press) and clears the
    // stroke button so hover painting does not inherit it.
    if (m_paintButton == 2) {
      m_paintButton = 0;
      m_dragButton = Qt::NoButton;
    }
    // CAM-PARITY: a right stroke that panned latches m_contextDragExceeded
    // (4px threshold) so the popup stays closed -- the upstream
    // ignore_right_up behavior after a camera move (GLCanvas3D.cpp:4641).
    m_rightDragPan = false;
    const bool suppress = !m_contextPressActive || m_contextDragExceeded
        || m_contextLayerEditingAtPress || m_contextToolCapturedAtPress;
    m_contextPressActive = false;
    m_contextToolCapturedAtPress = false;
    m_contextLayerEditingAtPress = false;
    if (!suppress) {
      const ViewportContextHit hit = classifyContextAt(event->position());
      if (hit.isMenuRequest()) {
        emit contextMenuRequested(int(hit.target), hit.sourceObjectIndex,
                                  hit.volumeIndex, hit.instanceIndex,
                                  hit.plateIndex, hit.popupPosition.x(),
                                  hit.popupPosition.y());
      }
    }
    event->accept();
    return;
  }
  // Phase 69: end the gizmo drag. The ViewModel coalesces all frame deltas
  // into one undo command.
  if (m_gizmoDragging && event->button() == Qt::LeftButton)
  {
    resetGizmoDragState();
    m_dragButton = Qt::NoButton;
    emit gizmoDragEnd();
    event->accept();
    return;
  }

  // Phase 240 (GIZ-04): end the cut-plane grab (upstream on_stop_dragging
  // takes the "Move/Rotate cut plane" snapshot).
  if (m_cutPlaneGrab != 0 && event->button() == Qt::LeftButton)
  {
    m_cutPlaneGrab = 0;
    m_dragButton = Qt::NoButton;
    event->accept();
    return;
  }

  // SEL-MODIFIERS-RECT (upstream GLCanvas3D.cpp:4384-4391): LeftUp applies
  // the rectangle selection and stops the drag. The containment candidates
  // are resolved here in C++ (projection of the visible pick-scene batches);
  // QML only forwards modifiers + rect + contained sources to
  // EditorViewModel::selectObjectsInRect, which owns the state change.
  if (event->button() == Qt::LeftButton && m_rectSelectActive) {
    m_rectSelectActive = false;
    const QRectF rect = m_selectionRubberBand;
    if (m_selectionRubberBandActive) {
      m_selectionRubberBandActive = false;
      emit selectionRubberBandChanged();
    }
    const QVariantList contained = containedRectangleSelectionSources(rect);
    emit rectangleSelectionFinished(int(m_pressModifiers), rect, contained);
    m_dragButton = Qt::NoButton;
    m_pressPickedSourceObjectIndex = -1;
    m_pressPickedVolumeIndex = -1;
    event->accept();
    return;
  }

  // CAM-PARITY: end the bed drag-move before click routing -- a moved object
  // is not a click, so the click-select below stays suppressed
  // (m_pressPickedSourceObjectIndex cleared); a sub-threshold release keeps
  // the candidate inert and falls through to the normal click select.
  if (event->button() == Qt::LeftButton && m_bedDragActive) {
    emit bedDragMoveEnd();
    m_bedDragActive = false;
    m_bedDragCandidate = false;
    m_bedDragSourceIndex = -1;
    m_bedDragLastWorldValid = false;
    m_pressPickedSourceObjectIndex = -1;
  }
  m_bedDragCandidate = false;
  m_bedDragSourceIndex = -1;

  if (event->button() == Qt::LeftButton && m_pressPickedSourceObjectIndex >= 0) {
    const QPointF releaseDelta = event->position() - m_pressPosition;
    const bool isClick = std::hypot(releaseDelta.x(), releaseDelta.y()) <= 4.0;
    if (isClick && pickSourceObjectAt(event->position()) == m_pressPickedSourceObjectIndex) {
      // SEL-MODIFIERS-RECT: Ctrl/Alt clicks route through the modifier-
      // carrying signal (Ctrl = additive toggle, Alt = part pick); the
      // no-modifier path keeps emitting objectPickedSource unchanged.
      const bool modifierClick = (m_pressModifiers & (Qt::ControlModifier | Qt::AltModifier)) != 0;
      if (modifierClick)
        emit objectPickedSourceWithModifiers(m_pressPickedSourceObjectIndex,
                                             m_pressPickedVolumeIndex,
                                             int(m_pressModifiers));
      else
        emit objectPickedSource(m_pressPickedSourceObjectIndex);
    }
  } else if (event->button() == Qt::LeftButton && m_pressPickedSourceObjectIndex < 0) {
    // Upstream GLCanvas3D.cpp LeftUp empty-canvas branch: a left click that
    // pressed and released on neither a volume nor a plate deselects all.
    // Guards mirrored from upstream: not while layer editing; Shift belongs
    // to the rectangle selection (handled in its own branch); with a gizmo
    // active a Ctrl+click keeps the selection, a plain click on empty canvas
    // closes it via deselect_all. The -1 pick forwards to
    // EditorViewModel::clearObjectSelection (PreparePage).
    const QPointF releaseDelta = event->position() - m_pressPosition;
    const bool isClick = std::hypot(releaseDelta.x(), releaseDelta.y()) <= 4.0;
    const bool ctrlHeld = (m_pressModifiers & Qt::ControlModifier) != 0;
    const bool gizmoGuardsSelection = ctrlHeld && m_selectedSourceObjectIndex >= 0;
    if (isClick && !m_layerEditingInputActive && !gizmoGuardsSelection)
      emit objectPickedSource(-1);
  }
  m_dragButton = Qt::NoButton;
  m_paintButton = 0;
  m_pressPickedSourceObjectIndex = -1;
  m_pressPickedVolumeIndex = -1;
  // Phase 121 (PAINT-03): release returns the brush cursor to hover (black).
  if (m_gizmoMode == GizmoSupportPaint ||
      m_gizmoMode == GizmoSeamPaint ||
      m_gizmoMode == GizmoMmuSegmentation)
    updateBrushCursorState(event->position(), 0 /*hover*/);
  event->accept();
}

bool RhiViewport::activeToolCapturesContextGesture() const
{
  return m_contextToolInputCaptured || m_gizmoDragging;
}

void RhiViewport::mouseUngrabEvent()
{
  // A stolen grab (popup opening, item hiding, window losing the pointer)
  // ends event delivery without a release. Leaving the latched drag state
  // would keep the camera orbit/pan (or an active tool) driving from stale
  // positions on the next interaction -- the intermittent fly-away class of
  // bugs. Reset every press-scoped latch the release paths clear.
  if (m_gizmoDragging) {
    resetGizmoDragState();
    emit gizmoDragEnd();
  }
  m_dragButton = Qt::NoButton;
  m_paintButton = 0;
  m_navigatorPressActive = false;
  m_contextPressActive = false;
  m_contextToolCapturedAtPress = false;
  m_contextLayerEditingAtPress = false;
  m_ctrlRotationCenterActive = false;
  m_cutPlaneGrab = 0;
  m_pressPickedSourceObjectIndex = -1;
  m_pressPickedVolumeIndex = -1;
  if (m_rectSelectActive) {
    m_rectSelectActive = false;
    if (m_selectionRubberBandActive) {
      m_selectionRubberBandActive = false;
      emit selectionRubberBandChanged();
    }
  }
}

void RhiViewport::hoverMoveEvent(QHoverEvent *event)
{
  // v5.16 (NAVIGATOR): cube hover highlight + face label (ImGuizmo hovered
  // face recolor, ImGuizmo.cpp:2923-2925).
  updateNavigatorHover(event->position(), false);
  // PLATEICONS-GL: bed icon hover swap + tooltip payload.
  updatePlateIconHover(event->position());
  setHoveredSourceObjectIndex(pickSourceObjectAt(event->position()));
  // Phase 115 (MEASURE-04): drive the snap UX on mouse-move while the measure
  // gizmo is active. The ViewModel runs the two-stage pick + getFeature and
  // updates the readout live. Mirrors upstream GLGizmoMeasure on_mouse move.
  emitMeasurePickIfActive(event->position(), event->modifiers());
  // Phase 240 (GIZ-03): drive the flatten hover highlight on mouse-move
  // (upstream GLGizmoFlatten hover recolors the hovered plane,
  // GLGizmoFlatten.cpp:107).
  emitFlattenPickIfActive(event->position(), /*click=*/false);
  // Phase 121 (PAINT-03/OV-05): track the brush-cursor position on hover so
  // the sphere cursor follows the mouse before a click (hover -> black cursor).
  updateBrushCursorState(event->position(), 0 /*hover*/);
  event->accept();
}

void RhiViewport::hoverLeaveEvent(QHoverEvent *event)
{
  // v5.16 (NAVIGATOR): clear the cube hover when the cursor leaves.
  updateNavigatorHover(event->position(), true);
  // PLATEICONS-GL: clear the bed-icon hover + tooltip payload.
  updatePlateIconHover(QPointF(-1e6, -1e6));
  setHoveredSourceObjectIndex(-1);
  // Phase 115 (MEASURE-04): clear the hovered feature when the cursor leaves
  // the viewport so no stale highlight lingers off-mesh.
  if (m_gizmoMode == GizmoMeasure)
    emit measureHoverLeft();
  // Phase 240 (GIZ-03): clear the flatten hover highlight when the cursor
  // leaves the viewport (pickedSourceIndex -1 clears it in the ViewModel).
  if (m_gizmoMode == GizmoFlatten)
    emitFlattenPickIfActive(event->position(), /*click=*/false);
  // Phase 121 (PAINT-03): hide the brush cursor when the mouse leaves.
  updateBrushCursorState(event->position(), -1 /*hide*/);
  event->accept();
}

void RhiViewport::wheelEvent(QWheelEvent *event)
{
  // PAINT-CTRL-WHEEL (upstream GLGizmoPainterBase.cpp:584-631): with a paint
  // gizmo active, Ctrl+wheel tunes the brush parameters instead of zooming.
  // Brush cursors step the radius 0.2 within [0.4, 8] (CursorRadius*); the
  // SmartFill tool steps its seed-fill angle 1 deg within [0, 90]
  // (SmartFillAngle*); the GapFill tool steps the gap area 0.2 mm2 within
  // [0, 5] (GapArea*, PAINT-GAPFILL-PORT, GLGizmoPainterBase.cpp:624-627).
  // PAINT-MMU-TOOLS: the MMU tool set routes its own channels in the upstream
  // order -- HeightRange steps the band span 0.2 within [0.1, 8]
  // (CursorHeight*, GLGizmoPainterBase.cpp:584-592, first branch upstream),
  // BucketFill steps the fill angle, GapFill steps the gap area, and the
  // pointer brush tunes nothing (upstream POINTER matches no branch -- the
  // wheel falls through to zoom).
  const bool mmuTool = (m_gizmoMode == GizmoMmuSegmentation);
  const bool mmuPointerBrush =
      mmuTool && m_mmuPaintTool == 0 && m_brushCursorType == 2;
  if (event->modifiers() & Qt::ControlModifier &&
      (m_gizmoMode == GizmoSupportPaint ||
       m_gizmoMode == GizmoSeamPaint ||
       m_gizmoMode == GizmoMmuSegmentation)) {
    const float delta = float(event->angleDelta().y()) / 120.0f;
    if (delta != 0.f && !mmuPointerBrush) {
      const bool down = delta < 0.f;
      if (mmuTool && m_mmuPaintTool == 4) {
        // PAINT-MMU-TOOLS: m_cursor_height step (upstream
        // GLGizmoPainterBase.cpp:584-592 -- CursorHeightStep 0.2 clamped to
        // [CursorHeightMin 0.1, CursorHeightMax 8],
        // GLGizmoPainterBase.hpp:242-245). The new value round-trips through
        // heightRangeTuned -> QML -> EditorViewModel.paintHeightRange so the
        // band + the panel slider follow the wheel.
        const double next = down ? m_brushHeightRange - 0.2
                                 : m_brushHeightRange + 0.2;
        m_brushHeightRange = qBound(0.1, next, 8.0);
        emit heightRangeTuned(m_brushHeightRange);
      } else if ((m_paintToolType == 3 &&
                  m_gizmoMode == GizmoSupportPaint) ||
                 (mmuTool && m_mmuPaintTool == 3)) {
        // PAINT-GAPFILL-PORT: gap_area step (upstream
        // GLGizmoPainterBase.cpp:624-627). The new value round-trips through
        // gapAreaTuned -> QML -> EditorViewModel.supportPaintGapArea so the
        // -3 fragment overlay and the panel slider follow the wheel.
        // PAINT-MMU-TOOLS: the MMU GapFill chip steps the same shared
        // threshold (upstream GAP_FILL wheel is gizmo-independent).
        const float next = down ? m_gapArea - 0.2f : m_gapArea + 0.2f;
        m_gapArea = qBound(0.0f, next, 5.0f);
        emit gapAreaTuned(double(m_gapArea));
      } else if (m_paintToolType == 2 || (mmuTool && m_mmuPaintTool == 1)) {
        const float next = down ? m_smartFillAngle - 1.f : m_smartFillAngle + 1.f;
        m_smartFillAngle = qBound(0.0f, next, 90.0f);
      } else {
        const float next = down ? m_brushRadius - 0.2f : m_brushRadius + 0.2f;
        m_brushRadius = qBound(0.4f, next, 8.0f);
      }
      event->accept();
      update();
      return;
    }
  }
  // PAINT-ALT-WHEEL-CLIP (upstream GLGizmoPainterBase.cpp:632-641, the
  // else-if after the ctrl_down branch): with a paint gizmo active, Alt+wheel
  // sweeps the cross-section (clipping) plane ratio instead of zooming --
  // wheel down steps -0.01 clamped to 0, wheel up steps +0.01 clamped to 1
  // (ObjectClipper::set_position_by_ratio). The new ratio round-trips through
  // paintClippingTuned -> QML -> EditorViewModel.paintClippingPosition so the
  // clipped-side pick rejection + the panel sliders follow the wheel (same
  // write-back contract as gapAreaTuned).
  else if (event->modifiers() & Qt::AltModifier &&
           (m_gizmoMode == GizmoSupportPaint ||
            m_gizmoMode == GizmoSeamPaint ||
            m_gizmoMode == GizmoMmuSegmentation)) {
    const float delta = float(event->angleDelta().y()) / 120.0f;
    if (delta != 0.f) {
      const double next = (delta < 0.f)
          ? m_paintClippingPosition - 0.01
          : m_paintClippingPosition + 0.01;
      m_paintClippingPosition = qBound(0.0, next, 1.0);
      emit paintClippingTuned(m_paintClippingPosition);
      event->accept();
      update();
      return;
    }
  }
  // v5.12 gap-closure: reverseZoom inverts the wheel direction (upstream
  // reverse_mouse_wheel_zoom, GLCanvas3D.cpp:3765). One notch = +-1 like the
  // upstream GetWheelRotation/GetWheelDelta ratio.
  float delta = float(event->angleDelta().y()) / 120.0f;
  if (m_reverseZoom)
    delta = -delta;
  if (!m_zoomToMouse) {
    // Upstream GLCanvas3D.cpp:3768: plain center zoom.
    m_camera.zoom(delta);
  } else {
    // Upstream zoom_to_mouse (GLCanvas3D.cpp:4007-4028): displacement between
    // the cursor and the canvas center unprojected at ONE shared depth plane
    // (upstream window z=0 = tight-frustum near plane; here the scene-front
    // plane via viewFrontPoint, matching the pan pick), translate, zoom, then
    // translate back scaled by the zoom ratio so the point under the cursor
    // stays. The view-parallel plane keeps the displacement scene-bounded at
    // any elevation -- the earlier bed-plane pick exploded at low elevation
    // and flew the camera.
    const float aspect = height() > 0 ? float(width()) / float(height()) : 1.0f;
    const QMatrix4x4 mvp = cameraMvp(aspect);
    const QSizeF vp(width(), height());
    const QVector3D referencePoint = m_camera.viewFrontPoint();
    QVector3D centerPoint;
    QVector3D mousePoint;
    if (CameraController::viewPlanePoint(
            mvp, vp, QPointF(width() * 0.5, height() * 0.5),
            referencePoint, &centerPoint)
        && CameraController::viewPlanePoint(mvp, vp, event->position(),
                                            referencePoint, &mousePoint)) {
      const QVector3D displacement = mousePoint - centerPoint;
      m_camera.translateWorld(displacement);
      const float distanceBefore = m_camera.distance();
      m_camera.zoom(delta);
      const float distanceAfter = m_camera.distance();
      if (distanceAfter > 1e-5f) {
        // Upstream: translate(-displacement / (new_zoom / origin_zoom));
        // zoom ~ 1/distance puts distanceAfter/distanceBefore in the
        // denominator position.
        m_camera.translateWorld(-displacement
                                * (distanceAfter / distanceBefore));
      }
    } else {
      m_camera.zoom(delta);
    }
  }
  m_cameraDirty = true;
  event->accept();
  update();
  emit navigatorLabelsChanged();
  emit plateAnchorsChanged();
}

QMatrix4x4 RhiViewport::cameraMvp(float aspect) const
{
  return m_camera.projMatrix(aspect) * m_camera.viewMatrix();
}

void RhiViewport::fitPreviewCameraToData()
{
  if (m_canvasType != CanvasPreview)
    return;

  if (m_previewData.size() < 8) {
    m_previewCameraFitted = false;
    m_previewFitHint = {};
    return;
  }
  if (std::memcmp(m_previewData.constData(), "GCV1", 4) != 0)
    return;

  int count = 0;
  std::memcpy(&count, m_previewData.constData() + 4, 4);
  if (count <= 0)
    return;

  const qsizetype payloadSize = qsizetype(count) * sizeof(GcvPackedSegment);
  if (m_previewData.size() < 8 + payloadSize)
    return;

  const auto *seg = reinterpret_cast<const GcvPackedSegment *>(m_previewData.constData() + 8);
  bool hasPoint = false;
  float minX = std::numeric_limits<float>::max();
  float minY = std::numeric_limits<float>::max();
  float minZ = std::numeric_limits<float>::max();
  float maxX = std::numeric_limits<float>::lowest();
  float maxY = std::numeric_limits<float>::lowest();
  float maxZ = std::numeric_limits<float>::lowest();

  const auto includePoint = [&](float x, float y, float z) {
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
      return;
    minX = std::min(minX, x);
    minY = std::min(minY, y);
    minZ = std::min(minZ, z);
    maxX = std::max(maxX, x);
    maxY = std::max(maxY, y);
    maxZ = std::max(maxZ, z);
    hasPoint = true;
  };

  for (int i = 0; i < count; ++i) {
    // Stored segments carry slic3r (x, y=depth, z=up); scene maps
    // (x, z, -y) -- AXES-ROT rotation mapping.
    includePoint(seg[i].x1, seg[i].z1, -seg[i].y1);
    includePoint(seg[i].x2, seg[i].z2, -seg[i].y2);
  }

  if (!hasPoint)
    return;

  const float cx = (minX + maxX) * 0.5f;
  const float cy = (minY + maxY) * 0.5f;
  const float cz = (minZ + maxZ) * 0.5f;
  const float rx = (maxX - minX) * 0.5f;
  const float ry = (maxY - minY) * 0.5f;
  const float rz = (maxZ - minZ) * 0.5f;
  const float radius = std::max(10.0f, std::sqrt(rx * rx + ry * ry + rz * rz));
  const QVector4D fitHint(cx, cy, cz, radius);

  const bool sameFit = m_previewCameraFitted
      && std::fabs(m_previewFitHint.x() - fitHint.x()) <= 0.001f
      && std::fabs(m_previewFitHint.y() - fitHint.y()) <= 0.001f
      && std::fabs(m_previewFitHint.z() - fitHint.z()) <= 0.001f
      && std::fabs(m_previewFitHint.w() - fitHint.w()) <= 0.001f;
  if (sameFit)
    return;

  m_previewFitHint = fitHint;
  m_previewCameraFitted = true;
  m_camera.fitView(cx, cy, cz, radius);
  m_cameraDirty = true;
}

void RhiViewport::updatePickingScene()
{
  if (m_pickModelGeneration == m_modelGeneration && m_pickSceneGeneration == m_sceneGeneration)
    return;

  QList<int> activeObjectIndices;
  activeObjectIndices.reserve(m_activePlateObjectIndices.size());
  for (const QVariant &value : m_activePlateObjectIndices)
    activeObjectIndices.append(value.toInt());

  QList<int> batchSourceObjectIndices;
  batchSourceObjectIndices.reserve(m_meshBatchSourceObjectIndices.size());
  for (const QVariant &value : m_meshBatchSourceObjectIndices)
    batchSourceObjectIndices.append(value.toInt());

  QList<int> batchVolumeIndices;
  batchVolumeIndices.reserve(m_meshBatchVolumeIndices.size());
  for (const QVariant &value : m_meshBatchVolumeIndices)
    batchVolumeIndices.append(value.toInt());

  QList<int> batchInstanceIndices;
  batchInstanceIndices.reserve(m_meshBatchInstanceIndices.size());
  for (const QVariant &value : m_meshBatchInstanceIndices)
    batchInstanceIndices.append(value.toInt());

  m_pickScene.setPlateContext(m_currentPlateIndex, m_plateCount, activeObjectIndices);
  m_pickScene.setBed(m_bedWidth, m_bedDepth, m_bedOriginX, m_bedOriginY,
                     m_bedShapeType, m_bedDiameter);
  m_pickScene.setModelMeshData(m_meshData, batchSourceObjectIndices,
                               batchVolumeIndices, batchInstanceIndices,
                               activeObjectIndices);
  m_pickScene.clearDirtyFlags();
  m_pickModelGeneration = m_modelGeneration;
  m_pickSceneGeneration = m_sceneGeneration;
}

int RhiViewport::pickSourceObjectAt(const QPointF &position)
{
  updatePickingScene();
  if (width() <= 1.0 || height() <= 1.0)
    return -1;

  const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
  const float aspect = float(viewSize.width()) / float(viewSize.height());
  auto [rayOrigin, rayDirection] = GizmoMath::computeRay(
      float(position.x()), float(position.y()),
      viewSize,
      m_camera.projMatrix(aspect),
      m_camera.viewMatrix());

  // PICK-BVH: BVH-backed pick over the picking scene.
  return m_pickScene.pickingRaycaster()
      ->pick(rayOrigin, rayDirection,
             m_pickScene.modelVertices(), m_pickScene.modelBatches())
      .sourceObjectIndex;
}

ViewportContextHit RhiViewport::classifyContextAt(const QPointF &position)
{
  ViewportContextHit result;
  result.popupPosition = position;
  updatePickingScene();
  if (width() <= 1.0 || height() <= 1.0) {
    result.target = ViewportContextTarget::Empty;
    return result;
  }

  const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
  const float aspect = float(viewSize.width()) / float(viewSize.height());
  const auto [rayOrigin, rayDirection] = GizmoMath::computeRay(
      float(position.x()), float(position.y()), viewSize,
      m_camera.projMatrix(aspect), m_camera.viewMatrix());
  // PICK-BVH: BVH-backed pick over the picking scene.
  const ObjectPicking::Hit objectHit = m_pickScene.pickingRaycaster()->pick(
      rayOrigin, rayDirection, m_pickScene.modelVertices(), m_pickScene.modelBatches());
  if (objectHit.isValid()) {
    result.target = ViewportContextTarget::Part;
    result.sourceObjectIndex = objectHit.sourceObjectIndex;
    result.volumeIndex = objectHit.volumeIndex;
    result.instanceIndex = objectHit.instanceIndex;
    result.plateIndex = m_pickScene.currentPlateIndex();
    return result;
  }

  if (std::abs(rayDirection.y()) <= 1e-6f) {
    result.target = ViewportContextTarget::Empty;
    return result;
  }
  const float groundDistance = -rayOrigin.y() / rayDirection.y();
  if (groundDistance < 0.0f) {
    result.target = ViewportContextTarget::Empty;
    return result;
  }
  const QVector3D groundPoint = rayOrigin + rayDirection * groundDistance;
  if (m_showWipeTower && m_wipeTowerWidth > 0.0f && m_wipeTowerDepth > 0.0f
      && groundPoint.x() >= m_wipeTowerX - m_wipeTowerWidth * 0.5f
      && groundPoint.x() <= m_wipeTowerX + m_wipeTowerWidth * 0.5f
      && groundPoint.z() >= m_wipeTowerZ - m_wipeTowerDepth * 0.5f
      && groundPoint.z() <= m_wipeTowerZ + m_wipeTowerDepth * 0.5f) {
    result.target = ViewportContextTarget::Suppressed;
    return result;
  }
  if (m_pickScene.containsCurrentPlatePoint(groundPoint.x(), groundPoint.z())) {
    result.target = ViewportContextTarget::Plate;
    result.plateIndex = m_pickScene.currentPlateIndex();
  } else {
    result.target = ViewportContextTarget::Empty;
  }
  return result;
}

// ===========================================================================
// SEL-MODIFIERS-RECT: rubber-band rectangle selection (upstream
// GLSelectionRectangle + GLCanvas3D.cpp:4136-4145/4255-4260/4384-4391).
// Geometry stays C++-owned: the viewport projects the visible pick-scene
// batches, QML only draws the band and forwards the finished stroke.
// ===========================================================================
QRectF RhiViewport::projectBoundsToScreenRect(
    const PrepareSceneData::ModelBounds &bounds,
    const QMatrix4x4 &viewProjection,
    const QSizeF &viewportSize)
{
  // GLSelectionRectangle::contains (GLSelectionRectangle.cpp:33-49): project
  // sample points into screen space and keep the enclosing rectangle. The 8
  // bounds corners sample the object's extent. Screen Y grows downward --
  // same convention as projectWorldToScreen(). Corners behind the camera
  // (w <= 0) are skipped so a partially clipped object does not fold the
  // rect across the viewport.
  if (viewportSize.isEmpty())
    return QRectF();

  const float corners[8][3] = {
      {bounds.minX, bounds.minY, bounds.minZ},
      {bounds.maxX, bounds.minY, bounds.minZ},
      {bounds.minX, bounds.maxY, bounds.minZ},
      {bounds.minX, bounds.minY, bounds.maxZ},
      {bounds.maxX, bounds.maxY, bounds.minZ},
      {bounds.maxX, bounds.minY, bounds.maxZ},
      {bounds.minX, bounds.maxY, bounds.maxZ},
      {bounds.maxX, bounds.maxY, bounds.maxZ}};

  float minX = 0.0f, minY = 0.0f, maxX = 0.0f, maxY = 0.0f;
  bool any = false;
  for (const auto &corner : corners) {
    const QVector4D clip = viewProjection * QVector4D(corner[0], corner[1], corner[2], 1.0f);
    if (clip.w() <= 0.0f)
      continue;
    const QVector3D ndc = clip.toVector3D() / clip.w();
    const float sx = float((ndc.x() * 0.5 + 0.5) * viewportSize.width());
    const float sy = float((1.0 - (ndc.y() * 0.5 + 0.5)) * viewportSize.height());
    if (!any) {
      minX = maxX = sx;
      minY = maxY = sy;
      any = true;
    } else {
      minX = std::min(minX, sx);
      minY = std::min(minY, sy);
      maxX = std::max(maxX, sx);
      maxY = std::max(maxY, sy);
    }
  }
  if (!any)
    return QRectF();
  return QRectF(QPointF(minX, minY), QPointF(maxX, maxY));
}

bool RhiViewport::rectHitsProjectedBounds(const QRectF &rect, const QRectF &screenBounds)
{
  // The upstream picking pass selects a volume when ANY of its rendered
  // pixels falls inside the rectangle (GLCanvas3D.cpp:7019-7030). The
  // projected-bounds approximation keeps that partial-overlap semantics; a
  // degenerate (click-sized) band still behaves like the upstream 1x1 pixel
  // fallback (GLCanvas3D.cpp:6899-6900).
  if (screenBounds.isEmpty())
    return false;
  return rect.intersects(screenBounds);
}

void RhiViewport::updateSelectionRubberBand(const QPointF &position)
{
  QRectF rect(m_rectSelectStart, position);
  rect = rect.normalized();
  if (!m_selectionRubberBandActive || rect != m_selectionRubberBand) {
    m_selectionRubberBand = rect;
    m_selectionRubberBandActive = true;
    emit selectionRubberBandChanged();
  }
  update();
}

QVariantList RhiViewport::containedRectangleSelectionSources(const QRectF &rect)
{
  QVariantList contained;
  if (width() <= 1.0 || height() <= 1.0)
    return contained;

  updatePickingScene();
  const QSizeF viewportSize(width(), height());
  const float aspect = float(width()) / float(height());
  const QMatrix4x4 viewProjection = cameraMvp(aspect);
  // Visible = the batches already filtered into the pick scene for the
  // current plate (PrepareSceneData::setModelMeshData drops every batch whose
  // source object is not on the active plate). Every batch of a source object
  // (all volumes + instances) is tested: any hit selects the object, matching
  // the upstream per-volume hover collection.
  for (const PrepareSceneData::ModelBatch &batch : m_pickScene.modelBatches()) {
    if (batch.sourceObjectIndex < 0 || contained.contains(QVariant(batch.sourceObjectIndex)))
      continue;
    const QRectF screenBounds =
        projectBoundsToScreenRect(batch.bounds, viewProjection, viewportSize);
    if (rectHitsProjectedBounds(rect, screenBounds))
      contained.append(batch.sourceObjectIndex);
  }
  return contained;
}

// ===========================================================================
// Phase 69/70: gizmo-axis picking helpers
// ===========================================================================
QVector3D RhiViewport::currentGizmoCenter() const
{
  // P15.11 (MULTICENTER): pivot on the union AABB of ALL selected objects
  // (upstream Selection::get_bounding_box().center()), matching the center the
  // renderer draws the gizmo at. Falls back to the back-compat single index
  // when only that legacy property was written.
  QList<int> indices;
  indices.reserve(m_selectedSourceObjectIndices.size());
  for (const QVariant &entry : m_selectedSourceObjectIndices)
    indices.append(entry.toInt());
  if (indices.isEmpty() && m_selectedSourceObjectIndex >= 0)
    indices.append(m_selectedSourceObjectIndex);
  // Reuse the extracted Phase 67 helper against the pick-scene batches.
  return GizmoCenter::fromSelectedIndices(indices, m_pickScene.modelBatches());
}

int RhiViewport::pickGizmoAxisAt(const QPointF &position)
{
  if (m_selectedSourceObjectIndex < 0 && m_selectedSourceObjectIndices.isEmpty())
    return 0;
  updatePickingScene();
  const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
  if (viewSize.width() <= 0 || viewSize.height() <= 0)
    return 0;
  const float aspect = float(viewSize.width()) / float(viewSize.height());
  if (m_gizmoMode == GizmoMove)
  {
    return GizmoMath::pickMoveAxis(
        float(position.x()), float(position.y()), viewSize,
        m_camera.projMatrix(aspect), m_camera.viewMatrix(),
        currentGizmoCenter(), m_camera.eye(),
        /*hasSelection=*/true);
  }
  if (m_gizmoMode == GizmoRotate)
  {
    return GizmoMath::pickRotateAxis(
        float(position.x()), float(position.y()), viewSize,
        m_camera.projMatrix(aspect), m_camera.viewMatrix(),
        currentGizmoCenter(), m_camera.eye(),
        /*hasSelection=*/true);
  }
  if (m_gizmoMode == GizmoScale)
  {
    return GizmoMath::pickScaleAxis(
        float(position.x()), float(position.y()), viewSize,
        m_camera.projMatrix(aspect), m_camera.viewMatrix(),
        currentGizmoCenter(), m_camera.eye(),
        /*hasSelection=*/true);
  }
  return 0;
}

void RhiViewport::resetGizmoDragState()
{
  m_gizmoDragging = false;
  m_gizmoAxis = 0;
  m_gizmoDragMode = GizmoMove;
  m_gizmoDragStartT = 0.f;
  m_gizmoRotateStartAngle = 0.f;
  m_gizmoDragCenter = {};
}

// ===========================================================================
// Phase 115 (MEASURE-04): measure-gizmo snap UX wiring
// ===========================================================================
void RhiViewport::emitMeasurePickIfActive(const QPointF &position,
                                          Qt::KeyboardModifiers modifiers)
{
  // Only the measure gizmo drives this path. Other gizmos (move/rotate/scale/
  // cut/flatten/...) keep their existing mouse handling untouched.
  if (m_gizmoMode != GizmoMeasure)
    return;

  // Stage-1: cheap ray->AABB prefilter over the scene vertices. Returns -1
  // when the ray misses every object's AABB (the ViewModel clears the hover
  // highlight in that case).
  const int pickedSourceIndex = pickSourceObjectAt(position);
  if (pickedSourceIndex < 0) {
    emit measureHoverLeft();
    return;
  }

  // Build the world-space pick ray (same GizmoMath::computeRay the object
  // picking + gizmo-axis picking already use). The ViewModel feeds this to
  // SceneRaycaster::hitTest (Phase 113 stage-2).
  const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
  if (viewSize.width() <= 1 || viewSize.height() <= 1)
    return;
  const float aspect = float(viewSize.width()) / float(viewSize.height());
  auto [rayOrigin, rayDirection] = GizmoMath::computeRay(
      float(position.x()), float(position.y()),
      viewSize,
      m_camera.projMatrix(aspect), m_camera.viewMatrix());

  // Shift toggle (GLGizmoMeasure.cpp:409-442): Qt::ShiftModifier forces
  // EMode::PointSelection; absence keeps the default FeatureSelection.
  const bool shiftHeld = (modifiers & Qt::ShiftModifier) != 0;
  // The parameters are named worldOrigin/worldDirection on the signal (not
  // rayOrigin/rayDirection) to keep the literal "ray" out of PreparePage.qml
  // (the rhiViewportSelectionPickingBridgeStaysCppOwned audit forbids it).
  emit measurePickRequested(rayOrigin, rayDirection, pickedSourceIndex, shiftHeld);
}

// ===========================================================================
// Phase HOLLOW: hollow-gizmo pick wiring (drain hole placement)
// ===========================================================================
void RhiViewport::emitHollowPickIfActive(const QPointF &position,
                                         Qt::KeyboardModifiers modifiers)
{
  // Only the hollow gizmo drives this path (mirror of emitMeasurePickIfActive).
  if (m_gizmoMode != GizmoHollow)
    return;

  Q_UNUSED(modifiers); // hollow placement is single-click, no modifier branching

  // Stage-1: cheap ray->AABB prefilter over the scene vertices.
  const int pickedSourceIndex = pickSourceObjectAt(position);
  if (pickedSourceIndex < 0)
    return; // missed every object's AABB -- no hole placed

  // Build the world-space pick ray (same GizmoMath::computeRay the measure
  // path uses). The ViewModel feeds this to SceneRaycaster::hitTest.
  const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
  if (viewSize.width() <= 1 || viewSize.height() <= 1)
    return;
  const float aspect = float(viewSize.width()) / float(viewSize.height());
  auto [rayOrigin, rayDirection] = GizmoMath::computeRay(
      float(position.x()), float(position.y()),
      viewSize,
      m_camera.projMatrix(aspect), m_camera.viewMatrix());

  emit hollowPickRequested(rayOrigin, rayDirection, pickedSourceIndex);
}

// ===========================================================================
// v5.13: AdvancedCut connector-pin pick wiring (place connector on click)
// ===========================================================================
void RhiViewport::emitAdvancedCutPickIfActive(const QPointF &position,
                                              Qt::KeyboardModifiers modifiers)
{
  // Only the AdvancedCut gizmo drives this path. The ViewModel's
  // placeAdvancedCutConnector no-ops unless the connectors toggle is on.
  if (m_gizmoMode != GizmoAdvancedCut)
    return;
  Q_UNUSED(modifiers);

  const int pickedSourceIndex = pickSourceObjectAt(position);
  if (pickedSourceIndex < 0)
    return;

  const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
  if (viewSize.width() <= 1 || viewSize.height() <= 1)
    return;
  const float aspect = float(viewSize.width()) / float(viewSize.height());
  auto [rayOrigin, rayDirection] = GizmoMath::computeRay(
      float(position.x()), float(position.y()),
      viewSize,
      m_camera.projMatrix(aspect), m_camera.viewMatrix());

  emit advancedCutPickRequested(rayOrigin, rayDirection, pickedSourceIndex);
}

// ===========================================================================
// Phase 120 (PAINT-01): paint-gizmo pick wiring
// ===========================================================================
void RhiViewport::emitPaintPickIfActive(const QPointF &position,
                                        Qt::KeyboardModifiers modifiers)
{
  // Only the three paint gizmos drive this path. Other gizmos (move/rotate/
  // scale/measure/cut/flatten/...) keep their existing mouse handling.
  // TS-06: gate on m_gizmoMode in {GizmoSupportPaint, GizmoSeamPaint,
  // GizmoMmuSegmentation}.
  const bool isPaintGizmo =
      (m_gizmoMode == GizmoSupportPaint ||
       m_gizmoMode == GizmoSeamPaint ||
       m_gizmoMode == GizmoMmuSegmentation);
  if (!isPaintGizmo)
    return;

  // Stage-1: cheap ray->AABB prefilter (same pickSourceObjectAt the measure
  // path uses). Returns -1 when the ray misses every object's AABB.
  const int pickedSourceIndex = pickSourceObjectAt(position);
  if (pickedSourceIndex < 0)
    return;

  // Build the world-space pick ray (same GizmoMath::computeRay the object
  // picking + measure pick already use). The ViewModel feeds this to
  // SceneRaycaster::hitTest (Phase 113 stage-2) which resolves the facet +
  // mesh-local hit that PaintEngine::paintAt needs.
  const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
  if (viewSize.width() <= 1 || viewSize.height() <= 1)
    return;
  const float aspect = float(viewSize.width()) / float(viewSize.height());
  auto [rayOrigin, rayDirection] = GizmoMath::computeRay(
      float(position.x()), float(position.y()),
      viewSize,
      m_camera.projMatrix(aspect), m_camera.viewMatrix());

  // Phase 240 (GIZ-02): when the SmartFill tool is active OR Shift is held on
  // CLICK, the pick routes to the smart-fill path instead (upstream
  // ToolType::SMART_FILL seed fill; Phase 240 spec maps Shift+click to smart
  // fill). Smart fill only fires on press -- a shift-DRAG keeps the brush
  // path so a drag does not spam seed fills (upstream SMART_FILL is
  // click-driven).
  const double brushRadius = double(m_brushRadius);
  const int    cursorType  = m_brushCursorType;
  const bool   shiftHeld   = (modifiers & Qt::ShiftModifier) != 0;
  // P11 (upstream GLGizmoPainterBase.cpp:662-671): Shift holds new_state at
  // NONE, i.e. Shift is a temporary ERASER for every painter gizmo (the
  // earlier smart-fill mapping inverted this). The right button paints the
  // alternate state (PainterBase default right=BLOCKER; the complement of
  // the active tool keeps the Enforcer/Blocker pair symmetric).
  int paintState;
  if (m_paintButton == 2)
    paintState = (m_paintState == 2) ? 1 : 2;
  else if (shiftHeld)
    paintState = 0;
  else
    paintState = m_paintState;
  // PAINT-MMU-TOOLS: the MMU gizmo routes its own tool set BEFORE the shared
  // channels (upstream GLGizmoMmuSegmentation keeps its own m_current_tool
  // independent of the other painter gizmos). Press-only like the smart fill
  // -- a drag would restage the same region on every move event. Shift keeps
  // its temporary-eraser meaning (paintState NONE above), so Shift+click on
  // these tools erases the region/band (upstream runs the same fill branch
  // with new_state NONE).
  if (m_gizmoMode == GizmoMmuSegmentation) {
    // HeightRange tool (upstream HeightRangeIcon -> BRUSH + HEIGHT_RANGE):
    // click anchors the band at the hit's world Z.
    if (m_mmuPaintTool == 4) {
      if (m_paintClickPress && m_paintButton != 2) {
        emit heightRangePickRequested(rayOrigin, rayDirection, m_camera.eye(),
                                      m_camera.forwardVector(),
                                      pickedSourceIndex, paintState,
                                      m_brushHeightRange);
      }
      return;
    }
    // BucketFill tool (upstream FillButtonIcon -> BUCKET_FILL) and the
    // pointer brush (upstream TriangleButtonIcon -> BRUSH + POINTER, routed
    // as bucket_fill with angle -1 + propagate false,
    // GLGizmoMmuSegmentation.cpp:594-595 + GLGizmoPainterBase.cpp:790-793).
    const bool bucketTool = (m_mmuPaintTool == 1);
    const bool pointerBrush =
        (m_mmuPaintTool == 0 && m_brushCursorType == 2);
    if ((bucketTool || pointerBrush) && m_paintClickPress &&
        m_paintButton != 2) {
      // Unchecked edge detection = angle -1 (upstream m_smart_fill_angle =
      // -1.f when m_detect_geometry_edge is off,
      // GLGizmoMmuSegmentation.cpp:633-637).
      const double seedFillAngle = bucketTool
          ? (m_mmuBucketEdgeDetection ? double(m_smartFillAngle) : -1.0)
          : -1.0;
      emit bucketFillPickRequested(rayOrigin, rayDirection,
                                   m_camera.forwardVector(),
                                   pickedSourceIndex, paintState,
                                   seedFillAngle, /*propagate=*/bucketTool);
      return;
    }
    // GapFill tool (upstream GapFillIcon -> GAP_FILL): clicks paint nothing
    // (upstream GAP_FILL matches no click branch -- the fragment merge runs
    // from the Perform button; the Qt6 port keeps the -3 preview only).
    if (m_mmuPaintTool == 3)
      return;
  }
  // Phase 240 (GIZ-02): SmartFill is the tool chip (paintToolType==2) firing
  // on PRESS only -- a shift-DRAG keeps the brush path and the right button
  // never smart-fills.
  const bool   smartFill   = (m_paintToolType == 2) && !shiftHeld &&
                             m_paintButton != 2 && m_paintClickPress;

  if (smartFill) {
    emit smartFillPickRequested(rayOrigin, rayDirection,
                                m_camera.forwardVector(), pickedSourceIndex,
                                paintState, double(m_smartFillAngle),
                                m_paintOnOverhangsOnly,
                                double(m_paintOverhangAngle));
    return;
  }

  // Forward to QML opaquely (no ray math in QML -- same contract as
  // measurePickRequested). QML connects this to EditorViewModel::paintAtFacet.
  // cameraForward (PAINT-ALT-WHEEL-CLIP) is the camera look direction the
  // cross-section plane normal derives from (upstream -camera.
  // get_dir_forward(), ObjectClipper::set_position_by_ratio).
  emit paintPickRequested(rayOrigin, rayDirection, m_camera.eye(),
                          m_camera.forwardVector(),
                          pickedSourceIndex, brushRadius, cursorType, paintState,
                          /*smartFill=*/0);
}

// ===========================================================================
// Phase 240 (GIZ-03): flatten gizmo pick wiring (hover highlight + click)
// ===========================================================================
void RhiViewport::emitFlattenPickIfActive(const QPointF &position, bool click)
{
  // Only the Flatten gizmo drives this path.
  if (m_gizmoMode != GizmoFlatten)
    return;

  // Stage-1: cheap ray->AABB prefilter (same pickSourceObjectAt the measure
  // path uses). -1 = miss (hover clears the highlight).
  const int pickedSourceIndex = pickSourceObjectAt(position);

  const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
  if (viewSize.width() <= 1 || viewSize.height() <= 1)
    return;
  const float aspect = float(viewSize.width()) / float(viewSize.height());
  auto [rayOrigin, rayDirection] = GizmoMath::computeRay(
      float(position.x()), float(position.y()),
      viewSize,
      m_camera.projMatrix(aspect), m_camera.viewMatrix());

  if (click)
    emit flattenPickRequested(rayOrigin, rayDirection, pickedSourceIndex);
  else
    emit flattenHoverRequested(rayOrigin, rayDirection, pickedSourceIndex);
}

// ===========================================================================
// Phase 240 (GIZ-04): interactive cut-plane pick + drag helpers
// ===========================================================================
QVector3D RhiViewport::cutPlaneCenter() const
{
  // The plane sits on the cut axis at cutPosition inside the selected
  // object's bounds; the renderer builds the quad from the same bounds.
  // Rebuild them from the picking scene batches (same data source the
  // renderer's uploadCutPlaneBuffers uses).
  const_cast<RhiViewport *>(this)->updatePickingScene();
  const int selected = m_selectedSourceObjectIndex;
  if (selected < 0)
    return {};
  bool found = false;
  float minX = 0.f, minY = 0.f, minZ = 0.f, maxX = 0.f, maxY = 0.f, maxZ = 0.f;
  for (const PrepareSceneData::ModelBatch &batch : m_pickScene.modelBatches()) {
    if (batch.sourceObjectIndex != selected)
      continue;
    if (!found) {
      minX = batch.bounds.minX; minY = batch.bounds.minY; minZ = batch.bounds.minZ;
      maxX = batch.bounds.maxX; maxY = batch.bounds.maxY; maxZ = batch.bounds.maxZ;
      found = true;
      continue;
    }
    minX = std::min(minX, batch.bounds.minX);
    minY = std::min(minY, batch.bounds.minY);
    minZ = std::min(minZ, batch.bounds.minZ);
    maxX = std::max(maxX, batch.bounds.maxX);
    maxY = std::max(maxY, batch.bounds.maxY);
    maxZ = std::max(maxZ, batch.bounds.maxZ);
  }
  if (!found)
    return {};
  switch (m_cutAxis) {
  case 0: return QVector3D(m_cutPosition, (minY + maxY) * 0.5f, (minZ + maxZ) * 0.5f);
  case 1: return QVector3D((minX + maxX) * 0.5f, m_cutPosition, (minZ + maxZ) * 0.5f);
  default: return QVector3D((minX + maxX) * 0.5f, (minY + maxY) * 0.5f, m_cutPosition);
  }
}

int RhiViewport::pickCutPlaneAt(const QPointF &position)
{
  if (m_gizmoMode != GizmoCut && m_gizmoMode != GizmoAdvancedCut)
    return 0;
  if (m_selectedSourceObjectIndex < 0)
    return 0;
  const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
  if (viewSize.width() <= 1 || viewSize.height() <= 1)
    return 0;
  const float aspect = float(viewSize.width()) / float(viewSize.height());
  const QMatrix4x4 proj = m_camera.projMatrix(aspect);
  const QMatrix4x4 view = m_camera.viewMatrix();

  const QVector3D center = cutPlaneCenter();
  if (center.isNull())
    return 0;

  // Grabber screen hit test (upstream GLGizmoCut3D rotation grabbers sit on
  // the plane border). Two tilt grabbers: one on +X side of the plane (tilt
  // around world X), one on +Y side (tilt around world Y). 14px hit radius
  // matches the gizmo axis hit tolerance.
  const QVector3D axisX = QVector3D(1.f, 0.f, 0.f);
  const QVector3D axisY = QVector3D(0.f, 1.f, 0.f);
  // Plane-local frame: the two axes perpendicular to the cut axis.
  QVector3D u, v;
  switch (m_cutAxis) {
  case 0: u = QVector3D(0.f, 1.f, 0.f); v = QVector3D(0.f, 0.f, 1.f); break;
  case 1: u = QVector3D(1.f, 0.f, 0.f); v = QVector3D(0.f, 0.f, 1.f); break;
  default: u = QVector3D(1.f, 0.f, 0.f); v = QVector3D(0.f, 1.f, 0.f); break;
  }
  auto projectToScreen = [&](const QVector3D &world) -> QVector2D {
    const QVector4D clip = proj * view * QVector4D(world, 1.f);
    if (qFuzzyIsNull(clip.w()))
      return {};
    const QVector3D ndc = clip.toVector3D() / clip.w();
    return QVector2D((ndc.x() * 0.5f + 0.5f) * float(viewSize.width()),
                     (1.f - (ndc.y() * 0.5f + 0.5f)) * float(viewSize.height()));
  };
  // Grabber extent: a small world-space offset along the plane axes so the
  // handles sit on the plane border area; the plane tilt rotates the
  // offsets so the pick stays glued to the rendered grabbers.
  QMatrix4x4 grabRot;
  grabRot.rotate(m_cutRotationX, 1.f, 0.f, 0.f);   // world X -> scene X
  grabRot.rotate(m_cutRotationY, 0.f, 0.f, 1.f);   // world Y -> scene Z
  grabRot.rotate(m_cutRotationZ, 0.f, 1.f, 0.f);   // world Z -> scene Y
  const QVector3D g1 = center + grabRot.mapVector(u * 6.f);
  const QVector3D g2 = center + grabRot.mapVector(v * 6.f);
  const QVector2D s1 = projectToScreen(g1);
  const QVector2D s2 = projectToScreen(g2);
  const QVector2D cursor(float(position.x()), float(position.y()));
  const float grabRadius = 14.f;
  if ((cursor - s1).length() <= grabRadius)
    return 2; // tilt around world X
  if ((cursor - s2).length() <= grabRadius)
    return 3; // tilt around world Y

  // Plane body: project the cursor ray onto the cut axis parameter and
  // accept when the ray's closest approach to the plane center is within the
  // object's half-diagonal (coarse plane hit; upstream unprojects onto the
  // cut plane).
  auto [orig, dir] = GizmoMath::computeRay(
      float(position.x()), float(position.y()), viewSize, proj, view);
  const QVector3D axisDir =
      (m_cutAxis == 0) ? axisX : (m_cutAxis == 1) ? axisY : QVector3D(0.f, 0.f, 1.f);
  const float t = GizmoMath::rayToAxisT(orig, dir, axisDir, center);
  if (t <= -1e6f)
    return 0; // ray parallel to axis -- treat as miss
  // Closest point on the ray to the plane center (perpendicular distance to
  // the plane quad, coarse).
  const QVector3D onAxis = center + axisDir * t;
  const QVector3D toRay = orig + dir * 1.f - onAxis;
  Q_UNUSED(toRay);
  // Accept the drag when the cursor is near the plane's on-axis position
  // (within 1/3 screen height of the projected center, matching the coarse
  // gizmo hit tolerance).
  const QVector2D centerScreen = projectToScreen(center);
  const QVector2D onAxisScreen = projectToScreen(onAxis);
  if ((centerScreen - onAxisScreen).length() <= float(viewSize.height()) * 0.2f)
    return 1;
  return 0;
}

void RhiViewport::updateCutPlane()
{
  // Rotation changes must re-upload the rotated plane quad. The renderer
  // keys off m_cutPlaneDirty via synchronize(); there is no direct handle
  // from the item, so bump the picking scene generation which the renderer
  // treats as a scene change (mirrors how cutAxis/cutPosition setters work
  // through the synchronize pull).
  ++m_pickModelGeneration;
  update();
}

// ===========================================================================
// Phase 240 (GIZ-05/GIZ-06): world<->screen helpers
// ===========================================================================
QPointF RhiViewport::projectWorldToScreen(float worldX, float worldY,
                                          float worldZ) const
{
  const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
  if (viewSize.width() <= 1 || viewSize.height() <= 1)
    return {};
  const float aspect = float(viewSize.width()) / float(viewSize.height());
  const QVector4D clip = m_camera.projMatrix(aspect) * m_camera.viewMatrix()
                             * QVector4D(worldX, worldY, worldZ, 1.f);
  if (qFuzzyIsNull(clip.w()))
    return {};
  const QVector3D ndc = clip.toVector3D() / clip.w();
  return QPointF(double((ndc.x() * 0.5f + 0.5f) * float(viewSize.width())),
                 double((1.f - (ndc.y() * 0.5f + 0.5f)) * float(viewSize.height())));
}

QPointF RhiViewport::screenToBedPoint(qreal screenX, qreal screenY) const
{
  const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
  if (viewSize.width() <= 1 || viewSize.height() <= 1)
    return QPointF();
  const float aspect = float(viewSize.width()) / float(viewSize.height());
  auto [orig, dir] = GizmoMath::computeRay(
      float(screenX), float(screenY), viewSize,
      m_camera.projMatrix(aspect), m_camera.viewMatrix());
  // Scene coordinates use Y as vertical, so the bed is the Y=0 plane and the
  // 2D bed coordinates are (X, Z). AXES-ROT: scene Z negates slic3r Y, so
  // the returned slic3r bed point is (hit.x, -hit.z). Reject parallel and
  // behind-camera hits.
  if (qFuzzyIsNull(dir.y()))
    return QPointF();
  const float t = -orig.y() / dir.y();
  if (t < 0.f)
    return QPointF();
  const QVector3D hit = orig + dir * t;
  return QPointF(double(hit.x()), double(-hit.z()));
}

// Phase 121 (PAINT-03/OV-05): track the mouse screen position + button state so
// renderBrushCursor can draw the sphere cursor at the brush location. buttonState
// drives the cursor color (0=hover black, 1=left blue, 2=right red). No-op when
// no paint gizmo is active (keeps the cursor hidden outside paint mode).
void RhiViewport::updateBrushCursorState(const QPointF &position, int buttonState)
{
  const bool isPaintGizmo =
      (m_gizmoMode == GizmoSupportPaint ||
       m_gizmoMode == GizmoSeamPaint ||
       m_gizmoMode == GizmoMmuSegmentation);
  if (!isPaintGizmo)
  {
    // Clear the cursor when not painting so it does not linger.
    if (m_brushButtonState != 0 || m_brushMouseScreenX != 0.f
        || m_brushMouseScreenY != 0.f)
    {
      m_brushButtonState = 0;
      m_brushMouseScreenX = 0.f;
      m_brushMouseScreenY = 0.f;
      update();
    }
    return;
  }
  m_brushMouseScreenX = float(position.x());
  m_brushMouseScreenY = float(position.y());
  m_brushButtonState = buttonState;
  update();
}
