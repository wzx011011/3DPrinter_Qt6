#pragma once

#include <QByteArray>
#include <QList>
#include <QSize>
#include <QVariant>
#include <QVector4D>
#include <QtGlobal>

#include <memory>

class PickingRaycaster;

// ═══════════════════════════════════════════════════════════════════════════
// S2 (2026-10-14): the item↔renderer mirror contract — single point of truth
// ═══════════════════════════════════════════════════════════════════════════
// One RhiViewport (GUI thread) drives one RhiViewportRenderer (render
// thread). The renderer never calls into the item outside synchronize();
// everything render() consumes is a renderer-owned mirror written during
// synchronize(). Every cross-thread value transfer follows exactly one of
// three sanctioned change-detection protocols — a new channel picks one of
// these, never invents a fourth:
//
//   1. GENERATION (heavy payloads): the item bumps a qint64 counter in its
//      property setter (meshData → m_modelGeneration, scene bed shape →
//      m_sceneGeneration, previewData → m_previewDataGeneration, bed model
//      STL → m_bedModelGeneration); synchronize() compares the counter
//      (O(1)) and re-mirrors the payload only on a bump. Byte/byte compares
//      of multi-megabyte payloads on the render thread are forbidden.
//   2. CONTENT KEY (cheap payloads): small byte streams / scalars / lists
//      (overlay markers, clearance streams, gizmo fields, assemble lists,
//      marker floats) are compared directly in synchronize(). A content key
//      may only be used when a full compare is cheaper than the re-mirror it
//      guards.
//   3. DOCUMENTED LATCH (thread handoff only): when the GUI thread produces
//      an object asynchronously and synchronize() must pull exactly the
//      finished artifact (plate-icon atlas image + quad bytes, bed-texture
//      path, camera-dirty pulse, thumbnail request), the item side exposes a
//      boolean handoff latch that synchronize() consumes (resets) on pull.
//      A latch documents WHO hands WHAT to WHOM — it is not a generic dirty
//      flag, and it must not gate GPU uploads by itself.
//
// WRITE MUST BE READ: every mirror write lands in the S2 upload-channel
// registry below (markDirty). render() gates ALL uploads through one rule —
// read the registry once, allocate one batch, dispatch each pending channel,
// clear the bits that settled. There is no second gate list, no per-frame
// unconditional batch, and no first-N-frames window: a swapchain rebuild
// re-arms the registry through seedMaskForCanvas(). Payload writes go
// through the setters, which mark the payload bits (DirtyBed..DirtyCamera);
// renderer mirror writes mark their channel bits
// (DirtySceneBuffers..DirtyCameraUniform) directly on the renderer's
// m_prepareScene.
// ═══════════════════════════════════════════════════════════════════════════

class PrepareSceneData
{
public:
  struct Vertex
  {
    float x = 0.0f;
    float y = 0.0f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    bool operator==(const Vertex &other) const;
  };

  struct ModelVertex
  {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
  };

  struct ModelBounds
  {
    float minX = 0.0f;
    float minY = 0.0f;
    float minZ = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    float maxZ = 0.0f;
  };

  struct ModelBatch
  {
    int renderObjectId = -1;
    int sourceObjectIndex = -1;
    int volumeIndex = -1;
    int instanceIndex = -1;
    int firstVertex = 0;
    int vertexCount = 0;
    ModelBounds bounds;
    // P15.2 (COLOR): alpha < 1 batches render in the second, depth-sorted
    // translucent pass (upstream _render_objects(Transparent)). Declared
    // after bounds so historical aggregate initializers keep compiling.
    bool translucent = false;
    // P15.3 (OUTOFBED): ProjectVolumeType int mirrored from the render
    // channels (0 = model part). Only model parts participate in the
    // outside-state check (upstream
    // shader_outside_printer_detection_enabled = is_model_part,
    // 3DScene.cpp:728). Trailing field keeps aggregate initializers valid.
    int volumeType = 0;
    // PREVIEW-GHOST-SHELL: upstream ModelVolume::extruder_id() for the batch
    // (0/absent when the render-channel arrays were not supplied). Consumed
    // by the preview ghost-shell pass to tint per volume
    // (update_colors_by_extruder, 3DScene.cpp:1216-1224). Trailing field.
    int extruderId = 0;
  };

  // P15.3 (OUTOFBED): upstream PrintVolumeDetection (gouraud.fs:11-22),
  // expressed in the Qt scene frame (X=right, Y=up, Z=depth) consumed
  // directly by the model_lit fragment stage.
  struct PrintVolume
  {
    int type = -1; // 0 = rectangle, 1 = circle, -1 = disabled (gouraud.fs:13)
    // rect: (minX, minZ, maxX, maxZ); circle: (cx, cz, radius, 0)
    QVector4D xyData{0.0f, 0.0f, 0.0f, 0.0f};
    // height bounds on the Qt Y axis (upstream z_data: (min z, max z))
    float zMin = 0.0f;
    float zMax = 0.0f;
  };

  // P15.5 (SINK): one bed-plane contour band range per sinking batch, into
  // sinkingContourVertices() (upstream GLVolume::SinkingContours, one band
  // model per volume, 3DScene.cpp:108-161).
  struct SinkingContourRange
  {
    int sourceObjectIndex = -1;
    int firstVertex = 0;
    int vertexCount = 0;
  };

  // ── S2 single dirty registry ─────────────────────────────────────────────
  // One word, two flag families:
  //  * payload bits (DirtyBed..DirtyCamera): WHAT scene data changed. Set by
  //    the setters, consumed as upload granularity by the scene/camera
  //    channels (uploadBedBuffers / uploadModelBuffer / uploadCameraUniform).
  //  * renderer channel bits (DirtySceneBuffers..DirtyCameraUniform): WHICH
  //    GPU upload channel is pending. Set by the renderer's synchronize()
  //    mirror writes / initialize() seeding, consumed ONLY through
  //    uploadChannelsForCanvas() by the single render() rule.
  // DirtyCameraUniform is a derived bit (never stored): routing projects the
  // camera feed payload onto it so both canvases share one camera channel.
  enum DirtyFlag : quint32
  {
    DirtyNone = 0,
    // payload bits
    DirtyBed = 1u << 0,
    DirtyPlate = 1u << 1,
    DirtyMesh = 1u << 2,
    DirtyVisibility = 1u << 3,
    DirtyGpu = 1u << 4,
    DirtySelection = 1u << 5,
    DirtyCamera = 1u << 6,
    // renderer channel bits (S2 registry)
    DirtySceneBuffers = 1u << 8,        // bed/model/highlight/assembly composite
    DirtyGizmoBuffer = 1u << 9,         // shared gizmo + selection-center + sidebar hints
    DirtyCutPlane = 1u << 10,
    DirtyWipeTower = 1u << 11,
    DirtyPaintOverlay = 1u << 12,
    DirtyBrushCursor = 1u << 13,
    DirtyHollowMarker = 1u << 14,
    DirtyAdvancedCutMarker = 1u << 15,
    DirtyMeasureOverlay = 1u << 16,
    DirtyFlattenHover = 1u << 17,
    DirtySequentialClearance = 1u << 18,
    DirtyPlateIconAtlas = 1u << 19,     // thread-handoff latch arms this
    DirtyPlateIconQuads = 1u << 20,     // thread-handoff latch arms this
    DirtyBedTexture = 1u << 21,
    DirtyBedModel = 1u << 22,
    DirtyBedTypeParts = 1u << 23,
    DirtyPreviewSegment = 1u << 24,
    DirtyGhostShell = 1u << 25,
    DirtyToolMarker = 1u << 26,
    DirtyPreviewCap = 1u << 27,         // Q3 seam arms this (was a boolean latch)
    DirtyNavigator = 1u << 28,
    DirtyCameraUniform = 1u << 29,      // derived from kCameraFeedMask by routing
  };

  // Registry layout constants (pure data, locked by static_asserts below and
  // exercised by PrepareSceneDataTests).
  static constexpr quint32 kScenePayloadMask =
      DirtyBed | DirtyPlate | DirtyMesh | DirtyVisibility | DirtyGpu
      | DirtySelection | DirtyCamera;
  // Payload bits whose change is reflected inside the camera UBO pack
  // (mvp/gizmoCenter/view/printVolume — uploadCameraUniform's feed).
  static constexpr quint32 kCameraFeedMask =
      DirtyCamera | DirtyGpu | DirtySelection | DirtyBed;
  // Payload bits whose change re-anchors the cut plane on the selected
  // object (uploadCutPlaneBuffers' legacy feed).
  static constexpr quint32 kCutPlaneFeedMask = DirtyMesh | DirtySelection | DirtyGpu;
  // Payload bits whose change re-anchors the bed-texture quad on the bed rect.
  static constexpr quint32 kBedTextureFeedMask = DirtyBed | DirtyGpu;
  // Every stored channel bit (DirtyCameraUniform is derived, never stored).
  static constexpr quint32 kRendererChannelMask =
      DirtySceneBuffers | DirtyGizmoBuffer | DirtyCutPlane | DirtyWipeTower
      | DirtyPaintOverlay | DirtyBrushCursor | DirtyHollowMarker
      | DirtyAdvancedCutMarker | DirtyMeasureOverlay | DirtyFlattenHover
      | DirtySequentialClearance | DirtyPlateIconAtlas | DirtyPlateIconQuads
      | DirtyBedTexture | DirtyBedModel | DirtyBedTypeParts
      | DirtyPreviewSegment | DirtyGhostShell | DirtyToolMarker
      | DirtyPreviewCap | DirtyNavigator;
  // Channels routable per canvas family. Canvas values mirror
  // RhiViewport::CanvasType (equality is locked by static_assert in
  // RhiViewport.h — PrepareSceneData cannot include that header).
  static constexpr int kCanvasView3D = 0;
  static constexpr int kCanvasPreview = 1;
  static constexpr int kCanvasAssembleView = 2;
  static constexpr quint32 kSceneCanvasChannelMask =
      DirtySceneBuffers | DirtyGizmoBuffer | DirtyCutPlane | DirtyWipeTower
      | DirtyPaintOverlay | DirtyBrushCursor | DirtyHollowMarker
      | DirtyAdvancedCutMarker | DirtyMeasureOverlay | DirtyFlattenHover
      | DirtySequentialClearance | DirtyPlateIconAtlas | DirtyPlateIconQuads
      | DirtyBedTexture | DirtyBedModel | DirtyBedTypeParts | DirtyNavigator;
  static constexpr quint32 kPreviewCanvasChannelMask =
      DirtyPreviewSegment | DirtyGhostShell | DirtyToolMarker
      | DirtyPreviewCap | DirtyNavigator;

  static_assert((kRendererChannelMask & kScenePayloadMask) == 0,
                "channel bits and payload bits must not collide");
  static_assert((kSceneCanvasChannelMask & ~kRendererChannelMask) == 0,
                "scene canvas mask must only name registry channels");
  static_assert((kPreviewCanvasChannelMask & ~kRendererChannelMask) == 0,
                "preview canvas mask must only name registry channels");

  // ── S2 pure routing/seeding seams (unit-locked, GPU-free) ────────────────
  // uploadChannelsForCanvas is THE render() gate: given the raw registry
  // word, return the channel bits this canvas must upload this frame. A
  // stored bit masked out for the canvas stays stored (harmless — a renderer
  // instance never changes canvas); payload bits are PROJECTED onto their
  // channel feeds (scene composite / camera / cut plane / bed-texture quad).
  static quint32 uploadChannelsForCanvas(int canvasType, quint32 dirtyFlags);
  // seedMaskForCanvas re-arms the registry after a swapchain rebuild: every
  // channel the canvas can route plus the full payload (the D3D12-safe
  // "first frame after initialize() re-uploads everything" guarantee of the
  // retired m_frameCount<=3 force window, expressed as registry state).
  static quint32 seedMaskForCanvas(int canvasType);

  PrepareSceneData();
  // PICK-BVH: movable, non-copyable. Moves are what keep scene builders
  // ergonomic without ever duplicating the (large) expanded vertex lists;
  // the shared_ptr member tolerates the incomplete raycaster type.
  PrepareSceneData(PrepareSceneData &&) = default;
  PrepareSceneData &operator=(PrepareSceneData &&) = default;

  void setBed(float widthMm,
              float depthMm,
              float originX,
              float originY,
              int shapeType,
              float diameterMm);
  void setShowBed(bool showBed);
  /// Upstream compute_colum_count (PartPlate.hpp:38): ceil(sqrt(count)).
  /// Public: the renderer bakes plate-grid offsets with the same formula.
  static int computePlateColumns(int plateCount);
  // P15.9 (PLATEANCHOR): per-plate grid offsets (upstream
  // PartPlate::compute_shape_position, PartPlate.cpp:3206: stride = bed size
  // * (1 + LOGICAL_PART_PLATE_GAP), PartPlate.cpp:53). Writes the slic3r-frame
  // left/top offsets of `plateIndex` relative to the bed origin; callers add
  // their own bed origin. Shared by the bed geometry, the axes anchor, the
  // height-limit rings, the renderer's bed-type offset, and the RhiViewport
  // plateAnchors projection so every consumer uses one plate layout.
  static void plateGridOffset(int plateIndex, int plateCount,
                              float bedWidth, float bedDepth,
                              float &outOffsetX, float &outOffsetY);
  // ── PLATEICONS-GL (2026-10-10): bed-surface plate icons ─────────────────
  // Upstream PartPlate::calc_vertex_for_icons / calc_vertex_for_number /
  // calc_vertex_for_plate_name_edit_icon (PartPlate.cpp:633-700, 3409-3422)
  // draws each plate's action icons as textured quads pinned to the bed
  // plane (GROUND_Z), so they move with the plate, deform with the camera
  // perspective, and are occluded by objects. One column at the plate's
  // max-X edge (overhanging gap-left outside the bed), descending from the
  // far edge; the index badge sits at the near-right corner; the rename
  // pencil sits right of the plate-name text at the far-left corner.
  // Qt scene frame: ground y=0, +Z toward the viewer; bedOriginY is the
  // plate's far (min-Z) edge (PrepareSceneData.cpp bed extents comment).
  enum class PlateIconId : int
  {
    Delete = 0,
    Orient = 1,
    Arrange = 2,
    Lock = 3,
    Settings = 4,
    MoveFront = 5,
    NameEdit = 6,
  };
  struct PlateIconRect
  {
    int plateIndex = 0;
    int iconId = 0;          // PlateIconId
    float x0 = 0, z0 = 0;    // min corner (ground plane)
    float x1 = 0, z1 = 0;    // max corner
  };
  // Upstream scale factor: bed_ext.size()(1) / 200 (PartPlate.cpp:639).
  static float plateIconScale(float bedDepth);
  // Upstream icon slots (PartPlate.cpp:3409-3422, single-extruder layout:
  // move-front takes slot 5; the dual-extruder filament-map slot is not
  // drawn). Returns the ground rect for one icon of one plate. iconId is a
  // PlateIconId value; the column slot follows the upstream index order
  // (Delete=0 ... MoveFront=5). plateCount feeds the grid column count, so
  // per-plate offsets stay consistent with the bed geometry.
  static PlateIconRect plateIconRect(int plateIndex, int plateCount,
                                     int iconId,
                                     float bedWidth, float bedDepth,
                                     float originX, float originY);
  // Upstream calc_vertex_for_plate_name_edit_icon (PartPlate.cpp:646-676):
  // 9-unit pencil right of the plate-name text at the far-left corner.
  static PlateIconRect plateNameEditRect(int plateIndex, int plateCount,
                                         float nameWidthWorldUnits,
                                         float bedWidth, float bedDepth,
                                         float originX, float originY);
  // Upstream calc_vertex_for_number #else branch (PartPlate.cpp:672-696):
  // index badge inset from the near-right corner.
  static PlateIconRect plateIndexBadgeRect(int plateIndex, int plateCount,
                                           float bedWidth, float bedDepth,
                                           float originX, float originY);
  // ── Q2 (PLATEICONS-WIP): pure decision seams for the icon pipeline ─────
  // The renderer must not hand-roll these conditions: the upload gate and
  // the hover early-exit are the two contracts that used to be "written but
  // never read" (m_plateIconAtlasCacheKey, hit-flag-only hover guard), and
  // both are permanently locked by PrepareSceneDataTests without a GPU.
  //
  // Atlas upload gate: pendingCacheKey identifies the atlas image staged by
  // the last synchronize(); uploadedCacheKey identifies the pixels actually
  // resident in the GPU texture. Equal keys + a valid same-sized texture
  // mean the 2048x2048x4 transfer can be skipped entirely.
  struct PlateIconAtlasUpload
  {
    bool recreateTexture = false;  // GPU texture missing or size mismatch
    bool uploadPixels = false;     // atlas pixels differ from the texture
  };
  static PlateIconAtlasUpload plateIconAtlasUploadDecision(
      quint64 pendingCacheKey, quint64 uploadedCacheKey,
      bool textureValid, const QSize &textureSize, const QSize &atlasSize);
  // Hover refresh gate: keyed on the hover TARGET (plate+icon; -1/-1 = not
  // hovering), not on the raw hit flag -- an empty-area mouse move is a
  // no-op instead of a full quad rebuild + sync-dirty latch.
  static bool plateIconHoverTargetChanged(int oldPlate, int oldIcon,
                                          bool hit, int newPlate, int newIcon);
  // ── Q3 (CAP-GATE): pure decision seam for the preview sequential-range
  // cap upload latch ──────────────────────────────────────────────────────
  // RhiViewportRenderer::synchronize() must not hand-roll this condition:
  // its former unconditional m_previewCapUploadPending = true neutered the
  // moveEnd change detection, so every preview frame with moveEnd > 0
  // re-ran rebuildPreviewCap's linear span scan (m_previewDrawSpans can hold
  // millions of spans) and re-uploaded the cap buffer even with the move
  // slider parked. The arm decision:
  //  - newMoveEnd != mirroredMoveEnd: the viewport's move position actually
  //    moved since the last synchronize() -> re-cap on the next frame;
  //  - newMoveEnd > 0 && lastCapMoveEnd != newMoveEnd: the GPU-side cap is
  //    missing or stale (renderer boot / first frame, resetPreviewGpuState
  //    after a payload swap or device reset dropped it to -1) even though
  //    moveEnd held still.
  // Inputs are the renderer's mirrors: mirroredMoveEnd is the last value
  // synchronize() stored (m_moveEnd); lastCapMoveEnd is the moveEnd the last
  // rebuildPreviewCap() attempt reflects (-1 = nothing resident).
  static bool previewCapUploadArmed(int newMoveEnd, int mirroredMoveEnd,
                                    int lastCapMoveEnd);
  void setPlateContext(int currentPlateIndex, int plateCount, const QList<int> &activeObjectIndices);
  void setMeshGeneration(qint64 generation);
  // P15.1/15.2 (COLOR): full render-channel form. batchVolumeTypes values are
  // ProjectVolumeType ints; extruderIds are upstream ModelVolume::extruder_id()
  // (1-based); extruderColors are the parsed filament_colour entries
  // (rgba 0..1, index 0 = extruder 1).
  void setModelMeshData(const QByteArray &meshData,
                        const QList<int> &batchSourceObjectIndices,
                        const QList<int> &batchVolumeIndices,
                        const QList<int> &batchInstanceIndices,
                        const QList<int> &activeSourceObjectIndices,
                        const QList<int> &batchVolumeTypes,
                        const QList<int> &batchExtruderIds,
                        const QList<int> &batchPrintableFlags,
                        const QList<QVector4D> &extruderColors);
  void setModelMeshData(const QByteArray &meshData,
                        const QList<int> &batchSourceObjectIndices,
                        const QList<int> &batchVolumeIndices,
                        const QList<int> &batchInstanceIndices,
                        const QList<int> &activeSourceObjectIndices)
  {
    setModelMeshData(meshData, batchSourceObjectIndices, batchVolumeIndices,
                     batchInstanceIndices, activeSourceObjectIndices,
                     QList<int>(), QList<int>(), QList<int>(),
                     QList<QVector4D>());
  }
  void setModelMeshData(const QByteArray &meshData,
                        const QList<int> &batchSourceObjectIndices,
                        const QList<int> &activeSourceObjectIndices)
  {
    setModelMeshData(meshData, batchSourceObjectIndices,
                     QList<int>(batchSourceObjectIndices.size(), 0),
                     QList<int>(batchSourceObjectIndices.size(), 0),
                     activeSourceObjectIndices);
  }
  void setSelectedSourceObjectIndex(int sourceObjectIndex);
  // P15.11 (MULTICENTER): ALL selected source object indices (upstream
  // Selection::get_bounding_box() unions every selected volume's bounds).
  // Consumers: the gizmo pivot, the white selection-center sphere, and the
  // white corner-tick highlight box all union across this list.
  void setSelectedSourceObjectIndices(const QList<int> &sourceObjectIndices);
  void setHoveredSourceObjectIndex(int sourceObjectIndex);
  // v5.16 (EXCLAREA): bed_exclude_area FLAT point stream [x1,y1,x2,y2,...]
  // (upstream coPoints; init_exclude_bounding_box groups every 4 points into
  // one rectangle). Bed-relative coordinates.
  void setBedExcludeAreas(const QVariantList &areas);
  const QVariantList &bedExcludeAreas() const { return m_bedExcludeAreas; }
  // v5.16 (HTLIMIT): upstream calc_height_limit/render_height_limit
  // (PartPlate.cpp:512-561/914) — ByObject plates draw clearance rings at
  // extruder_clearance_height_to_rod/_to_lid.
  void setHeightLimit(bool active, float heightToRod, float heightToLid);
  // P15.3 (OUTOFBED): print volume height clamp (upstream z_data =
  // {0, printable_height}, GLCanvas3D.cpp:7183/7189). Not yet plumbed from
  // the viewmodel, so the default is unbounded (FLT_MAX) which disables the
  // top-height darkening while the XZ boundary test stays exact.
  void setPrintableHeight(float height);
  void markCameraDirty();

  // ── S2 registry access (single render() rule + renderer mirror writes) ──
  // markDirty arms registry/payload bits (idempotent); clearDirtyFlags(mask)
  // settles exactly the handled bits so a failed channel stays pending while
  // its siblings' work is kept; takeDirtyFlags consumes the whole word (the
  // all-channels-settled fast path).
  void markDirty(quint32 flags);
  quint32 peekDirtyFlags() const;
  quint32 takeDirtyFlags();
  void clearDirtyFlags();
  void clearDirtyFlags(quint32 mask);

  float bedWidth() const;
  float bedDepth() const;
  float bedOriginX() const;
  float bedOriginY() const;
  int bedShapeType() const;
  float bedDiameter() const;
  bool showBed() const;

  float fineGridSpacingMm() const;
  float coarseGridSpacingMm() const;

  int currentPlateIndex() const;
  int plateCount() const;
  const QList<int> &activeObjectIndices() const;
  qint64 meshGeneration() const;

  const QList<Vertex> &bedFillVertices() const;
  const QList<Vertex> &bedLineVertices() const;
  // v5.16 (BEDBOTTOM): grid-only line set for below-horizon camera views
  // (upstream PartPlate::render_grid(true) draws the grid in
  // LINE_BOTTOM_COLOR while render_background/render_logo are skipped).
  const QList<Vertex> &bedBottomLineVertices() const;
  // P15.8 (BOLDGRID): every-5th grid line as ~2px-wide flat quads in the
  // plate line color (upstream render_grid draws the bolder line set a
  // second time with glLineWidth(2), PartPlate.cpp:909-911).
  const QList<Vertex> &bedBoldLineVertices() const;
  // P15.5 (SINK): white bed-plane contour bands of sinking batches
  // (upstream GLVolume::SinkingContours, 3DScene.cpp:108-161), split into
  // per-batch ranges so the renderer can draw hovered/displaced ranges
  // depth-unconditional (glDepthFunc(GL_ALWAYS), 3DScene.cpp:1018-1034).
  const QList<ModelVertex> &sinkingContourVertices() const;
  const QList<SinkingContourRange> &sinkingContourRanges() const;
  // P15.7 (AXES): origin arrows (X red / Y green / Z blue, upstream
  // Bed3D::Axes::render 3DBed.cpp:183-245) as lit-shader triangles.
  const QList<ModelVertex> &bedAxesVertices() const;
  const QList<ModelVertex> &bedLimitVertices() const;
  const QList<ModelVertex> &modelVertices() const;
  const QList<ModelBatch> &modelBatches() const;
  const ModelBounds &modelBounds() const;
  bool hasModelBounds() const;
  // PICK-BVH: BVH-accelerated nearest-triangle pick over the current model
  // batches -- the Qt6 counterpart of the upstream per-volume
  // GUI::MeshRaycaster AABB tree (MeshUtils.hpp:159). The tree is rebuilt
  // lazily on the first pick after every mesh revision (setModelMeshData /
  // clearModelGeometry drop it); input handling and nearest-hit semantics are
  // identical to ObjectPicking::pick. Returns null never: an empty scene
  // yields a valid-but-empty raycaster whose pick reports no hit.
  PickingRaycaster *pickingRaycaster();
  int selectedSourceObjectIndex() const;
  // P15.11 (MULTICENTER): full multi-selection index list (sorted).
  const QList<int> &selectedSourceObjectIndices() const;
  int hoveredSourceObjectIndex() const;
  bool containsCurrentPlatePoint(float x, float z) const;
  // P15.3 (OUTOFBED): print volume uniforms for the current plate
  // (upstream m_volumes.set_print_volume, GLCanvas3D.cpp:7177-7205).
  const PrintVolume &printVolume() const;
  // P15.3 (OUTOFBED): true when any model-part instance of the current
  // plate is not fully inside the print volume (upstream
  // GLVolumeCollection::check_outside_state + _is_any_volume_outside,
  // 3DScene.cpp:1043-1171 / GLCanvas3D.cpp:7069-7090 — turns the viewport
  // background to the upstream error color).
  bool anyVolumeOutside() const;
  float printableHeight() const { return m_printableHeight; }

private:
  static float sanitizeExtent(float value, float fallback);
  static bool nearlyEqual(float left, float right);
  static bool sameObjectIndices(const QList<int> &left, const QList<int> &right);

  void rebuildBedGeometry();
  void rebuildPlateGeometry(int plateRow, int plateCol, float left, float top, bool selected);
  void clearModelGeometry();
  void updateModelBounds(const ModelVertex &vertex);
  static bool activeSourceContains(const QList<int> &activeSourceObjectIndices, int sourceObjectIndex);
  static quint32 colorForSourceObject(int sourceObjectIndex, float &r, float &g, float &b);
  void appendLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a);
  void appendBottomLine(float x1, float y1, float x2, float y2);
  // P15.8 (BOLDGRID): ~2px-wide bolder grid line as a flat quad (two
  // triangles). Bottom-view twin: two parallel 1px lines (appendBottomLine).
  void appendBoldLine(float x1, float y1, float x2, float y2, float r, float g, float b);
  void appendRectFill(float left, float top, float right, float bottom,
                      float r, float g, float b, float a);
  void appendRectBorder(float left, float top, float right, float bottom,
                        float r, float g, float b);
  void appendExcludeFills(float plateLeft, float plateTop, bool selected);
  void rebuildHeightLimitGeometry();
  // P15.3 (OUTOFBED): print volume + per-instance outside state refresh
  // (bed shape and/or model batches changed).
  void updatePrintVolume();
  void recomputeOutsideState();
  // P15.5 (SINK): bed-plane contour band of one sinking batch.
  void appendSinkingContours(const ModelBatch &batch);
  // P15.7 (AXES): stilized arrow mesh along an axis (GLModel.cpp:850-932
  // stilized_arrow port), emitted as colored triangles.
  void appendArrow(const float origin[3], const float dir[3],
                   float stemLength, float r, float g, float b);
  void rebuildAxesGeometry();


  float m_bedWidth = 220.0f;
  float m_bedDepth = 220.0f;
  float m_bedOriginX = 0.0f;
  float m_bedOriginY = 0.0f;
  int m_bedShapeType = 0;
  float m_bedDiameter = 220.0f;
  bool m_showBed = true;

  QVariantList m_bedExcludeAreas;
  bool m_heightLimitActive = false;
  float m_heightToRod = 0.0f;
  float m_heightToLid = 0.0f;

  int m_currentPlateIndex = 0;
  int m_plateCount = 1;
  QList<int> m_activeObjectIndices;
  qint64 m_meshGeneration = 0;

  QList<Vertex> m_bedFillVertices;
  QList<Vertex> m_bedLineVertices;
  QList<Vertex> m_bedBottomLineVertices;
  // P15.8 (BOLDGRID): every-5th grid line flat quads (Triangles).
  QList<Vertex> m_bedBoldLineVertices;
  // P15.5 (SINK): white contour bands (Triangles) + per-batch ranges.
  QList<ModelVertex> m_sinkingContourVertices;
  QList<SinkingContourRange> m_sinkingContourRanges;
  // P15.7 (AXES): origin arrows (Triangles, lit pipeline).
  QList<ModelVertex> m_bedAxesVertices;
  QList<ModelVertex> m_bedLimitVertices;
  QList<ModelVertex> m_modelVertices;
  QList<ModelBatch> m_modelBatches;
  ModelBounds m_modelBounds;
  bool m_hasModelBounds = false;
  int m_selectedSourceObjectIndex = -1;
  // P15.11 (MULTICENTER): every selected source object (sorted ascending).
  QList<int> m_selectedSourceObjectIndices;
  int m_hoveredSourceObjectIndex = -1;
  quint32 m_dirtyFlags = DirtyNone;
  // P15.3 (OUTOFBED): print volume + per-instance outside state.
  PrintVolume m_printVolume;
  bool m_anyVolumeOutside = false;
  float m_printableHeight = 0.0f; // 0 = unbounded top clamp
  // PICK-BVH: lazily built ray over the model batches; dropped whenever the
  // mesh geometry is rebuilt (clearModelGeometry). shared_ptr (not
  // unique_ptr) so the implicitly-defined, inline destructor works in every
  // TU with PickingRaycaster incomplete.
  std::shared_ptr<PickingRaycaster> m_pickingRaycaster;
};
