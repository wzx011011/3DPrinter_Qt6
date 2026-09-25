# Phase 53: Prepare Object, Plate, and Viewport Workflow - Context

**Gathered:** 2026-07-01
**Status:** Ready for planning
**Mode:** Autonomous smart discuss, using prior user decisions as locked defaults

<domain>
## Phase Boundary

Restore the Prepare workspace surface that connects imported models/projects to
the object list, plate bar, viewport controls, and supported gizmo entry points.
This phase covers Prepare-page object and plate operations, camera/view controls,
tool gating, and renderer-stability checks. It does not implement Preview-page
layout or G-code export; those remain assigned to later v3.6 phases.

</domain>

<decisions>
## Implementation Decisions

### Completeness And Legacy Handling
- If an existing Prepare UI component is visibly off-design, semantically wrong,
  mock-only, or corrupted beyond safe repair, replace it and remove the old
  route/import/resource/test references in the same phase.
- Preserve existing C++ services/viewmodels when they already implement
  OrcaSlicer behavior; otherwise extend or replace them rather than adding a
  parallel compatibility path.
- No new placeholder behavior may be presented as complete. Unsupported actions
  must be disabled, hidden, or marked with a clear status classification.
- Source comments added or changed during this phase must be English ASCII only.

### Object Workflow
- Object list and context-menu behavior follows OrcaSlicer `GUI_ObjectList.cpp`
  and `Plater.cpp`: select, multi-select, rename, duplicate, delete,
  printable/unprintable, add volume, change volume type, split, fix mesh, export,
  and instance-to-object actions.
- QML may present menus, inline rename fields, and compact status chips, but the
  durable behavior and state checks must live in `EditorViewModel` /
  `ProjectServiceMock`.
- Volume inspection is in scope only where the current C++ model exposes real or
  hybrid data. Missing operations are gated instead of simulated.
- Existing object/volume APIs are reused when correct; missing enable-state
  properties can be added to the viewmodel for honest QML gating.

### Plate Workflow
- Plate switching and management follows OrcaSlicer `PartPlateList` behavior:
  create, select, rename, clone, delete, reorder, lock/unlock, printable state,
  clear/select all on plate, and move selected object to a target plate.
- The bottom plate bar remains the screenshot-aligned primary plate surface for
  Prepare. It must not overlap object-info banners, slice controls, or viewport
  interaction hot zones.
- Plate thumbnails and slice-result status are kept, but any stale or missing
  result state must be explicit through the existing `plateSliceResultStatus`
  contract.

### Viewport And Gizmos
- View/camera controls follow OrcaSlicer `GLCanvas3D`: fit/zoom to bed,
  selection/volume fit where available, view presets, mouse orbit/pan/zoom, and
  active plate rendering.
- Vertical tool buttons must be backed by real renderer/viewmodel behavior or
  explicitly gated. Move, rotate, scale, flatten, cut, seam/support paint,
  measure, simplify, text/SVG/emboss, and mesh boolean need per-tool status.
- VDB/OpenVDB-dependent features remain blocked in this Windows build and must
  not look like working features.
- Renderer stability is a phase requirement: selection, camera movement, tool
  switching, plate switching, and Prepare/Preview return must keep the viewport
  visible and avoid stale selected/tool state.

### the agent's Discretion
- The exact QML component split is at implementation discretion, but reusable
  controls/components should replace long inline blocks when doing so reduces
  risk and removes corrupted legacy markup.
- Tests can combine C++ viewmodel regression checks with QML source audits and
  renderer-path smoke checks; manual visual validation remains required for
  screenshot parity.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/core/viewmodels/EditorViewModel.{h,cpp}` already exposes most object,
  volume, plate, arrange, slice, and bed-state APIs used by Prepare QML.
- `src/core/services/ProjectServiceMock.{h,cpp}` owns object arrays, volume
  entries, plate membership, PartPlateList integration, and object transforms.
- `src/qml_gui/pages/PreparePage.qml` owns the central viewport, context menus,
  plate bar, floating gizmo panels, import/export dialogs, and GLViewport
  binding.
- `src/qml_gui/panels/ObjectList.qml` owns the object tree/list and row context
  menus.
- `src/qml_gui/components/GLToolbars.qml` owns the viewport overlay toolbars.
- `src/qml_gui/Renderer/RhiViewport`, `SoftwareViewport`, and `GLViewport`
  provide the QML-compatible viewport surface.
- `tests/ViewModelSmokeTests.cpp` and `tests/QmlUiAuditTests.cpp` are the
  existing regression targets for viewmodel behavior and QML wiring.

### Established Patterns
- QML binds to `backend.editorViewModel` and calls `Q_INVOKABLE` methods; it does
  not own business behavior.
- Existing tests prefer targeted smoke checks and source audits over fragile
  pixel tests when no stable UI automation harness exists.
- Slice/export gating now flows through C++ readiness properties added in
  Phases 51 and 52.
- Renderer object membership must use C++-provided `activePlateObjectIndices`
  and `meshBatchSourceObjectIndices`, not QML filtering.

### Integration Points
- Import/project load enters through `EditorViewModel::loadFile`.
- Object actions enter through `EditorViewModel` object/volume invokables.
- Plate actions enter through `EditorViewModel` plate invokables and
  `ProjectServiceMock` PartPlateList methods.
- Viewport controls enter through the registered `GLViewport` QML type and its
  `requestViewPreset`, `requestFitView`, and gizmo-mode APIs.

</code_context>

<specifics>
## Specific Ideas

- Use screenshots in `shotScreen/` as visual truth and OrcaSlicer source as
  behavior truth.
- Phase 50 inventory maps relevant regions:
  `PREP-OBJLIST`, `PREP-VIEWPORT`, `PREP-VTOOLBAR`, `PREP-GIZMOFLOAT`,
  `PREP-PLATEBAR`, and `PREP-VIEWOPTS`.
- User preference is complete implementation over MVP behavior; old wrong code
  should be removed rather than preserved.

</specifics>

<deferred>
## Deferred Ideas

- Preview layout, layer sliders, and G-code view panels are Phase 54.
- Full OpenVDB-dependent gizmo behavior remains blocked by dependency status.
- Full live screenshot parity still requires manual visual validation after
  automated checks pass.

</deferred>
