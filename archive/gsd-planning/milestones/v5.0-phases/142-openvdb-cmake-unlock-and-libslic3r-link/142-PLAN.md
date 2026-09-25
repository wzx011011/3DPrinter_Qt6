# Phase 142: OpenVDB CMake Unlock And libslic3r Link — Plan

**Phase:** 142 (v5.0 / WS2)
**Requirements:** VDB-01, VDB-02
**Mode:** Auto (skip_discuss=true)

## Goal

Correct the v4.x "OpenVDB unavailable" premise at the build layer: wire find_package(OpenVDB), create the openvdb_libs alias, activate OpenVDBUtils.cpp so mesh_to_grid/grid_to_mesh/redistance_grid symbols resolve at exe link.

## Plans

### 142-01: Wire find_package(OpenVDB) + create openvdb_libs alias (VDB-01)

Root `CMakeLists.txt` — insert an OpenVDB-unlock block AFTER the libslic3r-from-source include (avoids TBB export-set collision):
- `set(OPENVDB_USE_STATIC_LIBS ON)` + `set(USE_BLOSC TRUE)`
- Add `${DEPS_PREFIX}/lib/cmake/OpenVDB` + OrcaSlicer's cmake/modules to CMAKE_MODULE_PATH
- Force-set `OpenVDB_ROOT` / `OPENVDB_ROOT` / `OPENVDB_LIBRARYDIR` to `${DEPS_PREFIX}` / `${DEPS_PREFIX}/lib` (the upstream find module does NOT use CMAKE_PREFIX_PATH for the lib search)
- `find_package(OpenVDB 5.0 COMPONENTS openvdb)`
- MSVC config remap for `OpenVDB::openvdb` / `IlmBase::Half` / `Blosc::blosc` (RelWithDebInfo → Release)
- Create `add_library(openvdb_libs INTERFACE IMPORTED)` shim re-exporting `OpenVDB::openvdb`
- Late-bind: `target_link_libraries(libslic3r_from_source PUBLIC openvdb_libs)` (the existing gate at BuildLibslic3rFromSource.cmake:718 evaluated false before this block)

### 142-02: Fix libnoise latent-issue (VDB-02 enabler)

`cmake/BuildLibslic3rFromSource.cmake` — the upstream `Findlibnoise.cmake` module leaves `LIBNOISE_INCLUDE_DIR-NOTFOUND` in `noise::noise`'s INTERFACE_INCLUDE_DIRECTORIES when its `find_path` fails. This was latent for years (didn't propagate to dependent targets' strict checks) but OpenVDB's expanded transitive INTERFACE_INCLUDE_DIRECTORIES propagation exposed it. Fix: force-set `LIBNOISE_INCLUDE_DIR` to `${VCPKG_INSTALLED_DIR}/include` before `find_package(libnoise)`.

### 142-03: Regression lock

`tests/QmlUiAuditTests.cpp` — new `v50OpenVdbUnlockWired()` slot asserting:
- Root CMakeLists has `find_package(OpenVDB 5.0 COMPONENTS openvdb)`
- Root CMakeLists sets `OPENVDB_LIBRARYDIR`
- Root CMakeLists creates `add_library(openvdb_libs INTERFACE IMPORTED)` + `INTERFACE_LINK_LIBRARIES "OpenVDB::openvdb"`
- BuildLibslic3rFromSource.cmake has the `if(TARGET openvdb_libs)` gate + `OpenVDBUtils.cpp` in the conditional list
- libnoise fix documents the NOTFOUND sentinel risk + uses FORCE

## Verification

- OWzxSlicer.exe links clean (no LNK2019 on OpenVDB symbols — the critical proof)
- 270/270 tests passing (269 prior + new `v50OpenVdbUnlockWired` slot)
- v4.6/v4.7/v4.8/v5.0-WS1 anchors all still PASS (no regression)

## Key discoveries during execution

1. **FindOpenVDB needs OPENVDB_LIBRARYDIR, not CMAKE_PREFIX_PATH.** First attempt set CMAKE_PREFIX_PATH (already done in v4.x fork for other deps) — find module located the headers but not libopenvdb.lib. Explicit `OPENVDB_LIBRARYDIR=${DEPS_PREFIX}/lib` was required.

2. **TBB export-set collision.** First attempt placed the OpenVDB block BEFORE libslic3r-from-source. This caused `TBBTargets.cmake:42` to fail ("Some targets already defined: TBB::tbb; not yet defined: TBB::tbbmalloc") because OpenVDB's transitive TBB dependency re-imported TBB. Moving the block AFTER libslic3r-from-source (which already imports TBB fully) resolved it. This also required late-binding the `target_link_libraries(libslic3r_from_source PUBLIC openvdb_libs)` call (the existing gate at BuildLibslic3rFromSource.cmake:718 ran before this block).

3. **libnoise latent issue exposed.** Adding OpenVDB expanded the transitive INTERFACE_INCLUDE_DIRECTORIES chain enough to trip CMake's strict relative/non-existent path check on the `LIBNOISE_INCLUDE_DIR-NOTFOUND` sentinel that had been sitting in `noise::noise` for years. Force-setting `LIBNOISE_INCLUDE_DIR` before find_package fixes it.

4. **OpenVDB version is 8.2.0, not 5.x.** Upstream OrcaSlicer requires `>= 5.0`; the actual DEPS_PREFIX build is 8.2.0 (ABI 8). No compatibility issues observed.
