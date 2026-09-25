# Phase 25: Prepare Model Mesh Rendering And Camera Interaction - Context

**Gathered:** 2026-06-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 25 completes the gated QRhi Prepare viewport vertical slice for loaded model mesh rendering, camera interaction, and object selection/hover feedback. It builds on Phase 24's `PrepareSceneData` bed/plate scene cache and must render only the active plate's model meshes through QRhi, preserve the stable Software/OpenGL fallback paths, and map user-visible behavior to OrcaSlicer Prepare viewport concepts. It must not complete Preview G-code rendering, full gizmo manipulation, paint tools, Vulkan promotion, or complex upstream selection handles.

</domain>

<decisions>
## Implementation Decisions

### Phase 25 MVP Boundary
- Render model meshes only for the current active plate. Continue excluding inactive-plate objects from the QRhi Prepare scene.
- Reuse the existing `EditorViewModel::meshData()` packed geometry as the initial mesh source, but add a C++ renderer-facing batch-to-source-object mapping where needed so selection and hover do not guess from transient render IDs.
- Do not implement full gizmo drag/transform behavior in this phase. Limit scope to model rendering, camera orbit/pan/zoom/fit, and selection/hover feedback.
- Keep QRhi behind `OWZX_RHI_RENDERER=1`; normal startup remains the stable SoftwareViewport path and the legacy OpenGL path remains independent.

### Camera And Interaction
- Reuse the existing `CameraController` for QRhi camera state and matrices instead of creating a second QRhi-specific camera implementation.
- Match the current GL/Software Prepare interaction semantics: left drag on empty space orbits, middle drag pans, wheel zooms, and right-click remains the context-menu path.
- Update fit view from mesh bounds when new mesh data arrives or the user requests fit. Ordinary camera movement must update camera state/uniforms only and must not rebuild or reupload model vertices.
- Keep model vertex buffers GPU-resident. Camera interaction updates MVP/uniform state, not baked vertex positions.

### Selection, Hover, And Highlight
- Treat `EditorViewModel` as the selection source of truth. QRhi reads current selection state and renders visible highlight feedback from it.
- For QRhi click selection, use C++ picking against renderer mesh batches and bridge the hit back to the visible/source object index expected by `EditorViewModel::selectObject()`.
- Implement lightweight object/batch-level hover detection first, using CPU ray/AABB or equivalent low-cost hit testing, and show hover tint or outline feedback.
- Use an object-level tint, outline, or wire overlay for selected/hovered objects in the first QRhi pass. Do not attempt to clone all upstream selection shaders, handles, or gizmo visuals in Phase 25.

### Performance, Tests, And Source Truth
- Keep model geometry resident in QRhi buffers and upload only when mesh, plate, or visibility dirty state changes. Selection, hover, and camera changes must not force full mesh upload.
- Use focused tests before implementation: `PrepareSceneData` or parser tests for model batch/dirty behavior, QML/static audits for boundary and fallback safety, QRhi smoke evidence, and then the canonical verification command.
- Map functional behavior to upstream `GLCanvas3D`, `Camera`, `Selection`, and `PartPlate` concepts. QRhi may diverge technically from upstream wx/OpenGL internals when the user-visible behavior remains aligned.
- Explicitly defer Preview rendering, full gizmo manipulation, paint tools, Vulkan promotion, and complex upstream selection handles to later phases or future work.

### the agent's Discretion
Use the smallest durable API additions needed to make renderer batches, source-object mapping, camera state, and picking testable in C++. Prefer extending `PrepareSceneData` and `RhiViewport` over adding QML logic. Keep renderer changes narrowly scoped so Phase 26 can add Preview buffers without inheriting Prepare-only assumptions.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/qml_gui/Renderer/PrepareSceneData.h/.cpp` already owns bed metrics, active plate object indices, mesh generation dirty state, bed fill/grid vertices, and dirty flags (`DirtyBed`, `DirtyPlate`, `DirtyMesh`, `DirtyVisibility`, `DirtyGpu`).
- `src/qml_gui/Renderer/RhiViewport.h/.cpp` is a `QQuickRhiItem` registered as `OWzxGL.GLViewport` only under the QRhi gate and already mirrors the current `GLViewport`/`SoftwareViewport` property surface, including `meshData`, bed metrics, `currentPlateIndex`, `plateCount`, and `activePlateObjectIndices`.
- `src/qml_gui/Renderer/RhiViewportRenderer.h/.cpp` already owns Phase 24 QRhi bed fill/line buffers and dirty-gated uploads. It currently records `meshData` size as generation but does not parse or draw model batches.
- `src/qml_gui/Renderer/CameraController.h/.cpp` is already used by GL and G-code rendering and provides orbit, pan, zoom, fit view, presets, `viewMatrix()`, `projMatrix()`, `eye()`, `target()`, and `distance()`.
- `src/core/services/ProjectServiceMock::meshData()` packs model geometry as `[int32 objectCount]`, then per batch `[int32 objectId][int32 triangleCount][float xyz * triangleCount * 3]`, followed by a bbox trailer. Coordinates are already transformed from slic3r into GL/world coordinates.
- `src/core/viewmodels/EditorViewModel` exposes `meshData`, `activePlateObjectIndices`, `selectedObjectIndex`, `selectedObjectCount`, and `selectObject(int visibleIndex)`. Selection storage uses source object indices internally, while `selectedObjectIndex()` returns visible-list index.
- `src/qml_gui/pages/PreparePage.qml` binds renderer properties into `GLViewport` and keeps QML as wiring. It already forwards fit view requests and keeps right-click context-menu dispatch outside renderer internals.

### Established Patterns
- Rendering backend gates are environment-variable based: default SoftwareViewport, legacy `OWZX_OPENGL`, and QRhi `OWZX_RHI_RENDERER`.
- QML must remain binding-only. Filtering, dirty decisions, mesh parsing, picking, and selection bridging belong in C++.
- Phase 24 proved the scene/cache pattern with pure C++ tests and static UI audits before QRhi rendering changes.
- Source-truth comments should reference upstream concepts/files where behavior is non-obvious, while acknowledging Qt-native QRhi transport differences.

### Integration Points
- Extend `PrepareSceneData` or add a colocated pure helper for packed mesh parsing, active-plate batch filtering, object bounds, dirty flags, and selection/hover metadata.
- Extend `RhiViewport` with minimal selection/hover bridge state if required, keeping matching no-op properties or invokables on fallback viewport types when QML bindings require parity.
- Extend `RhiViewportRenderer` with model vertex/index buffers, camera uniform buffer, model/selection pipelines, and object-range metadata. Keep bed buffers separate from model buffers.
- Add focused coverage to `tests/PrepareSceneDataTests.cpp`, `tests/QmlUiAuditTests.cpp`, and `tests/ViewModelSmokeTests.cpp` where behavior can be verified without a live GPU.

</code_context>

<specifics>
## Specific Ideas

- Add a typed model batch representation with source object index, render batch ID, triangle/vertex range, bbox, base color, selected flag, and hovered flag.
- Keep first QRhi mesh rendering simple and fast: solid material colors plus basic directional shading or vertex color, with optional wire/outline overlay for selected/hovered batches.
- Use active plate object indices as a filter contract and add tests that inactive objects do not enter the QRhi model scene.
- Treat `meshData` object IDs as renderer batch IDs only unless a stable source-object mapping is explicitly added; do not assume the current pointer-derived IDs match `EditorViewModel` object indices.
- Reuse `CameraController::fitView()` from mesh bbox and add evidence that camera motion does not call full model buffer upload paths.

</specifics>

<deferred>
## Deferred Ideas

- Preview G-code QRhi segment buffers, color modes, layer range, playback, and legend/stat synchronization remain Phase 26-27.
- Full gizmo move/rotate/scale drag behavior, transform undo integration from viewport picking, paint tools, and complex selection handles remain future/advanced viewport work unless later phases pull them in.
- Vulkan backend promotion remains future work until a Vulkan-enabled Qt SDK is installed and same-machine benchmark evidence proves it beats D3D12/D3D11.
- Pixel-perfect screenshot testing is deferred unless the QRhi path becomes default or visual regressions cannot be covered by parser/static/smoke tests.

</deferred>
