---
quick_id: 260722-rds
slug: p12-3-complete-prepare-viewport-model-and-plate-context-workflow
date: 2026-07-22
status: completed
mode: quick
source_truth: third_party/OrcaSlicer@4cb3b9ce792f15c38fabfb4bb9700895d32b1166
requires:
  - P12.1 local upstream baseline and its two approved compatibility deltas
provides:
  - Pointer-resolved Prepare right-click context workflow for object, part, plate, and blank targets
  - Source-truth menu families with C++ capability and action ownership
  - Regression coverage for hit metadata, selection synchronization, suppression, and menu routing
affects: [prepare-viewport, object-selection, part-plate, context-menus, exporter]
---

# Quick Task 260722-rds: P12.3 Complete Prepare Viewport Model and Plate Context Workflow

## Goal

Complete tracker item P12.3 without inventing product behavior: right-clicking the Prepare viewport must resolve the pointer target in C++, synchronize selection before a menu is chosen, suppress menus for the upstream-owned input cases, and expose every applicable source-truth menu action through a real C++ path.

The task is complete only when object, instance/part/text/SVG, multi-selection, plate, and blank-canvas menu families are selected from the actual hit result and each visible item is either backed by a real capability/action or absent. Do not retain the current selection-only QML dispatch or placeholder action paths.

## Source Truth And Coverage

| Source item | Required Qt6 outcome | Planned owner |
| --- | --- | --- |
| `GLCanvas3D.cpp:4698-4756` | On right-button release, resolve object/plate/empty hit; object selection is synchronized before menu dispatch; blank clears selection; no menu for drag, layer editing, wipe tower, or captured gizmo input. | Tasks 1 and 3 |
| `Selection.cpp:157-223` | A hit already included in a multi-selection keeps that selection; a newly hit object becomes the sole selection. | Task 1 |
| `Plater.cpp:11529-11597` | Route to default, object/instance, part/text/SVG, multi, or plate menu only after synchronized selection. | Tasks 1 and 3 |
| `GUI_Factories.cpp:1390-1523` | Restore default and object/part menu entries with capability predicates and non-placeholder actions. | Tasks 2 and 3 |
| `GUI_Factories.cpp:1702-1804` | Restore plate-scoped commands, including current-plate selection before mutation. | Tasks 2 and 3 |
| `GUI_Factories.cpp:1905-1988` | Restore multi-selection object/part distinctions and their real actions. | Tasks 2 and 3 |
| `GUI_Factories.cpp:2013` and `EditorViewModel.cpp:5550` | Replace the fixed 3x3 fill-bed behavior with source-truth instance-count and arrangement actions. | Tasks 2 and 3 |
| `PreparePage.qml:3865` | Plate-card right click must synchronize the ViewModel's selected plate before the shared plate menu opens. | Task 3 |
| P12.3 tracker entry | Cover pointer hit, object/plate/blank synchronization, and drag/gizmo/layer/wipe-tower suppression. | Tasks 1 through 3 |
| `260721-jjs-RESEARCH.md` findings 1-6 | Close all identified Qt gaps, including metadata, fallback support, menu divergence, and action coverage. | Tasks 1 through 3 |

## Non-Negotiable Boundaries

- Do not edit anything under `third_party/OrcaSlicer`, its gitlink, `AGENTS.md`, baseline/tracker documents, or the `260721-jjs` quick-task artifacts. P12.3 consumes the local P12.1 baseline; it does not update it.
- Preserve unrelated dirty working-tree changes. `src/core/services/ProjectServiceMock.cpp` and `tests/ViewModelSmokeTests.cpp` are task-owned but already dirty: inspect and integrate with their existing hunks; never reset, reformat, or overwrite user work outside this task's additions. Do not touch the other dirty docs, logs, scripts, `.planning/debug/`, or `BBLTopbar.qml`.
- Keep all business rules, hit testing, selection decisions, capability checks, file-path validation, and action execution in C++. QML may bind state, open dialogs, and display the menu family returned by C++.
- New/modified source comments must be English and ASCII-only. Preserve UTF-8 without BOM.
- Do not add dependencies, alternate build directories, alternate build scripts, or new mock/no-op menu actions.

## Interface Contracts

Implement the following contracts before wiring menus. Names may follow local conventions, but their data and behavior are fixed.

1. `ViewportContextHit` is a shared C++ value contract used by RHI and software viewports. It carries: target kind (`Object`, `Part`, `Plate`, `Empty`, `Suppressed`), source-object index, volume index, instance index, plate index, and popup screen position. `Suppressed` is never forwarded to QML as a menu request.
2. Mesh batch metadata remains aligned one-to-one with the packed mesh batches. Add source object, volume, and instance identities together; reject inconsistent arrays rather than guessing a target. `ObjectPicking` returns the nearest complete hit record, not only a source-object integer.
3. `EditorViewModel` owns a single context-selection synchronization entry point and explicit instance-context state. Object/part hits use source indices only after validating current-plate membership; a target already selected preserves the full multi-selection; a newly hit target becomes the primary selection. A nonzero instance hit records its real instance index and resolves the instance menu/action context instead of silently degrading to object 0. A plate hit selects that plate and clears object selection only when there is no object/part hit; empty clears selection. The entry point returns the resolved menu family/capability context for QML.
4. Context suppression is input ownership, not a `gizmoMode` enum check. A normal selected Move/Rotate/Scale tool must still permit a context menu. Suppress only an actual gesture captured by a gizmo/tool, a right-button motion beyond the drag threshold, active layer-editing input, or a wipe-tower hit. This maps the upstream `Undefined`/consumed-input gate without treating Qt's default `GizmoMove` as an active capture.
5. Both `RhiViewport` and the `SoftwareViewport` fallback expose the same context-request signal and resolve the same typed contract. The software fallback must use the shared CPU hit path; it must not fall back to QML's stale-selection menu.

## Ordered Tasks

### Task 1: Establish typed pointer-hit and selection synchronization contracts

**Depends on:** P12.1 local baseline only.

**Files:**

- `CMakeLists.txt`
- `src/qml_gui/Renderer/ViewportContextHit.h` (new)
- `src/qml_gui/Renderer/PrepareSceneData.h`
- `src/qml_gui/Renderer/PrepareSceneData.cpp`
- `src/core/rendering/ObjectPicking.h`
- `src/core/rendering/ObjectPicking.cpp`
- `src/core/services/ProjectServiceMock.h`
- `src/core/services/ProjectServiceMock.cpp`
- `src/core/viewmodels/EditorViewModel.h`
- `src/core/viewmodels/EditorViewModel.cpp`
- `src/qml_gui/Renderer/RhiViewport.h`
- `src/qml_gui/Renderer/RhiViewport.cpp`
- `src/qml_gui/Renderer/SoftwareViewport.h`
- `src/qml_gui/Renderer/SoftwareViewport.cpp`
- `tests/ObjectPickingTests.cpp`
- `tests/PrepareSceneDataTests.cpp`
- `tests/ViewportContextMenuTests.cpp` (new)
- `tests/ViewModelSmokeTests.cpp`

**Test-first behavior:**

- Extend `ObjectPickingTests` to prove nearest hit returns source object, volume, and instance identities, and rejects malformed or misaligned metadata.
- Extend `PrepareSceneDataTests` to prove the current plate footprint accepts its valid rectangular/circular bed point and rejects off-bed points; invalid plate context never reports a plate hit.
- Add `ViewModelSmokeTests` coverage for context synchronization: (a) a newly hit object replaces selection, (b) a hit already in a multi-selection preserves every selected source index, (c) a nonzero instance hit records that actual instance and selects/enables actions for that instance rather than instance 0, (d) an object outside the active plate is rejected without a stale menu state, (e) a plate hit switches `currentPlateIndex` and clears object selection only when no object was hit, and (f) blank clears selection.
- Add `ViewportContextMenuTests` as a behavioral event test, linked to the real viewport classes. Use test subclasses to send right-button press/move/release events and `QSignalSpy` to inspect the actual context-request signal from both `RhiViewport` and `SoftwareViewport`, without creating a render pass. For each implementation assert object/plate/empty classification and exact popup position, then assert no request for right-drag threshold crossing, actual captured gizmo/tool input, active layer-editing input, and wipe-tower hit. Include the regression case that normal `GizmoMove` selection alone still emits the valid context request.

**Implementation:**

1. Carry per-batch volume and instance identities from `ProjectServiceMock::meshData()`'s existing per-volume/per-instance emission through `EditorViewModel`, the QML bindings, `PrepareSceneData::ModelBatch`, and both viewport implementations. Do not encode metadata into synthetic IDs or inspect QML list state; maintain length alignment with `meshBatchSourceObjectIndices` and invalidate the complete batch set on malformed data.
2. Upgrade `ObjectPicking` from an integer return to a typed nearest-hit result with the metadata above and the hit position/distance needed for target classification. Keep its CPU ray/AABB/triangle path deterministic.
3. Add exact bed/plate point-in-shape resolution to `PrepareSceneData`, using the same current-plate dimensions/origin/shape as rendering. A model hit wins over a plate hit; a point off the current plate is `Empty`.
4. In both viewport implementations, own the right-button press/move/release gesture. On release, classify `Object`/`Part`/`Plate`/`Empty`, apply the real suppression conditions, and emit one typed context request with the release coordinates as popup coordinates. Preserve regular object left-click and transform behavior. Do not suppress merely because `gizmoMode` is `GizmoMove`, `GizmoRotate`, or `GizmoScale`; track whether the current pointer sequence was actually consumed by tool input or exceeded the right-drag threshold. Expose the layer-editing input-active state as a C++-owned property/binding and test it through the event path, not by checking source text.
5. Add the `EditorViewModel` synchronization API, instance-context state/accessors, and context-family result. It must make the selection mutation before QML receives a menu choice, use real source/volume/instance indices, retain an already-contained multi-selection as `Selection::add(..., true, true)` does upstream, and centralize the plate/empty clearing semantics.
6. Include the new shared header in the explicit CMake source manifest. Do not alter the upstream submodule or the existing packed-mesh protocol except for the explicit parallel metadata contract.

**Done:** The active renderer and software fallback can emit only a fully classified, validated context hit; direct event tests prove release classification, popup placement, right-drag/input-capture/layer/wipe suppression, and the no-default-gizmo-suppression rule. C++ selection tests prove nearest metadata, nonzero instance context, plate membership, multi-selection preservation, and object/plate/blank synchronization.

### Task 2: Complete C++ menu capabilities and source-truth action slices

**Depends on:** Task 1's typed hit/context selection contract.

**Files:**

- `CMakeLists.txt`
- `src/core/services/ProjectServiceMock.h`
- `src/core/services/ProjectServiceMock.cpp`
- `src/core/viewmodels/EditorViewModel.h`
- `src/core/viewmodels/EditorViewModel.cpp`
- `tests/ViewModelSmokeTests.cpp`

**Test-first behavior:**

- Add focused smoke slots for every new action family. Each slot must assert a real model/plate state change or a real exported artifact, not a status string.
- Export tests must write to `QTemporaryDir`: selected STL output contains data, selected DRC uses `Slic3r::store_drc`, and multi-object one-file versus per-object export follows the selected source indices.
- Action tests must cover both split targets, instance add/remove/instance-to-object, setting an explicit nonzero instance count, and filling a bed through real arrangement rather than a fixed 3x3 clone loop. They must also cover drop/auto-drop, process-settings copy/paste, unit conversion, replace-all, selected/plate-scoped reload, add/paste/arrange for an explicit target plate, plate-targeted `Replace all with 3D files`, and handy-model loading from the deployed resource path. Guard unavailable source files with the same real capability predicate used by the menu; do not report success for a fake operation.

**Implementation:**

1. Make `ProjectServiceMock` and `EditorViewModel` the sole authority for menu availability and execution. Add context-aware capability queries and actions for object, part/text/SVG, multi-object, plate, and default families; all indices and file paths must be bounds-checked before mutation.
2. Restore the source-truth actions identified in `GUI_Factories.cpp`, using current upstream/libslic3r APIs rather than status-only stubs: set number of instances; increase/decrease instances and instance-to-object; fill bed with instances using real available-bed/arrangement semantics instead of `EditorViewModel.cpp:5550`'s fixed 3x3 mock loop; clone/merge; split-to-objects and split-to-parts as distinct operations; center/drop/auto-drop; mesh repair/simplify/subdivision where source-truth capability exists; mirror; printable/visibility; per-object process/settings copy-paste; unit conversion; reload/replace/replace-all; and real selected/multi STL and DRC export. Use `Slic3r::store_drc` for DRC rather than treating a notification as an export.
3. Restore default/plate actions with explicit target-plate parameters, so a right-clicked plate never mutates whichever plate happened to be current: select current/all plates, delete current plate contents, arrange/auto-orient/reload a target plate, add models/primitives/handy models to that plate, paste to that plate, replace all parts on that target plate with selected 3D files, and delete/clone/reorder/lock/rename it. Update selection and slice invalidation after every successful mutation.
4. Mirror upstream `append_submenu_add_handy_model` table/data semantics and package the required `third_party/OrcaSlicer/resources/handy_models` assets for runtime lookup through CMake. A missing packaged model must produce a failed capability/action with an actionable error, never an inert menu item.
5. Keep dialogs and file pickers out of the service layer: C++ validates the path and executes the operation; QML supplies a user-selected path. Do not create a parallel mock implementation for any action.

**Done:** Every required backend menu action has a capability predicate and a real mutation/import/export path verified by focused smoke tests. Plate-scoped actions receive their target plate explicitly, and no action is implemented as a log message, timeout, or fake success.

### Task 3: Route all QML menu families through the C++ context contract and lock the workflow

**Depends on:** Tasks 1 and 2.

**Files:**

- `src/qml_gui/components/PrepareContextMenus.qml` (new)
- `src/qml_gui/qml.qrc`
- `src/qml_gui/pages/PreparePage.qml`
- `src/qml_gui/panels/ObjectList.qml`
- `tests/QmlUiAuditTests.cpp`
- `tests/ViewModelSmokeTests.cpp`

**Test-first behavior:**

- Add a `QmlUiAuditTests` source-audit slot that requires the typed viewport signal and `EditorViewModel` context synchronizer, rejects the stale `MouseArea` selection-only dispatch, and asserts that `PreparePage` and `ObjectList` consume the same C++ capability/action surface.
- Assert all five presentation families are present and routed: default; object/instance; part/text/SVG; multi-selection; and plate. Assert the specialized split-to-objects and split-to-parts entries call different C++ actions.
- Extend the smoke test with a menu-context sequence that proves a plate-card right-click synchronizes the actual selected plate before an arrange/reload/paste/replace-all action, and that suppressed hits do not mutate selection or emit a menu request.

**Implementation:**

1. Extract the menu presentation into `PrepareContextMenus.qml` so the viewport and ObjectList cannot drift. It may choose which already-resolved family to show and open file/confirmation dialogs, but it must contain no picking, selection mutation, capability inference, or durable action logic.
2. Replace `PreparePage`'s full-screen right-click `MouseArea` with the typed C++ context signal. Forward the hit directly to the ViewModel synchronizer, then ask the shared component to display only the returned menu family at the supplied popup position. Route the existing plate-card right-click at `PreparePage.qml:3865` through the same `Plate` synchronization path before `PrepareContextMenus` opens; assigning `contextPlateIndex` alone is insufficient. Do not check `selectedObjectIndex` before `selectedObjectCount`; the prior ordering made the multi-selection menu unreachable.
3. Bind each menu item `enabled` state to a C++ capability and each trigger to a real C++ action or an explicit QML file/dialog handoff into one. Add the source-truth Set Number of Instances control (numeric input passed to the C++ setter), Fill Bed with Instances action, and plate-targeted Replace All with 3D Files handoff. Include all applicable source-truth entries described in Task 2; remove any legacy entry that cannot meet this contract instead of leaving a visible no-op.
4. Rewire ObjectList's existing object/volume menus to the same C++ action surface and correct the current defect where both split variants invoke split-to-objects. Use the typed volume metadata to select the part/text/SVG family rather than treating every right-click as an object.
5. Register the new component in `qml.qrc`, keep all user-visible text in `qsTr()`, and update no unrelated QML pages.

**Done:** QML is presentation-only; menu family and action availability come from C++. Right-clicking object, part/text/SVG, plate, multi-selection, and blank canvas presents the correct upstream family after C++ synchronization, while captured/drag/layer/wipe-tower interactions present none.

## Required Verification

Run focused tests after the canonical configure/build has produced them, then run the one permitted full verification command:

```powershell
build\ObjectPickingTests.exe
build\PrepareSceneDataTests.exe
build\ViewModelSmokeTests.exe rendererPickingSelectsSourceObjectThroughEditorViewModel
build\ViewModelSmokeTests.exe viewportContextSelectionSynchronizesBeforeMenuRouting
build\ViewModelSmokeTests.exe prepareContextMenuActionsAreRealAndPlateScoped
build\ViewportContextMenuTests.exe
build\QmlUiAuditTests.exe prepareViewportContextMenuWorkflowIsCppOwned
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Also run the UTF-8 guard only for task-owned modified text files, then inspect the scoped diff for whitespace errors:

```powershell
python $env:USERPROFILE\.coding-encoding-guard\encoding_guard.py CMakeLists.txt src\core\services\ProjectServiceMock.h src\core\services\ProjectServiceMock.cpp src\core\viewmodels\EditorViewModel.h src\core\viewmodels\EditorViewModel.cpp src\core\rendering\ObjectPicking.h src\core\rendering\ObjectPicking.cpp src\qml_gui\Renderer\PrepareSceneData.h src\qml_gui\Renderer\PrepareSceneData.cpp src\qml_gui\Renderer\ViewportContextHit.h src\qml_gui\Renderer\RhiViewport.h src\qml_gui\Renderer\RhiViewport.cpp src\qml_gui\Renderer\SoftwareViewport.h src\qml_gui\Renderer\SoftwareViewport.cpp src\qml_gui\components\PrepareContextMenus.qml src\qml_gui\pages\PreparePage.qml src\qml_gui\panels\ObjectList.qml src\qml_gui\qml.qrc tests\ObjectPickingTests.cpp tests\PrepareSceneDataTests.cpp tests\ViewportContextMenuTests.cpp tests\ViewModelSmokeTests.cpp tests\QmlUiAuditTests.cpp
git diff --check -- CMakeLists.txt src/core/services/ProjectServiceMock.h src/core/services/ProjectServiceMock.cpp src/core/viewmodels/EditorViewModel.h src/core/viewmodels/EditorViewModel.cpp src/core/rendering/ObjectPicking.h src/core/rendering/ObjectPicking.cpp src/qml_gui/Renderer src/qml_gui/components/PrepareContextMenus.qml src/qml_gui/pages/PreparePage.qml src/qml_gui/panels/ObjectList.qml src/qml_gui/qml.qrc tests
```

The canonical verifier is the authority for configure, build, application smoke, E2E, all CTest groups, QML warnings, and whether `ViewModelSmokeTests.exe` was actually executed. Report each outcome separately; do not claim full verification from a targeted test run.

## Must-Haves

- A right-click target is calculated from the viewport pointer, never inferred from stale QML selection.
- Object, nonzero instance, volume, current plate, and blank hits use real source indices and current renderer geometry; no synthetic wipe-tower selection ID exists.
- A context click on an already-selected object keeps a multi-selection intact; a different hit selects only that target; a plate or blank click follows the upstream clearing rules.
- Normal Move/Rotate/Scale tool selection still permits the context menu. Direct viewport event tests prove that only actual gesture ownership, right drag, layer editing, or a wipe-tower hit suppresses it and that a valid right release uses the release point for popup placement.
- `RhiViewport` and the software fallback expose the same behavior, not divergent QML fallbacks.
- Default, object/instance, part/text/SVG, multi, and plate menus cover the current upstream action families and only invoke real C++ capabilities/actions.
- Plate menu actions, including Replace All with 3D Files, operate on the right-clicked plate, even when it was not current before the click.
- No upstream file, baseline document/gitlink, or unrelated dirty file is changed.
