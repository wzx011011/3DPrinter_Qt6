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
- 🚧 **v5.1** v5.0 Deferred Items Closure — Phases 154-159 (in planning)

## Current Milestone: v5.1 v5.0 Deferred Items Closure

**Goal:** Close the 4 documented v5.0 partials (PSET-05 / EMB-06 / PLATE-05 / PLATE-06) plus the Emboss style/SVG follow-ups. Each item is a 1-phase completion of a primitive already shipped in v5.0 — low-risk closures, no new architecture.

**Scope rule:** All work is offline/local and maps to OrcaSlicer v7.0.1 upstream. SLA print path is explicitly deferred to v5.2 (a dedicated SLA milestone — research at `.planning/research/sla-scope.md` confirms it is ~4 phases for VDB-06 close, not the 8-10 previously estimated; libslic3r SLA files are already compiled and linked). LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows remain removed.

**Five workstreams (7 requirements across 6 phases, 154-159):**
- **CLOS — v5.0 Partial Closures (CLOS-01..04, Phases 154-157):** four independent 1-phase closures, each completing a primitive already shipped in v5.0 — QML diff-view consumer for the existing `comparePresets` (PSET-05); 3MF `<text>` metadata round-trip via `TextConfigurationSerialization` (EMB-06); runtime thumbnail capture scheduler + `setPlateThumbnailFromBase64` write path (PLATE-05); live multi-plate round-trip ctest via a `ProjectServiceMock` fixture (PLATE-06).
- **EMBO-F — Emboss Follow-ups (EMBO-F01 + EMBO-F02, Phase 158):** style controls wired to upstream `FontProp` axes (boldness/italic/use-surface/curve-projection) + SVG advanced features (curve projection + depth modifier on the existing `addSvgVolume` path). Combined into a single phase per granularity=standard.
- **Cross-WS — REGRESS-05 (Phase 159):** consolidated v5.1 regression gate.

**Coverage:** 7/7 v5.1 requirements mapped to exactly one phase (CLOS-01→154, CLOS-02→155, CLOS-03→156, CLOS-04→157, EMBO-F01+F02→158, REGRESS-05→159). 0 unmapped.

## Phases

- [ ] Phase 154: QML Preset Diff-View Dialog (CLOS, PSET-05 closure)
- [ ] Phase 155: Emboss 3MF Text Metadata Round-Trip (CLOS, EMB-06 closure)
- [ ] Phase 156: Runtime Plate Thumbnail Capture Scheduler (CLOS, PLATE-05 closure)
- [ ] Phase 157: Live Multi-Plate Round-Trip ctest Fixture (CLOS, PLATE-06 closure)
- [ ] Phase 158: Emboss Style Controls + SVG Advanced Features (EMBO-F)
- [ ] Phase 159: v5.1 Cross-Workstream Regression Gate (REGRESS-05)

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

---

## Next Milestone

After v5.1 ships, the next milestone is **v5.2 SLA Print Path** — wire `SLAPrint` into SliceService via `PrinterTechnology` dispatch (currently FFF-only), close VDB-06, and unblock SlaSupports + FaceDetector. Research at `.planning/research/sla-scope.md` confirms the libslic3r SLA surface is already compiled and linked (the v5.0 audit's "wiring from scratch ~35 files" premise was wrong — those files already build); the real work is ~4 phases of Qt orchestration: (1) `PrinterTechnology` dispatch in SliceService + SL1Archive output + forward hollow Q_PROPERTYs; (2-3) SLA preset schema minimal (1 printer + 1 material + 1 print synthesized; `printer_technology` config option read); (4) `.sl1` output dialog + `exportArchiveToPath` API + verify VDB-06 cavity. SlaSupports gizmo port (1237 lines + RHI point rendering) is a follow-on cluster.

Other candidate backlog (post-v5.1): de/fr/ja/ko translation long tail (~906 messages/lang remaining after v4.8 I18N-05); calibration `.drc` tower geometry (v4.6 CALIB tech debt); D3D12 default-backend promotion (deferred from v4.5).

**Archives:** `.planning/milestones/v{version}-ROADMAP.md`, `v{version}-REQUIREMENTS.md`, `v{version}-MILESTONE-AUDIT.md`, `v{version}-phases/`.

---

*Last updated: 2026-07-17 via `/gsd:roadmapper` — v5.1 roadmap created (6 phases, 154-159, 7 requirements mapped, 0 unmapped).*
