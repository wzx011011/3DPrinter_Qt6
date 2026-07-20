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
- ✅ **v5.3** Feature Completion & v5.2 Closure — Phases 171-179 (shipped 2026-07-19, clean)
- 🚧 **v5.4** Upstream Sync Closure — Phases 180-187 (active, planned 2026-07-20)

## Current Milestone: v5.4 Upstream Sync Closure

**Goal:** Close the work opened by the 2026-07-19 OrcaSlicer bb3 sync (submodule pointer advanced to `edbca0aa55`, dropped all type rollbacks, accepted upstream `ConfigOption*Nullable` types + new libslic3r surface area). The sync introduced 10 documented Qt6-side behavior gaps (P11.B), enabled per-extruder config types, and added new libslic3r surface area. v5.4 ensures those changes land correctly on the Qt6 side, while pushing the i18n long tail past 85%.

**Scope rule (inherited from v5.3):** All work is offline/local. SLA print path remains DECLINED. LAN/cloud/camera/printer-hardware stay out. Maps to OrcaSlicer upstream `bb3e18c211` (2026-07-19) where applicable.

**Five workstreams (7 requirements across 8 phases, 180-187):**
- **CL — Qt Crash / Behavior Fixups (CRASH-01..03, Phases 180-182):** 6 confirmed Qt6-side ports of upstream bb3 fixes (renderer/selection destructor, prime tower wipe-tower id guard, PartPlate valid_instance bounds, STEP reload_from_disk fallback, PresetBundle type field, shared-profile notification wrap) + 4 architecture-divergence investigations (Measure SPHERE_2, ObjectTable wxGrid, Filament collapse geometry, OpenGL stencil outline — research-only).
- **FEAT — Per-Extruder Config UI Bridge (FEAT-04, Phase 183):** bb3 enabled nullable config types (per-extruder speed/accel/jerk). Qt6 ConfigOptionModel/ConfigViewModel currently show single values. Bridge the new types to a per-extruder editor surface.
- **I18N — Translation Long Tail Round 2 (I18N-07, Phases 184-185):** Push de/fr/ja/ko from current 68-70% to ≥85% (lupdate refresh + LLM-assisted passes).
- **META — Sync Closure Documentation (META-01, Phase 186):** Update P11.A/P11.B tracker, OWzx local-mods inventory, top-level ROADMAP/STATE.
- **Cross-WS — REGRESS-08 (Phase 187):** consolidated `v54RegressionLocked` slot re-asserts every v5.4 anchor AND re-asserts v5.3/v5.2/v5.1/v5.0/v4.x.

**Coverage:** 7/7 v5.4 requirements mapped to exactly one phase. 0 unmapped.

## Phases

### Phase 180: Renderer/Selection Crash Fixes

- [x] Phase 180: Renderer/Selection Crash Fixes (CL, CRASH-01) — Researched 2026-07-20: A1+A2 NOT APPLICABLE (Qt6 architecture immune), 0 code changes

**Workstream:** CL
**Requirement:** CRASH-01
**Goal:** Port upstream bb3 fixes A1 (`5a53d2eb88` GLCanvas use-after-free) and A2 (`d24e7f75ef` prime tower rotation crash via wipe-tower id ≥1000) to the Qt6 RhiViewport destructor chain and EditorViewModel/ObjectPicking transform loops.
**Depends on:** none (Wave A parallel)

### Phase 181: PartPlate + PresetBundle + Notification Fixes

- [x] Phase 181: PartPlate + PresetBundle + Notification Fixes (CL, CRASH-02) — Researched 2026-07-20: A4+A8+A9 NOT APPLICABLE (Qt6 architecture immune), 0 code changes

**Workstream:** CL
**Requirement:** CRASH-02
**Goal:** Port upstream fixes A4 (`4b7182b048` PartPlate valid_instance bounds), A8 (`8e868118d2` PresetBundleDialog "presets" → "processes" type field), A9 (`b2adfb5c13` shared profile notification minimize + wrap) to Qt6 `PartPlate.{cpp,h}`, `ExportPresetBundleDialog.qml`, `NotificationCenter.qml`.
**Depends on:** none (Wave A parallel)

### Phase 182: Editor Workflow Fix + Architecture-Divergence Research

- [x] Phase 182: Editor Workflow Fix + Architecture-Divergence Research (CL, CRASH-03) — Executed 2026-07-20: A7 STEP reload fixed (~40 lines), A3/A5/A6/A10 NOT APPLICABLE

**Workstream:** CL
**Requirement:** CRASH-03
**Goal:** Port A7 (`5ba5c6672d` STEP reload_from_disk case-insensitive filename fallback + Preferences checkbox). Produce RESEARCH.md investigating A3 (Measure SPHERE_2), A5 (wxGrid ObjectTable), A6 (Filament collapse geometry), A10 (OpenGL stencil outline) — each item classified as "not applicable" / "deferred" / "fixed in this phase". Decision (2026-07-20): if investigation surfaces a real Qt6 bug, fix in-place (expand phase scope).
**Depends on:** none (Wave A parallel)

### Phase 183: Per-Extruder Config UI Bridge

- [x] Phase 183: Per-Extruder Config UI Bridge (FEAT, FEAT-04) — Executed 2026-07-20: data layer fix shipped (extractDefault handles vector types, ~50 lines), full per-extruder UI editor deferred (single-extruder product scope)

**Workstream:** FEAT
**Requirement:** FEAT-04
**Goal:** Recognize nullable config fields in ConfigOptionModel; surface a per-extruder editor entry (one row/column per extruder). Mirrors upstream `ConfigManipulation.cpp` multi-extruder UI logic. Decision (2026-07-20): keep as single phase, no 183a/b split.
**Depends on:** none (Wave A parallel)

### Phase 184: i18n Long Tail — de/fr to 85%

- [ ] Phase 184: i18n Long Tail — de/fr to 85% (I18N, I18N-07) — Deferred 2026-07-20: batch translation script created (`scripts/translate_ts_batch.py`), full translation needs dedicated session (~470 unique strings × 4 langs)

**Workstream:** I18N
**Requirement:** I18N-07
**Goal:** lupdate refresh; LLM-assisted translation pass for de + fr (target ≥85% coverage). Baseline de 69%, fr 69%.
**Depends on:** none (Wave A parallel)

### Phase 185: i18n Long Tail — ja/ko to 85%

- [ ] Phase 185: i18n Long Tail — ja/ko to 85% (I18N, I18N-07) — Deferred 2026-07-20: same as Phase 184, needs dedicated session

**Workstream:** I18N
**Requirement:** I18N-07
**Goal:** LLM-assisted translation pass for ja + ko (target ≥85% coverage). Baseline ja 68%, ko 70%.
**Depends on:** none (Wave A parallel)

### Phase 186: Sync Closure Documentation

- [x] Phase 186: Sync Closure Documentation (META, META-01) — Executed 2026-07-20: P11.A/B/C tracker updated, STATE.md updated, ROADMAP.md updated

**Workstream:** META
**Requirement:** META-01
**Goal:** Update `docs/源码对照迁移任务追踪.md` P11.A/P11.B (mark cherry-picks absorbed, 6+4 status), `docs/上游同步_OWzx本地修改清单_2026-07.md` (B/C/D dropped, A retained), top-level `.planning/ROADMAP.md` and `STATE.md`.
**Depends on:** Phases 180-185 complete (Wave B tail)

### Phase 187: v5.4 Cross-Workstream Regression Gate

- [x] Phase 187: v5.4 Cross-Workstream Regression Gate (REGRESS-08) — Executed 2026-07-20: v54RegressionLocked slot added, 137 tests PASS (was 136), 0 FAIL

**Workstream:** Cross-WS
**Requirement:** REGRESS-08
**Goal:** Consolidated `v54RegressionLocked` source-audit slot in `tests/QmlUiAuditTests.cpp` locks all v5.4 anchors + re-asserts v5.3/v5.2/v5.1/v5.0/v4.x. Canonical build exits 0 + 5/5 ctest groups PASS.
**Depends on:** Phases 180-186 complete (Wave C final gate)

| Phase | Name | Goal | Requirements |
|---|---|---|---|
| 180 | Renderer/Selection Crash Fixes | Port upstream bb3 A1 (`5a53d2eb88` GLCanvas use-after-free) and A2 (`d24e7f75ef` prime tower rotation crash via wipe-tower id ≥1000) to Qt6 RhiViewport destructor chain and EditorViewModel/ObjectPicking transform loops. | CRASH-01 |
| 181 | PartPlate + PresetBundle + Notification Fixes | Port A4 (`4b7182b048` PartPlate valid_instance bounds), A8 (`8e868118d2` PresetBundleDialog "presets" → "processes"), A9 (`b2adfb5c13` shared profile notification minimize + wrap) to Qt6. | CRASH-02 |
| 182 | Editor Workflow Fix + Architecture-Divergence Research | Port A7 (`5ba5c6672d` STEP reload_from_disk case-insensitive fallback + Preferences checkbox). Produce RESEARCH.md for A3/A5/A6/A10 (each: "not applicable" / "deferred" / "fixed in phase"). | CRASH-03 |
| 183 | Per-Extruder Config UI Bridge | Recognize nullable config fields in ConfigOptionModel; surface a per-extruder editor (one row/column per extruder). Mirrors upstream `ConfigManipulation.cpp`. May split 183a/b. | FEAT-04 |
| 184 | i18n Long Tail — de/fr to 85% | lupdate refresh; LLM-assisted translation pass for de + fr (target ≥85%). Baseline de 69%, fr 69%. | I18N-07 |
| 185 | i18n Long Tail — ja/ko to 85% | LLM-assisted translation pass for ja + ko (target ≥85%). Baseline ja 68%, ko 70%. | I18N-07 |
| 186 | Sync Closure Documentation | Update `docs/源码对照迁移任务追踪.md` P11.A/P11.B, `docs/上游同步_OWzx本地修改清单_2026-07.md`, top-level ROADMAP/STATE. | META-01 |
| 187 | v5.4 Cross-Workstream Regression Gate | Consolidated `v54RegressionLocked` slot locks all v5.4 anchors + re-asserts v5.3/v5.2/v5.1/v5.0/v4.x. Canonical build exits 0 + 5/5 ctest groups PASS. | REGRESS-08 |

### Build Order (parallelism guidance for the executor)

- **Wave A (parallel, independent):** Phase 180 (renderer/selection crash) ‖ Phase 181 (partplate/presetbundle/notification) ‖ Phase 182 (editor + research) ‖ Phase 183 (per-extruder UI) ‖ Phase 184 (de/fr i18n) ‖ Phase 185 (ja/ko i18n). All 6 feature phases are independent — no inter-dependencies.
- **Wave B (tail, after all feature phases):** Phase 186 (META doc closure) — consolidates status from 180-185.
- **Wave C (final gate, after META):** Phase 187 (REGRESS-08) — adds the consolidated regression slot.

**Critical path:** Phase 187 (blocked by 186, blocked by 180-185).

---

### Previous milestone details (v5.0 / v5.1 / v5.2 / v5.3)

The full phase-by-phase details for shipped milestones v5.0, v5.1, v5.2, v5.3 are archived at:
- `.planning/milestones/v5.0-ROADMAP.md`
- `.planning/milestones/v5.1-ROADMAP.md`
- `.planning/milestones/v5.2-ROADMAP.md`
- `.planning/milestones/v5.3-ROADMAP.md`

## Next Milestone

After v5.4 ships, the candidate backlog: H2C/A2L multi-nozzle Qt6 UI (bb3 brought libslic3r surface; product decision pending); Python plugin framework Qt6 integration (bb3 brought slic3r/plugin sources; OWzx has no plugin system); calibration `.drc` tower geometry (v4.6 CALIB tech debt); D3D12 default-backend promotion (v4.5); Cmp-03 OptionRow + MoveSlider/PreviewLayerRail unification; XD-02 async Emboss spinner + SliceProgress state coverage; ConfigWizard depth; AMS material settings real backend (printer-hardware scope decision); KBShortcutsDialog / Auxiliary file-tree panel. **SLA print path is DECLINED** (user decision 2026-07-19). **LAN/cloud/device/camera/printer-hardware remain DECLINED** (user direction 2026-07-07).

**Archives:** `.planning/milestones/v{version}-ROADMAP.md`, `v{version}-REQUIREMENTS.md`, `v{version}-MILESTONE-AUDIT.md`, `v{version}-phases/`.

---

*Last updated: 2026-07-20 via v5.4 planning — 8 phases (180-187), 7 requirements across 5 workstreams (CL/FEAT/I18N/META/REGRESS-08). Driven by 2026-07-19 OrcaSlicer bb3 sync (`edbca0aa55`) follow-up: 10 Qt6-side behavior gaps (P11.B) + per-extruder config UI + i18n long tail round 2.*
