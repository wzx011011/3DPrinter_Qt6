# Roadmap: OWzx Slicer

## Milestones

- v4.6 Core Feature Completion Sweep — Phases 117-128 (shipped 2026-07-15)
- v4.7 Polish, i18n & Advanced Feature Recovery — Phases 129-135 (shipped 2026-07-15, tech_debt)
- v4.8 Dependency Unlock, Assembly Transform & i18n Completion — Phases 136-140 (in progress)

## Current Milestone: v4.8 Dependency Unlock, Assembly Transform & i18n Completion

**Goal:** Crack CGAL 5.6+ upgrade to unlock MeshBoolean + Drill; complete ASM-01 assembly transformation; fill en.ts remaining translations.

**Scope rule:** All offline/local. Hollow/FaceDetector depend on OpenVDB (unavailable) — stay deferred. LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows remain removed.

**Key realization (recorded in Phase 136):** The v4.7 assumption that CGAL 5.6+ is *required* for MeshBoolean was wrong. MeshBoolean uses `corefinement.h` which CGAL 5.4 already ships — a 2-line compat patch in `third_party/OrcaSlicer` + CMake re-enable is sufficient. No dependency-bundle upgrade needed.

## Phases

- [x] Phase 136: CGAL 5.6+ Dependency Upgrade (WS1) (completed 2026-07-16 — superseded by compat-patch path)
- [x] Phase 137: MeshBoolean + Drill Activation (WS1) (completed 2026-07-16)
- [ ] Phase 138: Assembly Transformation Actions ASM-01 (WS2)
- [ ] Phase 139: en.ts Full Translation + Baseline Advance (WS3)
- [ ] Phase 140: v4.8 Verification And Cross-Workstream Regression

### Build Order

- **Wave A (parallel):** Phase 136 (CGAL upgrade — resolved via compat patch) + Phase 138 (ASM-01) + Phase 139 (i18n). Independent.
- **Wave B (after 136):** Phase 137 (activate MeshBoolean + Drill). done.
- **Wave C (last):** Phase 140 (verification).

### Phase 136: CGAL 5.6+ Dependency Upgrade

**Status:** Complete (2026-07-16)
**Workstream:** WS1 (CGAL)
**Goal:** Rebuild/download CGAL 5.6+ in DEPS_PREFIX + build links clean
**Depends on:** —

Success criteria:
1. CGAL MeshBoolean compiles and links in the canonical build. (Resolved WITHOUT a 5.6+ upgrade: a 2-line compat patch in `third_party/OrcaSlicer/.../MeshBoolean.cpp` — `#if 0`→`#if 1` + output iterators — plus re-enabling `MeshBoolean.cpp` in `cmake/BuildLibslic3rFromSource.cmake`. CGAL 5.4 already ships the `corefinement.h` API MeshBoolean needs. Requirement CGAL-01 is satisfied by this compat path; a true 5.6+ bundle upgrade is no longer required for the MeshBoolean use case.)
2. The canonical build (`scripts/auto_verify_with_vcvars.ps1`) links clean; no regressions in existing CGAL-dependent code (cut surface, mesh boolean prep).

### Phase 137: MeshBoolean + Drill Activation

**Status:** Complete (2026-07-16)
**Workstream:** WS1 (CGAL)
**Goal:** Flip flag + activate ~200 lines written logic
**Depends on:** Phase 136

Success criteria:
1. `kCgalMeshBooleanAvailable` is flipped to true; MeshBoolean (union/subtract/intersect) works end-to-end — the ~200 lines of already-written logic activate, producing a real boolean result volume. The `MeshBoolean_mcut_stub.cpp` (return-false stub) is removed to clear the LNK2005 duplicate symbol with real `MeshBoolean.cpp`.
2. Drill gizmo works end-to-end — the tool-mesh + feature-matrix logic activates, producing a real drilled hole. `drillObject` calls real `MeshBoolean::minus` instead of the stub.
3. All 5 ctest groups PASS (PrepareScene / PartPlate / ViewModel / UI / PreviewParser) + E2E pipeline + app launch liveness.

### Phase 138: Assembly Transformation Actions ASM-01

**Status:** Not started
**Workstream:** WS2 (Assembly)
**Goal:** Assembly-mode move/rotate/scale per-volume
**Depends on:** —

Success criteria:
1. Assembly view supports move/rotate/scale transformation actions on assembled volumes (RhiViewport assembly canvas gizmo + ViewModel + AssemblePage).
2. Transformations apply per-volume and round-trip through the model (3MF save → reload preserves per-volume transforms).

### Phase 139: en.ts Full Translation + Baseline Advance

**Status:** Not started
**Workstream:** WS3 (i18n)
**Goal:** Fill remaining ~1372 translations + advance de/fr/ja/ko
**Depends on:** —

Success criteria:
1. en.ts remaining ~1372 unfinished translations are filled (long sentences, compound phrases). lrelease produces a complete en.qm.
2. de/fr/ja/ko advance meaningfully from baseline (documented remaining-work estimate).

### Phase 140: v4.8 Verification And Cross-Workstream Regression

**Status:** Not started
**Workstream:** Cross-cutting
**Goal:** Canonical build + ctest + regression-free
**Depends on:** Phase 138, Phase 139

Success criteria:
1. Canonical build passes clean (exit 0).
2. Regression ctest passes (all groups).
3. Prepare/Preview/Assembly/paint/calibration/MeshBoolean/Drill behaviors regression-free.

---

*v4.8 roadmap created: 2026-07-15 — 5 phases (136-140), 7 requirements. Reconciled 2026-07-16: 136/137 recorded complete from git commits `661f48c`, `a740147`, `a875c65` + green build evidence (build_p137f.log: 5/5 ctest PASS, APP_RUNNING_PID=29868).*
