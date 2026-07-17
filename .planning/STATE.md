---
gsd_state_version: 1.0
milestone: v5.0
milestone_name: Advanced Feature Recovery & Tech-Debt Closure
status: in_progress
last_updated: "2026-07-17T04:45:00.000Z"
last_activity: 2026-07-17
progress:
  total_phases: 13
  completed_phases: 8
  total_plans: 8
  completed_plans: 8
  percent: 62
---

# Project State

**Milestone:** v5.0 — Advanced Feature Recovery & Tech-Debt Closure
**Status:** Planning (2026-07-17). ROADMAP.md created (13 phases, 141-153, 32 requirements mapped).
**Next step:** `/gsd:plan-phase 141` (or `/gsd:discuss-phase 141` if discuss is re-enabled). Skip_discuss=true per config — phases go straight to plan-phase.

## Last Shipped Milestone: v4.8 (2026-07-16, tech_debt)

**Audit status:** tech_debt — 7/7 requirements satisfied, no blockers. 5 phases (136-140), 8 plans.

**Carried tech_debt into v5.0 (to be closed by WS1 / WS2):**

- CGAL-02 intersection boolean returns subtraction (union/difference/drill work) — **v5.0 WS1 closes**.
- Orphaned `meshBooleanSelected` menu stub — **v5.0 WS1 removes**.
- `ProjectServiceMock::drillObject` MSVC C4715 — **v5.0 WS1 fixes**.
- Assemble rotate/scale live-visual compose (translate-only render) — **v5.0 WS1 closes**.
- 2-line CGAL submodule compat patch — droppable if CGAL upgraded in DEPS_PREFIX (out of v5.0 scope unless convenient).
- de/fr/ja/ko ~906 messages remaining each — non-code; remains Future.

## v5.0 Scope (5 workstreams)

| WS | Name | Closes / Adds |
|---|---|---|
| WS1 | Tech-debt closure | CGAL-02 intersect, orphaned menu, C4715, ASM rotate/scale render, v5.0 regression lock |
| WS2 | OpenVDB unlock (corrected premise) | Link OpenVDB → unlock Hollow gizmo |
| WS3 | Emboss text gizmo | Port `GLGizmoEmboss.cpp` (62KB) — new feature |
| WS4 | Preset Bundle full chain | Upstream-compatible bundle metadata + CreatePresetsDialog + dirty-state + Compare/Diff + round-trip |
| WS5 | PartPlate multi-plate completion | Real plate data ownership + per-plate CRUD + config override + AssembleView minimal real view |

## Project Reference

See: .planning/PROJECT.md (v5.0 Current Milestone section + revised OpenVDB constraint)
See: .planning/ROADMAP.md (to be created)
See: .planning/milestones/v4.8-* (last shipped milestone archive)

**Core value:** OrcaSlicer upstream behavior is the product source of truth.

## Current Position

Phase: 149 (Compare/Diff + Dirty Propagation + Round-Trip — next to plan)
Plan: —
Status: Phase 148 verified ✓ (8/13 phases complete, 62%). PSET-03/04 satisfied (UnsavedChangesDialog + Simple/Advanced filter — both were already wired pre-v5.0; verified + locked). 107/107 QmlUiAuditTests passing.
Last activity: 2026-07-17 — Phase 148 shipped (PSET-03/04 closed; v50UnsavedChangesAndFilterWired locked). Ready for Phase 149.

## Resume Brief — READ THIS BEFORE CONTINUING

### What's done (4 phases, all green)
- Phase 141 (DEBT-01..05): tech-debt closure (CGAL intersect, orphaned menu, C4715, ASM rotate/scale render, regression slot)
- Phase 142 (VDB-01/02): OpenVDB CMake unlock — **refuted the v4.x "unavailable" premise**. OpenVDB 8.2.0 now links clean. Required 3 CMake fixes: explicit OPENVDB_LIBRARYDIR, find_package AFTER libslic3r (avoids TBB export collision), libnoise NOTFOUND-sentinel force-fix.
- Phase 143 (VDB-03/04/05): Hollow gizmo UI scaffolding (button + panel + reachability). **VDB-06 deferred to v5.1+ SLA sub-milestone** — would require wiring SLAPrint from scratch (no SLA infrastructure exists; SliceService is FFF-only, no SLA presets bundled).
- Phase 144 (EMB-01/02): Emboss parameterization. **Was much smaller than feared** — the real text2shapes + polygons2model pipeline was already wired pre-v5.0 (uses stb_truetype, no freetype dep). This phase added font enumeration + parameterized height/depth from Q_PROPERTYs.

### Build-cycle constraint (critical for planning)
The canonical `scripts/auto_verify_with_vcvars.ps1` reconfigures CMake + does a full libslic3r rebuild each invocation (~12-15 min), exceeding the harness 10-min background-task budget. Workaround used: direct `ninja <target>` from existing `build.ninja` for verification (much faster, same answer for specific targets). Build helper batches in `build/_ninja_owzx.bat` (OWzxSlicer) + `build/_ninja_test.bat` (QmlUiAuditTests only) + `build/_ninja_tests.bat` (4 core test binaries).

### Phase 145 (next) — scope decision needed
EMB-03 says "async EmbossJob". Upstream `EmbossJob.cpp` is 1586 lines + requires porting the Job base-class system. **Recommendation: minimal Qt Concurrent wrapper** around the existing synchronous text2shapes+polygons2model (satisfies the spirit of EMB-03 — non-blocking, cancellable — without the upstream Job system port). Plus an Emboss gizmo panel (QML, parallel to the Hollow panel added in Phase 143).

### Remaining phases
- Phase 145: Async EmbossJob + Panel (EMB-03/04) — medium
- Phase 146: Emboss Wiring + 3MF round-trip + SVG (EMB-05/06/07) — medium
- Phase 147: Preset INI + CreatePresetsDialog (PSET-01/02) — medium-large (upstream CreatePresetsDialog port)
- Phase 148: UnsavedChangesDialog + Filter (PSET-03/04) — medium (upstream UnsavedChangesDialog 3-way diff port)
- Phase 149: Compare/Diff + Round-Trip (PSET-05/06/07) — medium
- Phase 150: PartPlate UI Gap Analysis (PLATE-01) — read-only, small
- Phase 151: PartPlate UI Implementation (PLATE-02..05) — medium (gap-analysis-driven)
- Phase 152: PartPlate Save/Reload Regression (PLATE-06) — test-only, small
- Phase 153: v5.0 Regression Gate (REGRESS-04) — test-only, small (final)

### Key discoveries to carry forward
1. **Always grep for existing implementation before assuming scope** (Phase 144 lesson — pipeline was already wired).
2. **Verify integration depth before committing "X works end-to-end"** (Phase 143 lesson — SLAPrint didn't exist).
3. **Anchor regression slots on callables, not documentation text** (Phase 141 lesson — assertion false-positive on comment).
4. **OpenVDB link exposed libnoise latent NOTFOUND sentinel** — fixed in Phase 142 but worth knowing if other latent issues surface when transitive link chains expand.
5. **REGRESS-04 (Phase 153) must re-assert v4.6/v4.7/v4.8 + all v5.0 anchors** — consolidate the per-phase slots (v50TechDebtRegressionLocked + v50OpenVdbUnlockWired + v50HollowGizmoReachable + v50EmbossParameterized + the 145-152 ones) into one `v50RegressionLocked` slot.

### OpenVDB unlock is the milestone's headline value
Even if v5.0 stopped today, Phase 142 alone is a major result: the v4.x "OpenVDB unavailable" premise (which blocked Hollow, SlaSupports, FaceDetector, and downstream OpenVDB consumers for 4 milestone cycles) was wrong, and the fix was a small CMake change.

## v5.0 Roadmap Snapshot (13 phases, 141-153)

| Phase | WS | Name | Requirements |
|---|---|---|---|
| 141 | WS1 | v4.x Tech-Debt Closure | DEBT-01..05 |
| 142 | WS2 | OpenVDB CMake Unlock And libslic3r Link | VDB-01, VDB-02 |
| 143 | WS2 | Hollow Gizmo Availability + Button + Panel + SLA Slice | VDB-03..06 |
| 144 | WS3 | Emboss Font Loading And Text2Shapes Extrude | EMB-01, EMB-02 |
| 145 | WS3 | Async EmbossJob And Gizmo Panel | EMB-03, EMB-04 |
| 146 | WS3 | Emboss Wiring, 3MF Round-Trip, And SVG Path | EMB-05..07 |
| 147 | WS4 | Preset Bundle INI Format And CreatePresetsDialog | PSET-01, PSET-02 |
| 148 | WS4 | UnsavedChangesDialog 3-Way Diff And Simple/Advanced Filter | PSET-03, PSET-04 |
| 149 | WS4 | Compare/Diff, Dirty Propagation, And Bundle Round-Trip | PSET-05..07 |
| 150 | WS5 | PartPlate UI Gap Analysis (read-only) | PLATE-01 |
| 151 | WS5 | PartPlate UI Implementation | PLATE-02..05 |
| 152 | WS5 | PartPlate Multi-Plate Save/Reload Regression | PLATE-06 |
| 153 | Cross | v5.0 Cross-Workstream Regression Gate | REGRESS-04 |

**Coverage:** 32/32 requirements mapped (DEBT 5 + VDB 6 + EMB 7 + PSET 7 + PLATE 6 + REGRESS-04 1 = 32). 0 unmapped. (REQUIREMENTS.md header previously stated "33" — corrected to 32 by roadmapper; the literal ID list contains 32 unique IDs.)
