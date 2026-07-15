# Roadmap: OWzx Slicer

## Milestones

- ✅ **v2.9** Implementation Realignment and Stabilization — Phases 10-15 (shipped 2026-06-25)
- ✅ **v3.0** PartPlate Core — Phases 16-22 (shipped 2026-06-26)
- ✅ **v3.1** QRhi Rendering — Phases 23-28 (shipped 2026-06-28)
- ✅ **v3.2** Multi-Plate Data Polish — Phases 29-32 (audited 2026-06-28)
- ✅ **v3.3** Slice Preview Main Flow MVP — Phases 33-36 (superseded by v3.4)
- ✅ **v3.4** Import to G-code Complete Workflow — Phases 37-43 (closed by automated E2E)
- ✅ **v3.5** Preset Authoring Complete Workflow — Phases 44-49 (superseded after Phase 46)
- ✅ **v3.6** Screenshot-Driven OrcaSlicer UI Restoration — Phases 50-58 (shipped 2026-07-03)
- ✅ **v3.7** Screenshot-Level UI Parity Closure — Phases 59-64 (2026-07-04)
- ✅ **v3.8** RHI Gizmo Parity — Phases 65-73 (shipped 2026-07-04)
- ✅ **v3.9** Prepare Page UI Restoration — Phases 74-78 (shipped 2026-07-06)
- ✅ **v4.0** Preview Page UI Restoration — Phases 79-83 (shipped 2026-07-07)
- ✅ **v4.1** Parameter Settings Dialogs Source-Truth Restoration — Phases 84-88 (shipped 2026-07-09)
- ✅ **v4.2** AssembleView Source-Truth Restoration — Phases 89-93 (shipped 2026-07-09)
- ✅ **v4.3** Real Thumbnail Capture And 3MF Round-Trip — Phases 94-98 (shipped 2026-07-10)
- ✅ **v4.4** Wipe-Tower Geometry Readback And Real Rendering — Phases 99-102 (shipped 2026-07-12)
- ✅ **v4.5** Backlog Closure — Phases 103-116 (shipped 2026-07-13)
- ✅ **v4.6** Core Feature Completion Sweep — Phases 117-128 (shipped 2026-07-15)
- 🚧 **v4.7** Polish, i18n & Advanced Feature Recovery — Phases 129-135 (in progress)

## Current Milestone: v4.7 Polish, i18n & Advanced Feature Recovery

**Goal:** Fix v4.6 carry-forward paint-gizmo gate bug + small polish; restore English i18n coverage; upgrade CGAL to unlock MeshBoolean + Drill; advance MEASURE-06 assembly transformation.

**Scope rule:** All offline/local. Hollow/FaceDetector depend on OpenVDB (unavailable) — stay deferred. LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows remain removed.

**Target features (4 workstreams, 12 requirements, 7 phases 129-135):**
- **WS1 Polish & bug-fix (POLISH-01..05):** flip stale paint-gizmo gate flag; wire Flatten to real orientObject; make fixMesh call real its_repair; add KBShortcutsDialog; wire ProjectPage property panel.
- **WS2 i18n English (I18N-02/03):** fill en.ts 1493 empty translations; advance de/fr/ja/ko baseline.
- **WS3 CGAL upgrade (CGAL-01/02/03):** upgrade 5.4→5.6+; activate MeshBoolean + Drill (~200 lines already written).
- **WS4 Assembly transformation (ASM-01):** MEASURE-06 assembly-mode move/rotate/scale.
- **REGRESS-02:** cross-workstream regression gate.

## Phases

- [x] Phase 129: Paint-Gizmo Gate Fix + Flatten + FixMesh (WS1) (completed 2026-07-15)
- [x] Phase 130: KBShortcutsDialog + ProjectPage Property Panel (WS1) (completed 2026-07-15)
- [x] Phase 131: English i18n Translation Fill + Baseline Advance (WS2) (completed 2026-07-15)
- [ ] Phase 132: CGAL 5.6+ Upgrade (WS3)
- [ ] Phase 133: MeshBoolean + Drill Activation (WS3)
- [ ] Phase 134: Assembly Transformation Actions MEASURE-06 (WS4)
- [ ] Phase 135: v4.7 Verification And Cross-Workstream Regression

| Phase | Name | Goal | Requirements |
|---|---|---|---|
| 129 | Paint-Gizmo Gate Fix + Flatten + FixMesh | Flip stale flag + wire real orientObject + real its_repair | POLISH-01, POLISH-02, POLISH-03 |
| 130 | KBShortcutsDialog + ProjectPage Property Panel | Add shortcut overview dialog + wire real project metadata | POLISH-04, POLISH-05 |
| 131 | English i18n Translation Fill + Baseline Advance | Fill en.ts 1493 empty + advance de/fr/ja/ko | I18N-02, I18N-03 |
| 132 | CGAL 5.6+ Upgrade | Upgrade dependency + build links clean | CGAL-01 |
| 133 | MeshBoolean + Drill Activation | Flip flag + activate ~200 lines written logic | CGAL-02, CGAL-03 |
| 134 | Assembly Transformation Actions MEASURE-06 | Assembly-mode move/rotate/scale per-volume | ASM-01 |
| 135 | v4.7 Verification And Cross-Workstream Regression | Canonical build + ctest + regression-free | REGRESS-02 |

### Build Order

- **Wave A (parallel):** Phase 129 (WS1 bug fixes) + Phase 131 (WS2 i18n) + Phase 132 (WS3 CGAL upgrade) + Phase 134 (WS4 assembly). All independent.
- **Wave B (after 129):** Phase 130 (WS1 UI additions). After 132: Phase 133 (WS3 activate — needs upgraded CGAL).
- **Wave C (last):** Phase 135 (verification) — needs all.

### Phase 129: Paint-Gizmo Gate Fix + Flatten + FixMesh

**Status:** Not started
**Workstream:** WS1 (Polish)

Success criteria:
1. `kViewportTrianglePickingAvailable` is true (or removed); canActivateGizmo returns true for Support/Seam/MMU paint gizmos; gizmoStatusText no longer falsely reports "viewport triangle picking unavailable".
2. Flatten gizmo calls the real `orientObject` (Slic3r::orientation::orient) instead of the mock 6-hardcoded-face path; the result is a real reorientation.
3. `fixMesh`/`reloadFromDisk` calls real `its_repair_*` (or re-reads from disk) instead of the no-op copy; the mesh is actually repaired.

### Phase 130: KBShortcutsDialog + ProjectPage Property Panel

**Status:** Not started
**Workstream:** WS1 (Polish)

Success criteria:
1. A KBShortcutsDialog exists, is reachable (menu/shortcut), and lists the keyboard shortcuts (aligned with the inline Shortcut{} declarations in main.qml).
2. ProjectPage property panel shows real values (path/format/size/modified-date) from ProjectServiceMock instead of hardcoded "—".

### Phase 131: English i18n Translation Fill + Baseline Advance

**Status:** Not started
**Workstream:** WS2 (i18n)

Success criteria:
1. en.ts has translations for all user-visible strings (the 1493 currently empty); lrelease produces a non-empty en.qm.
2. de/fr/ja/ko advance from the v4.6 baseline with an updated remaining-work estimate (not falsely 100%).

### Phase 132: CGAL 5.6+ Upgrade

**Status:** Not started
**Workstream:** WS3 (CGAL)

Success criteria:
1. CGAL is upgraded to 5.6+ in the dependency bundle (DEPS_PREFIX or vcpkg).
2. The canonical build links against the new CGAL version; no regressions in existing CGAL-dependent code (cut surface, mesh boolean prep).

### Phase 133: MeshBoolean + Drill Activation

**Status:** Not started
**Workstream:** WS3 (CGAL)

Success criteria:
1. `kCgalMeshBooleanAvailable` is true; MeshBoolean (union/subtract/intersect) works end-to-end — the ~200 lines of already-written logic activate, producing a real boolean result volume.
2. Drill gizmo works end-to-end — the tool-mesh + feature-matrix logic activates, producing a real drilled hole.

### Phase 134: Assembly Transformation Actions MEASURE-06

**Status:** Not started
**Workstream:** WS4 (Assembly)

Success criteria:
1. Assembly view supports move/rotate/scale transformation actions on assembled volumes, building on the v4.5/v4.6 feature-picking + AssemblyMeasureGeometry foundation.
2. Transformations apply per-volume in the assembly context and round-trip through the model.

### Phase 135: v4.7 Verification And Cross-Workstream Regression

**Status:** Not started
**Workstream:** Cross-Cutting

Success criteria:
1. Canonical build (j6) exit 0; regression ctest PASS.
2. Cross-workstream: paint gizmos still work after gate-flag flip; CGAL upgrade did not break cut surface; assembly transforms do not break Prepare/Preview.
3. A v47CrossWorkstreamRegressionLocked source-audit slot consolidates the v4.7 anchors.

---

*v4.7 roadmap created: 2026-07-15 — 7 phases (129-135), 12 requirements.*
