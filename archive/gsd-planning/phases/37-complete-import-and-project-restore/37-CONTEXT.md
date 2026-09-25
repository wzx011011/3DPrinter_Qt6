# Phase 37: Complete Import and Project Restore - Context

**Gathered:** 2026-06-28
**Status:** Ready for planning

<domain>

## Phase Boundary

Phase 37 delivers complete import and project restore for the local import-to-G-code workflow: all normal UI entry points must route into a single C++ import path, exposed file formats must be either real and verified or explicitly classified/hidden/blocked, imported data must refresh Prepare mesh/object/plate/fit state, and import must invalidate stale slice/Preview/export state. It does not implement slicing, Preview rendering fixes, or export finalization; those are Phases 38-42.

</domain>

<decisions>

## Implementation Decisions

### Source-Truth Coverage

- Use OrcaSlicer `Plater::load_files` and libslic3r `Model::read_from_file` / `Model::read_from_archive` as behavior references; no new OWzx import behavior.
- Treat each UI-advertised format as a contract: `.stl`, `.obj`, `.amf`, `.3mf`, Orca/BBS `.3mf`, `.step/.stp` when OCCT/runtime is available. Unsupported/dependency-blocked formats must be user-visible and not silently advertised as working.
- Preserve existing libslic3r import routes; do not alter geometry algorithms or file parsers.
- Record source-truth gaps in planning artifacts if exact upstream behavior is blocked.

### Entry Points and State

- Normalize topbar, Prepare open dialog, drag/drop, and Project page import into the same backend import semantics.
- Import success refreshes ProjectService, EditorViewModel object entries, active plate membership, mesh payload, fit hint, selection, and load progress.
- Import failure/cancel clears loading state and reports an actionable error without leaving stale UI.
- Importing a new model/project invalidates affected slice, Preview, and export state immediately.

### 3MF Project Restore

- Restore plate names, current plate, locked/printable state, bed type, print sequence, spiral mode, filament maps/manual mode, and thumbnails if present.
- Restore compatibility/version/config-substitution messages as user-visible warnings rather than silently degrading to geometry-only import.
- Keep multi-plate metadata extraction in `ProjectServiceMock` / PartPlate model, not in QML.
- If a 3MF field exists upstream but is not yet represented locally, classify it as Hybrid/Blocked/Future with evidence.

### Verification

- Add focused failing tests before implementation where practical.
- Cover at least real STL and real 3MF fixtures in automated tests; OBJ/AMF/STEP must be verified or classified according to current dependency state.
- Add QML audit coverage for advertised import filters and entry point consistency if behavior is mostly wiring.
- Use canonical build command for full verification before claiming Phase 37 complete.

### The Agent's Discretion

Implementation details, helper boundaries, and exact test decomposition are at the agent's discretion as long as C++ owns durable behavior and QML only wires presentation.

</decisions>

<code_context>

## Existing Code Insights

### Reusable Assets

- `src/core/services/ProjectServiceMock.{h,cpp}` already owns real libslic3r import, 3MF plate metadata extraction, mesh payload generation, and project state.
- `src/core/viewmodels/EditorViewModel.{h,cpp}` already exposes `loadFile`, loading progress, object list, active plate data, meshData, fitHint, and slice bridge state to QML.
- `src/qml_gui/BackendContext.{h,cpp}` already has topbar import/open handlers and notification infrastructure.
- `src/qml_gui/pages/PreparePage.qml`, `src/qml_gui/BBLTopbar.qml`, `src/qml_gui/pages/ProjectPage.qml`, and drag/drop code are the relevant UI entry points.
- Existing tests: `tests/ViewModelSmokeTests.cpp`, `tests/E2EWorkflowTests.cpp`, `tests/QmlUiAuditTests.cpp`, `tests/PartPlateTests.cpp`, plus `tests/data/test_model.stl`.

### Established Patterns

- Business rules in C++ services/viewmodels; QML calls invokable actions and binds state.
- Source-truth status vocabulary: Real/Hybrid/Mock/Blocked/Placeholder.
- Tests can combine static QML audits with ViewModel/E2E fixture tests.
- Existing 3MF restore work already uses `pendingPlate*` staging in `ProjectServiceMock`.

### Integration Points

- Source truth: `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp` (`load_files`) and libslic3r format readers.
- Local backend: `ProjectServiceMock::loadFile`, project load path, `EditorViewModel::loadFile`, `BackendContext::topbarImportModel`.
- Local UI: topbar import filters, Prepare file dialog/drop, Project page file dialog.
- Downstream invalidation connects to Phase 38, but Phase 37 must at least clear stale slice/Preview/export state after import.

</code_context>

<specifics>

## Specific Ideas

- The user explicitly rejected MVP scope; Phase 37 must plan for complete import/project restore, not a "works for STL only" path.
- User wants the fastest/most-complete local main workflow path; avoid spending this phase on device/cloud print or renderer backend experiments.
- The app previously had visible Preview instability; import must not hand off stale or partial state into Preview.

</specifics>

<deferred>

## Deferred Ideas

- Device send/upload/cloud print and Monitor job lifecycle.
- Full export finalization, reslice semantics, and Preview rendering fixes are later v3.4 phases.
- D3D12/Vulkan backend promotion.

</deferred>
