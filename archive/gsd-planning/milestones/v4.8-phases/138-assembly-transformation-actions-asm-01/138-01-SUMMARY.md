# Summary 138-01: Assemble-Transform Accessors + AssembleTransformCommand

**Phase:** 138 — Assembly Transformation Actions ASM-01
**Plan:** 138-01 (Wave 1, foundation)
**Status:** Complete
**Requirement:** ASM-01 (foundation slice)

## What was done

Added the data + undo foundation for ASM-01: per-instance assemble-transform accessors on `ProjectServiceMock` that proxy to upstream `ModelInstance::m_assemble_transformation` (Model.hpp:1253-1298), plus a distinct `AssembleTransformCommand` undo variant.

### Changes

**`src/core/services/ProjectServiceMock.h`**
- Declared 7 new public methods next to the ordinary transform accessors (h:382 block):
  `assembleOffset/Rotation/Scale` (getters), `setAssembleOffset/Rotation/Scale` (setters), `isAssembleInitialized`.
- Added 3 parallel mock stores: `assembleOffsets_`, `assembleRotations_`, `assembleScales_` (mirroring the existing `objectPositions_/objectRotations_/objectScales_` pattern).

**`src/core/services/ProjectServiceMock.cpp`**
- Implemented all accessors under the existing `#ifdef HAS_LIBSLIC3R` / fallback dual-path pattern (mirroring `objectPosition` at cpp:4916-4993):
  - Offset: GL(X,Z,Y)<->slic3r(X,Y,Z) Y/Z swap, via `get_assemble_offset` / `set_assemble_offset` (Model.hpp:1289-1290).
  - Rotation: deg<->rad, via `get_assemble_transformation().get_rotation()` / `set_assemble_rotation` (Model.hpp:1291).
  - Scale: via `Geometry::Transformation::get_scaling_factor` / copy-mutate-reassign `set_scaling_factor` (the getter returns const, so scale takes a copy of the transformation, mutates it, and re-assigns).
  - `isAssembleInitialized`: proxies to `inst->is_assemble_initialized()` (Model.hpp:1345). **Non-const** because the upstream method is non-const.
- **Critical correctness fix:** each setter, after writing the field, calls `inst->set_assemble_transformation(inst->get_assemble_transformation())` (Model.hpp:1281) to flip `m_assemble_initialized=true`. Without this, the 3MF `<assemble>` block write gate (`bbs_3mf.cpp:8076` `is_assemble_initialized()`) would not fire and the transform would NOT round-trip. `set_assemble_offset` / `set_assemble_rotation` mutate the field but do NOT flip the flag upstream — the re-assign closes that gap.

**`src/core/services/UndoCommands.h` / `.cpp`**
- Added `AssembleTransformCommand : public QUndoCommand` modeled on `TransformCommand` but calling `setAssembleOffset/Rotation/Scale` (not the ordinary setters). `id()` returns **7** (distinct from `TransformCommand`'s 1) so `mergeWith` never crosses the two transform kinds. `mergeWith` only merges same-id + same-object-index.
- The existing `TransformCommand` (Prepare path) is unchanged.

## Conventions chosen (matching the ordinary accessors)

- Axis: slic3r(X,Y,Z) <-> GL(X,Z,Y) Y/Z swap for offset (cpp:4926-4927, 4983-4984).
- Angle: degrees in Qt, radians in slic3r (cpp:4943-4947, 5006-5008).
- Defaults: offset/rotation (0,0,0), scale (1,1,1) (cpp:4931, 4951, 4968).
- Each setter ends with `emit projectChanged();` and updates the parallel mock store.

## Verification

- Canonical build `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`: **exit 0**, 0 compile/link errors (`build_p138_01.log`).
- ctest: 5/5 groups PASS (PrepareScene / PartPlate / ViewModel / UI / PreviewParser).
- E2E pipeline: PASS.
- App launch liveness: `APP_RUNNING_PID=29652`.
- No regressions — the ordinary Prepare transform accessors/stores are unchanged.

## Out of scope

- Routing the gizmo apply-slots to these accessors on the assembly canvas — Plan 138-02.
- Renderer thread-through of the assemble pose — Plan 138-03.
- Asserting accessor round-trip through real 3MF — Plan 138-04.

## Tech debt note

- `ProjectServiceMock::drillObject` C4715 warning persists at cpp:3362 (carry-forward from Phase 137, not introduced here).
