# Roadmap: OWzx Slicer

## Milestones

- ✅ **v2.9** Implementation Realignment and Stabilization — Phases 10-15 (shipped 2026-06-25)
- ✅ **v3.0** PartPlate Core — Phases 16-22 (shipped 2026-06-26)
- ✅ **v3.1** QRhi Rendering — Phases 23-28 (shipped 2026-06-28)
- ✅ **v3.2** Multi-Plate Data Polish — Phases 29-32 (audited 2026-06-28)
- ✅ **v3.3** Slice Preview Main Flow MVP — Phases 33-36 (superseded by v3.4)
- ✅ **v3.4** Import to G-code Complete Workflow — Phases 37-43 (closed by automated E2E)
- ✅ **v3.5** Preset Authoring Complete Workflow — Phases 44-49 (superseded after Phase 46)
- ✅ **v3.6** Screenshot-Driven OrcaSlicer UI Restoration — Phases 50-58 (shipped 2026-07-03)
- ✅ **v3.7** Screenshot-Level UI Parity Closure — Phases 59-64 (2026-07-04)
- ✅ **v3.8** RHI Gizmo Parity — Phases 65-73 (shipped 2026-07-04)
- ✅ **v3.9** Prepare Page UI Restoration — Phases 74-78 (shipped 2026-07-06)
- ✅ **v4.0** Preview Page UI Restoration — Phases 79-83 (shipped 2026-07-07)
- ✅ **v4.1** Parameter Settings Dialogs Source-Truth Restoration — Phases 84-88 (shipped 2026-07-09)
- ✅ **v4.2** AssembleView Source-Truth Restoration — Phases 89-93 (shipped 2026-07-09)
- ✅ **v4.3** Real Thumbnail Capture And 3MF Round-Trip — Phases 94-98 (shipped 2026-07-10)
- ✅ **v4.4** Wipe-Tower Geometry Readback And Real Rendering — Phases 99-102 (shipped 2026-07-12)
- ✅ **v4.5** Backlog Closure — Phases 103-116 (shipped 2026-07-13)
- ✅ **v4.6** Core Feature Completion Sweep — Phases 117-128 (shipped 2026-07-15)
- ✅ **v4.7** Polish, i18n & Advanced Feature Recovery — Phases 129-135 (shipped 2026-07-15, tech_debt)
- ✅ **v4.8** Dependency Unlock, Assembly Transform & i18n Completion — Phases 136-140 (shipped 2026-07-16, tech_debt)
- ✅ **v5.0** Advanced Feature Recovery & Tech-Debt Closure — Phases 141-153 (shipped 2026-07-17, tech_debt)
- ✅ **v5.1** v5.0 Deferred Items Closure — Phases 154-159 (shipped 2026-07-17, clean)
- 🚧 **v5.2** UI Excellence — Phases 160-170 (in planning)

## Current Milestone: v5.2 UI Excellence

**Goal:** Drive every UI surface (pages, dialogs, components, controls, panels) to consistent, design-system-backed excellence. App-wide audit baseline average = 12/24 (50%); v5.2 target = ≥20/24 (83%) per surface. Driven by 6 retroactive UI audits (`/gsd-ui-review` of PreparePage + 5 parallel audits).

**Quality rule (user 2026-07-19):** "不要在意实现难度和时间成本，只要最好的效果" — quality first, ignore cost/difficulty. The milestone is sized by what's needed for UI excellence, not by what fits one cycle.

**Scope rule:** All work is offline/local. No new product behavior — only consistency, polish, and design-system enforcement. SLA print path is DECLINED (user decision 2026-07-19; reclassified from tech_debt). Maps to OrcaSlicer v7.0.1 upstream where applicable.

**Nine workstreams (14 requirements across 11 phases, 160-170):**
- **DS — Design-System Foundation (DS-01..03, Phases 160-161):** Theme.qml token additions + dead-token cleanup (Phase 160); Cx* control library hardening as the design-system carrier (Phase 161). Serial foundation for all later phases.
- **TK — Token Migration (TK-01..03, Phases 162-164):** color hardcode sweep (162); typography hardcode sweep (163); spacing hardcode sweep + sidebar width fix (164). Three mechanical-replacement phases, parallel-safe after 160-161 land.
- **SW — Sidebar Width System (SW-01, Phase 164):** unbreak the 7-layer 392px lock; make DockableSidebar drag handle functional; honor Phase 74 UI-SPEC "compact" requirement. Bundled into Phase 164 (spacing sweep) because they share file touch points.
- **CW — Copywriting & Language (CW-01..02, Phase 165):** pick ZH source language and sweep EN dialogs; kill dev-jargon strings; add qsTr() to Cx* library.
- **Dlg — Dialog Consistency (Dlg-01..02, Phase 166):** fix 8 empty-header dialogs; replace pseudo-buttons with CxButton; standardize button-row + modal policy.
- **Cmp — Component Coherence (Cmp-01..03, Phase 167):** collapse notification trio → NotificationCard base; resolve 4 orphan components; unify parallel idioms.
- **VS — Visual Control Migration (VS-01..02, Phase 168):** replace Rectangle+MouseArea pseudo-buttons in pages with CxButton/CxIconButton; Emboss Slider → CxSlider.
- **XD — Experience Safety (XD-01..02, Phase 169):** destructive-action confirmation component + route 12+ triggers; async spinner on Emboss; SliceProgress state coverage.
- **Cross-WS — REGRESS-06 (Phase 170):** consolidated v5.2 regression gate — locks every UI contract from Phases 160-169 via source-audit slots.

**Coverage:** 14/14 v5.2 requirements mapped to exactly one phase. 0 unmapped.

## Phases

## Phases

- [x] Phase 154: QML Preset Diff-View Dialog (CLOS, PSET-05 closure)
- [x] Phase 155: Emboss 3MF Text Metadata Round-Trip (CLOS, EMB-06 closure)
- [x] Phase 156: Runtime Plate Thumbnail Capture Scheduler (CLOS, PLATE-05 closure)
- [x] Phase 157: Live Multi-Plate Round-Trip ctest Fixture (CLOS, PLATE-06 closure)
- [x] Phase 158: Emboss Style Controls + SVG Advanced Features (EMBO-F)
- [x] Phase 159: v5.1 Cross-Workstream Regression Gate (REGRESS-05)
- [ ] Phase 160: Theme Token Foundation (DS-01)
- [ ] Phase 161: Cx* Control Library Hardening (DS-02, DS-03)
- [ ] Phase 162: Color Hardcode Sweep (TK-01)
- [ ] Phase 163: Typography Hardcode Sweep (TK-02)
- [ ] Phase 164: Spacing Sweep + Sidebar Width Fix (TK-03, SW-01)
- [ ] Phase 165: Copywriting & Language Sweep (CW-01, CW-02)
- [ ] Phase 166: Dialog Consistency Repair (Dlg-01, Dlg-02)
- [ ] Phase 167: Component Coherence (Cmp-01, Cmp-02, Cmp-03)
- [ ] Phase 168: Visual Control Migration (VS-01, VS-02)
- [ ] Phase 169: Experience Safety (XD-01, XD-02)
- [ ] Phase 170: v5.2 Cross-Workstream UI Regression Gate (REGRESS-06)

| Phase | Name | Goal | Requirements |
|---|---|---|---|
| 154 | QML Preset Diff-View Dialog | Wire a QML side-by-side diff dialog that consumes the existing `comparePresets(A, B)` primitive (shipped Phase 149) | CLOS-01 |
| 155 | Emboss 3MF Text Metadata Round-Trip | Persist editable-text metadata via upstream `TextConfigurationSerialization` so reloaded TextEmboss volumes are re-editable | CLOS-02 |
| 156 | Runtime Plate Thumbnail Capture Scheduler | Add the session-capture loop + `setPlateThumbnailFromBase64` write path so non-current plates show real thumbnails in-session | CLOS-03 |
| 157 | Live Multi-Plate Round-Trip ctest Fixture | Ship a `ProjectServiceMock` test fixture so the PLATE-06 multi-plate round-trip ctest actually runs (Phase 152 was source-audit-locked only) | CLOS-04 |
| 158 | Emboss Style Controls + SVG Advanced Features | Wire FontProp style axes (boldness/italic/use-surface/curve-projection) + SVG curve projection + depth modifier | EMBO-F01, EMBO-F02 |
| 159 | v5.1 Cross-Workstream Regression Gate | Consolidate all v5.1 anchors + re-assert v5.0/v4.8/v4.7/v4.6 | REGRESS-05 |

### Build Order (parallelism guidance for the executor)

Phase numbers are sequential, but the 4 CLOS phases are independent (each closes a different v5.0 partial) and parallel-safe. EMBO-F depends on nothing from CLOS. REGRESS-05 is the final consolidation phase.

- **Wave A (parallel, no deps):** Phase 154 (CLOS-01 PSET-05 diff dialog — QML consumer of an existing primitive). Phase 155 (CLOS-02 EMB-06 3MF text metadata — C++ serialization). Phase 156 (CLOS-03 PLATE-05 runtime thumbnails — capture scheduler). Phase 157 (CLOS-04 PLATE-06 ctest fixture — test infra). All four close a different v5.0 partial and touch disjoint files.
- **Wave B (parallel with A, no deps on CLOS):** Phase 158 (EMBO-F Emboss style + SVG advanced — extends the Phase 145/146 Emboss panel and `addSvgVolume` path; independent of all CLOS work).
- **Wave C (last, after all feature phases):** Phase 159 (REGRESS-05) — needs all feature phases (154-158) to consolidate their anchors.

**Recommended critical-path serial summary:** any one of {154, 155, 156, 157, 158} → REGRESS-05 (159). With `parallelization=true`, Waves A+B can run concurrently (5 phases in flight); 159 is the sole serial tail.

---

### Phase 154: QML Preset Diff-View Dialog

**Status:** Not started
**Workstream:** CLOS (PSET-05 closure)
**Goal:** Wire a QML side-by-side diff dialog that consumes the existing `PresetServiceMock::comparePresets(A, B)` primitive (shipped Phase 149 as a QVariantList API). The primitive exists; v5.1 wires the QML consumer that was deferred.
**Depends on:** —
**UI hint:** yes (new QML diff dialog reachable from the settings sidebar — `ui_safety_gate` applies)
**Requirements:** CLOS-01

Success criteria:
1. A QML side-by-side diff dialog is reachable from the settings sidebar; the user can select two presets (A, B) and the dialog renders the `{key, valueA, valueB, status}` entries produced by the existing `PresetServiceMock::comparePresets(A, B)` primitive as a 3-column visual diff (key / valueA / valueB) with added / removed / changed classification badges (mapped to upstream `UnsavedChangesDialog` diff view mode).
2. Selecting two presets that differ only in known keys produces a diff that exactly matches the upstream classification semantics (no extra keys, no missing keys, correct added/removed/changed status per row); selecting two identical presets produces an empty-diff state with a clear "no differences" affordance (no fabricated rows).
3. Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0; 5/5 ctest groups PASS (PrepareScene / PartPlate / ViewModel / UI / PreviewParser); the 12 v5.0 regression slots still pass — no regression in the Preset/Settings paths touched by the dialog wiring.

---

### Phase 155: Emboss 3MF Text Metadata Round-Trip

**Status:** Not started
**Workstream:** CLOS (EMB-06 closure)
**Goal:** Persist editable-text metadata via the upstream `TextConfigurationSerialization` path so that a TextEmboss volume saved to 3MF reloads as a re-editable TextEmboss (not just opaque geometry). Phase 146 shipped the geometry round-trip; v5.1 adds the `<text>` 3MF block.
**Depends on:** —
**UI hint:** no (C++ 3MF serialization round-trip; no new QML surfaces — the existing Emboss panel from Phase 145 reads the restored metadata)
**Requirements:** CLOS-02

Success criteria:
1. TextEmboss volumes persist editable-text metadata via the upstream `TextConfigurationSerialization` path; the 3MF `<text>` block (text content + font family + size + depth + style flags) is written on save and parsed on load, mapped to upstream `TextConfigurationSerialization` + the `<text>` 3MF metadata block.
2. Save→reload restores a TextEmboss volume as re-editable text with identical text + font + size + depth (not just opaque geometry); the reloaded volume opens in the existing Emboss panel showing the same values and re-extrudes correctly on edit.
3. A `testEmbossTextMetadataRoundTrip` ctest asserts text + font + size + depth match across save→reload (replacing/augmenting the Phase 146 geometry-only round-trip); canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0; 5/5 ctest groups PASS.

---

### Phase 156: Runtime Plate Thumbnail Capture Scheduler

**Status:** Not started
**Workstream:** CLOS (PLATE-05 closure)
**Goal:** Add the session-capture loop closed out of Phase 151 (which shipped persisted-plate thumbnails only). A new `Q_INVOKABLE setPlateThumbnailFromBase64(plateIndex, b64)` write path on ProjectServiceMock routes decoded QImages into `PartPlate::setThumbnail(QImage)`, and a capture scheduler iterates plates on content-change (or before save) via the existing `requestThumbnailCapture` path.
**Depends on:** —
**UI hint:** yes (PlateBar cards for non-current plates now render real in-session thumbnails — `ui_safety_gate` applies)
**Requirements:** CLOS-03

Success criteria:
1. A `Q_INVOKABLE setPlateThumbnailFromBase64(plateIndex, b64)` write path exists on ProjectServiceMock and routes the decoded QImage into `PartPlate::setThumbnail(QImage)`; a capture scheduler iterates plates on plate-content-change (or before save) and captures each via the existing `requestThumbnailCapture` path (Phase 151 shipped persisted-plate thumbnails; v5.1 adds the session-capture loop).
2. PlateBar cards for non-current plates render real thumbnails captured within the session (not blank); modifying a non-current plate's content triggers a re-capture that updates its card within the same session, before any save.
3. Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0; 5/5 ctest groups PASS; the 12 v5.0 regression slots (including the Phase 151 plate-thumbnail anchors) still pass — no regression in the persisted-thumbnail path.

---

### Phase 157: Live Multi-Plate Round-Trip ctest Fixture

**Status:** Not started
**Workstream:** CLOS (PLATE-06 closure)
**Goal:** Ship the `ProjectServiceMock` test fixture that was the unit-test harness gap forcing Phase 152 to source-audit-lock only. With the fixture, the PLATE-06 multi-plate round-trip ctest actually executes (rather than being a grep-locked assertion).
**Depends on:** —
**UI hint:** no (test-only — fixture + ctest)
**Requirements:** CLOS-04

Success criteria:
1. A `ProjectServiceMock` test fixture exists (the unit-test harness gap that forced Phase 152 to source-audit-lock only); the fixture constructs a multi-plate project (≥3 plates with at least one per-plate config override, one non-default print sequence, mixed bed types, and mixed locked/printable flags) without requiring GUI/runtime.
2. A live multi-plate save/reload ctest runs against the fixture and asserts the full plate state — count, names, per-plate config overrides, print sequence, bed type, locked/printable flags, and thumbnails — round-trips through 3MF; the previously source-audit-only PLATE-06 assertions now execute as a real ctest (the Phase 152 source-audit lock is either replaced or augmented by the live assertion).
3. Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0; 5/5 ctest groups PASS (the new ctest counts toward the PartPlate group); the 12 v5.0 regression slots still pass.

---

### Phase 158: Emboss Style Controls + SVG Advanced Features

**Status:** Not started
**Workstream:** EMBO-F (Emboss Follow-ups)
**Goal:** Wire the upstream `FontProp` style axes that Phase 145 surfaced minimally — boldness slider (the `font_prop.boldness` field already exists), italic flag, use-surface option, and curve-projection option — into the existing Emboss panel, and extend the existing `addSvgVolume` path (Phase 146 verified) with curve projection + depth modifier.
**Depends on:** —
**UI hint:** yes (extends the existing Emboss panel with style controls — `ui_safety_gate` applies)
**Requirements:** EMBO-F01, EMBO-F02

Success criteria:
1. Emboss style controls are wired to upstream `FontProp` axes — a boldness slider bound to `font_prop.boldness` (field already exists; UI now exposes it), an italic flag, and the use-surface + curve-projection options all surface in the existing Emboss panel and round-trip through the volume's FontProp (changes are reflected in the next extrude and survive a panel close/reopen in the same session).
2. SVG emboss advanced features — curve projection option + depth modifier — extend the existing `addSvgVolume` path (Phase 146 verified the baseline SVG path); an SVG loaded with curve-projection enabled wraps onto the selected surface, and the depth modifier scales the extrude depth predictably (a ctest asserts the SVG-with-depth-modifier volume's depth differs from the baseline by the modifier factor).
3. Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0; 5/5 ctest groups PASS; the 12 v5.0 regression slots (including the Phase 146 Emboss/SVG anchors) still pass — no regression in the baseline Emboss/SVG paths.

---

### Phase 159: v5.1 Cross-Workstream Regression Gate

**Status:** Not started
**Workstream:** Cross-cutting (REGRESS-05)
**Goal:** Consolidated v5.1 regression gate that re-asserts all v5.1 anchors (CLOS-01..04, EMBO-F01..02) AND re-asserts the v5.0/v4.8/v4.7/v4.6 anchors. This is the final phase — it depends on every feature phase.
**Depends on:** Phases 154-158 (all feature phases)
**UI hint:** no (test-only)
**Requirements:** REGRESS-05

Success criteria:
1. A consolidated `v51RegressionLocked` source-audit slot (in `QmlUiAuditTests` and/or `ViewModelSmokeTests`) re-asserts every v5.1 anchor from Phases 154-158; a single anchor regression fails the build.
2. The slot ALSO re-asserts the v5.0 (`v50RegressionLocked` from Phase 153), v4.8 (`v48CrossWorkstreamRegressionLocked`), v4.7, and v4.6 anchors — no regression introduced into earlier milestones by the v5.1 work; the 12 v5.0 regression slots still pass.
3. Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0; 5/5 ctest groups PASS (PrepareScene / PartPlate / ViewModel / UI / PreviewParser); app launch liveness confirmed (the executable reaches the Prepare page without error).

### Phase 160: Theme Token Foundation

**Status:** Not started
**Workstream:** DS (Design-System Foundation)
**Goal:** Make Theme.qml the single source of truth by adding all missing tokens + removing/wiring dead ones. Foundation for all later v5.2 phases.
**Depends on:** —
**UI hint:** no (token additions only; no QML consumer changes in this phase)
**Requirements:** DS-01

Success criteria:
1. Theme.qml gains ~25 missing tokens: statusErrorDark/Pressed, accentSubtlePressed, scrollBarColor, borderActive (referenced but undefined), fontMono, fontSize13, statusSeverity{0..9} palette (collapses 3 private notification severity tables), 11 missing sizing tokens (sliderTrackHeight, switchTrackWidth, dialogHeaderHeight, dialogFooterHeight, controlHeightXL, panelPaddingSM, sidebarWidthMin/Max/Default, rightPanelWidthMin/Max).
2. Dead tokens resolved: Theme.sidebarWidth=240 and Theme.panelPadding=12 (currently never read) are either wired to real consumers (in later phases) or removed with a comment explaining why.
3. Canonical token list documented in-code (Theme.qml header comment) so future audits can verify against it. Canonical build exits 0; 5/5 ctest groups PASS.

### Phase 161: Cx* Control Library Hardening

**Status:** Not started
**Workstream:** DS
**Goal:** Harden the Cx* primitives as the design-system carrier. Every control consumes Theme tokens, has unified state coverage, and ships no hardcodes.
**Depends on:** Phase 160 (needs the new tokens)
**UI hint:** yes (controls are visible everywhere — `ui_safety_gate` applies)
**Requirements:** DS-02, DS-03

Success criteria:
1. Qt.darker() runtime manipulation removed from CxButton.qml:31-32 and CxIconButton.qml:48 — replaced with the new statusErrorDark/Pressed and accentSubtlePressed tokens from Phase 160.
2. Disabled-state pattern unified across all 16 Cx* controls — one pattern (opacity 0.45 OR color branch), not both. Every interactive control has hover/press/disabled/focus states (CxButton gains press-scale; CxButton/CxPillAction gain focus border; ToolTip support expanded beyond CxIconButton).
3. CxSpinBox text no longer renders at 8px (fontSizeXS-2 below the XS=10 floor). 4 FAIL-class controls (CxPillAction/CxScrollView/CxSpinBox/CxDialog) raised to PASS per Controls-UI-REVIEW.md verdicts.
4. Zero hardcoded hex colors in Cx* controls; zero `font.pixelSize` literals in Cx* controls; zero missing `qsTr()` on user-visible strings in Cx* controls. Canonical build exits 0; 5/5 ctest groups PASS.

### Phase 162: Color Hardcode Sweep

**Status:** Not started
**Workstream:** TK (Token Migration)
**Goal:** Replace every hardcoded `#rrggbb` literal in pages/dialogs/components/panels with the matching Theme.* token.
**Depends on:** Phase 160 (new tokens available); Phase 161 (controls are token-compliant so cascading consumers inherit)
**UI hint:** yes (visible everywhere)
**Requirements:** TK-01

Success criteria:
1. PreferencesPage.qml: 129 hardcoded hex literals → Theme.* tokens (this is the worst-offender file flagged by Pages audit). Mechanical replacement confirmed by `grep -cE '#[0-9A-Fa-f]{3,8}' src/qml_gui/pages/PreferencesPage.qml` returning ≤5 (status-color allowlist only).
2. PreparePage: 46 literals → tokens. Dialogs: 228 literals across 14 dialogs → tokens. LeftSidebar private palette (6 colors) → Theme tokens. PresetDiffDialog status badges → Theme.statusSuccess/Error/Warning. ObjectList/StatusBar/GLToolbars hex literals → tokens.
3. Accent drift fixed everywhere — `#22c564` literals (drift from Theme.accent #18c75e) replaced with Theme.accent. Canonical build exits 0; 5/5 ctest groups PASS.

### Phase 163: Typography Hardcode Sweep

**Status:** Not started
**Workstream:** TK
**Goal:** Replace every `font.pixelSize` literal with Theme.fontSize* tokens; consolidate off-scale sizes.
**Depends on:** Phase 160 (fontSize13 token available); Phase 161 (Cx* controls compliant)
**UI hint:** yes
**Requirements:** TK-02

Success criteria:
1. PreparePage: 114/179 font.pixelSize literals → Theme.fontSize* tokens. Dialogs: 228 literals → tokens. Pages: 17 distinct sizes consolidated to the (now 7-token with fontSize13) scale.
2. Off-scale sizes eliminated: 8px (CxSpinBox) → 10px (XS floor); scattered 9px → 10px; 17× size 13 → fontSize13 token.
3. 26 `font.family: "Consolas"` hardcodes → Theme.fontMono across 8 component files. Canonical build exits 0; 5/5 ctest groups PASS.

### Phase 164: Spacing Sweep + Sidebar Width Fix

**Status:** Not started
**Workstream:** TK + SW
**Goal:** Introduce Theme.spacing*/panelPadding usage everywhere; unbreak the 7-layer 392px sidebar lock; make DockableSidebar drag handle functional.
**Depends on:** Phase 160 (sidebarWidthMin/Max/Default tokens available)
**UI hint:** yes (sidebar resize behavior is user-visible — `ui_safety_gate` applies)
**Requirements:** TK-03, SW-01

Success criteria:
1. Sidebar width system unbroken: the 7-layer 392px hardcode (4 QML + 3 C++ constexpr at BackendContext.h:467-469) replaced with the new sidebarWidthMin/Max/Default tokens. DockableSidebar drag handle actually resizes (no more `qBound(392, w, 392)` no-op). Phase 74 UI-SPEC "compact" requirement honored — 392 was a screenshot comment, not a spec mandate.
2. PreparePage: zero Theme.spacing* usage → consistent spacing-scale usage (worst pillar 1/4 in audit). main.qml/BBLTopbar/PreviewPage/PreferencesPage same. Arbitrary values (bottomMargin: 52, 5/8/10/14 mixed) replaced with the spacing scale.
3. Theme.panelPadding=12 either wired across panels or removed. Canonical build exits 0; 5/5 ctest groups PASS.

### Phase 165: Copywriting & Language Sweep

**Status:** Not started
**Workstream:** CW
**Goal:** One source language (Chinese per project positioning); no dev jargon; complete qsTr() coverage.
**Depends on:** —
**UI hint:** yes (every visible string)
**Requirements:** CW-01, CW-02

Success criteria:
1. One source language across all qsTr() strings. English-source dialogs (CreatePresets, PresetDiff, FilamentGroupPopup) swept to Chinese. English-in-Chinese-surface strings swept (PreparePage info bar Sliced/Stale/Ready/OK; MultiMachinePage 55 EN qsTr; CalibrationPage EN/zh splits; untranslated LAN/CP……). User never sees language flips between tabs/dialogs.
2. No developer-jargon strings in user-facing UI. Phase 158 Emboss tooltips (`Emboss.hpp`/`ProjectCurve`/上游/持久化) replaced with user-appropriate copy.
3. Cx* control library has complete qsTr() coverage on user-visible strings (was zero). Canonical build exits 0; 5/5 ctest groups PASS.

### Phase 166: Dialog Consistency Repair

**Status:** Not started
**Workstream:** Dlg
**Goal:** Make the 24 dialogs consistent: no empty headers, no pseudo-buttons, uniform button-row + modal policy.
**Depends on:** Phase 161 (CxButton/CxIconButton hardened)
**UI hint:** yes
**Requirements:** Dlg-01, Dlg-02

Success criteria:
1. All 8 empty-header dialogs fixed: `title:` → `dialogTitle:` in CreatePresets, ExportPresetBundle, NetworkTest, PresetDiff, SavePreset, SelectMachine, Troubleshoot, UnsavedChanges (CxDialog.qml:14 suppresses `title: ""` so they currently render a 44px header with only ✕).
2. No hand-rolled `Rectangle+Text+MouseArea` pseudo-buttons in dialogs — replaced with CxButton/CxIconButton. Button-row layout standardized using FilamentGroupPopup as the exemplar.
3. Modal/closePolicy normalized across the 24 dialogs (currently 8 modal / 11 NoAutoClose / 1 conditional / 1 escape-only / 1 NonModal / 2 unset chaos). Canonical build exits 0; 5/5 ctest groups PASS.

### Phase 167: Component Coherence

**Status:** Not started
**Workstream:** Cmp
**Goal:** Collapse the notification trio; resolve orphan components; unify parallel idioms.
**Depends on:** Phase 160 (statusSeverity palette); Phase 161 (Cx* base hardened)
**UI hint:** yes
**Requirements:** Cmp-01, Cmp-02, Cmp-03

Success criteria:
1. Notification system collapsed to one shared NotificationCard base + Theme.statusSeverity{0..9} palette. ErrorBanner/ErrorToast/NotificationCenter no longer maintain 3 private copies of the 10-level severity→color table. sev=1 no longer renders in both banner and toast simultaneously.
2. 4 orphan components resolved (CxPanel, CxSectionHeader, FilamentSlot, GroupNavSidebar — registered in qml.qrc with zero consumers) — either deleted or wired into actual use.
3. Parallel idioms unified: MoveSlider/PreviewLayerRail step-button idiom; LeftSidebar vs DockableSidebar redundancy resolved; OptionRow inline NumericEdit/Badge promoted to controls/ if broadly useful. Canonical build exits 0; 5/5 ctest groups PASS.

### Phase 168: Visual Control Migration

**Status:** Not started
**Workstream:** VS
**Goal:** Replace `Rectangle+Text+MouseArea` pseudo-buttons across pages with CxButton/CxIconButton; migrate Emboss Slider → CxSlider.
**Depends on:** Phase 161 (Cx* controls hardened)
**UI hint:** yes
**Requirements:** VS-01, VS-02

Success criteria:
1. No `Rectangle+Text+MouseArea` pseudo-buttons in pages — replaced with CxButton/CxIconButton. Verified by grep on MonitorPage (92 instances), MultiMachinePage (51), CalibrationPage (27), PreparePage gizmo action rows. Each replacement preserves tooltip/disabled/hover states via the Cx* control's contract.
2. Phase 158 boldness control migrated from raw `Slider` to `CxSlider` — consistent with every peer gizmo panel (Simplify, Support paint).
3. Card-in-card nesting violations (Phase 74) resolved. Canonical build exits 0; 5/5 ctest groups PASS.

### Phase 169: Experience Safety

**Status:** Not started
**Workstream:** XD
**Goal:** Every destructive action confirms before firing; async operations show progress; states are complete.
**Depends on:** Phase 166 (CxDialog base for the confirm component)
**UI hint:** yes
**Requirements:** XD-01, XD-02

Success criteria:
1. Every destructive action confirms before firing. A shared confirmation dialog component is built; 12+ triggers routed through it: PreparePage deleteSelection/removeAllOnPlate/deletePlate/clearAllPaintData; CaliHistoryDialog 清空; HomePage cloudUnbindDevice; MultiMachinePage removeDevice/stopAllLocalTasks/stopAllCloudTasks; MonitorPage disconnectDevice; ObjectList bulk delete.
2. Async Emboss has a progress/spinner affordance (was zero feedback). SliceProgress has indeterminate/canceled/error states (was missing).
3. Components-layer loading states backfilled where missing. Canonical build exits 0; 5/5 ctest groups PASS.

### Phase 170: v5.2 Cross-Workstream UI Regression Gate

**Status:** Not started
**Workstream:** Cross-cutting (REGRESS-06)
**Goal:** Consolidated v5.2 regression gate. Locks every UI contract from Phases 160-169 via source-audit slots AND re-asserts v5.1/v5.0/v4.x anchors.
**Depends on:** Phases 160-169 (all feature phases)
**UI hint:** no (test-only)
**Requirements:** REGRESS-06

Success criteria:
1. A consolidated `v52RegressionLocked` source-audit slot re-asserts every v5.2 UI contract: zero hardcoded hex in Cx* controls; zero font.pixelSize literals in Cx* controls; every destructive trigger wrapped in confirm; sidebar width uses real tokens; etc.
2. The slot ALSO re-asserts v5.1/v5.0/v4.8/v4.7/v4.6 anchors — no regression introduced into earlier milestones by the v5.2 work.
3. Canonical build exits 0; 5/5 ctest groups PASS; app launch liveness confirmed.

---

### Build Order (parallelism guidance)

- **Wave A (serial foundation, must complete first):** Phase 160 (Theme tokens) → Phase 161 (Cx* controls hardened). Phase 161 depends on 160.
- **Wave B (parallel, after A):** Phase 162 (color sweep) ‖ Phase 163 (typography sweep) ‖ Phase 165 (copywriting). Three independent mechanical sweeps.
- **Wave C (parallel with B):** Phase 164 (spacing + sidebar width). Touches BackendContext.h C++ — separate from pure-QML sweeps.
- **Wave D (after B+C):** Phase 166 (dialogs) ‖ Phase 167 (components) ‖ Phase 168 (visual control migration) ‖ Phase 169 (experience safety). All consume the hardened Cx* controls from Phase 161.
- **Wave E (tail, after all feature phases):** Phase 170 (REGRESS-06).

**Critical path:** 160 → 161 → {162|163|164|165|166|167|168|169} → 170. With parallelization, ~3 waves of execution; serially, 11 phases.

---

## Next Milestone

After v5.2 ships, the candidate backlog: de/fr/ja/ko translation long tail (~906 messages/lang remaining after v4.8 I18N-05); calibration `.drc` tower geometry (v4.6 CALIB tech debt); D3D12 default-backend promotion (deferred from v4.5). **SLA print path is DECLINED** (user decision 2026-07-19).

**Archives:** `.planning/milestones/v{version}-ROADMAP.md`, `v{version}-REQUIREMENTS.md`, `v{version}-MILESTONE-AUDIT.md`, `v{version}-phases/`.

---

*Last updated: 2026-07-19 via v5.2 planning — 11 phases (160-170), 14 requirements mapped to 9 workstreams, driven by 6 retroactive UI audits (`/gsd-ui-review` PreparePage + 5 parallel). SLA declined; quality-over-cost rule per user direction.*
