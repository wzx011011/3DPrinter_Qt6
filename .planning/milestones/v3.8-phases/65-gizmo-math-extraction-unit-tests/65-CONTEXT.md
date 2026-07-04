# Phase 65: Gizmo Math Extraction + Unit Tests - Context

**Gathered:** 2026-07-04
**Status:** Ready for planning

<domain>
## Phase Boundary

Extract the seven pure-math gizmo functions currently embedded as private
methods of `GLViewportRenderer` into a standalone, dependency-free
`GizmoMath` class, and establish the first unit-test coverage for them.

This phase delivers ONLY the math foundation:

- New `src/core/rendering/GizmoMath.{h,cpp}` (header-only library is
  acceptable; see decisions).
- Unit tests in `tests/GizmoMathTests.cpp` registered via the existing
  `qt_add_executable` + `Qt6::Test` + `add_test` pattern.
- `GLViewportRenderer` updated to delegate to `GizmoMath::` for the seven
  named functions, preserving bit-identical behavior.

Out of scope (deferred to later v3.8 phases):

- Geometry builders (move arrows / rotate torus / scale shaft+box) -> Phase 66.
- RHI state wiring -> Phase 67.
- RHI rendering of any gizmo -> Phase 68+.
- `pickObject` / `rayAABB` / ray-triangle Möller-Trumbore helpers -> Phase 72
  (precise picking). The seven named functions stay scoped to gizmo pick/drag
  math, not object pick math.

The seven functions in scope (per ROADMAP deliverables GMATH-01/02/03):

1. `computeRay` — screen-space (sx, sy) -> world-space (origin, direction).
2. `rayToAxisT` — closest parametric t along an axis line from a screen ray.
3. `computeRotateAngle` — projected angle for rotate gizmo drag.
4. `pickMoveAxis` — which of X/Y/Z move arrow the cursor hit (0/1/2/3).
5. `pickRotateAxis` — which of X/Y/Z rotate ring the cursor hit.
6. `pickScaleAxis` — which of X/Y/Z scale shaft the cursor hit (+ uniform).
7. `rayXZIntersect` — world point where the screen ray crosses the Y=0 plane.

</domain>

<decisions>
## Implementation Decisions

### API Shape

- **Static class `GizmoMath`** with static methods
  (`GizmoMath::computeRay(...)`, `GizmoMath::pickMoveAxis(...)`, etc.).
- No instance state; no member variables; every input passed as a parameter.
- Matches ROADMAP deliverable wording ("class `GizmoMath` (or namespace)") and
  Qt conventions (QMathHelper-style utility class).
- Lives under `src/core/rendering/` alongside `SupportPaintTypes.h`,
  `GLShaderUtil`, etc. — pure-core, no QML/Quick dependency.

### State Parameterization

- **Full parameterization.** Every function takes ALL its inputs as explicit
  parameters — no hidden coupling to renderer member variables.
  - `computeRay(sx, sy, viewSize, projMatrix, viewMatrix)` — replaces reads of
    `m_viewSize`, `m_camera.projMatrix()`, `m_camera.viewMatrix()`.
  - `pickMoveAxis(sx, sy, viewSize, projMatrix, viewMatrix, gizmoCenter,
    cameraEye)` — replaces reads of `m_gizmoCenter`, `m_camera.eye()`.
  - `pickRotateAxis(... same + ...)` and `pickScaleAxis(... same + ...)`.
  - `rayToAxisT(orig, dir, axisDir, gizmoCenter)` — already mostly pure; just
    promote `m_gizmoCenter` to a parameter.
  - `computeRotateAngle(sx, sy, axis, viewSize, projMatrix, viewMatrix,
    gizmoCenter, rotateStartAngle)` — `rotateStartAngle` is the fallback for
    the parallel-plane branch.
  - `rayXZIntersect(sx, sy, viewSize, projMatrix, viewMatrix)`.
- Parameter order convention: `(screen coords, viewport, camera matrices,
  gizmo state, ...rest)`. Consistent across all functions so call sites read
  predictably.

### GL Equivalence Guarantee

- **Strict bit-equivalence + snapshot tests.** GLViewportRenderer must produce
  bit-identical results before and after extraction.
- Verified by:
  1. Unit tests on the new `GizmoMath` functions with hand-checked inputs and
     expected outputs (computed independently, not by copying GL code).
  2. Snapshot/regression tests asserting that for a fixed set of inputs
     (deterministic camera, gizmo center, sample cursor positions), the GL
     renderer's pre-extraction and post-extraction results match exactly.
  3. Build + ctest green; no behavior drift in the GL path.
- Any divergence between pre/post extraction is a Phase 65 failure that must
  be resolved before planning Phase 66.

### Coverage Scope

- **ROADMAP-listed seven functions only** (computeRay, rayToAxisT,
  computeRotateAngle, pickMoveAxis, pickRotateAxis, pickScaleAxis,
  rayXZIntersect).
- `pickObject`, `rayAABB`, and the Möller-Trumbore ray-triangle helpers are
  explicitly deferred to Phase 72 (Precise Object Picking).
- `transformedAABB`, `computeModelMatrix`, `rayAABB` stay where they are —
  they couple to mesh-batch state and aren't on the gizmo pick/drag path.

### Claude's Discretion

- Choice of header-only (`GizmoMath.h` with inline functions) vs.
  header+source pair. Either is fine; pick whichever matches existing
  `src/core/rendering/` conventions most closely (likely header+cpp to mirror
  `GLShaderUtil`).
- Exact test fixture values (camera matrices, gizmo positions, cursor
  coordinates) — Claude picks deterministic, easy-to-reason-about inputs.
- Whether to extract helper sub-functions (e.g., a private
  `closestPointsBetweenRays` used by both `pickMoveAxis` and `rayToAxisT`) —
  allowed if it improves clarity without changing output bits.
- Exact names for the new test target (`GizmoMathTests` recommended for
  consistency with `ViewModelSmokeTests`, `PreviewParserTests`, etc.).

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets

- `src/qml_gui/Renderer/GLViewportRenderer.{h,cpp}` — current location of the
  seven functions as private methods. Lines:
  - `computeRay`: cpp:1250-1266
  - `rayXZIntersect`: cpp:1271-1278
  - `rayToAxisT`: cpp:1346-1359
  - `pickMoveAxis`: cpp:1364-1418
  - `computeRotateAngle`: cpp:1491-1517
  - `pickRotateAxis`: cpp:1630+ (declared at h:61)
  - `pickScaleAxis`: cpp:1899+
  - All seven declared in `GLViewportRenderer.h:57-65`.
- `src/core/rendering/` — target directory; existing peers:
  `SupportPaintTypes.h`, `GLShaderUtil.{h,cpp}`, etc.
- `CameraController` (referenced as `m_camera`) — provides `projMatrix(aspect)`,
  `viewMatrix()`, `eye()`. Used to derive the parameters that get passed to
  the extracted functions.

### Established Patterns

- **Test target wiring** (CMakeLists.txt lines 359-462):
  `qt_add_executable(<Name> tests/<Name>.cpp)` ->
  `target_link_libraries(<Name> PRIVATE [owzx_app_core] Qt6::Test [Qt6::Core])`
  -> `add_test(NAME <Name> COMMAND <Name>)`.
  Existing peers: `E2EWorkflowTests`, `ViewModelSmokeTests`, `QmlUiAuditTests`,
  `InventoryAuditTests`, `PrepareSceneDataTests`, `PartPlateTests`,
  `PreviewParserTests`.
- **AUTOMOC caveat** (documented in `tests/ViewModelSmokeTests.cpp` lines
  1-10): single-file QtTest with cpp-internal `Q_OBJECT` has weak moc tracking.
  If `GizmoMathTests.cpp` uses private slots, the canonical verify script's
  cmake reconfigure step handles it; otherwise no special handling needed.
- **No clang-format / editorconfig** — match surrounding 2-space indent,
  K&R/Allman hybrid braces, camelCase functions.
- Build command (per AGENTS.md + STATE.md verification rule): do NOT run the
  full canonical build (`scripts/auto_verify_with_vcvars.ps1`) until Phase 73.
  During Phase 65 use source reads, encoding guard, `git diff --check`, and
  focused static checks only. A targeted test build (`cmake --build build
  --target GizmoMathTests` then `build\GizmoMathTests.exe`) is acceptable for
  verifying the new tests compile and pass.

### Integration Points

- `GLViewportRenderer.cpp` lines 304, 306, 308, 323, 325, 330, 337, 348, 388,
  389, 397, 411, 412 — call sites that must be updated to delegate to
  `GizmoMath::` (passing the now-explicit parameters).
- `CMakeLists.txt` — new test target must be added under the existing test
  block (around lines 359-462). The new `GizmoMath.{h,cpp}` source files must
  be added to `owzx_app_core` (or whichever target GLViewportRenderer links
  against) so both the renderer and the test executable can see them.
- `tests/CMakeLists.txt` does not exist — test wiring is in the root
  `CMakeLists.txt`.

</code_context>

<specifics>
## Specific Ideas

- Parameter-order convention (screen -> viewport -> camera -> gizmo -> rest)
  is a user preference to keep call sites predictable across the seven
  functions; document it in the `GizmoMath.h` header comment.
- The new test target should be named `GizmoMathTests` (consistent with the
  existing `<Topic>Tests` naming).
- The seven functions are the gizmo pick/drag math, NOT the object pick math
  — keep that boundary crisp so Phase 72 (precise picking) can extract its own
  helpers without untangling from gizmo code.

</specifics>

<deferred>
## Deferred Ideas

- `pickObject` / `rayAABB` / ray-triangle (Möller-Trumbore) extraction ->
  Phase 72 (Precise Object Picking).
- Geometry builders (move arrows / rotate torus / scale shaft+box) ->
  Phase 66.
- RHI gizmo state wiring (synchronize reading gizmoMode/cutAxis/cutPosition)
  -> Phase 67.
- Any actual RHI rendering of gizmos -> Phase 68+.
- A fuzz/property-based test harness for the math (e.g., verify
  `pickMoveAxis` returns a value in {0,1,2,3} for any screen coordinate) —
  nice-to-have but not required by ROADMAP; defer unless trivial to add on
  top of the deterministic unit tests.

</deferred>
