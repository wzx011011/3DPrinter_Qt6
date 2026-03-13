---
description: "Use when continuing this project in the next steady migration step. Reads current docs/TASKS.md and upstream CrealityPrint code, selects the next highest-priority unfinished task, and advances it without drifting away from source-of-truth migration rules."
agent: source-truth-migration
---

# Continue Source-Truth Migration

Continue advancing this repository as a source-truth migration of CrealityPrint into Qt6/QML.

## Goal

Pick the next best task from [docs/TASKS.md](../../docs/TASKS.md), ground the work in `third_party/CrealityPrint`, and move the project forward in a way that is measurable and does not drift from upstream behavior.

## Required Execution Rules

1. Read the current [docs/TASKS.md](../../docs/TASKS.md), [docs/PROJECT_STRUCTURE.md](../../docs/PROJECT_STRUCTURE.md), and [docs/CrealityPrint_Qt_GUI_Rewrite_Architecture.md](../../docs/CrealityPrint_Qt_GUI_Rewrite_Architecture.md).
2. Select the highest-priority unfinished task unless the user explicitly overrides the priority.
3. Identify the matching upstream source file or module in `third_party/CrealityPrint`.
4. Identify the Qt6 target module in `src/core` or `src/qml_gui`.
5. State the task ID, upstream module, Qt6 target module, and current status before making changes.
6. If the task boundary in [docs/TASKS.md](../../docs/TASKS.md) is incomplete or ambiguous, update the task document first.
7. Prefer root-cause migration work over visual-only patching.
8. Keep QML focused on presentation and move business logic into services, viewmodels, or rendering adapters when appropriate.
9. After implementation, validate the work. If runtime behavior is affected, use `scripts/auto_verify_with_vcvars.ps1`.
10. Update [docs/TASKS.md](../../docs/TASKS.md) status only according to these rules:

- `[x]` fully aligned and validated against upstream
- `[-]` partial migration or basic closure only
- `[ ]` not started or placeholder only
11. **Continuous execution**: After completing a task and updating TASKS.md, immediately re-read TASKS.md. If there are remaining unfinished tasks, proceed to the next highest-priority task without stopping for user confirmation. Continue this loop until all tasks are marked `[x]` or the context window is exhausted. Print a brief progress summary (completed count / total count) after every task.

## Preferred Priority Order

1. P0 source-of-truth versioning and feature matrix
2. P2 Prepare workspace
3. P3 Preview workspace
4. P4 Settings / Preset / override scopes
5. P5 Device / Monitor / Network
6. P6 Calibration / Model Mall / MultiMachine
7. P7 quality and release closure

## Output Format

At the start of the work, explicitly report:

- Task ID
- Upstream module
- Qt6 target module
- Why this is the next priority

At the end of each task, explicitly report:

- What was advanced
- What remains for the task to reach `[x]`
- Whether [docs/TASKS.md](../../docs/TASKS.md) status changed
- Running progress: e.g., "Progress: 12/30 tasks complete" (count of `[x]` vs total in section 3)

If continuing to the next task, briefly state the next task ID and module before proceeding.

## Do Not

- Do not free-design new workflows if upstream behavior already exists.
- Do not mark work complete just because UI exists.
- Do not skip reading upstream code.
- Do not continue random page beautification without a mapped task.
