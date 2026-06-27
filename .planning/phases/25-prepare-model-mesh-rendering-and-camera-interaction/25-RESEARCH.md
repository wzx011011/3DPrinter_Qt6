# Phase 25: Prepare Model Mesh Rendering And Camera Interaction - Research

**Researched:** 2026-06-27
**Domain:** Qt QRhi Prepare model mesh rendering, camera interaction, selection/hover bridge
**Confidence:** HIGH

<user_constraints>
## User Constraints

### Locked Decisions
- Use the Qt-native QRhi route for the high-performance renderer: `QQuickRhiItem`, QRhi buffers, and the Phase 23 D3D12-first/D3D11-fallback selector behind `OWZX_RHI_RENDERER=1`.
- Keep default startup stable. Normal launches keep the SoftwareViewport path; legacy OpenGL remains independent.
- Render only active-plate model meshes in Phase 25. Inactive plate objects must not enter the QRhi Prepare scene.
- Reuse existing packed mesh geometry from `EditorViewModel::meshData()`, but add renderer-facing source-object mapping so selection and hover never guess from transient render IDs.
- Reuse `CameraController` for QRhi orbit, pan, zoom, fit, view matrix, and projection matrix behavior.
- Camera movement must update camera/uniform state only. It must not rebuild model vertices or reupload full model buffers.
- Selection source of truth is `EditorViewModel`. QRhi click picking bridges back to editor selection; QRhi reads selection state for highlight rendering.
- QML remains binding and signal forwarding only. Mesh parsing, active-plate filtering, picking, dirty decisions, and renderer upload ownership stay in C++.
- Preserve right-click context-menu routing.
- Verification uses the canonical command only for full verification: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.

### Out-Of-Scope Items
- Preview G-code rendering, segment buffers, color modes, playback, legends, and statistics belong to Phase 26-27.
- Full gizmo drag/transform behavior, paint tools, and complex upstream selection handles are outside Phase 25.
- Vulkan promotion requires a Vulkan-enabled Qt SDK plus benchmark evidence and is not part of Phase 25.
</user_constraints>

## Summary

Phase 25 should extend the Phase 24 `PrepareSceneData` contract from bed/plate context into model mesh batches. The durable boundary is a pure C++ model scene representation with packed mesh parsing, active-plate filtering, batch-to-source-object mapping, object bounds, dirty flags, selected source object state, and hovered source object state. This makes the performance-critical behavior testable without starting a graphics backend. [VERIFIED: `PrepareSceneData`, `ProjectServiceMock`, `EditorViewModel`]

The current packed mesh format is suitable for vertices but not enough for selection identity. `ProjectServiceMock::meshData()` writes pointer-derived render IDs and triangle vertices; those IDs are not `EditorViewModel` visible or source indices. Phase 25 must add an explicit mesh-batch source-index list produced by the same C++ source path that creates packed mesh bytes, then pass that list to QRhi, Software, and GL-compatible viewport property surfaces. [VERIFIED: `ProjectServiceMock::meshData()`, `EditorViewModel::selectObject()`]

QRhi model rendering should use persistent model buffers plus lightweight uniform and overlay updates. Model bytes are uploaded only when mesh, plate, visibility, or GPU resource dirty state changes. Camera changes update a uniform buffer. Selection/hover changes update small overlay state or per-batch draw metadata, not the full model vertex buffer. [VERIFIED: Phase 24 QRhi bed buffer pattern]

**Primary recommendation:** Add source-index-aware model batch parsing to `PrepareSceneData`, expose mesh batch source indices and selected source index from `EditorViewModel`, bind them into `RhiViewport` with fallback parity, then implement QRhi model buffers, `CameraController` synchronization, C++ picking, and selected/hover overlay drawing.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|--------------|----------------|-----------|
| Packed mesh geometry | `ProjectServiceMock` / `EditorViewModel` | `PrepareSceneData` parser | Existing services already own model transforms and libslic3r access. |
| Batch source identity | `ProjectServiceMock` / `EditorViewModel` | Viewport bindings | Selection must use stable source object indices, not renderer IDs. |
| Active-plate model filtering | `PrepareSceneData` | `RhiViewportRenderer` | Filtering belongs in C++; QML only passes source properties. |
| QRhi model buffers | `RhiViewportRenderer` | `PrepareSceneData` snapshot | GPU resources are renderer-owned and dirty-gated. |
| Camera input and state | `RhiViewport` + `CameraController` | `RhiViewportRenderer` uniforms | GUI-thread input owns camera updates; renderer receives copied matrices/state. |
| Picking bridge | `RhiViewport` C++ hit testing | `EditorViewModel` selection API | Emits source-object hits and lets the viewmodel own selection mutation. |
| Visual selection/hover feedback | `RhiViewportRenderer` | `PrepareSceneData` selected/hover state | Feedback uses renderer state while selection truth remains in the viewmodel. |

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| PREP-02 | Active plate STL/OBJ/3MF models render through QRhi with correct transform, scale, orientation, and material color. | `meshData()` already emits transformed GL/world vertices; QRhi needs parser, active filtering, colors, and buffers. |
| PREP-03 | Rotate, pan, zoom, and fit camera operations work in QRhi Prepare without per-frame mesh rebuild. | Existing `CameraController` provides orbit/pan/zoom/fit and matrix calculation. |
| PREP-04 | Selection and hover feedback are visible and synchronized with `EditorViewModel`. | Add source-index selection API/properties and C++ picking/hover. |
| PREP-07 | Prepare behavior has upstream source-truth mapping notes. | Map visible behavior to upstream `GLCanvas3D`, `Camera`, `Selection`, and `PartPlate` concepts. |
</phase_requirements>

## Standard Stack

| Library / Module | Version / Source | Purpose | Why Standard |
|------------------|------------------|---------|--------------|
| Qt Quick `QQuickRhiItem` | Qt 6.10 local SDK | QML-hosted QRhi viewport | Phase 23 established the gated renderer host. |
| QRhi private headers | Qt 6.10 local SDK | Vertex buffers, uniforms, pipelines, resource updates | Existing QRhi bed renderer uses these APIs. |
| Qt Shader Tools | Qt 6.10 local SDK | Compile `.qsb` vertex/fragment shaders | Already wired by Phase 23. |
| `CameraController` | project code | Orbit/pan/zoom/fit and view/projection matrices | Used by existing GL and G-code renderers. |
| Qt Test | project test stack | Pure scene tests, viewmodel tests, QML/static audits | Existing canonical verification runs Qt Test binaries. |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Explicit batch source-index list | Infer from packed render ID | Current render IDs are pointer-derived and cannot safely select editor objects. |
| `PrepareSceneData` parser | Parse packed mesh separately in every renderer | Duplicates filtering and makes dirty behavior harder to test. |
| Camera uniforms | Bake camera transform into vertices | Violates performance goal by tying camera movement to mesh rebuild/upload. |
| C++ picking bridge | QML hit testing | Violates QML boundary and duplicates geometry logic. |

**Installation:** No external packages are installed in this phase.

## Architecture Patterns

### System Architecture Diagram

```text
ProjectServiceMock / PartPlateList
  | packed mesh bytes + mesh batch source indices
  v
EditorViewModel
  | meshData + meshBatchSourceObjectIndices + selectedSourceObjectIndex
  v
PreparePage.qml (bindings and signal forwarding only)
  v
RhiViewport (QQuickRhiItem, camera input, C++ picking)
  v synchronize()
PrepareSceneData (model batches, active filtering, dirty flags, bounds)
  v
RhiViewportRenderer (QRhi model buffers, uniforms, overlay draw)
```

### Pattern 1: Source-Index-Aware Model Batch Contract
**What:** `PrepareSceneData` parses packed mesh bytes together with a same-order source-object index list.
**When to use:** Model rendering, active plate filtering, selected/hovered batch lookup, object bounds, fit view.
**Why:** Render IDs are implementation details; source indices are the editor selection contract.

### Pattern 2: Uniform-Only Camera Updates
**What:** Camera input mutates `CameraController`; renderer uploads view/projection uniforms while model buffers remain resident.
**When to use:** Orbit, pan, wheel zoom, fit view, preset view changes.
**Why:** This is the main Phase 25 performance requirement.

### Pattern 3: Lightweight Selection Overlay
**What:** Draw selected and hovered batches through small overlay state or per-batch draw commands.
**When to use:** Current selection highlight and hover feedback.
**Why:** Selection and hover should be responsive without full mesh reupload.

### Anti-Patterns to Avoid
- QML filtering active plate meshes or computing hit tests.
- Treating pointer-derived packed render IDs as editor object indices.
- Uploading the whole model buffer during orbit/pan/zoom, hover, or selection-only changes.
- Replacing or weakening the default SoftwareViewport startup path.
- Mixing Preview segment rendering into Phase 25.

## Common Pitfalls

### Pitfall 1: Batch Identity Drift
**What goes wrong:** Selection hits the wrong object because mesh batches are mapped by render ID or list position only.
**How to avoid:** Generate a same-order batch source-index list in C++ and test it with synthetic packed mesh data.

### Pitfall 2: Render-Thread Selection Mutation
**What goes wrong:** `QQuickRhiItemRenderer` tries to mutate `EditorViewModel` from the render thread.
**How to avoid:** Keep picking in `RhiViewport` or a GUI-thread C++ helper using a `PrepareSceneData` snapshot, then emit a source-index signal for QML to forward to `EditorViewModel`.

### Pitfall 3: Camera Movement Reuploads Geometry
**What goes wrong:** Orbit/pan/zoom rebuilds CPU vertices or QRhi model buffers.
**How to avoid:** Separate `DirtyCamera`/uniform work from `DirtyMesh`/model-buffer work and add static audit coverage.

### Pitfall 4: Right Click Is Captured By Camera Logic
**What goes wrong:** Existing Prepare context menu stops opening under the QRhi gate.
**How to avoid:** Handle left/middle/wheel camera input and leave right-click routing compatible with `PreparePage.qml`.

### Pitfall 5: Active Plate Filtering Uses Visible List State Only
**What goes wrong:** Hidden or inactive plate objects render because source/visible indices are mixed.
**How to avoid:** Filter QRhi model batches by active source object indices and bridge selection through source-index APIs.

## Source-Truth Mapping

| Upstream Area | Functional Mapping |
|---------------|--------------------|
| `GLCanvas3D` | Prepare viewport interaction flow, picking, hover, selected object feedback |
| `Camera` / `CameraUtils` | Orbit/pan/zoom/fit behavior and view/projection calculations |
| `Selection` | Editor-owned selected object state and user-visible highlight |
| `PartPlate` / `PartPlateList` | Active-plate membership and current-plate isolation |
| `3DBed` | Bed/grid/axis context behind model geometry |

Technical QRhi buffer management is Qt-native and does not need to mirror upstream OpenGL internals.

## Validation Architecture

| Req ID | Behavior | Test Type | Automated Command |
|--------|----------|-----------|-------------------|
| PREP-02 | Packed model bytes parse into source-indexed active-plate batches | unit | `build/PrepareSceneDataTests.exe` |
| PREP-02 | QRhi renderer owns persistent model buffers and draws model vertices | static/build | `build/QmlUiAuditTests.exe`; canonical verify |
| PREP-03 | Camera operations use `CameraController` and uniform updates | static/build | `build/QmlUiAuditTests.exe`; canonical verify |
| PREP-04 | Click selection bridges source object index to `EditorViewModel` | unit/static | `build/ViewModelSmokeTests.exe`; `build/QmlUiAuditTests.exe` |
| PREP-04 | Hover/selection feedback has lightweight overlay path | static/build | `build/QmlUiAuditTests.exe`; canonical verify |
| PREP-07 | Implementation references source-truth areas | static | `rg -n "GLCanvas3D|Camera|Selection|PartPlate|3DBed" src/qml_gui/Renderer src/core/viewmodels` |

### Wave 0 Gaps
- [ ] Source-index-aware packed mesh parser tests.
- [ ] Renderer-facing mesh batch source index API.
- [ ] QRhi model vertex buffer and camera uniform audits.
- [ ] Selection/hover bridge tests and fallback property parity audits.

## Threat Model

| Threat | STRIDE | Standard Mitigation |
|--------|--------|---------------------|
| Malformed packed mesh creates unbounded buffers | Denial of Service | Validate byte lengths, triangle counts, and batch counts before allocating. |
| Inactive plate object is rendered | Information Disclosure | Filter by active source object indices in C++ and test empty/out-of-range plate behavior. |
| Render thread mutates UI state | Elevation of Privilege / Tampering | Selection mutation stays on GUI thread through `EditorViewModel`. |
| Camera updates trigger full geometry upload | Denial of Service | Separate dirty flags and audit upload call sites. |

## Open Questions (RESOLVED)

1. **Can packed render IDs be used as selection IDs?** RESOLVED: No. Add explicit source-object mapping.
2. **Can QML forward a picked source-object signal?** RESOLVED: Yes, if QML only forwards a C++ signal to a viewmodel method and does not filter, parse, or decide.
3. **Should selected/hover feedback clone all upstream handles?** RESOLVED: No. Phase 25 uses object-level tint/outline or overlay feedback.
4. **Should Preview share this renderer work?** RESOLVED: No. Preview has separate segment-buffer requirements in Phase 26.

## Sources

### Primary (HIGH confidence)
- `src/qml_gui/Renderer/PrepareSceneData.h/.cpp` - Phase 24 scene data and dirty flag contract.
- `src/qml_gui/Renderer/RhiViewport.h/.cpp` - QRhi QML item and property surface.
- `src/qml_gui/Renderer/RhiViewportRenderer.h/.cpp` - QRhi bed renderer, persistent buffers, and dirty-gated uploads.
- `src/qml_gui/Renderer/CameraController.h/.cpp` - reusable camera behavior.
- `src/core/services/ProjectServiceMock.h/.cpp` - packed mesh data and active plate object index APIs.
- `src/core/viewmodels/EditorViewModel.h/.cpp` - mesh data, selected object state, active plate state.
- `src/qml_gui/pages/PreparePage.qml` - viewport bindings and context-menu wiring.
- `tests/PrepareSceneDataTests.cpp`, `tests/QmlUiAuditTests.cpp`, `tests/ViewModelSmokeTests.cpp` - available verification surfaces.
- `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.*`, `Camera.*`, `Selection.*`, `PartPlate.*`, `3DBed.*` - source-truth behavior references.

### Secondary (MEDIUM confidence)
- `gsd-sdk query history-digest` - prior Phase 23-24 decisions and handoff notes.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - confirmed in local code and prior phases.
- Architecture: HIGH - extends Phase 24 scene/cache and existing viewmodel boundaries.
- Pitfalls: MEDIUM - render-thread picking and batch identity risks require implementation care.

**Research date:** 2026-06-27
**Valid until:** 2026-07-27 or until Qt SDK/rendering architecture changes.
