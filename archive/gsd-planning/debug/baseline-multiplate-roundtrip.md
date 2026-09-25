---
status: resolved
trigger: "Latest OrcaSlicer baseline causes ViewModelSmokeTests::multiPlateFullStateRoundTrip to expect print sequence 2 but receive 0."
created: 2026-07-22
updated: 2026-07-22
---

# Baseline MultiPlate RoundTrip

## Symptoms

- Expected: the full ViewModel smoke suite remains green after the source-truth baseline update.
- Actual: canonical verification configures and builds successfully, but `ViewModelSmokeTests` reports 104/105 passing; `multiPlateFullStateRoundTrip` expected `2` and received `0`.
- Errors: no compiler or linker error; application smoke/E2E and QML gates were not reached because the test failed.
- Timeline: failure appeared after moving the submodule from `edbca0aa55` to official upstream `8b93cc5d` while retaining only the reviewed Config and CGAL compatibility deltas.
- Reproduction: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` with the baseline child commit `4cb3b9ce792f15c38fabfb4bb9700895d32b1166`.

## Current Focus

- hypothesis: `PartPlate::m_print_sequence` stores the Plate Settings dialog choice (`0 = global`, `1 = by layer`, `2 = by object`), but the 3MF adapter treated that choice as a `Slic3r::PrintSequence` enum (`0 = by layer`, `1 = by object`, `2 = global`). This inversion caused the round-trip loss.
- test: run the encoding guard only for `ProjectServiceMock.cpp` and this debug-session file.
- expecting: both changed files are UTF-8 without BOM; any reported mojibake must be pre-existing in untouched text and be recorded rather than rewritten.
- next_action: run the broader canonical verification separately from this focused debugging session.

reasoning_checkpoint:
  hypothesis: "The three-value Qt plate-settings choice is persisted as if it were the differently ordered Slic3r PrintSequence enum, so choice 2 becomes the non-serializable ByDefault sentinel and reloads as 0."
  confirming_evidence:
    - "PreparePage forwards choice 0/1/2 directly for Follow global/By layer/By object."
    - "Upstream Plater.cpp converts those choices with 0 -> ByDefault and nonzero choice - 1 -> PrintSequence."
    - "buildPlateDataList writes PartPlate::printSequence() directly to the upstream print_sequence option, while bbs_3mf writes names only for ByLayer and ByObject."
    - "The focused regression reproduces with exit code 1 and the known assertion is choice 2 reloading as 0."
  falsification_test: "After the conversion, the saved archive must reload choice 2 for By object; a reload value of 0 or 1 would disprove either the write or read mapping."
  fix_rationale: "Translation belongs at the persistence adapter because PartPlate and QML deliberately share the upstream dialog-choice representation, while the 3MF serializer consumes Slic3r enum values."
  blind_spots: "SliceService currently applies only PartPlate::config, not m_print_sequence; that separate existing behavior is outside this serialization regression and is not changed here."

## Evidence

- timestamp: 2026-07-22; canonical build passed and full ViewModel smoke failed only at `multiPlateFullStateRoundTrip`, 104/105.
- timestamp: 2026-07-22; child submodule diff against `8b93cc5d` is exactly `Config.hpp` and `MeshBoolean.cpp`.
- timestamp: 2026-07-22; current upstream `PrintConfig.hpp` defines `PrintSequence` as `ByLayer = 0`, `ByObject = 1`, `ByDefault = 2`; this contradicts the test comment and raw test input that call value `2` `ByObject`.
- timestamp: 2026-07-22; upstream `bbs_3mf.cpp` writes `print_sequence` only when its enum-name table contains the stored integral value and reads missing metadata as `PrintSequence::ByLayer` (`0`).
- timestamp: 2026-07-22; `ProjectServiceMock::setPlatePrintSequence` and `PartPlate::setPrintSequence` preserve the supplied raw integer, and `buildPlateDataList` writes that integer to the upstream `print_sequence` enum option without validation or remapping.
- timestamp: 2026-07-22; focused `build\\ViewModelSmokeTests.exe multiPlateFullStateRoundTrip` exited with code 1, reproducing the failing focused gate; its Qt test runner emitted no diagnostic text in this execution channel.
- timestamp: 2026-07-22; `PreparePage.qml` has choices `[Follow global, By layer, By object]` and forwards the combo index unchanged, while upstream `Plater.cpp:18992-19039` maps those choice indexes as `0 -> PrintSequence::ByDefault` and nonzero `choice - 1 -> PrintSequence`; the local code lacks this required conversion.
- timestamp: 2026-07-22; SliceService merges only `PartPlate::config` into a slice config and does not read `PartPlate::m_print_sequence`, so the confirmed regression boundary is 3MF save/load rather than a second raw-sequence path in the slicer.
- timestamp: 2026-07-22; added source-truth conversions in ProjectServiceMock: UI choices `1` and `2` write upstream `ByLayer` and `ByObject`; choice `0` removes the 3MF override; both real-3MF load paths reverse explicit upstream enum values to the UI choices.
- timestamp: 2026-07-22; the first focused target build invoked Ninja but failed before compiling the change because the current PowerShell process lacks the MSVC standard-library include environment (`fatal error C1083: type_traits`).
- timestamp: 2026-07-22; after loading vcvars64.bat, `cmake --build build --target ViewModelSmokeTests` completed successfully and linked the focused test executable.
- timestamp: 2026-07-22; `build\\ViewModelSmokeTests.exe multiPlateFullStateRoundTrip -v1` exited 0 after the conversion fix.
- timestamp: 2026-07-22; `build\\ViewModelSmokeTests.exe multiPlate3mfRoundTripPreservesState -v1` exited 0 after the conversion fix.
- timestamp: 2026-07-22; `git diff --check` passed for the changed source and debug-session files, and the focused source diff contains only the UI-choice/upstream-enum conversion boundary.
- timestamp: 2026-07-22; `encoding_guard.py src/core/services/ProjectServiceMock.cpp .planning/debug/baseline-multiplate-roundtrip.md` passed with `encoding_guard ok`.

## Eliminated

- hypothesis: the current upstream baseline changed `PrintSequence::ByObject` from integer `2` to integer `1`.
  evidence: the pre-update baseline `edbca0aa55` and the current baseline both define `ByLayer`, `ByObject`, `ByDefault`, `Count` in that order, so their underlying enum values are identical.
  timestamp: 2026-07-22

## Resolution

root_cause: ProjectServiceMock conflated the Plate Settings dialog indexes with the differently ordered upstream PrintSequence enum while serializing and restoring 3MF plate metadata.
fix: Added explicit UI-choice to upstream-enum conversion at the 3MF write boundary, omitted the inherited choice, and converted explicit 3MF enum values back to UI choices in both real-3MF load paths.
verification: focused target build, multiPlateFullStateRoundTrip, multiPlate3mfRoundTripPreservesState, focused diff check, and encoding guard passed. No broad canonical verification was run.
files_changed:
  - src/core/services/ProjectServiceMock.cpp

## Postscript: 2026-07-25 stale-binary false alarm

A 2026-07-24 canonical verification run reported `multiPlateFullStateRoundTrip`
failing again (`platePrintSequence(2)` returned 0). Investigation showed this
was **not a regression** — the `build/ViewModelSmokeTests.exe` binary was stale
(mtime 2026-07-23 02:11, built from HEAD `a67a4c4`), predating the fix commit
`fcfeb16` (2026-07-24 15:37). The failing log (`build_latest_verify.log`) ran
the 07-23 binary against 07-24 source, so it reproduced the already-fixed bug.

The current HEAD source (merge `36f951d` and later) contains the conversion
helpers (`syncPlatePrintSequenceConfig` / `uiPrintSequenceIndexForUpstream` /
`upstreamPrintSequenceForUiIndex`) at the correct call sites. A clean rebuild
resolves the false alarm.

Lesson: a canonical verify failure whose line number (`:3322`) does not match
the current source line (reload assertion at `:3657`) is a strong stale-binary
signal — confirm the test executable mtime against the source mtime before
treating it as a real regression.

Separately, the `tests/output/test_results.txt` file (UTF-16, dated 2026-06-02)
that referenced the old `E2EPipelineTests` class name and the cereal
StaticObject SKIP was deleted on 2026-07-25: those E2E skips are resolved (the
E2E suite now runs 29 passed / 0 skipped with real `Print::apply()`), and the
stale file was actively misleading root-cause investigations.
