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

## Canonical Verify Pass Criteria (Gate Truth: 9 Blocking + 3 Non-Blocking)

The real pass condition of `auto_verify_with_vcvars.ps1` is: **the 9 blocking
suites all report 0 failed AND the script exits 0** — not the literal
"all 12 suites 0 failed". Three of the 12 suites the script builds and runs
are deliberately non-blocking (failure prints a yellow warning, the script
does not exit). This split was audited as Q13
(docs/代码债与架构审计_2026-10-10.md ④ 章 Q13) and is recorded here so the
gate is never described as stricter than it actually is. Cite this section —
not a suite count — when stating whether a verify run passed.

### Blocking suites (red + `exit` on failure; missing exe = red `exit 1`)

1. `PrepareSceneDataTests`
2. `PartPlateTests`
3. `ObjectPickingTests`
4. `ViewModelSmokeTests`
5. `QmlUiAuditTests`
6. `GizmoGeometryTests`
7. `NavigatorCubeTests`
8. `CameraParityTests`
9. `PreviewParserTests`

Configure, build (all 12 test targets), the app-launch liveness check, the
conditional RenderBench stage, and the conditional slice regression gate are
also blocking. On success the script's final action is `exit 0`.

### Non-blocking suites (yellow on failure; gray skip if exe missing)

| Suite | Why it is non-blocking today | Converts to blocking when |
|---|---|---|
| `AppToolTests` | Deterministic import-time heap-corruption crash inside `initTestCase` since 2026-08-31 (0xC0000005 in ntdll heap alloc, identical address across runs; needs PageHeap/admin to attribute). See the `[AppTool]` stage comment in the verify script. | The registered debt item below lands: crash attributed **and fixed**, the suite actually passes, then the stage restores red-exit. |
| `ViewportContextMenuTests` | Same import-path heap-corruption family (silent exit during `initTestCase` import; floats between processes/build layouts — passed 5/5 in earlier same-day runs). See the `[ViewportContext]` stage comment. | Same attribution pass and same conversion as `AppToolTests`. |
| `E2EWorkflowTests` | ~20 `QSKIP` sites absorb slice failures (e.g. tests/E2EWorkflowTests.cpp:324), so "environment missing slice preconditions" and "pipeline broken" are indistinguishable; a red gate today would be a false red. | Together with the Q12 environment-probe semantics fix: SKIP only when slice preconditions are missing (counted and reported); FAIL when preconditions exist and slicing fails. |

Non-blocking applies to **runtime results only**. The script still builds all
three targets via `Invoke-NinjaTarget` with `Required = $true`; a compile
failure of a non-blocking suite fails the whole verify.

When a suite converts to blocking, update this section and the AGENTS.md
self-test wording in the same change; the acceptance criterion then rises
automatically to "all suites 0 failed". The suite count itself will grow
12 → 13 when the Q14 QML smoke suite enters the gate (dev plan 2026-10-10,
M2) — sync this section and AGENTS.md at that point, too.

### Registered debt: import-time heap-corruption attribution (Q13 follow-up)

**Deadline: end of milestone M1** (dev plan 2026-10-10 §2/§5 — the audit's
quick-win ordering places full Q13 alongside the M1 items). If the deadline
slips, the item must be explicitly re-planned in the next milestone plan; it
must not silently remain indefinite yellow. Scope:

1. **Fix the `EarlyCrashDiag` compile guard** (`src/core/debug/EarlyCrashDiag.cpp`):
   the entire TU is guarded by `#ifdef Q_OS_WIN` while the file deliberately
   includes no Qt headers (its own header comment says so), so the macro is
   never defined, the TU compiles to an empty object file and the early-crash
   handler is inert at runtime. Include a Qt header, or switch the guard to a
   compiler/platform macro (e.g. `_WIN32`), so the handler actually registers.
2. **Attribute both import-time heap corruptions** via PageHeap (needs admin)
   reproduction: the `initTestCase` import path of `AppToolTests` and
   `ViewportContextMenuTests` (0xC0000005 family; see the `[AppTool]` /
   `[ViewportContext]` stage comments in the verify script).
3. **Fix the attributed corruption**, confirm both suites pass, then restore
   red-exit blocking for them in `auto_verify_with_vcvars.ps1` and update this
   section + AGENTS.md in the same change. `E2EWorkflowTests` converts with
   Q12 as described in the table above, not with this item.

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

## RHI Rendering Backend Commitment

- The supported rendering configuration is **Windows + D3D11, a single
  backend**. The backend selector probes D3D11-first
  (`src/qml_gui/Renderer/RhiBackendSelector.cpp`); that is the product
  commitment, not an accident of the code.
- D3D12 is **not** part of the commitment: it crashes at QQuickWindow
  swapchain init (0xC0000005) on AMD Radeon APU (mainstream integrated)
  graphics — confirmed on a real machine in v5.7 Phase 211. It remains
  reachable only as an explicit debug opt-in via `OWZX_RHI_RENDERER=d3d12`
  (with `OWZX_D3D12_DEBUG` for the debug layer), not as a default candidate.
- Non-D3D11 backends (Vulkan/OpenGL/Metal) and non-Windows CI are
  deliberately out of scope until a product decision re-opens them (dev
  plan 2026-10-10 §5). Do not widen the backend matrix or "fix" gate
  failures by adding backend candidates without that decision.

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
