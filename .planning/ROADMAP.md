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
- Planned: **v4.0 Preview Page UI Restoration** - Phases 79-83

## Current Milestone: v4.0 Preview Page UI Restoration

**Goal:** Restore the Preview page to screenshot-level OrcaSlicer parity using `shotScreen/预览页.png` as visual truth and OrcaSlicer Preview/G-code source as behavior truth.

## Phases

- [x] Phase 79: Preview Source-Truth Gap Audit (completed 2026-07-06)
- [x] Phase 80: Preview Layout And Panels Restoration (completed 2026-07-06)
- [ ] Phase 81: Preview Layer Move And Playback Controls
- [ ] Phase 82: Preview G-code Roles Color Modes And Rendering
- [ ] Phase 83: Preview Verification And Cleanup

| Phase | Name | Goal | Requirements |
|---|---|---|---|
| 79 | Preview Source-Truth Gap Audit | Freeze the v4.0 Preview region map, current gaps, upstream anchors, Qt targets, and verification expectations before edits. | PVAUDIT-01 |
| 80 | Preview Layout And Panels Restoration | Restore the screenshot-visible Preview layout, statistics/legend panels, and remove visible placeholders/dead surfaces. | PVLAYOUT-01, PVLAYOUT-02, PVLAYOUT-03 |
| 81 | Preview Layer Move And Playback Controls | Restore layer range/current-layer controls, move/layer playback controls, and camera interaction stability. | PVCTRL-01, PVCTRL-02, PVCTRL-03 |
| 82 | Preview G-code Roles Color Modes And Rendering | Align G-code role colors, role visibility, color modes, and payload preservation with upstream semantics. | PVRENDER-01, PVRENDER-02, PVRENDER-03 |
| 83 | Preview Verification And Cleanup | Remove stale Preview paths, lock restored bindings with audits/tests, run canonical verifier, launch app, and capture Preview visual evidence. | PVCLEAN-01, PVVERIFY-01, PVVERIFY-02 |

### Phase 79: Preview Source-Truth Gap Audit

**Status:** Complete
**Plans:** 1/1 plans complete

Success criteria:
1. Current Preview page regions are mapped to target screenshot areas and OrcaSlicer source files.
2. Each region records Qt target files, modify-vs-replace decision, and verification method.
3. v3.6/v3.7 residual Preview gaps are reconciled into v4.0 requirements or explicitly deferred.

### Phase 80: Preview Layout And Panels Restoration

**Status:** Complete
**Plans:** 1/1 plans complete

Success criteria:
1. Preview top controls, viewport, side panels, layer slider, and bottom playback/status controls match screenshot layout without overlap.
2. Statistics, metadata, estimates, and legend surfaces use upstream-like density and placement.
3. Visible Preview placeholders, raw labels, and dead controls are removed or honestly gated.

### Phase 81: Preview Layer Move And Playback Controls

**Status:** Not started
**Plans:** 0/1 plans complete

Success criteria:
1. Layer range/current-layer controls update PreviewViewModel and renderer state.
2. Layer/move playback and stepping controls stay synchronized with backend Preview state.
3. Camera rotate/pan/zoom/fit interactions do not make model or toolpath data disappear.

### Phase 82: Preview G-code Roles Color Modes And Rendering

**Status:** Not started
**Plans:** 0/1 plans complete

Success criteria:
1. Role colors and role visibility controls map to OrcaSlicer/libvgcode source truth.
2. Screenshot-visible color modes are wired or honestly gated when blocked.
3. Prepare -> slice -> Preview -> adjust layer/range/roles -> return flow preserves G-code payload state.

### Phase 83: Preview Verification And Cleanup

**Status:** Not started
**Plans:** 0/1 plans complete

Success criteria:
1. Replaced Preview UI paths leave no stale files, imports, resource entries, tests, or disconnected controls.
2. Automated source/QML audits cover upstream mapping, placeholder removal, and required bindings.
3. The canonical verifier passes, `build/OWzxSlicer.exe` launches, and Preview visual evidence is recorded against the target screenshot.

## Completed Milestones

<details>
<summary>v3.9 Prepare Page UI Restoration (Phases 74-78) - SHIPPED 2026-07-06</summary>

- [x] Phase 74: Prepare Source-Truth Gap Audit (completed 2026-07-05)
- [x] Phase 75: Prepare Sidebar Restoration (completed 2026-07-05)
- [x] Phase 76: Prepare Workflow Panels Restoration (completed 2026-07-05)
- [x] Phase 77: Prepare Viewport Controls And Gizmo UI (completed 2026-07-05)
- [x] Phase 78: Prepare Verification And Cleanup (completed 2026-07-05)

Archive: `.planning/milestones/v3.9-ROADMAP.md`
Audit: `.planning/milestones/v3.9-MILESTONE-AUDIT.md`
Requirements: `.planning/milestones/v3.9-REQUIREMENTS.md`
Phases: `.planning/milestones/v3.9-phases/`

</details>

## Archived Phase Groups

<details>
<summary>v3.6 Screenshot-Driven OrcaSlicer UI Restoration (Phases 50-58) - SHIPPED 2026-07-03</summary>

- [x] Phase 50: Screenshot and Source-Truth Inventory (completed 2026-07-03)
- [x] Phase 51: Shell and Navigation Restoration (completed 2026-07-03)
- [x] Phase 52: Prepare Sidebar and Preset Controls (completed 2026-07-03)
- [x] Phase 53: Prepare Object, Plate, and Viewport Workflow (completed 2026-07-03)
- [x] Phase 54: Preview Layout, Sliders, and Right Panels (completed 2026-07-03)
- [x] Phase 55: G-code Preview Semantics and Rendering Stability (completed 2026-07-03)
- [x] Phase 56: Parameter Settings Dialogs Restoration (completed 2026-07-03)
- [x] Phase 57: Deprecated UI Removal and Architecture Cleanup (completed 2026-07-03)
- [x] Phase 58: End-to-End Visual and Functional Verification (completed 2026-07-03; manual visual UAT deferred)

Archive: `.planning/milestones/v3.6-ROADMAP.md`

</details>

<details>
<summary>v3.8 RHI Gizmo Parity (Phases 65-73) - SHIPPED 2026-07-04</summary>

- [x] Phase 65: Gizmo math extraction + unit tests (completed 2026-07-04)
- [x] Phase 66: Gizmo geometry builders port (completed 2026-07-04)
- [x] Phase 67: RHI gizmo state wiring (completed 2026-07-04)
- [x] Phase 68: Move gizmo RHI render (completed 2026-07-04; visual evidence deferred)
- [x] Phase 69: Move gizmo pick + drag interaction loop (completed 2026-07-04)
- [x] Phase 70: Rotate + Scale gizmos (completed 2026-07-04)
- [x] Phase 71: Cut plane + wipe tower (completed 2026-07-04)
- [x] Phase 72: Precise object picking (completed 2026-07-04)
- [x] Phase 73: Retire GLViewport + verification (completed 2026-07-04)

Archive: `.planning/milestones/v3.8-ROADMAP.md`
Audit: `.planning/milestones/v3.8-MILESTONE-AUDIT.md`
Requirements: `.planning/milestones/v3.8-REQUIREMENTS.md`

</details>

## Deferred Backlog

- v3.4 Phase 43 manual UAT is closed by canonical E2E coverage and current runtime launch evidence, not by a separate user click-through.
- v3.5 Phase 47-49 are superseded by v3.6 and should not be resumed as standalone work unless explicitly reopened.
- Device send/upload/cloud print and Monitor task workflow.
- AssembleView.
- Auto filament-map recommendation and wipe-tower geometry/rendering.
- Real thumbnail capture and 3MF pixel round-trip.
- D3D12 root-cause investigation and future Vulkan/D3D12 backend promotion.
- ModelMall/Home WebView and cloud workflows.
- Full hardware calibration completion beyond preserving existing implemented preset read/write paths.

## Next Step

Continue v4.0 with Phase 81:

```text
$gsd-discuss-phase 81 --auto
```

---

*Last updated: 2026-07-06 after v4.0 milestone start.*
