---
phase: 66-gizmo-geometry-builders-port
plan: 01
status: complete
requirements_covered: [GGEO-01, GGEO-02, GGEO-03]
files_modified:
  - src/core/rendering/GizmoGeometry.h
  - src/core/rendering/GizmoGeometry.cpp
  - src/core/rendering/GizmoVertex.h
  - src/qml_gui/Renderer/RhiViewportRenderer.h
  - src/qml_gui/Renderer/GLViewportRenderer.cpp
  - tests/GizmoGeometryTests.cpp
  - CMakeLists.txt
---

# Phase 66 Plan 01 - Summary

**Completed:** 2026-07-04
**Status:** Complete — all 16 GizmoGeometryTests slots pass; OWzxSlicer compiles with the GL delegation + per-vertex-color shaders.

## What Shipped

### New: `src/core/rendering/GizmoGeometry.{h,cpp}`
Static utility class with three pure-CPU vertex generators, each returning `QVector<GizmoVertex>` with per-axis RGBA color baked in (zero GL/RHI upload calls):

- `buildMoveGizmoVertices(&offsets)` -> 114 verts (3 axes × 38: 2 shaft + 36 cone). Hardcoded `kMoveVerts[114][3]` array copied verbatim from GLViewportRenderer.cpp:944-1059.
- `buildRotateGizmoVertices(&offsets)` -> 5184 verts (3 rings × 48 segments × 6 sides × 6). Torus `addRing` lambda ported verbatim (majorR=0.7, minorR=0.035).
- `buildScaleGizmoVertices(&offsets)` -> 114 verts (3 axes × 38: 2 shaft + 36 box). `addCube` lambda ported verbatim (shaftEnd=0.7, boxSize=0.08, boxCenter=0.78).

Also exposes:
- `GizmoGeometryOffsets` struct (shaft/cone/box/ring base + count per axis).
- `kAxisColorRGB[3][3]` constexpr — single source of truth for axis colors (X=red 0.90/0.18/0.18, Y=green 0.22/0.80/0.22, Z=blue 0.18/0.40/0.95).

### New: `src/core/rendering/GizmoVertex.h`
Extracted the shared POD vertex layout `{x,y,z,r,g,b,a}` (7 floats, 28 bytes) into its own header so `GizmoGeometry` doesn't pull in the heavy `RhiViewportRenderer.h` (which requires Qt6::Quick / qrhi.h). `RhiViewportRenderer::Vertex` is now a `using = GizmoVertex` alias, so the RHI mesh path consumes the identical layout via a different name.

### Updated: `src/qml_gui/Renderer/GLViewportRenderer.cpp`
- Three `build*Geometry` methods now call `GizmoGeometry::build*GizmoVertices(&offsets)` and upload the returned `QVector<GizmoVertex>` via `glGenBuffers`/`glBufferData` with stride 28 (position 3 floats + color 4 floats). The color attribute is bound at location 1 with offset `3*sizeof(float)`.
- Gizmo shaders (`kGizmoVertSrc`/`kGizmoFragSrc`) updated to read per-vertex color: `layout(location=1) in vec4 aColor` -> `out vec4 vColor` -> `in vec4 vColor` -> `fragColor = vColor`. Removed `uniform vec4 uGizmoColor`.
- Three `render*Gizmo` loops no longer call `setUniformValue(m_uGizmoColor, col)` (color comes from the vertex buffer). The `active` flag is preserved with a `(void)active` + `// Phase 68: highlight via uGizmoHighlight uniform` TODO.
- Removed `kAxisColors[3]` and `kAxisHighlight[3]` static arrays (now in GizmoGeometry.h / deferred to Phase 68).
- Removed `m_uGizmoColor = m_gizmoProg.uniformLocation(...)` lookup.

### Updated: `src/qml_gui/Renderer/RhiViewportRenderer.h`
- Added `#include "core/rendering/GizmoVertex.h"`.
- `struct Vertex` replaced with `using Vertex = GizmoVertex;` (layout-identical alias).

### New: `tests/GizmoGeometryTests.cpp`
Single-file QtTest with 14 slots covering all three builders + color constants:
- Vertex counts: move=114, rotate=5184, scale=114.
- Per-axis colors: every vert in each axis block matches `kAxisColorRGB[axis]` with alpha 1.0.
- Bounding boxes: move max~1.0 (cone tip), rotate within radius 0.735 (majorR+minorR), scale max~0.82 (boxCenter+boxSize/2).
- Move perpendicular-negative diagnostic: confirms each coordinate has >=10 negative-perpendicular verts (cone base radius).
- Offsets: shaft/cone/box/ring base indices match the GL originals exactly.

### Updated: `CMakeLists.txt`
- `GizmoGeometry.{cpp,h}` + `GizmoVertex.h` added to `owzx_app_core` sources.
- `GizmoGeometryTests` target registered (standalone, compiles GizmoGeometry.cpp directly, links Qt6::Test/Core/Gui).

## Verification

### Build path used
Targeted `ninja GizmoGeometryTests` via the Phase 65 vcvars64-sourcing PowerShell wrapper (plain bash lacks MSVC INCLUDE — same finding as Phase 65). QTest output captured with `QT_FORCE_STDERR_LOGGING=1` + `| cat`. OWzxSlicer compile confirmed via the same wrapper (`ninja OWzxSlicer` exited 0 with the GL delegation changes).

### Test result
```
$ ./GizmoGeometryTests.exe
...
Totals: 16 passed, 0 failed, 0 skipped, 0 blacklisted, 206ms
```
(14 test slots + initTestCase + cleanupTestCase = 16.)

### Compile check
`ninja OWzxSlicer` (via vcvars wrapper) reported `exit: 0` — OWzxSlicer.exe (33.7 MB) links the updated GLViewportRenderer.cpp with the per-vertex-color shaders and GizmoGeometry delegation.

## Decisions Resolved (from 66-CONTEXT.md)

| Decision | Choice | Outcome |
|----------|--------|---------|
| Color approach | Per-vertex color (align RHI Vertex) | ✓ RGBA baked into every vert via `kAxisColorRGB` |
| Vertex layout | Reuse RhiViewportRenderer::Vertex | ✓ Now `using = GizmoVertex` (shared POD header) |
| Builder API | New GizmoGeometry class | ✓ `src/core/rendering/GizmoGeometry.{h,cpp}` with 3 static builders |
| GL path | GL delegates to builders | ✓ 3 build*Geometry methods call GizmoGeometry::, shaders consume aColor |

## Issues Hit + Resolutions

1. **RhiViewportRenderer.h transitively requires Qt6::Quick** — the initial `GizmoGeometry.h` included it for the Vertex struct, which forced `GizmoGeometryTests` to link Qt6::Quick (heavy). Fixed by extracting the POD `GizmoVertex` struct into its own lightweight header (`src/core/rendering/GizmoVertex.h`) and making `RhiViewportRenderer::Vertex` a `using = GizmoVertex` alias. Now GizmoGeometry has zero QtQuick dependency.

2. **`testMoveGizmoBoundingBox` initial assertion wrong** — first attempt required global min in `[-0.06, -0.05]` (perpendicular cone radius), but the global min on each coordinate is actually 0.0 (the origin vert is shared by all three axis shafts). Loosened to `[-0.06, 0.01]` and added a separate `testMoveGizmoHasNegativePerpendicularCoords` diagnostic that explicitly counts negative-perpendicular verts (>=10 per coordinate) to confirm the cone radius is real.

3. **OWzxSlicer rebuild via plain bash hits vcvars issue** — `ninja OWzxSlicer` in a non-vcvars shell fails with `C1083: "assert.h"` (MSVC STL missing from INCLUDE). This is the same Phase 65 finding. OWzxSlicer was verified via the vcvars-sourcing wrapper instead.

## Carry-Forward to Next Phases

- **Phase 67** (RHI State Wiring): `RhiViewportRenderer::synchronize` can now read `gizmoMode`/`gizmoCenter` and the gizmo vertex buffers are ready to upload.
- **Phase 68** (Move Gizmo RHI Render): consumes `GizmoGeometry::buildMoveGizmoVertices()` via QRhi buffer upload; reuses the mesh vertex-input layout (location 0 = position, location 1 = color) since GizmoVertex matches the existing RHI Vertex struct.
- **Phase 68** also decides highlight (hover) handling — the `active` flag is preserved in the GL render loops with a TODO; Phase 68 picks between a uniform override or a parallel highlight buffer.
- **Phase 70** (Rotate + Scale): consumes `buildRotateGizmoVertices` / `buildScaleGizmoVertices` via the same upload path.
- **Phase 73** (Retire GL): deletes the GL shader strings and render*Gizmo methods entirely once RHI parity is proven.

## Requirement Traceability

- **GGEO-01** (Geometry generators produce identical vertex counts): ✓ — move=114, rotate=5184, scale=114 (all asserted in GizmoGeometryTests).
- **GGEO-02** (No GL or RHI calls in the geometry layer): ✓ — GizmoGeometry.{h,cpp} contain only CPU vertex generation + GizmoVertex POD include (grep for glGenBuffers/glBufferData/QRhi returns zero).
- **GGEO-03** (Snapshot tests pin the vertex layout): ✓ — 14 GizmoGeometryTests slots cover counts, per-axis colors, bounding ranges, and offset metadata.
