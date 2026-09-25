# Phase 182: Editor Workflow Fix + Architecture-Divergence Research

**Status:** Planned
**Workstream:** CL
**Requirement:** CRASH-03
**Dependencies:** none (Wave A parallel)

## Goal

(1) Port one editor-workflow fix (STEP reload_from_disk). (2) Produce a RESEARCH.md deliverable classifying 4 architecture-divergence items as applicable / not applicable / deferred.

## Scope

### A7 — STEP reload_from_disk case-insensitive fallback (upstream `5ba5c6672d`)

- **Upstream fix:** `src/slic3r/GUI/Plater.cpp` + `Preferences.cpp` — reopening a project failed STEP `reload_from_disk` because only the filename was stored (the absolute path was lost on save). The fix adds a case-insensitive filename fallback match and a new Preferences checkbox "preserve full source path".
- **Qt6 equivalent:** `src/core/viewmodels/EditorViewModel.cpp` (the `reload_from_disk` method already exists) + `src/qml_gui/pages/PreferencesPage.qml` (for the new checkbox).
- **Approach:** Audit `EditorViewModel::reloadFromDisk` path-matching logic. Add a case-insensitive filename-only fallback when the absolute path doesn't exist. Add the Preferences checkbox and respect it in the save path.

### RESEARCH.md deliverable (4 items, investigation only — no code unless investigation surfaces a real Qt6 bug)

Produce `.planning/milestones/v5.4-phases/182-editor-workflow-and-arch-divergence-research/RESEARCH.md` classifying each item:

#### A3 — Measure SPHERE_2 first/second feature confusion (upstream `adc8763099`)
- **Upstream fix:** `src/slic3r/GUI/Gizmos/GLGizmoMeasure.cpp` — SPHERE_2 gripper used `.first.feature` where it should have used `.second.feature`; pure-edge selection crashed.
- **Qt6 path to investigate:** `src/core/rendering/MeasureEngine.{cpp,h}` + `AssemblyMeasureGeometry.{cpp,h}`. Qt6 Measure is a **self-built rewrite** (not a port of GLGizmoMeasure).
- **Expected outcome:** Not applicable — Qt6 Measure doesn't have the SPHERE_2 gripper concept. Confirm by searching for `.first.feature` / `.second.feature` patterns; if absent, mark "not applicable, Qt6 Measure is independent".

#### A5 — wxGrid ObjectTable dynamic_cast crash (upstream `59e655dd1d`)
- **Upstream fix:** `src/slic3r/GUI/GUI_ObjectTable.cpp` + `.hpp` — wxGrid dynamic_cast crash when rendering coFloat cells with nullable variants.
- **Qt6 path to investigate:** `src/qml_gui/panels/ObjectList.qml`. Qt6 has **no wxGrid** — ObjectList is a QML ListView/TableView.
- **Expected outcome:** Not applicable — Qt6 has no equivalent grid-rendering path. Mark "not applicable, no wxGrid in Qt6".

#### A6 — Filament region collapse boundary (upstream `49fe64cb07`)
- **Upstream fix:** `src/slic3r/GUI/Plater.cpp` — toolchanger/IDEX sidebar filament-region collapse boundary calculation.
- **Qt6 path to investigate:** `src/qml_gui/components/FilamentSlot.qml`. Qt6 uses QML layout, not wxWidgets sidebar geometry.
- **Expected outcome:** Not applicable (or "deferred") — Qt6's filament slot layout is fundamentally different. Confirm by inspecting FilamentSlot.qml's collapse behavior; if no equivalent geometry issue exists, mark "not applicable".

#### A10 — OpenGL stencil outline thickness (upstream `33909a51cd`)
- **Upstream fix:** `src/slic3r/GUI/3DScene.cpp` + 10 GLSL shaders (gouraud/phong/ssao in {110,140}/{fs,vs}). Stencil-buffer outline thickness rendering fix (`frag_color` → `gl_FragColor`).
- **Qt6 path to investigate:** `src/qml_gui/Renderer/RhiViewportRenderer.cpp` + `src/qml_gui/Renderer/shaders/`. Qt6 uses **RHI + self-built shaders** (`rhi_viewport.{frag,vert}` + `rhi_gizmo.{frag,vert}` — only 4 shaders, no gouraud/phong/ssao).
- **Expected outcome:** Not applicable — Qt6 has no stencil-buffer outline path using those shaders. Confirm by checking RhiViewportRenderer's outline rendering; if it uses a different technique (e.g. inverted hull), mark "not applicable, Qt6 RHI outline is independent".

## Out of Scope

- Any code change for A3/A5/A6/A10 beyond what RESEARCH.md findings justify.
- Any change to libslic3r.

## Decision (2026-07-20): In-place fix policy for research findings

If investigation of A3/A5/A6/A10 surfaces a real Qt6 bug (not just "not applicable"), **fix it inside this phase** rather than opening a follow-up phase. The phase scope expands to absorb the fix. Document the expanded scope in RESEARCH.md and SUMMARY.md. This avoids the overhead of spinning up a new phase for what may be a small fix once the bug is identified.

If the fix turns out to be large (>300 lines), surface it in SUMMARY.md and pause for user decision before continuing — large scope expansion should be deliberate.

## Verification

- A7: QmlUiAuditTests anchor added in Phase 187. Manual test: save project with STEP model, close, reopen, trigger reload_from_disk — STEP model reloads successfully even if absolute path moved.
- RESEARCH.md: each of A3/A5/A6/A10 has an explicit "not applicable" / "deferred" / "fixed in this phase" verdict with evidence (grep output / file references).
- Canonical build exits 0, 0 errors.

## Deliverables

- EditorViewModel.cpp + PreferencesPage.qml changes (A7 fix).
- `RESEARCH.md` in this phase directory (4-item investigation report).
