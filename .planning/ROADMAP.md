# Roadmap: Milestone v1.3 — CLI Port

## Overview

Port upstream CrealityPrint's headless CLI to Qt6, enabling automated slicing, CI/CD integration, and batch processing without a GUI. The Qt6 service layer (ProjectServiceMock, SliceService, PresetServiceMock) is already standalone-capable — the gap is infrastructure: CLI entry point, CMake target, argument parsing, and E2E test coverage.

Upstream reference: `CLI::run()` in `CrealityPrint.cpp` (~6000 lines), supports `--load`, `--slice`, `--export-3mf`, `--load-settings`, `--arrange`, `--orient`.

## Phases

- [ ] **Phase 1: CMake + CLI Skeleton** — Build `creality-cli.exe`, `--help` runs
- [ ] **Phase 2: Arg Parsing + Model Loading** — `--load model.stl` prints object/plate info
- [ ] **Phase 3: Preset Config + Slicing** — Load → config → slice → G-code export pipeline
- [ ] **Phase 4: 3MF Export + Transforms** — `--export-3mf`, `--arrange`, `--orient`, `--rotate`, `--scale`

## Phase Details

### Phase 1: CMake + CLI Skeleton
**Mode:** MVP
**Goal:** `creality-cli.exe --help` runs and prints usage.
**Depends on:** Nothing (builds on v1.1/v1.2 service layer)
**Requirements:** CLI-01
**Success Criteria:**
  1. `creality_cli_core` OBJECT library compiles without QML/OpenGL deps
  2. `creality-cli` executable links and runs
  3. `--help` prints usage with `--load`, `--slice`, `--output-dir` options
  4. No args prints usage (or exits with help), no crash
  5. Invalid args produce error messages (not crashes)
**Plans:** 1 plan
Plans:
- [ ] v13-01-01-PLAN.md — CMake skeleton, entry point, CliRunner orchestrator

### Phase 2: Arg Parsing + Model Loading
**Mode:** MVP
**Goal:** `--load model.stl` prints object names, plate count; `--load project.3mf` shows multi-plate layout.
**Depends on:** Phase 1
**Requirements:** CLI-02
**Success Criteria:**
  1. `--load hotend.stl` loads STL, prints 1 object, plate count = 1
  2. `--load ksr_fdmtest_v4.3mf` loads 3MF, prints multi-plate layout
  3. `--load nonexistent.stl` exits with code -3 (CLI_FILE_NOTFOUND)
  4. Multiple `--load` flags load multiple models
  5. Exit codes match upstream CLI_* error code macros
**Plans:** 1 plan
Plans:
- [ ] v13-02-01-PLAN.md — QCommandLineParser, model loading via ProjectServiceMock

### Phase 3: Preset Config + Slicing
**Mode:** MVP
**Goal:** `--load model.stl --load-settings preset.json --slice --output-dir ./out` produces G-code.
**Depends on:** Phase 2
**Requirements:** CLI-03
**Success Criteria:**
  1. Load STL → slice → G-code file exists and is >1KB
  2. G-code contains slicer header comments and G0/G1 movement commands
  3. `--load-settings` JSON overlays onto default preset config
  4. Multi-plate 3MF produces per-plate G-code files
  5. Invalid preset exits with code -5 (CLI_CONFIG_FILE_ERROR)
  6. Slice results (time, weight, filament) emitted to stdout
**Plans:** 1 plan
Plans:
- [ ] v13-03-01-PLAN.md — PresetServiceMock wiring, SliceService async pipeline, G-code export

### Phase 4: 3MF Export + Transforms
**Mode:** MVP
**Goal:** Support `--export-3mf`, `--arrange`, `--orient`, `--rotate`, `--scale`.
**Depends on:** Phase 3
**Requirements:** CLI-04
**Success Criteria:**
  1. `--export-3mf` produces valid 3MF that round-trips (reload in GUI preserves data)
  2. `--arrange` repositions objects using real bed boundaries
  3. `--orient` applies auto-orientation
  4. `--rotate x,y,z` and `--scale factor` apply transforms before slice
  5. Arrange failure exits with code -21 (CLI_OBJECT_ARRANGE_FAILED)
**Plans:** 1 plan
Plans:
- [ ] v13-04-01-PLAN.md — store_bbs_3mf export, transform APIs, round-trip verification

## E2E Testing

New CTest target `CliTests` with phase-gated test cases. Uses `QProcess` to invoke CLI as subprocess, validates exit codes, stdout/stderr, and output files. See plan file for full test matrix (22 test cases across 4 phases).

Test models reuse existing: `hotend.stl`, `3DBenchy.stl`, `Block20XY.stl`, `ksr_fdmtest_v4.3mf`.

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. CMake + CLI Skeleton | 0/1 | Executed | 2026-06-02 |
| 2. Arg Parsing + Model Loading | 0/1 | Executed | 2026-06-02 |
| 3. Preset Config + Slicing | 0/1 | Blocked | — |
| 4. 3MF Export + Transforms | 0/1 | Pending | — |
