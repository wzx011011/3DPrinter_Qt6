# Roadmap: OWzx Slicer

## Milestones

- Complete: **v2.9** - Phases 10-15 (shipped 2026-06-25)
- Complete: **v3.0 PartPlate Core** - Phases 16-22 (shipped 2026-06-26)
- Complete: **v3.1 QRhi Rendering** - Phases 23-28 (shipped 2026-06-28)
- Complete with tech debt: **v3.2 Multi-Plate Data Polish** - Phases 29-32 (audited 2026-06-28)
- Complete at MVP level, superseded for completeness: **v3.3 Slice Preview Main Flow MVP** - Phases 33-36
- Active: **v3.4 Import to G-code Complete Workflow** - Phases 37-43

## Active Milestone: v3.4 Import to G-code Complete Workflow

**Goal:** Complete the full local user workflow from importing a model/project through local G-code export, with source-truth-aligned Prepare readiness, slicing/reslicing, D3D11 QRhi Preview, and export finalization.

**Success criteria:**
- A user can import supported local files through the normal UI paths and see consistent Prepare object, plate, mesh, selection, and fit state.
- Slice readiness and result invalidation are correct after import, object/plate edits, transforms, printable changes, and relevant config changes.
- Slicing, reslicing, cancellation, failure, existing G-code reuse, current-plate slicing, and all-printable-plate slicing have coherent state and notifications.
- Preview renders and remains visible after layer/move/camera interactions using D3D11 QRhi as the normal path.
- Local G-code export writes valid current-plate and all-printable-plate files with safe naming, same-path protection, progress, and failure reporting.
- Automated and manual UAT cover the complete local path before the milestone is marked complete.

## Phases

- [x] **Phase 37:** Complete Import and Project Restore (completed 2026-06-28)
- [x] **Phase 38:** Prepare Readiness and Slice Invalidation (completed 2026-06-28)
- [x] **Phase 39:** Complete Slicing and Reslicing State Machine (completed 2026-06-28)
- [x] **Phase 40:** Complete Preview Data and Upstream View Semantics (completed 2026-06-29)
- [ ] **Phase 41:** D3D11 Preview Rendering and Interaction Stability
- [ ] **Phase 42:** Local G-code Export and Finalization
- [ ] **Phase 43:** End-to-End Verification and Handoff

### Phase 37: Complete Import and Project Restore

**Goal:** Make every model/project import path exposed by the local workflow real, observable, and state-consistent.

**Requirements:** `IMP-01`, `IMP-02`, `IMP-03`, `IMP-04`, `IMP-05`, `IMP-06`.

**Deliverables:**
- Import path audit against upstream `Plater::load_files` and local `ProjectServiceMock::loadFile`.
- Real support or explicit blocked classification for each exposed format.
- Unified import state across topbar, Prepare, drag/drop, and Project page entry points.
- 3MF project restore for plates, filament maps, thumbnails, and compatibility warnings.
- Slice/Preview invalidation after imports.

**Success criteria:**
1. Importing real STL and real 3MF fixtures updates Prepare state, renderer mesh payload, active plate, and fit hint.
2. Unsupported or dependency-blocked formats show clear user-visible errors and do not remain advertised as working.
3. Orca/BBS 3MF plate/project metadata restores or is explicitly classified with a source-truth gap.
4. Import success/failure cannot leave stale loading, slice, Preview, or export state.

### Phase 38: Prepare Readiness and Slice Invalidation

**Goal:** Make Prepare accurately tell the user when slicing, Preview, and export are valid.

**Requirements:** `PREP-01`, `PREP-02`, `PREP-03`, `PREP-04`.

**Deliverables:**
- Central slice-readiness model in C++ viewmodel/service state.
- Invalidation hooks for object, volume, plate, transform, printable, arrangement, bed, filament-map, and config changes.
- Per-plate slice result and export availability state.
- UI enablement reasons for disabled/no-op actions.

**Success criteria:**
1. Any slice-affecting edit clears affected Preview/export availability immediately.
2. Switching plates shows the correct status and statistics for that plate.
3. Prepare never allows Preview/export of stale or missing results.
4. Automated tests cover representative invalidation triggers.

### Phase 39: Complete Slicing and Reslicing State Machine

**Goal:** Align the Qt slicing lifecycle with upstream `BackgroundSlicingProcess` semantics for the local workflow.

**Requirements:** `SLICE-01`, `SLICE-02`, `SLICE-03`, `SLICE-04`, `SLICE-05`, `SLICE-06`.

**Deliverables:**
- Source-truth comparison with `BackgroundSlicingProcess`, `Plater::reslice`, and per-plate slice handling.
- Coherent `SliceService` state machine for slicing, reslicing, cancellation, failure, completion, and previous G-code reuse.
- Per-plate output/statistics storage.
- Validation and warning propagation into Prepare and Preview.

**Success criteria:**
1. Current-plate slicing uses the correct model, presets, bed, plate config, and filament map.
2. All-printable-plate slicing skips locked/non-printable plates and records per-plate results.
3. Reusing an existing G-code file refreshes Preview state without pretending a model slice occurred.
4. Failed or cancelled slices cannot leave exportable stale output.

### Phase 40: Complete Preview Data and Upstream View Semantics

**Goal:** Make Preview data complete enough for the upstream visible Preview controls in the local workflow.

**Requirements:** `PREVIEW-01`, `PREVIEW-02`, `PREVIEW-03`, `PREVIEW-04`.

**Deliverables:**
- Parser/data audit against upstream `GCodeViewer`, `GCodeProcessorResult`, and `IMSlider`.
- Data model support for the required view modes and statistics.
- Legend, marker, layer-time, move-time, and tick/custom-code marker data.
- Tests for parser edge cases and Preview state resets.

**Success criteria:**
1. Preview data contains no stale result after plate switch, reslice, import, or failure.
2. View mode changes recolor from stored data rather than reparsing incorrectly.
3. Legend/statistics/marker/tick data remain consistent with the active G-code result.
4. Parser fixtures cover common OrcaSlicer output tags and motion semantics.

### Phase 41: D3D11 Preview Rendering and Interaction Stability

**Goal:** Make the high-performance Qt Preview renderer stable under real user interaction.

**Requirements:** `PREVIEW-05`, `PREVIEW-06`, `PREVIEW-07`, `PREVIEW-08`.

**Deliverables:**
- D3D11 QRhi normal-path guard and diagnostics.
- Renderer fixes for layer/move range, camera fit/orbit/pan/zoom, resize, travel/bed/marker toggles, and plate switching.
- Buffer lifecycle and dirty-flag hardening so pure range/camera changes do not corrupt or unnecessarily rebuild payloads.
- Performance checks against large toolpath payloads.

**Success criteria:**
1. Dragging layer/move controls never blanks a valid toolpath.
2. Orbiting, panning, zooming, fitting, and resizing preserve visible G-code.
3. Preview does not use `SoftwareViewport` as the normal path on capable Windows systems.
4. Large Preview payloads remain responsive enough for interactive testing.

### Phase 42: Local G-code Export and Finalization

**Goal:** Complete local `.gcode` export semantics for current plate and all printable plates.

**Requirements:** `EXPORT-01`, `EXPORT-02`, `EXPORT-03`, `EXPORT-04`, `EXPORT-05`, `EXPORT-06`.

**Deliverables:**
- Source-truth comparison with `Plater::export_gcode`, `BackgroundSlicingProcess::export_gcode`, and `get_export_gcode_filename`.
- Safe default naming and per-plate target path generation.
- Same-path/self-copy protection and output file validation.
- Export state/progress notifications and failure reporting.
- Export current plate and export all printable plates from normal UI entry points.

**Success criteria:**
1. Export is unavailable when the slice is missing, stale, invalid, or already exporting.
2. Current-plate export writes a non-empty valid `.gcode` file to the selected destination.
3. All-printable-plate export writes deterministic per-plate files or reports per-plate failure.
4. Export can finalize/reslice when required by upstream semantics rather than copying stale output.

### Phase 43: End-to-End Verification and Handoff

**Goal:** Prove the complete local import-to-G-code workflow works and is regression guarded.

**Requirements:** `VERIFY-01`, `VERIFY-02`, `VERIFY-03`, `VERIFY-04`, `VERIFY-05`.

**Deliverables:**
- Automated end-to-end tests for import, Prepare readiness, slice, Preview interaction, and export.
- Format coverage matrix for STL, 3MF, OBJ, AMF, and STEP.
- QML/UI audits for Preview bindings and D3D11 normal path.
- Runtime diagnostics for backend, state transitions, and Preview payload/range.
- Manual UAT checklist and code review.

**Success criteria:**
1. Canonical verification passes with the required E2E coverage.
2. Manual UAT passes for the full local workflow on a running app.
3. No Preview disappearing, stale export, or silent fallback issue remains open at P0/P1 severity.
4. Deferred items are explicitly classified as future or blocked with source-truth rationale.

## Deferred Backlog

- Device send/upload/cloud print and Monitor task workflow.
- Full preset authoring and CreatePresetsDialog workflows beyond slice-correct config use.
- AssembleView.
- Auto filament-map recommendation and wipe-tower geometry/rendering beyond imported-state preservation.
- Real thumbnail capture and 3MF pixel round-trip if not required by local G-code export.
- D3D12 crash root cause and future Vulkan/D3D12 backend promotion.
- ModelMall/Home WebView and cloud workflows.

## Next Step

Plan Phase 41:

```text
$gsd-autonomous --from 41
```

---

*Last updated: 2026-06-29 after Phase 40 execution.*
