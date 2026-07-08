---
status: resolved
trigger: prepare page top chrome overlap, printer/filament settings buttons not clickable, and viewport/cube visual verification gap
created: 2026-07-08
updated: 2026-07-08
---

# Symptoms

- Expected: Prepare top chrome rows are visually separated; workflow tabs do not overlap the file/save/title controls.
- Actual: Screenshot shows the top tool row and workflow row occupying the same vertical region.
- Expected: Printer and filament settings entry buttons open their settings pages.
- Actual: The buttons can be disabled or route to the wrong wizard entry.
- Expected: The lower-left viewport/thumbnail area does not overlap controls, and a 3D cube can be loaded for visual verification without UI clicks.
- Actual: The bottom view toolbar overlaps the plate thumbnail strip; model loading requires UI interaction.

# Current Focus

- hypothesis: BBLTopbar first RowLayout fills the full 70px chrome while workflowBar starts at 36px; LeftSidebar gates settings entry buttons with preset rename blockers and printer header opens config wizard; GLToolbars bottom controls share the same bottom space as plateBar.
- test: Add QmlUiAuditTests source checks covering the three regressions.
- expecting: New tests fail before implementation and pass after the QML/C++ changes.
- next_action: Add RED tests.

# Evidence

- 2026-07-08: `src/qml_gui/BBLTopbar.qml` line 123 uses `anchors.fill: parent` for the first tool row; `workflowBar` starts lower in the same 70px chrome.
- 2026-07-08: `src/qml_gui/panels/LeftSidebar.qml` printer header calls `backend.showConfigWizard()`; printer and filament gear entries use `presetActionBlocker(..., "rename")` in `enabled`.
- 2026-07-08: `src/qml_gui/components/GLToolbars.qml` lower-left view controls use a fixed bottom margin of 42; `src/qml_gui/pages/PreparePage.qml` plateBar uses bottom margin 50 and height 44.
- 2026-07-09: Startup with `--open-page prepare --load-model 20mm_cube.obj` loaded the cube but exposed a plate thumbnail data URL double-prefix warning.

# Eliminated

- hypothesis: The global resize overlay blocks settings buttons.
  reason: The overlay Rectangle does not accept mouse input except narrow edge MouseAreas; button disable state is local to LeftSidebar.

# Resolution

- root_cause: Top title controls filled the full 70px chrome and overlapped workflowBar; settings entry buttons were incorrectly tied to preset rename mutability and one printer header opened the first-run wizard; lower-left view controls shared the same bottom band as plate thumbnails; GL thumbnail data already included a data URL prefix while QML prepended another one.
- fix: Constrained BBLTopbar title controls to 36px, made printer/filament settings entries always route to `forwardSettingsRequest`, moved GLToolbars lower-left controls above the plate bar, added `--load-model` startup loading, and normalized thumbnail source URL handling.
- verification: `ctest --test-dir build -R "PrepareSceneDataTests|PartPlateTests|ViewModelSmokeTests|QmlUiAuditTests|PreviewParserTests" --output-on-failure` passed; `OWzxSlicer.exe --skip-first-run --open-page prepare --load-model .../20mm_cube.obj` stayed running and logged handled open-page/load-model with no thumbnail decode warning.
- files_changed: `src/qml_gui/BBLTopbar.qml`, `src/qml_gui/panels/LeftSidebar.qml`, `src/qml_gui/components/GLToolbars.qml`, `src/qml_gui/pages/PreparePage.qml`, `src/qml_gui/main_qml.cpp`, `tests/QmlUiAuditTests.cpp`.
