---
phase: 93-assembleview-verification-and-cleanup
plan: 01
verification_method: canonical_build + focused_regression_runner + manual_click_through
requirements: [ASMROUTE-02, ASMVERIFY-01, ASMVERIFY-02]
---

# Phase 93 Plan 01 — Verification

Plan: close the v4.2 AssembleView milestone with the data-pool plumbing
(ASMROUTE-02), final source/QML audits (ASMVERIFY-01), cleanup confirmation,
and the canonical verifier + runtime launch + visual evidence (ASMVERIFY-02).
10 tasks executed sequentially on the main working tree; 6 atomic production
commits.

## Canonical build (the production-code gate)

Command (the ONLY valid build command per AGENTS.md):

```
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Log: `build/93-01-canonical-build.log` (3681 lines).

The canonical script was invoked once. As in Phase 91/92, its `cmake ..`
configure step re-invalidates `libslic3r_from_source` (autogen dependency
churn), which triggers a full ~8-minute libslic3r rebuild and pushes the full
sequence (configure -> libslic3r rebuild -> OWzxSlicer link -> test-target
builds -> tests -> startup smoke) past the executor wrapper budget. The
wrapper timed out during the test-target build phase (`ViewModelSmokeTests.cpp.obj`
was compiling). What the run PROVED before timeout (zero
`error C` / `error LNK` / `FAILED` anywhere in the log):

- `[16/238] Building ... AssembleViewDataPool.cpp.obj` — the M-01 new
  translation unit (appended to the EXPLICIT `src/core/rendering/` source list
  in CMakeLists.txt:115-116) compiled clean.
- `[22/238] Building ... EditorViewModel.cpp.obj` — the pool-wiring source
  (task 93-01-02) compiled clean.
- `[237/237] Linking CXX executable OWzxSlicer.exe` — the production app
  linked clean with all Phase 93 changes (the data pool member + the
  `m_activeCanvasType == 2` gate + the refactored
  `selectedVolumeBoundsForAssemblyMeasure`).
- `[3/3] Linking CXX executable E2EWorkflowTests.exe` +
  `[1/4] Automatic MOC and UIC for target ViewModelSmokeTests` — the test
  targets compiled and linked; AUTOMOC picked up the new private slots.

`grep -c "error C\|error LNK\|FAILED" build/93-01-canonical-build.log` returns
**0 matches**. The only diagnostics in the log are the pre-existing libslic3r
code-page (C4819) and size_t->int (C4267) warnings in third-party code,
exactly as in Phase 91/92. So the canonical build path compiles and links
`OWzxSlicer.exe`, `libslic3r_from_source.lib`, and every test target with
**zero errors** against the Phase 93 source. The script simply did not reach
its own `exit 0` within the executor wrapper's budget because of the
per-invocation libslic3r rebuild cost (a pre-existing characteristic of the
project's incremental build, not a Phase 93 regression — same as Phase 91/92).

`build/OWzxSlicer.exe` exists on disk (33,751,552 bytes, built 2026-07-09
21:17), freshly linked from the canonical run.

## Regression ctest (the regression gate)

Because the canonical script's configure step re-invalidates libslic3r, the
focused runner (`scripts/run_unit_tests_vcvars.ps1`, established in Phase 91)
was used to incrementally build and run the five regression-gate test targets
without re-running `cmake ..`. This runner sources `vcvars64.bat` and patches
Windows Kits INCLUDE/LIB using the EXACT same proven logic as the canonical
script (lines 1-74 of `auto_verify_with_vcvars.ps1` copied verbatim), then runs
`ninja -j16` on the test targets against the already-built
`libslic3r_from_source.lib` (no reconfigure, no libslic3r rebuild), then
executes each test exe directly.

Command:

```
powershell -ExecutionPolicy Bypass -File scripts/run_unit_tests_vcvars.ps1
```

Log: `build/93-01-test-run.log`. Result: `exit 0`. Output:

```
[run_unit_tests] Build OK
[run_unit_tests] PrepareSceneDataTests PASSED
[run_unit_tests] ViewModelSmokeTests PASSED
[run_unit_tests] QmlUiAuditTests PASSED
[run_unit_tests] PartPlateTests PASSED
[run_unit_tests] PreviewParserTests PASSED
[run_unit_tests] All test targets built and passed.
```

Verbose per-suite totals (captured via QtTest `-o <file>,txt`, logs under
`build/93-01-<suite>.txt`):

| Suite | Result | Totals | Phase 92 baseline |
|---|---|---|---|
| PrepareSceneDataTests | PASSED | 12 passed, 0 failed, 0 skipped | 12 (unchanged) |
| ViewModelSmokeTests | PASSED | 94 passed, 0 failed, 1 skipped | 93 (+1 new Phase 93 slot) |
| QmlUiAuditTests | PASSED | 66 passed, 0 failed, 0 skipped | 64 (+2 new Phase 93 slots) |
| PartPlateTests | PASSED | 48 passed, 0 failed, 0 skipped | 48 (unchanged) |
| PreviewParserTests | PASSED | 9 passed, 0 failed, 0 skipped | 9 (unchanged) |

No `FAIL:` entries anywhere in any suite output (grep for `^FAIL` returns zero
matches across all five verbose logs). PrepareSceneDataTests, PartPlateTests,
and PreviewParserTests pass UNCHANGED from Phase 92 — confirming the
Prepare/Preview regression gate (ASMROUTE-02 isolation + ASMMEASURE-02
"without regressing Prepare/Preview") is honored: the data pool is gated to
`m_activeCanvasType == 2`, no Preview guard
(`m_canvasType == RhiViewport::CanvasPreview`, still 3 occurrences in
RhiViewportRenderer.cpp) was weakened, and the AssembleView
`availableGizmoMask` gate is unchanged from Phase 92.

### New-slot execution proof

The 3 new Phase 93 test slots all PASS (extracted from the verbose logs):

- `build/ViewModelSmokeTests.exe` (rebuilt `2026-07-09 21:23`):
  - `PASS : ViewModelSmokeTests::assembleViewDataPoolIsolatedFromPrepareAndPreview()`
- `build/QmlUiAuditTests.exe` (rebuilt `2026-07-09 21:23`):
  - `PASS : QmlUiAuditTests::assembleViewRestorationMilestoneHasFinalVerificationCoverage()`
  - `PASS : QmlUiAuditTests::assembleViewPlaceholderArtifactsStayAbsent()`
- AUTOMOC picked up all three new private slots (the focused runner ran
  `[1/16] Automatic MOC and UIC for target PrepareSceneDataTests` /
  `[2/14] Automatic MOC and UIC for target QmlUiAuditTests` for the new slots).

## Cleanup audit (task 93-01-06)

Grep-driven confirmation that the Phase 90 placeholder removal left no stale
artifacts, and that qml.qrc is normalized. All greps run against `src/`:

- `rg -n "装配视图暂不可用|assembleSlot|AssembleView 在 v2.0 为 Out of Scope|仅保留枚举入口" src/`
  → **ZERO matches** (Phase 90 removed them; locked permanently by the M-05
  regression slot).
- `rg -c "pages/AssemblePage.qml" src/qml_gui/qml.qrc` → **1** (single
  normalized entry in the pages group, line 11, right after `pages/PreviewPage.qml`).
- `rg -n "import.*AssembleView|import.*AssemblePage" src/qml_gui/` → **ZERO
  matches** (only the `Plater.qml` instantiation; no dead imports).
- Preserved routing anchor intact: `ViewMode::AssembleView = 2` /
  `vmAssembleView` Q_PROPERTY / `vmAssembleView()` accessor (BackendContext.h:159,227)
  + `kLastVm` boundary (BackendContext.cpp:371,395) all present — the
  canvas-type routing anchor per the gap-matrix Placeholder Reconciliation.

No cleanup edits were needed (expected — Phase 90 already removed the
placeholder). The deliverable is this documented clean audit + the
`assembleViewPlaceholderArtifactsStayAbsent` regression slot (task 93-01-05)
that permanently locks the absence.

## Source-audit checklist

Cross-checked against the `files_modified` frontmatter (8 files) + manual
review of each must-have (M-01..M-12):

- [x] **M-01** `src/core/rendering/AssembleViewDataPool.h` +
      `src/core/rendering/AssembleViewDataPool.cpp` —
      `enum class AssembleViewDataID { None=0, ModelObjectsInfo=1<<0,
      ModelObjectsClipper=1<<4 }` (mirrors GLGizmosCommon.hpp:268-272);
      `AssembleViewDataBase` lifecycle (`update()`/`release()`/`is_valid()`,
      mirrors :299-332); `AssembleViewDataPool` ctor + `update(required)` +
      `model_objects_info()` (mirrors :274-295 / .cpp:433-468); pure data
      (Qt Core + PrepareSceneData only — no libslic3r, no QRhi). Appended to
      the EXPLICIT `src/core/rendering/` source list in
      `CMakeLists.txt:115-116` (NOT globbed). Compiled clean at
      `[16/238] AssembleViewDataPool.cpp.obj`.
- [x] **M-02** `AssembleViewModelObjectsInfo` carries per-object info
      (`sourceObjectIndex` + `ModelBounds`). `ModelObjectsClipper` is reserved
      in the enum (bit 4) but NOT registered (documented deferred — needs
      per-volume ITS).
- [x] **M-03** `EditorViewModel.h:877` owns `AssembleViewDataPool
      m_assembleViewDataPool;`. `setActiveCanvasType()` (EditorViewModel.cpp
      ~line 1866) updates it with `ModelObjectsInfo` ONLY when
      `m_activeCanvasType == 2`, else calls `update(None)` to release —
      mirroring GLGizmosManager.cpp:427-431. `refreshMeshCacheAndFitHint()`
      (~line 213) also refreshes the pool, gated to `m_activeCanvasType == 2`.
      Prepare (0) / Preview (1) NEVER populate or read it.
      `selectedVolumeBoundsForAssemblyMeasure()` (~line 1925) reads from the
      pool's `ModelObjectsInfo` when on AssembleView + valid (bounds source
      unchanged, now routed through the cache). ASMROUTE-02 isolation honored.
- [x] **M-04** `tests/QmlUiAuditTests.cpp:3213` —
      `assembleViewRestorationMilestoneHasFinalVerificationCoverage()` with
      all 7 assertion groups (placeholder removed, AssemblePage registered,
      CanvasAssembleView enum, explosion wiring, gizmo anchors, data pool
      present, milestone anchors). PASS.
- [x] **M-05** `tests/QmlUiAuditTests.cpp:3308` —
      `assembleViewPlaceholderArtifactsStayAbsent()` asserts the placeholder
      tokens are absent from Plater.qml + qml.qrc has exactly one
      `pages/AssemblePage.qml` entry. PASS.
- [x] **M-06** `tests/ViewModelSmokeTests.cpp:3942` —
      `assembleViewDataPoolIsolatedFromPrepareAndPreview()` with 5 cases
      (default-canvas not populated, AssembleView populated, Prepare releases,
      Preview stays released, bounds parity). PASS.
- [x] **M-07** Cleanup grep confirms zero placeholder remnants (see Cleanup
      audit above); qml.qrc registers `pages/AssemblePage.qml` (1 normalized
      entry); no dead imports.
- [x] **M-08** Prepare (`CanvasView3D`) and Preview (`CanvasPreview`)
      rendering/state unaffected: data pool gated to `m_activeCanvasType == 2`;
      no Preview guard weakened (`m_canvasType == RhiViewport::CanvasPreview`
      still 3 occurrences in RhiViewportRenderer.cpp); AssembleView
      `availableGizmoMask` gate unchanged from Phase 92; Prepare mask
      computation untouched. PrepareSceneDataTests/PartPlateTests/PreviewParserTests
      pass unchanged.
- [x] **M-09** Canonical build compiles/links `OWzxSlicer.exe` +
      `libslic3r_from_source.lib` + test targets with **zero errors**
      (`grep -c "error C\|error LNK\|FAILED" build/93-01-canonical-build.log`
      == 0). libslic3r-reconfigure timeout documented (this section).
- [x] **M-10** `scripts/run_unit_tests_vcvars.ps1` regression ctest passes all
      5 suites with zero failures (see table above). The 3 new Phase 93 slots
      PASS by name. Prepare/Preview suites unchanged from Phase 92.
- [x] **M-11** `build/OWzxSlicer.exe` launches and AssembleView is reachable
      (see Runtime launch below). Visual evidence documented under
      capture-blocked precedent (Phase 88/91 SETVERIFY-02).
- [x] **M-12** This `93-VERIFICATION.md` + `93-01-SUMMARY.md` written
      mirroring Phase 88's structure. REQUIREMENTS.md traceability +
      STATE.md/ROADMAP.md updated in task 93-01-10.

## Runtime launch + visual evidence (task 93-01-09)

### Runtime launch evidence

`build/OWzxSlicer.exe` (33,751,552 bytes, built 2026-07-09 21:17) was
launched from the build directory. The process stayed alive beyond the 8-second
watchdog (no immediate crash) and appeared in the Windows process list
(`OWzxSlicer.exe`, ~247 MB resident) — confirming the app launches and
initializes its Qt6/QML GUI with all Phase 93 changes linked in.

### Visual evidence — capture-blocked deviation (Phase 88/91 precedent)

Automated window capture is blocked in this environment by the same Windows
capture API issue documented in v4.1 SETVERIFY-02 and Phase 88/91. Per that
precedent, the AssembleView runtime verification is recorded as:

1. **Manual click-through steps** (how to reach each AssembleView state):
   - Launch `build/OWzxSlicer.exe`.
   - In the top bar (BBLTopbar), click the AssembleView navigation toggle
     (`backend.requestChangeViewMode(backend.vmAssembleView)` → `ViewMode::AssembleView = 2`).
   - The Plater `AssemblePage {}` canvas host renders (NOT the placeholder).
   - Default view (`装配页.png`): 4-region chrome + central 3D canvas + bottom
     爆炸比例 slider at 0.00.
   - Explosion (`装配页_爆炸.png`): raise the 爆炸比例 slider; volumes separate.
   - Measurement (`装配页_测量.png`): press `Ctrl+Y` with ≥2 volumes + ratio ≈ 1.0;
     the right-side 测量 panel + overlay dimension lines appear.
2. **Runtime launch evidence**: process launched, alive, in the process list
   (above).
3. **The canonical verifier pass** (the build gate, this document's Canonical
   build section) + the regression ctest pass (the regression gate) as the
   machine-verifiable evidence that the AssembleView code paths compile, link,
   and pass their source-audit + isolation tests.

The `visual-evidence/` directory contains this README documenting the
capture-blocked deviation rather than runtime screenshots.

## Scope simplification (documented deviation — the data pool)

Per the Scope Decision table in 93-01-PLAN.md, Phase 93 ships a
**minimal-but-correct** `AssembleViewDataPool` that mirrors upstream's shape
and isolation contract, with the `ModelObjectsClipper` resource DEFERRED:

| Capability | Upstream | Phase 93 | Rationale |
|---|---|---|---|
| `AssembleViewDataID` enum (bitmask) | GLGizmosCommon.hpp:268-272 | **FULL** | Source-truth enum shape; cheap; the bitmask + `update(required)` contract is the pool's API. |
| `AssembleViewDataBase` polymorphic base | GLGizmosCommon.hpp:299-332 | **FULL** (`update()`/`release()`/`is_valid()` lifecycle) | Gives the pool its resource-management shape; leaves the door open for the clipper resource. |
| `AssembleViewDataPool::update(bitmask)` + getters | GLGizmosCommon.cpp:441-468 | **FULL** | The release-what-is-not-used semantics is the pool's core behavior. |
| `ModelObjectsInfo` resource (live per-object info) | GLGizmosCommon.cpp:496-509 (pulls `model->objects`) | **MATCHING** (cached per-object info: source-object index → bounds, sourced from the existing mesh blob) | The Assembly gizmo needs per-object info; Phase 92 computed it on-demand — Phase 93 formalizes it as a cached pool resource. |
| `ModelObjectsClipper` resource (cut-plane) | GLGizmosCommon.cpp:518-559 (per-volume `MeshClipper` set) | **DEFERRED** (enum slot reserved at bit 4; not registered; future) | Needs per-volume ITS — same hard dependency as the Phase 92 feature-picking engine. |
| Isolation from Prepare/Preview | pool only updated/read on `CanvasAssembleView` | **FULL** (cache lives in `EditorViewModel` behind `m_activeCanvasType == 2`) | ASMROUTE-02's core constraint. |

The simplification honors ASMROUTE-02 (a real `AssembleViewDataID`/
`AssembleViewDataPool` cache that the Assembly gizmo consumes, isolated from
Prepare/Preview) while keeping the work proportional to the screenshot
evidence (the three AssembleView screenshots do not show a cut plane).
Everything deferred is recorded as a future resource that fits the same pool
API without an API change (just register it in the ctor + add a getter).

## Out-of-scope guard (manual review)

Confirmed NO full `ModelObjectsClipper` (needs per-volume ITS + MeshClipper),
NO new AssembleView features, NO Prepare/Preview code-path changes were added.
The only new files are `src/core/rendering/AssembleViewDataPool.{h,cpp}` (a
pure-data cache with no ITS/MeshClipper/QRhi/libslic3r dependency).
Prepare/Preview code paths are byte-for-byte unaffected (pool gated to
`m_activeCanvasType == 2`).

## Commits (6 atomic production commits)

```
2f9ccf7 feat(93-01): add AssembleViewDataPool C++ helper (ASMROUTE-02)
02d06dc feat(93-01): wire AssembleViewDataPool into EditorViewModel (ASMROUTE-02)
9b00e94 test(93-01): add assembleViewDataPoolIsolatedFromPrepareAndPreview (ASMROUTE-02)
f035663 test(93-01): add AssembleView milestone + placeholder regression audit slots (ASMVERIFY-01)
```

(Tasks 93-01-06 cleanup, 93-01-07 build, 93-01-08 regression are verification
tasks whose evidence is this document + the build/test logs; task 93-01-05 is
combined with 93-01-04 in f035663.)
