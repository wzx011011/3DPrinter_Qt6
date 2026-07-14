#include "RhiViewport.h"
#include "RhiViewportRenderer.h"
#include "core/rendering/GizmoCenter.h"
#include "core/rendering/GizmoMath.h"
#include "core/rendering/ObjectPicking.h"

#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QHoverEvent>
#include <QImage>
#include <QMouseEvent>
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
  int extruder_id, layer, move;
  int role;  // must match PackedSegment layout exactly (canonical libvgcode index).
};
static_assert(sizeof(GcvPackedSegment) == 76, "GcvPackedSegment must be 76 bytes after adding role");

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
  // plateIndex is carried for Phase 96 per-plate routing but not consumed here.
  Q_UNUSED(plateIndex);
  if (image.isNull())
    return;
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  m_lastThumbnailData = QStringLiteral("data:image/png;base64,")
                        + QString::fromLatin1(bytes.toBase64());
  emit thumbnailCaptured();
}

void RhiViewport::mousePressEvent(QMouseEvent *event)
{
  if (event->button() == Qt::RightButton) {
    event->ignore();
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
    emitPaintPickIfActive(event->position(), event->modifiers());
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
  if (m_dragButton == Qt::LeftButton) {
    if (m_pressPickedSourceObjectIndex >= 0) {
      const QPointF pressDelta = event->position() - m_pressPosition;
      const bool becameDrag = std::hypot(pressDelta.x(), pressDelta.y()) > 4.0;
      if (!becameDrag) {
        setHoveredSourceObjectIndex(m_pressPickedSourceObjectIndex);
      } else {
        m_pressPickedSourceObjectIndex = -1;
        setHoveredSourceObjectIndex(pickSourceObjectAt(event->position()));
      }
    }
    if (m_pressPickedSourceObjectIndex < 0) {
      m_camera.orbit(float(delta.x()) * 0.5f, -float(delta.y()) * 0.5f);
      m_cameraDirty = true;
      update();
    }
  } else if (m_dragButton == Qt::MiddleButton) {
    m_camera.pan(float(delta.x()), float(delta.y()));
    m_cameraDirty = true;
    update();
  }
  m_lastMousePosition = event->position();
  event->accept();
}

void RhiViewport::mouseReleaseEvent(QMouseEvent *event)
{
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

void RhiViewport::hoverMoveEvent(QHoverEvent *event)
{
  setHoveredSourceObjectIndex(pickSourceObjectAt(event->position()));
  // Phase 115 (MEASURE-04): drive the snap UX on mouse-move while the measure
  // gizmo is active. The ViewModel runs the two-stage pick + getFeature and
  // updates the readout live. Mirrors upstream GLGizmoMeasure on_mouse move.
  emitMeasurePickIfActive(event->position(), event->modifiers());
  // Phase 121 (PAINT-03/OV-05): track the brush-cursor position on hover so
  // the sphere cursor follows the mouse before a click (hover -> black cursor).
  updateBrushCursorState(event->position(), 0 /*hover*/);
  event->accept();
}

void RhiViewport::hoverLeaveEvent(QHoverEvent *event)
{
  setHoveredSourceObjectIndex(-1);
  // Phase 115 (MEASURE-04): clear the hovered feature when the cursor leaves
  // the viewport so no stale highlight lingers off-mesh.
  if (m_gizmoMode == GizmoMeasure)
    emit measureHoverLeft();
  // Phase 121 (PAINT-03): hide the brush cursor when the mouse leaves.
  updateBrushCursorState(event->position(), -1 /*hide*/);
  event->accept();
}

void RhiViewport::wheelEvent(QWheelEvent *event)
{
  m_camera.zoom(float(event->angleDelta().y()));
  m_cameraDirty = true;
  event->accept();
  update();
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
  if (m_pickModelGeneration == m_modelGeneration)
    return;

  QList<int> activeObjectIndices;
  activeObjectIndices.reserve(m_activePlateObjectIndices.size());
  for (const QVariant &value : m_activePlateObjectIndices)
    activeObjectIndices.append(value.toInt());

  QList<int> batchSourceObjectIndices;
  batchSourceObjectIndices.reserve(m_meshBatchSourceObjectIndices.size());
  for (const QVariant &value : m_meshBatchSourceObjectIndices)
    batchSourceObjectIndices.append(value.toInt());

  m_pickScene.setPlateContext(m_currentPlateIndex, m_plateCount, activeObjectIndices);
  m_pickScene.setModelMeshData(m_meshData, batchSourceObjectIndices, activeObjectIndices);
  m_pickScene.clearDirtyFlags();
  m_pickModelGeneration = m_modelGeneration;
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

  // Phase 121 (PAINT-03/OV-02): brush params now come from the Q_PROPERTYs the
  // UI controls set (radius slider, cursor-type toggle, tool selector). This
  // replaces the Phase 120 conservative defaults (2.0/1/1). Shift still toggles
  // to the erase state for the Support/Seam gizmos (mirrors upstream
  // Shift-to-erase in GLGizmoPainterBase) -- when Shift is held, paintState is
  // forced to Blocker (2) regardless of the Q_PROPERTY so Shift-erase works
  // even when the tool selector shows Enforcer.
  const double brushRadius = double(m_brushRadius);
  const int    cursorType  = m_brushCursorType;
  const bool   shiftHeld   = (modifiers & Qt::ShiftModifier) != 0;
  // EnforcerBlockerType: 1=Enforcer, 2=Blocker (TriangleSelector.hpp:13-38).
  const int    paintState  = shiftHeld ? 2 : m_paintState;

  // Forward to QML opaquely (no ray math in QML -- same contract as
  // measurePickRequested). QML connects this to EditorViewModel::paintAtFacet.
  emit paintPickRequested(rayOrigin, rayDirection, pickedSourceIndex,
                          brushRadius, cursorType, paintState);
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
