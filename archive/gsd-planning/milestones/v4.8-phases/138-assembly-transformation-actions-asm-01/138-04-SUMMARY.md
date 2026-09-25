# Summary 138-04: Assemble-Transform 3MF Round-Trip Test

**Phase:** 138 — Assembly Transformation Actions ASM-01
**Plan:** 138-04 (Wave 4, deps 138-01/02/03)
**Status:** Complete
**Requirement:** ASM-01 (persistence slice — closes criterion 2)

## What was done

Added an automated round-trip test that locks in ASM-01's persistence guarantee: a per-instance assemble transform written through the Plan 01 accessors survives a REAL 3MF save (`saveProjectAs` -> `Slic3r::store_3mf`) and reload (`loadProject`).

### Changes (`tests/ViewModelSmokeTests.cpp`)

- New private slot `testAssembleTransformRoundTrip` (declaration at the `multiPlate3mfRoundTripPreservesState` declaration block, implementation immediately after that test).
- Loads the committed FIXTURE-01 (`tests/data/test_model.stl`) so the project has real geometry.
- Writes a known non-identity assemble transform (GL space) to object 0: offset (12.5, -7.0, 3.25), rotation (15°, -25°, 45°), scale (1.1, 0.9, 1.0).
- Asserts `isAssembleInitialized(0)` is true after the writes (proves the Plan 01 setters flip `m_assemble_initialized`, gating the `<assemble>` write).
- Saves via `saveProjectAs`/`saveProject` (real `store_3mf`).
- Reloads into a fresh `ProjectServiceMock` via `loadProject` (real reader).
- Asserts the reloaded offset/rotation/scale match within epsilon (offset 1e-4, rotation 1e-3 deg to absorb matrix->euler drift, scale 1e-4) AND that `isAssembleInitialized(0)` is still true.
- `#ifdef HAS_LIBSLIC3R` guarded with `QSKIP` fallback.
- No CMakeLists.txt edit needed — the new slot rides in the existing ViewModelSmokeTests target (AUTOMOC reconfigured by the canonical verify).

## Verification

- Canonical build: **exit 0**, 0 errors (`build_p138_04b.log`).
- ctest ViewModel group: **PASS** — `testAssembleTransformRoundTrip` green, proving the round-trip succeeds end-to-end through the real `bbs_3mf.cpp` `<assemble>` block.
- All 5 ctest groups PASS (PrepareScene/PartPlate/ViewModel/UI/PreviewParser).
- E2E pipeline: PASS.
- App launch liveness: `APP_RUNNING_PID=30584`.

## What this proves

The Qt accessors (Plan 01) feed the upstream `<assemble>` contract correctly:
- The GL<->slic3r Y/Z offset swap and deg<->rad rotation conversion survive the full serialize/deserialize cycle.
- The `m_assemble_initialized` flag flip in the Plan 01 setters causes `<assemble item>` to be written (bbs_3mf.cpp:8076 gate).
- The reloaded transform matches the written transform within numeric tolerance.

This closes ASM-01 success criterion 2.

## Out of scope

- Manual UI smoke (launch app, enter assembly view, drag Move gizmo, observe volume translate) — the one human-gated step, deferred to Phase 140 manual verification gate.
