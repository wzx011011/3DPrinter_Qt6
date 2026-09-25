# Phase 43: End-to-End Verification and Handoff - Context

**Gathered:** 2026-06-29
**Status:** Ready for planning

<domain>

## Phase Boundary

Phase 43 closes v3.4 by proving the complete local workflow works as one user-visible path:

```text
Import model/project -> Prepare readiness -> Slice/reslice -> Preview interaction -> Export local G-code
```

This is not an MVP checkpoint. It is the full v3.4 acceptance pass for the local main workflow already delivered by Phases 37-42. The phase may add tests, diagnostics, audits, review evidence, and handoff artifacts. It may fix blocking P0/P1 defects discovered during verification, but it must not add unrelated product scope.

Out of scope remains unchanged: device send/upload/cloud print, Monitor print-job lifecycle, full preset authoring, AssembleView, auto filament-map recommendation, wipe-tower geometry/rendering, real thumbnail capture, and D3D12/Vulkan default promotion.

</domain>

<decisions>

## Implementation Decisions

### Verification Scope

- D-43-01: The acceptance path must start from real import input and end with a verified local `.gcode` file. Passing isolated unit tests is not enough.
- D-43-02: Automated tests must cover import, Prepare readiness, slicing, Preview data availability, Preview interaction controls, current-plate export, all-printable-plate export, and output validation.
- D-43-03: Format coverage must include real STL and real 3MF fixtures. OBJ, AMF, and STEP must be tested if fixtures/dependencies are available; otherwise they must be explicitly classified with a source-truth and dependency rationale.
- D-43-04: Preview normal path must be the high-performance QRhi/D3D11 path on Windows. `SoftwareViewport` may remain as a fallback type, but Preview must not instantiate or rely on it for the normal path.
- D-43-05: Runtime diagnostics must make blank-preview and stale-export bugs diagnosable by recording selected renderer backend, import/slice/export transitions, Preview payload size/range, and render range decisions.
- D-43-06: Manual UAT must run the app and exercise the same workflow a user will perform: import, slice, enter Preview, drag layer/move controls, rotate camera, export current G-code, and export all valid plates.
- D-43-07: No Preview disappearing, stale export, silent fallback, or missing output issue may remain open at P0/P1 severity when v3.4 is marked complete.
- D-43-08: If verification finds an implementation defect, fix the defect in the narrowest owning layer and rerun canonical verification before closeout.

### Source-Truth and Architecture

- D-43-09: Upstream OrcaSlicer remains the user-visible behavior source truth for import, slicing, Preview semantics, and local export. Qt-specific renderer implementation may differ, but feature behavior must remain aligned.
- D-43-10: Business behavior and validation stay in C++ services/viewmodels; QML audits may guard bindings, but QML must not become the owner of workflow state.
- D-43-11: Full verification uses only the canonical command `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` and only the `build/` directory.

</decisions>

<canonical_refs>

## Canonical References

### Planning Scope

- `.planning/ROADMAP.md` - v3.4 Phase 43 goal, deliverables, and success criteria.
- `.planning/REQUIREMENTS.md` - `VERIFY-01` through `VERIFY-05` and out-of-scope boundaries.
- `.planning/STATE.md` - current milestone state and phase routing.

### Prior Phase Evidence

- `.planning/phases/37-complete-import-and-project-restore/37-SUMMARY.md` - import/project restore completion evidence.
- `.planning/phases/38-prepare-readiness-and-slice-invalidation/38-SUMMARY.md` - Prepare readiness and stale-state evidence.
- `.planning/phases/39-complete-slicing-and-reslicing-state-machine/39-SUMMARY.md` - slicing lifecycle evidence.
- `.planning/phases/40-complete-preview-data-and-upstream-view-semantics/40-SUMMARY.md` - Preview data semantics evidence.
- `.planning/phases/41-d3d11-preview-rendering-and-interaction-stability/41-SUMMARY.md` - D3D11 Preview rendering and interaction stability evidence.
- `.planning/phases/42-local-gcode-export-and-finalization/42-SUMMARY.md` - local G-code export evidence.

### Code and Tests

- `tests/E2EWorkflowTests.cpp` - automated workflow tests for slicing, Preview payload, stale invalidation, and export.
- `tests/QmlUiAuditTests.cpp` - QML/source audits for renderer selection, Preview bindings, export UI, and fallback restrictions.
- `tests/ViewModelSmokeTests.cpp` - viewmodel smoke coverage for import, rendering-facing state, and service wiring.
- `src/qml_gui/main_qml.cpp` - startup renderer selection and diagnostic logging.
- `src/qml_gui/Renderer/RhiBackendSelector.cpp` - QRhi/D3D11 backend selection and diagnostics.
- `src/qml_gui/Renderer/RhiViewport.cpp` - QML viewport item state and Preview interaction setters.
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp` - QRhi draw path, Preview segment upload, and render range handling.
- `src/core/services/ProjectServiceMock.cpp` - model/project import and format handling.
- `src/core/services/SliceService.cpp` - slicing lifecycle, Preview payload generation, and export finalization.
- `src/core/viewmodels/EditorViewModel.cpp` - Prepare page orchestration and export entry points.
- `src/core/viewmodels/PreviewViewModel.cpp` - Preview view state and controls.

</canonical_refs>

<specifics>

## Specific Verification Requirements

- Add one automated E2E path that loads a real STL fixture, verifies Prepare readiness, slices, verifies Preview payload metadata, applies layer/move/view interactions, exports current `.gcode`, exports all valid plates, and validates non-empty output files.
- Add or extend 3MF fixture coverage so project import is not silently treated as geometry-only without traceability.
- Add a deterministic format matrix artifact for STL, 3MF, OBJ, AMF, and STEP with statuses `tested`, `blocked`, `deferred`, or `not applicable`.
- Add QML/source audits that fail if Preview normal path references `SoftwareViewport`, if startup defaults away from D3D11, or if Preview controls lose bindings used by the main workflow.
- Add runtime diagnostics only where missing. Prefer existing `startup_diagnostics.log`, `qInfo`, and service/viewmodel state logging patterns over a new logging framework.
- Produce `43-REVIEW.md`, `43-VERIFICATION.md`, `43-UAT.md`, and `43-SUMMARY.md` before marking v3.4 complete.

</specifics>

<deferred>

## Deferred Ideas

- D3D12 crash root cause and Vulkan/D3D12 backend promotion remain future backend-performance work.
- Device send/upload/cloud printing and Monitor task lifecycle remain v3.5+.
- Full preset authoring and CreatePresetsDialog workflows remain v3.5+.
- Real GL/QRhi thumbnail capture and 3MF pixel round-trip remain v3.5+ unless a Phase 43 test exposes a local G-code workflow blocker.

</deferred>

---

*Phase: 43-end-to-end-verification-and-handoff*
*Context gathered: 2026-06-29*
