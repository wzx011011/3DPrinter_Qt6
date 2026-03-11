# CrealityPrint Qt6 Workspace Instructions

This repository is a source-truth migration project.

## Core Rule

- Treat `third_party/CrealityPrint` as the upstream source of truth for user-visible behavior, workflow, and feature scope.
- Treat the Qt6/QML code under `src/` as the migration and presentation layer that must inherit upstream behavior.
- Do not invent new product behavior when an upstream implementation already exists.

## Required Workflow

Before making code changes:

1. Identify the matching task in [docs/TASKS.md](../docs/TASKS.md).
2. Identify the upstream source file or module in `third_party/CrealityPrint`.
3. Confirm the target Qt6 module that should carry the behavior.

If no matching task exists:

- Update [docs/TASKS.md](../docs/TASKS.md) first.
- Do not start implementation against an undocumented target.

## Progress Accounting

- Do not mark work as complete just because a page exists or a basic chain runs.
- A task is complete only after upstream behavior has been compared and validated.
- Use the task status semantics defined in [docs/TASKS.md](../docs/TASKS.md):
  - `[x]` fully aligned and validated against upstream
  - `[-]` basic migration or partial closure only
  - `[ ]` not started or placeholder only

## Architecture Boundaries

- Keep architecture aligned with [docs/CrealityPrint_Qt_GUI_Rewrite_Architecture.md](../docs/CrealityPrint_Qt_GUI_Rewrite_Architecture.md).
- Keep module boundaries aligned with [docs/PROJECT_STRUCTURE.md](../docs/PROJECT_STRUCTURE.md).
- Prefer adapting upstream behavior into `src/core/services`, `src/core/viewmodels`, and `src/core/rendering` instead of embedding business logic in QML.
- Keep QML focused on presentation, composition, and interaction wiring.

## Preferred Execution Order

Prioritize work in this order unless the user explicitly overrides it:

1. P0 source-of-truth versioning and feature matrix
2. P2 Prepare workspace
3. P3 Preview workspace
4. P4 Settings / Preset / override scopes
5. P5 Device / Monitor / Network
6. P6 Calibration / Model Mall / MultiMachine
7. P7 quality and release closure

## Validation Expectations

- Use `scripts/auto_verify_with_vcvars.ps1` as the authoritative full validation path when code changes affect runtime behavior.
- Prefer root-cause fixes over visual-only patches.
- When reporting progress, mention both the task ID and the upstream module being migrated.

## Anti-Patterns

- Do not continue page-by-page beautification without a mapped upstream target.
- Do not treat old phase completion notes as proof of feature completeness.
- Do not add new mock-only behavior if the real upstream path already exists or should exist.
