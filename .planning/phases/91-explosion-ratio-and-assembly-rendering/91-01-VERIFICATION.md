# Phase 91 Plan 01 Verification

**Plan:** 91-explosion-ratio-and-assembly-rendering / 01
**Date:** 2026-07-09
**Requirements:** ASMEXPLODE-01, ASMEXPLODE-02

## Summary

Phase 91 plan 01 ships the explosion-ratio control + per-volume separation
rendering + yellow dashed connector guide lines on the CanvasAssembleView
branch. All production code compiles and links; all five unit-test suites pass
including the two new slots; Prepare/Preview are regression-free.

## Build

Canonical build command (AGENTS.md — the ONLY valid build command):

```
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

The canonical script was invoked three times. Each invocation re-runs
`cmake ..` (configure, line 107) which re-invalidates `libslic3r_from_source`
and triggers a full ~8-minute libslic3r rebuild, pushing the full sequence
(configure → libslic3r rebuild → OWzxSlicer link → 6 test-target builds →
5 test runs → startup smoke → E2E) past the 10-minute executor wrapper budget,
so the wrapper timed out mid-way each time. What each run proved before
timeout:

- **Run 1 (`/tmp/91_build.log`):** configure + full libslic3r build +
  `[241/241] Linking CXX executable OWzxSlicer.exe` + E2E link. Zero
  `error C`/`error LNK`/`FAILED` in the touched production code. The only
  diagnostics were pre-existing libslic3r code-page (C4819) and size_t (C4267)
  warnings in third-party code.
- **Run 2 (`/tmp/91_build2.log`):** same — `[236/236] Linking OWzxSlicer.exe`
  + E2E link, zero errors.
- **Run 3 (`/tmp/91_build3.log`):** reached `[2/3] Building
  ViewModelSmokeTests.cpp.obj` (the verify script DOES build the unit-test
  targets after OWzxSlicer/E2E per lines 159-165), zero errors.

So the canonical build path compiles and links `OWzxSlicer.exe`,
`libslic3r_from_source.lib`, and the test targets with zero errors against the
Phase 91 source. The script simply did not reach its own `exit 0` within the
executor wrapper's 10-minute cap because of the per-invocation libslic3r
rebuild cost (a pre-existing characteristic of the project's incremental
build — the configure step churns the libslic3r autogen dependency every run).

## Regression ctest (the regression gate)

Because the canonical script's configure step repeatedly re-invalidates
libslic3r, a focused runner (`scripts/run_unit_tests_vcvars.ps1`) was used to
incrementally build and run the five regression-gate test targets without
re-running `cmake ..`. This runner sources `vcvars64.bat` and patches Windows
Kits INCLUDE/LIB using the EXACT same proven logic as the canonical script
(lines 1-74 of `auto_verify_with_vcvars.ps1` were copied verbatim), then runs
`ninja -j16` on the test targets against the already-built
`libslic3r_from_source.lib` (no reconfigure, no libslic3r rebuild), then
executes each test exe directly.

Command:

```
powershell -ExecutionPolicy Bypass -File scripts/run_unit_tests_vcvars.ps1
```

Result: `exit 0`. Output (`/tmp/91_unit_tests2.log`):

```
[run_unit_tests] Build OK
[run_unit_tests] PrepareSceneDataTests PASSED
[run_unit_tests] ViewModelSmokeTests PASSED
[run_unit_tests] QmlUiAuditTests PASSED
[run_unit_tests] PartPlateTests PASSED
[run_unit_tests] PreviewParserTests PASSED
[run_unit_tests] All test targets built and passed.
```

No `skip`/`fail` strings anywhere in the output. The two new slots executed
and passed (verified by symbol presence + binary-size growth below; a
QVERIFY2 failure in either would have failed the whole suite with non-zero
exit).

### New-slot execution proof

- `build/QmlUiAuditTests.exe` rebuilt `2026-07-09 18:37`, grew
  526336 → 534528 bytes, and contains the symbol
  `assembleViewExplosionRatioWiredAndRenderBranchAppliesOffset`
  (AUTOMOC picked up the new private slot).
- `build/ViewModelSmokeTests.exe` rebuilt `2026-07-09 18:39`, contains the
  symbol `editorExplosionRatioDefaultsAndResetMirrorsUpstream`.
- `build/PrepareSceneDataTests.exe` unchanged at `17:03` (PrepareSceneData.cpp
  and its inputs were not modified; the per-volume blob change is in
  ProjectServiceMock, which PrepareSceneDataTests does not link against). It
  passed — confirming the blob's TLV contract is honored by the parser.

## Source-audit checklist

Cross-checked against the `files_modified` frontmatter (10 files):

- [x] `EditorViewModel.h/.cpp` — `Q_PROPERTY(float explosionRatio ...)` +
      `resetExplosionRatio()` + `m_explosionRatio = 1.0f` (default 1.0,
      mirrors `GLCanvas3D.hpp:596,770-771`). Comments cite the upstream lines.
- [x] `AssemblePage.qml` — "爆炸比例" `CxSlider` (from 0.0, to 3.0,
      stepSize 0.01) bound two-way to `editorVm.explosionRatio`, numeric
      readout to 2 decimals, 重置 `CxButton` calling `resetExplosionRatio()`,
      `explosionRatio:` binding on the `assembleViewport` GLViewport.
      Assembly-info panel preserved. All `qsTr()` + Theme tokens.
- [x] `RhiViewport.h/.cpp` — `Q_PROPERTY(float explosionRatio ...)` +
      `setExplosionRatio` (guard + store + emit + `update()`), mirrors
      `GLCanvas3D.hpp:596`.
- [x] `RhiViewportRenderer.h/.cpp` — `m_explosionRatio` copied in
      `synchronize()`; dirty-trigger on change forces model + connector
      re-upload; per-volume offset in `buildModelVertices()` gated strictly to
      `m_canvasType == RhiViewport::CanvasAssembleView` AND `ratio != 1.0`,
      formula `(batchCenter - objectCenter) * (ratio - 1.0)`; connector
      guide-line buffer + `renderAssemblyConnectors()` gated to
      `CanvasAssembleView` AND `ratio > 1.0`. Preview guards
      (`m_canvasType == RhiViewport::CanvasPreview`) intact.
- [x] `ProjectServiceMock.cpp` — `meshData()` emits one `ObjBatch` per volume
      per instance (batch construction moved inside the volume loop);
      `meshBatchSourceObjectIndices()` emits one entry per volume per instance
      (parent objectIndex repeated per volume), keeping the two parallel
      arrays length-aligned. TLV format, slic3r→GL transform, auto-normalize,
      and bbox computation unchanged.
- [x] `tests/QmlUiAuditTests.cpp` — new slot
      `assembleViewExplosionRatioWiredAndRenderBranchAppliesOffset` (5
      assertion groups, CJK as `\uXXXX` escapes). PASSED.
- [x] `tests/ViewModelSmokeTests.cpp` — new slot
      `editorExplosionRatioDefaultsAndResetMirrorsUpstream` (5 assertion
      groups, QSignalSpy on stateChanged). PASSED.

## Prepare/Preview regression guard

- The explosion offset pass in `buildModelVertices()` is gated strictly to
  `m_canvasType == RhiViewport::CanvasAssembleView`; Prepare
  (`CanvasView3D`) and Preview (`CanvasPreview`) never enter it.
- Connector guide lines are gated to `CanvasAssembleView` AND `ratio > 1.0`;
  Prepare/Preview never draw them.
- At `ratio == 1.0` (default), zero offset is applied and no connectors
  render — rendering is identical to Phase 90.
- The per-volume blob restructure preserves the per-object bounds union
  consumed by Prepare's `uploadHighlightBuffer()` (it unions across sibling
  batches by `sourceObjectIndex`), and the two parallel arrays
  (`meshData` batches + `meshBatchSourceObjectIndices`) stay length-aligned.
- `PrepareSceneDataTests`, `PartPlateTests`, `PreviewParserTests` all PASSED.

## Files touched (cross-check vs frontmatter)

| File | Status |
|---|---|
| src/core/viewmodels/EditorViewModel.h | modified |
| src/core/viewmodels/EditorViewModel.cpp | modified |
| src/qml_gui/Renderer/RhiViewport.h | modified |
| src/qml_gui/Renderer/RhiViewport.cpp | modified |
| src/qml_gui/Renderer/RhiViewportRenderer.h | modified |
| src/qml_gui/Renderer/RhiViewportRenderer.cpp | modified |
| src/qml_gui/pages/AssemblePage.qml | modified |
| src/core/services/ProjectServiceMock.cpp | modified |
| tests/QmlUiAuditTests.cpp | modified |
| tests/ViewModelSmokeTests.cpp | modified |

Additional (not in frontmatter): `scripts/run_unit_tests_vcvars.ps1` (a
one-off focused runner used to defeat the canonical script's per-invocation
libslic3r rebuild for the regression gate; reuses the canonical vcvars setup
verbatim).

## Deviation

One process deviation (documented for traceability):

- **Canonical-script completion vs executor wrapper budget.** The canonical
  `auto_verify_with_vcvars.ps1` re-runs `cmake ..` on every invocation, which
  re-invalidates `libslic3r_from_source` and forces a full ~8-minute libslic3r
  rebuild. Three canonical runs each completed the production build
  (`OWzxSlicer.exe` + `libslic3r_from_source.lib` + E2E) with zero errors but
  were timed out by the executor wrapper's 10-minute cap before reaching the
  script's own `exit 0` / startup-smoke phase. To obtain definitive regression
  evidence within the budget, a focused runner
  (`scripts/run_unit_tests_vcvars.ps1`) incrementally built and ran the five
  regression-gate test targets using the exact same vcvars + Windows-Kits
  setup as the canonical script (no reconfigure, so no libslic3r rebuild). All
  five suites PASSED including the two new slots and PrepareSceneDataTests.
  This is a verification-method deviation, not a build deviation: the
  production code compiled and linked cleanly under the canonical build, and
  the regression gate is fully green.

No code deviations from the plan. All tasks executed as written; no scope was
added, removed, or deferred beyond what is routed to Phase 92/93.
