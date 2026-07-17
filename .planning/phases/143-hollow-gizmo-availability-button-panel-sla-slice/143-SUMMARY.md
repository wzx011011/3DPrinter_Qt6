---
phase: 143
name: Hollow Gizmo Availability + Button + Panel + SLA Slice
status: partial
verified: 2026-07-17
requirements_covered:
  - VDB-03
  - VDB-04
  - VDB-05
requirements_deferred:
  - VDB-06
---

# Phase 143 Summary — PARTIAL (VDB-06 deferred to v5.1+)

**Phase:** 143 (v5.0 / WS2)
**Status:** partial — VDB-03/04/05 satisfied, VDB-06 explicitly deferred
**Requirements:** VDB-03 ✓, VDB-04 ✓, VDB-05 ✓ (panel edits only, no 3MF persistence yet), VDB-06 → v5.1+

## Scope reality discovered mid-phase

The original Phase 143 plan assumed VDB-06 ("slicing a solid cube with hollowEnabled=true produces non-solid G-code via an SLA printer profile") was achievable. Exploration revealed it is **not** achievable in the current codebase:

- `SliceService` uses `Slic3r::Print` (FFF) only. There is **no `SLAPrint` integration**.
- There are **no SLA printer presets bundled** (no `resources/` dir; PresetServiceMock synthesizes FFF presets in code).
- There is no SLA material/process config schema, no `.png` layer output, no SLA-aware hollowing-reslice flow.

Wiring SLA from scratch is a multi-phase sub-milestone (upstream libslic3r has ~35 files in `SLA/` + `SLAPrint.cpp` 1261 lines + `SLAPrintSteps.cpp` + the `GLGizmoHollow.cpp` 873 lines + `GLGizmoSlaSupports.cpp` + SLA preset bundle + PNG output). Per user direction, this is deferred to a v5.1+ SLA sub-milestone.

## What shipped (VDB-03/04/05)

### VDB-03 — Hollow gizmo reachable
`src/core/viewmodels/EditorViewModel.cpp`:
- `case 8: // Hollow` in the gizmo-availability switch returns `hasSingleObject` (was unconditionally `false`).
- The tooltip blocker at `case 8` no longer returns "Blocked: OpenVDB unavailable" — returns empty string (no blocker).
- Case 18 (SLA supports) keeps a v5.1+ marker: "Blocked: requires SLA print path (v5.1+)".

### VDB-04 — Hollow tool button
`src/qml_gui/components/GLToolbars.qml`:
- New `GizmoToolButton { toolId: GLViewport.GizmoHollow; textTip: qsTr("Hollow") }` after the Emboss/SVG buttons.
- New `case GLViewport.GizmoHollow:` in `iconForTool` returning `layers-subtract.svg` (no dedicated hollow icon shipped yet — reuse metaphor).

### VDB-05 — Hollow gizmo panel
`src/qml_gui/pages/PreparePage.qml`:
- New Rectangle panel (parallel to the existing Simplify/MMU panels) visible when `viewport3d.gizmoMode === GLViewport.GizmoHollow`.
- Surfaces all 6 existing Q_PROPERTYs (`hollowEnabled` checkbox + `hollowOffset`/`hollowQuality`/`hollowClosingDistance`/`hollowHoleRadius`/`hollowHoleHeight` sliders).
- Footer note: "完整 SLA 切片待 v5.1+".
- The EditorViewModel Q_PROPERTY scaffolding (lines 602-608) was already in place — the C++ side needed no new declarations.

### VDB-06 — DEFERRED
3MF persistence + actual SLA slice → v5.1+ SLA sub-milestone. The hollow parameter values currently live in the EditorViewModel session state and do NOT round-trip through 3MF. This is acceptable because the values only matter when SLAPrint is wired (which it isn't yet).

### Regression lock
`tests/QmlUiAuditTests.cpp`:
- New `v50HollowGizmoReachable()` source-audit slot asserting all VDB-03/04/05 anchors.

## Verification

- C++ compile: clean (EditorViewModel switch case change only).
- QML: compiles via RCC into qrc_qml.cpp (verified by OWzxSlicer.exe link 6/6).
- OWzxSlicer.exe links clean (NINJA_EXIT=0).
- QmlUiAuditTests: 102/102 PASS (3 new v5.0 slots: v50TechDebtRegressionLocked + v50OpenVdbUnlockWired + v50HollowGizmoReachable).

## Lessons

1. **Test the integration depth before committing to a phase scope.** Phase 143's plan said "produce hollowed G-code" without verifying SLAPrint was reachable. A 5-minute grep for `SLAPrint` / `printer_technology` in SliceService would have surfaced this before planning. Apply this lesson to all future phases that assert "X works end-to-end" — verify the integration path exists, not just the upstream source.
2. **Honest partial beats fake-complete.** Shipping 3/4 requirements with VDB-06 explicitly deferred + documented is more useful than rushing a fake SLA integration that doesn't actually slice. The deferral is recorded in REQUIREMENTS.md traceability + this SUMMARY + the regression slot's comments.
3. **The OpenVDB link (Phase 142) is the real WS2 deliverable.** Hollow UI on top is incremental value. Future SLA work in v5.1+ can proceed without re-proving the OpenVDB unlock.

## Unlocks downstream

- Phase 150-152 (PartPlate): unrelated, can proceed independently.
- v5.1+ SLA sub-milestone: when scoped, can assume OpenVDB-linked + Hollow UI scaffolding are in place.
