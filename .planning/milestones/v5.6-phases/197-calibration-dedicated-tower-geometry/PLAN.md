# Phase 197 Plan: Calibration Dedicated Tower Geometry

**Requirement:** CALIB-04 (source-truth geometry migration)
**Goal:** Replace the current-plate geometry that the four tower calibration
modes slice with the dedicated upstream test-tower models, so the sliced G-code
matches upstream OrcaSlicer output for TempTower / Vol_speed / VFA / Retraction.

## Background / source truth

Upstream `Plater::calib_temp` / `calib_max_vol_speed` / `calib_VFA` /
`calib_retraction` (Plater.cpp:9797-10003) each call `new_project()` then
`add_model(<resources>/calib/<mode>/<tower>.{stl,step})` before applying
per-mode config overrides and `set_calib_params`. The Qt6 port instead sliced
the current-plate geometry via `SliceService::startSlice` -> `cloneCurrentPlateModel()`
(SliceService.cpp:394). The G-code parameter sweep (temp/speed/retraction
injection in GCode.cpp:4608-4622) is mode-driven and tower-shape-agnostic, so it
worked, but the precision geometry differed from upstream. This was documented
as tech-debt at CalibrationServiceMock.cpp:46-51 (with the inaccurate `.drc`
term; the real files are `.stl`/`.3mf`/`.step`).

## Files

- Modify: `src/core/services/CalibrationServiceMock.cpp`
  - Fix the tech-debt comment term `.drc` -> `.stl/.3mf/.step` and note Phase 197.
  - Add `#include "ProjectServiceMock.h"`, `<QTemporaryFile>`, `<QFileInfo>`.
  - In `startCalibration`: for tower modes (6/7/8/9) with a ProjectServiceMock
    injected, extract the bundled tower qrc to a temp file and `loadFile()` it
    onto the current plate before slicing; defer `setCalibParams`/`startSlice`
    to a one-shot `loadFinished` lambda (loadFile is async). Fall back to the
    legacy current-plate geometry path when extraction fails or no
    ProjectServiceMock is wired.
  - Add `setProjectService(ProjectServiceMock*)`.
  - Add static helpers `towerModelQrcPathForMode(int)` and
    `extractQrcToTempFile(const QString&)`.
  - In `cancelCalibration`: drop the pending tower load connection + flag.
- Modify: `src/core/services/CalibrationServiceMock.h`
  - Forward-declare `ProjectServiceMock`.
  - Add `setProjectService()` injector, the two static helpers, the
    `m_projectService` + `m_pendingCalibTowerLoad` members.
- Modify: `src/qml_gui/BackendContext.cpp`
  - Wire `calibrationService_->setProjectService(projectService_)`.
- Modify: `src/qml_gui/qml.qrc`
  - Register the 7 calib model assets under `assets/calib/` in the `/qml`
    qresource prefix.
- Add: `src/qml_gui/assets/calib/temperature_tower.stl`
- Add: `src/qml_gui/assets/calib/VFA.stl`
- Add: `src/qml_gui/assets/calib/retraction_tower.stl`
- Add: `src/qml_gui/assets/calib/SpeedTestStructure.step`
- Add: `src/qml_gui/assets/calib/pressure_advance_test.stl`
- Add: `src/qml_gui/assets/calib/flowrate-test-pass1.3mf`
- Add: `src/qml_gui/assets/calib/flowrate-test-pass2.3mf`

## Steps

- [x] Fix `.drc` -> `.stl/.3mf/.step` in the CalibrationServiceMock.cpp tech-debt
      comment (only the source comment; historical phase docs left untouched as
      evidence).
- [x] Bundle the 7 upstream calib models under `src/qml_gui/assets/calib/`
      (copied verbatim from `D:/work/OrcaSlicer/resources/calib/`).
- [x] Add `towerModelQrcPathForMode(int)`: maps CalibMode 6/7/8/9 to the
      `:/qml/assets/calib/<tower>` qrc path; returns empty for PA(1)/FlowRate(5)
      which keep the current-plate model (upstream generates their geometry
      in-code via separate wizard paths).
- [x] Add `extractQrcToTempFile(const QString&)`: libslic3r's
      `Model::read_from_file` uses plain filesystem I/O (boost::nowide), not Qt's
      virtual qrc FS, so materialize the resource into a temp file preserving the
      extension (load_step keys off `.step` suffix, Model.cpp:213-215).
      `autoRemove=false` so the file survives until read_from_file runs.
- [x] Wire the dedicated-tower load into `startCalibration`: one-shot
      `loadFinished` lambda captures the calib params + projectName and runs
      `setCalibParams` + `startSlice` after the tower model lands on the plate.
- [x] Add fallback: if ProjectServiceMock is null, qrc path empty, or extraction
      fails, slice the current-plate geometry as before (no regression).
- [x] Clear the pending tower load on `cancelCalibration`.
- [x] Register the 7 assets in `qml.qrc` under the `/qml` prefix.
- [x] Inject ProjectServiceMock in BackendContext.
- [x] Bracket-balance check on all modified files.

## Key design decisions

1. **Geometry replacement replaces the user's plate model** (source-truth). This
   mirrors upstream `new_project()` + `add_model()` -- the tower model is loaded
   onto the current plate, and `cloneCurrentPlateModel()` (called inside
   `SliceService::startSlice`) then picks up the tower. The user's previous model
   is replaced for the calibration slice, exactly as upstream replaces it. This
   is intentional and documented; no save/restore of the user's model is
   performed (upstream does not either).

2. **Async loadFile -> deferred slice.** `ProjectServiceMock::loadFile` runs on
   `QtConcurrent::run` and emits `loadFinished(success, msg)` on completion. The
   slice cannot start before the model is resident, so `setCalibParams` +
   `startSlice` are deferred to a one-shot lambda connected to `loadFinished`.
   The connection disconnects itself after firing so a later user-initiated
   `loadFile` is not intercepted.

3. **qrc extraction, not direct qrc read.** `Model::read_from_file` cannot read
   `:/...` qrc paths, so bundled models are written to a temp file at slice time.
   `autoRemove=false` keeps the file alive; the OS temp dir handles cleanup. This
   keeps the binary self-contained (no external `resources/calib/` dependency at
   runtime) while honoring libslic3r's filesystem-only loader.

4. **Minimal scope: geometry only.** Per-mode object-level config overrides
   (upstream applies `wall_loops`, `spiral_mode`, `brim_width`,
   `top_shell_layers`, etc. on `model().objects[0]->config`) are NOT applied in
   this phase. The existing G-code parameter sweep works without them; adding
   object-level config mutation requires a ProjectServiceMock API to edit a loaded
   object's config and is deferred to a follow-up. Documented below as a known
   limitation -- the sliced tower is geometrically correct but uses the user's
   global print settings rather than upstream's per-calibration overrides.

5. **PA(1) and FlowRate(5) unchanged.** Upstream generates their geometry in-code
   (pa_pattern.3mf / flowrate-test-pass*.3mf via dedicated wizard paths), so they
   keep using the current-plate model. The bundled `pressure_advance_test.stl`
   and `flowrate-test-pass{1,2}.3mf` are registered as resources for a future
   PA/FlowRate dedicated-model migration but are not yet wired into the slice
   path.

## Known limitations / deferred

- **Per-mode object config overrides not applied** (wall_loops, spiral_mode,
  brim, top/bottom_shell_layers per upstream Plater.cpp:9805-9988). The tower
  slices with the user's current global print config. This affects brim/wall
  count precision but not the parameter sweep itself. Follow-up phase needed with
  a ProjectServiceMock object-config-edit API.
- **No `cut_horizontal` of the tower** to the user's start/end range. Upstream
  cuts the temperature/speed/retraction towers to the requested range
  (Plater.cpp:9822-9840, 9904-9908, 9953-9958, 9996-10000). Qt6 slices the full
  tower; the G-code sweep still iterates the configured range. The unused tower
  bands print as plain geometry (harmless but wasteful). Follow-up with a
  cut_horizontal ProjectServiceMock API.
- **Temp file cleanup.** Extracted tower files persist in the OS temp dir until
  the system cleans it. Acceptable for calibration runs; a cleanup hook could be
  added post-slice.

## Verify

- [ ] Build with `HAS_LIBSLIC3R`: CalibrationServiceMock compiles with the new
      ProjectServiceMock include + lambda.
- [ ] Manual: start a TempTower calibration with a clean plate; the
      `temperature_tower.stl` loads and slices, G-code contains the temperature
      sweep markers.
- [ ] Manual: same for VFA / Retraction / Vol_speed.
- [ ] Manual: PA and FlowRate still slice the current-plate geometry (no tower
      loaded).
- [ ] Manual: cancel during tower load does not start a stray slice.
- [ ] Fallback: with no ProjectServiceMock injected (e.g. unit test), tower
      modes slice the current plate as before (no crash).
