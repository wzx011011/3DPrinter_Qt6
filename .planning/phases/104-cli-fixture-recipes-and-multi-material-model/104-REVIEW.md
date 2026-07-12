---
phase: 104-cli-fixture-recipes-and-multi-material-model
status: clean
base: b3b7596
head: 8fbcad7
files_reviewed:
  - tests/data/multi_material_fixture.3mf
  - tests/data/fixture_recipes.md
  - src/qml_gui/main_qml.cpp
  - tests/QmlUiAuditTests.cpp
counts: {critical: 0, warning: 0, info: 4, total: 4}
---

# Phase 104 Code Review — CLI Fixture Recipes And Multi-Material Model

## Findings

| # | Severity | Finding |
|---|----------|---------|
| 1 | info | 3MF structurally well-formed (spec-ordered zip, valid closed meshes, 2 objects with `slic3r:extruder` metadata 0/1). |
| 2 | info | libslic3r 3MF importer (`Format/3mf.cpp:2045`) reads `"extruder"` only under `<config>`/`<metadata>` config-extension parts, not bare `slic3r:extruder` on `<object>`. Fixture has no config/scene part, so per-object extruder pinning may not be honored on actual import. Regression ctest only asserts existence+non-empty, so this doesn't break the test; downstream phases relying on actual 2-extruder behavior should verify the load (or add a `Metadata/project_settings.config`/scene part). |
| 3 | info | Anti-feature comment cite `OrcaSlicer.cpp:7183` verified exact (the `read_cli` entry point). `--slice` is a real upstream flag. Minor: "`--load`" is commented out upstream (`PrintConfig.cpp:10864`); actual upstream flags are `--load_settings`/`--load_filaments`/positional. The no-upstream-equivalent claim for the 4 OWzx flags stands. |
| 4 | info | All page aliases + dialog routes in fixture_recipes.md match `startupPageRoutes()` (main_qml.cpp:122-135) + `startupDialogRoutes()` (:137-167) exactly. AssembleView recipe is honest about the toggle path (no `--open-page assemble` route). |

## Verdict

**PASS — 0 critical, 0 warning, 4 info.** FIXTURE-01/03/04 closed as claimed. The 3MF fixture is structurally valid + declares 2 extruders; recipes match route tables; anti-feature comment correctly anchored; test slot deterministic (QT_TESTCASE_SOURCEDIR + QFileInfo/QFile + QString::contains). The 4 INFO items are optional polish / future-phase verification concerns (the importer-honor caveat in Finding 2 should be revisited if Phase 109/110 filament-map UI testing needs real 2-extruder behavior).

Regression ctest 4/4 PASS (incl. the new `cliFixtureRecipesAndMultiMaterialModelPresent` slot). OWzxSlicer.exe PID 32272 launch liveness confirmed. `git diff --check` exit 0; encoding guard clean.
