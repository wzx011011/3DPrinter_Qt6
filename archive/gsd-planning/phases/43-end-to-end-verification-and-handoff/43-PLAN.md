---
phase: 43
plan: 01
type: verification
wave: 1
depends_on: []
files_modified:
  - tests/E2EWorkflowTests.cpp
  - tests/QmlUiAuditTests.cpp
  - tests/ViewModelSmokeTests.cpp
  - src/qml_gui/main_qml.cpp
  - src/qml_gui/Renderer/RhiViewport.cpp
  - src/qml_gui/Renderer/RhiViewportRenderer.cpp
  - src/core/services/ProjectServiceMock.cpp
  - src/core/services/SliceService.cpp
  - src/core/viewmodels/EditorViewModel.cpp
  - src/core/viewmodels/PreviewViewModel.cpp
  - .planning/phases/43-end-to-end-verification-and-handoff/43-REVIEW.md
  - .planning/phases/43-end-to-end-verification-and-handoff/43-VERIFICATION.md
  - .planning/phases/43-end-to-end-verification-and-handoff/43-UAT.md
  - .planning/phases/43-end-to-end-verification-and-handoff/43-SUMMARY.md
  - .planning/REQUIREMENTS.md
  - .planning/ROADMAP.md
  - .planning/STATE.md
autonomous: true
requirements_addressed: [VERIFY-01, VERIFY-02, VERIFY-03, VERIFY-04, VERIFY-05]
source_truth:
  - third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp
  - third_party/OrcaSlicer/src/slic3r/GUI/BackgroundSlicingProcess.cpp
  - third_party/OrcaSlicer/src/slic3r/GUI/GCodeViewer.cpp
  - third_party/OrcaSlicer/src/slic3r/GUI/GUI_App.cpp
---

# Phase 43 Plan: End-to-End Verification and Handoff

<objective>
Close v3.4 by proving the complete local import-to-G-code workflow works as a single user-visible path, is regression guarded by automated tests and QML/source audits, has enough runtime diagnostics to debug renderer/state regressions, and has manual UAT evidence before milestone completion.
</objective>

<truths>

- D-43-01: Verification must start from real import input and end with verified local `.gcode` output.
- D-43-02: Automated coverage must include import, Prepare readiness, slicing, Preview payload/control interaction, current-plate export, all-printable-plate export, and output validation.
- D-43-03: STL and 3MF require real fixture coverage; OBJ, AMF, and STEP require tested/blocked/deferred classification.
- D-43-04: Normal Preview path must use QRhi/D3D11 on Windows. `SoftwareViewport` is allowed only as a fallback registration, not as the normal Preview implementation.
- D-43-05: Runtime diagnostics must include selected renderer backend, import/slice/export state transitions, Preview payload size/range, and render range decisions.
- D-43-06: Manual UAT must exercise import, slice, Preview layer/move/camera interaction, current export, and all-plate export in the running app.
- D-43-07: P0/P1 issues in Preview disappearance, stale export, silent fallback, or missing output block closeout.
- D-43-08: Only `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` is accepted as full verification.
- D-43-09: OrcaSlicer remains the source truth for import, slicing, Preview semantics, and local export behavior.
- D-43-10: Business behavior and validation remain in C++ services/viewmodels; QML only presents state and wiring.
- D-43-11: Full verification uses the canonical build command and the existing `build/` directory only.

</truths>

<tasks>

## Task 1 - Add End-to-End Workflow Regression Coverage

type: tdd
files:
- `tests/E2EWorkflowTests.cpp`
- `tests/ViewModelSmokeTests.cpp`

action:
- Add or extend an automated workflow test named `test_local_import_slice_preview_export_workflow`.
- The test must load a real STL fixture through `EditorViewModel::loadFile`, wait for load completion, verify Prepare state can slice, request slicing, wait for `SliceService::sliceFinished`, verify `PreviewViewModel::gcodePreviewData()` is non-empty, exercise Preview layer range/move range/view controls through existing viewmodel or renderer-facing APIs, export current `.gcode` to a deterministic temp path, and verify the exported file exists and has non-zero size.
- Extend coverage for all-printable-plate export by using existing multi-plate result state from `SliceService` and validating deterministic per-plate output files for valid unlocked printable plates.
- Keep tests deterministic by using existing fixtures and temp directories. Do not introduce network, printer, cloud, or hardware dependencies.

read_first:
- `tests/E2EWorkflowTests.cpp`
- `tests/ViewModelSmokeTests.cpp`
- `src/core/services/SliceService.h`
- `src/core/services/SliceService.cpp`
- `src/core/viewmodels/EditorViewModel.h`
- `src/core/viewmodels/EditorViewModel.cpp`
- `src/core/viewmodels/PreviewViewModel.h`
- `src/core/viewmodels/PreviewViewModel.cpp`
- `.planning/phases/37-complete-import-and-project-restore/37-SUMMARY.md`
- `.planning/phases/38-prepare-readiness-and-slice-invalidation/38-SUMMARY.md`
- `.planning/phases/39-complete-slicing-and-reslicing-state-machine/39-SUMMARY.md`
- `.planning/phases/40-complete-preview-data-and-upstream-view-semantics/40-SUMMARY.md`
- `.planning/phases/42-local-gcode-export-and-finalization/42-SUMMARY.md`

verify:
- Run focused E2E binary after build:
  - `.\build\E2EWorkflowTests.exe -o build\e2e_phase43_workflow.txt,txt`
- If the binary is stale or missing, run canonical verification.

acceptance_criteria:
- `tests/E2EWorkflowTests.cpp` contains `test_local_import_slice_preview_export_workflow`.
- The new or extended E2E test verifies non-empty Preview payload and non-empty exported `.gcode` output.
- The test covers both current-plate export and all-valid-plate export without depending on hardware, network, or cloud services.

## Task 2 - Add Format Coverage Matrix and Import Assertions

type: verification
files:
- `tests/E2EWorkflowTests.cpp`
- `tests/ViewModelSmokeTests.cpp`
- `.planning/phases/43-end-to-end-verification-and-handoff/43-VERIFICATION.md`

action:
- Add tests or explicit verification notes for STL, 3MF, OBJ, AMF, and STEP.
- STL and 3MF must use real fixtures and must verify import success or a clearly surfaced compatibility failure. 3MF must not silently become geometry-only import without warning when project/config compatibility is relevant.
- OBJ, AMF, and STEP must be classified in `43-VERIFICATION.md` as `tested`, `blocked`, `deferred`, or `not applicable`, with the exact fixture/dependency reason. STEP classification must account for current OCCT availability and any libslic3r import limitations observed in the code path.
- Do not mark blocked/deferred formats as satisfied behavior; mark them as explicit coverage classification for `VERIFY-02`.

read_first:
- `tests/E2EWorkflowTests.cpp`
- `tests/ViewModelSmokeTests.cpp`
- `src/core/services/ProjectServiceMock.cpp`
- `.planning/REQUIREMENTS.md`
- `.planning/phases/37-complete-import-and-project-restore/37-SUMMARY.md`

verify:
- Run focused import/format tests if available:
  - `.\build\E2EWorkflowTests.exe -o build\e2e_phase43_formats.txt,txt`
  - `.\build\ViewModelSmokeTests.exe -o build\viewmodel_phase43_formats.txt,txt`
- Record the format matrix in `43-VERIFICATION.md`.

acceptance_criteria:
- `43-VERIFICATION.md` includes a table for STL, 3MF, OBJ, AMF, and STEP.
- STL and 3MF have real fixture evidence.
- OBJ, AMF, and STEP have explicit tested/blocked/deferred/not-applicable classification with a concrete reason.

## Task 3 - Guard Preview Normal Path and Interaction Bindings

type: verification
files:
- `tests/QmlUiAuditTests.cpp`
- `src/qml_gui/pages/PreviewPage.qml`
- `src/qml_gui/main_qml.cpp`
- `src/qml_gui/Renderer/RhiBackendSelector.cpp`
- `src/qml_gui/Renderer/RhiViewport.cpp`
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp`

action:
- Extend QML/source audits so `PreviewPage.qml` does not instantiate `SoftwareViewport` directly and normal Preview bindings use `OWzxGL.GLViewport`.
- Assert startup QRhi policy remains D3D11-first on Windows and that D3D12/Vulkan are not promoted to default by Phase 43.
- Assert `SoftwareViewport` remains fallback-only registration in `main_qml.cpp`.
- Assert Preview layer range, move range, show-travel, color/view mode, and camera interaction properties are forwarded to the renderer without clearing `previewData`.
- Assert renderer resource rebuild and draw-range code preserves non-empty Preview segment buffers across layer slider and mouse rotation paths.

read_first:
- `tests/QmlUiAuditTests.cpp`
- `src/qml_gui/pages/PreviewPage.qml`
- `src/qml_gui/main_qml.cpp`
- `src/qml_gui/Renderer/RhiBackendSelector.cpp`
- `src/qml_gui/Renderer/RhiViewport.cpp`
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp`
- `.planning/phases/41-d3d11-preview-rendering-and-interaction-stability/41-SUMMARY.md`

verify:
- Run:
  - `.\build\QmlUiAuditTests.exe -o build\qml_phase43_preview.txt,txt`

acceptance_criteria:
- QML/source audits fail if Preview normal path uses `SoftwareViewport`.
- QML/source audits fail if D3D11 is no longer the default Windows QRhi candidate.
- QML/source audits cover layer/move/camera interaction bindings that previously caused disappearing Preview.

## Task 4 - Add Runtime Diagnostics for Main Workflow State

type: implementation
files:
- `src/qml_gui/main_qml.cpp`
- `src/qml_gui/Renderer/RhiViewport.cpp`
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp`
- `src/core/services/ProjectServiceMock.cpp`
- `src/core/services/SliceService.cpp`
- `src/core/viewmodels/EditorViewModel.cpp`
- `src/core/viewmodels/PreviewViewModel.cpp`
- `tests/QmlUiAuditTests.cpp`

action:
- Add missing diagnostics using existing logging patterns only. Prefer `appendStartupLog`, `qInfo`, and existing tagged logs.
- Ensure startup logs selected renderer backend and fallback reasons from `RhiBackendSelection::diagnostics()`.
- Ensure import logs include local path, recognized format extension, success/failure, and loaded object/plate counts where available.
- Ensure slice logs include request start, finish/fail/cancel, active plate index, output path, stale/valid state, and Preview payload byte count.
- Ensure export logs include current/all-plate export start, target path, per-plate result, byte count, and failure reason.
- Ensure Preview renderer logs include payload size, layer min/max, move end, segment count, and render draw range when those values change or when an empty range would be drawn.
- Keep logs low-volume and deterministic enough for diagnostics; do not log per-vertex or per-G-code-line data.

read_first:
- `src/qml_gui/main_qml.cpp`
- `src/qml_gui/Renderer/RhiBackendSelector.cpp`
- `src/qml_gui/Renderer/RhiViewport.cpp`
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp`
- `src/core/services/ProjectServiceMock.cpp`
- `src/core/services/SliceService.cpp`
- `src/core/viewmodels/EditorViewModel.cpp`
- `src/core/viewmodels/PreviewViewModel.cpp`
- `tests/QmlUiAuditTests.cpp`

verify:
- Run QML/source audit tests.
- Inspect `startup_diagnostics.log` after app launch or canonical smoke and record selected backend evidence in `43-VERIFICATION.md`.

acceptance_criteria:
- Diagnostics include renderer backend selection and fallback reason text.
- Diagnostics include import, slice, Preview payload/range, and export transitions.
- Diagnostic additions do not create high-volume logs inside per-vertex or per-line render loops.

## Task 5 - Run Canonical Verification, Review, Manual UAT, and Closeout

type: verification
files:
- `.planning/phases/43-end-to-end-verification-and-handoff/43-REVIEW.md`
- `.planning/phases/43-end-to-end-verification-and-handoff/43-VERIFICATION.md`
- `.planning/phases/43-end-to-end-verification-and-handoff/43-UAT.md`
- `.planning/phases/43-end-to-end-verification-and-handoff/43-SUMMARY.md`
- `.planning/REQUIREMENTS.md`
- `.planning/ROADMAP.md`
- `.planning/STATE.md`

action:
- Stop existing `OWzxSlicer.exe` processes before verification if needed.
- Run:
  - `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
  - `git diff --check`
- Review changed code and tests for:
  - Preview disappearing after layer/move/camera interaction;
  - accidental normal-path `SoftwareViewport` fallback;
  - stale export leakage;
  - missing current/all-plate output verification;
  - format coverage misclassification;
  - diagnostics that are too noisy or omit failure reasons.
- Write `43-REVIEW.md`, `43-VERIFICATION.md`, `43-UAT.md`, and `43-SUMMARY.md`.
- Launch `build/OWzxSlicer.exe` for manual UAT after automated verification passes.
- Manual UAT checklist must include:
  - import a real STL;
  - import/open a real 3MF if available;
  - verify Prepare readiness and slice button state;
  - slice and enter Preview;
  - drag layer height/range controls;
  - drag move controls if visible;
  - rotate/zoom camera with mouse;
  - verify model/G-code does not disappear;
  - export current plate `.gcode`;
  - export all printable valid plates;
  - verify exported files are non-empty.
- Mark `VERIFY-01` through `VERIFY-05` satisfied only after automated verification, review, and manual UAT evidence are recorded.

read_first:
- `.codex/rules/build-rules.md`
- `.codex/rules/source-truth-migration.md`
- `.planning/REQUIREMENTS.md`
- `.planning/ROADMAP.md`
- `.planning/STATE.md`
- `.planning/phases/43-end-to-end-verification-and-handoff/43-CONTEXT.md`
- `.planning/phases/37-complete-import-and-project-restore/37-SUMMARY.md`
- `.planning/phases/38-prepare-readiness-and-slice-invalidation/38-SUMMARY.md`
- `.planning/phases/39-complete-slicing-and-reslicing-state-machine/39-SUMMARY.md`
- `.planning/phases/40-complete-preview-data-and-upstream-view-semantics/40-SUMMARY.md`
- `.planning/phases/41-d3d11-preview-rendering-and-interaction-stability/41-SUMMARY.md`
- `.planning/phases/42-local-gcode-export-and-finalization/42-SUMMARY.md`

verify:
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- `git diff --check`
- Runtime launch of `build/OWzxSlicer.exe` for user/manual UAT.

acceptance_criteria:
- Canonical verification exits 0.
- `git diff --check` exits 0.
- `43-REVIEW.md`, `43-VERIFICATION.md`, `43-UAT.md`, and `43-SUMMARY.md` exist.
- `VERIFY-01` through `VERIFY-05` are marked satisfied only after evidence is recorded.
- The app is launched for manual UAT or the reason it could not be launched is recorded.

</tasks>

<verification>

1. Add/extend RED-capable automated tests for the full local workflow and format matrix.
2. Add/extend QML/source audits for Preview D3D11 normal path and interaction bindings.
3. Add targeted runtime diagnostics if missing.
4. Run focused test binaries where useful.
5. Run canonical verification:
   - `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
6. Run `git diff --check`.
7. Perform code review and record `43-REVIEW.md`.
8. Write `43-VERIFICATION.md` with automated evidence and format matrix.
9. Launch the app and run or hand off `43-UAT.md`.
10. Update requirements, roadmap, and state only after verification evidence exists.

</verification>

<success_criteria>

- `VERIFY-01`: automated tests cover import, Prepare readiness, slice, Preview interaction, export, and output validation.
- `VERIFY-02`: STL and 3MF have real fixture coverage; OBJ/AMF/STEP are tested or explicitly classified.
- `VERIFY-03`: QML/source audits prevent Preview normal-path `SoftwareViewport` usage and catch binding regressions for main workflow controls.
- `VERIFY-04`: runtime diagnostics record renderer backend, workflow state transitions, Preview payload/range, and export outcomes.
- `VERIFY-05`: manual UAT checklist matches and verifies the full local workflow.
- No P0/P1 Preview disappearing, stale export, silent fallback, or missing-output issue remains open.
- v3.4 can be marked complete or sent to milestone audit with clear evidence.

</success_criteria>
