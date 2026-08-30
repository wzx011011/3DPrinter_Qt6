#include "PrepareSceneData.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <QHash>
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
  // P15.8 (BOLDGRID): upstream calc_gridlines bolds every 5th grid line
  // (PartPlate.cpp:483/496 count % 5) and render_grid draws the bold set a
  // second time with glLineWidth(2) (PartPlate.cpp:909). QRhi has no line
  // width, so the bold lines are flat quads ~0.5mm wide (2px at the typical
  // bed-view zoom).
  constexpr int kBoldEveryNth = 5;
  constexpr float kBoldHalfWidthMm = 0.25f;
  // P15.5 (SINK): upstream GLVolume::SinkingContours (3DScene.cpp:108-161).
  constexpr float kSinkingHalfWidth = 0.25f;     // SinkingContours::HalfWidth (3DScene.cpp:108)
  constexpr float kSinkingZLift = 0.015f;        // anti z-fight lift (3DScene.cpp:150)
  constexpr float kSinkingThreshold = -0.001f;   // SINKING_Z_THRESHOLD (Model.hpp:1709)
  // P15.3 (OUTOFBED): BuildVolume::SceneEpsilon (BuildVolume.hpp:79,
  // libslic3r.h:52) used by the print volume inflation / state tests.
  constexpr float kSceneEpsilon = 1e-4f;
  // P15.7 (AXES): Bed3D::Axes constants (3DBed.cpp:183-186, :30).
  constexpr float kAxesGroundZ = -0.04f;         // GROUND_Z (3DBed.cpp:30)
  constexpr float kArrowStemRadius = 0.5f;       // Axes::DefaultStemRadius (3DBed.cpp:183)
  constexpr float kArrowTipRadius = 1.25f;       // 2.5 * DefaultStemRadius (3DBed.cpp:185)
  constexpr float kArrowTipLength = 5.0f;        // Axes::DefaultTipLength (3DBed.cpp:186)
  // AXIS_X/Y/Z_COLOR = ColorRGBA::X()/Y()/Z() (3DBed.cpp:188-190,
  // Color.hpp:143-145).
  constexpr float kAxisXR = 0.75f;
  constexpr float kAxisYG = 0.75f;
  constexpr float kAxisZB = 0.75f;
  // P15.6 (CIRCLEBED): boundary polygon segment count for circular plates
  // (upstream fills/clips the circle bed-shape polygon, PartPlate
  // calc_triangles/calc_gridlines).
  constexpr int kCircleSegments = 48;
  constexpr float kPi = 3.14159265358979f;
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

void PrepareSceneData::setPrintableHeight(float height)
{
  // P15.3 (OUTOFBED): upstream print_volume z_data.y = printable_height
  // (GLCanvas3D.cpp:7183/7189). 0 or negative = unbounded (feature not yet
  // plumbed from the viewmodel).
  const float normalized = height > 0.0f && std::isfinite(height) ? height : 0.0f;
  if (nearlyEqual(m_printableHeight, normalized))
    return;
  m_printableHeight = normalized;
  updatePrintVolume();
  recomputeOutsideState();
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
    // P15.3 (OUTOFBED): ProjectVolumeType for the outside-state gate.
    batch.volumeType = batchVolumeTypes.isEmpty() ? 0
                                                  : batchVolumeTypes.at(objectIndex);

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

  // P15.3 (OUTOFBED)/P15.5 (SINK): derived per-batch state. Only computed
  // from a fully valid payload so a malformed stream cannot leave stale
  // outside flags behind.
  if (valid) {
    // P15.5 (SINK): contour bands for sinking batches (bbox dips under the
    // bed plane without being fully below it — upstream is_sinking,
    // 3DScene.cpp:614-620 with SINKING_Z_THRESHOLD = -0.001,
    // Model.hpp:1709).
    for (const ModelBatch &batch : m_modelBatches) {
      if (batch.bounds.minY < kSinkingThreshold
          && batch.bounds.maxY >= kSinkingThreshold)
        appendSinkingContours(batch);
    }
    updatePrintVolume();
    recomputeOutsideState();
  } else {
    m_sinkingContourVertices.clear();
    m_sinkingContourRanges.clear();
    m_anyVolumeOutside = false;
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

const QList<PrepareSceneData::Vertex> &PrepareSceneData::bedBoldLineVertices() const
{
  // P15.8 (BOLDGRID): every-5th grid line as ~2px quads
  // (upstream render_grid second bolder draw, PartPlate.cpp:909-911).
  return m_bedBoldLineVertices;
}

const QList<PrepareSceneData::ModelVertex> &PrepareSceneData::sinkingContourVertices() const
{
  // P15.5 (SINK): white bed-plane contour bands
  // (upstream GLVolume::SinkingContours, 3DScene.cpp:108-161).
  return m_sinkingContourVertices;
}

const QList<PrepareSceneData::SinkingContourRange> &PrepareSceneData::sinkingContourRanges() const
{
  return m_sinkingContourRanges;
}

const QList<PrepareSceneData::ModelVertex> &PrepareSceneData::bedAxesVertices() const
{
  // P15.7 (AXES): X/Y/Z origin arrows
  // (upstream Bed3D::Axes::render, 3DBed.cpp:183-245).
  return m_bedAxesVertices;
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

const PrepareSceneData::PrintVolume &PrepareSceneData::printVolume() const
{
  return m_printVolume;
}

bool PrepareSceneData::anyVolumeOutside() const
{
  return m_anyVolumeOutside;
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

void PrepareSceneData::plateGridOffset(int plateIndex, int plateCount,
                                       float bedWidth, float bedDepth,
                                       float &outOffsetX, float &outOffsetY)
{
  // P15.9 (PLATEANCHOR): single source of the per-plate grid layout.
  // Upstream compute_shape_position (PartPlate.cpp:3206) offsets each plate
  // by col/row * (bed size * (1 + LOGICAL_PART_PLATE_GAP 1/5),
  // PartPlate.cpp:53); columns come from compute_colum_count
  // (PartPlate.hpp:38).
  const int count = plateCount > 0 ? plateCount : 1;
  const int index = plateIndex > 0 ? plateIndex : 0;
  const int cols = computePlateColumns(count);
  const float strideX = bedWidth * (1.0f + kPlateGapRatio);
  const float strideD = bedDepth * (1.0f + kPlateGapRatio);
  outOffsetX = float(index % cols) * strideX;
  outOffsetY = float(index / cols) * strideD;
}

void PrepareSceneData::rebuildBedGeometry()
{
  m_bedFillVertices.clear();
  m_bedLineVertices.clear();
  m_bedBottomLineVertices.clear();
  m_bedBoldLineVertices.clear();
  m_bedLimitVertices.clear();
  m_bedAxesVertices.clear();

  if (!m_showBed) {
    updatePrintVolume();
    return;
  }

  // Source-truth mapping: upstream PartPlateList renders EVERY plate in a
  // grid — cols = ceil(sqrt(count)) (PartPlate.hpp:38 compute_colum_count),
  // stride = bed size * (1 + 1/5) (PartPlate.cpp:53 LOGICAL_PART_PLATE_GAP,
  // compute_shape_position PartPlate.cpp:3206). The selected plate uses
  // SELECT_COLOR + LINE_TOP_SEL_COLOR; the others UNSELECT_DARK_COLOR +
  // LINE_TOP_DARK_COLOR (PartPlate::render_background/render_grid).
  const int plateCount = m_plateCount > 0 ? m_plateCount : 1;
  const int cols = computePlateColumns(plateCount);

  for (int i = 0; i < plateCount; ++i) {
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    plateGridOffset(i, plateCount, m_bedWidth, m_bedDepth, offsetX, offsetY);
    const float left = m_bedOriginX + offsetX;
    const float top = m_bedOriginY + offsetY;
    const bool selected = (m_plateCount <= 0) || (i == m_currentPlateIndex);
    rebuildPlateGeometry(i / cols, i % cols, left, top, selected);
  }

  rebuildAxesGeometry();
  updatePrintVolume();
  rebuildHeightLimitGeometry();
}

void PrepareSceneData::rebuildPlateGeometry(int plateRow, int plateCol,
                                            float left, float top, bool selected)
{
  Q_UNUSED(plateRow);
  Q_UNUSED(plateCol);
  const float right = left + m_bedWidth;
  const float bottom = top + m_bedDepth;

  const float fillR = selected ? kFillSelR : kFillUnselR;
  const float fillG = selected ? kFillSelG : kFillUnselG;
  const float fillB = selected ? kFillSelB : kFillUnselB;
  const float fillA = selected ? kFillSelA : kFillUnselA;
  const float lineR = selected ? kLineSelR : kLineUnselR;
  const float lineG = selected ? kLineSelG : kLineUnselG;
  const float lineB = selected ? kLineSelB : kLineUnselB;

  // Upstream render order (PartPlate::render 2728-2765): background fill,
  // then exclude area, then grid. Same fill buffer keeps the layering.
  if (m_bedShapeType == 1) {
    // P15.6 (CIRCLEBED): circular plate — fan-triangulated boundary polygon
    // fill (upstream init_model_from_poly convex fan over the circle bed
    // shape) + boundary chords clipped to the circle (upstream
    // calc_gridlines intersection_pl with the plate polygon).
    const float cx = left + m_bedWidth * 0.5f;
    const float cz = top + m_bedDepth * 0.5f;
    const float radius = m_bedDiameter * 0.5f;
    for (int s = 0; s < kCircleSegments; ++s) {
      const float a0 = 2.0f * kPi * float(s) / float(kCircleSegments);
      const float a1 = 2.0f * kPi * float(s + 1) / float(kCircleSegments);
      const float x0 = cx + radius * std::cos(a0);
      const float z0 = cz + radius * std::sin(a0);
      const float x1 = cx + radius * std::cos(a1);
      const float z1 = cz + radius * std::sin(a1);
      // fan triangle center -> s -> s+1
      m_bedFillVertices.append(Vertex{cx, cz, fillR, fillG, fillB, fillA});
      m_bedFillVertices.append(Vertex{x0, z0, fillR, fillG, fillB, fillA});
      m_bedFillVertices.append(Vertex{x1, z1, fillR, fillG, fillB, fillA});
      // boundary chord (upstream appends the bed contour lines to the
      // gridline set, PartPlate.cpp:507-509)
      appendLine(x0, z0, x1, z1, lineR, lineG, lineB, 0.95f);
      appendBottomLine(x0, z0, x1, z1);
    }

    // Fine grid clipped to the circle: straight lines every 10mm with the
    // chord endpoints computed from the circle (upstream clips the same
    // straight lines against the circle polygon, PartPlate.cpp:504-505).
    // Bolder every 5th line (PartPlate.cpp:483/496), x exclusive / y
    // inclusive like calc_gridlines.
    for (int k = 1; ; ++k) {
      const float x = left + float(k) * kFineGridMm;
      if (x >= right)
        break;
      const float dx = x - cx;
      if (std::abs(dx) >= radius)
        continue;
      const float half = std::sqrt(radius * radius - dx * dx);
      const bool bold = (k % kBoldEveryNth) == 0;
      if (bold) {
        appendBoldLine(x, cz - half, x, cz + half, lineR, lineG, lineB);
      } else {
        appendLine(x, cz - half, x, cz + half, lineR, lineG, lineB, 0.6f);
        appendBottomLine(x, cz - half, x, cz + half);
      }
    }
    for (int k = 1; ; ++k) {
      const float y = top + float(k) * kFineGridMm;
      if (y >= bottom)
        break;
      const float dy = y - cz;
      if (std::abs(dy) >= radius)
        continue;
      const float half = std::sqrt(radius * radius - dy * dy);
      const bool bold = (k % kBoldEveryNth) == 0;
      appendLine(cx - half, y, cx + half, y, lineR, lineG, lineB, 0.6f);
      appendBottomLine(cx - half, y, cx + half, y);
      if (bold)
        appendBoldLine(cx - half, y, cx + half, y, lineR, lineG, lineB);
    }
    return;
  }

  appendRectFill(left, top, right, bottom, fillR, fillG, fillB, fillA);
  appendExcludeFills(left, top, selected);
  appendRectBorder(left, top, right, bottom, lineR, lineG, lineB);

  // The grid lines are ALSO emitted into the bottom-view buffer in
  // LINE_BOTTOM_COLOR: upstream render_grid(bottom=true) keeps every grid
  // line visible from below while border/origin axes stay top-only
  // (render_background runs under `if (!bottom)`).
  // P15.8 (BOLDGRID): every 5th line moves to the ~2px bolder set
  // (PartPlate.cpp:483/496 count % 5, :909-911 second draw). Upstream
  // calc_gridlines counts from the plate edge: bold lines sit at multiples
  // of 5 * 10mm from it. X bold lines are exclusive to the bolder set;
  // Y bold lines stay in the fine set too (PartPlate.cpp:483-500).
  int gridIndex = 1; // first inner line sits one spacing from the edge
  for (float x = left + kFineGridMm; x < right; x += kFineGridMm, ++gridIndex) {
    if (gridIndex % kBoldEveryNth == 0) {
      appendBoldLine(x, top, x, bottom, lineR, lineG, lineB);
    } else {
      appendLine(x, top, x, bottom, lineR, lineG, lineB, 0.6f);
      appendBottomLine(x, top, x, bottom);
    }
  }
  gridIndex = 1;
  for (float y = top + kFineGridMm; y < bottom; y += kFineGridMm, ++gridIndex) {
    appendLine(left, y, right, y, lineR, lineG, lineB, 0.6f);
    appendBottomLine(left, y, right, y);
    if (gridIndex % kBoldEveryNth == 0)
      appendBoldLine(left, y, right, y, lineR, lineG, lineB);
  }
}

void PrepareSceneData::clearModelGeometry()
{
  m_modelVertices.clear();
  m_modelBatches.clear();
  m_modelBounds = ModelBounds{};
  m_hasModelBounds = false;
  // P15.5 (SINK)/P15.3 (OUTOFBED): derived state follows the batches.
  m_sinkingContourVertices.clear();
  m_sinkingContourRanges.clear();
  m_anyVolumeOutside = false;
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

void PrepareSceneData::appendBoldLine(float x1, float y1, float x2, float y2, float r, float g, float b)
{
  // P15.8 (BOLDGRID): upstream draws the every-5th-line set a second time
  // with glLineWidth(2.0f * scale) (PartPlate.cpp:909-911). QRhi pipelines
  // have no line width, so the bolder line is a flat 0.5mm quad (two
  // triangles) perpendicular to the line direction — ~2px at the typical
  // bed-view zoom. The bottom-view twin uses two parallel 1px lines in
  // LINE_BOTTOM_COLOR (render_grid draws the same bolder set from below).
  const float dx = x2 - x1;
  const float dy = y2 - y1;
  const float len = std::sqrt(dx * dx + dy * dy);
  if (len <= 1e-6f)
    return;
  const float nx = -dy / len * kBoldHalfWidthMm;
  const float ny = dx / len * kBoldHalfWidthMm;
  const float a1x = x1 + nx, a1y = y1 + ny;
  const float a2x = x2 + nx, a2y = y2 + ny;
  const float b1x = x1 - nx, b1y = y1 - ny;
  const float b2x = x2 - nx, b2y = y2 - ny;
  m_bedBoldLineVertices.append(Vertex{a1x, a1y, r, g, b, 1.0f});
  m_bedBoldLineVertices.append(Vertex{a2x, a2y, r, g, b, 1.0f});
  m_bedBoldLineVertices.append(Vertex{b2x, b2y, r, g, b, 1.0f});
  m_bedBoldLineVertices.append(Vertex{a1x, a1y, r, g, b, 1.0f});
  m_bedBoldLineVertices.append(Vertex{b2x, b2y, r, g, b, 1.0f});
  m_bedBoldLineVertices.append(Vertex{b1x, b1y, r, g, b, 1.0f});
  // Bottom view: same bolder hierarchy in LINE_BOTTOM_COLOR as two parallel
  // 1px lines offset by the quad half width.
  appendBottomLine(a1x, a1y, a2x, a2y);
  appendBottomLine(b1x, b1y, b2x, b2y);
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

// P15.7 (AXES): origin coordinate arrows on the selected plate, replacing
// the former invented green cross. Upstream Bed3D::Axes::render
// (3DBed.cpp:183-245): three stilized arrows — X (AXIS_X_COLOR), Y
// (AXIS_Y_COLOR), Z (AXIS_Z_COLOR) — with stem length 0.1x the bed size
// (3DBed.cpp:326), rooted at the plate origin at GROUND_Z (-0.04,
// 3DBed.cpp:30/325). Colors are ColorRGBA::X()/Y()/Z() = 0.75 on their
// channel (Color.hpp:143-145). Upstream world axes map to Qt scene axes as
// X->X, Y->Z (depth), Z->Y (height); each arrow is generated along the
// upstream +Z frame (GLModel.cpp:850-932 stilized_arrow) and remapped.
void PrepareSceneData::rebuildAxesGeometry()
{
  m_bedAxesVertices.clear();
  if (!m_showBed)
    return;

  const int plateCount = m_plateCount > 0 ? m_plateCount : 1;
  const bool validPlate = m_currentPlateIndex >= 0 && m_currentPlateIndex < plateCount;
  const int current = validPlate ? m_currentPlateIndex : 0;
  float offsetX = 0.0f;
  float offsetY = 0.0f;
  plateGridOffset(current, plateCount, m_bedWidth, m_bedDepth, offsetX, offsetY);
  const float left = m_bedOriginX + offsetX;
  const float top = m_bedOriginY + offsetY;
  // Bed3D::set_shape: stem length = 0.1 * bounding_volume max size
  // (3DBed.cpp:326); printable height is not tracked here, so the max uses
  // the plate footprint.
  const float stemLength = 0.1f * std::max(m_bedWidth, m_bedDepth);
  const float origin[3] = {left, kAxesGroundZ, top};

  // x axis (upstream assemble_transform(origin, {0, 0.5*PI, 0}), 3DBed.cpp:232)
  const float dirX[3] = {1.0f, 0.0f, 0.0f};
  appendArrow(origin, dirX, stemLength, kAxisXR, 0.0f, 0.0f);
  // y axis (upstream {-0.5*PI, 0, 0}, 3DBed.cpp:236); lands on Qt +Z.
  const float dirZ[3] = {0.0f, 0.0f, 1.0f};
  appendArrow(origin, dirZ, stemLength, 0.0f, kAxisYG, 0.0f);
  // z axis (upstream identity, 3DBed.cpp:240); = Qt +Y (height).
  const float dirY[3] = {0.0f, 1.0f, 0.0f};
  appendArrow(origin, dirY, stemLength, 0.0f, 0.0f, kAxisZB);
}

// stilized_arrow port (GLModel.cpp:850-932): triangle soup generated along
// upstream +Z (tip at z = stemLength + tipHeight), rotated onto `dir`,
// remapped to Qt scene axes and translated to `origin`. Per-face normals
// are recomputed by the renderer from the winding; the lit shader is
// winding-agnostic, so the handedness of the axis remap is irrelevant.
void PrepareSceneData::appendArrow(const float origin[3], const float dir[3],
                                   float stemLength, float r, float g, float b)
{
  const int resolution = 16;
  const float tipRadius = kArrowTipRadius;
  const float tipHeight = kArrowTipLength;
  const float stemRadius = kArrowStemRadius;
  const float stemHeight = stemLength;
  const float totalHeight = tipHeight + stemHeight;

  float cosines[resolution];
  float sines[resolution];
  for (int i = 0; i < resolution; ++i) {
    const float angle = 2.0f * kPi * float(i) / float(resolution);
    cosines[i] = std::cos(angle);
    sines[i] = -std::sin(angle);
  }

  // Emit in the upstream frame; the branch below applies the upstream axis
  // rotation and the (X,Y,Z)upstream -> (X,Z,Y)qt remap in one step.
  auto emitVertex = [&](float x, float y, float z) {
    float qx, qy, qz;
    if (dir[1] > 0.5f) {
      // upstream +Z arrow (identity): (x,y,z) -> (x, z, y)
      qx = x; qy = z; qz = y;
    } else if (dir[0] > 0.5f) {
      // upstream +X arrow (rotY +90deg): (x,y,z) -> (z, y, -x) -> qt
      qx = z; qy = y; qz = -x;
    } else {
      // upstream +Y arrow (rotX -90deg): (x,y,z) -> (x, z, -y) -> qt
      qx = x; qy = -y; qz = -z;
    }
    m_bedAxesVertices.append(ModelVertex{origin[0] + qx, origin[1] + qy,
                                         origin[2] + qz, r, g, b, 1.0f});
  };

  // tip: apex (index 0) + ring (GLModel.cpp:871-881)
  const int ringBase = 1;
  {
    float vx[resolution + 1];
    float vy[resolution + 1];
    float vz[resolution + 1];
    vx[0] = 0.0f; vy[0] = 0.0f; vz[0] = totalHeight;
    for (int i = 0; i < resolution; ++i) {
      vx[ringBase + i] = tipRadius * sines[i];
      vy[ringBase + i] = tipRadius * cosines[i];
      vz[ringBase + i] = stemHeight;
    }
    for (int i = 0; i < resolution; ++i) {
      const int v3 = (i < resolution - 1) ? ringBase + i + 1 : ringBase;
      emitVertex(vx[0], vy[0], vz[0]);
      emitVertex(vx[ringBase + i], vy[ringBase + i], vz[ringBase + i]);
      emitVertex(vx[v3], vy[v3], vz[v3]);
    }
  }
  // stem side + bottom cap (GLModel.cpp:901-929). The tip cap faces into
  // the cone joint and is omitted (invisible at render scale).
  for (int i = 0; i < resolution; ++i) {
    const int j = (i < resolution - 1) ? i + 1 : 0;
    const float ax = stemRadius * sines[i], ay = stemRadius * cosines[i];
    const float bx = stemRadius * sines[j], by = stemRadius * cosines[j];
    // side quad
    emitVertex(ax, ay, stemHeight);
    emitVertex(bx, by, stemHeight);
    emitVertex(bx, by, 0.0f);
    emitVertex(ax, ay, stemHeight);
    emitVertex(bx, by, 0.0f);
    emitVertex(ax, ay, 0.0f);
    // bottom cap fan wedge
    emitVertex(0.0f, 0.0f, 0.0f);
    emitVertex(bx, by, 0.0f);
    emitVertex(ax, ay, 0.0f);
  }
}

// P15.3 (OUTOFBED): current-plate print volume in Qt scene coordinates for
// the model_lit fragment test. Mirrors GLCanvas3D.cpp:7177-7205: the
// rectangle volume is the plate bounds inflated by BuildVolume::SceneEpsilon
// (:7180), the circle volume carries center/radius+SceneEpsilon (:7188) and
// printable_height (+SceneEpsilon) (:7189).
void PrepareSceneData::updatePrintVolume()
{
  if (m_bedShapeType == 1) {
    m_printVolume.type = 1;
    const float cx = m_bedOriginX + m_bedWidth * 0.5f;
    const float cz = m_bedOriginY + m_bedDepth * 0.5f;
    const float radius = m_bedDiameter * 0.5f + kSceneEpsilon;
    m_printVolume.xyData = QVector4D(cx, cz, radius, 0.0f);
    m_printVolume.zMin = 0.0f;
    m_printVolume.zMax = m_printableHeight > 0.0f
        ? m_printableHeight + kSceneEpsilon
        : std::numeric_limits<float>::max();
  } else {
    m_printVolume.type = 0;
    m_printVolume.xyData = QVector4D(m_bedOriginX - kSceneEpsilon,
                                     m_bedOriginY - kSceneEpsilon,
                                     m_bedOriginX + m_bedWidth + kSceneEpsilon,
                                     m_bedOriginY + m_bedDepth + kSceneEpsilon);
    m_printVolume.zMin = 0.0f;
    m_printVolume.zMax = m_printableHeight > 0.0f
        ? m_printableHeight
        : std::numeric_limits<float>::max();
  }
}

// P15.3 (OUTOFBED): upstream GLVolumeCollection::check_outside_state
// (3DScene.cpp:1043-1171) classifies every model-part instance
// Inside / Partly / Fully outside / Below; _render_background then turns
// the viewport red when any volume is not Inside
// (GLCanvas3D.cpp:7069-7090 _is_any_volume_outside). QRhi works on the
// baked per-batch bounds, so the classification runs on the per-instance
// union bbox with volume_state_bbox semantics (BuildVolume.cpp:308-320:
// Below when max z <= -SceneEpsilon, Inside when contained, Colliding when
// intersecting, Outside otherwise). The circle variant approximates the
// convex-hull vertex test (BuildVolume::object_state, BuildVolume.cpp:291)
// with the bbox corners. Only model-part batches participate
// (shader_outside_printer_detection_enabled = is_model_part,
// 3DScene.cpp:728).
void PrepareSceneData::recomputeOutsideState()
{
  m_anyVolumeOutside = false;
  if (m_modelBatches.isEmpty())
    return;

  struct InstanceBounds
  {
    ModelBounds box;
    bool valid = false;
  };
  QHash<qint64, InstanceBounds> instances;
  for (const ModelBatch &batch : m_modelBatches) {
    if (batch.volumeType != 0)
      continue;
    const qint64 key = (qint64(batch.sourceObjectIndex) << 32)
        | quint32(batch.instanceIndex);
    InstanceBounds &slot = instances[key];
    if (!slot.valid) {
      slot.box = batch.bounds;
      slot.valid = true;
    } else {
      slot.box.minX = std::min(slot.box.minX, batch.bounds.minX);
      slot.box.minY = std::min(slot.box.minY, batch.bounds.minY);
      slot.box.minZ = std::min(slot.box.minZ, batch.bounds.minZ);
      slot.box.maxX = std::max(slot.box.maxX, batch.bounds.maxX);
      slot.box.maxY = std::max(slot.box.maxY, batch.bounds.maxY);
      slot.box.maxZ = std::max(slot.box.maxZ, batch.bounds.maxZ);
    }
  }

  for (auto it = instances.constBegin(); it != instances.constEnd(); ++it) {
    const InstanceBounds &inst = it.value();
    // Below the print bed (volume_state_bbox first clause).
    if (inst.box.maxY <= -kSceneEpsilon) {
      m_anyVolumeOutside = true;
      break;
    }
    if (m_printVolume.type == 1) {
      const float cx = m_printVolume.xyData.x();
      const float cz = m_printVolume.xyData.y();
      const float r = m_printVolume.xyData.z();
      const float cornersX[4] = {inst.box.minX, inst.box.maxX, inst.box.minX, inst.box.maxX};
      const float cornersZ[4] = {inst.box.minZ, inst.box.minZ, inst.box.maxZ, inst.box.maxZ};
      bool allInside = true;
      for (int c = 0; c < 4 && allInside; ++c) {
        const float d2 = (cornersX[c] - cx) * (cornersX[c] - cx)
            + (cornersZ[c] - cz) * (cornersZ[c] - cz);
        allInside = d2 <= r * r;
      }
      const bool heightOk = inst.box.maxY < m_printVolume.zMax + kSceneEpsilon;
      // Inside only when contained; Colliding and Outside both count as
      // "not inside" for the error background.
      if (!(allInside && heightOk)) {
        m_anyVolumeOutside = true;
        break;
      }
    } else {
      const bool insideRect =
          inst.box.minX >= m_printVolume.xyData.x()
          && inst.box.minZ >= m_printVolume.xyData.y()
          && inst.box.maxX <= m_printVolume.xyData.z()
          && inst.box.maxZ <= m_printVolume.xyData.w()
          && inst.box.maxY <= m_printVolume.zMax;
      if (!insideRect) {
        m_anyVolumeOutside = true;
        break;
      }
    }
  }
}

// P15.5 (SINK): bed-plane contour band of one sinking batch (upstream
// GLVolume::SinkingContours::update, 3DScene.cpp:124-161): slice the batch
// triangles with the bed plane (upstream slice_mesh at 0.0, :144), build
// the 0.5mm-wide white band around the contour (expand/shrink by HalfWidth
// 0.25, :108/:145) and lift it by 0.015 against z-fighting (:150). The
// clipper polygon offsetting is approximated per slice segment with a
// perpendicular quad of the same total width; upstream unions the slice
// polygons first, so self-overlapping contours may double-draw where the
// clipper path would not.
void PrepareSceneData::appendSinkingContours(const ModelBatch &batch)
{
  const int end = static_cast<int>(std::min<qsizetype>(
      batch.firstVertex + batch.vertexCount, m_modelVertices.size()));
  SinkingContourRange range;
  range.sourceObjectIndex = batch.sourceObjectIndex;
  range.firstVertex = m_sinkingContourVertices.size();
  for (int i = std::max(0, batch.firstVertex); i + 2 < end; i += 3) {
    const ModelVertex &a = m_modelVertices.at(i);
    const ModelVertex &b = m_modelVertices.at(i + 1);
    const ModelVertex &c = m_modelVertices.at(i + 2);
    const float py[3] = {a.y, b.y, c.y};
    const float px[3] = {a.x, b.x, c.x};
    const float pz[3] = {a.z, b.z, c.z};
    // Edge crossings with the y = 0 bed plane (2 for a real intersection).
    float sx[2];
    float sz[2];
    int crossings = 0;
    for (int e = 0; e < 3 && crossings < 2; ++e) {
      const int f = (e + 1) % 3;
      const bool eAbove = py[e] > 0.0f;
      const bool fAbove = py[f] > 0.0f;
      if (eAbove == fAbove)
        continue;
      const float t = py[e] / (py[e] - py[f]);
      sx[crossings] = px[e] + (px[f] - px[e]) * t;
      sz[crossings] = pz[e] + (pz[f] - pz[e]) * t;
      ++crossings;
    }
    if (crossings < 2)
      continue;
    float dx = sx[1] - sx[0];
    float dz = sz[1] - sz[0];
    const float len = std::sqrt(dx * dx + dz * dz);
    if (len <= 1e-6f)
      continue;
    dx /= len;
    dz /= len;
    const float nx = -dz * kSinkingHalfWidth;
    const float nz = dx * kSinkingHalfWidth;
    const float y = kSinkingZLift;
    const float ax = sx[0] + nx, az = sz[0] + nz;
    const float bx = sx[1] + nx, bz = sz[1] + nz;
    const float cx2 = sx[1] - nx, cz2 = sz[1] - nz;
    const float dx2 = sx[0] - nx, dz2 = sz[0] - nz;
    m_sinkingContourVertices.append(ModelVertex{ax, y, az, 1.0f, 1.0f, 1.0f, 1.0f});
    m_sinkingContourVertices.append(ModelVertex{bx, y, bz, 1.0f, 1.0f, 1.0f, 1.0f});
    m_sinkingContourVertices.append(ModelVertex{cx2, y, cz2, 1.0f, 1.0f, 1.0f, 1.0f});
    m_sinkingContourVertices.append(ModelVertex{ax, y, az, 1.0f, 1.0f, 1.0f, 1.0f});
    m_sinkingContourVertices.append(ModelVertex{cx2, y, cz2, 1.0f, 1.0f, 1.0f, 1.0f});
    m_sinkingContourVertices.append(ModelVertex{dx2, y, dz2, 1.0f, 1.0f, 1.0f, 1.0f});
  }
  range.vertexCount = m_sinkingContourVertices.size() - range.firstVertex;
  if (range.vertexCount > 0)
    m_sinkingContourRanges.append(range);
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
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    plateGridOffset(i, plateCount, m_bedWidth, m_bedDepth, offsetX, offsetY);
    const float left = m_bedOriginX + offsetX;
    const float top = m_bedOriginY + offsetY;
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
