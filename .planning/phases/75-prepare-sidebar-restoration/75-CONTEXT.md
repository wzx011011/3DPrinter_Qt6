---
phase: 75-prepare-sidebar-restoration
status: planned
created: 2026-07-05T16:10:00+08:00
source_phase: 74-prepare-source-truth-gap-audit
requirements: [SIDE-01, SIDE-02, SIDE-03]
---

# Phase 75 Context

## Objective

Restore the Prepare page left sidebar to the screenshot/source-truth contract
without changing object-list, plate-strip, slice-action, or viewport-toolbar
behavior. Phase 75 owns:

- `PREP-SIDEBAR`
- `PREP-SIDEBAR-OPTIONS`
- `PREP-FILAMENT`

## Source Truth

- Visual target: `shotScreen/准备页.png`
- Current evidence: `.planning/milestones/prepare-gap-current-20260704.png`
- Audit matrix: `.planning/phases/74-prepare-source-truth-gap-audit/74-GAP-MATRIX.md`
- Upstream anchors:
  - `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp:480-556`
  - `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp:1730-2178`
  - `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp:2051-2178`
  - `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp:2324-2421`
  - `third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp`
  - `third_party/OrcaSlicer/src/slic3r/GUI/ConfigManipulation.*`

## Current Gaps

1. Sidebar width and density are visually heavier than the target at the
   current runtime window size.
2. Section cards use a prominent rounded-card visual treatment that does not
   match the flatter, compact upstream sidebar.
3. Filament slots render as large colored blocks rather than compact rows with
   a color swatch, index, preset combo, and status affordance.
4. Inline process options still expose raw value-source keys such as
   `default`, `print`, and `filament`.
5. Option rows are taller than the target sidebar and use oversized edit boxes.

## Locked Scope

- Modify current sidebar files instead of replacing the whole Prepare page.
- Keep all existing backend calls and preset state gates.
- Keep object list and object settings placement for Phase 76.
- Keep top action placement, slice button placement, and viewport controls for
  Phases 76 and 77.
- Do not add new product behavior without an upstream source anchor.

## Target Files

- `src/qml_gui/pages/PreparePage.qml`
- `src/qml_gui/panels/DockableSidebar.qml`
- `src/qml_gui/panels/LeftSidebar.qml`
- `src/qml_gui/components/CollapsibleSection.qml`
- `src/qml_gui/components/FilamentSlot.qml`
- `src/qml_gui/components/OptionRow.qml`
- `src/qml_gui/BackendContext.h`
- `src/qml_gui/BackendContext.cpp`
- Translation catalogs only if lupdate/lrelease impact is required by build.

## Verification Targets

- QML/source audit confirms no visible `default`, `print`, `filament`, or
  `printer` source keys are displayed in inline sidebar value-source text.
- QML/source audit confirms sidebar width clamps are updated consistently
  between backend and Prepare page.
- `git diff --check` passes.
- Encoding guard passes.
- Focused build or QML smoke verification runs if the touched files require it.
