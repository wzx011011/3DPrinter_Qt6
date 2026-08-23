#include "RhiViewport.h"
#include "RhiViewportRenderer.h"
#include "core/rendering/GizmoCenter.h"
#include "core/rendering/GizmoMath.h"
#include "core/rendering/NavigatorCube.h"
#include "core/rendering/ObjectPicking.h"

#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QHoverEvent>
#include <QImage>
#include <QMouseEvent>
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
  float feedrate, fan_speed, temperature, width, layer_time, acceleration;
  float jerk, pressure_advance, actual_speed, actual_flow;
  int extruder_id, layer, move;
  int role;  // must match PackedSegment layout exactly (canonical libvgcode index).
};
static_assert(sizeof(GcvPackedSegment) == 92,
              "GcvPackedSegment must be 92 bytes (19 floats + 4 ints)");

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
}

void RhiViewport::setBedDepth(float value)
{
  if (qFuzzyCompare(m_bedDepth, value))
    return;
  m_bedDepth = value;
  ++m_sceneGeneration;
  update();
}

void RhiViewport::setBedOriginX(float value)
{
  if (qFuzzyCompare(m_bedOriginX, value))
    return;
  m_bedOriginX = value;
  ++m_sceneGeneration;
  update();
}

void RhiViewport::setBedOriginY(float value)
{
  if (qFuzzyCompare(m_bedOriginY, value))
    return;
  m_bedOriginY = value;
  ++m_sceneGeneration;
  update();
}

void RhiViewport::setBedShapeType(int value)
{
  if (m_bedShapeType == value)
    return;
  m_bedShapeType = value;
  ++m_sceneGeneration;
  update();
}

void RhiViewport::setBedDiameter(float value)
{
  if (qFuzzyCompare(m_bedDiameter, value))
    return;
  m_bedDiameter = value;
  ++m_sceneGeneration;
  update();
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
}

void RhiViewport::setBedHeightLimitActive(bool value)
{
  if (m_bedHeightLimitActive == value)
    return;
  m_bedHeightLimitActive = value;
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
  if (dir == QLatin1String("top"))
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
    // lives in the GL XZ plane (Y up); bedOriginX/bedOriginY are slic3r X/Y
    // which map to GL X/Z (same mapping as meshData's coordinate swap).
    const float w = qMax(1.0f, m_bedWidth);
    const float d = qMax(1.0f, m_bedDepth);
    if (m_bedShapeType == 1)
    {
      const float radius = qMax(1.0f, m_bedDiameter);
      m_camera.fitView(m_bedOriginX, 0.f, m_bedOriginY, radius * 0.5f);
    }
    else
    {
      m_camera.fitView(m_bedOriginX + w * 0.5f, 0.f, m_bedOriginY + d * 0.5f,
                       0.5f * std::sqrt(w * w + d * d));
    }
  }
  m_viewPreset = 3; // isometric family; legacy int preset stays for GLToolbars
  m_cameraDirty = true;
  update();
  emit navigatorLabelsChanged();
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

void RhiViewport::requestThumbnailCapture(int plateIndex, int size)
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
  m_thumbnailRequestPending = true;
  update();  // schedule synchronize()+render() on the render thread
}

void RhiViewport::deliverThumbnail(const QImage &image, int plateIndex)
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
  emit thumbnailCapturedForPlate(plateIndex, m_lastThumbnailData);
}

// ── v5.16 (NAVIGATOR): bottom-left 3D navigator cube ──
// Upstream GLCanvas3D::_render_3d_navigator (GLCanvas3D.cpp:5669-5733) hosts
// ImGuizmo::ViewManipulate (ImGuizmo.cpp:2779-3140): a real 3D cube pinned to
// the canvas bottom-left corner whose orientation tracks the camera. Presses
// and drags on the cube are consumed before any scene interaction (upstream
// ImGui handles the ImGuizmo hit before GLCanvas3D::on_mouse), face/edge/
// corner clicks snap the view through a 40-frame interpolation, and cube
// drags rotate the camera with a horizon clamp.

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
  const struct
  {
    const char *text;
    QVector3D axis;
  } axes[3] = {
      {"x", QVector3D(1, 0, 0)},
      {"y", QVector3D(0, 1, 0)},
      {"z", QVector3D(0, 0, 1)},
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
  if (m_navigatorHoverBox >= 0 && !m_navigatorHoverFaceNormal.isNull()) {
    const QString face = navigatorFaceName(m_navigatorHoverFaceNormal);
    if (!face.isEmpty()) {
      const QPointF p = NavigatorCube::projectToRect(
          m_navigatorHoverFaceNormal * 0.5f, basis, rect);
      labels.append(QVariantMap{
          {"kind", QStringLiteral("face")},
          {"text", face},
          {"x", p.x()},
          {"y", p.y()}});
    }
  }
  return labels;
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
}

void RhiViewport::mousePressEvent(QMouseEvent *event)
{
  // v5.16 (NAVIGATOR): the bottom-left cube consumes left presses before any
  // scene interaction (upstream ImGui processes the ImGuizmo hit first).
  if (event->button() == Qt::LeftButton
      && startNavigatorPress(event->position())) {
    event->accept();
    return;
  }
  if (event->button() == Qt::RightButton) {
    m_contextPressPosition = event->position();
    m_contextPressActive = true;
    m_contextDragExceeded = false;
    m_contextToolCapturedAtPress = activeToolCapturesContextGesture();
    m_contextLayerEditingAtPress = m_layerEditingInputActive;
    event->accept();
    return;
  }

  m_lastMousePosition = event->position();
  m_pressPosition = event->position();
  m_dragButton = event->button();
  m_pressPickedSourceObjectIndex = -1;
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
    // Phase 240 (GIZ-02): smart fill fires on PRESS only (a shift-drag keeps
    // the erase-brush path).
    m_paintClickPress = true;
    emitPaintPickIfActive(event->position(), event->modifiers());
    m_paintClickPress = false;
    event->accept();
    return;
  }

  // Phase 69/70: active gizmo hit tests take priority over object picking.
  if (event->button() == Qt::LeftButton &&
      (m_gizmoMode == GizmoMove || m_gizmoMode == GizmoRotate || m_gizmoMode == GizmoScale) &&
      m_selectedSourceObjectIndex >= 0)
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
    m_pressPickedSourceObjectIndex = pickSourceObjectAt(event->position());
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
    event->accept();
    return;
  }
  // Phase 120 (PAINT-01): continuous-paint-on-drag. While a paint gizmo is
  // active and the left button is held, every mouse-move drives the
  // TriangleSelector brush (mirrors upstream GLGizmoPainterBase on_mouse
  // move-while-LeftDown). mousePressEvent already accept()-ed the initial
  // click for paint gizmos, so m_gizmoDragging stays false here.
  if (m_dragButton == Qt::LeftButton &&
      (m_gizmoMode == GizmoSupportPaint ||
       m_gizmoMode == GizmoSeamPaint ||
       m_gizmoMode == GizmoMmuSegmentation)) {
    updateBrushCursorState(event->position(), 1 /*left*/);
    emitPaintPickIfActive(event->position(), event->modifiers());
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

  const QPointF delta = event->position() - m_lastMousePosition;
  // v5.12 gap-closure: cameraNavStyle Touchpad (1) swaps orbit/pan between
  // left and middle button (对齐 upstream camera_navigation_style=Touchpad).
  // Default (0): Left=orbit, Middle=pan. Touchpad (1): Left=pan, Middle=orbit.
  const bool touchpad = (m_cameraNavStyle == 1);
  const bool leftIsOrbit = !touchpad;
  if (m_dragButton == Qt::LeftButton) {
    if (leftIsOrbit && m_pressPickedSourceObjectIndex >= 0) {
      const QPointF pressDelta = event->position() - m_pressPosition;
      const bool becameDrag = std::hypot(pressDelta.x(), pressDelta.y()) > 4.0;
      if (!becameDrag) {
        setHoveredSourceObjectIndex(m_pressPickedSourceObjectIndex);
      } else {
        m_pressPickedSourceObjectIndex = -1;
        setHoveredSourceObjectIndex(pickSourceObjectAt(event->position()));
      }
    }
    if (leftIsOrbit) {
      if (m_pressPickedSourceObjectIndex < 0) {
        m_camera.orbit(float(delta.x()) * 0.5f, -float(delta.y()) * 0.5f);
        m_cameraDirty = true;
        update();
        emit navigatorLabelsChanged();
      }
    } else {
      // Touchpad: left drag = pan
      m_camera.pan(float(delta.x()), float(delta.y()));
      m_cameraDirty = true;
      update();
    }
  } else if (m_dragButton == Qt::MiddleButton) {
    if (touchpad) {
      // Touchpad: middle drag = orbit
      m_camera.orbit(float(delta.x()) * 0.5f, -float(delta.y()) * 0.5f);
      m_cameraDirty = true;
    } else {
      m_camera.pan(float(delta.x()), float(delta.y()));
      m_cameraDirty = true;
    }
    update();
    emit navigatorLabelsChanged();
  }
  m_lastMousePosition = event->position();
  event->accept();
}

void RhiViewport::mouseReleaseEvent(QMouseEvent *event)
{
  // v5.16 (NAVIGATOR): cube release either starts the snap interpolation
  // (click) or ends the drag; the scene handlers never see it.
  if (event->button() == Qt::LeftButton && m_navigatorPressActive) {
    finishNavigatorPress();
    event->accept();
    return;
  }
  if (event->button() == Qt::RightButton) {
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

  if (event->button() == Qt::LeftButton && m_pressPickedSourceObjectIndex >= 0) {
    const QPointF releaseDelta = event->position() - m_pressPosition;
    const bool isClick = std::hypot(releaseDelta.x(), releaseDelta.y()) <= 4.0;
    if (isClick && pickSourceObjectAt(event->position()) == m_pressPickedSourceObjectIndex)
      emit objectPickedSource(m_pressPickedSourceObjectIndex);
  }
  m_dragButton = Qt::NoButton;
  m_pressPickedSourceObjectIndex = -1;
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

void RhiViewport::hoverMoveEvent(QHoverEvent *event)
{
  // v5.16 (NAVIGATOR): cube hover highlight + face label (ImGuizmo hovered
  // face recolor, ImGuizmo.cpp:2923-2925).
  updateNavigatorHover(event->position(), false);
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
  // v5.12 gap-closure: reverseZoom inverts the wheel direction.
  float delta = float(event->angleDelta().y());
  if (m_reverseZoom)
    delta = -delta;
  m_camera.zoom(delta);
  m_cameraDirty = true;
  event->accept();
  update();
  emit navigatorLabelsChanged();
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
    includePoint(seg[i].x1, seg[i].z1, seg[i].y1);
    includePoint(seg[i].x2, seg[i].z2, seg[i].y2);
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

  return ObjectPicking::pickSourceObject(rayOrigin,
                                         rayDirection,
                                         m_pickScene.modelVertices(),
                                         m_pickScene.modelBatches());
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
  const ObjectPicking::Hit objectHit = ObjectPicking::pick(
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
// Phase 69/70: gizmo-axis picking helpers
// ===========================================================================
QVector3D RhiViewport::currentGizmoCenter() const
{
  // Reuse the extracted Phase 67 helper against the pick-scene batches.
  return GizmoCenter::fromSelectedBatch(m_selectedSourceObjectIndex,
                                        m_pickScene.modelBatches());
}

int RhiViewport::pickGizmoAxisAt(const QPointF &position)
{
  if (m_selectedSourceObjectIndex < 0)
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
  // EnforcerBlockerType: 1=Enforcer, 2=Blocker (TriangleSelector.hpp:13-38).
  const int    paintState  = shiftHeld ? 2 : m_paintState;
  const bool   smartFill   = (shiftHeld || m_paintToolType == 2) && m_paintClickPress;

  if (smartFill) {
    emit smartFillPickRequested(rayOrigin, rayDirection, pickedSourceIndex,
                                paintState, double(m_smartFillAngle),
                                m_paintOnOverhangsOnly,
                                double(m_paintOverhangAngle));
    return;
  }

  // Forward to QML opaquely (no ray math in QML -- same contract as
  // measurePickRequested). QML connects this to EditorViewModel::paintAtFacet.
  emit paintPickRequested(rayOrigin, rayDirection, m_camera.eye(),
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
  // 2D bed coordinates are (X,Z). Reject parallel and behind-camera hits.
  if (qFuzzyIsNull(dir.y()))
    return QPointF();
  const float t = -orig.y() / dir.y();
  if (t < 0.f)
    return QPointF();
  const QVector3D hit = orig + dir * t;
  return QPointF(double(hit.x()), double(hit.z()));
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
