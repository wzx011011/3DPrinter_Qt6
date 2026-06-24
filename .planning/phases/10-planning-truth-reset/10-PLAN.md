---
phase: 10
phase_name: Planning Truth Reset
plan_id: 10-01
title: Restore planning and rule source of truth
status: ready
wave: 1
requirements_addressed:
  - PLAN-01
  - PLAN-02
  - PLAN-03
  - PLAN-04
  - PLAN-05
files_modified:
  - .Codex/rules/source-truth-migration.md
  - .Codex/rules/build-rules.md
  - .Codex/rules/qml-boundaries.md
  - .Codex/agents/source-truth-migration.toml
  - .planning/phases/10-planning-truth-reset/10-SUMMARY.md
  - .planning/phases/10-planning-truth-reset/10-VERIFICATION.md
  - .planning/phases/10-planning-truth-reset/10-REVIEW.md
  - .planning/STATE.md
  - .planning/ROADMAP.md
  - .planning/REQUIREMENTS.md
---

# Plan 10-01: Restore planning and rule source of truth

## Objective

Complete Phase 10 by making the repository-level rules and active planning artifacts agree with the v2.9 baseline.

## Must Haves

- `AGENTS.md` rule references resolve to real files.
- Rule files state OrcaSlicer/OWzx as the active source-truth project.
- Build rules preserve the single canonical command and `build/` directory.
- QML boundary rules state that durable behavior belongs in C++ services/viewmodels.
- The phase records evidence that planning files already reflect v2.7/v2.8 as landed history and v2.9 as active work.
- Legacy CrealityPrint references are classified as history, vendor data, or cleanup debt.

## Tasks

1. Create missing `.Codex/rules/` rule files.
2. Update `.Codex/agents/source-truth-migration.toml` to point at OrcaSlicer/OWzx rules.
3. Run text checks for missing rule references, stale active milestone text, and classification coverage.
4. Write Phase 10 summary and verification artifacts.
5. Update `.planning/STATE.md`, `.planning/ROADMAP.md`, and `.planning/REQUIREMENTS.md` for Phase 10 completion.
6. Commit only Phase 10 planning/rule files.

## Verification

- `Test-Path .Codex/rules/source-truth-migration.md`
- `Test-Path .Codex/rules/build-rules.md`
- `Test-Path .Codex/rules/qml-boundaries.md`
- `rg -n "third_party/CrealityPrint|CrealityPrint -> Qt6|CrealityPrint 鈫|\\.claude/rules|\\.claude/skills" .codex/agents .codex/hooks.json .codex/rules`
- `rg -n "Current Milestone: v2.6|Milestone:\\*\\* v2.6|v2.7 新|v2.6 Complete" .planning/PROJECT.md .planning/STATE.md .planning/ROADMAP.md .planning/INDEX.md`
- `rg -n "Real|Hybrid|Mock|Blocked|Placeholder" .planning/REQUIREMENTS.md .planning/INDEX.md .planning/PROJECT.md`

## Out of Scope

- Source code cleanup under `src/`.
- Full canonical rebuild.
- Product feature implementation.
