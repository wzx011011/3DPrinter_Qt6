# ApplyOrcaSlicerPatches.cmake
#
# The third_party/OrcaSlicer submodule is kept as a PRISTINE upstream
# checkout (gitlink pinned to the upstream baseline commit) so it can be
# advanced/synced with upstream without carrying local edits. Every OWzx
# compatibility change lives instead in this repository under
# patches/orcaslicer/*.patch as an ordered series (format-patch exports of
# the former owzx-cgal54-on-0632 compat branch, oldest first), applied onto
# the submodule working tree at CONFIGURE time, before the from-source
# libslic3r build globs its sources.
#
# Applied-state detection (robust against series evolution):
#   A SHA-256 fingerprint of the whole series (per-file hashes, in order) is
#   stamped into third_party/OrcaSlicer/.owzx-patches-applied after a
#   successful apply. A configure run treats the submodule as already patched
#   only when the stamp matches the CURRENT series fingerprint AND the
#   working tree actually carries modifications relative to its HEAD (a bare
#   `git submodule update` resets the tree but not the untracked stamp, so
#   the dirty-tree check is what makes the stamp trustworthy).
#   Otherwise each patch is forward-checked and applied IN ORDER (later
#   patches rework files created by earlier ones, and a single multi-file
#   `git apply` invocation does not compose those intermediate states); if
#   the tree is in any other state (wrong baseline, partial series, series
#   changed under a patched tree) configure fails hard rather than building
#   libslic3r from a tree of unknown provenance.
#
# A fresh clone therefore self-heals: `git submodule update` checks out the
# pristine baseline, the next configure re-applies the whole series.

set(OWZX_ORCASLICER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/OrcaSlicer")
set(OWZX_ORCASLICER_PATCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/patches/orcaslicer")
set(OWZX_PATCH_STAMP "${OWZX_ORCASLICER_DIR}/.owzx-patches-applied")

if(NOT EXISTS "${OWZX_ORCASLICER_DIR}/.git")
    message(STATUS
        "[OrcaSlicerPatches] submodule checkout not found; nothing to patch")
    return()
endif()

file(GLOB OWZX_ORCASLICER_PATCHES "${OWZX_ORCASLICER_PATCH_DIR}/*.patch")
list(SORT OWZX_ORCASLICER_PATCHES)

if(NOT OWZX_ORCASLICER_PATCHES)
    message(STATUS
        "[OrcaSlicerPatches] no patches in ${OWZX_ORCASLICER_PATCH_DIR}")
    return()
endif()

# Resolve git once up front: execute_process resolves commands through PATH at
# spawn time, and a configure environment without git on PATH would otherwise
# surface as a misleading "patch does not apply" failure.
find_program(OWZX_GIT_EXECUTABLE git)
if(NOT OWZX_GIT_EXECUTABLE)
    message(FATAL_ERROR
        "[OrcaSlicerPatches] git not found on PATH; it is required to apply "
        "the OrcaSlicer compatibility series at configure time")
endif()

# Fingerprint of the current series (file order is part of the identity).
set(OWZX_SERIES_FINGERPRINT "")
foreach(patch_file IN LISTS OWZX_ORCASLICER_PATCHES)
    file(SHA256 "${patch_file}" patch_file_hash)
    string(APPEND OWZX_SERIES_FINGERPRINT "${patch_file_hash};")
endforeach()
string(SHA256 OWZX_SERIES_HASH "${OWZX_SERIES_FINGERPRINT}")

# Already-applied check: stamp matches the current series AND the working
# tree is actually modified relative to the submodule HEAD.
set(OWZX_SERIES_ALREADY_APPLIED FALSE)
if(EXISTS "${OWZX_PATCH_STAMP}")
    file(READ "${OWZX_PATCH_STAMP}" stamped_hash)
    string(STRIP "${stamped_hash}" stamped_hash)
    if(stamped_hash STREQUAL OWZX_SERIES_HASH)
        execute_process(
            COMMAND "${OWZX_GIT_EXECUTABLE}" status --porcelain
            WORKING_DIRECTORY "${OWZX_ORCASLICER_DIR}"
            OUTPUT_VARIABLE submodule_status_output
            RESULT_VARIABLE submodule_status_result)
        if(submodule_status_result EQUAL 0
                AND NOT submodule_status_output STREQUAL "")
            set(OWZX_SERIES_ALREADY_APPLIED TRUE)
        endif()
    endif()
endif()

if(OWZX_SERIES_ALREADY_APPLIED)
    message(STATUS
        "[OrcaSlicerPatches] series already applied (fingerprint "
        "${OWZX_SERIES_HASH}); skipping")
    return()
endif()

# Check-and-apply each patch IN ORDER. Later patches rework files created by
# earlier ones, so a patch must be applied before the next one is checked --
# a "check the whole series first, apply afterwards" pass would judge every
# post-creation patch against a tree missing the created files. A mid-series
# failure therefore leaves a partially patched tree; the fatal message
# directs the reset that restores a known state (the stamp is only written
# after the whole series lands).
foreach(patch_file IN LISTS OWZX_ORCASLICER_PATCHES)
    get_filename_component(patch_name "${patch_file}" NAME)

    execute_process(
        COMMAND "${OWZX_GIT_EXECUTABLE}" apply --check "${patch_file}"
        WORKING_DIRECTORY "${OWZX_ORCASLICER_DIR}"
        RESULT_VARIABLE patch_check_result
        OUTPUT_QUIET
        ERROR_QUIET)
    if(NOT patch_check_result MATCHES "^[0-9]+$")
        message(FATAL_ERROR
            "[OrcaSlicerPatches] failed to launch git (${patch_check_result})")
    endif()
    if(NOT patch_check_result EQUAL 0)
        message(FATAL_ERROR
            "[OrcaSlicerPatches] ${patch_name} does not apply to the "
            "submodule checkout and no matching applied-state stamp exists. "
            "The submodule is out of sync with patches/orcaslicer (wrong "
            "baseline commit, partially applied series, or the series "
            "changed under a patched tree). Reset third_party/OrcaSlicer to "
            "the pinned baseline commit, drop untracked files created by the "
            "series, and re-run configure.")
    endif()

    execute_process(
        COMMAND "${OWZX_GIT_EXECUTABLE}" apply "${patch_file}"
        WORKING_DIRECTORY "${OWZX_ORCASLICER_DIR}"
        RESULT_VARIABLE patch_apply_result
        OUTPUT_QUIET
        ERROR_QUIET)
    if(NOT patch_apply_result MATCHES "^[0-9]+$")
        message(FATAL_ERROR
            "[OrcaSlicerPatches] failed to launch git (${patch_apply_result})")
    endif()
    if(NOT patch_apply_result EQUAL 0)
        message(FATAL_ERROR
            "[OrcaSlicerPatches] ${patch_name} passed --check but failed to "
            "apply; reset the submodule working tree and re-run configure")
    endif()
endforeach()

file(WRITE "${OWZX_PATCH_STAMP}" "${OWZX_SERIES_HASH}\n")
message(STATUS
    "[OrcaSlicerPatches] applied ${OWZX_ORCASLICER_PATCHES} onto the "
    "pristine submodule checkout (fingerprint ${OWZX_SERIES_HASH})")
