# Requirements: OWzx Slicer v3.1 QRhi High-Performance Prepare/Preview Rendering

**Defined:** 2026-06-27
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## v3.1 Requirements

### RHI Infrastructure

- [ ] **RHI-01**: User can start OWzx with the existing stable viewport path unchanged by default while the new QRhi renderer is gated behind an explicit build/runtime switch.
- [ ] **RHI-02**: User can enable the QRhi renderer on Windows and get D3D12 as the first attempted backend with D3D11 fallback when D3D12 is unavailable.
- [ ] **RHI-03**: Developer can build QRhi renderer shaders through Qt Shader Tools as `.qsb` resources using the canonical `build/` directory and canonical verification script.
- [ ] **RHI-04**: Renderer can keep Prepare mesh data and Preview G-code segment data in GPU-resident buffers and update only dirty ranges.
- [ ] **RHI-05**: QML can host the QRhi viewport and overlays without moving rendering business logic or source-truth behavior into QML scripts.
- [ ] **RHI-06**: Developer can run the optional `owzx-render-bench` benchmark to compare available QRhi backends and capture JSON performance metrics.

### Prepare Rendering

- [ ] **PREP-01**: User can see the active bed/plate rendered through the QRhi path with correct bed dimensions, grid, origin cues, and plate selection context.
- [ ] **PREP-02**: User can load STL/OBJ/3MF models and see model meshes rendered from ProjectService/PartPlate data with correct transform, scale, orientation, and material color.
- [ ] **PREP-03**: User can rotate, pan, zoom, and fit the Prepare camera in the QRhi viewport with interaction behavior aligned to the existing source-truth mapped viewport controls.
- [ ] **PREP-04**: User can select and hover models in the QRhi viewport and receive visible highlight/outline feedback consistent with current editor selection state.
- [ ] **PREP-05**: User can switch plates and see the QRhi viewport update to the selected plate without leaking objects from inactive plates.
- [ ] **PREP-06**: User is returned to the stable fallback viewport when QRhi initialization fails, with a diagnostic notification instead of a crash or blank view.
- [ ] **PREP-07**: Developer can trace each implemented Prepare viewport behavior to the corresponding OrcaSlicer upstream behavior or an explicit documented performance-only implementation difference.

### Preview Rendering

- [ ] **PREV-01**: User can slice a model and see G-code extrusion/travel segments rendered through the QRhi preview path.
- [ ] **PREV-02**: User can scrub layer range controls without forcing a full G-code CPU reparse or full GPU buffer rebuild on every interaction.
- [ ] **PREV-03**: User can switch core Preview color modes for the QRhi path, including at least feature/move type, filament/tool, and speed-based coloring where data is available.
- [ ] **PREV-04**: User can toggle visibility for extrusion/travel categories in the QRhi path with behavior matching the existing Preview controls.
- [ ] **PREV-05**: User can use Preview playback/current-layer navigation and see the rendered range/toolhead state update without full-buffer reupload.
- [ ] **PREV-06**: User can read Preview legend/statistics that remain synchronized with the QRhi color mode and visible layer range.
- [ ] **PREV-07**: Developer can load a large synthetic or real Preview workload and verify interactive QRhi rendering without UI hangs from per-frame CPU filtering.

### Performance And Verification

- [ ] **PERF-01**: Developer can capture Prepare and Preview frame timing, first-frame timing, upload timing, and selected backend in structured logs or benchmark JSON.
- [ ] **PERF-02**: QRhi Prepare and Preview paths avoid per-frame full geometry upload for steady camera movement and layer scrubbing.
- [ ] **PERF-03**: Canonical verification still passes with QRhi code present, and the optional benchmark can be enabled without changing the default app startup path.
- [ ] **PERF-04**: UI audit or smoke coverage guards that the stable fallback path remains available and that QRhi remains explicitly gated until promoted.
- [ ] **PERF-05**: Vulkan is documented as non-blocking for v3.1 because the installed Qt 6.10 SDK has QtGui Vulkan disabled; Vulkan may only be reconsidered after a Vulkan-enabled Qt SDK benchmark.
- [ ] **PERF-06**: Code review and UI review findings for the QRhi renderer are fixed or explicitly deferred before the milestone is considered complete.

## Future Requirements

Deferred to future releases. Tracked but not in the v3.1 roadmap.

### Vulkan Backend

- **VK-01**: Developer can install or build a Qt 6.10 SDK with QtGui public Vulkan support enabled.
- **VK-02**: Developer can run the same `owzx-render-bench --backend all` workload against D3D12, Vulkan, and D3D11 on the target machine.
- **VK-03**: OWzx can promote Vulkan to first-choice backend only if same-machine benchmark data proves Vulkan initializes reliably and outperforms D3D12 for representative Prepare/Preview workloads.

### Advanced Viewport Features

- **ADV-01**: User can use all upstream gizmo overlays in the QRhi Prepare viewport.
- **ADV-02**: User can use support/seam/paint-style tools in QRhi once their data pipelines are migrated.
- **ADV-03**: User can use full AssembleView bird's-eye multi-plate editing after the QRhi plate rendering foundation is stable.
- **ADV-04**: User can use full preset bundle/CreatePresetsDialog workflows in a later preset-focused milestone.

## Out of Scope

Explicitly excluded from v3.1.

| Feature | Reason |
|---|---|
| Replacing libslic3r slicing algorithms | Rendering milestone only; slicing engine remains source-truth backend. |
| Making Vulkan the default backend | Current Qt 6.10 SDK disables QtGui Vulkan; no valid local Vulkan QRhi backend exists. |
| Full AssembleView workflow completion | Deferred behind the QRhi plate/model rendering foundation. |
| Full preset system completion | Separate large source-truth area; not required to validate rendering performance. |
| OpenVDB/HMS-dependent rendering tools | Dependency block remains outside this rendering foundation milestone. |

## Traceability

| Requirement | Phase | Status |
|---|---:|---|
| RHI-01 | Phase 23 | Pending |
| RHI-02 | Phase 23 | Pending |
| RHI-03 | Phase 23 | Pending |
| RHI-04 | Phase 24 | Pending |
| RHI-05 | Phase 23 | Pending |
| RHI-06 | Phase 23 | Pending |
| PREP-01 | Phase 24 | Pending |
| PREP-02 | Phase 25 | Pending |
| PREP-03 | Phase 25 | Pending |
| PREP-04 | Phase 25 | Pending |
| PREP-05 | Phase 24 | Pending |
| PREP-06 | Phase 28 | Pending |
| PREP-07 | Phase 25 | Pending |
| PREV-01 | Phase 26 | Pending |
| PREV-02 | Phase 26 | Pending |
| PREV-03 | Phase 26 | Pending |
| PREV-04 | Phase 26 | Pending |
| PREV-05 | Phase 27 | Pending |
| PREV-06 | Phase 26 | Pending |
| PREV-07 | Phase 27 | Pending |
| PERF-01 | Phase 27 | Pending |
| PERF-02 | Phase 27 | Pending |
| PERF-03 | Phase 28 | Pending |
| PERF-04 | Phase 28 | Pending |
| PERF-05 | Phase 23 | Pending |
| PERF-06 | Phase 28 | Pending |

**Coverage:**
- v3.1 requirements: 26 total
- Mapped to phases: 26
- Unmapped: 0

---
*Requirements defined: 2026-06-27*
*Last updated: 2026-06-27 after v3.1 milestone definition*
