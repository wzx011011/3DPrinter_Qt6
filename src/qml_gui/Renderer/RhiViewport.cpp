#include "RhiViewport.h"
#include "RhiViewportRenderer.h"
#include "core/rendering/GizmoCenter.h"
#include "core/rendering/GizmoMath.h"

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

void RhiViewport::requestFitView(float cx, float cy, float cz, float r)
{
  m_camera.fitView(cx, cy, cz, r);
  m_cameraDirty = true;
  ++m_fitRequestCount;
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
  Q_UNUSED(plateIndex);
  const int side = qMax(32, size);
  QImage image(side, side, QImage::Format_ARGB32_Premultiplied);
  image.fill(QColor("#18222c"));
  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  m_lastThumbnailData = QStringLiteral("data:image/png;base64,") + QString::fromLatin1(bytes.toBase64());
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
  m_gizmoDragging = false;
  m_gizmoAxis = 0;
  m_gizmoDragStartT = 0.f;
  m_gizmoDragCenter = {};

  // Phase 69: move-axis hit test takes priority over object picking when
  // the move gizmo is active and an object is selected.
  if (event->button() == Qt::LeftButton && m_gizmoMode == GizmoMove &&
      m_selectedSourceObjectIndex >= 0)
  {
    const int axis = pickGizmoAxisAt(event->position());
    if (axis != 0)
    {
      m_gizmoAxis = axis;
      m_gizmoDragging = true;
      m_gizmoDragCenter = currentGizmoCenter();
      const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
      const float aspect = float(viewSize.width()) / float(viewSize.height());
      auto [orig, dir] = GizmoMath::computeRay(
          float(event->position().x()), float(event->position().y()),
          viewSize,
          m_camera.projMatrix(aspect), m_camera.viewMatrix());
      static const QVector3D kAxes[3] = {
          QVector3D(1, 0, 0), QVector3D(0, 1, 0), QVector3D(0, 0, 1)};
      m_gizmoDragStartT = GizmoMath::rayToAxisT(orig, dir, kAxes[axis - 1],
                                                m_gizmoDragCenter);
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
  // Phase 69: active gizmo drag translates the selected object along the
  // picked axis. Consumes the event so camera orbit never fires mid-drag.
  if (m_gizmoDragging && m_dragButton == Qt::LeftButton)
  {
    if (m_gizmoAxis < 1 || m_gizmoAxis > 3)
    {
      m_gizmoDragging = false;
      m_gizmoAxis = 0;
      m_gizmoDragStartT = 0.f;
      m_gizmoDragCenter = {};
      emit gizmoDragEnd();
      event->accept();
      return;
    }

    const QSize viewSize{std::max(1, int(width())), std::max(1, int(height()))};
    const float aspect = float(std::max(1, viewSize.width())) / float(std::max(1, viewSize.height()));
    auto [orig, dir] = GizmoMath::computeRay(
        float(event->position().x()), float(event->position().y()), viewSize,
        m_camera.projMatrix(aspect), m_camera.viewMatrix());
    static const QVector3D kAxes[3] = {
        QVector3D(1, 0, 0), QVector3D(0, 1, 0), QVector3D(0, 0, 1)};
    const QVector3D axisDir = kAxes[m_gizmoAxis - 1];
    const float curT = GizmoMath::rayToAxisT(orig, dir, axisDir, m_gizmoDragCenter);
    const float worldDeltaT = curT - m_gizmoDragStartT;
    m_gizmoDragStartT = curT;
    const QVector3D frameDelta = axisDir * worldDeltaT;
    emit gizmoMoveRequested(frameDelta);
    m_lastMousePosition = event->position();
    event->accept();
    return;
  }

  const QPointF delta = event->position() - m_lastMousePosition;
  if (m_dragButton == Qt::LeftButton) {
    if (m_pressPickedSourceObjectIndex >= 0) {
      setHoveredSourceObjectIndex(m_pressPickedSourceObjectIndex);
    } else {
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
    m_gizmoDragging = false;
    m_gizmoAxis = 0;
    m_gizmoDragStartT = 0.f;
    m_gizmoDragCenter = {};
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
  event->accept();
}

void RhiViewport::hoverMoveEvent(QHoverEvent *event)
{
  setHoveredSourceObjectIndex(pickSourceObjectAt(event->position()));
  event->accept();
}

void RhiViewport::hoverLeaveEvent(QHoverEvent *event)
{
  setHoveredSourceObjectIndex(-1);
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

  int pickedSourceObjectIndex = -1;
  float nearestDepth = std::numeric_limits<float>::max();
  for (const PrepareSceneData::ModelBatch &batch : m_pickScene.modelBatches()) {
    if (batch.sourceObjectIndex < 0)
      continue;

    float depth = 0.0f;
    const QRectF screenRect = projectBoundsToScreenRect(batch.bounds, &depth);
    if (!screenRect.isValid() || !screenRect.contains(position))
      continue;

    if (depth < nearestDepth) {
      nearestDepth = depth;
      pickedSourceObjectIndex = batch.sourceObjectIndex;
    }
  }

  return pickedSourceObjectIndex;
}

QRectF RhiViewport::projectBoundsToScreenRect(const PrepareSceneData::ModelBounds &bounds,
                                              float *depth) const
{
  const float w = float(width());
  const float h = float(height());
  if (w <= 1.0f || h <= 1.0f)
    return {};

  const float aspect = w / h;
  const QMatrix4x4 mvp = cameraMvp(aspect);
  const QVector4D corners[] = {
      QVector4D(bounds.minX, bounds.minY, bounds.minZ, 1.0f),
      QVector4D(bounds.maxX, bounds.minY, bounds.minZ, 1.0f),
      QVector4D(bounds.minX, bounds.maxY, bounds.minZ, 1.0f),
      QVector4D(bounds.maxX, bounds.maxY, bounds.minZ, 1.0f),
      QVector4D(bounds.minX, bounds.minY, bounds.maxZ, 1.0f),
      QVector4D(bounds.maxX, bounds.minY, bounds.maxZ, 1.0f),
      QVector4D(bounds.minX, bounds.maxY, bounds.maxZ, 1.0f),
      QVector4D(bounds.maxX, bounds.maxY, bounds.maxZ, 1.0f),
  };

  bool hasPoint = false;
  float left = std::numeric_limits<float>::max();
  float top = std::numeric_limits<float>::max();
  float right = std::numeric_limits<float>::lowest();
  float bottom = std::numeric_limits<float>::lowest();
  float nearestDepth = std::numeric_limits<float>::max();

  for (const QVector4D &corner : corners) {
    const QVector4D clip = mvp * corner;
    if (clip.w() <= 0.0001f)
      continue;

    const float invW = 1.0f / clip.w();
    const float ndcX = clip.x() * invW;
    const float ndcY = clip.y() * invW;
    const float ndcZ = clip.z() * invW;
    if (!std::isfinite(ndcX) || !std::isfinite(ndcY) || !std::isfinite(ndcZ))
      continue;

    const float screenX = (ndcX * 0.5f + 0.5f) * w;
    const float screenY = (1.0f - (ndcY * 0.5f + 0.5f)) * h;
    left = std::min(left, screenX);
    right = std::max(right, screenX);
    top = std::min(top, screenY);
    bottom = std::max(bottom, screenY);
    nearestDepth = std::min(nearestDepth, ndcZ);
    hasPoint = true;
  }

  if (!hasPoint)
    return {};

  if (depth)
    *depth = nearestDepth;

  return QRectF(QPointF(left, top), QPointF(right, bottom)).normalized().adjusted(-3.0, -3.0, 3.0, 3.0);
}

// ===========================================================================
// Phase 69: gizmo-axis picking helpers
// ===========================================================================
QVector3D RhiViewport::currentGizmoCenter() const
{
  // Reuse the extracted Phase 67 helper against the pick-scene batches.
  return GizmoCenter::fromSelectedBatch(m_selectedSourceObjectIndex,
                                        m_pickScene.modelBatches());
}

int RhiViewport::pickGizmoAxisAt(const QPointF &position)
{
  // Only meaningful in Move mode with an active selection.
  if (m_gizmoMode != GizmoMove || m_selectedSourceObjectIndex < 0)
    return 0;
  updatePickingScene();
  const QSize viewSize{int(width()), int(height())};
  if (viewSize.width() <= 0 || viewSize.height() <= 0)
    return 0;
  const float aspect = float(viewSize.width()) / float(viewSize.height());
  return GizmoMath::pickMoveAxis(
      float(position.x()), float(position.y()), viewSize,
      m_camera.projMatrix(aspect), m_camera.viewMatrix(),
      currentGizmoCenter(), m_camera.eye(),
      /*hasSelection=*/true);
}
