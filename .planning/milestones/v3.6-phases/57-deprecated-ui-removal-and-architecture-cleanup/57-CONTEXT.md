# Phase 57: Deprecated UI Removal and Architecture Cleanup - Context

**Gathered:** 2026-07-03
**Status:** Ready for planning
**Mode:** Minimal context — cleanup checklist is locked by Phase 50 §1.6

<domain>
## Phase Boundary

Phase 57 removes the off-design / replaced UI code that Phase 56 left in place
alongside the new independent settings dialogs, and enforces clean architecture
boundaries. The deletion checklist is **frozen by Phase 50** (`50-INVENTORY.md`
§1.6) — Phase 57 executes it, it does not re-decide it.

**In scope (CLEAN-01..04):**
1. **Locked Settings cleanup** (`50-INVENTORY.md` §1.6):
   - Delete files: `src/qml_gui/pages/SettingsPage.qml`,
     `src/qml_gui/pages/ConfigPage.qml`, `src/qml_gui/components/ParamsPage.qml`,
     `src/qml_gui/components/SearchDialog.qml`.
   - Remove their `<file>` entries from `src/qml_gui/qml.qrc`.
   - Remove routes: `BackendContext::canLeaveSettingsPage()`,
     `BackendContext::requestConfigPageExitIfNeeded()`,
     `ConfigViewModel::requestLeaveSettingsPage()` (+ any page-router refs to
     Config/Settings pages).
   - Remove imports of `components/ParamsPage.qml` / `components/SearchDialog.qml`.
   - Rewrite stale doc paths: `docs/源码真值功能矩阵.md` and `docs/源码真值基线.md`
     (`third_party/CrealityPrint/` → `third_party/OrcaSlicer/`).
2. **Broader dead-UI audit (CLEAN-02):** scan for other parallel/legacy/
   disconnected copies of replaced components. Known candidates from Phase 52:
   legacy `src/qml_gui/panels/Sidebar.qml` (452 lines, not wired into
   PreparePage), `FilamentPanel.qml`, `PrintSettings.qml` (Phase 52 explicitly
   deferred their removal to 57). Any other `old`/`legacy`/`deprecated`/`unused`/
   placeholder-only UI discovered by grep is in scope.
3. **QML business-logic audit (CLEAN-03):** verify no durable workflow behavior
   leaked into QML inline scripts in the changed areas; migrate any found into
   C++ services/viewmodels (per `.claude/rules/qml-boundaries.md` /
   `core-architecture.md`).
4. **Encoding cleanup (CLEAN-04):** changed source/QML/Markdown/JSON/CMake files
   remain UTF-8 without BOM, English ASCII-only comments.

**Out of scope:**
- The new Phase 56 SettingsDialog/OptionRow/GroupNavSidebar (just shipped).
- Device/AssembleView/auto-filament-map/wipe-tower (Future).
- Phase 58 (E2E visual + functional verification).

</domain>

<decisions>
## Implementation Decisions

### Locked (from Phase 50 §1.6 + Phase 52 deferrals)
- The 4 Settings files ARE deleted (not kept as reference). Their removal was
  the decision-of-record in `50-INVENTORY.md` §1.5 (14 `replace` rows).
- Routes/imports/qrc entries for the deleted files are removed in the same phase
  (CLEAN-01: remove obsolete files, qml.qrc entries, registrations, routes,
  imports, tests, documentation references).
- Legacy Sidebar.qml / FilamentPanel.qml / PrintSettings.qml: verify they are
  truly unreferenced before deletion; if any are still referenced by a live
  path, DO NOT delete — flag and migrate the reference instead.

### Audit discipline (CLEAN-02)
- For every deletion candidate, run a reference grep BEFORE deleting
  (`qml.qrc`, `Loader source:`, `sourceComponent`, `import`, `Qt.resolvedUrl`,
  C++ `qmlRegisterType`/`setContextProperty`, page-router `setCurrentPage`).
  A file is only deleted if it has ZERO live references. Any live reference
  blocks deletion until resolved.
- The success criterion is "No active UI path retains parallel old/legacy/
  deprecated/unused placeholder-only or disconnected copies of replaced Prepare,
  Preview, or Settings components" — proven by a post-cleanup QmlUiAudit-style
  grep that no deleted path is referenced anywhere.

### Claude's Discretion
- Whether to split into 2 plans (Settings-locked-cleanup vs broader-audit) or
  one plan — planner decides based on audit size.
- Test placement for the no-stale-reference audit (extend QmlUiAuditTests.cpp
  vs a new target).
- Whether to also remove now-empty C++ methods/signals beyond the 3 named routes
  (e.g., if removing requestLeaveSettingsPage leaves dead code in ConfigViewModel).
- Exact migration target for any business logic found in QML (usually the
  relevant viewmodel).

</decisions>

<code_context>
## Existing Code Insights

### Deletion targets (verified to exist on disk by Phase 50 §1.6)
- `src/qml_gui/pages/SettingsPage.qml` — off-design embedded settings surface
  (replaced by SettingsDialog.qml in Phase 56).
- `src/qml_gui/pages/ConfigPage.qml` — off-design "参数配置" stub.
- `src/qml_gui/components/ParamsPage.qml` — off-design option renderer (replaced
  by OptionRow.qml + GroupNavSidebar.qml).
- `src/qml_gui/components/SearchDialog.qml` — off-design search dialog (the new
  dialogs have inline search; cross-category search via this dialog is dropped).

### Known legacy (Phase 52 deferred to 57)
- `src/qml_gui/panels/Sidebar.qml` (452 lines, per Phase 52 CONTEXT — NOT wired
  into PreparePage). Embeds FilamentPanel.qml + PrintSettings.qml.
- `src/qml_gui/panels/FilamentPanel.qml`, `src/qml_gui/panels/PrintSettings.qml`.

### Integration points to verify after deletion
- `src/qml_gui/main.qml` — page router must not reference Config/Settings pages.
- `src/qml_gui/BackendContext.{h,cpp}` — the 3 named routes + any page-nav enum
  entries for Config/Settings.
- `src/qml_gui/qml.qrc` — must contain only live components (CLEAN-02).
- `src/core/viewmodels/ConfigViewModel.{h,cpp}` — requestLeaveSettingsPage +
  related dead code.
- The new Phase 56 dialogs (SettingsDialog/OptionRow/GroupNavSidebar) must keep
  working — they are the replacement, NOT touched here.

### Established patterns
- v14-01 component gate: only Cx* controls in consuming code; the deleted files
  leaving means that gate has fewer files to audit.
- Build: ONLY `scripts/auto_verify_with_vcvars.ps1`; ONLY `build/`.
- Encoding: UTF-8 no BOM, ASCII-only comments (global guard).
- QML boundaries: business logic in C++; QML presentation/wiring only.

</code_context>

<specifics>
## Specific Ideas

- The deletion is the easy half; the audit (CLEAN-02) is the high-value half —
  grep-driven, prove-no-stale-references. A compiled audit test (like the
  InventoryAuditTests planned for Phase 58) that asserts the 4 deleted paths +
  the 3 deleted routes are absent from the repo is the regression guard.
- Phase 56's new dialogs already import the controls they need directly; they do
  NOT depend on ParamsPage/SearchDialog/SettingsPage/ConfigPage. Verify that
  pre-deletion (a quick grep for SettingsDialog referencing the deleted files).
- The 2 doc rewrites are mechanical (CrealityPrint→OrcaSlicer path strings) but
  must be done — they're in the locked checklist.

</specifics>

<deferred>
## Deferred Ideas

- Full business-logic-in-QML sweep across the ENTIRE codebase (CLEAN-03 is
  scoped to the changed areas + obvious violations; a full sweep is ongoing).
- Removing every `*Mock` service name suffix (vocabulary cleanup) — not in this
  phase.
- Legacy calibration/Mall/device dead UI if any — separate phases.

</deferred>
