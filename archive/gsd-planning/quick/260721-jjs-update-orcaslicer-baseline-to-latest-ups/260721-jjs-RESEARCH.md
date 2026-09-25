---
quick_task: 260721-jjs
status: complete
date: 2026-07-21
---

# Research: latest upstream and Prepare context menus

## Upstream baseline

- Official `upstream/main` fetched at `8b93cc5df3347c657ce6ac9a58f6923a21c2959b` on 2026-07-21.
- Latest stable tag is `v2.4.2` at `8500fcdccaa10b5099ac20d252af3a7c560046f1`.
- The project currently pins `third_party/OrcaSlicer` to local branch commit `edbca0aa55db88af88b8469cdcc16435b6fcfe49`.
- Of the ten local commits after the grafted upstream snapshot, six source deltas are now upstream-equivalent and two are broad snapshot-sync commits that must not be replayed.
- Preserve two load-bearing OWzx deltas: the CGAL 5.4 MeshBoolean compatibility patch and commit `2976d14778`'s null-`keys_map` guards in `Config.hpp` used by the G-code `append_full_config` path.
- Update the submodule onto `upstream/main`, then reapply only those two reviewed compatibility deltas. Record the resulting exact commit in `docs/源码真值基线.md`; do not keep the stale `v7.0.1` metadata.

## Upstream interaction contract

The live upstream right-click behavior remains centered in:

- `src/slic3r/GUI/GLCanvas3D.cpp`: refresh hover on right-button release, select the hit volume, select a hit plate, clear selection on empty space, suppress menus for drags, active gizmos, layer editing, and wipe tower hits.
- `src/slic3r/GUI/Plater.cpp`: resolve default, object/instance, part/text/SVG, multi-selection, and plate menu families after selection synchronization.
- `src/slic3r/GUI/GUI_Factories.cpp`: build dynamic menu items and capability predicates.

The latest upstream changes after the current imported snapshot do not materially change this contract; intervening changes to these files are unrelated FBO/theme maintenance.

## Qt6 root gaps

1. `RhiViewport::mousePressEvent` ignores the right button. The QML overlay opens a menu from stale ViewModel selection instead of the pointer hit.
2. `ObjectPicking` returns only a source-object index. There is no volume/instance hit metadata and no plate hit result.
3. `PreparePage.qml` checks single selection before multi-selection, making the multi menu unreachable in normal multi-select state.
4. Plate-card right-click records `contextPlateIndex` without activating that plate, so actions backed by current-plate APIs can mutate the wrong plate.
5. Context-menu suppression for active gizmos, right-drag, layer editing, and wipe tower hits is absent.
6. Viewport and ObjectList maintain divergent menus; ObjectList's volume menu maps both split variants to split-object and gates several volume actions through object-only capability checks.

## Implementation guidance

1. Add a C++ context-hit contract owned by the viewport, carrying target kind, source object index, volume/instance index when available, plate index, and screen/world position.
2. Resolve right-button release in C++ after drag/gizmo gating. Reuse object triangle picking and add bed/plate point-in-shape picking. Emit one context request signal; QML only chooses and displays the already-resolved menu family.
3. Centralize source-truth selection synchronization in `EditorViewModel`: hit object becomes primary unless already included in a multi-selection; hit plate becomes current and clears object selection when no object was hit; empty clears selection.
4. Make menu actions and enabled states C++ capabilities. Restore object, part/text/SVG, multi, plate, and default menu families without durable business rules in QML.
5. Complete backend gaps that are directly exposed by these menus: instance lifecycle, split-to-parts, drop/auto-drop, process-settings copy/paste, unit conversion, replace-all, real STL/DRC export, handy models, and plate-scoped arrange/reload/add/paste.
6. Add deterministic C++ tests for hit resolution/selection/gating plus QML audit/runtime tests for menu routing. Run the canonical verification command and the encoding guard.

## Risks

- Updating the submodule and implementation in one task crosses a submodule boundary; keep the submodule update as its own commit before Qt changes.
- The software viewport lacks equivalent object picking and needs either the same CPU resolver or an explicit capability-disabled path.
- Plate picking is only correct when renderer plate layout and plate IDs share the same source as rendering; do not infer a plate from QML card state.
