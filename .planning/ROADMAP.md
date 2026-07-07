# Roadmap: OWzx Slicer

## Milestones

- Complete: **v2.9** - Phases 10-15 (shipped 2026-06-25)
- Complete: **v3.0 PartPlate Core** - Phases 16-22 (shipped 2026-06-26)
- Complete: **v3.1 QRhi Rendering** - Phases 23-28 (shipped 2026-06-28)
- Complete with tech debt: **v3.2 Multi-Plate Data Polish** - Phases 29-32 (audited 2026-06-28)
- Complete at MVP level, superseded for completeness: **v3.3 Slice Preview Main Flow MVP** - Phases 33-36
- Complete by automated E2E closure: **v3.4 Import to G-code Complete Workflow** - Phases 37-43
- Superseded after Phase 46: **v3.5 Preset Authoring Complete Workflow** - Phases 44-49
- Automated verification passed, visual UAT not closed: **v3.6 Screenshot-Driven OrcaSlicer UI Restoration** - Phases 50-58 (shipped 2026-07-03)
- Complete with residual gaps: **v3.7 Screenshot-Level UI Parity Closure** - Phases 59-64 (2026-07-04; D3D12 and manual visual debt carried forward)
- Complete with tech debt: **v3.8 RHI Gizmo Parity** - Phases 65-73 (shipped 2026-07-04; 21/21 requirements satisfied, Phase 68 visual evidence deferred)
- Complete with process debt: **v3.9 Prepare Page UI Restoration** - Phases 74-78 (shipped 2026-07-06; 12/12 requirements satisfied, canonical verifier passed, runtime screenshot captured)
- Complete: **v4.0 Preview Page UI Restoration** - Phases 79-83 (shipped 2026-07-07; 13/13 requirements satisfied, canonical verifier passed, runtime Preview screenshot captured)

## Current Milestone: v4.1 Parameter Settings Dialogs Source-Truth Restoration

**Goal:** Restore printer, material, and process settings dialogs to screenshot/source-truth parity while preserving the Phase 56 backend semantics for typed options, dirty state, save/reset, slice invalidation, and project persistence.

**Scope rule:** This milestone is local/offline only. LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware workflows remain removed from the forward roadmap unless explicitly reopened.

## Phases

- [x] Phase 84: Settings Source-Truth Gap Audit (completed 2026-07-07)
- [ ] Phase 85: Settings Shell And Tab Layout Restoration
- [ ] Phase 86: Settings Option Sections And Typed Controls
- [ ] Phase 87: Settings Preset Semantics And Workflow Stability
- [ ] Phase 88: Settings Verification And Cleanup

| Phase | Name | Goal | Requirements |
|---|---|---|---|
| 84 | Settings Source-Truth Gap Audit | Complete 2026-07-07: froze the v4.1 settings region map, current gaps, upstream anchors, Qt targets, replacement decisions, and verification expectations. | SETAUDIT-01, SETAUDIT-02 |
| 85 | Settings Shell And Tab Layout Restoration | Restore the screenshot-visible settings window chrome, preset/action row, tab strip, sizing, density, and clean user-facing text. | SETLAYOUT-01, SETLAYOUT-02, SETLAYOUT-03 |
| 86 | Settings Option Sections And Typed Controls | Restore option section rendering and typed control visuals for printer/material/process settings without breaking existing C++ option models. | SETCTRL-01, SETCTRL-02, SETCTRL-03 |
| 87 | Settings Preset Semantics And Workflow Stability | Preserve and harden save/reset/search/dirty/edit semantics across settings dialogs, Prepare, Preview, project persistence, and slice invalidation. | SETSEM-01, SETSEM-02, SETSEM-03 |
| 88 | Settings Verification And Cleanup | Remove stale settings paths, lock bindings with audits/tests, run canonical verifier, launch app, and capture settings visual evidence. | SETCLEAN-01, SETVERIFY-01, SETVERIFY-02 |

### Phase 84: Settings Source-Truth Gap Audit

**Status:** Complete 2026-07-07
**Plans:** 1/1 plans complete

Success criteria:
1. Printer, material, and process settings regions are mapped to target screenshots, upstream source files, and Qt targets.
2. Phase 56 residual visual-UAT items are reconciled into v4.1 requirements with explicit owner phases.
3. Current SettingsDialog/OptionRow/GroupNavSidebar gaps are classified as modify, replace, remove, or defer.

### Phase 85: Settings Shell And Tab Layout Restoration

**Status:** Planned
**Plans:** 0/1 plans complete

Success criteria:
1. Printer/material/process settings open as independent non-modal windows from every intended entry point.
2. Window title, preset selector, action icons, search/advanced affordance, tab strip, and close behavior match screenshot density without mojibake or raw labels.
3. Off-design surfaces such as the unused left group sidebar are removed from the visible settings window unless source-truth evidence requires them.

### Phase 86: Settings Option Sections And Typed Controls

**Status:** Planned
**Plans:** 0/1 plans complete

Success criteria:
1. Option sections render with screenshot-like headers, dividers, icons, and compact vertical spacing.
2. Bool, numeric/unit, enum, text/color, nullable/vector, and min/max paired controls render and edit through existing C++ models.
3. Dirty, read-only, value-source, validation, and disabled states stay visible without row overlap or layout jumps.

### Phase 87: Settings Preset Semantics And Workflow Stability

**Status:** Planned
**Plans:** 0/1 plans complete

Success criteria:
1. Preset selection, save, save-as, reset option/group/all, discard, cancel, and unsaved-close flows keep upstream-mapped behavior.
2. Search and simple/advanced filtering remain per-dialog and do not hide active dirty/error states.
3. Settings edits invalidate slice state, persist dirty overrides through project save/load, and do not clear Prepare/Preview payload state.

### Phase 88: Settings Verification And Cleanup

**Status:** Planned
**Plans:** 0/1 plans complete

Success criteria:
1. Replaced settings UI paths leave no stale files, imports, resource entries, tests, or disconnected controls.
2. Automated source/QML audits cover region mapping, text cleanup, layout structure, option bindings, and upstream anchors.
3. The canonical verifier passes, `build/OWzxSlicer.exe` launches, and printer/material/process settings visual evidence is recorded against the target screenshots.

## Deferred Backlog

- AssembleView.
- Auto filament-map recommendation and wipe-tower geometry/rendering.
- Real thumbnail capture and 3MF pixel round-trip.
- Missing CLI fixtures and deterministic argv-based GUI fixture loading for screenshots.
- D3D12 root-cause investigation and future Vulkan/D3D12 backend promotion.

## Removed Scope

- LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware calibration are no longer backlog items.

## Next Step

Start Phase 85:

```text
$gsd-discuss-phase 85 --auto
```

or plan directly:

```text
$gsd-plan-phase 85
```

---

*Last updated: 2026-07-07 after v4.1 milestone planning.*
