# Phase 66: Gizmo Geometry Builders Port - Context

**Gathered:** 2026-07-04
**Status:** Ready for planning

<domain>
## Phase Boundary

Port the three gizmo geometry construction functions (move arrows, rotate
torus rings, scale shafts + box handles) from `GLViewportRenderer` into a
standalone, pure-CPU vertex generator class, decoupled from any GL or RHI
calls. Establish snapshot tests pinning the vertex counts and layouts so
Phase 67+ can consume the geometry from the RHI path without re-deriving it.

This phase delivers ONLY the geometry generation + snapshot tests:

- New `src/core/rendering/GizmoGeometry.{h,cpp}` with three static builders
  returning `QVector<RhiViewportRenderer::Vertex>` (position + per-vertex
  RGBA color).
- Snapshot tests in `tests/GizmoGeometryTests.cpp` asserting vertex counts
  and bounding ranges match the GL originals.
- `GLViewportRenderer`'s three `build*Geometry` methods updated to call the
  extracted builders, then upload via `glGenBuffers`/`glBufferData` (the
  GL-specific tail stays in the renderer).

Out of scope (deferred to later v3.8 phases):

- RHI gizmo state wiring (synchronize reading gizmoMode/gizmoCenter) ->
  Phase 67.
- RHI rendering of any gizmo ( pipelines, shaders, actual draw calls) ->
  Phase 68+.
- Highlight/hover color handling beyond baking the base color — Phase 68
  decides whether hover uses a uniform override or a parallel buffer.
- Cut plane geometry and wipe tower geometry -> Phase 71.

The three builders in scope (per ROADMAP deliverables GGEO-01/02/03):

1. `buildMoveGizmoVertices()` — 114 hardcoded vertices (3 axes x 38 each):
   shaft line segments + cone triangle fan. Per-axis color baked in.
2. `buildRotateGizmoVertices()` — 3 torus rings (segments=48, sides=6),
   1728 vertices per ring, 5184 total. Per-axis color baked in.
3. `buildScaleGizmoVertices()` — 3 shafts (2 verts each) + 3 cube handles
   (36 verts each), 114 total. Per-axis color baked in.

Each builder also returns offset metadata (shaft/cone/box base indices per
axis) so the caller can issue per-axis draw calls without recomputing offsets.

</domain>

<decisions>
## Implementation Decisions

### Color Approach - Per-Vertex Color (align with RHI Vertex)

- **Bake per-axis color into every vertex as RGBA.** Axis colors match the
  GL `kAxisColors[3]` constants:
  - X (red):    `QVector4D(0.90f, 0.18f, 0.18f, 1.f)`
  - Y (green):  `QVector4D(0.22f, 0.80f, 0.22f, 1.f)`
  - Z (blue):   `QVector4D(0.18f, 0.40f, 0.95f, 1.f)`
- The constants move from `GLViewportRenderer.cpp` (anonymous `static`)
  into `GizmoGeometry.h` as `constexpr` (or `static const`) so both the
  builders and any future highlight logic share one source of truth.
- **Highlight (hover) handling is deferred to Phase 68.** This phase bakes
  only the base color. Phase 68 will decide between (a) a uniform override
  that multiplies/replaces the vertex color in the gizmo shader, or (b) a
  parallel highlight vertex buffer. Either approach is compatible with the
  per-vertex-color base produced here.

### Vertex Layout - Reuse RhiViewportRenderer::Vertex

- Builders return `QVector<RhiViewportRenderer::Vertex>` (the struct defined
  at `RhiViewportRenderer.h:20-29`: `{float x, y, z, r, g, b, a}` — 7 floats,
  28 bytes).
- This is the SAME struct the RHI mesh path already uses, so Phase 68 can
  reuse the existing mesh vertex-input description and pipeline topology
  without a gizmo-specific layout.
- Implication: `GizmoGeometry.h` must `#include "qml_gui/Renderer/RhiViewportRenderer.h"`
  to see the Vertex struct. This is a one-way dependency (core/rendering ->
  qml_gui/Renderer), which is acceptable since the Vertex struct is a plain
  POD data type (no Qt/RHI/OpenGL symbols instantiated at include time).
  - Alternative considered: define a local `GizmoVertex` POD with identical
    layout and `static_assert(sizeof(GizmoVertex) == sizeof(RhiViewportRenderer::Vertex))`.
    REJECTED because it duplicates the struct and risks drift if either side
    changes; the direct reuse is simpler and the include cost is trivial.

### Builder API - New GizmoGeometry Class

- New class `GizmoGeometry` at `src/core/rendering/GizmoGeometry.{h,cpp}`,
  peer to `GizmoMath` (Phase 65) and `GLShaderUtil`.
- Three static methods + offset-metadata struct:

  ```cpp
  struct GizmoGeometryOffsets
  {
    // Per-axis vertex offsets (in vertex count, NOT bytes) for draw calls.
    // Index 0=X, 1=Y, 2=Z.
    int shaftBase[3] = {0, 0, 0};   // move + scale shaft line start
    int coneBase[3]   = {0, 0, 0};  // move cone triangle start
    int boxBase[3]    = {0, 0, 0};  // scale box handle start
    int ringBase[3]   = {0, 0, 0};  // rotate ring start
    int shaftVertCount = 0;
    int coneVertCount  = 0;
    int boxVertCount   = 0;
    int ringVertCount  = 0;
  };

  class GizmoGeometry
  {
  public:
    static QVector<RhiViewportRenderer::Vertex> buildMoveGizmoVertices(
        GizmoGeometryOffsets *out = nullptr);
    static QVector<RhiViewportRenderer::Vertex> buildRotateGizmoVertices(
        GizmoGeometryOffsets *out = nullptr);
    static QVector<RhiViewportRenderer::Vertex> buildScaleGizmoVertices(
        GizmoGeometryOffsets *out = nullptr);

    // Axis color constants (single source of truth; X=0, Y=1, Z=2).
    static constexpr float kAxisColorRGB[3][3] = {
        {0.90f, 0.18f, 0.18f}, // X red
        {0.22f, 0.80f, 0.22f}, // Y green
        {0.18f, 0.40f, 0.95f}, // Z blue
    };

  private:
    GizmoGeometry() = delete;
  };
  ```

- Callers that don't need offsets pass `nullptr` (default); callers that need
  to issue per-axis draws pass a pointer to receive the offset struct.
- `GizmoGeometry() = delete` mirrors Phase 65's GizmoMath (static-only utility).

### GL Path - Delegates to Builders (match Phase 65)

- `GLViewportRenderer`'s three `build*Geometry` methods are rewritten to:
  1. Call the corresponding `GizmoGeometry::build*GizmoVertices(&offsets)`.
  2. Cache the returned offsets into the renderer's existing
     `m_moveShaftBase[3]` / `m_moveConeBase[3]` / `m_rotateRingBase[3]` /
     `m_rotateRingVertCount` / `m_scaleShaftBase[3]` / `m_scaleBoxBase[3]` /
     `m_scaleBoxVertCount` members (these stay in the renderer — they're
     used by the GL render*Gizmo methods).
  3. Upload the vertex buffer via the existing `glGenBuffers`/`glBufferData`/
     `glVertexAttribPointer` sequence.
- The GL vertex-input description must be updated from position-only
  (3 floats, stride 12) to position+color (7 floats, stride 28). This
  requires adding a second `glVertexAttribPointer` call for the color
  attribute (location 1) and updating the gizmo vertex/fragment shaders
  to read color from the vertex input instead of the `uGizmoColor` uniform.
  - Wait — this is a behavior change to the GL render path. Re-evaluate:
    the GL shaders currently use `uniform vec4 uGizmoColor` set per-axis.
    If the vertex buffer now carries color, the shader must read it as
    `in vec4 aColor` (location 1) and the per-axis uniform updates in
    renderMoveGizmo/renderRotateGizmo/renderScaleGizmo (lines 864-865,
    1531-1532, 1651-1652) become dead code.
  - Decision: **update the GL shaders + render loops to consume per-vertex
    color too**, removing the `uGizmoColor` uniform entirely. This keeps
    GL and RHI on the same vertex format and avoids maintaining two
    shading paths. The highlight (hover) uniform is preserved as
    `uGizmoHighlight` (a single vec4 + int active-axis) so hover still
    works without rebuilding geometry — Phase 68 carries this forward.
  - This is a larger GL change than Phase 65's pure delegation, but it's
    necessary for the vertex-format equivalence to hold. The GL path is
    being retired in Phase 73 anyway; spending a small amount of effort
    here to keep it functional through 67-72 is worthwhile.

### Claude's Discretion

- Exact placement of the offset struct (in GizmoGeometry.h vs a separate
  header) — keep it in GizmoGeometry.h for simplicity.
- Whether to keep `kAxisHighlight[3]` constants in GLViewportRenderer.cpp
  (current location) or move them to GizmoGeometry.h alongside `kAxisColorRGB`.
  Recommendation: move them — single source of truth for all gizmo colors.
- Whether the GL shader source strings (kGizmoVertSrc / kGizmoFragSrc at
  GLViewportRenderer.cpp:70-76) get edited in place or extracted. Edit in
  place — they're GL-specific and Phase 73 deletes them.
- Exact snapshot-test fixture values (expected vertex counts, bounding-box
  mins/maxes) — derived from the GL originals by reading the source.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets

- `src/core/rendering/GizmoMath.{h,cpp}` (Phase 65) — established pattern
  for a static-only utility class in `src/core/rendering/`. Match its
  include style, brace style, `GizmoGeometry() = delete` idiom.
- `src/qml_gui/Renderer/RhiViewportRenderer.h:20-29` — `Vertex` struct
  `{x, y, z, r, g, b, a}` that the builders will return.
- `src/qml_gui/Renderer/GLViewportRenderer.cpp` lines:
  - 940-1077 — `buildGizmoGeometry` (move arrows): hardcoded 114-vertex
    `kGizmoVerts[][3]` array + `m_moveShaftBase[3]`/`m_moveConeBase[3]`
    offset tracking + `glGenBuffers` upload.
  - 1434-1506 — `buildRotateGizmoGeometry` (torus rings): nested theta/phi
    loops with segments=48, sides=6, majorR=0.7, minorR=0.035; per-ring
    `addRing` lambda; `m_rotateRingBase[3]`/`m_rotateRingVertCount`.
  - 1553-1626 — `buildScaleGizmoGeometry` (shafts + boxes): `addCube`
    lambda generating 36-vert cubes; shaftEnd=0.7, boxSize=0.08,
    boxCenter=0.78; `m_scaleShaftBase[3]`/`m_scaleBoxBase[3]`/
    `m_scaleBoxVertCount`.
  - 127-137 — `kAxisColors[3]` + `kAxisHighlight[3]` constants (RGBA).
  - 70-76 — `kGizmoVertSrc`/`kGizmoFragSrc` shader source strings
    (uniform `uGizmoColor`, no vertex color input).
  - 842-894 — `renderMoveGizmo`: per-axis loop, sets `uGizmoColor` uniform
    (line 865) based on `active` flag, draws shaft (GL_LINES) + cone
    (GL_TRIANGLES) per axis.
  - 1511-1552 — `renderRotateGizmo`: similar per-axis loop.
  - 1631-1670 — `renderScaleGizmo`: similar per-axis loop.

### Established Patterns

- **Static utility class pattern** (Phase 65 GizmoMath): `class X { public:
  static ...; private: X() = delete; };` in `src/core/rendering/X.{h,cpp}`.
- **Test target wiring** (CMakeLists.txt, established in Phase 65): single-
  file QtTest + `qt_add_executable` + `target_link_libraries PRIVATE Qt6::Test
  Qt6::Core Qt6::Gui` + `target_include_directories PRIVATE src` + compile
  the source-under-test directly into the test target. GizmoGeometryTests
  will compile `GizmoGeometry.cpp` directly (no owzx_app_core link) since
  GizmoGeometry has zero render deps beyond the POD Vertex struct include.
- **`#include "X.moc"` footer** for single-file QtTest (Phase 65 pattern).
- **`msg()` helper for QVERIFY2 descriptions** using `QString::asprintf`
  (Phase 65 GizmoMathTests pattern, since Qt 6 removed `QString::arg(float)`).
- **AUTOMOC caveat** for single-file QtTest (documented in
  tests/ViewModelSmokeTests.cpp:1-10 and tests/GizmoMathTests.cpp:1-10).
- **Build env**: targeted test builds need the vcvars64-sourcing PowerShell
  wrapper pattern from Phase 65 (plain bash lacks MSVC INCLUDE). QTest output
  needs `QT_FORCE_STDERR_LOGGING=1` + `| cat` to be visible in bash pipes.

### Integration Points

- `CMakeLists.txt` around line 105 — add `GizmoGeometry.{cpp,h}` to
  `owzx_app_core` sources (right after `GizmoMath.{cpp,h}` added in Phase 65).
- `CMakeLists.txt` after the `GizmoMathTests` block (Phase 65) — register
  `GizmoGeometryTests` target mirroring `GizmoMathTests`.
- `GLViewportRenderer.cpp` lines 940-1077, 1434-1506, 1553-1626 — replace
  inline vertex generation with builder calls; keep the GL upload tail.
- `GLViewportRenderer.cpp` lines 70-76 — update gizmo shader source to read
  per-vertex color (add `layout(location=1) in vec4 aColor;` /
  `out vec4 fragColor; void main(){ fragColor = aColor; }`), remove
  `uniform vec4 uGizmoColor` (replaced by per-vertex color), keep a uniform
  for highlight override if Phase 68 needs it (defer the exact uniform name).
- `GLViewportRenderer.cpp` lines 864-865, 1531-1532, 1651-1652 — remove the
  per-axis `setUniformValue(m_uGizmoColor, col)` calls (color now comes from
  the vertex buffer). Keep the `active` flag computation for Phase 68's
  highlight work.
- `GLViewportRenderer.cpp` line 904 (`m_uGizmoColor = m_gizmoProg.uniformLocation("uGizmoColor")`)
  — remove (uniform gone). May need a new `m_uGizmoHighlightAxis`/
  `m_uGizmoHighlightColor` location lookup if highlight is uniform-driven
  in Phase 68; for Phase 66 just remove the old uniform.

</code_context>

<specifics>
## Specific Ideas

- The move gizmo's 114 vertices are a hardcoded static array — port it as a
  `static const float kMoveVerts[114][3]` inside GizmoGeometry.cpp (NOT
  recomputed at runtime; the GL original is also static). The per-axis color
  is applied in a second pass when expanding into `QVector<Vertex>`.
- The rotate gizmo's torus rings ARE runtime-generated (theta/phi loops) —
  port the `addRing` lambda verbatim into GizmoGeometry.cpp, expanding each
  output vertex into the `Vertex` struct with the ring's axis color.
- The scale gizmo's `addCube` lambda is also runtime-generated — port verbatim.
- Snapshot tests assert EXACT vertex counts (114 / 5184 / 114) and bounding
  ranges (e.g., move gizmo vertices span [0,1] on the active axis, [-0.055,
  0.055] on the perpendicular axes). These pin the layout so Phase 67-68 can
  rely on the counts.
- The `kAxisColorRGB` constexpr in GizmoGeometry.h is the single source of
  truth; GLViewportRenderer.cpp's `kAxisColors` and `kAxisHighlight` should
  reference it (or be removed in favor of the constexpr).

</specifics>

<deferred>
## Deferred Ideas

- Highlight/hover color rendering (uniform override vs parallel buffer) ->
  Phase 68.
- Cut plane geometry (translucent quad + outline) -> Phase 71.
- Wipe tower geometry (translucent box) -> Phase 71.
- GCodeRenderer or other gizmo-adjacent geometry -> not part of v3.8.
- Parametric gizmo geometry (configurable shaft length, cone size, ring
  radius) — the GL originals use fixed constants; keep them fixed for
  parity. If a future phase needs parametric sizing, add overloads then.
- Moving `kAxisHighlight[3]` constants fully into GizmoGeometry.h — recommended
  but if it complicates the GL shader edit, leave them in GLViewportRenderer.cpp
  for now and reconcile in Phase 68.

</deferred>
