# Phase 137: MeshBoolean + Drill Activation - Context

**Gathered:** 2026-07-16
**Status:** Complete (backfilled from git evidence)
**Mode:** Reconciliation — phase was implemented directly without GSD discuss/plan/execute.

<domain>
## Phase Boundary

Activate the ~200 lines of already-written MeshBoolean + Drill logic by flipping `kCgalMeshBooleanAvailable` to true and replacing the stub call sites with real `MeshBoolean::minus` / `self_union` invocations. Depends on Phase 136 (MeshBoolean now compiles/links).

</domain>

<decisions>
## Implementation Decisions

### Flag flip
- Set `kCgalMeshBooleanAvailable = true` (was false).

### Stub removal
- Delete `MeshBoolean_mcut_stub.cpp` — its return-false stub clashed (LNK2005 duplicate symbol) with the now-compiled real `MeshBoolean.cpp`. The real implementation is the single source of truth.

### Call-site wiring
- `ProjectServiceMock::booleanObject` (union/subtract/intersect) — replace the stub branch with real `MeshBoolean::minus` / `self_union` calls.
- `ProjectServiceMock::drillObject` — replace the stub with real `MeshBoolean::minus` (drill = subtract tool mesh from target).

### Status-text + test alignment
- `EditorViewModel::gizmoStatusText` no longer reports "CGAL unavailable" for cases 11 (MeshBoolean) and 13 (Drill) now that the gizmo is available.
- `tests/ViewModelSmokeTests.cpp` assertions updated to reflect the activated path.

### Claude's Discretion
None — phase already implemented.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/core/services/ProjectServiceMock.cpp` — `booleanObject` / `drillObject` Q_INVOKABLE entry points.
- `src/core/viewmodels/EditorViewModel.cpp` — `gizmoStatusText` gizmo availability messaging.
- `third_party/OrcaSlicer/src/libslic3r/MeshBoolean.{cpp,hpp}` — the real boolean engine (Phase 136 made it compile).
- `tests/ViewModelSmokeTests.cpp` — gizmo-availability smoke assertions.

### Established Patterns
- Gizmo availability is gated by a `k*Available` constexpr; status text + tests both branch on it.

### Integration Points
- EditorViewModel gizmo case 11 (MeshBoolean) and case 13 (Drill) now resolve to the real path.

</code_context>

<specifics>
## Specific Ideas

Phase 136 + 137 together close CGAL-02 and CGAL-03. The "~200 lines already written" framing from the v4.7/v4.8 roadmap refers to the MeshBoolean engine itself — Phase 137 only flips the flag and rewires 2 call sites.

</specifics>

<deferred>
## Deferred Ideas

None for this phase. OpenVDB-dependent hollow/support-paint gizmos remain deferred (separate dependency blocker, out of v4.8 scope).

</deferred>
