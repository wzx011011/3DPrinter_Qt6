# Roadmap: OWzx Slicer

## Milestones

- Complete **v2.9 Implementation Realignment and Stabilization** - Phases 10-15 (shipped 2026-06-25)
- Complete **v3.0 PartPlate Core** - Phases 16-22 (shipped 2026-06-26)
- Planning **v3.1 QRhi High-Performance Prepare/Preview Rendering** - Phases 23-28
- Candidate **v3.2 AssembleView + Multi-Plate Polish** - deferred until QRhi plate/model rendering foundation is stable
- Candidate **v3.3 Preset System Completion** - upstream-compatible bundles, CreatePresetsDialog, dirty-state prompts
- Candidate **v3.4 Web, Cloud, Multi-Machine** - blocked/future areas

## Active Milestone: v3.1 QRhi High-Performance Prepare/Preview Rendering

**Goal:** Establish the Qt-native high-performance rendering foundation for Prepare and Preview using QRhi with D3D12-first/D3D11 fallback, while keeping upstream-visible behavior aligned and preserving the current stable fallback path.

**Requirements:** 26 total, 26 mapped, 0 unmapped.

## Phases

### Phase 23: QRhi Renderer Foundation And Backend Gate

- [x] Phase 23: QRhi Renderer Foundation And Backend Gate (completed 2026-06-27)

**Goal:** Introduce the gated QRhi renderer infrastructure, shader pipeline, backend selection policy, and benchmark evidence without changing the default stable viewport startup.

**Requirements:** RHI-01, RHI-02, RHI-03, RHI-05, RHI-06, PERF-05

**Plans:** 3/3 plans complete
- `23-01-PLAN.md` - QRhi backend gate and startup fallback
- `23-02-PLAN.md` - QQuickRhiItem viewport host and shader pipeline
- `23-03-PLAN.md` - benchmark evidence, verification, and requirement traceability

**Success criteria:**
1. `OWZX_RHI_RENDERER` or equivalent runtime/build gate can enable the QRhi path while default startup remains stable.
2. QRhi backend selection attempts D3D12 first and D3D11 fallback on Windows; Vulkan is not default with the current SDK.
3. Qt Shader Tools compile renderer shaders into `.qsb` resources through the canonical `build/` tree.
4. `owzx-render-bench --backend all` reports D3D12/D3D11 metrics and explicitly reports current QtGui Vulkan-disabled status.
5. QML hosts the new viewport item without moving source-truth behavior or rendering business logic into QML scripts.

### Phase 24: Prepare Scene Data And Plate Rendering

- [x] Phase 24: Prepare Scene Data And Plate Rendering (completed 2026-06-27)

**Goal:** Build the GPU scene/cache layer and render active bed/plate state through QRhi, including plate switching and dirty-range data updates.

**Requirements:** RHI-04, PREP-01, PREP-05

**Plans:** 4/4 plans complete
- [x] `24-01-PLAN.md` - Prepare scene data contract and dirty flags
- [x] `24-02-PLAN.md` - Prepare bed and plate context binding
- [x] `24-03-PLAN.md` - QRhi bed grid and plate rendering
- [x] `24-04-PLAN.md` - verification traceability and handoff

**Success criteria:**
1. Active bed/plate dimensions, grid, origin cues, and selected plate context render through QRhi.
2. Plate switching updates rendered plate/object visibility without leaking inactive-plate objects.
3. Prepare scene data has explicit CPU-side dirty tracking and GPU buffer/cache ownership.
4. Plate rendering behavior is mapped to OrcaSlicer source-truth references or documented as performance-only implementation detail.

### Phase 25: Prepare Model Mesh Rendering And Camera Interaction

**Goal:** Render loaded model meshes through QRhi with editor selection/camera interactions aligned to existing source-truth mapped Prepare behavior.

**Requirements:** PREP-02, PREP-03, PREP-04, PREP-07

**Plans:** 1/4 plans executed
- [x] `25-01-PLAN.md` - model batch contract and source mapping
- [ ] `25-02-PLAN.md` - QRhi model mesh rendering and camera uniforms
- [ ] `25-03-PLAN.md` - selection, hover, and picking bridge
- [ ] `25-04-PLAN.md` - verification traceability and handoff

**Success criteria:**
1. STL/OBJ/3MF models render from ProjectService/PartPlate data with correct transform, scale, orientation, and material color.
2. Rotate, pan, zoom, and fit camera operations work in the QRhi viewport without per-frame mesh rebuild.
3. Selection and hover feedback are visible and synchronized with `EditorViewModel` selection state.
4. Implemented Prepare behaviors include upstream source-truth mapping notes.
5. Existing software/OpenGL fallback behavior remains available when QRhi is not enabled.

### Phase 26: Preview G-Code GPU Pipeline

**Goal:** Move Preview rendering onto a QRhi segment-buffer pipeline with color modes, visibility toggles, legends, and layer-range draw control.

**Requirements:** PREV-01, PREV-02, PREV-03, PREV-04, PREV-06

**Success criteria:**
1. After slicing, Preview extrusion/travel segments are converted into compact GPU buffers.
2. Layer range scrubbing changes draw ranges or uniforms instead of reparsing or rebuilding all segments.
3. Core color modes work for feature/move type, filament/tool, and speed-based coloring where data is available.
4. Extrusion/travel visibility toggles affect QRhi rendering consistently with existing Preview controls.
5. Legend/statistics remain synchronized with current color mode and visible layer range.

### Phase 27: Preview Interaction And Performance Gate

**Goal:** Validate large Preview workloads, playback/current-layer interaction, and performance instrumentation against the no-full-reupload design goal.

**Requirements:** PREV-05, PREV-07, PERF-01, PERF-02

**Success criteria:**
1. Preview playback/current-layer navigation updates rendered range/toolhead state without full-buffer reupload.
2. Frame timing, first-frame timing, upload timing, selected backend, and segment counts are captured in structured output.
3. 1M-segment synthetic workload remains interactive under QRhi on the target Windows machine.
4. 5M-segment synthetic workload is benchmarked or explicitly documented as memory/driver limited.
5. Camera movement and layer scrubbing do not trigger per-frame full geometry upload.

### Phase 28: Fallback, Verification, Reviews, And Handoff

**Goal:** Harden the QRhi renderer integration, verify fallback behavior, run canonical checks, and complete code/UI review before milestone handoff.

**Requirements:** PREP-06, PERF-03, PERF-04, PERF-06

**Success criteria:**
1. QRhi initialization failure falls back to the stable viewport with diagnostic notification, not a crash or blank view.
2. Canonical verification passes with QRhi code present and benchmark disabled by default.
3. Optional benchmark can be enabled from the canonical script without changing normal app startup behavior.
4. UI audit or smoke tests guard that fallback remains available and QRhi is explicitly gated until promoted.
5. Code review and UI review P0/P1 findings are fixed or explicitly deferred with rationale.
6. Milestone handoff documents the measured backend choice, remaining Vulkan SDK prerequisite, and next recommended milestone.

## Deferred Milestone Candidates

### v3.2 AssembleView + Multi-Plate Polish

- AssembleView non-placeholder bird's-eye multi-plate layout.
- Multi-plate arrangement and layout operations.
- Wipe-tower geometry polish.
- Multi-thumbnail kinds and filament-map UI.
- PLATE-09 real-model 3MF fixture closure.

### v3.3 Preset System Completion

- Upstream-compatible preset bundles.
- CreatePresetsDialog workflows.
- Dirty-state prompts and preset inheritance parity.

### v3.4 Web, Cloud, Multi-Machine

- ModelMall/Home WebView when QtWebEngine/policy is resolved.
- Cloud account/login flows.
- Multi-machine and live hardware verification.

## Previously Shipped

<details>
<summary>v3.0 PartPlate Core (Phases 16-22) - SHIPPED 2026-06-26</summary>

- [x] Phase 16: PartPlate Data Model Foundation (2 plans)
- [x] Phase 17: Plate Lifecycle Completion (1 plan)
- [x] Phase 18: 3MF Multi-Plate Persistence (1 plan)
- [x] Phase 19: Per-Plate Slice Scheduling (1 plan)
- [x] Phase 20: Verification and Handoff (1 plan)
- [x] Phase 21: Review-Driven Bug Fixes (1 plan, code review)
- [x] Phase 22: UI Review-Driven Fixes (1 plan)

**Requirements:** 14/14 satisfied. **Audit:** `tech_debt` (review-clean).

</details>

<details>
<summary>v2.9 Implementation Realignment and Stabilization (Phases 10-15) - SHIPPED 2026-06-25</summary>

- [x] Phase 10-15. **Requirements:** 28/28. **Details:** `.planning/milestones/v2.9-ROADMAP.md`.

</details>

## Next Step

Execute Phase 25:

```text
$gsd-execute-phase 25
```

---
*Last updated: 2026-06-27 after Phase 25 plan 25-01 execution.*
