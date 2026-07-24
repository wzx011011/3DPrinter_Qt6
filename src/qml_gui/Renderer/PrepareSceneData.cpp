#include "PrepareSceneData.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

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

  constexpr qsizetype kPackedBatchHeaderBytes = qsizetype(sizeof(qint32) * 2);
  constexpr qsizetype kPackedTrailerBytes = qsizetype(sizeof(float) * 6);
  constexpr qsizetype kPackedVertexBytes = qsizetype(sizeof(float) * 3);
  constexpr qint32 kMaxPackedObjects = 100000;
  constexpr qint32 kMaxPackedTrianglesPerBatch = 20000000;

  template <typename T>
  bool readValue(const QByteArray &bytes, qsizetype &offset, T &value)
  {
    if (offset < 0 || bytes.size() - offset < qsizetype(sizeof(T)))
      return false;
    std::memcpy(&value, bytes.constData() + offset, sizeof(T));
    offset += sizeof(T);
    return true;
  }
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

void PrepareSceneData::setModelMeshData(const QByteArray &meshData,
                                        const QList<int> &batchSourceObjectIndices,
                                        const QList<int> &batchVolumeIndices,
                                        const QList<int> &batchInstanceIndices,
                                        const QList<int> &activeSourceObjectIndices)
{
  clearModelGeometry();

  qint32 objectCount = 0;
  qsizetype offset = 0;
  bool valid = readValue(meshData, offset, objectCount)
      && objectCount >= 0
      && objectCount <= kMaxPackedObjects
      && batchSourceObjectIndices.size() == objectCount
      && batchVolumeIndices.size() == objectCount
      && batchInstanceIndices.size() == objectCount;

  if (valid) {
    m_modelVertices.reserve(std::min<qsizetype>(meshData.size() / kPackedVertexBytes, 1000000));
  }

  for (qint32 objectIndex = 0; valid && objectIndex < objectCount; ++objectIndex) {
    qint32 renderObjectId = 0;
    qint32 triangleCount = 0;
    valid = readValue(meshData, offset, renderObjectId)
        && readValue(meshData, offset, triangleCount)
        && triangleCount >= 0
        && triangleCount <= kMaxPackedTrianglesPerBatch;
    if (!valid)
      break;

    const qsizetype vertexCount = qsizetype(triangleCount) * 3;
    const qsizetype payloadBytes = vertexCount * kPackedVertexBytes;
    if (payloadBytes < 0 || meshData.size() - offset < payloadBytes) {
      valid = false;
      break;
    }

    const int sourceObjectIndex = batchSourceObjectIndices.at(objectIndex);
    const int volumeIndex = batchVolumeIndices.at(objectIndex);
    const int instanceIndex = batchInstanceIndices.at(objectIndex);
    if (sourceObjectIndex < 0 || volumeIndex < 0 || instanceIndex < 0) {
      valid = false;
      break;
    }
    const bool active = activeSourceContains(activeSourceObjectIndices, sourceObjectIndex);
    ModelBatch batch;
    batch.renderObjectId = renderObjectId;
    batch.sourceObjectIndex = sourceObjectIndex;
    batch.volumeIndex = volumeIndex;
    batch.instanceIndex = instanceIndex;
    batch.firstVertex = m_modelVertices.size();
    batch.vertexCount = int(vertexCount);
    bool batchHasBounds = false;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    colorForSourceObject(sourceObjectIndex, r, g, b);

    for (qsizetype vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
      float x = 0.0f;
      float y = 0.0f;
      float z = 0.0f;
      valid = readValue(meshData, offset, x)
          && readValue(meshData, offset, y)
          && readValue(meshData, offset, z)
          && std::isfinite(x)
          && std::isfinite(y)
          && std::isfinite(z);
      if (!valid)
        break;

      if (!active)
        continue;

      const ModelVertex vertex{x, y, z, r, g, b, 1.0f};
      m_modelVertices.append(vertex);
      if (!batchHasBounds) {
        batch.bounds = ModelBounds{x, y, z, x, y, z};
        batchHasBounds = true;
      } else {
        batch.bounds.minX = std::min(batch.bounds.minX, x);
        batch.bounds.minY = std::min(batch.bounds.minY, y);
        batch.bounds.minZ = std::min(batch.bounds.minZ, z);
        batch.bounds.maxX = std::max(batch.bounds.maxX, x);
        batch.bounds.maxY = std::max(batch.bounds.maxY, y);
        batch.bounds.maxZ = std::max(batch.bounds.maxZ, z);
      }
      updateModelBounds(vertex);
    }

    if (valid && active) {
      batch.vertexCount = m_modelVertices.size() - batch.firstVertex;
      if (batch.vertexCount > 0)
        m_modelBatches.append(batch);
    }
  }

  if (valid) {
    if (meshData.size() - offset < kPackedTrailerBytes)
      valid = false;
    else
      offset += kPackedTrailerBytes;
  }

  if (!valid) {
    clearModelGeometry();
  }

  m_meshGeneration = meshData.size();
  // Functional source-truth mapping: upstream GLCanvas3D/PartPlate render only
  // current plate model volumes. QRhi keeps transport-specific buffers separate
  // from this source-object-aware scene contract.
  markDirty(DirtyMesh | DirtyGpu);
}

void PrepareSceneData::setSelectedSourceObjectIndex(int sourceObjectIndex)
{
  if (m_selectedSourceObjectIndex == sourceObjectIndex)
    return;

  m_selectedSourceObjectIndex = sourceObjectIndex;
  // Selection is an upstream Selection/GLCanvas3D state change. It affects a
  // small highlight path, not the resident model vertex buffer.
  markDirty(DirtySelection);
}

void PrepareSceneData::setHoveredSourceObjectIndex(int sourceObjectIndex)
{
  if (m_hoveredSourceObjectIndex == sourceObjectIndex)
    return;

  m_hoveredSourceObjectIndex = sourceObjectIndex;
  markDirty(DirtySelection);
}

void PrepareSceneData::markCameraDirty()
{
  markDirty(DirtyCamera);
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

const QList<PrepareSceneData::ModelVertex> &PrepareSceneData::modelVertices() const
{
  return m_modelVertices;
}

const QList<PrepareSceneData::ModelBatch> &PrepareSceneData::modelBatches() const
{
  return m_modelBatches;
}

const PrepareSceneData::ModelBounds &PrepareSceneData::modelBounds() const
{
  return m_modelBounds;
}

bool PrepareSceneData::hasModelBounds() const
{
  return m_hasModelBounds;
}

int PrepareSceneData::selectedSourceObjectIndex() const
{
  return m_selectedSourceObjectIndex;
}

int PrepareSceneData::hoveredSourceObjectIndex() const
{
  return m_hoveredSourceObjectIndex;
}

bool PrepareSceneData::containsCurrentPlatePoint(float x, float z) const
{
  if (m_currentPlateIndex < 0 || m_currentPlateIndex >= m_plateCount
      || !std::isfinite(x) || !std::isfinite(z)) {
    return false;
  }

  const float left = m_bedOriginX;
  const float top = m_bedOriginY;
  if (m_bedShapeType == 1) {
    const float radius = m_bedDiameter * 0.5f;
    const float cx = left + m_bedWidth * 0.5f;
    const float cz = top + m_bedDepth * 0.5f;
    const float dx = x - cx;
    const float dz = z - cz;
    return dx * dx + dz * dz <= radius * radius;
  }

  return x >= left && x <= left + m_bedWidth
      && z >= top && z <= top + m_bedDepth;
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

void PrepareSceneData::clearModelGeometry()
{
  m_modelVertices.clear();
  m_modelBatches.clear();
  m_modelBounds = ModelBounds{};
  m_hasModelBounds = false;
}

void PrepareSceneData::updateModelBounds(const ModelVertex &vertex)
{
  if (!m_hasModelBounds) {
    m_modelBounds = ModelBounds{vertex.x, vertex.y, vertex.z, vertex.x, vertex.y, vertex.z};
    m_hasModelBounds = true;
    return;
  }

  m_modelBounds.minX = std::min(m_modelBounds.minX, vertex.x);
  m_modelBounds.minY = std::min(m_modelBounds.minY, vertex.y);
  m_modelBounds.minZ = std::min(m_modelBounds.minZ, vertex.z);
  m_modelBounds.maxX = std::max(m_modelBounds.maxX, vertex.x);
  m_modelBounds.maxY = std::max(m_modelBounds.maxY, vertex.y);
  m_modelBounds.maxZ = std::max(m_modelBounds.maxZ, vertex.z);
}

bool PrepareSceneData::activeSourceContains(const QList<int> &activeSourceObjectIndices, int sourceObjectIndex)
{
  return activeSourceObjectIndices.contains(sourceObjectIndex);
}

quint32 PrepareSceneData::colorForSourceObject(int sourceObjectIndex, float &r, float &g, float &b)
{
  static constexpr float kPalette[][3] = {
    {0.48f, 0.68f, 0.95f},
    {0.70f, 0.58f, 0.88f},
    {0.95f, 0.64f, 0.38f},
    {0.42f, 0.76f, 0.58f},
    {0.88f, 0.54f, 0.58f},
    {0.62f, 0.72f, 0.42f}
  };
  const quint32 index = quint32(std::abs(sourceObjectIndex)) % (sizeof(kPalette) / sizeof(kPalette[0]));
  r = kPalette[index][0];
  g = kPalette[index][1];
  b = kPalette[index][2];
  return index;
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
