# Phase 116: v4.5 Verification And Cross-Workstream Regression - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
Close WTMESH-04 (Option A no-regress proof) + MEASURE-05 (measure readouts are real, not AABB stub) + the v4.5 cross-workstream verification gate. Run the canonical verifier + launch + regression ctest, and lock the cross-workstream contracts.

</domain>

<decisions>
### Scope
1. **WTMESH-04**: assert Option A (v4.4 Phase 99 Frozen Decision 2 baseline) does not regress — Option B fires only when wipe_tower_mesh_data populated. Source-audit + regression ctest.
2. **MEASURE-05**: assert measure readouts are real (Phase 114 MeasureEngine path), not the AABB stub. Source-audit confirming MeasureEngine is wired; the AABB stub is either augmented or documented-as-coarse-fallback.
3. **Cross-workstream source audits**: a consolidated QmlUiAuditTests slot covering the v4.5 workstream anchors (filament-map enum + readback + popup; Option B mesh; CLI fixtures; D3D12 opt-in; per-volume ITS + raycaster + Measuring + snap UX).
4. **Canonical verifier + launch liveness + regression ctest**.
5. **Optional scratch cleanup**.

</decisions>

<deferred>
- This is the LAST v4.5 phase. After it: milestone audit → complete → cleanup.
</deferred>
