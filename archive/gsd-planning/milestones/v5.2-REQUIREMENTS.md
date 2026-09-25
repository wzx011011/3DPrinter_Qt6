# Requirements: OWzx Slicer — OrcaSlicer Qt6/QML Migration

**Defined:** 2026-07-19 (v5.2)
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code must inherit that behavior and must not invent new product behavior without an explicit upstream mapping or documented block.

## v5.2 Requirements — UI Excellence

**Milestone goal:** Drive every UI surface to consistent, design-system-backed excellence. App-wide audit baseline average = 12/24 (50%); v5.2 target = ≥20/24 (83%) on every surface.

**Quality rule (user 2026-07-19):** "不要在意实现难度和时间成本，只要最好的效果" — quality first, ignore cost/difficulty. Milestone sized by what's needed for UI excellence, not what fits one cycle.

**Scope rule:** All work is offline/local. SLA print path is DECLINED (user decision 2026-07-19). No new product behavior — only consistency, polish, and design-system enforcement of existing behavior. Maps to OrcaSlicer v7.0.1 upstream where applicable.

**Phases start at 160** (continuing from v5.1 phase 159).

---

### DS — Design-System Foundation

- [ ] **DS-01**: Theme.qml is the single source of truth for all design tokens. Missing tokens added (statusErrorDark/Pressed, accentSubtlePressed, scrollBarColor, borderActive, fontMono, fontSize13, statusSeverity{0..9} palette, 11 missing sizing tokens incl. sliderTrackHeight/switchTrackWidth/dialogHeaderHeight/sidebarWidthMin/Max/Default/rightPanelWidthMin/Max). Dead tokens removed or wired (sidebarWidth=240, panelPadding=12 — currently never read).
- [ ] **DS-02**: Cx* control library is hardened as the design-system carrier — Qt.darker() runtime manipulation removed; disabled-state pattern unified to one (opacity 0.45 OR color branch, not both); every interactive control has hover/press/disabled/focus states; CxSpinBox text no longer renders at 8px (below XS=10 floor); 4 FAIL-class controls (CxPillAction/CxScrollView/CxSpinBox/CxDialog) raised to PASS.
- [ ] **DS-03**: No hardcoded hex colors in Cx* controls; no `font.pixelSize` literals in Cx* controls; no `qsTr()` missing on user-visible strings in Cx* controls.

### TK — Token Migration (Color/Typography/Spacing)

- [ ] **TK-01**: Color hardcode sweep — every `#rrggbb` literal in pages/dialogs/components/panels replaced with the matching Theme.* token (PreferencesPage 129 literals, PreparePage 46, Dialogs 228, ObjectList/StatusBar/LeftSidebar private palettes, PresetDiffDialog status badges, GLToolbars viewport translucents). Allowlist for status colors only.
- [ ] **TK-02**: Typography hardcode sweep — every `font.pixelSize` literal replaced with Theme.fontSize* tokens (PreparePage 114 literals, Dialogs 228 literals, Pages 17 distinct sizes). Off-scale sizes (8px CxSpinBox, 9px scattered, 13px 17×) consolidated into the 6-token scale (with new fontSize13 token). 26 `font.family: "Consolas"` → Theme.fontMono.
- [ ] **TK-03**: Spacing hardcode sweep — introduce Theme.spacing*/panelPadding usage across all surfaces (PreparePage currently zero, PreferencesPage/main.qml/BBLTopbar/PreviewPage zero). Arbitrary values (bottomMargin: 52, 5/8/10/14 mixed) replaced with the spacing scale. Theme.panelPadding=12 either used everywhere or removed.

### SW — Sidebar Width System

- [ ] **SW-01**: Sidebar width system unbroken end-to-end. The 7-layer 392px hardcode (4 QML + 3 C++ constexpr at BackendContext.h:467-469) removed. DockableSidebar drag handle is no longer a visible no-op (`qBound(392, w, 392)` discards drags). Theme.sidebarWidth token (or new min/max/default tokens) drives a real, resizable sidebar. Phase 74 UI-SPEC "compact" requirement honored — 392 was a screenshot-measurement comment, not a spec mandate.

### CW — Copywriting & Language

- [ ] **CW-01**: One source language for all qsTr() strings (Chinese per project positioning). Sweep English-source dialogs (CreatePresets, PresetDiff, FilamentGroupPopup) to Chinese. Sweep English-in-Chinese-surface strings (PreparePage info bar Sliced/Stale/Ready/OK/non-manifold edges; MultiMachinePage 55 EN qsTr; CalibrationPage EN/zh splits; untranslated LAN/CP……). User never sees language flips between tabs/dialogs.
- [ ] **CW-02**: No developer-jargon strings in user-facing UI. Phase 158 Emboss tooltips (`Emboss.hpp`/`ProjectCurve`/上游/持久化) replaced with user-appropriate copy or removed.

### Dlg — Dialog Consistency

- [ ] **Dlg-01**: All 8 empty-header dialogs fixed (CreatePresets, ExportPresetBundle, NetworkTest, PresetDiff, SavePreset, SelectMachine, Troubleshoot, UnsavedChanges set `title:` instead of `dialogTitle:` — CxDialog.qml:14 suppresses `title: ""` so they render a 44px header with only ✕).
- [ ] **Dlg-02**: No hand-rolled `Rectangle+Text+MouseArea` pseudo-buttons in dialogs — all replaced with CxButton/CxIconButton. Button-row layout standardized (cancel/affirmative order, alignment, style) using FilamentGroupPopup as the exemplar. Modal/closePolicy normalized across the 24 dialogs (currently 8 modal / 11 NoAutoClose / 1 conditional / 1 escape-only / 1 NonModal / 2 unset chaos).

### Cmp — Component Coherence

- [ ] **Cmp-01**: Notification system collapsed to one shared NotificationCard base + Theme-level severity palette. ErrorBanner/ErrorToast/NotificationCenter no longer maintain 3 private copies of the 10-level severity→color table (~50 duplicated hex literals). sev=1 no longer renders in both banner and toast simultaneously.
- [ ] **Cmp-02**: 4 orphan components resolved (CxPanel, CxSectionHeader, FilamentSlot, GroupNavSidebar — registered in qml.qrc with zero consumers). Either deleted or wired into actual use.
- [ ] **Cmp-03**: Parallel idioms unified — MoveSlider/PreviewLayerRail step-button idiom; LeftSidebar vs DockableSidebar redundancy resolved; OptionRow's inline NumericEdit/Badge promoted to controls/ if broadly useful.

### VS — Visual Control Migration

- [ ] **VS-01**: No `Rectangle+Text+MouseArea` pseudo-buttons in pages — replaced with CxButton/CxIconButton (MonitorPage 92 instances, MultiMachinePage 51, CalibrationPage 27, PreparePage gizmo action rows).
- [ ] **VS-02**: Phase 158 boldness control migrated from raw `Slider` to `CxSlider` (every peer gizmo panel — Simplify, Support paint — already uses CxSlider).

### XD — Experience Safety

- [ ] **XD-01**: Every destructive action confirms before firing. 12+ triggers routed through a shared confirmation dialog component: PreparePage deleteSelection/removeAllOnPlate/deletePlate/clearAllPaintData; CaliHistoryDialog 清空; HomePage cloudUnbindDevice; MultiMachinePage removeDevice/stopAllLocalTasks/stopAllCloudTasks; MonitorPage disconnectDevice; ObjectList bulk delete.
- [ ] **XD-02**: Async feedback affordances — Emboss async has a progress/spinner; SliceProgress has indeterminate/canceled/error states (currently missing); components-layer loading states backfilled where missing.

### Cross-Workstream

- [ ] **REGRESS-06**: v5.2 regression gate. A `v52RegressionLocked` source-audit slot re-asserts every UI contract from Phases 160-169 (zero hardcoded hex in Cx* controls; zero font.pixelSize literals in Cx* controls; every destructive trigger wrapped in confirm; etc.). Re-asserts v5.1/v5.0/v4.x anchors. Canonical build exits 0 + 5/5 ctest groups PASS.

---

## Deferred / Declined

### SLA print path — DECLINED (user decision 2026-07-19)

**VDB-06 / SLA-01..06 / Hollow 3MF persistence / SlaSupports gizmo / FaceDetector gizmo** — all reclassified from tech_debt to **declined-with-reason**.

**Reason:** OrcaSlicer itself does not push SLA (it's inherited PrusaSlicer tech debt — Prusa makes the SL1 resin printer, but Bambu/OrcaSlicer only target FDM). OWzx's audience is FDM users, who have no hollowing need (FDM uses infill, not hollowing). The maintenance burden (SLA presets, .sl1 export, PrinterTechnology dispatch, ongoing SLA support) is disproportionate to user value. Hollow UI scaffolding from Phase 142/143 stays in place but has no slice execution path.

### Future (post-v5.2 candidate backlog)

- de/fr/ja/ko translation long tail (~906 messages/lang remaining after v4.8 I18N-05) — content work, folds into v5.2 CW-01 if scope allows.
- Calibration `.drc` tower geometry (v4.6 CALIB tech debt) — not UI; deferred.
- D3D12 default-backend promotion (deferred from v4.5) — rendering, not UI; deferred.

## Out of Scope

Explicitly excluded from v5.2. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| SLA print path / SlaSupports / FaceDetector | DECLINED — see above |
| LAN device discovery, device send/upload, MQTT/SSDP | Removed from forward scope by user direction 2026-07-07. |
| Cloud print, cloud account/sync, OAuth | Removed from forward scope by user direction 2026-07-07. |
| Monitor task lifecycle, ModelMall/Home WebView | Removed from forward scope by user direction 2026-07-07. |
| Live camera / RTSP / WebRTC streams | Removed from forward scope; FFmpeg/WebRTC deps unavailable. |
| Printer-connected hardware workflows | Hardware-dependent; stays out under printer-hardware removal rule. |
| libslic3r slicing algorithm changes | Out of scope — GUI migration only. |
| New build directories or non-canonical build scripts | Build rules: only `build/` + `scripts/auto_verify_with_vcvars.ps1`. |
| New product behavior | v5.2 is consistency/polish only — no new behavior. |

## Traceability

Populated by gsd-roadmapper during ROADMAP.md creation.
