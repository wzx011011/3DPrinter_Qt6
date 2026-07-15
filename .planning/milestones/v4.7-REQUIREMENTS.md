# Requirements: OWzx Slicer — v4.7 Polish, i18n & Advanced Feature Recovery

**Defined:** 2026-07-15
**Core Value:** OrcaSlicer upstream behavior is the product source of truth.
**Milestone:** v4.7 — Polish, i18n & Advanced Feature Recovery. Fix v4.6 carry-forward bugs + small polish; restore English i18n; upgrade CGAL to unlock MeshBoolean + Drill; advance MEASURE-06 assembly transformation.

## Scope Basis

A very-thorough code gap analysis (run during `/gsd:new-milestone`) confirmed the v4.7 targets. Key evidence:

| WS | Target | Current State (evidence) |
|---|---|---|
| 1 | Paint-gizmo gate flag + Flatten + fixMesh + KBShortcutsDialog + ProjectPage | `EditorViewModel.cpp:47` `kViewportTrianglePickingAvailable=false` stale (Phase 121-123 real but flag not flipped); Flatten `:1622` mock 6-face (orientObject `:2664` real but unwired); fixMesh `ProjectServiceMock.cpp:3871` no-op copy; no KBShortcutsDialog; ProjectPage `:229` hardcoded "—" |
| 2 | en.ts translation fill | `i18n/en.ts` 1493 unfinished (source is Chinese, 92% empty translation); de/fr/ja/ko 1629 each unfinished |
| 3 | CGAL upgrade unlock | `kCgalMeshBooleanAvailable=false` (`:48`); MeshBoolean `ProjectServiceMock.cpp:3097` + Drill `:3234` each `return false` "CGAL version too old" with ~200 lines already written |
| 4 | MEASURE-06 assembly transformation | AssemblyMeasureGeometry (Phase 92) + MeasureEngine + SceneRaycaster (v4.5/v4.6) foundation shipped; assembly-mode move/rotate/scale actions deferred |

## v4.7 Requirements

### WS1 — Polish & Bug-Fix Pack

- [ ] **POLISH-01**: The stale `kViewportTrianglePickingAvailable` flag is flipped to true (or removed); Support/Seam/MMU paint gizmos report correct availability (no false "Blocked: viewport triangle picking unavailable" when the engine is real).
- [ ] **POLISH-02**: Flatten gizmo uses the real `orientObject` (which calls `Slic3r::orientation::orient`) instead of the mock 6-hardcoded-face path; the flattened result is a real orientation.
- [ ] **POLISH-03**: `fixMesh`/`reloadFromDisk` calls real mesh repair (`its_repair_*` / re-read from disk) instead of the no-op copy (`set_mesh(move(copy))`); the "mesh repair" button actually repairs.
- [ ] **POLISH-04**: A KBShortcutsDialog (keyboard-shortcut overview) exists and is reachable (upstream has it; current project has only inline Shortcut{}).
- [ ] **POLISH-05**: ProjectPage property panel shows real values (path/format/size/modified-date) instead of hardcoded "—".

### WS2 — i18n English Translation

- [ ] **I18N-02**: en.ts has translations for all user-visible strings (the 1493 currently empty). Source strings are Chinese; the English translation is filled (draft + review). lrelease produces a non-empty en.qm.
- [ ] **I18N-03**: de/fr/ja/ko .ts advance from the v4.6 baseline (documented remaining-work estimate updated); not falsely claimed as 100%.

### WS3 — CGAL Upgrade Unlock

- [ ] **CGAL-01**: CGAL is upgraded to 5.6+ in the dependency bundle; the build links against the new version.
- [ ] **CGAL-02**: `kCgalMeshBooleanAvailable` is flipped to true; MeshBoolean (boolean union/subtract/intersect) works end-to-end — the ~200 lines of already-written logic activate, producing a real boolean result volume.
- [ ] **CGAL-03**: Drill gizmo works end-to-end — the already-written tool-mesh construction + feature-matrix logic activates, producing a real drilled hole.

### WS4 — Assembly Transformation (MEASURE-06)

- [ ] **ASM-01**: Assembly view supports transformation actions (move/rotate/scale) on assembled volumes, building on the v4.5/v4.6 feature-picking + AssemblyMeasureGeometry foundation. The transformations apply per-volume in the assembly context and round-trip through the model.

### Cross-Cutting — Verification & Regression

- [ ] **REGRESS-02**: Canonical build (j6) passes clean, regression ctest passes, and Prepare/Preview/Assembly/paint/calibration behaviors are regression-free across all v4.7 workstreams.

## Future Requirements

- **HOLLOW-01**: GLGizmoHollow (depends on OpenVDB — unavailable).
- **FACEDET-01**: GLGizmoFaceDetector (depends on OpenVDB or a face-detection lib).
- **SLASUP-01**: GLGizmoSlaSupports (SLA, depends on OpenVDB).
- **D3D12-04**: D3D12 default-backend promotion (needs confirmed root cause).
- **PLATE-09-FULL**: Full save/reload state assertions.

## Out of Scope

| Feature | Reason |
|---------|--------|
| Hollow / FaceDetector / SlaSupports | Depend on OpenVDB (unavailable per project constraints). |
| LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware | Removed from forward scope by user direction. |
| Hardware-dependent Calibration modes | Require live printer hardware. |
| D3D12 default-backend promotion | Needs confirmed root cause + clean repro. |
| Full de/fr/ja/ko 100% translation | Documented baseline; full coverage future. |

## Traceability

Filled during roadmap creation.

| Requirement | Phase | Status |
|-------------|-------|--------|
| POLISH-01 | Phase 129 | Pending |
| POLISH-02 | Phase 129 | Pending |
| POLISH-03 | Phase 129 | Pending |
| POLISH-04 | Phase 130 | Pending |
| POLISH-05 | Phase 130 | Pending |
| I18N-02 | Phase 131 | Pending |
| I18N-03 | Phase 131 | Pending |
| CGAL-01 | Phase 132 | Pending |
| CGAL-02 | Phase 133 | Pending |
| CGAL-03 | Phase 133 | Pending |
| ASM-01 | Phase 134 | Pending |
| REGRESS-02 | Phase 135 | Pending |

**Coverage:**
- v4.7 requirements: 12 total
- Mapped to phases: 12
- Unmapped: 0 ✓

---
*Requirements defined: 2026-07-15*
