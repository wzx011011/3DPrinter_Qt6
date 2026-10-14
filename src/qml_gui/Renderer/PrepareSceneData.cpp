#include "PrepareSceneData.h"

// Q8(5): single source of the plate-grid layout (gap 1/5, stride,
// col/row decode) shared with the model side (PartPlateList.cpp).
#include "core/model/PartPlateList.h"

#include "core/rendering/PickingRaycaster.h"

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
  // vp-7 (GRIDSTEP): base grid spacings of the upstream adaptive ladder
  // (2DBed.cpp:27-36). kFineGridMm is the <600mm rung; kCoarseGridMm is the
  // >=1200mm rung (previously a dead constant, now live in the ladder).
  constexpr float kFineGridMm = 10.0f;
  constexpr float kCoarseGridMm = 50.0f;

  // vp-7 (GRIDSTEP): upstream Bed_2D::calculate_grid_step ladder
  // (2DBed.cpp:27-36) -- the spacing adapts to the bed's short edge so
  // large beds no longer draw a dense 10mm mesh.
  float adaptiveGridStepMm(float shortEdgeMm)
  {
    if (shortEdgeMm >= 6000.0f)
      return 100.0f;             // short edge >= 6000mm: main grid 5 x 100 = 500mm
    if (shortEdgeMm >= 1200.0f)
      return kCoarseGridMm;      // short edge >= 1200mm: main grid 5 x 50  = 250mm
    if (shortEdgeMm >= 600.0f)
      return 20.0f;              // short edge >= 600mm:  main grid 5 x 20  = 100mm
    return kFineGridMm;          // short edge <  600mm:  main grid 5 x 10  = 50mm
  }

  // Upstream SELECT_COLOR (PartPlate.cpp:86; the load_render_colors /
  // update_render_colors round-trip at PartPlate.cpp:145-152 preserves it).
  // The vp-5 screenshot-adjudicated #515154 variant is withdrawn: it broke
  // the upstream-truth gate (PrepareSceneDataTests selection-color pins).
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

PickingRaycaster *PrepareSceneData::pickingRaycaster()
{
  if (!m_pickingRaycaster) {
    m_pickingRaycaster = std::make_shared<PickingRaycaster>();
    m_pickingRaycaster->build(m_modelVertices, m_modelBatches);
  }
  return m_pickingRaycaster.get();
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

  // AXES-ROT 2026-10-10: the scene keeps treating Z as the bed-depth axis
  // ([origin', origin'+depth]), but the slic3r→scene mapping is now the
  // ROTATION (scene z = -slic3r y, det=+1) instead of the old mirror (+y).
  // Normalizing here means every downstream consumer of the stored rect
  // (grid/border/fill/axes/texture quad/print volume/bed model centering)
  // stays scene-correct with no per-site sign flips: slic3r minY (the
  // origin/front edge) lands at scene max Z (screen-near), matching the
  // upstream topfront view.
  const float normalizedOriginY = -(originY + depth);

  if (nearlyEqual(m_bedWidth, width)
      && nearlyEqual(m_bedDepth, depth)
      && nearlyEqual(m_bedOriginX, originX)
      && nearlyEqual(m_bedOriginY, normalizedOriginY)
      && m_bedShapeType == shapeType
      && nearlyEqual(m_bedDiameter, diameter)) {
    return;
  }

  m_bedWidth = width;
  m_bedDepth = depth;
  m_bedOriginX = originX;
  m_bedOriginY = normalizedOriginY;
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
                                        const QList<QVector4D> &extruderColors,
                                        const QList<QMatrix4x4> &batchWorldTransforms)
{
  // Blob v2 magic ('MDS2'): decomposed volume-local geometry. A v1 blob's
  // first int32 is the batch count (>= 0), so a leading 0x3244534D can only
  // be the S3 magic (that count would exceed kMaxPackedObjects anyway).
  constexpr qint32 kMeshBlobMagicV2 = 0x3244534D;

  clearModelGeometry();

  qint32 objectCount = 0;
  qsizetype offset = 0;
  qint32 leading = 0;
  bool valid = readValue(meshData, offset, leading)
      && leading >= 0;
  const bool decomposed = (leading == kMeshBlobMagicV2);
  objectCount = leading;
  if (decomposed)
    valid = readValue(meshData, offset, objectCount)
        && objectCount >= 0;

  // P15.1 (COLOR): the render-channel arrays are optional (the picking scene
  // passes none); when present they must cover every batch.
  valid = valid
      && objectCount <= kMaxPackedObjects
      && batchSourceObjectIndices.size() == objectCount
      && batchVolumeIndices.size() == objectCount
      && batchInstanceIndices.size() == objectCount
      // Review P2-5: each optional channel must be empty or cover every
      // batch -- a partially supplied array would send .at(objectIndex) out
      // of bounds below (QList::at is unchecked in release).
      && (batchVolumeTypes.isEmpty()
          || batchVolumeTypes.size() == objectCount)
      && (batchExtruderIds.isEmpty()
          || batchExtruderIds.size() == objectCount)
      && (batchPrintableFlags.isEmpty()
          || batchPrintableFlags.size() == objectCount)
      // S3: a decomposed blob without a covering transform list would parse
      // into an identity-composed scene at the wrong place -- fail loud
      // instead (clearModelGeometry above already dropped the old scene).
      && (!decomposed || batchWorldTransforms.size() == objectCount);

  if (valid) {
    // PERF (scene_expand): exact-capacity pre-pass. Walk only the per-batch
    // headers to total the vertices the loop below can actually append
    // (well-indexed batches of active source objects), so multi-million-vertex
    // scenes grow the vertex list once instead of through repeated QList
    // regrowth copies. Pure hint: a malformed header still fails identically
    // in the main loop (the failed expansion clears the geometry either way),
    // and batches dropped mid-loop only ever shrink the appended count.
    qsizetype headerCursor = offset;
    qsizetype reservableVertices = 0;
    bool headersReadable = true;
    for (qint32 headerIndex = 0; headerIndex < objectCount; ++headerIndex) {
      qint32 headerObjectId = 0;
      qint32 headerTriangles = 0;
      if (!readValue(meshData, headerCursor, headerObjectId)
          || !readValue(meshData, headerCursor, headerTriangles)
          || headerTriangles < 0
          || headerTriangles > kMaxPackedTrianglesPerBatch) {
        headersReadable = false;
        break;
      }
      const int headerSource = batchSourceObjectIndices.at(headerIndex);
      if (headerSource < 0 || batchVolumeIndices.at(headerIndex) < 0
          || batchInstanceIndices.at(headerIndex) < 0
          || !activeSourceContains(activeSourceObjectIndices, headerSource))
        continue;
      reservableVertices += qsizetype(headerTriangles) * 3;
    }
    if (headersReadable)
      m_modelVertices.reserve(reservableVertices);
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
      // Review P2-5: each channel is independently optional -- only index
      // into an array that actually covers this batch.
      const int extruderId = batchExtruderIds.size() == objectCount
                                 ? batchExtruderIds.at(objectIndex) : 0;
      const bool printable = batchPrintableFlags.size() == objectCount
                                 ? batchPrintableFlags.at(objectIndex) != 0 : true;
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
    // PREVIEW-GHOST-SHELL: per-volume extruder identity for the ghost pass.
    batch.extruderId =
        batchExtruderIds.size() == objectCount ? batchExtruderIds.at(objectIndex) : 0;

    // PERF (scene_expand): the per-batch payload bounds are already proven
    // above, so the hot loop reads the 3 floats through a raw cursor instead
    // of three bounds-checked readValue calls per vertex. The cursor tracks
    // the same byte positions readValue would leave behind, including the
    // mid-batch break on a non-finite vertex (offset is synced right after
    // the loop; the failed expansion clears the geometry either way).
    const char *const meshDataBase = meshData.constData();
    const char *const batchBase = meshDataBase + offset;
    // S3: decomposed payloads compose through the batch's world transform as
    // they append, so the resident scene keeps its world-space contract
    // (bounds, contours, picking, outside state all read world coordinates
    // unchanged). A mirrored instance (det<0) flips triangle winding at
    // compose time -- the production-side swap only covered the volume's own
    // left-handed transform.
    const QMatrix4x4 batchWorld = decomposed
        ? batchWorldTransforms.at(objectIndex) : QMatrix4x4();
    const bool swapWinding = decomposed && batchWorld.determinant() < 0.0f;
    for (qsizetype vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
      // S3 winding: triangles append in source order (0,1,2); a left-handed
      // world matrix appends (0,2,1) instead.
      const int triLocal = int(vertexIndex % 3);
      const int srcLocal = swapWinding ? ((triLocal == 0) ? 0 : 3 - triLocal)
                                       : triLocal;
      const char *vertexCursor = batchBase
          + qsizetype(vertexIndex - triLocal + srcLocal) * kPackedVertexBytes;
      float x = 0.0f;
      float y = 0.0f;
      float z = 0.0f;
      std::memcpy(&x, vertexCursor, sizeof(float));
      std::memcpy(&y, vertexCursor + sizeof(float), sizeof(float));
      std::memcpy(&z, vertexCursor + 2 * sizeof(float), sizeof(float));
      if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) {
        valid = false;
        break;
      }

      if (!active)
        continue;

      if (decomposed) {
        const QVector3D worldPoint = batchWorld.map(QVector3D(x, y, z));
        x = worldPoint.x();
        y = worldPoint.y();
        z = worldPoint.z();
      }

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
    if (valid)
      offset = qsizetype(batchBase - meshDataBase) + vertexCount * kPackedVertexBytes;

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
    // S3: the resident format + the transform channel the world-space
    // vertices were composed with (index-aligned with m_modelBatches).
    m_meshDecomposedFormat = decomposed;
    m_batchWorldTransforms = decomposed ? batchWorldTransforms
                                        : QList<QMatrix4x4>();
    // P15.5 (SINK): contour bands for sinking batches (bbox dips under the
    // bed plane without being fully below it — upstream is_sinking,
    // 3DScene.cpp:614-620 with SINKING_Z_THRESHOLD = -0.001,
    // Model.hpp:1709).
    rebuildSinkingContours();
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

// P15.11 (MULTICENTER): mirror the full multi-selection (upstream
// Selection::get_bounding_box unions every selected volume). A change marks
// DirtySelection so the highlight + gizmo paths rebuild on the next frame.
void PrepareSceneData::setSelectedSourceObjectIndices(const QList<int> &sourceObjectIndices)
{
  if (sameObjectIndices(m_selectedSourceObjectIndices, sourceObjectIndices))
    return;

  m_selectedSourceObjectIndices = sourceObjectIndices;
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

// S2: masked settle — clear exactly the bits whose channel uploads succeeded
// this frame; failed channels (and untouched payload bits) stay pending and
// re-route on the next frame.
void PrepareSceneData::clearDirtyFlags(quint32 mask)
{
  m_dirtyFlags &= ~mask;
}

// ── S2 pure routing/seaming: the render() single rule reads the registry
// only through these two functions, so the canvas routing and the
// swapchain-rebuild seeding stay unit-locked without a GPU.
quint32 PrepareSceneData::uploadChannelsForCanvas(int canvasType, quint32 dirtyFlags)
{
  const quint32 payload = dirtyFlags & kScenePayloadMask;
  // The camera feed (view/MVP/eye changes) is consumed by MORE channels than
  // the camera UBO: the navigator cube re-projects its overlay pixels
  // through the fresh inverse MVP (uploadNavigatorBuffer reads m_cameraView
  // + m_cameraMvp), so the feed projects onto DirtyNavigator on BOTH
  // canvases — otherwise an orbit would redraw the cube's stale world-space
  // vertices through the new MVP (drift/deformation).
  const bool cameraFeed = (payload & kCameraFeedMask) != 0;
  quint32 channels = dirtyFlags & kRendererChannelMask;
  if (canvasType == kCanvasPreview) {
    // Preview canvas: only its own channels route. Scene-canvas bits seeded
    // before the first synchronize (or set by shared mirror code) stay
    // stored but never route, so they cannot cause a per-frame batch.
    channels &= kPreviewCanvasChannelMask;
    if (cameraFeed) {
      channels |= DirtyCameraUniform;
      channels |= DirtyNavigator;
    }
  } else {
    channels &= kSceneCanvasChannelMask;
    // Any payload change opens the scene composite (the historical (a) gate:
    // bed/model/highlight/assembly re-check their own granularity inside).
    if (payload != DirtyNone)
      channels |= DirtySceneBuffers;
    if (cameraFeed) {
      channels |= DirtyCameraUniform;
      channels |= DirtyNavigator;
    }
    if ((payload & kCutPlaneFeedMask) != 0)
      channels |= DirtyCutPlane;
    if ((payload & kBedTextureFeedMask) != 0)
      channels |= DirtyBedTexture;
  }
  return channels;
}

quint32 PrepareSceneData::seedMaskForCanvas(int canvasType)
{
  // Full payload: the sub-buffer granularity inside the scene/camera channels
  // re-runs once (the retired *Uploaded-flag reset + m_frameCount window did
  // exactly this). Channel bits: everything the canvas can route, so the
  // first frame after initialize() re-uploads every channel — per-channel
  // first-frame visibility by construction.
  if (canvasType == kCanvasPreview)
    return kScenePayloadMask | kPreviewCanvasChannelMask;
  return kScenePayloadMask | kSceneCanvasChannelMask;
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
  // vp-7 (GRIDSTEP): report the live adaptive step (2DBed.cpp:27-36 ladder)
  // so external readers match what rebuildPlateGeometry actually draws.
  return adaptiveGridStepMm(m_bedShapeType == 1
                                ? m_bedDiameter
                                : std::min(m_bedWidth, m_bedDepth));
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

const QList<int> &PrepareSceneData::selectedSourceObjectIndices() const
{
  return m_selectedSourceObjectIndices;
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
  // Q8(5): delegate to the shared helper (upstream PartPlate.hpp:38
  // compute_colum_count) instead of keeping a renderer-local copy -- the
  // model grid (PartPlateList) and the rendered grid can no longer drift.
  return OWzx::compute_colum_count(plateCount);
}

void PrepareSceneData::plateGridOffset(int plateIndex, int plateCount,
                                       float bedWidth, float bedDepth,
                                       float &outOffsetX, float &outOffsetY)
{
  // P15.9 (PLATEANCHOR) + Q8(5): delegate to the shared PartPlateGridLayout
  // helper (upstream compute_shape_position PartPlate.cpp:3206 + gap 1/5
  // PartPlate.cpp:53 + compute_colum_count PartPlate.hpp:38) so the rendered
  // grid IS the model grid. The helper already negates the row stride (the
  // grid runs toward the far rows -- scene -Z under the AXES-ROT mapping)
  // and clamps degenerate index/count inputs exactly like the old local copy.
  double offX = 0.0;
  double offY = 0.0;
  OWzx::PartPlateGridLayout::gridOffset(plateIndex, plateCount,
                                        double(bedWidth), double(bedDepth),
                                        &offX, &offY);
  outOffsetX = float(offX);
  outOffsetY = float(offY);
}

// ── PLATEICONS-GL: bed icon ground rects (upstream PartPlate.cpp:633-700) ──
namespace {
// PartPlate.cpp:61-70 constants (mm at the 200mm reference bed).
constexpr float kIconSize = 16.0f;
constexpr float kNameEditIconSize = 9.0f;
constexpr float kIconGapTop = 3.0f;
constexpr float kIconGapLeft = 3.0f;
constexpr float kIconGapY = 5.0f;
constexpr float kTextOffsetX1 = 3.0f;
constexpr float kTextOffsetX2 = 1.0f;
constexpr float kTextOffsetY = 1.0f;
}  // namespace

float PrepareSceneData::plateIconScale(float bedDepth)
{
  return bedDepth / 200.0f;
}

PrepareSceneData::PlateIconRect PrepareSceneData::plateIconRect(
    int plateIndex, int plateCount, int iconId, float bedWidth, float bedDepth,
    float originX, float originY)
{
  PlateIconRect rect;
  rect.plateIndex = plateIndex;
  rect.iconId = iconId;
  const float f = plateIconScale(bedDepth);
  const float size = kIconSize * f;
  // Column slot = the upstream index order (Delete 0 ... MoveFront 5);
  // NameEdit does not live in the column (pencil sits by the plate name).
  const int slot = iconId >= int(PlateIconId::NameEdit) ? 0 : iconId;
  float offsetX = 0.0f;
  float offsetY = 0.0f;
  plateGridOffset(plateIndex, plateCount, bedWidth, bedDepth, offsetX, offsetY);
  const float farZ = originY + offsetY;
  const float maxX = originX + offsetX + bedWidth;
  // p = bed max corner + (gap_left, -(slot*(size+gap_y)+gap_top)), then the
  // quad spans (p.x, p.y-size)..(p.x+size, p.y) in slic3r bed coords
  // (PartPlate.cpp:641-655). slic3r +Y = far; Qt +Z = near, so the slic3r
  // descent toward the viewer is an INCREASING z here, and the quad's
  // slic3r y-range [p.y - size, p.y] maps to z [top, top + size] with
  // top = farZ + gap_top + slot*(size+gap_y).
  const float top = farZ + (kIconGapTop + float(slot) * (size + kIconGapY)) * f;
  // The column hugs the INSIDE of the plate's right edge. Upstream hangs
  // it just outside the bed extents (gap_left beyond bed_ext max), but the
  // Qt6 viewport does not rasterize quads past the bed-fill boundary
  // (investigated 2026-10-10: correct vertices, correct upload, no pixels
  // for x > bedWidth) -- and hugging the inner edge keeps the printable
  // strip clear for models, honoring the 2026-10-10 user ruling.
  rect.x0 = maxX - kIconGapLeft * f - size;
  rect.x1 = maxX - kIconGapLeft * f;
  rect.z0 = top;
  rect.z1 = top + size;
  return rect;
}

PrepareSceneData::PlateIconRect PrepareSceneData::plateNameEditRect(
    int plateIndex, int plateCount, float nameWidthWorldUnits,
    float bedWidth, float bedDepth, float originX, float originY)
{
  PlateIconRect rect;
  rect.plateIndex = plateIndex;
  rect.iconId = int(PlateIconId::NameEdit);
  const float f = plateIconScale(bedDepth);
  const float size = kNameEditIconSize * f;
  float offsetX = 0.0f;
  float offsetY = 0.0f;
  plateGridOffset(plateIndex, plateCount, bedWidth, bedDepth, offsetX, offsetY);
  // Pencil = name width + text offset Y right of the far-left corner
  // (PartPlate.cpp:660-663: p = bed min corner + (name_width, offset_y)).
  rect.x0 = originX + offsetX + nameWidthWorldUnits + kTextOffsetY * f;
  rect.x1 = rect.x0 + size;
  // The pencil hangs just below the far edge, on the plate (the rendered
  // reference shows the pencil inside the plate outline beside the name).
  rect.z0 = originY + offsetY + kTextOffsetY * f;
  rect.z1 = rect.z0 + size;
  return rect;
}

PrepareSceneData::PlateIconRect PrepareSceneData::plateIndexBadgeRect(
    int plateIndex, int plateCount, float bedWidth, float bedDepth,
    float originX, float originY)
{
  PlateIconRect rect;
  rect.plateIndex = plateIndex;
  rect.iconId = -1;
  const float f = plateIconScale(bedDepth);
  const float size = kIconSize * f;
  float offsetX = 0.0f;
  float offsetY = 0.0f;
  plateGridOffset(plateIndex, plateCount, bedWidth, bedDepth, offsetX, offsetY);
  // calc_vertex_for_number #else branch (PartPlate.cpp:685-695): at the
  // near-right corner, inset by the text offsets on both axes. slic3r
  // measures the badge upward from the near edge (+Y = far = -z here).
  const float nearZ = originY + offsetY + bedDepth;
  rect.x0 = originX + offsetX + bedWidth - kIconGapLeft * f - size + kTextOffsetX1 * f;
  rect.x1 = originX + offsetX + bedWidth - kIconGapLeft * f - kTextOffsetX2 * f;
  rect.z0 = nearZ - (size - kTextOffsetY * f);
  rect.z1 = nearZ - kTextOffsetY * f;
  return rect;
}

// ── Q2 (PLATEICONS-WIP): pure decision seams (see PrepareSceneData.h) ────────
PrepareSceneData::PlateIconAtlasUpload PrepareSceneData::plateIconAtlasUploadDecision(
    quint64 pendingCacheKey, quint64 uploadedCacheKey,
    bool textureValid, const QSize &textureSize, const QSize &atlasSize)
{
  PlateIconAtlasUpload out;
  out.recreateTexture = !textureValid || textureSize != atlasSize;
  // A freshly (re)created texture holds garbage until the first upload, so
  // recreation always forces the pixel transfer; afterwards the cacheKey of
  // the pixels actually resident in the texture gates every re-upload (an
  // unchanged key means the 2048x2048x4 transfer is redundant).
  out.uploadPixels = out.recreateTexture || pendingCacheKey != uploadedCacheKey;
  return out;
}

bool PrepareSceneData::plateIconHoverTargetChanged(int oldPlate, int oldIcon,
                                                   bool hit, int newPlate,
                                                   int newIcon)
{
  // A miss normalizes to the (-1, -1) target so empty->empty moves compare
  // equal and stay no-ops; any plate/icon transition (enter, leave, swap)
  // reports a change.
  const int targetPlate = hit ? newPlate : -1;
  const int targetIcon = hit ? newIcon : -1;
  return targetPlate != oldPlate || targetIcon != oldIcon;
}

// ── Q3 (CAP-GATE): pure decision seam (see PrepareSceneData.h) ───────────────
bool PrepareSceneData::previewCapUploadArmed(int newMoveEnd,
                                             int mirroredMoveEnd,
                                             int lastCapMoveEnd)
{
  // A real move-position change always re-caps. Otherwise the cap only
  // comes back when the GPU-side state is known stale while a partial view
  // is active (moveEnd > 0); moveEnd 0 is the full view -- no cap is drawn,
  // so there is nothing to arm.
  if (newMoveEnd != mirroredMoveEnd)
    return true;
  return newMoveEnd > 0 && lastCapMoveEnd != newMoveEnd;
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
  // vp-7 (GRIDSTEP): upstream sizes the grid off the bed's short edge
  // (PartPlate::calc_gridlines -> Bed_2D::calculate_grid_step,
  // PartPlate.cpp:545-566, 2DBed.cpp:27-36); the circle bed's short edge is
  // its diameter.
  const float gridStep = adaptiveGridStepMm(m_bedShapeType == 1
                                                ? m_bedDiameter
                                                : std::min(m_bedWidth, m_bedDepth));

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

    // Fine grid clipped to the circle: straight lines every gridStep mm
    // (vp-7 GRIDSTEP) with the chord endpoints computed from the circle
    // (upstream clips the same straight lines against the circle polygon,
    // PartPlate.cpp:504-505). Bolder every 5th line measured from the plate
    // edge (upstream generate_grid counts from the plate origin with
    // count % 5, 2DBed.cpp:41-67; the origin line itself is the border, so
    // inner bold lines sit at k = 5/10/15/20 spacings).
    for (int k = 1; ; ++k) {
      const float x = left + float(k) * gridStep;
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
      const float y = top + float(k) * gridStep;
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
  // P15.8/vp-7 (BOLDGRID): every 5th line moves to the ~2px bolder set.
  // Upstream generate_grid counts from the plate origin with
  // `(count % 5 ? lines_thin : lines_bold)` (2DBed.cpp:41-67): count 0 is
  // the origin/border line itself, so inner bold lines sit at k = 5/10/15/20
  // spacings from the edge and the first inner line is thin.
  // PartPlate.cpp:909-911 draws the bold set a second time. X bold lines are
  // exclusive to the bolder set; Y bold lines stay in the fine set too
  // (PartPlate.cpp:478-500).
  int gridIndex = 1; // first inner line sits one spacing from the edge
  for (float x = left + gridStep; x < right; x += gridStep, ++gridIndex) {
    if (gridIndex % kBoldEveryNth == 0) {
      appendBoldLine(x, top, x, bottom, lineR, lineG, lineB);
    } else {
      appendLine(x, top, x, bottom, lineR, lineG, lineB, 0.6f);
      appendBottomLine(x, top, x, bottom);
    }
  }
  gridIndex = 1;
  for (float y = top + gridStep; y < bottom; y += gridStep, ++gridIndex) {
    appendLine(left, y, right, y, lineR, lineG, lineB, 0.6f);
    appendBottomLine(left, y, right, y);
    if (gridIndex % kBoldEveryNth == 0)
      appendBoldLine(left, y, right, y, lineR, lineG, lineB);
  }
}

void PrepareSceneData::clearModelGeometry()
{
  // PICK-BVH: any geometry rebuild invalidates the picking tree; the next
  // pick rebuilds it lazily (upstream MeshRaycaster is rebuilt the same way
  // when a volume's mesh changes).
  m_pickingRaycaster.reset();
  m_modelVertices.clear();
  m_modelBatches.clear();
  m_modelBounds = ModelBounds{};
  m_hasModelBounds = false;
  // S3: the decomposed format + its transform channel describe the resident
  // batches; with the batches gone they are meaningless.
  m_meshDecomposedFormat = false;
  m_batchWorldTransforms.clear();
  // P15.5 (SINK)/P15.3 (OUTOFBED): derived state follows the batches.
  m_sinkingContourVertices.clear();
  m_sinkingContourRanges.clear();
  m_anyVolumeOutside = false;
}

// S3: contour bands rebuilt from the resident world-space batches. Shared by
// the parse path and the transform-only recompose (sinking contact bands
// move with instance transforms).
void PrepareSceneData::rebuildSinkingContours()
{
  m_sinkingContourVertices.clear();
  m_sinkingContourRanges.clear();
  for (const ModelBatch &batch : m_modelBatches) {
    if (batch.bounds.minY < kSinkingThreshold
        && batch.bounds.maxY >= kSinkingThreshold)
      appendSinkingContours(batch);
  }
}

// S3: recompose one batch's vertex range in place through a new world
// matrix. World matrix with det<0 flips triangle winding per triple (the
// same compose-time flip the parse path applies to decomposed payloads).
void PrepareSceneData::recomposeBatchRange(ModelBatch &batch,
                                           const QMatrix4x4 &world,
                                           bool swapWinding)
{
  const int first = std::max(0, batch.firstVertex);
  const int end = static_cast<int>(std::min<qsizetype>(
      batch.firstVertex + batch.vertexCount, m_modelVertices.size()));
  for (int i = first; i < end; i += 3) {
    const int tripleEnd = std::min(i + 3, end);
    if (swapWinding && tripleEnd - i == 3) {
      // (0,1,2) -> (0,2,1): exchange the second and third vertex.
      std::swap(m_modelVertices[i + 1], m_modelVertices[i + 2]);
    }
    for (int j = i; j < tripleEnd; ++j) {
      ModelVertex &v = m_modelVertices[j];
      const QVector3D p = world.map(QVector3D(v.x, v.y, v.z));
      v.x = p.x();
      v.y = p.y();
      v.z = p.z();
    }
  }

  bool batchHasBounds = false;
  for (int i = first; i < end; ++i) {
    const ModelVertex &v = m_modelVertices.at(i);
    if (!batchHasBounds) {
      batch.bounds = ModelBounds{v.x, v.y, v.z, v.x, v.y, v.z};
      batchHasBounds = true;
    } else {
      batch.bounds.minX = std::min(batch.bounds.minX, v.x);
      batch.bounds.minY = std::min(batch.bounds.minY, v.y);
      batch.bounds.minZ = std::min(batch.bounds.minZ, v.z);
      batch.bounds.maxX = std::max(batch.bounds.maxX, v.x);
      batch.bounds.maxY = std::max(batch.bounds.maxY, v.y);
      batch.bounds.maxZ = std::max(batch.bounds.maxZ, v.z);
    }
  }
}

void PrepareSceneData::setInstanceTransforms(
    const QList<QMatrix4x4> &batchWorldTransforms)
{
  // Legacy world-baked blobs carry their transforms inside the vertices --
  // a channel update must not compose a second time on top.
  if (!m_meshDecomposedFormat)
    return;
  if (m_batchWorldTransforms == batchWorldTransforms)
    return;

  const int batchCount = m_modelBatches.size();
  const bool fullRecompose = batchWorldTransforms.size() != batchCount;
  bool anyChanged = false;
  for (int batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
    const QMatrix4x4 &next = fullRecompose
        ? batchWorldTransforms.value(batchIndex)
        : batchWorldTransforms.at(batchIndex);
    if (!fullRecompose && next == m_batchWorldTransforms.at(batchIndex))
      continue; // untouched batch: no vertex recompose, no bounds rebuild
    ModelBatch &batch = m_modelBatches[batchIndex];
    const bool swapWinding = next.determinant() < 0.0f;
    recomposeBatchRange(batch, next, swapWinding);
    anyChanged = true;
    m_batchWorldTransforms[batchIndex] = next;
  }
  // Full-recompose mode may also SHRINK the channel (protocol violation from
  // the producer); keep the stored list aligned with what was applied.
  if (fullRecompose) {
    m_batchWorldTransforms = batchWorldTransforms;
    anyChanged = true;
  }
  if (!anyChanged)
    return;

  // Scene bounds = union of the per-batch bounds (the batches tile the
  // vertex list, so the union equals the per-vertex scan the parse does).
  m_modelBounds = ModelBounds{};
  m_hasModelBounds = false;
  for (const ModelBatch &batch : m_modelBatches) {
    if (batch.vertexCount <= 0)
      continue;
    if (!m_hasModelBounds) {
      m_modelBounds = batch.bounds;
      m_hasModelBounds = true;
      continue;
    }
    m_modelBounds.minX = std::min(m_modelBounds.minX, batch.bounds.minX);
    m_modelBounds.minY = std::min(m_modelBounds.minY, batch.bounds.minY);
    m_modelBounds.minZ = std::min(m_modelBounds.minZ, batch.bounds.minZ);
    m_modelBounds.maxX = std::max(m_modelBounds.maxX, batch.bounds.maxX);
    m_modelBounds.maxY = std::max(m_modelBounds.maxY, batch.bounds.maxY);
    m_modelBounds.maxZ = std::max(m_modelBounds.maxZ, batch.bounds.maxZ);
  }
  // Derived world-space state follows the moved batches. PICK-BVH: the tree
  // spans world-space triangles, so a moved batch invalidates it.
  m_pickingRaycaster.reset();
  rebuildSinkingContours();
  recomputeOutsideState();
  ++m_instanceTransformGeneration;
  // THE S3 cheap path ends here: the registry arms exactly what a mesh
  // revision would arm, but no blob was re-produced and no payload
  // re-parsed -- render()'s single rule re-uploads from the recomposed
  // resident vertices.
  markDirty(DirtyMesh | DirtyGpu);
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
    // triangles (p0,p1,p2) and (p0,p2,p3). AXES-ROT: the stream carries
    // slic3r bed-absolute Y; scene Z negates it (dy already folds in the
    // normalized rect origin + plate offset).
    m_bedFillVertices.append(Vertex{px[0] + dx, -py[0] + dy, r, g, b, 1.0f});
    m_bedFillVertices.append(Vertex{px[1] + dx, -py[1] + dy, r, g, b, 1.0f});
    m_bedFillVertices.append(Vertex{px[2] + dx, -py[2] + dy, r, g, b, 1.0f});
    m_bedFillVertices.append(Vertex{px[0] + dx, -py[0] + dy, r, g, b, 1.0f});
    m_bedFillVertices.append(Vertex{px[2] + dx, -py[2] + dy, r, g, b, 1.0f});
    m_bedFillVertices.append(Vertex{px[3] + dx, -py[3] + dy, r, g, b, 1.0f});
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
  // AXES-ROT: the axes origin is slic3r (minX, minY) = the plate's scene
  // NEAR corner (max Z) under the rotation mapping — the normalized
  // m_bedOriginY is the plate's far (min-Z) edge, so the near edge adds the
  // depth back.
  const float origin[3] = {left, kAxesGroundZ, top + m_bedDepth};

  // x axis (upstream assemble_transform(origin, {0, 0.5*PI, 0}), 3DBed.cpp:232)
  const float dirX[3] = {1.0f, 0.0f, 0.0f};
  appendArrow(origin, dirX, stemLength, kAxisXR, 0.0f, 0.0f);
  // y axis (upstream {-0.5*PI, 0, 0}, 3DBed.cpp:236); slic3r +Y = scene -Z
  // under the AXES-ROT rotation mapping.
  const float dirZ[3] = {0.0f, 0.0f, -1.0f};
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
      // qt remap (a,b,c)->(a,c,b) puts the arrow length (upstream z) on the
      // depth axis; AXES-ROT: slic3r +Y = scene -Z, so qz negates (the
      // arrow points away from the viewer, Bed3D.cpp:236).
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
