# OWzx Slicer CLI Fixture Recipes

Canonical argv combos (`OWzxSlicer.exe <flags>`) that reach each major GUI
state. These are **OWzx-only test-evidence plumbing** (FIXTURE-04 / FIXTURE-03):
upstream OrcaSlicer has no equivalent argv surface (upstream argv is CLI-only
`--load` / `--slice` / positional at `OrcaSlicer.cpp:7183`). They exist so visual
verification can reach a target state without simulated clicks, and are gated on
`QQmlApplicationEngine::objectCreated` + `QQuickWindow::frameSwapped` (the
FIXTURE-02 deterministic-readiness gate landed in Phase 103) so screenshots are
taken after at least one rendered frame. Do NOT promote these to a user-facing
deep-link product feature (anti-feature per REQUIREMENTS.md).

## Flags

| Flag | Value | Effect |
|------|-------|--------|
| `--open-page <page>` | page alias | Switches the top-level tab after QML startup. |
| `--open-dialog <dialog>` | dialog route (repeatable) | Opens a named dialog after QML startup. |
| `--load-model <path>` | file path (repeatable) | Loads a model file after QML startup. |
| `--skip-first-run` | (none) | Suppresses the first-run config wizard for this launch. |

### Page aliases (from `startupPageRoutes` in `main_qml.cpp`)

`home` | `prepare` (alias `3d`/`editor`/`plater`) | `preview` | `device` (alias
`monitor`) | `multi-device` (alias `multi`) | `project` | `calibration` (alias
`calibrate`).

### Dialog routes (from `startupDialogRoutes` in `main_qml.cpp`)

`settings:printer` (alias `printer-settings`) | `settings:filament` (alias
`settings:material` / `filament-settings` / `material-settings`) |
`settings:process` (alias `settings:print` / `process-settings` /
`print-settings`) | `config-wizard` (alias `wizard`) | `bed-shape` (alias
`bed`) | `ams-settings` (alias `ams`) | `firmware` | `speed-limit` |
`wipe-tower` | `print-host` | `plugin-manager` (alias `plugins`) | `lite-mode`
(alias `enable-lite-mode`).

## Recipes (one-liner per major GUI state)

Each recipe is executable as `OWzxSlicer.exe <flags>` from the build directory.

### Prepare (empty plate)
```
OWzxSlicer.exe --skip-first-run --open-page prepare
```
Opens directly on the Prepare tab with an empty plate and no model loaded. Use
this as the baseline for the Prepare chrome (top bar, sidebars, empty-plate
prompt).

### Prepare (with the multi-material fixture)
```
OWzxSlicer.exe --skip-first-run --load-model tests/data/multi_material_fixture.3mf --open-page prepare
```
Loads the 2-extruder fixture (`tests/data/multi_material_fixture.3mf`,
FIXTURE-01) onto the Prepare plate. Use this to exercise multi-material-
dependent surfaces (filament slots, wipe-tower preview, per-extruder coloring).

### Prepare (with the single-material fixture)
```
OWzxSlicer.exe --skip-first-run --load-model tests/data/test_model.stl --open-page prepare
```
Loads the existing single-material STL (`tests/data/test_model.stl`) onto the
Prepare plate. Use this for the single-material Prepare baseline.

### Preview
```
OWzxSlicer.exe --skip-first-run --open-page preview
```
Opens directly on the Preview tab (G-code layer view, no slice result until a
slice is run). Use this to inspect the Preview chrome and empty-state controls.

### AssembleView
```
OWzxSlicer.exe --skip-first-run --load-model tests/data/multi_material_fixture.3mf --open-page prepare
```
Then toggle the AssembleView canvas from the Prepare page (the AssembleView
canvas is a Prepare-page canvas-type toggle, not a separate top-level page;
see `CanvasAssembleView` in `RhiViewport.h`). Loading the 2-extruder fixture
first gives the assemble/explode/measurement gizmos >=2 volumes to operate on.

### settings:printer
```
OWzxSlicer.exe --skip-first-run --open-dialog settings:printer
```
Opens the Printer Settings dialog after QML startup (stays on the home page
otherwise). Use this to capture the settings dialog chrome. Repeat the flag
for multiple dialogs (e.g. `--open-dialog settings:printer --open-dialog
settings:filament`).

### settings:filament
```
OWzxSlicer.exe --skip-first-run --open-dialog settings:filament
```
Opens the Filament Settings dialog after QML startup.

### settings:process
```
OWzxSlicer.exe --skip-first-run --open-dialog settings:process
```
Opens the Process (Print) Settings dialog after QML startup.

### calibration
```
OWzxSlicer.exe --skip-first-run --open-page calibration
```
Opens directly on the Calibration tab.

### wipe-tower dialog
```
OWzxSlicer.exe --skip-first-run --open-dialog wipe-tower
```
Opens the Wipe-Tower Settings dialog after QML startup (useful for verifying
the v4.4 Option A dimensioned-box baseline parameters surface).

### config-wizard (without --skip-first-run)
```
OWzxSlicer.exe --open-page prepare
```
Omitting `--skip-first-run` lets the first-run config wizard fire (the wizard
is the default state for a fresh launch). Combine with `--open-dialog
config-wizard` to force the wizard without clearing QSettings.

## Determinism notes

- All recipes rely on the Phase 103 FIXTURE-02 gate: the page/dialog/load
  requests are applied only after `QQuickWindow::frameSwapped` fires once,
  guaranteeing the scene graph rendered a frame before capture.
- The `--skip-first-run` flag suppresses the wizard for this process only (no
  QSettings persistence) so recipes are reproducible across runs.
- Multiple `--load-model` / `--open-dialog` flags accumulate (the parser uses
  `parser.values(...)`, not `parser.value(...)`).

## Source fixtures

| File | Material | Origin |
|------|----------|--------|
| `tests/data/test_model.stl` | single-material | pre-existing (single-material baseline). |
| `tests/data/multi_material_fixture.3mf` | 2-extruder (FIXTURE-01) | hand-authored (no suitable multi-material 3MF exists in `third_party/OrcaSlicer/tests/data/`; the single `Buechse.3mf` sample is single-material). Two 20mm cubes, one per extruder, with `slic3r:extruder` metadata pinning each cube to extruder 0 / 1. |
