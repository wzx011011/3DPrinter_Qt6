#include "PrepareSceneData.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <QPair>

namespace
{
  constexpr float kDefaultBedSizeMm = 220.0f;
  constexpr float kMinBedSizeMm = 1.0f;
  constexpr float kMaxBedSizeMm = 2000.0f;
  constexpr float kFineGridMm = 10.0f;
  constexpr float kCoarseGridMm = 50.0f;

  // Upstream plate palette (PartPlate.cpp:77-85, dark theme variants):
  // selected plate fill, unselected fill, per-plate grid/border line colors.
  constexpr float kFillSelR = 0.2666f;
  constexpr float kFillSelG = 0.2784f;
  constexpr float kFillSelB = 0.2784f;
  constexpr float kFillSelA = 1.0f;
  constexpr float kFillUnselR = 0.384f;
  constexpr float kFillUnselG = 0.384f;
  constexpr float kFillUnselB = 0.412f;
  constexpr float kFillUnselA = 1.0f;
  constexpr float kLineSelR = 0.5294f;
  constexpr float kLineSelG = 0.5451f;
  constexpr float kLineSelB = 0.5333f;
  constexpr float kLineUnselR = 0.431f;
  constexpr float kLineUnselG = 0.431f;
  constexpr float kLineUnselB = 0.463f;
  // Upstream LINE_BOTTOM_COLOR (PartPlate.cpp:85): every plate grid drawn in
  // this color when the camera looks from below (render_grid(true)).
  constexpr float kLineBottomR = 0.8f;
  constexpr float kLineBottomG = 0.8f;
  constexpr float kLineBottomB = 0.8f;
  constexpr float kLineBottomA = 0.4f;
  // Upstream LOGICAL_PART_PLATE_GAP = 1/5 (PartPlate.cpp:53): the stride
  // between plate origins is bed size * (1 + gap).
  constexpr float kPlateGapRatio = 1.0f / 5.0f;
  constexpr float kAxisR = 0.12f;
  constexpr float kAxisG = 0.78f;
  constexpr float kAxisB = 0.37f;
  // Upstream render_exclude_area palette (PartPlate.cpp:859-860):
  // selected vs unselected exclude-region fill.
  constexpr float kExclSelR = 0.765f;
  constexpr float kExclSelG = 0.7686f;
  constexpr float kExclSelB = 0.7686f;
  constexpr float kExclUnsel = 0.9f;
  // Upstream height-limit palette (PartPlate.cpp:86-87).
  constexpr float kLimitBottomR = 0.4f;
  constexpr float kLimitBottomG = 0.4f;
  constexpr float kLimitBottomB = 1.0f;
  constexpr float kLimitTopR = 0.6f;
  constexpr float kLimitTopG = 0.6f;
  constexpr float kLimitTopB = 1.0f;
  constexpr float kLimitGroundZ = 0.02f;

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

void PrepareSceneData::setBedExcludeAreas(const QVariantList &areas)
{
  if (m_bedExcludeAreas == areas)
    return;

  m_bedExcludeAreas = areas;
  rebuildBedGeometry();
  markDirty(DirtyBed | DirtyGpu);
}

void PrepareSceneData::setHeightLimit(bool active, float heightToRod, float heightToLid)
{
  if (m_heightLimitActive == active
      && nearlyEqual(m_heightToRod, heightToRod)
      && nearlyEqual(m_heightToLid, heightToLid))
    return;

  m_heightLimitActive = active;
  m_heightToRod = heightToRod;
  m_heightToLid = heightToLid;
  rebuildHeightLimitGeometry();
  markDirty(DirtyBed | DirtyGpu);
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
  // v5.16: the plate grid layout/selection colors follow the plate context,
  // so the bed geometry rebuilds with it (upstream repositions PartPlates on
  // every plate-count change).
  rebuildBedGeometry();
  markDirty(DirtyPlate | DirtyBed | DirtyGpu);
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
                                        const QList<int> &activeSourceObjectIndices,
                                        const QList<int> &batchVolumeTypes,
                                        const QList<int> &batchExtruderIds,
                                        const QList<int> &batchPrintableFlags,
                                        const QList<QVector4D> &extruderColors)
{
  clearModelGeometry();

  qint32 objectCount = 0;
  qsizetype offset = 0;
  // P15.1 (COLOR): the render-channel arrays are optional (the picking scene
  // passes none); when present they must cover every batch.
  bool valid = readValue(meshData, offset, objectCount)
      && objectCount >= 0
      && objectCount <= kMaxPackedObjects
      && batchSourceObjectIndices.size() == objectCount
      && batchVolumeIndices.size() == objectCount
      && batchInstanceIndices.size() == objectCount
      && (batchVolumeTypes.isEmpty()
          || (batchVolumeTypes.size() == objectCount
              && batchExtruderIds.size() == objectCount
              && batchPrintableFlags.size() == objectCount));

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
    // P15.1/15.2 (COLOR): upstream model coloring — color_from_model_volume
    // (3DScene.cpp:306-334) for the special volume types, then
    // update_colors_by_extruder (:1184-1235, filament_colour[extruder-1]
    // with the index clamped to 0) for model parts; NEUTRAL_COLOR fallback
    // when no filament colors are available.
    float r = 0.8f;
    float g = 0.8f;
    float b = 0.8f;
    float a = 1.0f;
    if (!batchVolumeTypes.isEmpty()) {
      const int volumeType = batchVolumeTypes.at(objectIndex);
      const int extruderId = batchExtruderIds.at(objectIndex);
      const bool printable = batchPrintableFlags.at(objectIndex) != 0;
      switch (volumeType) {
      case 1: r = 0.3f; g = 0.3f; b = 0.3f; a = 0.4f; break; // MODEL_NEGTIVE_COL
      case 2: r = 1.0f; g = 1.0f; b = 0.0f; a = 0.6f; break; // MODEL_MIDIFIER_COL
      case 3: r = 1.0f; g = 0.3f; b = 0.3f; a = 0.4f; break; // SUPPORT_BLOCKER_COL
      case 4: r = 0.3f; g = 0.3f; b = 1.0f; a = 0.4f; break; // SUPPORT_ENFORCER_COL
      default: {
        if (!extruderColors.isEmpty()) {
          int colorIndex = extruderId - 1;
          if (colorIndex < 0 || colorIndex >= extruderColors.size())
            colorIndex = 0;
          const QVector4D color = extruderColors.at(colorIndex);
          r = color.x();
          g = color.y();
          b = color.z();
          a = color.w() > 0.0f ? color.w() : 1.0f;
        }
        if (!printable) { // UNPRINTABLE_COLOR (3DScene.cpp:170)
          r = 0.0f; g = 0.0f; b = 0.0f; a = 0.5f;
        }
        break;
      }
      }
      // adjust_color_for_rendering (3DScene.cpp:93-103): fully transparent
      // materials render white @0.3 alpha, near-black lifts to 0.2 so black
      // filament stays visible on the dark theme.
      if (a < 0.1f) {
        r = 1.0f; g = 1.0f; b = 1.0f; a = 0.3f;
      } else if (r < 0.2f && g < 0.2f && b < 0.2f) {
        r = 0.2f; g = 0.2f; b = 0.2f;
      }
    }
    batch.translucent = a < 0.999f;

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

      const ModelVertex vertex{x, y, z, r, g, b, a};
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

const QList<PrepareSceneData::Vertex> &PrepareSceneData::bedBottomLineVertices() const
{
  return m_bedBottomLineVertices;
}

const QList<PrepareSceneData::ModelVertex> &PrepareSceneData::bedLimitVertices() const
{
  return m_bedLimitVertices;
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

int PrepareSceneData::computePlateColumns(int plateCount)
{
  // Upstream PartPlate.hpp:38 compute_colum_count.
  const float value = std::sqrt(float(std::max(1, plateCount)));
  const float rounded = std::round(value);
  return value > rounded ? int(rounded) + 1 : int(rounded);
}

void PrepareSceneData::rebuildBedGeometry()
{
  m_bedFillVertices.clear();
  m_bedLineVertices.clear();
  m_bedBottomLineVertices.clear();
  m_bedLimitVertices.clear();

  if (!m_showBed)
    return;

  // Source-truth mapping: upstream PartPlateList renders EVERY plate in a
  // grid — cols = ceil(sqrt(count)) (PartPlate.hpp:38 compute_colum_count),
  // stride = bed size * (1 + 1/5) (PartPlate.cpp:53 LOGICAL_PART_PLATE_GAP,
  // compute_shape_position PartPlate.cpp:3206). The selected plate uses
  // SELECT_COLOR + LINE_TOP_SEL_COLOR; the others UNSELECT_DARK_COLOR +
  // LINE_TOP_DARK_COLOR (PartPlate::render_background/render_grid).
  const int plateCount = m_plateCount > 0 ? m_plateCount : 1;
  const float strideX = m_bedWidth * (1.0f + kPlateGapRatio);
  const float strideD = m_bedDepth * (1.0f + kPlateGapRatio);
  const int cols = computePlateColumns(plateCount);

  for (int i = 0; i < plateCount; ++i) {
    const int row = i / cols;
    const int col = i % cols;
    const float left = m_bedOriginX + float(col) * strideX;
    const float top = m_bedOriginY + float(row) * strideD;
    const float right = left + m_bedWidth;
    const float bottom = top + m_bedDepth;
    const bool selected = (m_plateCount <= 0) || (i == m_currentPlateIndex);

    const float fillR = selected ? kFillSelR : kFillUnselR;
    const float fillG = selected ? kFillSelG : kFillUnselG;
    const float fillB = selected ? kFillSelB : kFillUnselB;
    const float fillA = selected ? kFillSelA : kFillUnselA;
    const float lineR = selected ? kLineSelR : kLineUnselR;
    const float lineG = selected ? kLineSelG : kLineUnselG;
    const float lineB = selected ? kLineSelB : kLineUnselB;

    // Upstream render order (PartPlate::render 2728-2765): background fill,
    // then exclude area, then grid. Same fill buffer keeps the layering.
    appendRectFill(left, top, right, bottom, fillR, fillG, fillB, fillA);
    appendExcludeFills(left, top, selected);
    appendRectBorder(left, top, right, bottom, lineR, lineG, lineB);

    // The grid lines are ALSO emitted into the bottom-view buffer in
    // LINE_BOTTOM_COLOR: upstream render_grid(bottom=true) keeps every grid
    // line visible from below while border/origin axes stay top-only
    // (render_background runs under `if (!bottom)`).
    for (float x = left + kFineGridMm; x < right; x += kFineGridMm) {
      appendLine(x, top, x, bottom, lineR, lineG, lineB, 0.6f);
      appendBottomLine(x, top, x, bottom);
    }
    for (float y = top + kFineGridMm; y < bottom; y += kFineGridMm) {
      appendLine(left, y, right, y, lineR, lineG, lineB, 0.6f);
      appendBottomLine(left, y, right, y);
    }

    // Origin axes only on the selected plate (upstream draws the origin
    // cross per plate shape; the selected plate is the working surface).
    if (selected) {
      const float originX = std::clamp(m_bedOriginX, left, right);
      const float originY = std::clamp(m_bedOriginY, top, bottom);
      appendLine(originX, top, originX, bottom, kAxisR, kAxisG, kAxisB, 0.95f);
      appendLine(left, originY, right, originY, kAxisR, kAxisG, kAxisB, 0.95f);
    }
  }

  rebuildHeightLimitGeometry();
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
  // v5.15 (MODELLIT): upstream renders default model volumes in a single
  // neutral color (GLVolume::NEUTRAL_COLOR = 0.8/0.8/0.8, 3DScene.cpp:169);
  // distinct per-object hues were an OWzx invention. The return value keeps
  // the historical palette-index contract (callers use it as a stable hash).
  static constexpr float kNeutral[3] = {0.8f, 0.8f, 0.8f};
  r = kNeutral[0];
  g = kNeutral[1];
  b = kNeutral[2];
  return quint32(std::abs(sourceObjectIndex)) % 6;
}

void PrepareSceneData::appendLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a)
{
  m_bedLineVertices.append(Vertex{x1, y1, r, g, b, a});
  m_bedLineVertices.append(Vertex{x2, y2, r, g, b, a});
}

void PrepareSceneData::appendBottomLine(float x1, float y1, float x2, float y2)
{
  // v5.16 (BEDBOTTOM): LINE_BOTTOM_COLOR grid line for below-horizon views
  // (upstream PartPlate::render_grid(true), PartPlate.cpp:85).
  m_bedBottomLineVertices.append(Vertex{x1, y1, kLineBottomR, kLineBottomG, kLineBottomB, kLineBottomA});
  m_bedBottomLineVertices.append(Vertex{x2, y2, kLineBottomR, kLineBottomG, kLineBottomB, kLineBottomA});
}

void PrepareSceneData::appendRectFill(float left, float top, float right, float bottom,
                                       float r, float g, float b, float a)
{
  m_bedFillVertices.append(Vertex{left, top, r, g, b, a});
  m_bedFillVertices.append(Vertex{right, top, r, g, b, a});
  m_bedFillVertices.append(Vertex{right, bottom, r, g, b, a});
  m_bedFillVertices.append(Vertex{left, top, r, g, b, a});
  m_bedFillVertices.append(Vertex{right, bottom, r, g, b, a});
  m_bedFillVertices.append(Vertex{left, bottom, r, g, b, a});
}

void PrepareSceneData::appendRectBorder(float left, float top, float right, float bottom,
                                         float r, float g, float b)
{
  appendLine(left, top, right, top, r, g, b, 0.95f);
  appendLine(right, top, right, bottom, r, g, b, 0.95f);
  appendLine(right, bottom, left, bottom, r, g, b, 0.95f);
  appendLine(left, bottom, left, top, r, g, b, 0.95f);
}

void PrepareSceneData::appendExcludeFills(float plateLeft, float plateTop, bool selected)
{
  // Upstream render_exclude_area (PartPlate.cpp:856-878): exclude polygons
  // filled over the background with selected/unselected grays. Points are
  // bed-relative; each plate offsets them by its grid position (upstream
  // set_shape PartPlate.cpp:2614 adds the plate position to every point).
  // The stream is FLAT [x,y,...]; upstream groups every 4 consecutive points
  // into one rectangle (init_exclude_bounding_box PartPlate.cpp:373-389,
  // index % 4 == 3), so we fan-triangulate each 4-point group.
  if (m_bedExcludeAreas.isEmpty())
    return;

  const float r = selected ? kExclSelR : kExclUnsel;
  const float g = selected ? kExclSelG : kExclUnsel;
  const float b = selected ? kExclSelB : kExclUnsel;
  const float dx = plateLeft - m_bedOriginX;
  const float dy = plateTop - m_bedOriginY;

  auto coordAt = [this](int index, float &value) -> bool {
    bool ok = false;
    value = m_bedExcludeAreas.at(index).toFloat(&ok);
    return ok && std::isfinite(value);
  };
  const int rectCount = m_bedExcludeAreas.size() / 8;
  for (int rect = 0; rect < rectCount; ++rect) {
    const int base = rect * 8;
    float px[4];
    float py[4];
    bool ok = true;
    for (int p = 0; p < 4 && ok; ++p)
      ok = coordAt(base + p * 2, px[p]) && coordAt(base + p * 2 + 1, py[p]);
    if (!ok)
      continue;
    // Fan triangulation from point 0 (init_model_from_poly convex fan):
    // triangles (p0,p1,p2) and (p0,p2,p3).
    m_bedFillVertices.append(Vertex{px[0] + dx, py[0] + dy, r, g, b, 1.0f});
    m_bedFillVertices.append(Vertex{px[1] + dx, py[1] + dy, r, g, b, 1.0f});
    m_bedFillVertices.append(Vertex{px[2] + dx, py[2] + dy, r, g, b, 1.0f});
    m_bedFillVertices.append(Vertex{px[0] + dx, py[0] + dy, r, g, b, 1.0f});
    m_bedFillVertices.append(Vertex{px[2] + dx, py[2] + dy, r, g, b, 1.0f});
    m_bedFillVertices.append(Vertex{px[3] + dx, py[3] + dy, r, g, b, 1.0f});
  }
}

void PrepareSceneData::rebuildHeightLimitGeometry()
{
  // Upstream calc_height_limit (PartPlate.cpp:512-561) rendered by
  // render_height_limit (:914): for each bed-shape corner a vertical from
  // the ground to extruder_clearance_height_to_rod plus a horizontal ring
  // at that height (BOTTOM color), then verticals rod->lid plus a ring at
  // the lid height (TOP color). Drawn only for ByObject plates; the active
  // flag carries that gate from the viewmodel.
  m_bedLimitVertices.clear();
  if (!m_showBed || !m_heightLimitActive || m_heightToRod <= 0.0f)
    return;

  const int plateCount = m_plateCount > 0 ? m_plateCount : 1;
  const float strideX = m_bedWidth * (1.0f + kPlateGapRatio);
  const float strideD = m_bedDepth * (1.0f + kPlateGapRatio);
  const int cols = computePlateColumns(plateCount);

  auto appendVertical = [](QList<ModelVertex> &vertices,
                           float x, float depth, float z0, float z1,
                           float r, float g, float b) {
    vertices.append(ModelVertex{x, z0, depth, r, g, b, 1.0f});
    vertices.append(ModelVertex{x, z1, depth, r, g, b, 1.0f});
  };
  auto appendRing = [&](float left, float top, float right, float bottom,
                        float height, float r, float g, float b) {
    QList<QPair<float, float>> corners{
        {left, top}, {right, top}, {right, bottom}, {left, bottom}};
    for (int i = 0; i < corners.size(); ++i) {
      const auto &a = corners.at(i);
      const auto &bEnd = corners.at((i + 1) % corners.size());
      m_bedLimitVertices.append(ModelVertex{a.first, height, a.second, r, g, b, 1.0f});
      m_bedLimitVertices.append(ModelVertex{bEnd.first, height, bEnd.second, r, g, b, 1.0f});
    }
  };

  for (int i = 0; i < plateCount; ++i) {
    const int row = i / cols;
    const int col = i % cols;
    const float left = m_bedOriginX + float(col) * strideX;
    const float top = m_bedOriginY + float(row) * strideD;
    const float right = left + m_bedWidth;
    const float bottom = top + m_bedDepth;

    QList<QPair<float, float>> corners{
        {left, top}, {right, top}, {right, bottom}, {left, bottom}};
    for (const auto &corner : corners) {
      appendVertical(m_bedLimitVertices, corner.first, corner.second,
                     kLimitGroundZ, m_heightToRod,
                     kLimitBottomR, kLimitBottomG, kLimitBottomB);
      if (m_heightToLid > m_heightToRod) {
        appendVertical(m_bedLimitVertices, corner.first, corner.second,
                       m_heightToRod, m_heightToLid,
                       kLimitTopR, kLimitTopG, kLimitTopB);
      }
    }
    appendRing(left, top, right, bottom, m_heightToRod,
               kLimitBottomR, kLimitBottomG, kLimitBottomB);
    if (m_heightToLid > m_heightToRod) {
      appendRing(left, top, right, bottom, m_heightToLid,
                 kLimitTopR, kLimitTopG, kLimitTopB);
    }
  }
}
