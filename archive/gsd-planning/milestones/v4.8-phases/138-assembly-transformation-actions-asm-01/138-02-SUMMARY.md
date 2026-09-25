# Summary 138-02: Assembly-Canvas Gizmo Routing

**Phase:** 138 — Assembly Transformation Actions ASM-01
**Plan:** 138-02 (Wave 2, deps 138-01)
**Status:** Complete
**Requirement:** ASM-01 (routing slice)

## What was done

Widened `availableGizmoMask()` to advertise Move/Rotate/Scale on AssembleView and branched all 9 gizmo drag-lifecycle slots to write the per-instance assemble transform when on the assembly canvas. No new gizmo machinery — pure routing.

### Changes (`src/core/viewmodels/EditorViewModel.cpp`)

**Task 1 — Mask widening.** The AssembleView branch of `availableGizmoMask()` (was: return bit 19 or 0) now builds `assembleMask`:
- OR in bits 0/1/2 (GizmoMove/Rotate/Scale) when exactly one source object is selected (same predicate `canActivateGizmo` uses: `!hasSelectedVolume() && m_selectedSourceIndices.size() == 1`).
- Still OR in bit 19 (GizmoAssemblyMeasure) when `isAssemblyMeasureActivable()`.
- Empty selection → mask 0 (unchanged behavior).
- Prepare path (`m_activeCanvasType != 2`) unchanged.

**Task 2 — Per-slot assemble routing.** Each of the 9 slots (`beginGizmoMoveDrag`/`applyGizmoMoveDelta`/`endGizmoMoveDrag`, plus Rotate and Scale triplets) now has an `if (m_activeCanvasType == 2)` branch that mirrors the Prepare body but:
- reads baseline via `assembleOffset/Rotation/Scale(idx)` (Plan 01);
- writes via `setAssembleOffset/Rotation/Scale(...)`;
- pushes `AssembleTransformCommand` (id 7) instead of `TransformCommand` (id 1);
- **skips `invalidateSliceResultsForCurrentPlate()`** (the assemble pose is not sliced — T-07);
- macro labels suffixed `(assembly)` to distinguish in the undo stack.
- `begin*Drag` captures the assemble transform baseline into the existing `m_gizmo{Move,Rotate,Scale}DragStart*` members.
- Every new branch ends with `emit stateChanged();`.

The Prepare branch in every slot is byte-for-byte unchanged.

## Verification

- Canonical build `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`: **exit 0**, 0 errors (`build_p138_02b.log`).
- ctest: 5/5 groups PASS — critically **ViewModelSmokeTests PASS**, confirming no Prepare-path regression.
- E2E pipeline: PASS.
- App launch liveness: `APP_RUNNING_PID=33344`.

## Rationale notes

- **Slice-invalidation skip on AssembleView:** `invalidateSliceResultsForCurrentPlate()` dirties the slice cache for the Prepare transform. The assemble transform is a non-sliced pose (it does not affect G-code), so invalidating on assemble edits would falsely mark clean slices dirty. Skipped only on the assembly canvas.
- **Distinct undo command id:** `AssembleTransformCommand::id() == 7` vs `TransformCommand::id() == 1` ensures `mergeWith` never merges an assemble drag into a Prepare drag (or vice versa), preventing cross-canvas undo corruption.

## Out of scope

- Renderer thread-through of the assemble pose (so the canvas visually reflects the edited pose) — Plan 138-03.
- AssemblePage transform-mode selector UI — Plan 138-03.
- Round-trip test asserting the transform survives 3MF save/load — Plan 138-04.

## Note

The assembly-canvas interaction is not yet user-visible (Plan 03 wires the UI; the renderer thread-through is also Plan 03). This plan closes the routing + undo plumbing that those plans build on.
