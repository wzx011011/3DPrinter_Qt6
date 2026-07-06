---
phase: 76-prepare-workflow-panels-restoration
status: planned
created: 2026-07-05T19:05:00+08:00
source_phase: 74-prepare-source-truth-gap-audit
requirements: [OBJ-01, PLATEUI-01, STATUS-01]
mode: autonomous
---

# Phase 76 Context

## Objective

Restore the Prepare workflow panels that were explicitly deferred from Phase 75:
object/volume list states, plate strip state, and slice/export readiness
surfaces. This phase owns:

- `PREP-OBJLIST`
- `PREP-PLATEBAR`
- `PREP-TOP`
- `PREP-SLICESTATUS`

## Source Truth

- Visual target: `shotScreen/鍑嗗椤?png`
- Current evidence: `.planning/milestones/prepare-gap-current-20260704.png`
- Audit matrix: `.planning/phases/74-prepare-source-truth-gap-audit/74-GAP-MATRIX.md`
- Upstream anchors:
  - `third_party/OrcaSlicer/src/slic3r/GUI/GUI_ObjectList.cpp:135-344`
  - `third_party/OrcaSlicer/src/slic3r/GUI/GUI_ObjectList.cpp:1478-1620`
  - `third_party/OrcaSlicer/src/slic3r/GUI/GUI_ObjectList.cpp:1844-1991`
  - `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp`
  - `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.hpp`
  - `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp:272-346`
  - `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp:4667-4679`
  - `third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp:5109-5239`

## Current Gaps

1. `ObjectList.qml` still reads like a custom rounded card/chip list instead
   of a compact no-header tree/list with small selected and disabled states.
2. Object list action state is visually noisy: several operations are visible
   as small buttons even though upstream keeps most actions in context menus
   or column affordances.
3. The bottom plate strip uses broad 120 px cards and tall thumbnails. It does
   not resemble the compact PartPlate workflow marker near the bed.
4. Slice state is split between a large floating viewport `>> Slice` button and
   sidebar status panels. The floating button is a high-visibility mismatch.
5. Slice-all is always clickable in the sidebar panel even when the backend
   says the current plate cannot slice.

## Locked Scope

- Keep all backend slice, preview, export, plate, and object ViewModel calls.
- Keep `QML -> ViewModel` boundaries; no service access and no business rules
  computed in QML beyond display gating.
- Do not add new object/plate operations without an upstream source anchor.
- Do not change Phase 75 sidebar width or preset density work.
- Leave viewport tool icon restoration and gizmo floating panels to Phase 77.

## Target Files

- `src/qml_gui/panels/ObjectList.qml`
- `src/qml_gui/panels/SliceProgress.qml`
- `src/qml_gui/pages/PreparePage.qml`
- `src/qml_gui/components/GLToolbars.qml`
- `src/qml_gui/panels/LeftSidebar.qml` only if object-list section sizing
  needs a small adjustment.
- `tests/QmlUiAuditTests.cpp`

## Verification Targets

- RED/GREEN static QML audit covers Phase 76 layout contracts.
- `QmlUiAuditTests.exe` passes.
- Canonical verifier passes:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- Encoding guard passes for staged files.
