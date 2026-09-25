# Phase 232 Summary — AdvancedCut Execution And Multi-Slot Filaments

**Completed:** 2026-08-15
**Requirements:** CIRC-03, CIRC-04 — done.
**Verification:** canonical `auto_verify_with_vcvars.ps1` exit 0 (all suites).
One transient ViewModelSmokeTests crash (testTabPositionEnumValues, 0xc0000005
in BackendContext/PresetServiceMock) matched the known stale-object-ABI
incident (`.planning/debug/preset-service-vendor-crash.md`): after the
ConfigViewModel.h member change, Ninja left stale owzx_app_core objects.
Purged the affected object dirs (owzx_app_core/owzx_cli_core/OWzxSlicer/
ViewModelSmokeTests/QmlUiAuditTests/CliTests/E2EWorkflowTests/owzx-cli) and
rebuilt — all green.

## Changes

1. **CIRC-03 AdvancedCut panel rework** (`PreparePage.qml`):
   - Execute button now calls the real executor `advCutSelected()` (was plain
     `cutSelected()` — groove/connector cuts were unreachable).
   - Panel binds the correct advCut* properties (advCutAxis/advCutPosition/
     advCutKeepBoth; was wrongly bound to the plane-cut cutAxis/cutPosition/
     cutKeepMode).
   - Added mode selector (planar / tongue-groove via cutMode / discrete
     connectors via advCutConnectors) matching the executor's dispatch,
     groove depth/width spinners (grooveDepth/grooveWidth), connector count
     readout + clear (advancedCutConnectorCount/clearAdvancedCutConnectors).
     Connector click routing stays GizmoAdvancedCut-only (RhiViewport.cpp:752),
     now coherent with the toggle living in this panel.
2. **CIRC-04 per-slot filament presets**:
   - `ConfigViewModel`: new `filamentSlotPresets_` list + Q_INVOKABLEs
     `filamentPresetForSlot(slot)`, `requestFilamentPresetForSlot(slot, name)`
     (dirty-guarded; slot 0 routes through the global switch path; emits
     sliceAffectingConfigChanged on real change), `isFilamentCompatibleForSlot(
     slot)`. Upstream `filament_presets` vector semantics.
   - `LeftSidebar.qml` slot rows: combo shows/edits ITS OWN slot preset (was:
     every slot drove the single global preset and displayed the category
     list's Nth entry); compat red dot + row border use per-slot compat;
     swatch color now from configured filament colours (editorVm.extrudersColours)
     with theme-palette fallback.
3. Test: `filamentSlotPresetsAreIndependent` (ViewModelSmokeTests) — slot
   independence, fallback, slice-invalidation signal, per-slot compat, slot-0
   global routing.

## Evidence

Canonical run 2026-08-15 02:28: all 8 suites passed, APP_RUNNING_PID=33556.
