# Phase 142: OpenVDB CMake Unlock And libslic3r Link - Context

**Gathered:** 2026-07-17
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Correct the v4.x "OpenVDB unavailable" premise at the build layer: invoke find_package(OpenVDB) in root CMakeLists, create the openvdb_libs alias target, and activate OpenVDBUtils.cpp so mesh_to_grid/grid_to_mesh/redistance_grid symbols resolve at exe link.

Requirements: VDB-01, VDB-02

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
Source-truth anchors (from Phase 141 exploration + v5.0 OpenVDB investigation):
- DEPS_PREFIX = E:/ai/3D-Printer/deps/build/OrcaSlicer_dep/usr/local (has lib/libopenvdb.lib 55MB + include/openvdb/ + lib/cmake/OpenVDB/FindOpenVDB.cmake + lib/libblosc.lib)
- Upstream OrcaSlicer/CMakeLists.txt:754-767 wires OpenVDB: set OPENVDB_USE_STATIC_LIBS ON, set USE_BLOSC TRUE, find_package(OpenVDB 5.0 COMPONENTS openvdb), FATAL_ERROR if not found, plus IlmBase::Half + Blosc::blosc remaps
- Qt6 fork cmake/BuildLibslic3rFromSource.cmake:366,718 gates on if(TARGET openvdb_libs) — but no file creates that target
- Root CMakeLists.txt has zero openvdb references
- Approach: add find_package(OpenVDB) call + create openvdb_libs ALIAS to OpenVDB::openvdb (or imported target); apply IlmBase::Half / Blosc::blosc remaps if needed
- HIGH RISK: real link errors possible (target name mismatches, missing Blosc/Half symbols, C++17 vs OpenVDB C++14 ABI). Each build cycle is ~15 min.

</decisions>

<code_context>
## Existing Code Insights

Key files:
- CMakeLists.txt (root) — add find_package(OpenVDB) here
- cmake/BuildLibslic3rFromSource.cmake — existing if(TARGET openvdb_libs) gates will auto-activate once the alias exists
- third_party/OrcaSlicer/CMakeLists.txt:754-767 — upstream reference pattern to copy

</code_context>

<specifics>
## Specific Ideas

No specific requirements — discuss phase skipped. Refer to ROADMAP phase description and success criteria.

</specifics>

<deferred>
## Deferred Ideas

None — discuss phase skipped.

</deferred>
