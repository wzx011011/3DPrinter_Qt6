# Wave 2b+ Design: indexed geometry pipeline (restored summary)

The original design doc was lost in the worktree migration. This is the
restored summary; the implementation itself is complete and merged
(commit 399d43f on perf/waves). Implementation details are readable from
the code: ProjectServiceMock::meshData (TLV v2 encoder),
PrepareSceneData::setModelMeshData (v2 decoder + compat soup),
RhiViewportRenderer::uploadModelBuffer (indexed draws).

## TLV v2 wire format (implemented)

```
"OMD2" magic, int32 geometryCount, int32 batchCount
per geometry: [int32 geomId][int32 vertexCount][int32 indexCount]
              [float x3 * vertexCount]  positions, slic3r-local mm
              [uint32 * indexCount]     triangle indices
              [float x3 * vertexCount]  normalized per-vertex normals
per batch:    [int32 renderObjectId][int32 sourceObjectIndex]
              [int32 volumeIndex][int32 instanceIndex][int32 geomId]
              [float x16] row-major GL world matrix
              [int32 volumeType][int32 extruderId][int32 printable]
trailer:      [float x6] scene bbox (GL world)
```

- One geometry per ModelVolume, SHARED by all instances (instance-count
  independent blob size).
- Mirror = det(M3) > 0 (axis swap folded into the matrix); consumers flip
  winding via index swap (renderer) or vertex swap (compat soup).
- Auto-placement offsets fold into the matrix translation column.
- Batch walk order matches collectMeshBatchIdentities (objects ->
  instances -> volumes, hasRenderableTriangle filter) so the parallel
  meshBatch* accessor arrays stay aligned.

## Delivered

- 2b-1: v2 encoder/decoder + compat soup expansion (byte-identical to v1).
- 2c: renderer indexed draws (drawIndexed per range; translucent sorted).
- 3c step 1: preview toolpath index compression (6 verts + 24 idx per
  prism, 672 -> 264 B/move).

## Remaining (spec'd, not implemented)

- Full GPU instancing for preview toolpaths (~76 B/move): unit prism
  vertex/index buffer + per-move instance buffer {endPos, axis, len, width,
  height, color, params} rendered via drawIndexed instanced; requires new
  preview_instanced.vert/frag shaders (qt_add_shaders block pattern exists
  in CMakeLists.txt:422).
- Per-draw matrix UBO for the model path (28 -> 24 B/vert, transform
  changes without rebaking world verts) — additive on top of the current
  indexed layout since the blob keeps local positions + normals.
- 2d three-page sharing: evaluated as NOT WORTH IT (invisible pages never
  render so expansion is already lazy; releasing hidden-page data would add
  ~124 ms stutter per page switch to save ~2 x 12 MB).
