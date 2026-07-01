---
phase: 52-prepare-sidebar-and-preset-controls
plan: 02
subsystem: ui
tags: [qt6, qml, sidebar, preset, bindings, scope, search, extruder-count]

# Dependency graph
requires:
  - phase: 52-prepare-sidebar-and-preset-controls (plan 01)
    provides: EditorViewModel.extruderCount Q_PROPERTY + BackendContext.forwardSettingsRequest Q_INVOKABLE + staleness Q_PROPERTYs (stalePlateIndices/hasStaleSliceResults) that this QML wave binds/invokes
provides:
  - LeftSidebar.qml filament Repeater bound dynamically to editorVm.extruderCount (guarded Math.max(1, ...)), replacing the hard-coded model:5
  - FilamentSlot.qml dead color-picker popup hidden (onClicked neutralized, Loader documented inactive) with an English ASCII TODO(Phase 56) -- no silent dead UI
  - Dirty-dot indicators on the printer and process preset rows bound to configVm.isPresetDirty; printer edit button gated by configVm.presetActionBlocker for builtin/read-only presets
  - Setting menu button visible+enabled, emitting backend.forwardSettingsRequest("process"); Advanced/Compare kept hidden with Phase-56 comments
  - Search box wired to configVm.filterOptionIndices on both onAccepted and onTextChanged (live filter)
  - Global/Object/Plate scope triad complete: existing Global/Object toggles verified correct, new Plate button added (requestPlateScope + settingsScope==="plate")
affects: [52-03-sidebar-tests, 56-settings-dialog, 57-deprecated-ui-removal]

# Tech tracking
tech-stack:
  added: []
  patterns:
    - "Dynamic-QML-model binding to a C++ Q_PROPERTY guarded by Math.max(1, ...) so a mock returning 0 never regresses the UI to zero slots"
    - "Honest-deferred-UI pattern: hide dead controls (empty onClicked) + English ASCII TODO(Phase N) rather than leaving silent non-functional UI; vs visible entry points that log a no-op (Setting button) -- the distinction is explicit deferral vs deception"

key-files:
  created: []
  modified:
    - src/qml_gui/panels/LeftSidebar.qml
    - src/qml_gui/components/FilamentSlot.qml

key-decisions:
  - "Repeater model is `Math.max(1, root.editorVm ? root.editorVm.extruderCount : 1)` -- the guard is essential because extruderCount() is a mock returning hasSliceResult()?1:0, so without the guard the sidebar would show 0 slots before a slice exists. Phase 56 wires real PresetBundle nozzle_diameter count."
  - "The color-picker Loader is kept (active:false) rather than deleted -- Phase 56 will repurpose it once real PresetBundle filament_colour metadata exists. Only its activation is neutralized."
  - "Object scope button uses `settingsScope !== \"global\"` (already in the file) rather than `=== \"object\"` -- verified functional, left unchanged; the new Plate button uses `=== \"plate\"` precisely."
  - "presetActionBlocker(2, currentPrinterPreset, \"rename\") is the read-only gate: a non-empty return string = blocked/builtin, which dims the edit button (opacity 0.4) and disables the click."

patterns-established:
  - "Dirty-dot Rectangle bound to configVm.isPresetDirty, mirrored across preset rows (printer + process) with a shared ToolTip"
  - "Read-only/builtin preset gating in QML via configVm.presetActionBlocker(category, presetName, action) !== \"\""
  - "Scope-toggle CxButton pattern: cxStyle binds Primary when settingsScope matches, Ghost otherwise; onClicked calls configVm.request*Scope(...)"

requirements-completed: [PREPSB-01, PREPSB-02, PREPSB-03, PREPSB-04]

# Metrics
duration: 16 min
completed: 2026-07-01
---

# Phase 52 Plan 02: QML LeftSidebar Wiring -- Slot Count, Dirty Dots, Setting Button, Search, Scope Toggles, Popup Hide Summary

**Wired the Prepare left sidebar's six stubbed/dead controls to existing C++ viewmodel state: dynamic extruder-count filament slots, hidden dead color picker, dirty dots + read-only gating, enabled Setting entry point, live search filter, and the complete Global/Object/Plate scope triad -- QML-only, no C++ touched**

## Performance

- **Duration:** ~16 min
- **Started:** 2026-07-01T13:05Z
- **Completed:** 2026-07-01T13:21Z
- **Tasks:** 7 (all completed)
- **Files modified:** 2

## Accomplishments
- PREPSB-01 (slot count parity): Replaced the hard-coded `Repeater { model: 5 }` with `Math.max(1, root.editorVm ? root.editorVm.extruderCount : 1)` so the filament slot count tracks the printer's extruder configuration instead of a fixed 5 (upstream Plater shows one slot per extruder). The `Math.max(1, ...)` guard is essential: `extruderCount()` is a mock returning `hasSliceResult() ? 1 : 0`, so without the guard the sidebar would render 0 slots before a slice exists. Full multi-extruder count resolves in Phase 56 when real PresetBundle nozzle_diameter data is wired.
- PREPSB-01 (honest UI): Hid the dead color-picker popup in FilamentSlot.qml -- the popup was visual-only (selecting a color did nothing). Neutralized the `onClicked` toggle to a no-op, documented the inactive Loader, and added an English ASCII `TODO(Phase 56)` referencing the real PresetBundle `filament_colour` metadata path. The compatibility dot (`isFilamentCompatible`) -- the PREPSB-03 surface -- is preserved untouched.
- PREPSB-03 (preset state indicators): Surfaced dirty-dot Rectangles bound to `configVm.isPresetDirty` on the printer preset row and the process topbar (visible when the active preset has unsaved option edits). Gated the printer edit ("✎") button honestly via `configVm.presetActionBlocker(2, currentPrinterPreset, "rename")` so a builtin/read-only preset renders dimmed (opacity 0.4) and the click is disabled. The compatibility dot in FilamentSlot was verified to render (already bound to `isFilamentCompatible`).
- PREPSB-02 (settings entry point): Made the "Setting ☰" button visible AND enabled, emitting `backend.forwardSettingsRequest("process")` (Plan 52-01's C++ forward -- an honest no-op log until Phase 56 wires the independent dialog). Kept "Advanced" (Simple/Advanced toggle) and "Compare" (DiffPresetDialog) hidden with Phase-56 comments since they are settings-dialog features.
- PREPSB-04 (search): Replaced the empty `onAccepted` stub with a real call to `configVm.filterOptionIndices("", text.trim(), false)` and added an `onTextChanged` handler for live incremental filtering (mirrors upstream SearchCtrl behavior). The third arg `false` = non-advanced mode. Full ParamsPanel option rendering stays Phase 56; this task only makes the search input drive the existing filter API.
- PREPSB-04 (scope): Verified the existing Global and Object CxButtons call `requestGlobalScope()` / `requestObjectScope(...)` and bind `cxStyle` to `settingsScope` (correct -- no change needed). Added the missing Plate scope button to complete the Global/Object/Plate triad: calls `requestPlateScope(currentPlateIndex)`, binds `cxStyle` to `settingsScope === "plate"`, and is enabled only when a plate is selected.
- Build passes via the sanitized-PATH ninja pattern (`ninja owzx_app_core OWzxSlicer`, exit 0; the qml.qrc RCC recompiled cleanly with the modified LeftSidebar.qml / FilamentSlot.qml -- no QML syntax error).

## Task Commits

Each task was committed atomically:

1. **Task 1: bind filament slot count to extruderCount (PREPSB-01)** - `76e4c2b` (feat)
2. **Task 2: hide dead color picker popup in FilamentSlot (PREPSB-01)** - `146e9fc` (feat)
3. **Task 3: surface preset dirty dots + read-only gating (PREPSB-03)** - `f20b3fa` (feat)
4. **Task 4: enable Setting button + mark Advanced/Compare Phase 56 (PREPSB-02)** - `79499b4` (feat)
5. **Task 5: wire search box to filterOptionIndices (PREPSB-04)** - `827cd5e` (feat)
6. **Task 6: add Plate scope toggle + verify Global/Object (PREPSB-04)** - `a8c0150` (feat)
7. **Task 7: build verification (no code change, RCC gate)** - (verification only, no separate commit)

**Plan metadata:** (this SUMMARY commit)

## Files Created/Modified
- `src/qml_gui/panels/LeftSidebar.qml` - (a) filament Repeater model bound to `Math.max(1, editorVm.extruderCount)`; (b) printer + process dirty dots bound to `isPresetDirty`; (c) edit button gated by `presetActionBlocker(2, ...)`; (d) Setting button enabled emitting `forwardSettingsRequest("process")`, Advanced/Compare comments marked Phase 56; (e) search box `onAccepted`/`onTextChanged` calling `filterOptionIndices`; (f) new Plate scope button.
- `src/qml_gui/components/FilamentSlot.qml` - Color-dot `onClicked` neutralized to `{}`; Loader `active: false` documented with a Phase-56 TODO referencing the real `filament_colour` metadata path. Compatibility dot (`isFilamentCompatible`) preserved.

## Decisions Made
- Used `Math.max(1, ...)` rather than a bare `extruderCount` binding because the mock accessor returns 0 before a slice exists -- the guard prevents a 0-slot regression and is documented honestly as resolving fully in Phase 56.
- Kept the color-picker Loader component (did not delete it) so Phase 56 can repurpose it once real color metadata exists; only its activation was neutralized. The CONTEXT explicitly said "do NOT leave silent dead UI" -- the empty onClicked + TODO achieves that without premature deletion.
- The Plate scope button mirrors the existing CxButton pattern (Primary vs Ghost on `settingsScope` match) and reuses `currentPlateIndex` for both the enabled guard and the `requestPlateScope` argument.

## Deviations from Plan

None - plan executed exactly as written. All 6 code tasks met their acceptance criteria on the first implementation; the build (Task 7) passed on the first attempt with no QML/RCC errors.

## Issues Encountered
- **vcvars64.bat environment issue (pre-existing, out of scope):** The canonical `scripts/auto_verify_with_vcvars.ps1` fails because the system PATH contains space-laden `C:\Program Files (x86)\VMware\VMware Workstation\` entries that break vcvars64.bat's batch parsing. This is an environmental issue carried from Phase 51 (documented in 51-03-SUMMARY.md), NOT caused by this plan's QML changes, and is out of scope to fix. Build verification used the established sanitized-PATH ninja pattern instead, which succeeded (RCC recompiled qml.qrc cleanly, OWzxSlicer linked).
- Note: RCC only catches some QML errors at build time (.qml files are embedded but not fully type-checked until runtime). The full QML sidebar binding audit is owned by Plan 52-03's QmlUiAuditTests; this plan verified property names match the C++ viewmodels exactly against the headers.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- The Prepare left sidebar's active controls are now wired to real C++ state. Plan 52-03 (tests) can assert: the filament Repeater binds extruderCount; dirty dots appear when isPresetDirty; the edit button is disabled for builtin presets; the search drives filterOptionIndices; the scope triad drives settingsScope.
- Plan 52-03's QmlUiAuditTests can grep-verify all the bindings landed (model:5 gone, filterOptionIndices present, requestPlateScope present, isPresetDirty >= 2, presetActionBlocker present, colorPickerLoader.onClick empty, TODO(Phase 56) present).
- Deferred to Phase 56: independent settings dialog (destination of forwardSettingsRequest), full ParamsPanel option rendering, full filament color metadata, Advanced/Compare buttons. Deferred to Phase 57: legacy Sidebar.qml / FilamentPanel.qml / PrintSettings.qml removal (untouched here).
- No blockers.

## Self-Check: PASSED
All plan-level `<verification>` commands (1-8) re-run green:
1. Filament slot count dynamic: `model: 5` count = 0; `editorVm.extruderCount` present; `Math.max(1,` present -- PASS
2. Dead color picker hidden: dead-toggle count = 0; `TODO(Phase 56)` >= 1; `active: false` = 1; `isFilamentCompatible` preserved = 1 -- PASS
3. Dirty dots surfaced: `isPresetDirty` count >= 2; `presetActionBlocker(2` >= 1 -- PASS
4. Setting button visible+enabled: "☰" CxButton has `enabled: true`/`visible: true`/`onClicked: backend.forwardSettingsRequest("process")`; Advanced/Compare `visible: false` -- PASS
5. Search box wired: `configVm.filterOptionIndices` count >= 2 (onAccepted + onTextChanged); empty stub gone -- PASS
6. Scope toggles complete: `requestGlobalScope()` >= 1; `requestObjectScope` >= 1; `requestPlateScope` >= 1; `settingsScope === "plate"` present -- PASS
7. Build passes via sanitized PATH: `ninja owzx_app_core OWzxSlicer` exit 0, RCC recompiled clean, no QML syntax error -- PASS
8. Scope fence: `git diff 4d32d95..HEAD` shows ONLY LeftSidebar.qml + FilamentSlot.qml; no .cpp/.h/.qrc/.txt/test/inventory -- PASS

---
*Phase: 52-prepare-sidebar-and-preset-controls*
*Plan: 02*
*Completed: 2026-07-01*
