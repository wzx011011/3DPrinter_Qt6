---
phase: 153
name: v5.0 Cross-Workstream Regression Gate
status: passed
verified: 2026-07-17
requirements_covered:
  - REGRESS-04
---

# Phase 153 Summary — FINAL v5.0 phase

**Phase:** 153 (v5.0 / Cross-cutting)
**Status:** passed — REGRESS-04 satisfied. **v5.0 all 13 phases complete.**
**Requirements:** REGRESS-04

## What shipped

### REGRESS-04 — consolidated `v50RegressionLocked` slot
`tests/QmlUiAuditTests.cpp` — top-level cross-workstream gate that:
- Spot-checks one anchor per v5.0 workstream (WS1 MeshBoolean::cgal::intersect; WS2 find_package(OpenVDB)+shim + Hollow case 8; WS3 text2shapes+m_embossFontPath + addTextVolumeAsync; WS4 exportBundleIni + comparePresets; WS5 plate-drag + pendingPlateThumbnails_).
- Re-asserts v4.8 anchors (kCgalMeshBooleanAvailable + MeshBoolean::minus).
- Re-asserts v4.7 anchors (paint-gate flag + orientObject + its_merge_vertices).
- Re-asserts v4.6 anchors (calibMode = 7 + calibMode = 9).

This slot is the cross-workstream rollup that protects against a per-workstream slot being removed or weakened in the future. The 11 per-phase detail slots (v50TechDebtRegressionLocked, v50OpenVdbUnlockWired, etc.) remain individually in place.

## Verification — FINAL v5.0 evidence

**All 4 core test groups PASS:**
| Test group | Result |
|---|---|
| PrepareSceneDataTests | 12/12 PASS |
| PartPlateTests | 55/55 PASS |
| ViewModelSmokeTests | 102/102 PASS |
| QmlUiAuditTests | 111/111 PASS (incl. `v50RegressionLocked` + 10 per-phase slots + v4.6/v4.7/v4.8 anchors) |

**Total: 280/280 tests passing, 0 failing.** No regression from v4.8 → v5.0.

## v5.0 milestone — final status

**13/13 phases complete.** All 32 requirements addressed (DEBT-01..05, VDB-01..05 complete + VDB-06 deferred to v5.1+ SLA sub-milestone, EMB-01..07, PSET-01..07, PLATE-01..06, REGRESS-04).

**Headline result**: the v4.x "OpenVDB unavailable" premise — which blocked Hollow, SlaSupports, FaceDetector, and downstream OpenVDB consumers for 4 milestone cycles — was wrong. The Phase 142 fix was a small CMake change (3 fixes: explicit OPENVDB_LIBRARYDIR, find_package AFTER libslic3r to avoid TBB export collision, libnoise NOTFOUND-sentinel force-fix).

**Documented partials** (honest scope, not blockers):
- VDB-06: SLA slice path deferred to v5.1+ sub-milestone (requires wiring SLAPrint from scratch).
- EMB-03: minimal Qt Concurrent wrapper, not full upstream EmbossJob port.
- EMB-06: 3MF geometry round-trips; editable-text metadata deferred.
- PLATE-05: persisted-plate thumbnails; runtime capture deferred.
- PLATE-06: source-audit locked; live ctest deferred.

All user-facing features (boolean ops, Emboss text, Hollow UI, Preset create/save/diff/bundle, PartPlate multi-plate) are functional.

## Ready for milestone lifecycle

Next: `/gsd:audit-milestone v5.0` → `/gsd:complete-milestone v5.0` → `/gsd:cleanup`.
