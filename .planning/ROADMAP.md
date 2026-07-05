# Roadmap: OWzx Slicer

## Milestones

- Complete: **v2.9** - Phases 10-15 (shipped 2026-06-25)
- Complete: **v3.0 PartPlate Core** - Phases 16-22 (shipped 2026-06-26)
- Complete: **v3.1 QRhi Rendering** - Phases 23-28 (shipped 2026-06-28)
- Complete with tech debt: **v3.2 Multi-Plate Data Polish** - Phases 29-32 (audited 2026-06-28)
- Complete at MVP level, superseded for completeness: **v3.3 Slice Preview Main Flow MVP** - Phases 33-36
- Automated verification passed, manual UAT deferred: **v3.4 Import to G-code Complete Workflow** - Phases 37-43
- Superseded after Phase 46: **v3.5 Preset Authoring Complete Workflow** - Phases 44-49
- Automated verification passed, visual UAT not closed: **v3.6 Screenshot-Driven OrcaSlicer UI Restoration** - Phases 50-58 (shipped 2026-07-03)
- Complete with residual gaps: **v3.7 Screenshot-Level UI Parity Closure** - Phases 59-64 (2026-07-04; D3D12 and manual visual debt carried forward)
- Complete with tech debt: **v3.8 RHI Gizmo Parity** - Phases 65-73 (shipped 2026-07-04; 21/21 requirements satisfied, Phase 68 visual evidence deferred)
- Active: **v3.9 Prepare Page UI Restoration** - Phases 74-78

## Active Milestone: v3.9 Prepare Page UI Restoration

**Goal:** Restore the Prepare page to screenshot-level OrcaSlicer parity using `shotScreen/` as visual truth and OrcaSlicer GUI source as behavior truth.

## Phases

- [x] Phase 74: Prepare Source-Truth Gap Audit (completed 2026-07-05)
- [x] Phase 75: Prepare Sidebar Restoration (completed 2026-07-05)
- [ ] Phase 76: Prepare Workflow Panels Restoration
- [ ] Phase 77: Prepare Viewport Controls And Gizmo UI
- [ ] Phase 78: Prepare Verification And Cleanup

| Phase | Name | Goal | Requirements |
|---|---|---|---|
| 74 | Prepare Source-Truth Gap Audit | Complete 2026-07-05: froze the v3.9 Prepare region map, current gaps, upstream anchors, and verification expectations before edits. | AUDIT-01 |
| 75 | Prepare Sidebar Restoration | Complete 2026-07-05: restored compact sidebar width/density, filament rows, localized option source labels, and preset/settings wiring. | SIDE-01, SIDE-02, SIDE-03 |
| 76 | Prepare Workflow Panels Restoration | Restore object list, plate strip, slice status, and workflow availability states to screenshot/source-truth parity. | OBJ-01, PLATEUI-01, STATUS-01 |
| 77 | Prepare Viewport Controls And Gizmo UI | Restore viewport controls, vertical tool buttons, and RHI gizmo floating panels in the screenshot layout. | VIEWUI-01, GIZMOUI-01 |
| 78 | Prepare Verification And Cleanup | Remove deprecated Prepare UI paths and verify with audits, canonical build, app launch, and visual evidence. | CLEAN-01, VERIFY-01, VERIFY-02 |

### Phase 74: Prepare Source-Truth Gap Audit

**Status:** Complete (2026-07-05)
**Plans:** 1/1 plans complete

Plans:
- [x] `74-01-PLAN.md` - Prepare source-truth gap matrix

Success criteria:
1. Current Prepare page regions are mapped to target screenshot areas and OrcaSlicer source files.
2. Each region records Qt target files, modify-vs-replace decision, and verification method.
3. v3.7 residual Prepare gaps are reconciled into v3.9 requirements or explicitly deferred.

### Phase 75: Prepare Sidebar Restoration

**Status:** Complete (2026-07-05)
**Plans:** 1/1 plans complete

Plans:
- [x] `75-01-PLAN.md` - Sidebar UI restoration

Success criteria:
1. Printer, filament, and process preset controls match the screenshot density and state model.
2. Visible option rows use upstream-mapped display names/translations instead of raw internal keys where applicable.
3. Global/Object/Plate scopes, search, settings entry points, and dirty indicators are live and free of visible unavailable placeholders.

### Phase 76: Prepare Workflow Panels Restoration

Success criteria:
1. Object/volume list states match screenshot layout for empty, selected, disabled, and action-visible cases.
2. Plate strip selection/add/state surfaces match the target layout and existing PartPlate behavior.
3. Slice/cancel/export readiness and progress surfaces are honest, live, and aligned with upstream workflow semantics.

### Phase 77: Prepare Viewport Controls And Gizmo UI

Success criteria:
1. Prepare viewport controls and vertical tool buttons are positioned and disabled consistently with the screenshot.
2. Move, rotate, scale, cut, and wipe-tower floating panels are visible in the correct states and do not overlap core page controls.
3. Default RHI interaction remains functional while UI restoration changes presentation.

### Phase 78: Prepare Verification And Cleanup

Success criteria:
1. Replaced Prepare UI paths leave no stale files, imports, resource entries, tests, or disconnected controls.
2. Automated source/QML audits cover upstream mapping, placeholder removal, and required bindings.
3. The canonical verifier passes, `build/OWzxSlicer.exe` launches, and Prepare visual evidence is recorded against the target screenshot.

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

- v3.4 Phase 43 manual UAT for import -> Prepare -> slice -> Preview -> export remains pending until the user can verify it.
- v3.5 Phase 47-49 are superseded by v3.6 and should not be resumed as standalone work unless explicitly reopened.
- Device send/upload/cloud print and Monitor task workflow.
- AssembleView.
- Auto filament-map recommendation and wipe-tower geometry/rendering.
- Real thumbnail capture and 3MF pixel round-trip.
- D3D12 crash root cause and future Vulkan/D3D12 backend promotion.
- ModelMall/Home WebView and cloud workflows.
- Full hardware calibration completion beyond preserving existing implemented preset read/write paths.

## Next Step

Plan Phase 76:

```text
$gsd-plan-phase 76
```

---

*Last updated: 2026-07-05 after Phase 75 sidebar restoration.*
