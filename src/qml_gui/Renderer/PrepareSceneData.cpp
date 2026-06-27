#include "PrepareSceneData.h"

#include <algorithm>
#include <cmath>

namespace
{
  constexpr float kDefaultBedSizeMm = 220.0f;
  constexpr float kMinBedSizeMm = 1.0f;
  constexpr float kMaxBedSizeMm = 2000.0f;
  constexpr float kFineGridMm = 10.0f;
  constexpr float kCoarseGridMm = 50.0f;

  constexpr float kFillR = 0.12f;
  constexpr float kFillG = 0.14f;
  constexpr float kFillB = 0.16f;
  constexpr float kFillA = 0.70f;

  constexpr float kBorderR = 0.56f;
  constexpr float kBorderG = 0.63f;
  constexpr float kBorderB = 0.70f;
  constexpr float kGridFine = 0.24f;
  constexpr float kGridCoarse = 0.34f;
  constexpr float kAxisR = 0.12f;
  constexpr float kAxisG = 0.78f;
  constexpr float kAxisB = 0.37f;
}

bool PrepareSceneData::Vertex::operator==(const Vertex &other) const
{
  return qFuzzyCompare(x, other.x)
      && qFuzzyCompare(y, other.y)
      && qFuzzyCompare(r, other.r)
      && qFuzzyCompare(g, other.g)
      && qFuzzyCompare(b, other.b)
      && qFuzzyCompare(a, other.a);
}

PrepareSceneData::PrepareSceneData()
{
  rebuildBedGeometry();
  markDirty(DirtyBed | DirtyGpu);
}

void PrepareSceneData::setBed(float widthMm,
                              float depthMm,
                              float originX,
                              float originY,
                              int shapeType,
                              float diameterMm)
{
  const float width = sanitizeExtent(widthMm, kDefaultBedSizeMm);
  const float depth = sanitizeExtent(depthMm, kDefaultBedSizeMm);
  const float diameter = sanitizeExtent(diameterMm, std::min(width, depth));

  if (nearlyEqual(m_bedWidth, width)
      && nearlyEqual(m_bedDepth, depth)
      && nearlyEqual(m_bedOriginX, originX)
      && nearlyEqual(m_bedOriginY, originY)
      && m_bedShapeType == shapeType
      && nearlyEqual(m_bedDiameter, diameter)) {
    return;
  }

  m_bedWidth = width;
  m_bedDepth = depth;
  m_bedOriginX = originX;
  m_bedOriginY = originY;
  m_bedShapeType = shapeType;
  m_bedDiameter = diameter;
  rebuildBedGeometry();
  markDirty(DirtyBed | DirtyGpu);
}

void PrepareSceneData::setShowBed(bool showBed)
{
  if (m_showBed == showBed)
    return;

  m_showBed = showBed;
  markDirty(DirtyVisibility | DirtyGpu);
}

void PrepareSceneData::setPlateContext(int currentPlateIndex,
                                       int plateCount,
                                       const QList<int> &activeObjectIndices)
{
  const int normalizedPlateCount = std::max(0, plateCount);
  const bool validPlate = currentPlateIndex >= 0 && currentPlateIndex < normalizedPlateCount;
  const int normalizedCurrentPlate = validPlate ? currentPlateIndex : -1;
  const QList<int> normalizedObjects = validPlate ? activeObjectIndices : QList<int>{};

  if (m_currentPlateIndex == normalizedCurrentPlate
      && m_plateCount == normalizedPlateCount
      && sameObjectIndices(m_activeObjectIndices, normalizedObjects)) {
    return;
  }

  m_currentPlateIndex = normalizedCurrentPlate;
  m_plateCount = normalizedPlateCount;
  m_activeObjectIndices = normalizedObjects;
  markDirty(DirtyPlate | DirtyGpu);
}

void PrepareSceneData::setMeshGeneration(qint64 generation)
{
  if (m_meshGeneration == generation)
    return;

  m_meshGeneration = generation;
  markDirty(DirtyMesh | DirtyGpu);
}

quint32 PrepareSceneData::peekDirtyFlags() const
{
  return m_dirtyFlags;
}

quint32 PrepareSceneData::takeDirtyFlags()
{
  const quint32 flags = m_dirtyFlags;
  m_dirtyFlags = DirtyNone;
  return flags;
}

void PrepareSceneData::clearDirtyFlags()
{
  m_dirtyFlags = DirtyNone;
}

float PrepareSceneData::bedWidth() const
{
  return m_bedWidth;
}

float PrepareSceneData::bedDepth() const
{
  return m_bedDepth;
}

float PrepareSceneData::bedOriginX() const
{
  return m_bedOriginX;
}

float PrepareSceneData::bedOriginY() const
{
  return m_bedOriginY;
}

int PrepareSceneData::bedShapeType() const
{
  return m_bedShapeType;
}

float PrepareSceneData::bedDiameter() const
{
  return m_bedDiameter;
}

bool PrepareSceneData::showBed() const
{
  return m_showBed;
}

float PrepareSceneData::fineGridSpacingMm() const
{
  return kFineGridMm;
}

float PrepareSceneData::coarseGridSpacingMm() const
{
  return kCoarseGridMm;
}

int PrepareSceneData::currentPlateIndex() const
{
  return m_currentPlateIndex;
}

int PrepareSceneData::plateCount() const
{
  return m_plateCount;
}

const QList<int> &PrepareSceneData::activeObjectIndices() const
{
  return m_activeObjectIndices;
}

qint64 PrepareSceneData::meshGeneration() const
{
  return m_meshGeneration;
}

const QList<PrepareSceneData::Vertex> &PrepareSceneData::bedFillVertices() const
{
  return m_bedFillVertices;
}

const QList<PrepareSceneData::Vertex> &PrepareSceneData::bedLineVertices() const
{
  return m_bedLineVertices;
}

float PrepareSceneData::sanitizeExtent(float value, float fallback)
{
  if (!std::isfinite(value))
    value = fallback;
  return std::clamp(value, kMinBedSizeMm, kMaxBedSizeMm);
}

bool PrepareSceneData::nearlyEqual(float left, float right)
{
  return std::abs(left - right) <= 0.0001f;
}

bool PrepareSceneData::sameObjectIndices(const QList<int> &left, const QList<int> &right)
{
  if (left.size() != right.size())
    return false;
  for (int i = 0; i < left.size(); ++i) {
    if (left.at(i) != right.at(i))
      return false;
  }
  return true;
}

void PrepareSceneData::markDirty(quint32 flags)
{
  m_dirtyFlags |= flags;
}

void PrepareSceneData::rebuildBedGeometry()
{
  m_bedFillVertices.clear();
  m_bedLineVertices.clear();

  if (!m_showBed)
    return;

  // Functional source-truth mapping: upstream 3DBed/PartPlate/GLCanvas3D render
  // a plate footprint with border, grid, and origin axes. QRhi owns only the
  // transport; these deterministic vertices keep that behavior testable.
  const float left = m_bedOriginX;
  const float top = m_bedOriginY;
  const float right = left + m_bedWidth;
  const float bottom = top + m_bedDepth;

  appendRectFill(left, top, right, bottom);
  appendRectBorder(left, top, right, bottom);

  for (float x = left + kFineGridMm; x < right; x += kFineGridMm) {
    const bool coarse = nearlyEqual(std::fmod(std::abs(x - left), kCoarseGridMm), 0.0f)
        || nearlyEqual(std::fmod(std::abs(x - left), kCoarseGridMm), kCoarseGridMm);
    const float c = coarse ? kGridCoarse : kGridFine;
    appendLine(x, top, x, bottom, c, c, c, 0.74f);
  }

  for (float y = top + kFineGridMm; y < bottom; y += kFineGridMm) {
    const bool coarse = nearlyEqual(std::fmod(std::abs(y - top), kCoarseGridMm), 0.0f)
        || nearlyEqual(std::fmod(std::abs(y - top), kCoarseGridMm), kCoarseGridMm);
    const float c = coarse ? kGridCoarse : kGridFine;
    appendLine(left, y, right, y, c, c, c, 0.74f);
  }

  const float originX = std::clamp(m_bedOriginX, left, right);
  const float originY = std::clamp(m_bedOriginY, top, bottom);
  appendLine(originX, top, originX, bottom, kAxisR, kAxisG, kAxisB, 0.95f);
  appendLine(left, originY, right, originY, kAxisR, kAxisG, kAxisB, 0.95f);
}

void PrepareSceneData::appendLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a)
{
  m_bedLineVertices.append(Vertex{x1, y1, r, g, b, a});
  m_bedLineVertices.append(Vertex{x2, y2, r, g, b, a});
}

void PrepareSceneData::appendRectFill(float left, float top, float right, float bottom)
{
  m_bedFillVertices.append(Vertex{left, top, kFillR, kFillG, kFillB, kFillA});
  m_bedFillVertices.append(Vertex{right, top, kFillR, kFillG, kFillB, kFillA});
  m_bedFillVertices.append(Vertex{right, bottom, kFillR, kFillG, kFillB, kFillA});
  m_bedFillVertices.append(Vertex{left, top, kFillR, kFillG, kFillB, kFillA});
  m_bedFillVertices.append(Vertex{right, bottom, kFillR, kFillG, kFillB, kFillA});
  m_bedFillVertices.append(Vertex{left, bottom, kFillR, kFillG, kFillB, kFillA});
}

void PrepareSceneData::appendRectBorder(float left, float top, float right, float bottom)
{
  appendLine(left, top, right, top, kBorderR, kBorderG, kBorderB, 0.95f);
  appendLine(right, top, right, bottom, kBorderR, kBorderG, kBorderB, 0.95f);
  appendLine(right, bottom, left, bottom, kBorderR, kBorderG, kBorderB, 0.95f);
  appendLine(left, bottom, left, top, kBorderR, kBorderG, kBorderB, 0.95f);
}
