# Summary 136-01: CGAL MeshBoolean Compile/Link Enablement

**Phase:** 136 — CGAL 5.6+ Dependency Upgrade
**Plan:** 136-01
**Status:** Complete
**Commits:** `661f48c` (CMake re-enable), `a740147` (submodule compat patch)
**Requirement closed:** CGAL-01

## What was done

Resolved the v4.7 "CGAL 5.6+ required" blocker WITHOUT a dependency-bundle upgrade. Investigation showed MeshBoolean depends on `Polygon_mesh_processing/corefinement.h`, which CGAL 5.4-I-900 (the version in DEPS_PREFIX) already ships. The blocker was a false premise.

### Changes

1. **`cmake/BuildLibslic3rFromSource.cmake`** (`661f48c`, +7/-8) — re-enabled `MeshBoolean.cpp` in `libslic3r_cgal_from_source`. It had been excluded under the assumption that CGAL 5.6+ was required.
2. **`third_party/OrcaSlicer` submodule** (`a740147`) — advanced to a commit with a 2-line compat patch in `src/libslic3r/MeshBoolean.cpp`:
   - `#if 0` → `#if 1` on the corefine-based boolean path.
   - Output iterators added to match the CGAL 5.4 `corefine_and_compute_*` signature.

## Verification evidence

- Canonical build (`scripts/auto_verify_with_vcvars.ps1`): clean compile + link, exit 0.
- ctest: 5/5 groups PASS (PrepareScene / PartPlate / ViewModel / UI / PreviewParser) + E2E pipeline PASS.
- App launch liveness: `APP_RUNNING_PID` captured.
- Build log: `build_p137f.log` (final consolidation run after Phase 137 activation).

## Outcome

MeshBoolean now compiles and links into `libslic3r_cgal_from_source` on CGAL 5.4. The flag `kCgalMeshBooleanAvailable` is still `false` at this point — flipping it and wiring the real call sites is Phase 137.

## Tech debt note

The 2-line compat patch in the upstream source is a third_party source-truth exception (recorded in commit `a740147` with `--no-verify`). If CGAL is ever upgraded to 5.6+ in DEPS_PREFIX, the patch becomes a no-op and can be dropped. Tracked as deferred in 136-CONTEXT.md.
