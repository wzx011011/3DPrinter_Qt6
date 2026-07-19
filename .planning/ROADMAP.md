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
- ✅ **v5.2** UI Excellence — Phases 160-170 (shipped 2026-07-19, clean)
- 🚧 **v5.3** Feature Completion & v5.2 Closure — Phases 171-179 (in planning)

## Current Milestone: v5.3 Feature Completion & v5.2 Closure

**Goal:** Three parallel tracks in one cycle — (1) close the v5.2 BLOCKERs that were honestly deferred (destructive confirms + dialog spacing + pseudo-button sweep); (2) ship the 3 functional feature gaps identified by the v5.3 code-level feature-parity audit (per-object settings dialog, object-layer range editor, simplify mesh gizmo); (3) attack the i18n long tail (de/fr/ja/ko). The user explicitly asked to "把剩下的工作全都规划到下个里程碑里边，一起完成".

**Scope rule:** All work is offline/local. SLA print path remains DECLINED. LAN/cloud/camera/printer-hardware stay out. Maps to OrcaSlicer v7.0.1 upstream where applicable.

**Four workstreams (8 requirements across 9 phases, 171-179):**
- **CL — v5.2 BLOCKER Closure (CL-01..03, Phases 171-173):** destructive-action confirm sweep (11 remaining triggers); dialog spacing sweep (23/24 dialogs use zero spacing tokens); pseudo-button sweep (~13 hand-converted sites).
- **FEAT — Functional Gaps (FEAT-01..03, Phases 174-176):** per-object settings override dialog (backend ready, UI missing); object layer-range editor (backend fully ready, UI missing); Simplify mesh gizmo (currently explicit stub — wire real QuadricEdgeCollapse).
- **I18N — Translation Long Tail (I18N-06, Phases 177-178):** lupdate refresh + per-language LLM-assisted translation passes (de/fr/ja/ko split across 2 phases for parallelism).
- **Cross-WS — REGRESS-07 (Phase 179):** consolidated v5.3 regression gate.

**Coverage:** 8/8 v5.3 requirements mapped to exactly one phase. 0 unmapped.

## Phases

- [ ] Phase 171: Destructive-Action Confirm Sweep (CL, CL-01)
- [ ] Phase 172: Dialog Spacing Sweep (CL, CL-02)
- [ ] Phase 173: Pseudo-Button Sweep (CL, CL-03)
- [ ] Phase 174: Per-Object Settings Override Dialog (FEAT, FEAT-01)
- [ ] Phase 175: Object Layer-Range Editor (FEAT, FEAT-02)
- [ ] Phase 176: Simplify Mesh Gizmo (FEAT, FEAT-03)
- [ ] Phase 177: i18n Long Tail — de/fr (I18N, I18N-06)
- [ ] Phase 178: i18n Long Tail — ja/ko (I18N, I18N-06)
- [ ] Phase 179: v5.3 Cross-Workstream Regression Gate (REGRESS-07)

| Phase | Name | Goal | Requirements |
|---|---|---|---|
| 171 | Destructive-Action Confirm Sweep | Route the 11 remaining destructive triggers through the existing ConfirmDialog component (CaliHistory 清空; HomePage cloudUnbindDevice; MultiMachinePage removeDevice/stopAllLocalTasks/stopAllCloudTasks; MonitorPage disconnectDevice; ObjectList bulk delete; etc.) | CL-01 |
| 172 | Dialog Spacing Sweep | Script-based mechanical migration of arbitrary spacing/margin values across 23 dialogs to the Theme.spacing* scale (mirrors Phase 162/163 color/typography sweep pattern) | CL-02 |
| 173 | Pseudo-Button Sweep | Hand-convert the ~13 true Rectangle+Text+MouseArea pseudo-button candidates to CxButton/CxIconButton (each requires site-specific context) | CL-03 |
| 174 | Per-Object Settings Override Dialog | Ship the QML inspector that opens on right-click → shows the object/volume's print/filament params, allows per-object override. Mirrors upstream `GUI_ObjectSettings`. Backend APIs already exist. | FEAT-01 |
| 175 | Object Layer-Range Editor | Ship the QML dialog to add/edit/delete per-layer-height ranges. Mirrors upstream `GUI_ObjectLayers`. Backend fully ready (`objectLayerRanges`/`setLayerRangeValue`); UI-only addition. | FEAT-02 |
| 176 | Simplify Mesh Gizmo | Replace the explicit stub at `EditorViewModel.cpp:1355` with a real implementation calling libslic3r's `QuadricEdgeCollapse::quadric_edge_collapse`. Add SimplifyDialog for preview/apply. | FEAT-03 |
| 177 | i18n Long Tail — de/fr | Refresh .ts via lupdate; LLM-assisted translation pass for de + fr (target ≥85% coverage). | I18N-06 |
| 178 | i18n Long Tail — ja/ko | LLM-assisted translation pass for ja + ko (target ≥85% coverage). | I18N-06 |
| 179 | v5.3 Cross-Workstream Regression Gate | Consolidated v53RegressionLocked slot locks all v5.3 anchors + re-asserts v5.2/v5.1/v5.0/v4.x. | REGRESS-07 |

### Build Order (parallelism guidance for the executor)

- **Wave A (parallel, independent):** Phase 171 (destructive confirms) ‖ Phase 172 (dialog spacing sweep) ‖ Phase 173 (pseudo-button sweep) ‖ Phase 174 (per-object settings) ‖ Phase 175 (layer-range editor) ‖ Phase 176 (Simplify gizmo) ‖ Phase 177 (de/fr i18n) ‖ Phase 178 (ja/ko i18n). All 8 feature phases are independent — no inter-dependencies.
- **Wave B (tail, after all feature phases):** Phase 179 (REGRESS-07) — consolidates anchors from all of 171-178.

**Critical path:** any of {171..178}

---

### Previous milestone details (v5.0 / v5.1 / v5.2)

The full phase-by-phase details for shipped milestones v5.0, v5.1, v5.2 are archived at:
- `.planning/milestones/v5.0-ROADMAP.md`
- `.planning/milestones/v5.1-ROADMAP.md`
- `.planning/milestones/v5.2-ROADMAP.md`

## Next Milestone

After v5.3 ships, the candidate backlog: calibration `.drc` tower geometry (v4.6 CALIB tech debt); D3D12 default-backend promotion (deferred from v4.5); Cmp-03 OptionRow + MoveSlider/PreviewLayerRail unification; XD-02 async Emboss spinner + SliceProgress state coverage; ConfigWizard depth; AMS material settings real backend (printer-hardware scope decision); KBShortcutsDialog / Auxiliary file-tree panel. **SLA print path is DECLINED** (user decision 2026-07-19).

**Archives:** `.planning/milestones/v{version}-ROADMAP.md`, `v{version}-REQUIREMENTS.md`, `v{version}-MILESTONE-AUDIT.md`, `v{version}-phases/`.

---

*Last updated: 2026-07-19 via v5.3 planning — 9 phases (171-179), 8 requirements across 4 workstreams (CL/FEAT/I18N/REGRESS-07). Driven by v5.3 code-level feature-parity audit (per-object settings, layer-range editor, Simplify gizmo) + v5.2 deferred BLOCKERs + i18n long tail.*
