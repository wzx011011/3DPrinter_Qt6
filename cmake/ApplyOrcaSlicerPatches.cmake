# ApplyOrcaSlicerPatches.cmake
#
# The third_party/OrcaSlicer submodule is kept as a PRISTINE upstream
# checkout (gitlink pinned to the upstream baseline commit) so it can be
# advanced/synced with upstream without carrying local edits. Every OWzx
# compatibility change lives instead in this repository under
# patches/orcaslicer/*.patch as an ordered series, applied onto the submodule
# working tree at CONFIGURE time, before the from-source libslic3r build globs
# its sources.
#
# Applied-state detection (C2, 2026-11-03):
#   After a successful apply, a stamp in the BUILD tree records the series
#   fingerprint (per-file hashes, in order) and the SHA-256 of the resulting
#   `git diff HEAD` of the submodule. A configure run treats the submodule as
#   already patched only when both match exactly, i.e. the tracked tree holds
#   precisely the patched content. (The old stamp lived inside the submodule
#   as an untracked file, which made `git status --porcelain` permanently
#   non-empty: a `git checkout .` in the submodule then built libslic3r
#   UNPATCHED with no error.)
#
#   Otherwise the tree is brought back to a known state: if every modified
#   tracked file is one the series touches (partial apply, edited or appended
#   series), those files are restored from HEAD and the whole series is
#   re-applied in order. Modifications to any other file are foreign edits;
#   configure then fails hard rather than discard them or build libslic3r from
#   a tree of unknown provenance.
#
# A fresh clone therefore self-heals: `git submodule update` checks out the
# pristine baseline, the next configure re-applies the whole series.

set(OWZX_ORCASLICER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/OrcaSlicer")
set(OWZX_ORCASLICER_PATCH_DIR "${CMAKE_CURRENT_SOURCE_DIR}/patches/orcaslicer")
set(OWZX_PATCH_STAMP "${CMAKE_BINARY_DIR}/owzx-orcaslicer-patches.stamp")
set(OWZX_LEGACY_PATCH_STAMP "${OWZX_ORCASLICER_DIR}/.owzx-patches-applied")

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

# Editing a patch in place must re-run configure (C2b).
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${OWZX_ORCASLICER_PATCHES})

# Resolve git once up front: execute_process resolves commands through PATH at
# spawn time, and a configure environment without git on PATH would otherwise
# surface as a misleading "patch does not apply" failure.
find_program(OWZX_GIT_EXECUTABLE git)
if(NOT OWZX_GIT_EXECUTABLE)
    message(FATAL_ERROR
        "[OrcaSlicerPatches] git not found on PATH; it is required to apply "
        "the OrcaSlicer compatibility series at configure time")
endif()

# The pre-2026-11-03 stamp lived inside the submodule; remove it so the
# submodule status is honest again.
if(EXISTS "${OWZX_LEGACY_PATCH_STAMP}")
    file(REMOVE "${OWZX_LEGACY_PATCH_STAMP}")
endif()

# Fingerprint of the current series (file order is part of the identity).
set(OWZX_SERIES_FINGERPRINT "")
set(OWZX_SERIES_FILES "")
foreach(patch_file IN LISTS OWZX_ORCASLICER_PATCHES)
    file(SHA256 "${patch_file}" patch_file_hash)
    string(APPEND OWZX_SERIES_FINGERPRINT "${patch_file_hash};")
    # Files the series touches (the "+++ b/<path>" headers).
    file(STRINGS "${patch_file}" patch_targets REGEX "^\\+\\+\\+ b/")
    foreach(target_line IN LISTS patch_targets)
        string(REGEX REPLACE "^\\+\\+\\+ b/" "" target_path "${target_line}")
        list(APPEND OWZX_SERIES_FILES "${target_path}")
    endforeach()
endforeach()
list(REMOVE_DUPLICATES OWZX_SERIES_FILES)
string(SHA256 OWZX_SERIES_HASH "${OWZX_SERIES_FINGERPRINT}")

# SHA-256 of the submodule's tracked changes relative to HEAD.
function(owzx_submodule_diff_hash out_var)
    execute_process(
        COMMAND "${OWZX_GIT_EXECUTABLE}" diff --binary HEAD
        WORKING_DIRECTORY "${OWZX_ORCASLICER_DIR}"
        OUTPUT_VARIABLE diff_output
        RESULT_VARIABLE diff_result)
    if(NOT diff_result EQUAL 0)
        message(FATAL_ERROR
            "[OrcaSlicerPatches] git diff failed in ${OWZX_ORCASLICER_DIR}")
    endif()
    string(SHA256 diff_hash "${diff_output}")
    set(${out_var} "${diff_hash}" PARENT_SCOPE)
endfunction()

owzx_submodule_diff_hash(OWZX_CURRENT_DIFF_HASH)

# Already-applied check: the stamp must match both the current series and
# the exact content of the tracked tree.
if(EXISTS "${OWZX_PATCH_STAMP}")
    file(STRINGS "${OWZX_PATCH_STAMP}" stamp_lines)
    list(LENGTH stamp_lines stamp_line_count)
    if(stamp_line_count EQUAL 2)
        list(GET stamp_lines 0 stamped_series)
        list(GET stamp_lines 1 stamped_diff)
        if(stamped_series STREQUAL OWZX_SERIES_HASH
                AND stamped_diff STREQUAL OWZX_CURRENT_DIFF_HASH)
            message(STATUS
                "[OrcaSlicerPatches] series already applied (fingerprint "
                "${OWZX_SERIES_HASH}); skipping")
            return()
        endif()
    endif()
endif()

# Not in the recorded patched state. Restore series-touched files to HEAD;
# refuse to touch anything else.
execute_process(
    COMMAND "${OWZX_GIT_EXECUTABLE}" diff --name-only HEAD
    WORKING_DIRECTORY "${OWZX_ORCASLICER_DIR}"
    OUTPUT_VARIABLE dirty_output
    RESULT_VARIABLE dirty_result
    OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT dirty_result EQUAL 0)
    message(FATAL_ERROR
        "[OrcaSlicerPatches] git diff --name-only failed in ${OWZX_ORCASLICER_DIR}")
endif()
set(OWZX_FOREIGN_EDITS "")
set(OWZX_RESTORE_FILES "")
if(NOT dirty_output STREQUAL "")
    string(REPLACE "\n" ";" dirty_files "${dirty_output}")
    foreach(dirty_file IN LISTS dirty_files)
        list(FIND OWZX_SERIES_FILES "${dirty_file}" series_index)
        if(series_index EQUAL -1)
            list(APPEND OWZX_FOREIGN_EDITS "${dirty_file}")
        else()
            list(APPEND OWZX_RESTORE_FILES "${dirty_file}")
        endif()
    endforeach()
endif()
if(OWZX_FOREIGN_EDITS)
    string(REPLACE ";" "\n  " foreign_list "${OWZX_FOREIGN_EDITS}")
    message(FATAL_ERROR
        "[OrcaSlicerPatches] third_party/OrcaSlicer has edits to files the "
        "patch series does not touch:\n  ${foreign_list}\n"
        "The submodule must stay pristine; move the change into "
        "patches/orcaslicer/ or revert it, then re-run configure.")
endif()
if(OWZX_RESTORE_FILES)
    message(STATUS
        "[OrcaSlicerPatches] submodule is not in the recorded patched state; "
        "restoring ${OWZX_RESTORE_FILES} from HEAD and re-applying the series")
    execute_process(
        COMMAND "${OWZX_GIT_EXECUTABLE}" checkout HEAD -- ${OWZX_RESTORE_FILES}
        WORKING_DIRECTORY "${OWZX_ORCASLICER_DIR}"
        RESULT_VARIABLE restore_result)
    if(NOT restore_result EQUAL 0)
        message(FATAL_ERROR
            "[OrcaSlicerPatches] failed to restore series files from HEAD")
    endif()
endif()

# Check-and-apply each patch IN ORDER. Later patches rework files touched by
# earlier ones, so a patch must be applied before the next one is checked. A
# mid-series failure leaves a partially patched tree; the next configure
# restores the series files and retries.
file(REMOVE "${OWZX_PATCH_STAMP}")
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
            "[OrcaSlicerPatches] ${patch_name} does not apply to the pristine "
            "submodule checkout. Either the submodule pin does not match the "
            "baseline the series was written against, or the patch itself is "
            "stale; regenerate it against third_party/OrcaSlicer HEAD.")
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
            "apply; re-run configure to restore and retry")
    endif()
endforeach()

owzx_submodule_diff_hash(OWZX_APPLIED_DIFF_HASH)
file(WRITE "${OWZX_PATCH_STAMP}" "${OWZX_SERIES_HASH}\n${OWZX_APPLIED_DIFF_HASH}\n")
message(STATUS
    "[OrcaSlicerPatches] applied ${OWZX_ORCASLICER_PATCHES} onto the "
    "pristine submodule checkout (fingerprint ${OWZX_SERIES_HASH})")
