---
phase: 46
slug: config-editing-dirty-state-and-reset-semantics
status: approved
shadcn_initialized: false
preset: none
created: 2026-06-30
reviewed_at: 2026-06-30
---

# Phase 46 - UI Design Contract

> Visual and interaction contract for preset config editing, dirty-state feedback, reset behavior, and unsaved-change guards in the settings/config workflow.

---

## Design System

| Property | Value |
|----------|-------|
| Tool | none |
| Preset | not applicable |
| Component library | Qt Quick Controls 2 Basic + existing OWzx QML controls |
| Icon library | existing QML assets/icons only; no new icon dependency |
| Font | existing Qt default app font through `Theme.qml` tokens |

Phase 46 does not create a new standalone product area. It tightens the behavior of the existing settings/config surfaces: `PrintSettings.qml`, `SettingsPage.qml`, `ParamsPage.qml`, and the preset-related dialogs already in the repo.

---

## Spacing Scale

Declared values for any new or revised preset-editing affordance:

| Token | Value | Usage |
|-------|-------|-------|
| xs | 4px | Inline badges, icon gaps, status dots |
| sm | 8px | Compact row spacing, small dialogs |
| md | 16px | Default content spacing |
| lg | 24px | Section padding |
| xl | 32px | Major panel spacing |
| 2xl | 48px | Large dialog breathing room |
| 3xl | 64px | Not expected in this phase |

Exceptions: none. This phase should reuse the existing dense settings-panel rhythm rather than introducing roomy marketing-style spacing.

---

## Typography

This phase is an operator-facing settings workflow. Typography must stay compact and utilitarian:

| Role | Size | Weight | Line Height |
|------|------|--------|-------------|
| Caption | 10px | 400 | 1.3 |
| Body | 12px | 400 | 1.4 |
| Label | 12-14px | 500-600 | 1.3 |
| Heading | 14-16px | 600 | 1.25 |

Rules:
- Dirty badges, warnings, and diff rows use compact body/caption sizes.
- No hero-scale text.
- Dialog explanatory text must stay short and task-focused.

---

## Color

| Role | Value | Usage |
|------|-------|-------|
| Dominant (60%) | existing `Theme.bgBase`, `Theme.bgPanel` | Settings page background and panels |
| Secondary (30%) | existing `Theme.bgElevated`, `Theme.bgInset`, `Theme.borderSubtle` | Option rows, dialogs, grouped panels |
| Accent (10%) | existing `Theme.accent` | Active preset tab, active selection, confirm/save action, focused editable state |
| Warning | existing `Theme.statusWarning` or current amber tokens | Dirty state, unsaved warning, modified count |
| Destructive | existing `Theme.statusError` / red tokens | Discard, destructive reset, invalid action |

Accent reserved for:
- active preset tier/tab,
- currently selected preset,
- explicit save/confirm actions,
- focused field state.

Warning color reserved for:
- modified/dirty indicators,
- unsaved-change warnings,
- resettable changed state.

Do not paint the whole settings surface in accent or warning colors just because a preset is dirty.

---

## Interaction Contract

| Interaction | Behavior |
|-------------|----------|
| Edit a config option | Update C++ state immediately and refresh row dirty state/count without requiring Apply |
| Reset one option | Restore the correct active-tier preset value or explicit chosen inheritance level; do not jump straight to schema default unless that is the requested level |
| Reset all modified options | Restore the active-tier preset reference values for the current editing target |
| Switch print/filament/printer preset with no dirty edits | Switch immediately |
| Switch print/filament/printer preset with dirty edits | Trigger unsaved-change guard with Save / Discard / Cancel |
| Switch object/plate/global scope with dirty edits | Trigger unsaved-change guard when the switch would discard or replace the current editing context |
| Leave settings/config page with dirty edits | Trigger unsaved-change guard before discarding context |
| New/Open project while dirty preset edits exist | Trigger unsaved-change guard before replacing context |

Additional rules:
- Cancel must leave the current visible state unchanged.
- Save and Discard must resume the originally requested action after resolution.
- QML may not directly bypass guarded transitions by calling raw setters in places where data loss is possible.

---

## Visual Hierarchy

Settings/config surfaces should communicate this hierarchy:

1. Current editing context: preset tier + selected preset + active scope.
2. Dirty state summary: compact, visible, always near the editing header or toolbar.
3. Modified rows: local, lightweight emphasis inside the parameter list.
4. Value-source / reset affordances: secondary tools, discoverable but not dominating the page.
5. Unsaved-change decision dialog: blocking modal only when the user is about to lose edits.

The unsaved dialog is a task modal, not a large review page. It should be dense, readable, and oriented toward resolution.

---

## Copywriting Contract

| Element | Copy |
|---------|------|
| Dirty summary | concise count + changed state, no technical jargon |
| Save action | direct verb (`保存`, `Save`) |
| Discard action | explicit loss wording (`丢弃修改`, `Discard changes`) |
| Cancel action | neutral abort wording (`取消`, `Cancel`) |
| Unsaved dialog body | short explanation that the current preset/config has unsaved changes and the next action would replace them |
| Reset action | should imply restoration to reference value, not generic wipe/reset unless that is truly what happens |

Copy rules:
- Do not mention implementation terms like snapshot, merged config, schema default, or internal tier cache.
- When a reset targets a non-default reference, the visible wording should not falsely imply "reset to factory default".
- Keep all user-visible strings in `qsTr()`.

---

## Registry Safety

| Registry | Blocks Used | Safety Gate |
|----------|-------------|-------------|
| shadcn official | none | not applicable |
| third-party registries | none | not applicable |

---

## Source-Truth Visual Mapping

| Upstream Area | Functional Mapping |
|---------------|--------------------|
| `Tab.cpp` dirty indicators | active preset label, dirty counts, reset affordances |
| `UnsavedChangesDialog.cpp` | blocking Save / Discard / Cancel decision flow |
| `SavePresetDialog.cpp` | save/rename naming surface remains compact and task-driven |
| Config tabs / option groups | typed option rows, grouped categories, source/reset affordances |

Technical implementation remains Qt/QML-native. Functional parity is the target, not wxWidgets visual imitation.

---

## Checker Sign-Off

- [x] Dimension 1 Copywriting: PASS
- [x] Dimension 2 Visuals: PASS
- [x] Dimension 3 Color: PASS
- [x] Dimension 4 Typography: PASS
- [x] Dimension 5 Spacing: PASS
- [x] Dimension 6 Registry Safety: PASS

**Approval:** approved 2026-06-30
