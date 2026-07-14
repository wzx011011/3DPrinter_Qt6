---
phase: 115-glgizmomeasure-snap-ux-and-feature-picking
status: NEEDS_CHANGES → PARTIALLY FIXED (H1 inherited from 114 fixed; M2 fixed; M1/M3 documented as deferred)
requirement: MEASURE-04
counts: {blockers: 0, highs: 0, mediums: 3, lows: 2}
---

# Phase 115 Code Review — GLGizmoMeasure Snap UX

## Verdict: NEEDS_CHANGES → PARTIALLY FIXED

Two-stage pick wiring is correct and clean. Three MEDIUM issues: M1 (Shift cosmetic-only — deferred), M2 (Escape doesn't reset — FIXED), M3 (selection mode reconciliation — deferred). Inherited Phase 114 H1 (double-transform) — FIXED in 114.

## Findings

| # | Severity | Finding | Status |
|---|----------|---------|--------|
| M1 | medium | Shift toggle is cosmetically-only — override only the hover-highlight fields; readout always uses the snapped feature. Deliberate divergence from upstream PointSelection. **Deferred** — document honestly that Shift toggles the indicator only; full PointSelection parity is a future enhancement. |
| M2 | medium | Escape does not reset the two-click measure flow (PreparePage.qml wires Escape → clearObjectSelection, NOT clearMeasureReadout). **FIXED** — add clearMeasureReadout to Escape handler when GizmoMeasure active. |
| M3 | medium | measureSelectionMode Q_PROPERTY and the Shift toggle are not reconciled — two sources of truth for the same UX concept. **Deferred** — decide authoritative mechanism in future UX pass. |
| L1 | low | No mouse-move throttle. Per-move cost O(hit volume) — acceptable for modest meshes; flag for large-mesh verification. |
| L2 | low | Source-audit doesn't lock Shift-affects-measurement (because it doesn't). Consistent with M1 deferral. |

## Verified Strong

- **Two-stage pick (pitfall 7):** hoverMoveEvent → emitMeasurePickIfActive → stage-1 AABB → stage-2 per-triangle ITS on candidate only. Candidate list bounded by objectVolumeCount. No whole-scene loop.
- **Signal-param rename:** grep for `ray` in PreparePage.qml returns ZERO. worldOrigin/worldDirection everywhere. Picking logic stays in C++.
- **Lazy shared cache:** m_sceneRaycaster + m_measureEngine both lazily constructed, both fed from same volumeMeshIts accessor, both dropped by invalidateMeasureEngine.
- **Cursor-leave clear:** consistent across all miss paths.

Regression: PartPlateTests 53/53, ViewModelSmokeTests 99/99, QmlUiAuditTests 83/83 pass.
