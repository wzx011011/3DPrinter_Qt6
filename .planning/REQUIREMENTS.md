# Requirements: OWzx Slicer v3.4 Import to G-code Complete Workflow

**Defined:** 2026-06-28
**Status:** Active - requirements defined, roadmap ready
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## Scope Contract

v3.4 is not an MVP milestone. It is the complete local main workflow from model/project import through local G-code export:

```text
Import model/project -> Prepare readiness -> Slice/reslice -> Preview -> Export local G-code
```

Every user-visible behavior in this path must either be implemented, verified, or explicitly classified as blocked by an unavailable dependency. Device sending, cloud printing, Monitor print jobs, ModelMall, and full application-wide preset authoring remain separate workflows.

## Status Terms

- **Real:** source-truth behavior is implemented and verified with deterministic evidence.
- **Hybrid:** a real path exists, but fallback/mock behavior remains or verification is incomplete.
- **Mock:** local simulation only.
- **Blocked:** requires an unavailable dependency, credential, protocol, or product decision.
- **Placeholder:** visible UI or enum exists but has no meaningful backend behavior.

## v3.4 Requirements

### Import and Project Restore

- [x] **IMP-01:** User can import the locally supported model/project formats exposed by the UI (`.stl`, `.obj`, `.amf`, `.3mf`, Orca/BBS `.3mf`, and `.step/.stp` when OCCT is available) through the normal topbar, Prepare page, drag/drop, and Project page paths.
- [x] **IMP-02:** Import progress, cancellation, success, and failure states are surfaced through Qt signals and user-visible notifications without leaving stale loading state.
- [x] **IMP-03:** Imported geometry populates the Prepare object list, active plate membership, printable flags, mesh render payload, selection state, and fit-view hint consistently.
- [x] **IMP-04:** Orca/BBS 3MF project import restores plate names, current plate, locked/printable state, bed type, print sequence, spiral mode, filament map mode, manual filament maps, and embedded plate thumbnails when present.
- [x] **IMP-05:** 3MF version/config compatibility outcomes are visible to the user and never silently turn project import into partial geometry-only import without a warning.
- [x] **IMP-06:** Importing a new model/project invalidates stale slice and preview state for affected plates while preserving unrelated valid plate state where upstream semantics allow it.

### Prepare Readiness and Invalidation

- [x] **PREP-01:** Prepare page exposes accurate slice readiness for the current plate, including loading state, object count, printable object state, current plate validity, and active slicing/export jobs.
- [x] **PREP-02:** Object, volume, plate, transform, printable flag, arrangement, bed-shape, filament-map, and relevant preset/config changes invalidate affected slice results and Preview data immediately.
- [x] **PREP-03:** Switching plates shows the correct per-plate slice status, Preview availability, output path, filament/time/cost/layer statistics, and export availability.
- [x] **PREP-04:** User-facing actions are enabled/disabled with source-truth-aligned reasons instead of allowing hidden no-ops, stale exports, or Preview of invalid results.

### Slicing and Reslicing

- [x] **SLICE-01:** Normal slicing uses the current plate model, merged presets, bed shape, per-plate config, filament map state, and calibration-neutral state consistently with upstream `BackgroundSlicingProcess`.
- [x] **SLICE-02:** Slice progress, completion, cancellation, and failure states remain coherent across `SliceService`, `EditorViewModel`, `PreviewViewModel`, notifications, and QML controls.
- [x] **SLICE-03:** Reslicing is triggered or required when imported data or slice-affecting settings change, and stale `outputPath` values cannot be treated as current results.
- [x] **SLICE-04:** Existing G-code reuse (`export_gcode_from_previous_file` equivalent) is supported when loading/reprocessing a valid previous G-code result, with Preview refreshed from the reused file.
- [x] **SLICE-05:** Multi-plate slicing covers current plate and all printable unlocked plates, stores per-plate output paths/statistics, and excludes locked or non-printable plates.
- [x] **SLICE-06:** Slice warnings and validation failures are surfaced as user-visible errors/warnings and block Preview/export when the generated result is invalid.

### Preview Data and Semantics

- [x] **PREVIEW-01:** Preview state is built from the current valid G-code result and exposes output path, layer count, move count, layer range, move range, tool position, and per-plate association without stale data.
- [x] **PREVIEW-02:** G-code parsing covers common OrcaSlicer output semantics required by Preview: travel/extrusion moves, extrusion modes, `G92 E`, Z/layer markers, feature/role tags, width, height, feedrate, fan speed, temperature, acceleration, tool changes, and elapsed time.
- [x] **PREVIEW-03:** Preview view modes align with upstream `GCodeViewer` for the local workflow: feature type, height, width, feedrate, fan speed, temperature, tool/extruder, filament/color where data is available, and chronology/layer-time modes when derivable from the result.
- [x] **PREVIEW-04:** Legend, statistics, per-extruder usage, layer-time chart data, move-time labels, tick/custom-code markers, and marker tooltip values match the active Preview data.

### Preview Rendering and Interaction

- [ ] **PREVIEW-05:** Default Windows Preview rendering uses D3D11 QRhi on capable systems and does not fall back to `SoftwareViewport` as the normal path.
- [ ] **PREVIEW-06:** Layer slider, move slider, travel visibility, bed visibility, marker visibility, color mode, play/pause, and camera controls update the same rendered toolpath without blanking, disappearing, freezing, or corrupting buffers.
- [ ] **PREVIEW-07:** Preview camera fit, orbit, pan, zoom, reset, plate switching, and resize preserve a visible valid toolpath for small and large G-code files.
- [ ] **PREVIEW-08:** Preview performance remains within the v3.1 D3D11 QRhi expectation for large toolpath payloads, with no avoidable full-buffer rebuilds on pure range/camera changes.

### Local G-code Export

- [ ] **EXPORT-01:** Export availability follows upstream semantics: export is blocked when there is no valid current slice, when slicing/export is running, or when the result is stale.
- [ ] **EXPORT-02:** Export uses upstream-style default file naming, including project/object name and plate name/index for multi-plate projects.
- [ ] **EXPORT-03:** Export finalizes the current valid temporary/generated G-code to the user-selected target path, protects same-path/self-copy cases, and verifies the target file exists and is non-empty.
- [ ] **EXPORT-04:** Export can trigger required reslice/finalization before writing, rather than only copying a previously generated path when upstream would update the background process.
- [ ] **EXPORT-05:** Export progress, success, failure, cancellation, and destination path notifications are visible and consistent across Prepare, topbar menu, and notification actions.
- [ ] **EXPORT-06:** Export current plate and export all printable plates are supported for local `.gcode` output, with deterministic per-plate paths and failures reported per plate.

### End-to-End Verification

- [ ] **VERIFY-01:** Automated tests cover the full local path: import fixture, verify Prepare readiness, slice, enter Preview, interact with layer/move/camera controls, export G-code, and validate output.
- [ ] **VERIFY-02:** Format coverage tests exercise at least real STL and real 3MF fixtures; OBJ/AMF/STEP support is tested or explicitly classified based on available dependencies.
- [ ] **VERIFY-03:** QML/UI audits prevent normal-path Preview fallback to `SoftwareViewport` and catch Preview binding regressions for controls used in the main workflow.
- [ ] **VERIFY-04:** Runtime diagnostics record selected renderer backend, import/slice/export state transitions, and Preview payload/render range enough to debug blank-preview regressions.
- [ ] **VERIFY-05:** Manual UAT checklist matches the complete local workflow and must pass before v3.4 is marked complete.

## Future Requirements

- Full device send/print workflows, including upload to printer, cloud printing, and Monitor task lifecycle.
- Full preset authoring and CreatePresetsDialog workflows beyond the preset data needed for local slicing correctness.
- AssembleView source-truth completion.
- Auto filament-map recommendation and wipe-tower geometry/rendering beyond what is required to preserve imported state.
- Real GL/QRhi-capture thumbnails and 3MF pixel round-trip if not required by local G-code export.
- D3D12 or Vulkan default promotion after separate backend feasibility and stability work.
- ModelMall/Home WebView and cloud-related workflows.
- Full i18n translation coverage beyond strings touched by the local workflow.

## Out of Scope

| Feature | Reason |
|---|---|
| Device send, upload, cloud print, and Monitor print-job workflow | Separate device workflow after local G-code export is complete. |
| Changing libslic3r slicing algorithms | GUI migration preserves libslic3r behavior. |
| Making D3D12 or Vulkan the default backend | D3D11 QRhi is the known stable Qt-native Windows backend; D3D12/Vulkan remain separate performance/backend work. |
| Full application-wide preset editing parity | v3.4 only includes preset/config behavior required by import, slicing, preview, and export correctness. |
| OpenVDB/WebRTC/FFmpeg-dependent workflows | Unavailable dependency areas unrelated to local G-code export. |

## Traceability

| Requirement | Phase | Status |
|---|---|---|
| IMP-01 | Phase 37 | Satisfied |
| IMP-02 | Phase 37 | Satisfied |
| IMP-03 | Phase 37 | Satisfied |
| IMP-04 | Phase 37 | Satisfied |
| IMP-05 | Phase 37 | Satisfied |
| IMP-06 | Phase 37 | Satisfied |
| PREP-01 | Phase 38 | Satisfied |
| PREP-02 | Phase 38 | Satisfied |
| PREP-03 | Phase 38 | Satisfied |
| PREP-04 | Phase 38 | Satisfied |
| SLICE-01 | Phase 39 | Satisfied |
| SLICE-02 | Phase 39 | Satisfied |
| SLICE-03 | Phase 39 | Satisfied |
| SLICE-04 | Phase 39 | Satisfied |
| SLICE-05 | Phase 39 | Satisfied |
| SLICE-06 | Phase 39 | Satisfied |
| PREVIEW-01 | Phase 40 | Satisfied |
| PREVIEW-02 | Phase 40 | Satisfied |
| PREVIEW-03 | Phase 40 | Satisfied |
| PREVIEW-04 | Phase 40 | Satisfied |
| PREVIEW-05 | Phase 41 | Pending |
| PREVIEW-06 | Phase 41 | Pending |
| PREVIEW-07 | Phase 41 | Pending |
| PREVIEW-08 | Phase 41 | Pending |
| EXPORT-01 | Phase 42 | Pending |
| EXPORT-02 | Phase 42 | Pending |
| EXPORT-03 | Phase 42 | Pending |
| EXPORT-04 | Phase 42 | Pending |
| EXPORT-05 | Phase 42 | Pending |
| EXPORT-06 | Phase 42 | Pending |
| VERIFY-01 | Phase 43 | Pending |
| VERIFY-02 | Phase 43 | Pending |
| VERIFY-03 | Phase 43 | Pending |
| VERIFY-04 | Phase 43 | Pending |
| VERIFY-05 | Phase 43 | Pending |

**Coverage:** 35 total; 35 mapped; 0 unmapped; 20 satisfied.

---

*Requirements defined: 2026-06-28*
*Last updated: 2026-06-29 after Phase 40 execution.*
