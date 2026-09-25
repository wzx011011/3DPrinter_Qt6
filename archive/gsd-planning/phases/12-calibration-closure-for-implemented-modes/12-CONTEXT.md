# Phase 12: Calibration Closure for Implemented Modes - Context

**Gathered:** 2026-06-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 12 turns the existing hybrid calibration implementation into visible, deterministic, verified behavior for the modes that already have a real `SliceService::setCalibParams()` path: Flow Dynamics / PA line, Flow Rate, and Temp Tower. It must not invent new calibration algorithms or treat hardware-only/mock-only calibration entries as complete.

</domain>

<decisions>
## Implementation Decisions

### Implemented Mode Scope
- Treat PA line, Flow Rate, and Temp Tower as the only Phase 12 implemented calibration modes because `CalibrationServiceMock::startCalibration()` and `SliceService::setCalibParams()` already map these to `CalibMode` values.
- Mark Bed Leveling, Vibration Compensation, Max Volumetric Speed, and other unimplemented or hardware-dependent modes as Pending or Blocked in UI/planning rather than silently routing them through mock success.
- Preserve OrcaSlicer source-truth mapping in comments and verification: `BBLTopbar::GetCalibMenu`, `CalibrationPanel`, `CalibrationWizard`, and `Plater::calib_*`.
- Do not modify libslic3r slicing algorithms; use the existing `Print::set_calib_params` / G-code export path.

### Visible Entry Behavior
- The topbar calibration menu should expose actionable entries for implemented modes and route them to the Calibration page with the matching selected item.
- The selected implemented item should be startable from the page/dialog without requiring hidden backend calls.
- Unimplemented entries should remain visible only when clearly labelled Pending or Blocked, or be hidden from the implemented-mode action path.
- QML may select the target item and forward actions, but durable mode classification and startability should live in C++ viewmodel/service APIs.

### Regression Coverage
- Add deterministic tests for mode mapping and slice request dispatch without requiring a full calibration G-code export for every mode.
- Keep the existing environment-dependent PA G-code generation test as deeper evidence, but do not make Phase 12 depend solely on long-running real slicing for all modes.
- Separately verify mock fallback: when no real `SliceService` job is dispatched, the timer fallback still advances and completes.
- Verify real progress callback behavior: when `SliceService` emits progress/finish/failure, calibration progress/status follows it instead of the timer.

### Status and Planning Truth
- Complete only CAL-01..CAL-05 when implemented entries are visible, tested, and classified.
- Record unimplemented calibration modes as Pending/Blocked with upstream references so broad calibration UI presence is not counted as completion.
- Leave broad visible-placeholder cleanup outside calibration for Phase 14.
- Keep all full verification claims tied to the canonical PowerShell command and `build/`.

### the agent's Discretion
- Choose the smallest C++ API that lets QML route by stable calibration id or known implemented mode without embedding business rules in QML.
- Prefer focused smoke tests in `ViewModelSmokeTests.cpp` unless a new smaller Qt test target is clearly cleaner.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/core/services/CalibrationServiceMock.*` owns calibration item metadata, fallback timer, SliceService injection, status/progress transitions, and history.
- `src/core/viewmodels/CalibrationViewModel.*` exposes selected item state and QML-invokable calibration actions.
- `src/core/services/SliceService.*` has `setCalibParams()` and resets calibration params after slicing.
- `src/qml_gui/BBLTopbar.qml`, `src/qml_gui/pages/CalibrationPage.qml`, and `src/qml_gui/dialogs/CalibrationDialog.qml` already provide the visible calibration surfaces.
- `tests/ViewModelSmokeTests.cpp` already contains calibration-related coverage and is the natural place for narrow viewmodel/service regressions.

### Established Patterns
- Backend behavior belongs in C++ services/viewmodels; QML forwards actions and binds properties.
- ViewModels expose `Q_PROPERTY` and `Q_INVOKABLE` APIs; services own mutable workflow state.
- Existing tests use `QSignalSpy` and direct service/viewmodel instantiation for deterministic coverage.
- The project uses source-truth comments referencing upstream files for migrated behavior.

### Integration Points
- `BackendContext` wires `CalibrationServiceMock`, `SliceService`, and `CalibrationViewModel`.
- Topbar calibration actions currently select `backend.calibrationViewModel.selectedIndex` and call `calibrationRequested()`.
- `CalibrationPage` starts calibration via `CalibrationDialog`, which calls `calibrationVm.startCalibration()`.
- Upstream references: `third_party/OrcaSlicer/src/slic3r/GUI/BBLTopbar.cpp`, `CalibrationPanel.cpp`, `CalibrationWizard.cpp`, and `Plater.cpp`.

</code_context>

<specifics>
## Specific Ideas

Autonomous default selected: close PA/Flow/Temp first, classify the rest explicitly, and do not expand this phase into a full calibration-center rewrite.

</specifics>

<deferred>
## Deferred Ideas

- Full hardware calibration integration requiring live printer/device command paths belongs to a future device-integration phase.
- Full upstream calibration wizard parity for all Orca modes belongs to a later source-truth calibration milestone.
- Non-calibration visible placeholder cleanup remains Phase 14 scope.

</deferred>
