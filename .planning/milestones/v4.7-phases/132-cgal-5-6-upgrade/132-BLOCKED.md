# Phase 132: CGAL 5.6+ Upgrade — BLOCKED

**Status:** Blocked by dependency (environment-level upgrade required, not code-solvable).

**Blocker:** CGAL 5.4 (current: 5.4-I-900 in DEPS_PREFIX) lacks
`Polygon_mesh_processing/corefine_and_constructions.h` — the corefine_and_compute_union/
intersection API required for MeshBoolean. Upgrading to 5.6+ requires replacing the
entire CGAL header/library set in `E:/ai/3D-Printer/deps/build/OrcaSlicer_dep/usr/local/`,
which is an environment dependency-management operation (rebuild or download new
pre-built bundle), not a code change.

**Deferred to:** A future milestone where the dependency bundle is rebuilt with CGAL 5.6+.
The ~200 lines of already-written MeshBoolean + Drill logic will activate once the flag
`kCgalMeshBooleanAvailable` is flipped (CGAL-02/03).
