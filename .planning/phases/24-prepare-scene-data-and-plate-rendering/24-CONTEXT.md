# Phase 24: Prepare Scene Data And Plate Rendering - Context

**Gathered:** 2026-06-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 24 turns the gated QRhi viewport from a diagnostic renderer into a Prepare scene renderer for bed/plate context. It must build the CPU scene/cache layer, own QRhi GPU buffers, render the active bed/plate with grid and origin cues, and update plate context when the active plate changes. It must not complete full model mesh rendering, selection/hover, gizmos, camera interaction, or Preview G-code rendering; those remain Phase 25-27 scope.

</domain>

<decisions>
## Implementation Decisions

### Performance-First Renderer Path
- Keep the technical route Qt-native: `QQuickRhiItem` + QRhi resources, using the Phase 23 D3D12-first/D3D11-fallback selector behind `OWZX_RHI_RENDERER=1`.
- Do not attempt Vulkan in Phase 24. The Phase 23 benchmark proved the installed QtGui lacks public Vulkan support, so Vulkan is not a viable app backend until the Qt SDK changes.
- Optimize for steady-frame cost: create/update QRhi buffers only when explicit scene dirty flags change, not every frame.
- Keep default startup stable. Normal launches still use the stable SoftwareViewport path unless the QRhi gate is explicitly enabled.

### Scene Data And Dirty Tracking
- Add a C++ scene/cache layer for Prepare data, colocated with the QRhi renderer boundary, that stores bed dimensions, grid/axis vertices, active plate metadata, and future mesh/cache hooks.
- Track dirty categories explicitly: bed geometry, plate context, mesh data, visibility flags, and GPU resource state. Phase 24 acceptance depends on proving only dirty ranges/resources are uploaded.
- Treat QML as binding-only. QML may pass existing viewmodel properties to the viewport, but scene conversion, dirty decisions, and GPU ownership stay in C++.
- If the current packed `meshData` format does not encode per-plate object membership strongly enough for GPU filtering, Phase 24 should add a typed C++ bridge rather than pushing filtering logic into QML.

### QRhi Bed And Plate Rendering
- Render active bed/plate context through QRhi: bed surface or outline, grid lines, border, origin cues/axes, and visual state tied to the selected plate.
- Start from existing Qt6/OpenGL functional expectations where useful: 220x220 fallback bed, 10 mm fine grid, 50 mm coarse grid, origin/axis cues, and a ground plane slightly below model height.
- Prefer simple GPU-friendly geometry and solid colors first. Textures, logos, icons, height-limit volumes, exclude areas, and calibration graphics can be deferred unless they are needed to satisfy active bed/plate context.
- Bed dimensions should come from existing `EditorViewModel` bed shape properties where available. If dynamic upstream printable-area parity is unavailable, document the gap and keep the renderer data model ready for later dynamic shape input.

### Plate Switching And Isolation
- Use `EditorViewModel::currentPlateIndex`, `plateCount`, and `ProjectServiceMock`/`PartPlateList` as the C++ source of truth for plate state.
- Active-plate switching must mark scene data dirty and update rendered plate context without leaking inactive-plate object data into the active scene.
- Phase 24 may limit visible object rendering to counts/context if full mesh drawing is deferred, but the CPU scene/cache API must preserve the active-plate isolation contract for Phase 25 mesh rendering.
- Existing QML bottom plate bar and thumbnail flow remain the UI interaction surface; Phase 24 should not redesign plate management UI.

### Source-Truth Mapping
- Upstream functional references: `3DBed` for build volume, ground plane, axes, shape/grid rendering; `PartPlate`/`PartPlateList` for plate origin, plate rendering, object membership, selected/current plate behavior; `GLCanvas3D`/`Plater` for active plate selection and update triggers.
- Technical implementation intentionally diverges from upstream wx/OpenGL/ImGui internals. The parity requirement is functional: the active bed/plate context and plate switching behavior should match the user-visible workflow.
- Keep inline source-truth comments concise and point to upstream concepts/files where the behavior is non-obvious.

### Testing And Verification
- Use TDD for implementation: create focused failing tests/audits before source changes.
- Static/QML audit should guard that Prepare binds active plate/bed properties to the QRhi-compatible viewport and does not compute scene logic in QML.
- C++ tests should cover scene dirty flags, bed/grid vertex generation, active-plate changes, and inactive-object isolation where the data bridge supports it.
- Verification must use the canonical command only: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.

### the agent's Discretion
Keep the first implementation vertically thin but architecturally real: a reusable scene/cache class, QRhi buffer upload ownership, and a visible QRhi bed/plate result are more valuable than broad but temporary rendering code. Defer polish that does not affect performance, correctness, or Phase 25 extensibility.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/qml_gui/Renderer/RhiViewport.h/.cpp` already exposes a `GLViewport`-compatible QML surface via `QQuickRhiItem`; `RhiViewportRenderer.cpp` currently draws only a diagnostic triangle.
- `src/qml_gui/Renderer/GLViewportRenderer.cpp` contains established bed/grid visuals: 220 mm fallback bed, 10 mm and 50 mm grid intervals, axis/origin cues, and procedural bed coloring.
- `src/core/viewmodels/EditorViewModel.h/.cpp` exposes `plateCount`, `currentPlateIndex`, `meshData`, plate operations, and bed shape properties (`bedWidth`, `bedDepth`, `bedMaxHeight`, origin and shape type).
- `src/core/services/ProjectServiceMock.h/.cpp` backs plate state with `PartPlateList`, exposes `currentPlateObjectIndices()`, `plateObjectIndices()`, `plateObjectCount()`, and packs current mesh data for the viewport.
- `src/core/model/PartPlate.h/.cpp` and `PartPlateList.h/.cpp` are already Qt6-native source-truth domain objects for plate membership and current plate selection.

### Established Patterns
- Renderer backend gates are environment-variable based: default stable viewport, legacy `OWZX_OPENGL`, and QRhi `OWZX_RHI_RENDERER`.
- `PreparePage.qml` instantiates `GLViewport` and binds `meshData` from `EditorViewModel`; the QML type may resolve to `RhiViewport` under the QRhi gate because Phase 23 registers it under the same `OWzxGL.GLViewport` name.
- Existing plate UI already uses `editorVm.setCurrentPlateIndex(index)`, `currentPlateIndex`, `plateName()`, `plateObjectCount()`, and thumbnail capture callbacks.
- `QmlUiAuditTests` already guard renderer registration/gates and are a low-cost place to add static boundary checks.

### Integration Points
- Extend `RhiViewport` with minimal bed/plate properties if existing `meshData` is insufficient, preserving QML compatibility with `GLViewport`.
- Extend `RhiViewportRenderer` from one diagnostic vertex buffer to separate GPU buffers/pipelines for bed fill, grid/axes lines, and eventually active mesh batches.
- Add a small C++ scene/cache helper under `src/qml_gui/Renderer/` or `src/core/rendering/` depending on ownership. If it depends on QRhi resource types, keep it in the renderer layer; if it is pure scene data/dirty logic, keep it testable without QRhi.
- Keep `SoftwareViewport` and legacy `GLViewport` behavior unchanged unless a compatibility property is required for QML binding.

</code_context>

<specifics>
## Specific Ideas

- A pure `PrepareSceneData`/`RhiPrepareScene` class can expose `setBed(width, depth, origin, shape)`, `setPlateContext(index, count, objectIndices)`, `setMeshBytes(...)`, `dirtyFlags()`, and geometry build methods for tests.
- Use one static/dynamic vertex format for colored line/triangle primitives first; add uniforms/camera matrices only if needed for visible bed framing.
- Preserve Phase 23 diagnostic/benchmark evidence while adding Phase 24-specific render-bench counters for dirty uploads if practical.
- Prefer C++ unit coverage for dirty flag behavior because visual screenshot automation for a gated desktop QRhi path is likely heavier than this phase needs.

</specifics>

<deferred>
## Deferred Ideas

- Full Prepare model mesh rendering, active object selection, hover, picking, gizmos, and camera behavior are Phase 25.
- Preview G-code segment buffers, color modes, layer range, and legend/stat sync are Phase 26-27.
- Vulkan promotion remains deferred until QtGui public Vulkan support is available and benchmark evidence beats D3D12/D3D11.
- Bed textures/logos, calibration graphics, exclusion zones, and full upstream plate icon rendering are deferred unless later requirements make them user-visible blockers.

</deferred>
