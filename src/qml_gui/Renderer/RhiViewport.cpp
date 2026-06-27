#include "RhiViewport.h"
#include "RhiViewportRenderer.h"

#include <QBuffer>
#include <QByteArray>
#include <QColor>
#include <QImage>
#include <QtGlobal>

RhiViewport::RhiViewport(QQuickItem *parent)
    : QQuickRhiItem(parent)
{
  setAcceptedMouseButtons(Qt::AllButtons);
  setAcceptHoverEvents(false);
  setMirrorVertically(true);
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
  emit canvasTypeChanged();
  update();
}

void RhiViewport::setMeshData(const QByteArray &data)
{
  if (m_meshData == data)
    return;
  m_meshData = data;
  update();
}

void RhiViewport::setPreviewData(const QByteArray &data)
{
  if (m_previewData == data)
    return;
  m_previewData = data;
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
  update();
}

void RhiViewport::setBedWidth(float value)
{
  if (qFuzzyCompare(m_bedWidth, value))
    return;
  m_bedWidth = value;
  update();
}

void RhiViewport::setBedDepth(float value)
{
  if (qFuzzyCompare(m_bedDepth, value))
    return;
  m_bedDepth = value;
  update();
}

void RhiViewport::setBedOriginX(float value)
{
  if (qFuzzyCompare(m_bedOriginX, value))
    return;
  m_bedOriginX = value;
  update();
}

void RhiViewport::setBedOriginY(float value)
{
  if (qFuzzyCompare(m_bedOriginY, value))
    return;
  m_bedOriginY = value;
  update();
}

void RhiViewport::setBedShapeType(int value)
{
  if (m_bedShapeType == value)
    return;
  m_bedShapeType = value;
  update();
}

void RhiViewport::setBedDiameter(float value)
{
  if (qFuzzyCompare(m_bedDiameter, value))
    return;
  m_bedDiameter = value;
  update();
}

void RhiViewport::setCurrentPlateIndex(int value)
{
  if (m_currentPlateIndex == value)
    return;
  m_currentPlateIndex = value;
  update();
}

void RhiViewport::setPlateCount(int value)
{
  if (m_plateCount == value)
    return;
  m_plateCount = value;
  update();
}

void RhiViewport::setActivePlateObjectIndices(const QVariantList &value)
{
  if (m_activePlateObjectIndices == value)
    return;
  m_activePlateObjectIndices = value;
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
  Q_UNUSED(cx);
  Q_UNUSED(cy);
  Q_UNUSED(cz);
  Q_UNUSED(r);
  ++m_fitRequestCount;
  update();
}

void RhiViewport::requestViewPreset(int preset)
{
  m_viewPreset = preset;
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
