# Parameter Regression Baseline 3MFs

These baseline projects are copied from `third_party/OrcaSlicer/resources/calib/`
so the regression harness starts from upstream-owned source-truth assets.

The primary goal is parameter-chain regression: each manifest case generates
the OWzx flat override JSON and the upstream process-preset JSON from one
source-truth override set. Representative slicing/G-code comparison is opt-in
via `scripts/param_regression.ps1 -RunSlicing`.

Current set:

- `auto_pa_line_single.3mf` - compact single-filament pressure advance project.
- `flowrate-test-pass1.3mf` - flow-rate calibration project with parameter-sensitive paths.
- `pa_pattern.3mf` - pressure advance pattern project with dense path planning.

The runner treats these files as immutable inputs. Add new baselines only when
they cover a new parameter-chain risk area, not to brute-force every possible
G-code output.
