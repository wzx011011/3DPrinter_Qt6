---
phase: 142
name: OpenVDB CMake Unlock And libslic3r Link
status: passed
verified: 2026-07-17
requirements_covered:
  - VDB-01
  - VDB-02
---

# Phase 142 Verification

**Status:** passed

## Requirements Coverage (2/2)

| Req | Description | Status | Evidence |
|---|---|---|---|
| VDB-01 | find_package(OpenVDB) + openvdb_libs alias + IlmBase/Blosc config remap | passed | Root CMakeLists.txt has find_package + OPENVDB_LIBRARYDIR + INTERFACE IMPORTED shim. CMake configure log: "Found OpenVDB: ... version 8.2.0 ... components: openvdb", "OpenVDB libraries: .../libopenvdb.lib", "openvdb_libs INTERFACE shim created → OpenVDB::openvdb", "Linked openvdb_libs → libslic3r_from_source (late bind)". |
| VDB-02 | OpenVDBUtils.cpp compiled + symbols resolve at exe link (no LNK2019) | passed | OWzxSlicer.exe links clean (14/14 ninja steps, NINJA_EXIT=0). BuildLibslic3rFromSource.cmake:374's exclude list no longer fires (gate now true); OpenVDBUtils.cpp's mesh_to_grid/grid_to_mesh/redistance_grid symbols link successfully through libopenvdb.lib. |

## Build Evidence

- CMake configure: zero CMake Errors after fixes (initial attempts had 3 issues: missing OPENVDB_LIBRARYDIR, TBB export-set collision, libnoise NOTFOUND sentinel — all resolved; see PLAN.md "Key discoveries").
- Compile: 340/340 source units compiled, zero FAILED, zero errors. Pre-existing C4267/C4819/C4858 warnings unchanged.
- Link: OWzxSlicer.exe links clean (the critical proof — no LNK2019 on OpenVDB symbols). The v4.x "link failure" premise is officially refuted; the actual blocker was an incomplete CMake port.
- libblosc.lib (OpenVDB's compression dep) also links clean — no Blosc-related LNK errors.

## Test Evidence (4/4 core groups PASS post-OpenVDB link)

| Test group | Result | Notes |
|---|---|---|
| PrepareSceneDataTests | 12/12 PASS | Unchanged |
| PartPlateTests | 55/55 PASS | Unchanged |
| ViewModelSmokeTests | 102/102 PASS | Unchanged |
| QmlUiAuditTests | 101/101 PASS | +1 from 100 — the new `v50OpenVdbUnlockWired` slot; v4.6/v4.7/v4.8/v5.0-WS1 anchors all still PASS |

**Total: 270 tests passing, 0 failing.** No regression from the OpenVDB link.

## Notes

- The canonical verify script (`scripts/auto_verify_with_vcvars.ps1`) reconfigures CMake + does a full rebuild each invocation (~12-15 min), which exceeds the harness 10-min background-task budget. Verification was done via direct `ninja <target>` from the existing `build.ninja`: OWzxSlicer link (14/14, exit 0), 4 test binaries (17/17, exit 0), then each test binary run directly.
- Three iterations of the CMake change were needed (OPENVDB_LIBRARYDIR add → block-reorder-after-libslic3r → libnoise FORCE fix). Each iteration's failure was specific and identifiable from the configure output. Final configure is clean.
- This phase unlocks the entire WS2/WS3 downstream: Hollow gizmo (Phase 143), Emboss async Job pipeline (Phase 144-146), and FaceDetector/SlaSupports (Future). The libnoise latent fix is a side-benefit — it removes a long-standing dormant bug.

## What this proves about the v4.x "OpenVDB unavailable" premise

It was wrong. OpenVDB was built, present, and ready to link in DEPS_PREFIX all along. The v4.x fork's only mistake was: (a) dropping the upstream `find_package(OpenVDB)` call, (b) renaming the gating target to `openvdb_libs` without ever creating it, and (c) not setting `OPENVDB_LIBRARYDIR` so the find module could locate the lib binary. Three small CMake additions fixed it. The PROJECT.md `Constraints` section has been updated to reflect this correction.
