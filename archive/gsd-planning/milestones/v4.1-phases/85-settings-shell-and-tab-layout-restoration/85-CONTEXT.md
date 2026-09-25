# Phase 85: Settings Shell And Tab Layout Restoration - Context

**Gathered:** 2026-07-07
**Status:** Ready for planning
**Mode:** Autonomous discuss defaults (`--auto`)

<domain>
## Phase Boundary

Phase 85 restores the screenshot-visible settings dialog shell for printer,
material, and process settings. It owns the visible window chrome, fixed/stable
window sizing, top preset/action row, horizontal tab strip, clean user-facing
text, and settings entry behavior.

In scope:

- `SettingsDialog.qml` shell layout, title text, top row, tabs, search/mode
  affordance, visible close behavior, scrollbar framing, and visible text.
- `main.qml` settings dialog dispatch only where needed to preserve the three
  independent non-modal dialog instances.
- `LeftSidebar.qml` settings entry points only where needed to keep
  printer/material/process settings buttons operable and cleanly labeled.
- Source/QML audits that prove Phase 85's visible-shell contract.

Out of scope:

- Option section header and typed-control renderer overhaul; Phase 86 owns
  `SET-SECTIONS`, `SET-TYPED-ROWS`, and most `SET-STATE-INDICATORS` visuals.
- Preset save/reset/dirty/close workflow hardening beyond preserving existing
  Phase 56 wiring; Phase 87 owns semantic hardening.
- Final canonical build, app launch, and screenshot evidence; Phase 88 owns the
  full verification bundle, though Phase 85 may run targeted checks.
- LAN/device/cloud/network, Monitor, ModelMall/Home WebView, camera streams,
  AssembleView, D3D12/Vulkan, and full upstream preset-bundle import/export.

</domain>

<decisions>
## Implementation Decisions

### Window Shell And Native Close

- **D-01:** Preserve `SettingsDialog.qml` as an independent non-modal
  `ApplicationWindow` at 736x593. The fixed target size comes directly from
  `shotScreen/打印机参数设置页.png` and `shotScreen/材料参数设置页.png`.
- **D-02:** Use the native window title/close affordance as the primary close
  UI. The current extra close button inside the top action row is off-design and
  should be removed from the visible row while keeping the dirty-close guard.
- **D-03:** Use clean visible titles: `打印机设置`, `材料设置`, and source-parity
  `工艺设置` for process unless Phase 85 source reads prove a more accurate
  upstream label. Do not preserve mojibake strings.
- **D-04:** Keep the body as a single main content pane with a right vertical
  scrollbar. No visible left group-navigation sidebar belongs in Phase 85.

### Preset And Action Row

- **D-05:** Recompose the top row to match the screenshots: preset combo on the
  left, compact action/icon affordances on the right. Do not keep the current
  always-wide search field plus text-heavy `Save` / `Save As...` row.
- **D-06:** Preserve existing semantics behind the row: preset selection,
  save/save-as trigger, search text, advanced/simple mode, dirty status, and
  compatibility status must keep routing through `ConfigViewModel` and existing
  dialog flows.
- **D-07:** Search should become a compact affordance, such as an icon button
  that opens or reveals search, rather than a permanently wide top-row field.
  The underlying `searchText` and filtering APIs remain the same.
- **D-08:** Advanced/simple mode should be compact and icon/toggle-like in the
  top row. Avoid visible instructional text inside the row unless screenshot or
  upstream evidence requires it.
- **D-09:** Use existing icon assets or existing control primitives where
  possible. Do not introduce custom-drawn one-off icons when a project asset or
  control already exists.

### Tabs And Labels

- **D-10:** Printer tab labels/order must match the target screenshot:
  `基础信息`, `打印机G-code`, `材料`, `挤出机`, `移动能力`, `注释`.
- **D-11:** Material tab labels/order must match the target screenshot:
  `耗材丝`, `冷却`, `参数覆盖`, `高级`, `材料`, `依赖`, `注释`.
- **D-12:** Process tabs use OrcaSlicer source-truth parity and the same
  restored shell; do not invent a process-only visual system.
- **D-13:** Keep stable internal tab keys used by filtering unless a proven
  source-truth mismatch requires changing the C++/QML mapping. Phase 85 should
  prefer visual label cleanup over backend key churn.

### Entry Points And Scope

- **D-14:** Preserve the three existing dialog instances in `main.qml`:
  printer, material/filament, and process/print. `BackendContext` must still set
  the active preset tier before `settingsRequested(category)` opens the dialog.
- **D-15:** Preserve Prepare/sidebar settings buttons as the intended entry
  points. If Phase 85 touches sidebar settings labels/tooltips, clean mojibake
  only for the settings-related visible text it touches.
- **D-16:** Topbar `backend.openSettings()` is not a substitute for the
  parameter settings entry path unless planning proves it should route to a
  specific printer/material/process dialog. Do not broaden Phase 85 into general
  preferences.

### Visual Style

- **D-17:** Match the settings screenshots' dark gray surfaces, compact row
  density, cyan/teal active-tab emphasis, and tight spacing locally in the
  settings dialog. Do not perform a broad global theme overhaul in Phase 85.
- **D-18:** Avoid cards inside cards and marketing-style spacing. The settings
  dialog is a dense operational tool, so rows and tabs should be compact and
  stable.
- **D-19:** All Phase 85 visible strings in restored settings windows must be
  clean UTF-8 and `qsTr()` wrapped where appropriate. Mojibake is a blocker for
  this phase.

### Verification Routing

- **D-20:** Phase 85 should add or update source/QML audit coverage for:
  no visible `GroupNavSidebar` in `SettingsDialog.qml`, clean tab/title labels,
  no text-heavy `Save`/`Save As...` top-row buttons, no duplicate in-row close
  button, and preserved settings dispatch.
- **D-21:** Phase 85 may use static/source checks and targeted Qt tests. Final
  runtime screenshot comparison remains Phase 88's responsibility.

### the agent's Discretion

- The planner may decide whether `SettingsDialog.qml` is patched in place or
  recomposed more substantially, as long as proven Phase 56 semantics remain
  wired and stale/off-design visible shell code is not left reachable.
- The planner may choose exact icon assets and dimensions after inspecting the
  available qrc/assets, but the result must visually read like the screenshots'
  compact action row.
- The planner may defer deeper row/section visual changes to Phase 86 when a
  change belongs to option content rather than the shell/top tabs.

</decisions>

<canonical_refs>
## Canonical References

Downstream agents MUST read these before planning or implementing.

### Planning Scope

- `.planning/PROJECT.md` - v4.1 scope, source-truth rule, screenshot truth,
  removed network/device scope, and settings source-truth candidates.
- `.planning/REQUIREMENTS.md` - `SETLAYOUT-01`, `SETLAYOUT-02`, and
  `SETLAYOUT-03` are Phase 85's requirements.
- `.planning/ROADMAP.md` - Phase 85 goal and success criteria.
- `.planning/STATE.md` - current workflow state and scope guard.
- `.planning/phases/84-settings-source-truth-gap-audit/84-GAP-MATRIX.md` -
  canonical routing source for `SET-SHELL`, `SET-PRESET-ACTIONS`, `SET-TABS`,
  `SET-ENTRYPOINTS`, `SET-SEARCH-MODE`, and `SET-CLEANUP`.

### Visual Truth

- `shotScreen/打印机参数设置页.png` - printer settings visual target.
- `shotScreen/材料参数设置页.png` - material settings visual target.

### Prior Phase Evidence

- `.planning/phases/84-settings-source-truth-gap-audit/84-CONTEXT.md` -
  decisions D-01 through D-18 that Phase 85 must honor.
- `.planning/phases/84-settings-source-truth-gap-audit/84-VERIFICATION.md` -
  confirms Phase 84 audit coverage and Phase 85 ownership.
- `.planning/phases/84-settings-source-truth-gap-audit/84-01-SUMMARY.md` -
  downstream handoff for Phase 85-88.
- `.planning/milestones/v3.6-phases/56-parameter-settings-dialogs-restoration/56-VERIFICATION.md` -
  Phase 56 semantic baseline and deferred visual-UAT items.
- `.planning/milestones/v3.6-phases/56-parameter-settings-dialogs-restoration/56-VALIDATION.md` -
  visual/manual rows that v4.1 reopens.

### Current Qt Targets

- `src/qml_gui/dialogs/SettingsDialog.qml` - primary Phase 85 target.
- `src/qml_gui/main.qml` - dialog instances and `settingsRequested` dispatch.
- `src/qml_gui/panels/LeftSidebar.qml` - Prepare/sidebar settings entry points.
- `src/qml_gui/Theme.qml` - shared tokens; use carefully and avoid broad theme
  churn.
- `src/qml_gui/dialogs/SavePresetDialog.qml` - save-as flow to preserve.
- `src/qml_gui/dialogs/UnsavedChangesDialog.qml` - dirty-close guard to
  preserve.
- `src/core/viewmodels/ConfigViewModel.h`
- `src/core/viewmodels/ConfigViewModel.cpp`
- `src/qml_gui/Models/ConfigOptionModel.h`
- `src/qml_gui/Models/ConfigOptionModel.cpp`
- `tests/QmlUiAuditTests.cpp`
- `tests/ViewModelSmokeTests.cpp`

### OrcaSlicer Source Truth

- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.hpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/PresetComboBoxes.hpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/PresetComboBoxes.cpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/SavePresetDialog.hpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/SavePresetDialog.cpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/UnsavedChangesDialog.hpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/UnsavedChangesDialog.cpp`

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets

- `SettingsDialog.qml` already has `ApplicationWindow`, `Qt.NonModal`, fixed
  736x593 sizing, three-tier parameterization, dirty-close guard, `SavePresetDialog`,
  `UnsavedChangesDialog`, preset combo binding, search state, advanced mode,
  and tab filtering.
- `main.qml` already instantiates `printerSettingsDialog`,
  `materialSettingsDialog`, and `processSettingsDialog`, and dispatches
  `settingsRequested(category)` to `.show()`.
- `LeftSidebar.qml` already routes printer, filament, and process settings
  buttons through `backend.forwardSettingsRequest(...)`.
- `ConfigViewModel` already exposes preset names/current preset selection,
  dirty state, compatibility status/message, search/filtering, and save flows.
- `QmlUiAuditTests.cpp` and `ViewModelSmokeTests.cpp` are the natural places
  for Phase 85 source/entrypoint regression coverage.

### Established Patterns

- QML should remain presentation and wiring; existing C++ viewmodels/services
  keep settings behavior.
- Screenshot-driven UI phases treat screenshot layout as visual truth and
  OrcaSlicer source as behavior truth.
- Existing `Cx*` controls and QML assets should be reused where they fit.
- Replaced visible UI paths should be removed or made unreachable in the same
  milestone, with final cleanup in Phase 88.

### Integration Points

- `BackendContext::forwardSettingsRequest(category)` sets active tier and emits
  `settingsRequested(category)`.
- `main.qml` maps `printer` to printer settings, `filament` to material
  settings, and `print`/`process` to process settings.
- `SettingsDialog.qml` uses `configVm.requestCurrent*Preset`,
  `requestSavePendingChanges`, `onSaveAsRequired`, `filterOptionIndices`, and
  `optionModel.filterIndicesByPage`.
- Dirty-close guard depends on `configVm.isPresetDirty`,
  `UnsavedChangesDialog`, and `requestDiscardPendingChanges`.

</code_context>

<specifics>
## Specific Ideas

- Printer target first viewport: black native title bar with `打印机设置`, preset
  combo `Creality K2 Plus 0.4 nozzle`, compact action icons, tabs ending with
  `移动能力` selected, no left group navigation, right scrollbar.
- Material target first viewport: black native title bar with `材料设置`, preset
  combo `Creality Generic PLA @K2-all`, compact action icons, tabs beginning
  with `耗材丝`, no left group navigation, right scrollbar.
- Current `SettingsDialog.qml` has visible mojibake labels, a permanent search
  field, text `Save` / `Save As...` buttons, an extra manual close button, and
  current tab labels that need cleanup.
- Current `GroupNavSidebar.qml` exists but is not visual truth for Phase 85.
  Do not add it to the visible shell.

</specifics>

<deferred>
## Deferred Ideas

- Option section headers, row internals, typed-control visuals, min/max paired
  row rendering, and dirty/value-source/validation indicator polishing belong
  to Phase 86 unless a minimal shell change is required to keep Phase 85
  coherent.
- Preset save/reset/dirty workflow hardening belongs to Phase 87.
- Stale resource/test cleanup, canonical verifier, runtime launch, and final
  screenshot evidence belong to Phase 88.
- LAN/device/cloud/network, Monitor, ModelMall/Home WebView, camera streams,
  AssembleView, D3D12/Vulkan, and full upstream preset-bundle import/export
  remain outside v4.1 scope.

</deferred>

---

*Phase: 85-Settings Shell And Tab Layout Restoration*
*Context gathered: 2026-07-07*
