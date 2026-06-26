# Quick Task 260626-wqx Summary

**Task:** Implement parameter regression automation comparing upstream OrcaSlicer and OWzx CLI G-code outputs
**Corrected objective:** Parameter-chain regression first; representative G-code comparison is opt-in.
**Status:** Complete

## Implemented

- Added `tools/gcode_compare.py` for optional raw, normalized, and semantic G-code comparison.
- Added Python tests for G-code normalization/summary and runner default behavior.
- Added `scripts/param_regression.ps1`:
  - Default `smoke`/`full` modes generate OWzx and upstream-facing parameter artifacts only.
  - `-RunSlicing` enables representative OWzx/upstream slicing and G-code comparison.
  - `compare-only` compares existing generated G-code pairs.
- Added upstream-sourced representative baseline 3MFs under `tests/param_regression/baselines/`.
- Added `tests/param_regression/manifest.json` with risk-area tagged parameter cases.
- Added CTest registration for Python parameter regression tests when Python3 is available.

## Verification

- `python -m unittest tests.param_regression.test_param_regression_runner -v` - passed.
- `python -m unittest tests.param_regression.test_gcode_compare tests.param_regression.test_param_regression_runner -v` - passed.
- `powershell -ExecutionPolicy Bypass -File scripts/param_regression.ps1 -Mode smoke -OwzxCli E:/missing/owzx-cli.exe` - passed; generated parameter artifacts without slicer binaries.
- `powershell -ExecutionPolicy Bypass -File scripts/param_regression.ps1 -Mode compare-only` - passed; skipped 21 cases because no existing G-code pairs were present.
- Manifest JSON validation with `python -m json.tool` - passed.

## Notes

- Full canonical build was not run in this pass.
- The current worktree also contains unrelated render benchmark changes in `CMakeLists.txt` and `scripts/auto_verify_with_vcvars.ps1`; they were not authored as part of this task and were left intact.
- To run representative slicing comparison later, provide upstream via `-UpstreamCli` or `ORCASLICER_CLI`, then run `scripts/param_regression.ps1 -Mode smoke -RunSlicing`.
