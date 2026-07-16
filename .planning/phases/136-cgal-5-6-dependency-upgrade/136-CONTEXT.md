# Phase 136: CGAL 5.6+ Dependency Upgrade - Context

**Gathered:** 2026-07-16
**Status:** Complete (backfilled from git evidence)
**Mode:** Reconciliation — phase was implemented directly without GSD discuss/plan/execute.

<domain>
## Phase Boundary

Make CGAL MeshBoolean compile and link in the canonical build, unlocking the ~200 lines of already-written MeshBoolean + Drill logic gated behind `kCgalMeshBooleanAvailable`.

The original v4.7 framing was "upgrade CGAL 5.4 → 5.6+ in DEPS_PREFIX". The actual resolution discovered during implementation did NOT require a dependency-bundle upgrade: CGAL 5.4 already ships the `Polygon_mesh_processing/corefinement.h` API that MeshBoolean needs. A 2-line compat patch in the upstream source + CMake re-enable was sufficient.

</domain>

<decisions>
## Implementation Decisions

### Resolution path (compat patch, not bundle upgrade)
- Do NOT rebuild/replace the CGAL headers in `E:/ai/3D-Printer/deps/build/OrcaSlicer_dep/usr/local/`. CGAL 5.4-I-900 is sufficient for MeshBoolean.
- Apply a minimal compat patch in `third_party/OrcaSlicer/src/libslic3r/MeshBoolean.cpp`:
  - Change `#if 0` → `#if 1` on the corefine-based code path (the API exists in 5.4).
  - Add output iterators required by the 5.4 signature.
- Re-enable `MeshBoolean.cpp` in `cmake/BuildLibslic3rFromSource.cmake` (it had been excluded under the false "needs 5.6+" assumption).

### Submodule patch recording
- The compat patch lives in the `third_party/OrcaSlicer` submodule, advanced to a commit containing the 2-line change. Documented as a third_party source-truth compat exception (design rule allowance).

### Claude's Discretion
None — phase already implemented.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `cmake/BuildLibslic3rFromSource.cmake` — controls which libslic3r sources compile into `libslic3r_cgal_from_source`.
- `third_party/OrcaSlicer/src/libslic3r/MeshBoolean.cpp` / `.hpp` — the already-written boolean logic, gated by `kCgalMeshBooleanAvailable`.

### Established Patterns
- Submodule advanced to a patched commit; commit message records the compat exception and `--no-verify` rationale (third_party source patch).

### Integration Points
- `libslic3r_cgal_from_source` target now includes `MeshBoolean.cpp`; consumers (`ProjectServiceMock::booleanObject`/`drillObject`) link against it.

</code_context>

<specifics>
## Specific Ideas

CGAL-01 is satisfied by the compat-patch path. A true 5.6+ bundle upgrade remains a deferred nice-to-have (would remove the patch) but is no longer a blocker for MeshBoolean/Drill.

</specifics>

<deferred>
## Deferred Ideas

- Full CGAL 5.6+ dependency-bundle rebuild (would let us drop the 2-line compat patch). Tracked as optional future work; not required for any current requirement.

</deferred>
