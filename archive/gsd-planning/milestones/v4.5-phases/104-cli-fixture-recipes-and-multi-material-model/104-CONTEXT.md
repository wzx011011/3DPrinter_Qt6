# Phase 104: CLI Fixture Recipes And Multi-Material Model - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Close FIXTURE-01 (multi-material fixture model), FIXTURE-03 (argv fixture recipes), FIXTURE-04 (anti-feature doc). Phase 103 closed FIXTURE-02 (the readiness gate); Phase 104 fills in the fixture content the gate now deterministically applies.

Success criteria (from ROADMAP):
1. A multi-material fixture model exists in `tests/data/` so multi-material-dependent features (wipe-tower, filament-map, AMS) can be exercised deterministically.
2. argv fixture recipes (documented combos of `--load-model X --open-page Y`) cover the major GUI states.
3. The argv fixtures are documented as test-evidence plumbing, NOT a user-facing deep-link product feature (anti-feature).

Requirements: **FIXTURE-01, FIXTURE-03, FIXTURE-04**.

</domain>

<decisions>
## Implementation Decisions

### Carry-Forward Inputs
- Phase 103 (FIXTURE-02): the argv plumbing is now deterministically gated on objectCreated + frameSwapped. Phase 104 uses that gate but does not modify it.
- Research SUMMARY.md: "argv fixtures are recipes + fixtures, not new flags." The 4 QCommandLineOption flags already exist (main_qml.cpp:177-196); the page/dialog routes already exist (main_qml.cpp:122-160 — 7 page routes: home/prepare/preview/device/multi-device/project/calibration; dialog routes for settings:printer etc).
- Existing fixture inventory: `tests/data/test_model.stl` (single-material), `tests/data/baseline/` (screenshot baselines). Multi-material fixture is missing.

### Known Phase 104 Scope
1. **FIXTURE-01 — multi-material fixture model:** Add a multi-material 3MF or STL fixture to `tests/data/`. Options: (a) copy a small upstream multi-material test 3MF from `third_party/OrcaSlicer/tests/data/`, (b) generate a minimal 2-extruder fixture programmatically. Prefer (a) — source-truth-aligned, real geometry.
2. **FIXTURE-03 — argv fixture recipes:** A `tests/data/fixture_recipes.md` (or similar doc) listing the canonical `--load-model X --open-page Y --open-dialog Z` combos that reach each major GUI state (Prepare empty, Prepare with model, Preview, AssembleView, settings:printer, calibration). Each recipe is a one-liner executable via `OWzxSlicer.exe <flags>`.
3. **FIXTURE-04 — anti-feature doc:** A comment block in main_qml.cpp (near the QCommandLineParser setup at :170-205) explicitly documenting that these flags are OWzx-only test-evidence plumbing, NOT a user-facing deep-link product feature (upstream has no equivalent — upstream argv is CLI-only `--load`/`--slice`/positional at `OrcaSlicer.cpp:7183`).
4. **Regression test:** a source-audit QmlUiAuditTests slot confirming the fixture recipes doc exists + the anti-feature comment is present (locks FIXTURE-03/04 against removal).

</decisions>

<code_context>
Key anchors:
- `tests/data/` (current fixture home — single-material only)
- `src/qml_gui/main_qml.cpp:122-160` (existing page/dialog routes — the recipe targets)
- `src/qml_gui/main_qml.cpp:170-205` (QCommandLineParser — where the anti-feature comment lands)
- `third_party/OrcaSlicer/tests/data/` (potential source-truth multi-material fixtures)
- `tests/QmlUiAuditTests.cpp` (source-audit test home)

</code_context>

<deferred>
- FIXTURE-02 (gate) — Phase 103 (done).
- WS4 D3D12 crash-repro that consumes these fixtures — Phase 106.
- LAN/device/cloud scope (removed).

</deferred>
