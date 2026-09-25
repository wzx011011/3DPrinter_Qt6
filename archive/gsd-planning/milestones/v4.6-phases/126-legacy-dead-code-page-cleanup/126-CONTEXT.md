# Phase 126: Legacy Dead-Code Page Cleanup - Context

**Gathered:** 2026-07-15
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
## Phase Boundary

Close CLEAN-01: remove/repair legacy dead-code pages. **Research corrected the REQUIREMENTS wording**: these pages are NOT "bugs to fix" — they are dead/orphaned code in the already-removed LAN/device scope. The correct action is DELETION, not repair.

</domain>

<decisions>
## Implementation Decisions (from code research)

### Delete, not repair
- **DeviceListPage.qml** — completely dead (zero references, unreachable; the "force-empty bug" is moot since the page is never instantiated; MonitorPage replaced it). DELETE.
- **AuxiliaryPage.qml** — reachable (StackLayout slot 7) but zero function: "analysis tools" UI whose actions all console.log "not yet implemented" or jump to Prepare/Preview; the injected `projectVm` is never read. AuxiliaryService (file-copy) is unrelated to the UI. DELETE + remove the slot 7 route.
- **AuxiliaryListPanel.qml** — orphan panel (zero refs, depends on unexposed `backend.auxiliaryService`). DELETE.
- **ModelMallPage.qml** — dead (not in any route/BBLTopbar tab; only qml.qrc + test refs). DELETE (ModelMall is in the removed cloud scope).
- **ConfigPage** — already deleted (Phase 57-01); only test-file string residuals remain. Optional test cleanup.

### Scope boundary
These are NOT in the active product surface (LAN/device/cloud/ModelMall are removed scope per PROJECT.md). Deleting them removes dead code per the No-Deprecated-UI rule, NOT product-feature removal. The active pages (Home/Prepare/Preview/Monitor/MultiMachine/Project/Calibration/Preferences/Assemble) are untouched.

### AuxiliaryService decision
After deleting AuxiliaryListPanel (its only would-be consumer), AuxiliaryService is unreferenced. Delete it too (BackendContext.cpp:112 instantiation + BackendContext.h member + the .h/.cpp + CMake entry). Confirm no other consumer first.

</decisions>

<specifics>
## Code Access Points
- main.qml:553-564 (StackLayout slot 7 AuxiliaryPage Loader — remove, replace with structural placeholder or remove tpPlaceholder1).
- main.qml:476-571 (full StackLayout — 9 slots).
- BBLTopbar.qml:58-64 + 306-315 (tab lists — may reference tpPlaceholder1; clean up).
- qml.qrc:17 (DeviceListPage), :50 (AuxiliaryListPanel), :19 (ModelMallPage) — remove entries.
- BackendContext.cpp:112 (AuxiliaryService instantiation), BackendContext.h:20/464 (member decl).
- CMakeLists.txt (AuxiliaryService.cpp/.h compile entry — remove if service deleted).
- tests/QmlUiAuditTests.cpp (ModelMallPage ref at :2009; ConfigPage residuals at :3179/3207/3216).

## Verification Anchors
- After deletion: `grep -rn "DeviceListPage\|AuxiliaryPage\|AuxiliaryListPanel\|ModelMallPage\|AuxiliaryService" src/` should return zero (except maybe comments documenting the removal).
- Build must remain clean (no broken imports/routes).

</specifics>

<deferred>
## Deferred Ideas
None — Phase 126 is pure deletion/cleanup.

</deferred>
