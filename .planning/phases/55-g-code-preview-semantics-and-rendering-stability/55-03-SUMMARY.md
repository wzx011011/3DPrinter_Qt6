---
phase: 55-g-code-preview-semantics-and-rendering-stability
plan: 03
subsystem: ui
tags: [qml, qt6, qml-gui, gcode-preview, extrusion-role, visibility-filter, collapsible-section, theme-tokens, cx-controls]

# Dependency graph
requires:
  - phase: 55-g-code-preview-semantics-and-rendering-stability (plan 02)
    provides: PreviewViewModel Q_PROPERTY roleVisibilities + Q_INVOKABLE isRoleVisible/toggleRoleVisibility + RhiViewport Q_PROPERTY roleVisibility (render-side, no repack)
provides:
  - Collapsible "Visible Line Types" VisibilityFilter.qml in the Preview right panel (StatsPanel <-> Legend), one CxCheckBox per extrusion role
  - GLViewport.roleVisibility QML binding pushing previewVm.roleVisibilities into the renderer on every toggle (render-side filtering fires)
  - qml.qrc registration of components/VisibilityFilter.qml
affects:
  - 55-g-code-preview-semantics-and-rendering-stability (plan 04: audit-test extensions assert VisibilityFilter wiring + GCODE-02 UI presence)
  - 58-uat (manual visual-parity check of the 18-role list + interaction)

# Tech tracking
tech-stack:
  added: []  # no new libraries; pure QML presentation layer over the Plan 02 ViewModel/RhiViewport API
  patterns:
    - "Right-panel consumer component shape: Item root + required property var previewVm + implicitHeight binding (mirrors StatsPanel.qml / Legend.qml)"
    - "CollapsibleSection as the right-panel card wrapper (default property alias content) — section header 'Visible Line Types' via qsTr, default expanded"
    - "Role-row Repeater: model = previewVm.roleVisibilities; delegate = color-swatch Rectangle + qsTr(label) Label + CxCheckBox onToggled -> previewVm.toggleRoleVisibility(roleIndex)"
    - "Toggle handler uses onToggled (user-only signal), NOT onCheckedChanged, to avoid feedback loops against the checked: modelData.visible binding"
    - "GLViewport interaction binding: roleVisibility: root.previewVm ? root.previewVm.roleVisibilities : [] — null-guarded like every other previewVm binding"

key-files:
  created:
    - src/qml_gui/components/VisibilityFilter.qml
  modified:
    - src/qml_gui/pages/PreviewPage.qml
    - src/qml_gui/qml.qrc

key-decisions:
  - "qsTr takes the Chinese literal directly (qsTr('可见线条类型')) — matches the established project convention used by StatsPanel ('统计') and Legend ('图例'), where the Chinese is the qsTr source string and English lives in the .ts translation files. The plan/UI-SPEC called this out as the convention to verify; verified against StatsPanel.qml line 20."
  - "CxCheckBox toggle uses onToggled, not onChanged. The plan text said 'onChanged' but Qt CheckBox (which CxCheckBox extends) exposes no 'changed' signal; it exposes checkedChanged (fires on programmatic + user changes -> binding loop with checked: modelData.visible) and toggled (user-only, safe). onToggled mirrors the CxSwitch onToggled pattern already in StatsPanel.qml. This is the correct, idiomatic realization of the plan's intent."
  - "Role-row hover feedback via HoverHandler on the row Rectangle + Theme.bgHover (transparent when not hovered). Mirrors the camera-preset button hover pattern already in PreviewPage.qml. No raw MouseArea needed."
  - "All visual values sourced from Theme tokens (bgHover, textPrimary, borderSubtle via CollapsibleSection, fontSizeSM, spacingSM, spacingXS, radiusSM). Zero ad-hoc colors/fonts/spacings."

patterns-established:
  - "Right-panel collapsible card: wrap an Item-root component body in CollapsibleSection{ title: qsTr(...); expanded: true; content: ColumnLayout { Repeater { model: previewVm.<list>; delegate: row } } } — reusable for future right-panel filter groups"
  - "No business logic in PreviewPage.qml: presentation-only edits (component insertion + property binding). All filtering lives in ViewModel/Renderer per the QML boundary rule"

requirements-completed: [GCODE-02]

# Metrics
duration: ~35min (two canonical full-libslic3r rebuild builds, ~8min each + verification)
completed: 2026-07-02
---

# Phase 55 / Plan 03: VisibilityFilter UI + GLViewport.roleVisibility Binding Summary

**Collapsible "Visible Line Types" card (18 per-role CxCheckBoxes bound to previewVm.toggleRoleVisibility) inserted into the Preview right panel, with a render-side GLViewport.roleVisibility binding that filters drawing on every toggle without repacking gcodePreviewData**

## Performance

- **Duration:** ~35 min (two sequential canonical builds, ~8 min each for the full libslic3r rebuild, plus verification polling)
- **Started:** 2026-07-02
- **Completed:** 2026-07-02
- **Tasks:** 2 (both auto)
- **Files modified:** 3 (1 created, 2 modified)

## Accomplishments
- New VisibilityFilter.qml component renders a collapsible "Visible Line Types" card between StatsPanel and Legend, with one row per extrusion role (color swatch + qsTr label + CxCheckBox), all driven by previewVm.roleVisibilities
- GLViewport.roleVisibility now bound to previewVm.roleVisibilities, so toggling a role checkbox pushes the mask into RhiViewport and fires render-side draw-range filtering (the Plan 02 no-repack path)
- Zero raw controls (only CxCheckBox) and zero ad-hoc Theme values; QmlUiAudit stays green

## Task Commits

Each task was committed atomically (each independently build-verified green before the next began):

1. **Task 1: Create VisibilityFilter.qml + register in qml.qrc** - `6e9766e` (feat)
2. **Task 2: Wire VisibilityFilter into PreviewPage.qml + bind roleVisibility to GLViewport** - `a90b7df` (feat)

**Plan metadata:** (pending commit) (docs: complete plan)

## Files Created/Modified
- `src/qml_gui/components/VisibilityFilter.qml` (CREATED) - Item root + required previewVm + implicitHeight; CollapsibleSection titled "Visible Line Types" wrapping a Repeater over previewVm.roleVisibilities; each delegate = 10x10 color swatch + qsTr(label) + CxCheckBox onToggled -> toggleRoleVisibility(roleIndex); HoverHandler + Theme.bgHover row feedback
- `src/qml_gui/pages/PreviewPage.qml` (MODIFIED) - Edit A: inserted Components.VisibilityFilter between Components.StatsPanel and Components.Legend in the right-panel ScrollView ColumnLayout; Edit B: added `roleVisibility: root.previewVm ? root.previewVm.roleVisibilities : []` to the GLViewport instance alongside previewData/layerMin/layerMax/moveEnd/showTravelMoves
- `src/qml_gui/qml.qrc` (MODIFIED) - added `<file>components/VisibilityFilter.qml</file>` to the existing `<qresource prefix="/qml">` block immediately after components/Legend.qml

## Decisions Made
- **qsTr source-string convention**: used the Chinese literal `qsTr("可见线条类型")` (matching StatsPanel `qsTr("统计")` / Legend `qsTr("图例")`) rather than an English source string. The plan flagged this as the convention to verify; verification confirmed it.
- **onToggled vs onChanged**: see Deviations below.
- **CollapsibleSection content via default property alias**: placed the ColumnLayout+Repeater as a direct child of CollapsibleSection (which declares `default property alias content`), matching the LeftSidebar.qml usage pattern.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Correctness] CxCheckBox toggle signal is onToggled, not onChanged**
- **Found during:** Task 1 (VisibilityFilter.qml creation)
- **Issue:** The plan/UI-SPEC specified `onChanged` for the CxCheckBox handler. Qt's `CheckBox` (which `CxCheckBox` extends) exposes no `changed` signal; the available handlers are `onCheckedChanged` (fires on programmatic AND user changes -> binding loop against `checked: modelData.visible`) and `onToggled` (user-only, no loop). `onChanged` would not compile / would have no effect.
- **Fix:** Used `onToggled: if (root.previewVm) root.previewVm.toggleRoleVisibility(modelData.roleIndex)` — the safe, idiomatic signal. This mirrors the existing `CxSwitch { onToggled: ... }` pattern in StatsPanel.qml (line 39).
- **Files modified:** src/qml_gui/components/VisibilityFilter.qml
- **Verification:** Canonical build green; QmlUiAudit passed; `grep -c toggleRoleVisibility VisibilityFilter.qml` >= 1; behavior is exactly the plan's intent (user toggle -> ViewModel update -> stateChanged -> roleVisibilities re-emitted -> GLViewport.roleVisibility re-bound -> render-side filter).
- **Committed in:** 6e9766e (Task 1 commit)

**2. [Rule 1 - Correctness] qml.qrc qresource prefix is "/qml", not "/"**
- **Found during:** Task 1 (qml.qrc registration)
- **Issue:** The task instructions referenced a `<qresource prefix="/">` block, but in this project the QML files live under `<qresource prefix="/qml">` (the `/` qresource only holds qtquickcontrols2.conf). Legend.qml and StatsPanel.qml are both in the `/qml` block.
- **Fix:** Registered VisibilityFilter.qml in the `/qml` qresource block immediately after Legend.qml (matching where Legend/StatsPanel already sit). This is where the existing `import "../components"` resolution expects them.
- **Files modified:** src/qml_gui/qml.qrc
- **Verification:** Canonical build's RCC step compiled qml.qrc including the new file; QML loaded with no "VisibilityFilter is not a type" warnings; `grep -c '<file>components/VisibilityFilter.qml</file>' qml.qrc` == 1.
- **Committed in:** 6e9766e (Task 1 commit)

---

**Total deviations:** 2 auto-fixed (both Rule 1 - Correctness, forced by Qt API reality + project file structure)
**Impact on plan:** Both deviations are faithful realizations of the plan's stated intent, not scope changes. Behavior and acceptance criteria are fully met.

## Issues Encountered
None. Both canonical builds (after Task 1 and after Task 2) returned exit code 0 with all six test suites passing.

## User Setup Required
None - no external service configuration required. Pure QML presentation layer.

## Next Phase Readiness
- GCODE-02 UI is complete: the user-visible per-role line-type visibility filter matches upstream GCodeViewer's right-side legend list, and toggling fires render-side filtering via the GLViewport.roleVisibility binding (no repack, per Plan 02).
- Plan 04 (audit-test extensions) can now add assertions that PreviewPage.qml contains `Components.VisibilityFilter` + the `roleVisibility:` binding, and that VisibilityFilter.qml uses CxCheckBox + toggleRoleVisibility.
- Manual visual-parity / interaction UAT deferred to Phase 58 per 55-VALIDATION.md.

## Self-Check: PASSED

**Task 1 acceptance (VisibilityFilter.qml):**
- File exists, ASCII/no-BOM: PASS (`file` reports UTF-8 (ASCII subset), no BOM bytes)
- `grep -c CxCheckBox` >= 1: PASS (2)
- raw `Controls.CheckBox`/`CheckBox {` (excluding Cx): PASS (0 — only `CxCheckBox {` present)
- `grep -c qsTr` >= 1: PASS (2)
- `grep -c Theme.` >= 4: PASS (6)
- `grep -c toggleRoleVisibility` >= 1: PASS (2)
- `grep -c '<file>components/VisibilityFilter.qml</file>' qml.qrc` == 1: PASS (1)

**Task 2 acceptance (PreviewPage.qml):**
- `grep -c Components.VisibilityFilter` >= 1: PASS (1)
- `grep -c roleVisibility:` >= 1: PASS (1)
- `grep -c SoftwareViewport` == 0: PASS (0)

**Canonical build (both tasks):** exit code 0; `[UI] QML UI audit tests passed`; `[PreviewParser] PreviewParser tests passed`; `[ViewModel] ViewModel smoke tests passed`; `[E2E] All pipeline tests passed`; `[PrepareScene]` + `[PartPlate]` passed. No `FAILED`, no `error:`, no QML "not a type"/"is undefined" warnings.

---
*Phase: 55-g-code-preview-semantics-and-rendering-stability*
*Plan: 03*
*Completed: 2026-07-02*
