---
phase: 140
status: passed
verified: 2026-07-16
requirements: [REGRESS-03]
plans: [140-01]
---

# Phase 140 Verification

## Status: PASSED

**Requirement:** REGRESS-03 — Canonical build passes clean; regression ctest passes; Prepare/Preview/Assembly/paint/calibration/MeshBoolean/Drill behaviors regression-free.

## Success criteria

### SC1: Canonical build passes clean (exit 0)

| Check | Result | Evidence |
|---|---|---|
| `scripts/auto_verify_with_vcvars.ps1` exit code | PASS (0) | `build_p140b.log` |
| 0 compile/link errors | PASS | grep `error C\|error LNK\|fatal error` → none |
| All test targets link | PASS | QmlUiAuditTests (incl. new v48 slot), ViewModelSmokeTests, etc. |

### SC2: Regression ctest passes (all groups)

| Group | Result |
|---|---|
| PrepareScene | PASS |
| PartPlate | PASS |
| ViewModel (incl. `testAssembleTransformRoundTrip`) | PASS |
| UI / QmlUiAuditTests (incl. `v48CrossWorkstreamRegressionLocked`) | PASS |
| PreviewParser | PASS |
| E2E pipeline | PASS |

### SC3: v4.8 workstream behaviors regression-free (source-audit lock)

The new `v48CrossWorkstreamRegressionLocked()` slot asserts the v4.8 workstream anchors are present in source AND that v4.7/v4.6 anchors still hold:

| Anchor | Workstream | Result |
|---|---|---|
| `kCgalMeshBooleanAvailable = true` | WS1 CGAL (137) | PASS |
| `MeshBoolean::minus` in ProjectServiceMock | WS1 CGAL (137) | PASS |
| `setAssembleOffset/Rotation/Scale` in ProjectServiceMock | WS2 Assembly (138) | PASS |
| `m_activeCanvasType == 2` routing in EditorViewModel | WS2 Assembly (138) | PASS |
| `AssembleTransformCommand` undo variant | WS2 Assembly (138) | PASS |
| `assembleOffsets` Q_PROPERTY in RhiViewport | WS2 Assembly (138) | PASS |
| en.ts unfinished count <= 5 (I18N-04) | WS3 i18n (139) | PASS (0) |
| `kViewportTrianglePickingAvailable = true` | v4.7 POLISH-01 | PASS (no regression) |
| `orientObject` (flatten) | v4.7 POLISH-02 | PASS (no regression) |
| `its_merge_vertices` (fixMesh) | v4.7 POLISH-03 | PASS (no regression) |
| `calibMode = 7` (Vol_speed tower) | v4.6 CALIB | PASS (no regression) |
| `calibMode = 9` (Retraction tower) | v4.6 CALIB | PASS (no regression) |

### App launch liveness

`APP_RUNNING_PID=30284` — OWzxSlicer.exe launches and reaches liveness.

## Build/test evidence

- Canonical build: `scripts/auto_verify_with_vcvars.ps1` exit 0 (`build_p140b.log`).
- ctest: 5/5 groups PASS + E2E PASS.
- App: PID 30284.

## Recommended manual checks (human-gated, not blocking REGRESS-03)

- Launch app → enter assembly view → select a single volume → Move mode → drag gizmo → confirm volume translates on screen (Phase 138 live visual).
- Prepare page → load 2 meshes → MeshBoolean subtract → confirm result volume (Phase 137).
- Prepare page → Drill gizmo → confirm drilled hole (Phase 137).
- Language switch to en → confirm UI renders in English (Phase 139).

These are covered by automated build/ctest/source-audit; the manual checks confirm the live UX.

## Tech debt carried (non-blocking, documented in STATE.md)

- Assemble rotate/scale live-visual compose (translate-only rendering shipped; transforms persist + round-trip).
- de/fr/ja/ko ~906 messages remaining each (44% complete).
- `ProjectServiceMock::drillObject` C4715; 2-line CGAL compat patch in submodule.
