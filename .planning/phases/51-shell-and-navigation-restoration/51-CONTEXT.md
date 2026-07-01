# Phase 51: Shell and Navigation Restoration - Context

**Gathered:** 2026-07-01
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 51 restores the application-level shell and workflow actions so Prepare,
Preview, and settings sit inside the right OrcaSlicer-like frame. Scope is
shell-only: the top bar (`BBLTopbar.qml` + `main.qml` chrome), page
navigation (`StackLayout` + `BackendContext` navigation API), shell-level
action states (enablement/loading/error/blocked), and notification/error
placement.

Out of scope: left sidebar (Phase 52), viewport/object/plate/gizmo workflow
(Phase 53), Preview page internals (Phase 54/55), settings dialogs (Phase
56), and page-level deprecated-UI removal (Phase 57). Phase 51 touches the
shell only.

</domain>

<decisions>
## Implementation Decisions

### Top Bar Component Reconciliation
- `BBLTopbar.qml` (725 lines) is the live PREP-TOP Qt target. The inventory's
  `docs/v3.6-ui-inventory.md` PREP-TOP row currently names the stale,
  unused `components/Toolbar.qml`. Update the inventory's `qt_target` cell to
  cite `BBLTopbar.qml` (primary) and remove the stale `Toolbar.qml`
  reference. Mark `Toolbar.qml` for Phase 57 removal in the inventory §7
  cleanup checklist.
- Add shell-level action gate properties on `BackendContext` that aggregate
  from EditorViewModel/PreviewViewModel: `canImport`, `canSlice`,
  `isSlicing`, `canExport`, `canSave`, `canUndo`, `canRedo`, `isBusy`. QML
  binds to these single properties so SHELL-03 ("driven by C++ state, not
  QML-only conditions") is satisfied. The BackendContext gates forward to
  the underlying viewmodel properties (e.g. `canSlice` reads
  `editorViewModel.canRequestSlice`).
- Bind Undo/Redo `enabled` in BBLTopbar to BOTH the page gate
  (`currentPage === tp3DEditor`) AND the shell-level `canUndo`/`canRedo`
  (which forward to `editorViewModel.canUndo`/`canRedo`). Today they are
  page-gated only, which lets Undo be clickable when the stack is empty.

### Page Navigation and State Preservation
- Keep the existing navigation mechanism: `StackLayout` with `currentIndex`
  bound to `backend.currentPage` (int, mirrors upstream
  `MainFrame::TabPosition` enum 0-8), driven by `requestSelectTab(int)`.
  This is upstream-aligned (ARCH-01/05/06/07). Modify in place; do not
  replace with StackView/Loader-switch or string-based routes.
- Keep the Prepare/Preview slot-sharing via the shared `Plater` instance and
  `currentViewMode` (ARCH-05). Prepare slot 1 + Preview slot 2 both resolve
  to the same Plater; `currentViewMode` (View3D/Preview/AssembleView)
  switches the visible page. This preserves Prepare state when switching to
  Preview.
- SHELL-02 state preservation: verify (and add tests asserting) that the
  following survive a Prepare→Preview→Prepare round-trip: (a) Prepare
  object/plate/selection state (lives in ProjectServiceMock +
  EditorViewModel, page-independent), (b) Preview layer/move/camera state
  (lives in PreviewViewModel), (c) sidebar collapse state, (d) active gizmo
  state. If any of these reset on switch, fix the reset.
- The 4 orphaned pages (ModelMallPage, PreferencesPage, DeviceListPage,
  ConfigPage) exist in `qml.qrc` but are not mounted in the StackLayout and
  not flagged in inventory §7. Document them as legacy in the phase
  SUMMARY and defer their removal to Phase 57 (Deprecated UI Removal) for
  consistency with the "no deprecated UI" rule's dedicated cleanup phase.
  Phase 51 does NOT delete them.

### Action States and Workflow Wiring (SHELL-03/04)
- All 8 shell actions get explicit C++ enabled/disabled/loading/error/blocked
  states: Import, Slice, Preview, Export, Save, Undo, Redo, Settings. Each
  exposes a gate property (bool) on BackendContext and, where the action can
  be blocked for a reason, a human-readable blocked-reason string (extend
  the existing `sliceActionLabel`/`sliceReadinessReason` pattern to
  `exportActionLabel`/`exportActionHint`, `saveActionLabel`/`saveActionHint`
  where the label depends on dirty/hasResult/multi-plate state).
- Loading state: when `isSlicing` is true, disable Import, Slice, Preview,
  Export, AND Save (do not mutate the project mid-slice). Progress shows in
  StatusBar (already wired via SliceService). No global modal spinner.
- Notification/error placement: keep all 3 existing placements — ErrorBanner
  (inline push-down between top bar and content, no overlap),
  ErrorToast (floating centered overlay, z=200), NotificationCenter
  (top-right popup under the title bar). They are already non-overlapping
  with viewport/sidebar. Add a guard test asserting ErrorBanner does not
  overlap the viewport and ErrorToast/NotificationCenter anchoring does not
  cover the left sidebar or the slice/print dropdowns.
- Action labels are driven from C++ viewmodel (not hardcoded in QML). The
  slice label already does this; extend to export/save where state-dependent.

### Legacy Route Removal and Scope Boundary
- Phase 51 removes ONLY shell-level legacy that conflicts with the restored
  workflow: delete the unused `components/Toolbar.qml` and its `qml.qrc`
  entry, and update the inventory PREP-TOP `qt_target` cell. All page-level
  removals (ConfigPage, ModelMallPage, ParamsPage, SettingsPage embedding,
  etc.) are deferred to Phase 57 per inventory §7.
- Phase 51 adds NO new product behavior. Only restore screenshot-visible
  actions (file menu, save/undo/redo, page tabs 准备/预览/…, more-menu,
  window controls, slice/print dropdowns). No "refresh", "sync", or other
  new actions — source-truth rule.
- Scope boundary: Phase 51 is shell-only. It does NOT touch the left
  sidebar (Phase 52), viewport/object/plate/gizmo workflow (Phase 53),
  Preview internals (Phase 54/55), or settings dialogs (Phase 56).
- Verification: automated — QML route/resource registration test (every
  routed page resolves, no dangling qrc entries for removed Toolbar.qml),
  C++ viewmodel state test (gate properties emit correctly, undo/redo gate
  works), build via `scripts/auto_verify_with_vcvars.ps1`. Manual visual UAT
  against the PREP-TOP screenshot region is deferred to Phase 58.

### Claude's Discretion
- Exact property names for the new BackendContext shell gates (canImport
  etc. are the recommended names; small variations acceptable if they match
  existing naming).
- Whether to consolidate the undo/redo page-gate into the shell-level
  canUndo/canRedo (so QML binds one property) or keep both checks in QML.
- Test file placement and naming (ViewModelSmokeTests.cpp extension vs a
  new ShellStateTests.cpp).
- Whether the blocked-reason strings get i18n keys immediately or plain
  English first (i18n coverage beyond active-workflow strings is a Future
  item per PROJECT.md).

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/qml_gui/main.qml` — ApplicationWindow, BBLTopbar host, StackLayout
  (currentIndex bound to backend.currentPage), StatusBar, FileDialogs,
  Shortcut accelerators, Slice/Print CxMenu popups. Navigation is solid and
  upstream-aligned.
- `src/qml_gui/BBLTopbar.qml` (725 lines) — the real top bar: Save,
  Undo/Redo (page-gated only — gap), Calibration, Account/ModelStore/Publish
  (visible:false placeholders), Slice dropdown, Print dropdown, bell
  (NotificationCenter), window controls. File/Edit/View/Help/Preferences/
  Calibration menus live here.
- `src/qml_gui/BackendContext.h/.cpp` — composition root. Owns
  `currentPage` (int, default tp3DEditor=1), `currentViewMode`,
  `requestSelectTab(int)`, `requestChangeViewMode(int)`, `setCurrentPage`,
  topbar I/O methods (topbarNewProject/Open/Import/Save/SaveAs),
  `canLeaveSettingsPage()`. TabPosition enum 0-8 mirrors upstream MainFrame.
  NO shell-level action gates today — these live on EditorViewModel.
- `src/core/viewmodels/EditorViewModel.h/.cpp` — owns `canUndo`, `canRedo`,
  `canRequestSlice`, `canPreview`, `canExportGCode`, `hasSliceResult`,
  `isSlicing()`, `hasSelection`, `hasClipboardContent`, `sliceActionLabel`,
  `sliceReadinessReason`, `previewActionHint`, `exportActionHint`,
  `switchToPreview()`. These are the source-of-truth states the new
  BackendContext shell gates will forward to.
- `src/qml_gui/components/ErrorBanner.qml` — inline push-down (Layout.
  fillWidth, 36px visible / 0 hidden). Non-overlapping. Severity 1.
- `src/qml_gui/components/ErrorToast.qml` — floating centered overlay,
  anchors.bottom, z=200. Severity 0 (info/success).
- `src/qml_gui/components/NotificationCenter.qml` — top-right Popup (x:
  width-340, y:44, 320x420). Opens on bell click.
- `tests/ViewModelSmokeTests.cpp` — existing QtTest smoke tests using
  QSignalSpy. Extend this (or add ShellStateTests.cpp) for the new gate
  properties.

### Established Patterns
- Q_PROPERTY with READ + NOTIFY on BackendContext; writable add WRITE.
- Single `stateChanged()` signal for bulk refresh; specific signals for
  independent state (`currentPageChanged`, `currentViewModeChanged`).
- Q_INVOKABLE actions as verb phrases (`requestSelectTab`,
  `topbarSaveProject`).
- `#ifdef HAS_LIBSLIC3R` guards for real backend integration.
- EditorViewModel→page-switch path: `switchToPreview()` emits
  `previewRequested`, consumed in PreparePage.qml:2935 →
  `backend.setCurrentPage(2)`.

### Integration Points
- New BackendContext shell gates (canImport/canSlice/isSlicing/canExport/
  canSave/canUndo/canRedo/isBusy) will be Q_PROPERTY on BackendContext,
  forwarding to EditorViewModel/PreviewViewModel properties, emitting
  `stateChanged()` on change.
- BBLTopbar.qml Undo/Redo/Slice/Print/Import/Export/Save bindings update
  from QML-only conditions to BackendContext gate properties.
- `components/Toolbar.qml` + its `qml.qrc` entry removed.
- `docs/v3.6-ui-inventory.md` PREP-TOP row `qt_target` cell updated to cite
  `BBLTopbar.qml`; §7 cleanup checklist gets a `file:` line for
  `components/Toolbar.qml`.
- Upstream source truth for PREP-TOP: `third_party/OrcaSlicer/src/slic3r/
  GUI/MainFrame.cpp/hpp` (TabPosition enum, menu bar, tab notebook) +
  `GUI_App.cpp/hpp` (app-level menu wiring) + `BBLTopbar.cpp`
  (title-strip chrome, CUSTOM_ID enum).

</code_context>

<specifics>
## Specific Ideas

- The inventory's PREP-TOP row must be reconciled with reality: the named
  `components/Toolbar.qml` is unused; `BBLTopbar.qml` is the live component.
  This is a factual correction, not a re-decision — update the inventory
  cell to match the codebase.
- Undo/Redo being clickable when the undo stack is empty (page-gate only)
  is a real UX bug visible in the screenshot contract; fix it by adding the
  canUndo/canRedo gate.
- The `isSlicing` state must disable Save too (not just Slice/Preview/
  Export) — mutating the project mid-slice is unsafe.
- Notification placement is already correct (non-overlapping); Phase 51's
  job is to verify and add a guard test, not to redesign placement.

</specifics>

<deferred>
## Deferred Ideas

- Removal of orphaned pages (ModelMallPage, PreferencesPage, DeviceListPage,
  ConfigPage) — deferred to Phase 57 (Deprecated UI Removal) per inventory
  §7 and the "no deprecated UI" rule's dedicated cleanup phase.
- Full i18n key coverage for the new blocked-reason strings — PROJECT.md
  lists "full i18n translation coverage beyond strings touched by active
  workflows" as a Future item. Use plain English first; add qsTr() keys for
  the active-workflow strings only.
- Refactoring navigation to string-based routes or StackView — not needed;
  the int-indexed StackLayout is upstream-aligned and works.
- A dedicated "shell viewmodel" — not needed; BackendContext is already the
  shell-state owner and is the composition root. Adding shell gates there
  is consistent with the existing architecture.

</deferred>
