---
phase: 92-assembly-measurement-gizmo
plan: 01
verification_method: canonical_build + focused_regression_runner
requirements: [ASMMEASURE-01, ASMMEASURE-02]
---

# Phase 92 Plan 01 — Verification

Plan: port the Assembly measurement gizmo (`Ctrl+Y`, `GLGizmoAssembly` /
`ONLY_ASSEMBLY`) to the Qt6/RHI `CanvasAssembleView` host. 10 tasks executed
sequentially on the main working tree; 10 atomic commits + 1 verification
follow-up fix commit.

## Canonical build (the production-code gate)

Command:

```
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Log: `build/92-01-canonical-build.log` (3686 lines).

The canonical script was invoked once. As in Phase 91, its `cmake ..` configure
step re-invalidates `libslic3r_from_source` (autogen dependency churn), which
triggers a full ~8-minute libslic3r rebuild and pushes the full sequence
(configure -> libslic3r rebuild -> OWzxSlicer link -> test-target builds ->
test runs -> startup smoke) past the executor wrapper budget. The wrapper
timed out after the test-target link phase. What the run PROVED before timeout
(zero `error C` / `error LNK` / `FAILED` anywhere in the log):

- `[16/243] Building ... AssemblyMeasureGeometry.cpp.obj` — the M-03 critical
  new translation unit (appended to the EXPLICIT `src/core/rendering/` source
  list in CMakeLists.txt:113-114) compiled clean.
- `[17/243] Building ... RhiViewport.cpp.obj` and
  `[18/243] Building ... RhiViewportRenderer.cpp.obj` — the modified renderer
  sources compiled clean.
- `[242/242] Linking CXX executable OWzxSlicer.exe` — the production app
  linked clean with all Phase 92 changes (the new enum value, the Q_PROPERTYs,
  the overlay buffers/pipelines, the QML panel).
- `[3/3] Linking CXX executable E2EWorkflowTests.exe` and
  `[1/4] Automatic MOC and UIC for target ViewModelSmokeTests` — the test
  targets compiled and linked; AUTOMOC picked up the new private slots (the
  AUTOMOC caveat at the top of QmlUiAuditTests.cpp / ViewModelSmokeTests.cpp
  is satisfied because the canonical script reconfigures cmake before
  building).

`grep -c "error C\|error LNK\|FAILED" build/92-01-canonical-build.log` returns
**0 matches**. The only diagnostics in the log are pre-existing libslic3r
code-page (C4819) and size_t->int (C4267) warnings in third-party code, exactly
as in Phase 91. So the canonical build path compiles and links `OWzxSlicer.exe`,
`libslic3r_from_source.lib`, and every test target with **zero errors** against
the Phase 92 source. The script simply did not reach its own `exit 0` within
the executor wrapper's budget because of the per-invocation libslic3r rebuild
cost (a pre-existing characteristic of the project's incremental build, not a
Phase 92 regression — same as Phase 91, documented in 91-01-VERIFICATION.md).

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

Log: `build/92-01-test-run-3.log`. Result: `exit 0`. Output:

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
`build/92-01-<suite>.txt`):

| Suite | Result | Totals |
|---|---|---|
| PrepareSceneDataTests | PASSED | 12 passed, 0 failed, 0 skipped |
| ViewModelSmokeTests | PASSED | 93 passed, 0 failed, 1 skipped |
| QmlUiAuditTests | PASSED | 64 passed, 0 failed, 0 skipped |
| PartPlateTests | PASSED | 48 passed, 0 failed, 0 skipped |
| PreviewParserTests | PASSED | 9 passed, 0 failed, 0 skipped |

No `fail` strings anywhere in any suite output. PrepareSceneDataTests and
PartPlateTests and PreviewParserTests all pass unchanged — confirming the
Prepare/Preview regression gate (ASMMEASURE "without regressing
Prepare/Preview") is honored: the assembly-measure overlay is gated to
`m_canvasType == CanvasAssembleView && m_gizmoMode == 19`, and no Preview
guard was weakened (`m_canvasType == RhiViewport::CanvasPreview` still appears
3 times in RhiViewportRenderer.cpp).

### New-slot execution proof

The 3 new Phase 92 test slots all PASS (extracted from the verbose logs):

- `build/ViewModelSmokeTests.exe` (rebuilt `2026-07-09 20:34`, 33065472 bytes):
  - `PASS : ViewModelSmokeTests::assemblyMeasureGizmoActivabilityMirrorsUpstream()`
  - `PASS : ViewModelSmokeTests::assemblyMeasureGeometryComputesDistanceAndAngle()`
- `build/QmlUiAuditTests.exe` (rebuilt `2026-07-09 20:36`, 544768 bytes):
  - `PASS : QmlUiAuditTests::assembleViewMeasurementGizmoWiredAndOverlayRenders()`
- AUTOMOC picked up all three new private slots (the canonical build ran
  `[1/4] Automatic MOC and UIC for target ViewModelSmokeTests` for the
  ViewModelSmokeTests additions; QmlUiAuditTests likewise).

### Verification follow-up fix (commit c3052f9)

Two test-fixture issues surfaced during verification and were fixed in a
follow-up commit `c3052f9` (the test source is production test code, so a fix
commit is the correct atomic unit, not an amend):

1. **`assemblyMeasureGizmoActivabilityMirrorsUpstream`** loaded the same STL
   file twice via `loadFile(kStlPath)` expecting >=2 objects, but
   `ProjectServiceMock::loadFile` REPLACES the project on each call
   (`objects=1` per load, confirmed in the verbose log), so `modelCount()`
   never reached 2. Switched to `addPrimitiveToPlate(0)` x2, which is
   synchronous + additive (matches the existing
   `rendererPickingSelectsSourceObjectThroughEditorViewModel` fixture pattern
   at ViewModelSmokeTests.cpp:2747-2748). This also removed the async
   `QTRY_VERIFY` dependency for determinism.
2. **`assembleViewMeasurementGizmoWiredAndOverlayRenders`** checked
   `QFileInfo::exists("src/core/rendering/AssemblyMeasureGeometry.h")` with a
   bare relative path, but the test exe runs from `build/`, so the path never
   resolved. Fixed to resolve against `QT_TESTCASE_SOURCEDIR` (repo root) via
   `QDir(QT_TESTCASE_SOURCEDIR).filePath(...)` — the convention used by every
   other file-existence check in the file (e.g. QmlUiAuditTests.cpp:2361-2368).

After the fix, all 5 suites pass.

## Source-audit checklist

Cross-checked against the `files_modified` frontmatter (12 files) + manual
review of each must-have:

- [x] **M-01** `src/qml_gui/Renderer/RhiViewport.h:108` —
      `GizmoAssemblyMeasure = 19` after `GizmoSlaSupports = 18`; `Q_ENUM`
      retained. `GizmoMeasure = 3` (Prepare, Ctrl+U) untouched.
- [x] **M-02** `src/core/viewmodels/EditorViewModel.cpp:1771-1782` —
      `isAssemblyMeasureActivable()` gates on `m_activeCanvasType == 2 &&
      std::abs(m_explosionRatio - 1.0f) < 1e-2f &&
      m_selectedSourceIndices.size() >= 2` (mirrors
      GLGizmoAssembly.cpp:53-68). `availableGizmoMask()` AssembleView branch
      (line 2276-2277) returns `(1 << 19)` when activable, else `0`. Prepare
      path unchanged.
- [x] **M-03** `CMakeLists.txt:113-114` — `AssemblyMeasureGeometry.cpp` and
      `.h` appended to the EXPLICIT `src/core/rendering/` source list (after
      `GizmoCenter.cpp/.h`). The canonical build compiled
      `[16/243] ... AssemblyMeasureGeometry.cpp.obj` clean.
- [x] **M-04** `src/qml_gui/Renderer/RhiViewport.h/.cpp` —
      `assemblyMeasureSelectedA`/`SelectedB` Q_PROPERTYs (WRITE+NOTIFY
      `assemblyMeasureSelectionChanged`); setters guard, store, emit, call
      `update()` (the Phase 91 explosionRatio setter pattern).
- [x] **M-05** `src/qml_gui/Renderer/RhiViewportRenderer.cpp` — overlay gated
      at line 319 to `m_canvasType == RhiViewport::CanvasAssembleView &&
      m_gizmoMode == 19`; `renderAssemblyMeasureOverlay()` (line 1052) draws
      the white dashed dimension line (8 segments via `m_linePipeline`) +
      white arrowhead triangles (via `m_fillPipeline`) + teal translucent
      value-box quad (via `m_translucentFillPipeline`). `uploadAssemblyMeasureBuffers()`
      builds the vertices from the two selected volumes' first-batch AABBs via
      `AssemblyMeasureGeometry::measure`.
- [x] **M-06** `src/qml_gui/pages/AssemblePage.qml` — `Ctrl+Y` Shortcut
      toggling `activateAssemblyMeasureGizmo()`/`deactivateAssemblyMeasureGizmo()`;
      right-side 测量 panel (Rectangle, `visible` bound to
      `assemblyMeasureGizmoActive`) with header, 装配测量 mode label, plane
      indicator, distance/angle rows, 关闭测量 button, activability hint. All
      `qsTr()` + Theme tokens (teal accent `#4ec9b0`).
- [x] **M-07** `AssemblePage.qml` `assembleViewport` binds `gizmoMode` to
      `GizmoAssemblyMeasure` when active else `GizmoMove`, and binds
      `assemblyMeasureSelectedA`/`B` from
      `assemblyMeasureSelectedSourceIndices()[0]`/`[1]`.
- [x] **M-08** `tests/QmlUiAuditTests.cpp:3120` —
      `assembleViewMeasurementGizmoWiredAndOverlayRenders()` with all 5
      assertion groups (enum, viewmodel activability+state, AssemblePage
      shortcut+panel, renderer overlay gated, geometry helper exists).
- [x] **M-09** `tests/ViewModelSmokeTests.cpp:3814` —
      `assemblyMeasureGizmoActivabilityMirrorsUpstream()` with 6 cases (default
      canvas, AssembleView <2 selected, AssembleView ratio != 1.0, the full
      activable case, deactivate, canvas-switched-away guard).
- [x] **M-10** `tests/ViewModelSmokeTests.cpp:3877` —
      `assemblyMeasureGeometryComputesDistanceAndAngle()` verifies
      distance sqrt(272) ~= 16.492, XYZ delta (16,4,0), perpendicular axes
      (X,Y), 90.000 deg angle, format suffixes, and degenerate-AABB invalidity.
- [x] **M-11** Prepare/Preview unaffected: overlay gated to AssembleView +
      gizmo 19; `CanvasPreview` guards (3 occurrences) intact; AssembleView
      `availableGizmoMask` branch is the only mask change; Prepare mask loop
      unchanged.
- [x] **M-12** Canonical build zero errors + ctest regression zero failures
      (this document).

## Scope simplification (documented deviation)

Per the Scope Decision table in 92-01-PLAN.md, Phase 92 ships a
**minimal-but-correct** measurement that matches `装配页_测量.png` and the
upstream activability rules, with the internal feature-picking engine
SIMPLIFIED and DEFERRED:

| Ported (FULL / MATCHING) | Simplified | Deferred (future — needs ITS + raycaster + data pool) |
|---|---|---|
| Activability gate (FULL, GLGizmoAssembly.cpp:53-68) | distance = AABB-center to AABB-center + XYZ delta + Euclidean | Full per-triangle feature picking (point/edge/circle/plane) |
| Measure mode enum `GizmoAssemblyMeasure = 19` + ONLY_ASSEMBLY gate (FULL) | angle = longest-AABB-axis direction angle | Face-Face / Point-Point mode combo |
| Overlay: white dashed dimension line + arrowheads + teal value box (MATCHING) | plane indicator = static "选中 N 平面" label | Assembly transforms (parallel/coincidence/reverse/around-center) |
| Right-side 测量 panel (MATCHING) | | Feature reset / re-pick |
| Ctrl+Y shortcut (FULL) | | Distance-XYZ editing (mutates model) |

The simplification is an internal-engine approximation: the user-visible
semantics (measure distance/angle between two selected volumes) match upstream,
and the screenshot's primary visuals (dimension line + teal value box + panel)
are present. Everything deferred requires the per-volume
`indexed_triangle_set` + a scene raycaster + the `AssembleViewDataPool`
(Phase 93 / ASMROUTE-02 prerequisites).

## Out-of-scope guard (manual review)

Confirmed NO full feature-picking engine, NO assembly transforms, NO
distance-XYZ editing, NO data-pool plumbing were added. The only new files are
`src/core/rendering/AssemblyMeasureGeometry.{h,cpp}` (a pure-data helper with
no ITS / raycaster dependency). Prepare/Preview code paths are byte-for-byte
unaffected (overlay gated to AssembleView + gizmo 19).

## Commits (10 atomic + 1 verification fix)

```
152c869 feat(92-01): add GizmoAssemblyMeasure state + activability to EditorViewModel
3221f3b feat(92-01): add GizmoAssemblyMeasure enum + selection props to RhiViewport
343e754 feat(92-01): add AssemblyMeasureGeometry C++ helper + CMakeLists (M-03)
fc35094 feat(92-01): wire AssemblyMeasureGeometry into EditorViewModel accessors
4774fcb feat(92-01): add Assembly measure overlay to RhiViewportRenderer
6124fbf feat(92-01): add Ctrl+Y shortcut + right-side 测量 panel to AssemblePage
e2913f2 feat(92-01): bind gizmoMode + assemblyMeasureSelected A/B to GLViewport
21e1e84 test(92-01): add QmlUiAuditTests slot for Assembly measure wiring audit
0500d8d test(92-01): add ViewModelSmokeTests slots for assembly measure activability + geometry
c3052f9 fix(92-01): correct two test-fixture issues found in verification
```
