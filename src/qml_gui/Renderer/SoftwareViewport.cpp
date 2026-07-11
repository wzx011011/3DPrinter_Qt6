#include "SoftwareViewport.h"

#include <QBuffer>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace
{
constexpr float kBedSize = 220.f;

float degToRad(float value)
{
  return value * 3.1415926535f / 180.f;
}

QColor objectColor(int index)
{
  static const QColor colors[] = {
      QColor(242, 140, 56),
      QColor(59, 141, 221),
      QColor(76, 175, 116),
      QColor(155, 89, 182),
      QColor(231, 76, 60),
      QColor(26, 188, 156)};
  return colors[index % (sizeof(colors) / sizeof(colors[0]))];
}
}

SoftwareViewport::SoftwareViewport(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
  setAcceptedMouseButtons(Qt::AllButtons);
  setAcceptHoverEvents(false);
  setAntialiasing(true);
  setRenderTarget(QQuickPaintedItem::Image);
}

void SoftwareViewport::setCanvasType(int value)
{
  if (m_canvasType == value)
    return;
  m_canvasType = value;
  emit canvasTypeChanged();
  update();
}

void SoftwareViewport::setMeshData(const QByteArray &data)
{
  if (m_meshData == data)
    return;
  m_meshData = data;
  parseMeshData();
  fitToMeshes();
  update();
}

void SoftwareViewport::setPreviewData(const QByteArray &data)
{
  if (m_previewData == data)
    return;
  m_previewData = data;
  update();
}

void SoftwareViewport::setLayerMin(int value)
{
  if (m_layerMin == value)
    return;
  m_layerMin = value;
  update();
}

void SoftwareViewport::setLayerMax(int value)
{
  if (m_layerMax == value)
    return;
  m_layerMax = value;
  update();
}

void SoftwareViewport::setMoveEnd(int value)
{
  if (m_moveEnd == value)
    return;
  m_moveEnd = value;
  update();
}

void SoftwareViewport::setShowTravelMoves(bool value)
{
  if (m_showTravelMoves == value)
    return;
  m_showTravelMoves = value;
  update();
}

void SoftwareViewport::setShowBed(bool value)
{
  if (m_showBed == value)
    return;
  m_showBed = value;
  update();
}

void SoftwareViewport::setBedWidth(float value)
{
  if (qFuzzyCompare(m_bedWidth, value))
    return;
  m_bedWidth = value;
  update();
}

void SoftwareViewport::setBedDepth(float value)
{
  if (qFuzzyCompare(m_bedDepth, value))
    return;
  m_bedDepth = value;
  update();
}

void SoftwareViewport::setBedOriginX(float value)
{
  if (qFuzzyCompare(m_bedOriginX, value))
    return;
  m_bedOriginX = value;
  update();
}

void SoftwareViewport::setBedOriginY(float value)
{
  if (qFuzzyCompare(m_bedOriginY, value))
    return;
  m_bedOriginY = value;
  update();
}

void SoftwareViewport::setBedShapeType(int value)
{
  if (m_bedShapeType == value)
    return;
  m_bedShapeType = value;
  update();
}

void SoftwareViewport::setBedDiameter(float value)
{
  if (qFuzzyCompare(m_bedDiameter, value))
    return;
  m_bedDiameter = value;
  update();
}

void SoftwareViewport::setCurrentPlateIndex(int value)
{
  if (m_currentPlateIndex == value)
    return;
  m_currentPlateIndex = value;
  update();
}

void SoftwareViewport::setPlateCount(int value)
{
  if (m_plateCount == value)
    return;
  m_plateCount = value;
  update();
}

void SoftwareViewport::setActivePlateObjectIndices(const QVariantList &value)
{
  if (m_activePlateObjectIndices == value)
    return;
  m_activePlateObjectIndices = value;
  update();
}

void SoftwareViewport::setMeshBatchSourceObjectIndices(const QVariantList &value)
{
  if (m_meshBatchSourceObjectIndices == value)
    return;
  m_meshBatchSourceObjectIndices = value;
  update();
}

void SoftwareViewport::setSelectedSourceObjectIndex(int value)
{
  if (m_selectedSourceObjectIndex == value)
    return;
  m_selectedSourceObjectIndex = value;
  update();
}

void SoftwareViewport::setHoveredSourceObjectIndex(int value)
{
  if (m_hoveredSourceObjectIndex == value)
    return;
  m_hoveredSourceObjectIndex = value;
  update();
}

void SoftwareViewport::setShowWipeTower(bool value)
{
  if (m_showWipeTower == value)
    return;
  m_showWipeTower = value;
  update();
}

void SoftwareViewport::setWipeTowerWidth(float value)
{
  if (qFuzzyCompare(m_wipeTowerWidth, value))
    return;
  m_wipeTowerWidth = value;
  update();
}

void SoftwareViewport::setWipeTowerDepth(float value)
{
  if (qFuzzyCompare(m_wipeTowerDepth, value))
    return;
  m_wipeTowerDepth = value;
  update();
}

void SoftwareViewport::setWipeTowerHeight(float value)
{
  if (qFuzzyCompare(m_wipeTowerHeight, value))
    return;
  m_wipeTowerHeight = value;
  update();
}

void SoftwareViewport::setWipeTowerX(float value)
{
  if (qFuzzyCompare(m_wipeTowerX, value))
    return;
  m_wipeTowerX = value;
  update();
}

void SoftwareViewport::setWipeTowerZ(float value)
{
  if (qFuzzyCompare(m_wipeTowerZ, value))
    return;
  m_wipeTowerZ = value;
  update();
}

void SoftwareViewport::setGcodeViewMode(int value)
{
  if (m_gcodeViewMode == value)
    return;
  m_gcodeViewMode = value;
  emit gcodeViewModeChanged();
  update();
}

void SoftwareViewport::setMarkerX(float value)
{
  if (qFuzzyCompare(m_markerX, value))
    return;
  m_markerX = value;
  update();
}

void SoftwareViewport::setMarkerY(float value)
{
  if (qFuzzyCompare(m_markerY, value))
    return;
  m_markerY = value;
  update();
}

void SoftwareViewport::setMarkerZ(float value)
{
  if (qFuzzyCompare(m_markerZ, value))
    return;
  m_markerZ = value;
  update();
}

void SoftwareViewport::setShowMarker(bool value)
{
  if (m_showMarker == value)
    return;
  m_showMarker = value;
  update();
}

void SoftwareViewport::setGizmoMode(int value)
{
  if (m_gizmoMode == value)
    return;
  m_gizmoMode = value;
  emit gizmoModeChanged();
  update();
}

void SoftwareViewport::setWireframeMode(bool value)
{
  if (m_wireframeMode == value)
    return;
  m_wireframeMode = value;
  emit wireframeModeChanged();
  update();
}

void SoftwareViewport::setCutAxis(int value)
{
  if (m_cutAxis == value)
    return;
  m_cutAxis = value;
  update();
}

void SoftwareViewport::setCutPosition(float value)
{
  if (qFuzzyCompare(m_cutPosition, value))
    return;
  m_cutPosition = value;
  update();
}

void SoftwareViewport::requestFitView(float cx, float cy, float cz, float r)
{
  m_center = QVector3D(cx, cy, cz);
  m_radius = std::max(20.f, r);
  m_pan = QPointF();
  m_zoom = 1.f;
  update();
}

void SoftwareViewport::requestViewPreset(int preset)
{
  switch (preset)
  {
  case 0:
    m_yaw = 0.f;
    m_pitch = 90.f;
    break;
  case 1:
    m_yaw = 0.f;
    m_pitch = 0.f;
    break;
  case 2:
    m_yaw = 90.f;
    m_pitch = 0.f;
    break;
  default:
    m_yaw = -35.f;
    m_pitch = 55.f;
    break;
  }
  update();
}

void SoftwareViewport::requestThumbnailCapture(int, int size)
{
  const int side = std::max(32, size);
  QImage image(side, side, QImage::Format_ARGB32_Premultiplied);
  image.fill(QColor("#202733"));
  QPainter painter(&image);
  paintScene(&painter, QRectF(0, 0, side, side));
  painter.end();

  QByteArray bytes;
  QBuffer buffer(&bytes);
  buffer.open(QIODevice::WriteOnly);
  image.save(&buffer, "PNG");
  m_lastThumbnailData = QStringLiteral("data:image/png;base64,") + QString::fromLatin1(bytes.toBase64());
  emit thumbnailCaptured();
}

void SoftwareViewport::paint(QPainter *painter)
{
  paintScene(painter, contentRect());
}

QRectF SoftwareViewport::contentRect() const
{
  return QRectF(0, 0, width(), height());
}

void SoftwareViewport::paintScene(QPainter *painter, const QRectF &target)
{
  painter->save();
  painter->setRenderHint(QPainter::Antialiasing, true);
  painter->fillRect(target, QColor("#56575d"));

  auto drawLine3D = [&](const QVector3D &a, const QVector3D &b, const QPen &pen) {
    painter->setPen(pen);
    painter->drawLine(project(a), project(b));
  };

  const QPen finePen(QColor(94, 108, 122, 120), 1);
  const QPen coarsePen(QColor(124, 142, 158, 160), 1.2);
  const QPen borderPen(QColor(195, 212, 224, 210), 2);
  for (float t = 0.f; t <= kBedSize + 0.1f; t += 10.f)
  {
    const bool coarse = std::fmod(t, 50.f) < 0.1f || std::abs(std::fmod(t, 50.f) - 50.f) < 0.1f;
    drawLine3D(QVector3D(t, 0, 0), QVector3D(t, 0, kBedSize), coarse ? coarsePen : finePen);
    drawLine3D(QVector3D(0, 0, t), QVector3D(kBedSize, 0, t), coarse ? coarsePen : finePen);
  }
  drawLine3D(QVector3D(0, 0, 0), QVector3D(kBedSize, 0, 0), borderPen);
  drawLine3D(QVector3D(kBedSize, 0, 0), QVector3D(kBedSize, 0, kBedSize), borderPen);
  drawLine3D(QVector3D(kBedSize, 0, kBedSize), QVector3D(0, 0, kBedSize), borderPen);
  drawLine3D(QVector3D(0, 0, kBedSize), QVector3D(0, 0, 0), borderPen);

  struct Face
  {
    QPolygonF poly;
    QColor color;
    float depth = 0.f;
  };
  QVector<Face> faces;
  for (int objectIndex = 0; objectIndex < m_meshes.size(); ++objectIndex)
  {
    const auto &mesh = m_meshes[objectIndex];
    const QColor base = objectColor(objectIndex);
    for (int i = 0; i + 2 < mesh.vertices.size(); i += 3)
    {
      const QVector3D a = rotatePoint(mesh.vertices[i] - m_center);
      const QVector3D b = rotatePoint(mesh.vertices[i + 1] - m_center);
      const QVector3D c = rotatePoint(mesh.vertices[i + 2] - m_center);
      const QVector3D normal = QVector3D::crossProduct(b - a, c - a);
      const float light = std::clamp(0.58f + normal.normalized().z() * 0.30f - normal.normalized().y() * 0.12f, 0.35f, 1.0f);
      QColor shaded = base;
      shaded.setRedF(std::clamp(shaded.redF() * light, 0.f, 1.f));
      shaded.setGreenF(std::clamp(shaded.greenF() * light, 0.f, 1.f));
      shaded.setBlueF(std::clamp(shaded.blueF() * light, 0.f, 1.f));
      shaded.setAlpha(225);

      Face face;
      face.poly << project(mesh.vertices[i]) << project(mesh.vertices[i + 1]) << project(mesh.vertices[i + 2]);
      face.color = shaded;
      face.depth = (a.z() + b.z() + c.z()) / 3.f;
      faces.append(face);
    }
  }

  // Phase 101 (WTRENDER-01): render the real-dims wipe-tower box on the
  // software fallback path so it does not lag the RHI path
  // (RhiViewportRenderer::uploadWipeTowerBuffer at .cpp:1064-1095). Gated on
  // m_showWipeTower (WTREAD-02 - no placeholder leak on single-material
  // slices). The dims arrive as the box CENTER (Phase 100 REVIEW W1
  // convention, commit b12d0e5). The color {0.35,0.60,0.85,0.50} and
  // kGroundY mirror the RHI box (GizmoGeometry.cpp:462-463); the geometry
  // (x-hw..x+hw, z-hd..z+hd, y0..y1, Y-up) mirrors buildWipeTowerVertices.
  if (m_showWipeTower && m_wipeTowerWidth > 0.f &&
      m_wipeTowerDepth > 0.f && m_wipeTowerHeight > 0.f)
  {
    constexpr float kGroundY = -0.04f; // mirrors GizmoGeometry.cpp:462
    const QColor wipeTowerColor = QColor::fromRgbF(0.35f, 0.60f, 0.85f, 0.50f);
    const float hw = m_wipeTowerWidth * 0.5f;
    const float hd = m_wipeTowerDepth * 0.5f;
    const float xMin = m_wipeTowerX - hw;
    const float xMax = m_wipeTowerX + hw;
    const float zMin = m_wipeTowerZ - hd;
    const float zMax = m_wipeTowerZ + hd;
    const float y0 = kGroundY;
    const float y1 = kGroundY + m_wipeTowerHeight;
    // 8 box corners (world space: X/Z bed plane, Y up).
    const QVector3D c000(xMin, y0, zMin);
    const QVector3D c100(xMax, y0, zMin);
    const QVector3D c110(xMax, y0, zMax);
    const QVector3D c010(xMin, y0, zMax);
    const QVector3D c001(xMin, y1, zMin);
    const QVector3D c101(xMax, y1, zMin);
    const QVector3D c111(xMax, y1, zMax);
    const QVector3D c011(xMin, y1, zMax);
    // Emit the 5 visible faces (top + 4 sides; bottom is coplanar with the
    // bed grid and hidden) into the unified depth-sorted `faces` vector so
    // they render correctly alongside model meshes (painter's algorithm).
    auto addWipeTowerFace = [&](const QVector3D &p0, const QVector3D &p1,
                                const QVector3D &p2, const QVector3D &p3) {
      const QVector3D r0 = rotatePoint(p0 - m_center);
      const QVector3D r1 = rotatePoint(p1 - m_center);
      const QVector3D r2 = rotatePoint(p2 - m_center);
      const QVector3D r3 = rotatePoint(p3 - m_center);
      Face face;
      face.poly << project(p0) << project(p1) << project(p2) << project(p3);
      face.color = wipeTowerColor;
      face.depth = (r0.z() + r1.z() + r2.z() + r3.z()) * 0.25f;
      faces.append(face);
    };
    addWipeTowerFace(c001, c101, c111, c011); // top
    addWipeTowerFace(c010, c110, c111, c011); // front (+Z)
    addWipeTowerFace(c100, c000, c001, c101); // back (-Z)
    addWipeTowerFace(c110, c100, c101, c111); // right (+X)
    addWipeTowerFace(c000, c010, c011, c001); // left (-X)
  }

  std::sort(faces.begin(), faces.end(), [](const Face &lhs, const Face &rhs) {
    return lhs.depth < rhs.depth;
  });
  for (const Face &face : faces)
  {
    painter->setPen(m_wireframeMode ? QPen(QColor(240, 246, 252, 160), 1) : QPen(QColor(20, 24, 28, 70), 0.6));
    painter->setBrush(m_wireframeMode ? Qt::NoBrush : QBrush(face.color));
    painter->drawPolygon(face.poly);
  }

  painter->setPen(QColor(218, 226, 234));
  painter->setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10));
  painter->drawText(target.adjusted(14, 12, -14, -12),
                    Qt::AlignLeft | Qt::AlignTop,
                    m_meshes.isEmpty()
                        ? tr("Software viewport - drag to rotate, wheel to zoom")
                        : tr("Software viewport - %1 object(s)").arg(m_meshes.size()));
  painter->restore();
}

QVector3D SoftwareViewport::rotatePoint(const QVector3D &point) const
{
  const float yaw = degToRad(m_yaw);
  const float pitch = degToRad(m_pitch);
  const float cy = std::cos(yaw);
  const float sy = std::sin(yaw);
  const float cp = std::cos(pitch);
  const float sp = std::sin(pitch);

  QVector3D p(point.x() * cy - point.z() * sy,
              point.y(),
              point.x() * sy + point.z() * cy);
  return QVector3D(p.x(), p.y() * cp - p.z() * sp, p.y() * sp + p.z() * cp);
}

QPointF SoftwareViewport::project(const QVector3D &point) const
{
  const QRectF rect = contentRect();
  const QVector3D p = rotatePoint(point - m_center);
  const float scale = (std::min(rect.width(), rect.height()) * 0.42f / std::max(30.f, m_radius)) * m_zoom;
  return QPointF(rect.center().x() + m_pan.x() + p.x() * scale,
                 rect.center().y() + m_pan.y() - p.y() * scale);
}

void SoftwareViewport::parseMeshData()
{
  m_meshes.clear();
  if (m_meshData.size() < int(sizeof(int32_t)))
    return;

  const char *p = m_meshData.constData();
  const char *end = p + m_meshData.size();
  int32_t objectCount = 0;
  memcpy(&objectCount, p, sizeof(int32_t));
  p += sizeof(int32_t);
  if (objectCount <= 0 || objectCount > 10000)
    return;

  for (int32_t objectIndex = 0; objectIndex < objectCount; ++objectIndex)
  {
    if (p + 2 * int(sizeof(int32_t)) > end)
      break;
    int32_t objectId = 0;
    int32_t triCount = 0;
    memcpy(&objectId, p, sizeof(int32_t));
    p += sizeof(int32_t);
    memcpy(&triCount, p, sizeof(int32_t));
    p += sizeof(int32_t);
    if (triCount <= 0)
      continue;
    const qsizetype floatCount = qsizetype(triCount) * 9;
    const qsizetype byteCount = floatCount * qsizetype(sizeof(float));
    if (byteCount < 0 || p + byteCount > end)
      break;

    MeshObject mesh;
    mesh.objectId = objectId;
    mesh.vertices.reserve(int(triCount) * 3);
    const float *values = reinterpret_cast<const float *>(p);
    mesh.min = QVector3D(std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max(),
                         std::numeric_limits<float>::max());
    mesh.max = QVector3D(-std::numeric_limits<float>::max(),
                         -std::numeric_limits<float>::max(),
                         -std::numeric_limits<float>::max());
    for (int i = 0; i < triCount * 3; ++i)
    {
      QVector3D v(values[i * 3], values[i * 3 + 1], values[i * 3 + 2]);
      mesh.vertices.append(v);
      mesh.min.setX(std::min(mesh.min.x(), v.x()));
      mesh.min.setY(std::min(mesh.min.y(), v.y()));
      mesh.min.setZ(std::min(mesh.min.z(), v.z()));
      mesh.max.setX(std::max(mesh.max.x(), v.x()));
      mesh.max.setY(std::max(mesh.max.y(), v.y()));
      mesh.max.setZ(std::max(mesh.max.z(), v.z()));
    }
    m_meshes.append(mesh);
    p += byteCount;
  }
}

void SoftwareViewport::fitToMeshes()
{
  if (m_meshes.isEmpty())
  {
    m_center = QVector3D(kBedSize * 0.5f, 0.f, kBedSize * 0.5f);
    m_radius = 170.f;
    return;
  }

  QVector3D min(std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max(),
                std::numeric_limits<float>::max());
  QVector3D max(-std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max(),
                -std::numeric_limits<float>::max());
  for (const auto &mesh : m_meshes)
  {
    min.setX(std::min(min.x(), mesh.min.x()));
    min.setY(std::min(min.y(), mesh.min.y()));
    min.setZ(std::min(min.z(), mesh.min.z()));
    max.setX(std::max(max.x(), mesh.max.x()));
    max.setY(std::max(max.y(), mesh.max.y()));
    max.setZ(std::max(max.z(), mesh.max.z()));
  }
  m_center = (min + max) * 0.5f;
  const QVector3D span = max - min;
  m_radius = std::max(40.f, span.length() * 0.55f);
}

void SoftwareViewport::mousePressEvent(QMouseEvent *event)
{
  m_lastMouse = event->position();
  m_dragButton = event->button();
  event->accept();
}

void SoftwareViewport::mouseMoveEvent(QMouseEvent *event)
{
  const QPointF delta = event->position() - m_lastMouse;
  if (m_dragButton == Qt::LeftButton)
  {
    m_yaw += float(delta.x()) * 0.45f;
    m_pitch = std::clamp(m_pitch + float(delta.y()) * 0.35f, -85.f, 85.f);
  }
  else if (m_dragButton == Qt::MiddleButton || m_dragButton == Qt::RightButton)
  {
    m_pan += delta;
  }
  m_lastMouse = event->position();
  event->accept();
  update();
}

void SoftwareViewport::mouseReleaseEvent(QMouseEvent *event)
{
  m_dragButton = Qt::NoButton;
  event->accept();
}

void SoftwareViewport::wheelEvent(QWheelEvent *event)
{
  const float steps = float(event->angleDelta().y()) / 120.f;
  m_zoom = std::clamp(m_zoom * std::pow(1.12f, steps), 0.15f, 12.f);
  event->accept();
  update();
}
