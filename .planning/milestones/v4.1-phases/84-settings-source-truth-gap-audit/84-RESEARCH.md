# Phase 84 Research: Settings Source-Truth Gap Audit

**Phase:** 84 - Settings Source-Truth Gap Audit
**Date:** 2026-07-07
**Mode:** Inline research fallback for `$gsd-plan-phase 84 --auto`

## Research Question

What needs to be known to plan Phase 84 well?

## Key Findings

### Audit Scope

Phase 84 is a documentation/source audit, not an implementation phase. The
correct deliverable is a canonical settings gap matrix that downstream phases
can execute without rediscovering visual truth, upstream behavior truth, Qt
targets, owner phases, or verification routes.

### Visual Truth

The active visual truth is:

- `shotScreen/打印机参数设置页.png`
- `shotScreen/材料参数设置页.png`

Both screenshots are 736x593. They show an independent dark settings window
with:

- top native title/chrome,
- preset selector row,
- compact action icons,
- horizontal tab strip,
- dense vertical sections,
- typed rows with unit fields,
- a right scroll bar,
- no visible left option-group navigation sidebar.

The process dialog has no screenshot. It should be audited by OrcaSlicer source
parity and same-shell consistency, not by invented design.

### Current Qt Evidence

Current settings implementation already contains useful semantic foundations:

- `src/qml_gui/dialogs/SettingsDialog.qml` provides an independent non-modal
  `ApplicationWindow`, tier-specific preset selection, tab filtering, save-as
  and unsaved guard wiring.
- `src/qml_gui/components/OptionRow.qml` dispatches typed controls and routes
  edits through `optionModel.setValue()`.
- `src/qml_gui/components/GroupNavSidebar.qml` implements a left group nav, but
  the target settings screenshots do not show that visible surface.
- `src/core/viewmodels/ConfigViewModel.*`,
  `src/qml_gui/Models/ConfigOptionModel.*`, and
  `src/core/services/PresetServiceMock.*` own the Phase 56 semantics that v4.1
  must preserve unless the audit finds source-truth mismatches.

### Historical Evidence

Phase 56 verified seven settings requirements covering independent dialogs,
typed option models, dirty/value-source/read-only state, save/reset/unsaved
guards, search/filtering, slice invalidation, and dirty override persistence.

Phase 56 also deferred three visual/runtime items:

1. settings dialog visual parity against printer/material screenshots,
2. typed-control rendering visual proof,
3. non-modal cross-window live edit evidence.

Phase 84 must reclassify these into v4.1 owner phases instead of treating Phase
56 visual closure as complete.

### Upstream Source Truth

Downstream planning and execution must map settings behavior to OrcaSlicer
sources:

- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.*`
- `third_party/OrcaSlicer/src/slic3r/GUI/PresetComboBoxes.*`
- `third_party/OrcaSlicer/src/slic3r/GUI/ConfigManipulation.*`
- `third_party/OrcaSlicer/src/slic3r/GUI/SavePresetDialog.*`
- `third_party/OrcaSlicer/src/slic3r/GUI/UnsavedChangesDialog.*`
- `third_party/OrcaSlicer/src/slic3r/GUI/CreatePresetsDialog.*`
- `third_party/OrcaSlicer/src/libslic3r/PrintConfig.*`
- `third_party/OrcaSlicer/src/libslic3r/Preset.*`
- `third_party/OrcaSlicer/src/libslic3r/PresetBundle.*`

### Planning Implication

One plan is enough. It should:

1. create `84-GAP-MATRIX.md`,
2. populate settings region rows,
3. reconcile Phase 56 deferred items,
4. verify coverage with text/source checks,
5. write Phase 84 verification evidence.

No full build is needed in Phase 84 unless execution unexpectedly changes
production source files. The full canonical verifier and app screenshots belong
to Phase 88.

## Validation Architecture

Phase 84 validation is document/source based.

Automated checks:

- `git diff --check`
- `python %USERPROFILE%\.coding-encoding-guard\encoding_guard.py` on Phase 84
  planning artifacts
- source/text assertions that `84-GAP-MATRIX.md` contains all required region
  IDs, `SETAUDIT-01`, `SETAUDIT-02`, downstream Phase 85-88 ownership, target
  screenshot paths, current Qt targets, upstream anchors, and Phase 56 residual
  reconciliation

Manual-only checks:

- none for Phase 84 itself; visual/manual evidence is intentionally routed to
  Phase 88.

## Research Complete

This research is sufficient to plan Phase 84 as a single documentation audit
plan.
