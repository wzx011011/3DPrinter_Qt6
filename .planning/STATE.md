# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-05-31)

**Core value:** Up upstream CrealityPrint source is functional truth -- Qt6 code must fully inherit upstream behavior, never freely design new product behavior.
**Current focus:** Milestone v1.3: CLI Port

## Current Position

Phase: v13-03 Preset Config + Slicing
Status: **Blocked** — libslic3r Print::apply() crashes with 0xC0000005 in CLI context
Last activity: 2026-06-02

Progress: [████░░░░] 50%

## Blocker

**libslic3r crash in headless context:** `Print::apply()` crashes with access violation (0xC0000005) when called from CLI/test context. Same crash affects E2EPipelineTests. The GUI app works because it has a different initialization path (OpenGL context, BackendContext, etc.).

Root cause TBD — likely related to CRT mismatch or missing global state initialization that the GUI path provides.

## Session Continuity

Last session: 2026-06-02
Stopped at: Phase 3 blocked by libslic3r crash
Resume file: .planning/ROADMAP.md
Next step: Debug libslic3r Print::apply() crash in headless context

## Completed This Session

- Phase 1 (CMake + CLI Skeleton): `creality-cli.exe --help` runs, exit codes correct
- Phase 2 (Arg Parsing + Model Loading): `--load model.stl` prints object/plate info, async loading with QEventLoop, file-not-found error (-3), --quiet mode
- Phase 2 fix: main_cli.cpp was entering app.exec() on negative exit codes — fixed to return directly
- Phase 3 code written: preset merge (printer→filament→print), SliceService wiring, G-code export
- Direct slice diagnostic: confirmed model has valid geometry (1 obj, 1 vol), clone works correctly, crash is inside libslic3r Print::apply()
