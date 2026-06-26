# Parameter Chain Regression Automation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build an automated parameter-chain regression harness that verifies OWzx and upstream-facing parameter overlays are generated from the same source-truth cases, with optional representative G-code comparison for high-risk slicing paths.

**Architecture:** A PowerShell runner owns manifest expansion, parameter overlay generation, optional slicing, optional G-code comparison, and report aggregation. A focused Python helper owns deterministic G-code normalization and comparison for the optional slicing layer. A small CTest entry runs offline smoke tests so the canonical build path verifies the architecture without requiring slicer binaries.

**Tech Stack:** PowerShell, Python 3 standard library, CMake/CTest, existing `owzx-cli.exe`, upstream OrcaSlicer CLI.

## Global Constraints

- Use only canonical full build command: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- Use only canonical build directory: `build/`.
- Do not modify libslic3r slicing algorithms.
- Upstream OrcaSlicer behavior is the source truth.
- Keep QML out of business logic; this task does not touch QML.

---

### Task 1: G-code Comparator Tests

**Files:**
- Create: `tests/param_regression/test_gcode_compare.py`

**Interfaces:**
- Produces expected API for `tools/gcode_compare.py`:
  - `normalize_gcode_text(text: str) -> list[str]`
  - `semantic_summary(lines: list[str]) -> dict[str, object]`
  - CLI: `python tools/gcode_compare.py --expected A --actual B --report R`

- [ ] **Step 1: Write failing tests**

Create tests that assert timestamp/path comments normalize away, motion lines remain, semantic summary counts layers and commands, and CLI writes a JSON report with `normalized_equal`.

- [ ] **Step 2: Run tests to verify RED**

Run: `python -m unittest tests.param_regression.test_gcode_compare -v`

Expected: import failure because `tools.gcode_compare` does not exist.

### Task 2: G-code Comparator Implementation

**Files:**
- Create: `tools/gcode_compare.py`
- Modify: `tools/__init__.py` if missing package marker is needed

**Interfaces:**
- Consumes tests from Task 1.
- Produces JSON report fields: `raw_equal`, `normalized_equal`, `expected_sha256`, `actual_sha256`, `normalized_diff`, `expected_summary`, `actual_summary`.

- [ ] **Step 1: Implement minimal comparator**

Implement normalization for generated-by, timestamp/date, absolute path, output file, thumbnail blocks, and elapsed/time statistic comments. Preserve G/M command lines and stable comments.

- [ ] **Step 2: Run tests to verify GREEN**

Run: `python -m unittest tests.param_regression.test_gcode_compare -v`

Expected: PASS.

### Task 3: Parameter Chain Manifest and Baselines

**Files:**
- Create: `tests/param_regression/manifest.json`
- Create: `tests/param_regression/baselines/README.md`
- Copy: 3 baseline `.3mf` files from `third_party/OrcaSlicer/resources/calib/...`

**Interfaces:**
- Runner reads manifest root fields:
  - `baseline_models[]` with `id`, `path`, `plates`
  - `parameter_cases[]` with `id`, `description`, `overrides`
  - `comparison.normalized_hard_fail`
  - `default_action` set to `generate-parameter-artifacts`

- [ ] **Step 1: Add compact representative baselines**

Use existing upstream 3MF files:
- `auto_pa_line_single.3mf`
- `flowrate-test-pass1.3mf`
- `pa_pattern.3mf`

- [ ] **Step 2: Add focused parameter cases**

Include smoke cases for layer height, wall loops, infill density, support enable, brim, temperature, and machine limits. Each case records `risk_area` so failures are triaged as parameter-chain coverage rather than full slicer algorithm coverage.

### Task 4: PowerShell Runner

**Files:**
- Create: `scripts/param_regression.ps1`

**Interfaces:**
- Inputs: `-Manifest`, `-OutDir`, `-OwzxCli`, `-UpstreamCli`, `-Mode smoke|full|compare-only`, `-RunSlicing`, `-SkipUpstream`
- Output: `tests/output/param_regression/summary.json`
- Calls: `python tools/gcode_compare.py --expected ... --actual ... --report ...`

- [ ] **Step 1: Implement manifest loading and case expansion**

Support `smoke` by expanding the first baseline and first two parameter cases. By default, generate OWzx flat override JSON and upstream process JSON only; mark each result `parameter_artifacts_generated`.

- [ ] **Step 2: Implement slicing commands**

Only when `-RunSlicing` is passed, run OWzx: `owzx-cli.exe --load <3mf> --load-settings <json> --slice --output-dir <dir> --quiet`.

Only when `-RunSlicing` is passed and upstream is configured, run upstream with configurable executable path, default from `ORCASLICER_CLI` env var. Use upstream-compatible args: `<3mf> --load_settings <json> --slice 0 --outputdir <dir>`.

- [ ] **Step 3: Implement compare-only and skip behavior**

If upstream CLI is absent during `-RunSlicing`, write a skipped summary instead of failing script startup unless `-RequireUpstream` is passed.

### Task 5: CTest Smoke Integration

**Files:**
- Modify: `CMakeLists.txt`

**Interfaces:**
- Adds CTest `ParamRegressionComparatorTests` that runs Python unit tests only.
- Adds runner smoke coverage for default parameter artifact generation.
- Does not require upstream OrcaSlicer binary.

- [ ] **Step 1: Add Python interpreter discovery**

Use `find_package(Python3 COMPONENTS Interpreter QUIET)`.

- [ ] **Step 2: Add test when Python is available**

`add_test(NAME ParamRegressionComparatorTests COMMAND ${Python3_EXECUTABLE} -m unittest tests.param_regression.test_gcode_compare -v WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})`.

### Task 6: Verification and Summary

**Files:**
- Create: `.planning/quick/260626-wqx-implement-parameter-regression-automatio/260626-wqx-SUMMARY.md`

**Verification Commands:**
- `python -m unittest tests.param_regression.test_gcode_compare -v`
- `python -m unittest tests.param_regression.test_param_regression_runner -v`
- `powershell -ExecutionPolicy Bypass -File scripts/param_regression.ps1 -Mode smoke`
- `powershell -ExecutionPolicy Bypass -File scripts/param_regression.ps1 -Mode compare-only`
- If time permits: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

**Completion:**
- Summarize changed files, verification results, skipped environment-dependent upstream slicing, and next steps for providing `ORCASLICER_CLI`.
