#pragma once

#include <QByteArray>
#include <QList>
#include <QVariant>
#include <QVector4D>
#include <QtGlobal>

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

  enum DirtyFlag : quint32
  {
    DirtyNone = 0,
    DirtyBed = 1u << 0,
    DirtyPlate = 1u << 1,
    DirtyMesh = 1u << 2,
    DirtyVisibility = 1u << 3,
    DirtyGpu = 1u << 4,
    DirtySelection = 1u << 5,
    DirtyCamera = 1u << 6
  };

  PrepareSceneData();

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

  quint32 peekDirtyFlags() const;
  quint32 takeDirtyFlags();
  void clearDirtyFlags();

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
  int selectedSourceObjectIndex() const;
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

  void markDirty(quint32 flags);
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
  int m_hoveredSourceObjectIndex = -1;
  quint32 m_dirtyFlags = DirtyNone;
  // P15.3 (OUTOFBED): print volume + per-instance outside state.
  PrintVolume m_printVolume;
  bool m_anyVolumeOutside = false;
  float m_printableHeight = 0.0f; // 0 = unbounded top clamp
};
