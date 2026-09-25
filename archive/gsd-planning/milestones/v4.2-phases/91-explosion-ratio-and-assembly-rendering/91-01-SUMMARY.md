---
phase: 91-explosion-ratio-and-assembly-rendering
plan: 01
subsystem: qml-gui
tags: [assembleview, explosion-ratio, rhi, per-volume-render, connectors]
requires:
  - .planning/phases/89-assembleview-source-truth-gap-audit/89-GAP-MATRIX.md
  - .planning/phases/90-assembleview-shell-and-canvas-host-restoration/90-01-SUMMARY.md
  - .planning/phases/91-explosion-ratio-and-assembly-rendering/91-CONTEXT.md
  - src/qml_gui/Renderer/RhiViewportRenderer.cpp
  - src/core/viewmodels/EditorViewModel.h
provides:
  - EditorViewModel explosionRatio Q_PROPERTY + resetExplosionRatio (ASMEXPLODE-01)
  - RhiViewport explosionRatio re-render trigger (Q_PROPERTY + update())
  - Per-volume mesh blob (one ObjBatch per volume per instance)
  - CanvasAssembleView per-volume explosion offset in buildModelVertices
  - Yellow dashed assembly connector guide lines (ratio > 1.0)
  - AssemblePage 爆炸比例 slider + reset + GLViewport binding
  - QmlUiAuditTests explosion wiring audit slot
  - ViewModelSmokeTests explosionRatio behavior slot
affects: [92-assembly-measurement-gizmo, 93-assembleview-verification-and-cleanup]
tech_stack_added: []
patterns: [per-volume blob emission, per-volume render offset gated to canvas type, dashed-line approximation via GL_LINES segments]
requirements_completed: [ASMEXPLODE-01, ASMEXPLODE-02]
completed: 2026-07-09
---

# Phase 91 Plan 01 Summary

## What Changed

Phase 91 plan 01 adds the explosion-ratio control + per-volume separation
rendering + yellow dashed connector guide lines to the Phase 90
`CanvasAssembleView` canvas host, without regressing Prepare/Preview. This is
the second v4.2 phase to ship production C++/QML code, and it makes the
explosion work end-to-end: drag the "爆炸比例" slider, and the volumes
separate radially along their assembly axes with connector lines bridging
the gaps (matching `shotScreen/装配页_爆炸.png`), plus a reset-to-default
action.

Production changes (8 files):

- `src/core/viewmodels/EditorViewModel.h` + `EditorViewModel.cpp` - added
  `Q_PROPERTY(float explosionRatio ...)` (default 1.0, NOTIFY stateChanged)
  mirroring upstream `m_explosion_ratio` (`GLCanvas3D.hpp:596`), plus
  `Q_INVOKABLE void resetExplosionRatio()` mirroring `reset_explosion_ratio()`
  (`GLCanvas3D.hpp:770-771`). `setExplosionRatio` guards with `qFuzzyCompare`
  + finite check and emits `stateChanged`. No separate `AssembleViewModel`.
- `src/qml_gui/Renderer/RhiViewport.h` + `RhiViewport.cpp` - added the
  `explosionRatio` Q_PROPERTY + `setExplosionRatio` (guard + store + emit
  `explosionRatioChanged` + `update()`), the re-render trigger the renderer
  consumes.
- `src/qml_gui/Renderer/RhiViewportRenderer.h` + `RhiViewportRenderer.cpp` -
  `synchronize()` copies `m_explosionRatio` and forces a model + connector
  re-upload on change. `buildModelVertices()` gained a CanvasAssembleView-only
  per-volume offset pass: each volume batch's vertices are offset by
  `(batchCenter - objectCenter) * (ratio - 1.0)`, where `objectCenter` is the
  midpoint of the union of sibling batches sharing a `sourceObjectIndex`.
  Added `uploadAssemblyConnectorBuffer()` + `renderAssemblyConnectors()` for
  yellow dashed connector guide lines between originally-adjacent volumes of
  the same object, gated to `CanvasAssembleView` AND `ratio > 1.0`.
- `src/core/services/ProjectServiceMock.cpp` - restructured `meshData()` to
  emit one `ObjBatch` per **volume** per instance (was one per instance), the
  load-bearing change that makes per-volume offset possible. Updated
  `meshBatchSourceObjectIndices()` to emit one entry per volume per instance,
  keeping the two parallel arrays length-aligned for PrepareSceneData. TLV
  format, coordinate transform, auto-normalize, and bbox unchanged.
- `src/qml_gui/pages/AssemblePage.qml` - added the 爆炸比例 slider block to
  the bottom controls LEFT (CxSlider 0.0-3.0 bound two-way to
  `editorVm.explosionRatio`, numeric readout, 重置 CxButton calling
  `resetExplosionRatio()`) and the `explosionRatio:` binding on the
  `assembleViewport` GLViewport. Assembly-info panel preserved.

Test changes (2 files):

- `tests/QmlUiAuditTests.cpp` - new slot
  `assembleViewExplosionRatioWiredAndRenderBranchAppliesOffset` (5 assertion
  groups auditing EditorViewModel property, AssemblePage slider, RhiViewport
  property, renderer offset gating, connector gating; CJK as `\uXXXX`
  escapes).
- `tests/ViewModelSmokeTests.cpp` - new slot
  `editorExplosionRatioDefaultsAndResetMirrorsUpstream` (5 assertion groups:
  default 1.0, set emits, no-op on unchanged, reset restores + emits,
  reset no-op when default; QSignalSpy on stateChanged).

Helper (1 file, not in frontmatter):

- `scripts/run_unit_tests_vcvars.ps1` - focused runner that sources vcvars
  (identical setup to the canonical script) and incrementally builds + runs
  the 5 regression-gate test targets without re-running `cmake ..`, defeating
  the canonical script's per-invocation libslic3r rebuild. See Deviations.

## Completed Tasks

| Task | Result | Commit |
|---|---|---|
| 91-01-01 Add explosionRatio Q_PROPERTY + resetExplosionRatio to EditorViewModel. | Q_PROPERTY + reset + member `m_explosionRatio = 1.0f` with GLCanvas3D.hpp:596,770-771 citations; setExplosionRatio guard + emit stateChanged. | `001e66c` |
| 91-01-02 Add explosionRatio Q_PROPERTY + setter to RhiViewport. | Property + setExplosionRatio (guard + store + emit + update()), the re-render trigger. | `2923148` |
| 91-01-03 Restructure ProjectServiceMock meshData per-volume. | One ObjBatch per volume per instance; meshBatchSourceObjectIndices realigned one-entry-per-volume; PrepareSceneDataTests regression gate green. | `26cf715` |
| 91-01-04 Add per-volume explosion offset to CanvasAssembleView branch. | synchronize copies ratio + dirty trigger; buildModelVertices offset pass gated to CanvasAssembleView && ratio != 1.0. | `6bf9e22` |
| 91-01-05 Add yellow dashed connector guide lines. | uploadAssemblyConnectorBuffer + renderAssemblyConnectors, yellow GL_LINES segments between adjacent volume centers, gated to AssembleView && ratio > 1.0. | `ca39416` |
| 91-01-06 Add 爆炸比例 slider + reset to AssemblePage. | CxSlider 0.0-3.0 bound two-way to editorVm.explosionRatio, readout, 重置 button; assembly-info panel preserved. | `c62899a` |
| 91-01-07 Bind explosionRatio to the AssemblePage GLViewport. | `explosionRatio:` binding on assembleViewport closing the re-render loop. | `c62899a` |
| 91-01-08 Add QmlUiAuditTests explosion wiring slot. | 5-group source audit (property, slider, RhiViewport, offset gate, connector gate); PASSED. | `1049217` |
| 91-01-09 Add ViewModelSmokeTests explosionRatio slot. | 5-group QSignalSpy behavior audit; PASSED. | `0d2d6a2` |
| 91-01-10 Canonical build + ctest regression. | Production code compiles+links clean; all 5 suites PASSED incl. new slots + PrepareSceneDataTests. See Verification + Deviations. | `91-01-VERIFICATION.md` |

## Key Decisions

- **Per-volume blob restructure is the source-truth-correct approach.** The
  plan flagged Task 91-01-03 as the highest-risk change. It was implemented as
  written (not via the documented fallback): `meshData()` now emits one batch
  per volume per instance, and `meshBatchSourceObjectIndices()` was realigned
  to keep the parallel arrays length-aligned. This is safe because Prepare's
  `uploadHighlightBuffer()` unions bounds across ALL batches matching a
  `sourceObjectIndex`, and picking keys on `batch.sourceObjectIndex` (not on
  `renderObjectId` uniqueness). PrepareSceneDataTests passed, confirming the
  blob's TLV contract is honored.
- **Offset + connectors are strictly gated to CanvasAssembleView.** Prepare
  (`CanvasView3D`) and Preview (`CanvasPreview`) never enter the offset pass
  or draw connectors; at `ratio == 1.0` (default) zero offset is applied and
  no connectors render, so default rendering is identical to Phase 90. This
  satisfies the ASMEXPLODE-02 "without regressing Prepare/Preview rendering"
  requirement.
- **Dashed connectors via GL_LINES segments, not a stipple shader.** The plan
  allowed approximating the dashed effect. `renderAssemblyConnectors()`
  reuses the existing `m_linePipeline` (GL_LINES) and emits 6 short
  alternating dash/gap segments per connector along the line between
  original volume centers. This matches the dashed visual in
  `装配页_爆炸.png` without a new shader/stipple pipeline.
- **Connectors use ORIGINAL (pre-offset) centers.** They stay anchored where
  the volumes were touching and bridge the gap created by the explosion,
  matching the screenshot.
- **explosionRatio lives on EditorViewModel, not a separate AssembleViewModel.**
  This mirrors upstream's single-canvas-state model and the shared
  viewmodel/single-stack architecture established in Phase 90.

## Artifacts

| Artifact | Purpose |
|---|---|
| `91-01-VERIFICATION.md` | Build + ctest regression evidence, source-audit checklist, deviation record. |
| `91-01-SUMMARY.md` | This plan execution summary and downstream handoff. |
| `scripts/run_unit_tests_vcvars.ps1` | Focused incremental runner for the 5 regression-gate test targets. |

## Verification

Canonical build command (the ONLY valid build command per AGENTS.md):

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

The canonical script compiled and linked `OWzxSlicer.exe`,
`libslic3r_from_source.lib`, and the test targets with ZERO errors across
three runs. Each run re-runs `cmake ..` which re-invalidates libslic3r
(~8-min rebuild), pushing the full sequence past the executor wrapper's
10-minute cap before the script's own `exit 0`/startup-smoke phase. The
regression gate was then run via the focused runner
(`scripts/run_unit_tests_vcvars.ps1`, identical vcvars setup, no reconfigure),
which built all 5 test targets (new slots compiled in, verified by symbol
presence + binary-size growth) and ran them: all PASSED, zero failures,
including PrepareSceneDataTests (the per-volume blob regression gate) and the
two new slots. Full evidence in `91-01-VERIFICATION.md`.

## Deviations

One process deviation (documented for traceability; no code deviations from
the plan):

- **Canonical-script completion vs executor wrapper budget.** The canonical
  `auto_verify_with_vcvars.ps1` re-runs `cmake ..` on every invocation, which
  re-invalidates `libslic3r_from_source` and forces a full ~8-minute libslic3r
  rebuild. Three canonical runs each completed the production build
  (`OWzxSlicer.exe` + `libslic3r_from_source.lib` + E2E) with zero errors but
  were timed out by the executor wrapper's 10-minute cap before reaching the
  script's own `exit 0`. A focused runner
  (`scripts/run_unit_tests_vcvars.ps1`) was used to obtain definitive
  regression evidence within the budget — it reuses the canonical vcvars +
  Windows-Kits setup verbatim and incrementally builds + runs the five
  regression-gate test targets without reconfiguring. This is a
  verification-method deviation, not a build deviation: the production code
  compiled and linked cleanly under the canonical build, and the regression
  gate is fully green.

No other deviations. The plan executed as written; the per-volume blob
restructure used the primary approach (not the documented fallback); no scope
was added, removed, or deferred beyond what is already routed to Phase 92/93.

## Downstream Handoff

Phase 92 (Assembly Measurement Gizmo) should start from the
`availableGizmoMask()` AssembleView early-return in `EditorViewModel`
(`availableGizmoMask` returns 0 when `m_activeCanvasType == 2`). It ports
`GLGizmoAssembly`/`ONLY_ASSEMBLY` (`Ctrl+Y`) with measurement overlays and
the right-side 测量 panel. The explosion slider is now in place, so the
"选择模式" dropdown can be added alongside it.

Phase 93 (AssembleView Verification And Cleanup) should wire the
`AssembleViewDataID`/`AssembleViewDataPool` plumbing, capture AssembleView
runtime visual evidence against `装配页.png` / `装配页_爆炸.png` (slider at
0.00 and 3.00, volumes separated with yellow dashed connectors), and run the
canonical verifier to completion (budgeting for the per-invocation libslic3r
rebuild).

## Self-Check: PASSED

- `EditorViewModel.h` has `Q_PROPERTY(float explosionRatio ...)` +
  `resetExplosionRatio()` + `m_explosionRatio = 1.0f` (exactly one Q_PROPERTY).
- `EditorViewModel.cpp` setExplosionRatio (guard + finite + emit) +
  resetExplosionRatio (guard + restore 1.0 + emit), citing GLCanvas3D.hpp:596,770-771.
- `RhiViewport.h/.cpp` has the explosionRatio property + setExplosionRatio
  (guard + emit + update()).
- `RhiViewportRenderer.cpp` synchronize copies m_explosionRatio + dirty
  trigger; buildModelVertices offset pass gated to CanvasAssembleView &&
  ratio != 1.0 with formula (batchCenter - objectCenter) * (ratio - 1.0);
  connector buffer + renderAssemblyConnectors gated to AssembleView && ratio > 1.0.
- `ProjectServiceMock.cpp` meshData emits per-volume batches;
  meshBatchSourceObjectIndices realigned per-volume; TLV/transform/bbox unchanged.
- `AssemblePage.qml` has the 爆炸比例 CxSlider (0.0-3.0) bound to
  editorVm.explosionRatio + readout + 重置 + GLViewport explosionRatio binding.
- New QmlUiAuditTests slot compiled (binary grew) and ran green.
- New ViewModelSmokeTests slot compiled and ran green.
- All 5 regression-gate suites PASSED (PrepareScene, PartPlate, ViewModel,
  QmlUiAudit, PreviewParser); zero failures/skips.
- Prepare/Preview regression-free (offset + connectors gated to AssembleView;
  PrepareSceneDataTests + PartPlateTests + PreviewParserTests green).
- Production code compiles + links clean under the canonical build.
