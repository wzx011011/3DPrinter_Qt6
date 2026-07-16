# Phase 142: OpenVDB CMake Unlock And libslic3r Link — Summary

**Phase:** 142 (v5.0 / WS2)
**Status:** Verified ✓
**Requirements:** VDB-01, VDB-02

## What shipped

The v4.x "OpenVDB unavailable" premise is officially refuted. OpenVDB was built, present, and ready to link in DEPS_PREFIX all along — the v4.x fork's only mistake was an incomplete CMake port. Three small CMake additions fixed it.

### Root cause (corrected)
- v4.x fork dropped the upstream `find_package(OpenVDB)` call entirely.
- v4.x fork renamed the gating target to `openvdb_libs` but no file ever created that target.
- The find module uses `OPENVDB_LIBRARYDIR` (not `CMAKE_PREFIX_PATH`) to locate the binary — v4.x never set it.

### Changes shipped

**CMakeLists.txt (root)** — OpenVDB unlock block after libslic3r-from-source:
- `OPENVDB_USE_STATIC_LIBS ON`, `USE_BLOSC TRUE`
- Module path additions for `FindOpenVDB.cmake` (DEPS_PREFIX copy + OrcaSlicer source vendored copy)
- `OpenVDB_ROOT` / `OPENVDB_ROOT` / `OPENVDB_LIBRARYDIR` force-set to DEPS_PREFIX / DEPS_PREFIX/lib
- `find_package(OpenVDB 5.0 COMPONENTS openvdb)`
- MSVC RelWithDebInfo→Release config remap for OpenVDB::openvdb / IlmBase::Half / Blosc::blosc
- `add_library(openvdb_libs INTERFACE IMPORTED)` shim re-exporting OpenVDB::openvdb
- Late-bind `target_link_libraries(libslic3r_from_source PUBLIC openvdb_libs)`

**cmake/BuildLibslic3rFromSource.cmake** — libnoise latent-issue fix:
- Force-set `LIBNOISE_INCLUDE_DIR` to vcpkg path before `find_package(libnoise)` to prevent the `LIBNOISE_INCLUDE_DIR-NOTFOUND` sentinel from landing in `noise::noise`'s INTERFACE_INCLUDE_DIRECTORIES (a dormant bug exposed by OpenVDB's expanded transitive include propagation).

**tests/QmlUiAuditTests.cpp** — new `v50OpenVdbUnlockWired()` source-audit slot:
- Asserts the find_package call, OPENVDB_LIBRARYDIR, openvdb_libs shim, OpenVDBUtils.cpp in conditional sources, and libnoise FORCE fix.

## Verification

- CMake configure clean (zero errors) after 3 iterations.
- OWzxSlicer.exe links clean (no LNK2019 on `mesh_to_grid`/`grid_to_mesh`/`redistance_grid`).
- 270/270 tests passing (269 prior + new `v50OpenVdbUnlockWired` slot).
- No regression in v4.6/v4.7/v4.8/v5.0-WS1 anchors.

## Lessons

1. **Read the actual find module before assuming CMAKE_PREFIX_PATH works.** Many find modules use bespoke cache vars (OpenVDB uses OPENVDB_LIBRARYDIR); CMAKE_PREFIX_PATH is a hint, not a guarantee.
2. **Order matters with transitive dependencies.** OpenVDB pulls TBB transitively; placing its find_package before libslic3r's own TBB import causes an export-set collision ("Some targets already defined"). After libslic3r is fine.
3. **Latent bugs surface when you expand propagation chains.** The libnoise NOTFOUND sentinel was dormant for years because the dependent-target strict-check wasn't being triggered. Adding any new transitive link can expose similar latent issues — be ready to fix or pin them.
4. **Each ~12-min build is expensive.** Iterating CMake changes that need a full reconfigure is slow; batching multiple plausible fixes per iteration (rather than one-at-a-time) conserves build cycles.

## Unlocks downstream

- Phase 143 (Hollow gizmo): the SLA hollowing path through `OpenVDBUtils::mesh_to_grid` is now linkable.
- Phase 144-146 (Emboss): the EmbossJob async pipeline benefits from the same libslic3r surface-area expansion pattern proved here.
- Future FaceDetector/SlaSupports: both are downstream OpenVDB consumers.
