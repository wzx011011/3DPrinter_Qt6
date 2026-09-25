# Requirements: OWzx Slicer — v4.8 Dependency Unlock, Assembly Transform & i18n Completion

**Defined:** 2026-07-15
**Core Value:** OrcaSlicer upstream behavior is the product source of truth.

## v4.8 Requirements

### WS1 — CGAL Dependency Upgrade

- [x] **CGAL-01**: CGAL is upgraded to 5.6+ in DEPS_PREFIX; the build links against the new version; no regressions in existing CGAL-dependent code. (Resolved via compat-patch path on CGAL 5.4 — see Phase 136 VERIFICATION; a true 5.6+ bundle upgrade is no longer required for MeshBoolean.)
- [x] **CGAL-02**: `kCgalMeshBooleanAvailable` is flipped to true; MeshBoolean (union/subtract/intersect) works end-to-end — ~200 lines of already-written logic activate.
- [x] **CGAL-03**: Drill gizmo works end-to-end — tool-mesh + feature-matrix logic activates.

### WS2 — Assembly Transformation (ASM-01)

- [x] **ASM-01**: Assembly view supports move/rotate/scale transformation actions on assembled volumes (RhiViewport assembly canvas gizmo + ViewModel + AssemblePage). Transformations apply per-volume and round-trip through the model.

### WS3 — i18n Completion

- [x] **I18N-04**: en.ts remaining ~1372 unfinished translations are filled (long sentences, compound phrases). lrelease produces a complete en.qm.
- [x] **I18N-05**: de/fr/ja/ko advance meaningfully from baseline (documented remaining-work estimate).

### Cross-Cutting

- [x] **REGRESS-03**: Canonical build passes clean; regression ctest passes; Prepare/Preview/Assembly/paint/calibration behaviors regression-free.

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| CGAL-01 | Phase 136 | Done (2026-07-16, compat-patch path) |
| CGAL-02 | Phase 137 | Done (2026-07-16) |
| CGAL-03 | Phase 137 | Done (2026-07-16) |
| ASM-01 | Phase 138 | Done (2026-07-16) |
| I18N-04 | Phase 139 | Done (2026-07-16) |
| I18N-05 | Phase 139 | Done (2026-07-16) |
| REGRESS-03 | Phase 140 | Done (2026-07-16) |

**Coverage:** 7 requirements, 7 mapped.

---
*Requirements defined: 2026-07-15*
