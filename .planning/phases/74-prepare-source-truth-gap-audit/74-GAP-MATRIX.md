# Phase 74 Prepare Gap Matrix

**Target evidence:** `shotScreen/准备页.png`
**Current evidence:** `.planning/milestones/prepare-gap-current-20260704.png`
**Scope:** Prepare page only

## Summary

The current Prepare page is structurally functional but still materially
off-target visually. The largest gaps are:

1. Left sidebar width, density, option labels, and card style.
2. Slice/export/action placement.
3. Viewport color/bed presentation and toolbar placement.
4. Text-glyph tool buttons instead of compact icon-first toolbars.
5. Need to re-check visible object/list/gizmo entries for dead or misleading
   controls before declaring parity.

## Canonical Region Matrix

| Region | Target Observation | Current Evidence | Qt Targets | Upstream Source | Gap | Severity | Owner | Requirement | Verification |
|---|---|---|---|---|---|---|---|---|---|
| PREP-TOP | Compact top shell with Prepare selected and slice/export actions in the top-right header. | Current runtime has a wider OWzx navigation shell and a separate floating viewport Slice button. | `src/qml_gui/BBLTopbar.qml`; `src/qml_gui/components/GLToolbars.qml`; `src/qml_gui/pages/PreparePage.qml` | `Plater.cpp` action button and main frame wiring | Action placement and shell density do not match target. | High | Phase 76 | STATUS-01, VIEWUI-01 | Runtime screenshot plus QML source audit for slice/export placement. |
| PREP-SIDEBAR | Narrow, compact, mostly flat sidebar. Printer/filament/process sections are dense and target-localized. | Current sidebar is 390px wide by default, card-heavy, dark navy, and visibly wider than target. | `PreparePage.qml:20-23`; `DockableSidebar.qml`; `LeftSidebar.qml:52-421` | `Plater.cpp:480-556`, `Plater.cpp:1730-2178`, `PresetComboBoxes.cpp` | Width, density, card radius, visual hierarchy, and section spacing differ from target. | Critical | Phase 75 | SIDE-01 | Screenshot diff and source audit for sidebar width/radius/spacing. |
| PREP-SIDEBAR-OPTIONS | Process option rows show localized user-facing labels and compact numeric controls. | Current screenshot still shows English/internal labels such as `Layer height`, `print`, and `default`, plus oversized empty value boxes. | `LeftSidebar.qml:428-556`; `ConfigViewModel.*`; `ConfigOptionModel.*` | `Tab.cpp`; `ConfigManipulation.*`; `PrintConfig.*` | Raw/internal labels and option-row density block screenshot parity. | Critical | Phase 75 | SIDE-02, SIDE-03 | QML/model audit proving display names are upstream/user-facing and placeholder rows are gone. |
| PREP-FILAMENT | Filament row is compact, with slot number, preset name, edit/status affordances, and target-like colors. | Current runtime shows a tall colored/outlined filament block with a blank-looking combo area. | `LeftSidebar.qml:178-260`; `FilamentSlot.qml` if used; `ConfigViewModel.*` | `Plater.cpp:2051-2178`, `Plater.cpp:2324-2421` | Filament row height, content density, and preset/status presentation are off-target. | High | Phase 75 | SIDE-01, SIDE-03 | Runtime state screenshot with one filament and source audit of slot controls. |
| PREP-OBJLIST | Object list is an upstream-style object/volume tree with correct selected, empty, context, drag, and disabled states. | Current `ObjectList.qml` has a custom rounded toolbar/chip UI and many context actions requiring dead/no-op verification. | `LeftSidebar.qml:577-688`; `ObjectList.qml` | `GUI_ObjectList.cpp:135-344`, `GUI_ObjectList.cpp:1478-1620`, `GUI_ObjectList.cpp:1844-1991` | Layout and action truth need restoration; visible context entries must be audited for real backing. | High | Phase 76 | OBJ-01, CLEAN-01 | Source audit of context entries plus runtime empty/selected object screenshots. |
| PREP-VIEWPORT | Grey workspace with target bed/model presentation as the visual focus. | Current runtime is very dark, grid-only in the baseline evidence, and lacks the target bed frame/material appearance. | `PreparePage.qml:1593-1631`; `RhiViewport*`; `PrepareSceneData.*` | `GLCanvas3D.cpp`; `Plater.cpp` PartPlate and bed rendering | Viewport palette, bed style, model/bed framing, and toolbar interference differ from target. | High | Phase 77 | VIEWUI-01 | Runtime screenshot after fixture load and RHI source audit. |
| PREP-VTOOLBAR | Compact icon toolbar near the top/viewport and view controls near the bed, matching target placement. | Current `GLToolbars.qml` uses text labels (`+`, `P+`, `O`, `A`, `M`, `R`, etc.), a top-left main toolbar, a left vertical gizmo bar, and a far-right view bar. | `GLToolbars.qml:35-344` | `GLCanvas3D.cpp:10548-10589`; `Gizmos/*` | Placement, icon language, grouping, and visual weight differ from target. | Critical | Phase 77 | VIEWUI-01, GIZMOUI-01 | QML audit for icon-first controls and runtime screenshot overlay check. |
| PREP-GIZMOFLOAT | Gizmo panels appear only in valid selection/tool states and do not overlap the core workspace controls. | v3.8 made RHI gizmos functional, but Phase 74 has no accepted screenshot evidence for final floating panel placement. | `PreparePage.qml` inline gizmo panels; `GLToolbars.qml`; `EditorViewModel.*`; `RhiViewportRenderer.*` | `Gizmos/GLGizmo*.cpp`; `GLCanvas3D.cpp` gizmo manager | Functional parity exists for core RHI gizmos, but final panel visibility/placement remains unproven. | High | Phase 77 | GIZMOUI-01 | Runtime screenshots for move/rotate/scale/cut/wipe-tower states. |
| PREP-PLATEBAR | Plate indicator/strip aligns with target bed-adjacent/bottom workflow and reflects active plate state. | Current `PreparePage.qml` owns a horizontal plate bar, but current visual evidence does not match the target's bed-adjacent plate marker and compact layout. | `PreparePage.qml:3009-3166`; `EditorViewModel.*`; `PartPlateList` | `Plater.cpp` PartPlateList; `GLCanvas3D.cpp` plate handling | Plate UI placement and compactness need runtime proof and likely adjustment. | Medium | Phase 76 | PLATEUI-01 | Runtime screenshot with multiple plates plus source audit of add/select/status states. |
| PREP-SLICESTATUS | Slice/export readiness is honest and placed with the target top/right action surfaces and notifications. | Current runtime has a large green floating `>> Slice` button in the viewport and status/footer text that does not match target placement. | `GLToolbars.qml:301-344`; `SliceProgress.qml`; `BBLTopbar.qml`; `BackendContext.*` | `Plater.cpp` background process/action buttons; `GLCanvas3D.cpp` warnings | Slice action placement is a visible mismatch; readiness and cancellation states need source audit. | High | Phase 76 | STATUS-01 | Runtime screenshots for unavailable/available/slicing states and source audit of gates. |
| PREP-VIEWOPTS | View orientation controls match the target compact viewport controls. | Current runtime shows a right-edge `T/F/R/I/Z` vertical bar and separate left gizmo bar, not the target placement/visual language. | `GLToolbars.qml:251-344`; `PreparePage.qml` view overlays | `GLCanvas3D.cpp` view toolbar | View control placement and labels/icons do not match target. | High | Phase 77 | VIEWUI-01 | QML audit and runtime screenshot after viewport control interaction. |

## v3.7 Residual Reconciliation

| Residual From v3.7 | v3.9 Owner | Status In This Audit |
|---|---|---|
| Config option display names still mostly English/internal in sidebar/settings. | Phase 75 | Promoted to Critical sidebar option-label gap. |
| Runtime Preview screenshot not reliable. | Future | Out of v3.9 Prepare scope unless it blocks Prepare. |
| QML startup layout warnings. | Phase 78 | Verification/cleanup concern; downstream phases must not add new layout warnings. |
| Prepare left sidebar structural changes passed, but not screenshot-level. | Phase 75 | Reopened as visual-density and label parity work. |
| Phase 68 visual evidence deferred for RHI gizmos. | Phase 77/78 | Reopened as gizmo overlay placement/runtime evidence work. |

## Requirement Coverage

| Requirement | Covered By |
|---|---|
| AUDIT-01 | This matrix maps all Prepare regions to source, Qt targets, owner phases, and verification. |
| SIDE-01 | PREP-SIDEBAR and PREP-FILAMENT rows. |
| SIDE-02 | PREP-SIDEBAR-OPTIONS row. |
| SIDE-03 | PREP-SIDEBAR, PREP-SIDEBAR-OPTIONS, PREP-FILAMENT rows. |
| OBJ-01 | PREP-OBJLIST row. |
| PLATEUI-01 | PREP-PLATEBAR row. |
| STATUS-01 | PREP-TOP and PREP-SLICESTATUS rows. |
| VIEWUI-01 | PREP-VIEWPORT, PREP-VTOOLBAR, PREP-VIEWOPTS rows. |
| GIZMOUI-01 | PREP-VTOOLBAR and PREP-GIZMOFLOAT rows. |
| CLEAN-01 | PREP-OBJLIST and downstream replacement/removal checks. |
| VERIFY-01 | Verification methods recorded in every row. |
| VERIFY-02 | Deferred to Phase 78 final runtime build and visual evidence. |

## Phase Routing

| Phase | Work To Start From This Audit |
|---|---|
| 75 | Sidebar width/density, printer/filament/process presentation, localized option display names, placeholder removal. |
| 76 | Top action placement, object list, plate bar, slice/status readiness and placement. |
| 77 | RHI viewport presentation, toolbar placement/icon language, view controls, gizmo floating panel evidence. |
| 78 | Deprecated path cleanup, source/QML audits, canonical build, app launch, and final Prepare screenshot evidence. |
