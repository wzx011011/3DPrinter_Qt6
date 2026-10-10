# Build Rules

This project has one canonical full verification path.

## Canonical Command

Run full verification with:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

## Canonical Build Directory

Use only:

```text
build/
```

## Rules

- Do not create alternate build directories.
- Do not use alternate full-build scripts.
- Do not bypass `scripts/auto_verify_with_vcvars.ps1` when claiming full build verification.
- Do not clean or delete `build/` unless the user explicitly asks for that operation.
- Documentation-only phases may use lightweight checks instead of a full rebuild, but must say that no full rebuild was run.

## Slice Regression Gate (part of the canonical verify)

`auto_verify_with_vcvars.ps1` ends with a conditional slice regression gate
(`scripts/perf/run_slice_gate.ps1`). It fires when uncommitted changes or the
commits since the last green gate run touch the slicing surface (SliceService,
PreviewViewModel, EditorViewModel, `src/cli/*`, ProjectServiceMock,
PresetServiceMock, the OrcaSlicer submodule pointer, PerfBench.cpp, and the
gate's own files).

- Correctness layer (hard fail, exit 1): headless `owzx-cli` slices of
  `bench_sphere_400k.stl` + `Block20XY.stl` with pinned config are normalized
  by `tools/gcode_compare.py` and compared field-exact against the committed
  golden baseline `tests/data/slice_gate_baseline.json`.
- Efficiency layer (hard fail, exit 2): PerfBench slice stage, min-of-N
  wall clock (default N=3, `OWZX_SLICE_GATE_REPEATS`), vs baseline thresholds
  (slice_ms +25%, preview_parse_ms +10%; one automatic re-measure round before
  failing — docs/perf-baseline.md noise rules).
- Exit codes: 0 pass / 1 correctness regression / 2 efficiency regression /
  3 infra error. Any nonzero exit fails the whole verify.
- `OWZX_SLICE_GATE=0` skips; `OWZX_SLICE_GATE=force` runs regardless of the diff.
- Re-golding after an INTENDED slice-output change (engine upgrade, config
  default change): run
  `powershell -NoProfile -ExecutionPolicy Bypass -File scripts/perf/run_slice_gate.ps1 -UpdateBaseline`
  on a green build and commit the rewritten `tests/data/slice_gate_baseline.json`
  together with the change. A missing baseline is auto-established (yellow)
  from the current build on the next gate run.
- Gate state (last green commit) lives in `build/perf_out/slice_gate_state.json`;
  per-run details land in `build/perf_out/slice_gate_report.json`.

## Expected Environment

- Windows 10/11.
- Visual Studio 2022 MSVC toolchain.
- CMake and Ninja.
- Qt 6.10.
- Prebuilt OrcaSlicer dependency prefix as configured by the project CMake files.
- Git on PATH at configure time: the from-source build applies
  `patches/orcaslicer/*.patch` onto the pristine `third_party/OrcaSlicer`
  submodule checkout (see `cmake/ApplyOrcaSlicerPatches.cmake`).

## OrcaSlicer Submodule

- The submodule stays a pristine upstream checkout; the gitlink pins the
  upstream baseline commit and carries no OWzx edits.
- All OWzx compatibility changes live in `patches/orcaslicer/*.patch` and are
  applied to the submodule working tree automatically at configure time.
  Configure is idempotent (fingerprint stamp `.owzx-patches-applied` plus a
  dirty-tree check); an out-of-sync submodule fails configure loudly instead
  of building an unknown tree.
- After changing the patch series, regenerate the baseline state with:
  reset the submodule to the pinned commit, drop untracked files created by
  the old series, and re-run configure.

## Verification Reporting

When reporting verification, state:

- The exact command run.
- Whether configure/build/app smoke/E2E tests passed.
- Whether `ViewModelSmokeTests.exe` was built only or also executed.
- Any environment-dependent checks that were skipped or require manual validation.
