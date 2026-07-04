# Roadmap: OWzx Slicer

## Milestones

- Complete: **v2.9** - Phases 10-15 (shipped 2026-06-25)
- Complete: **v3.0 PartPlate Core** - Phases 16-22 (shipped 2026-06-26)
- Complete: **v3.1 QRhi Rendering** - Phases 23-28 (shipped 2026-06-28)
- Complete with tech debt: **v3.2 Multi-Plate Data Polish** - Phases 29-32 (audited 2026-06-28)
- Complete at MVP level, superseded for completeness: **v3.3 Slice Preview Main Flow MVP** - Phases 33-36
- Automated verification passed, manual UAT deferred: **v3.4 Import to G-code Complete Workflow** - Phases 37-43
- Superseded after Phase 46: **v3.5 Preset Authoring Complete Workflow** - Phases 44-49
- Automated verification passed, visual UAT not closed: **v3.6 Screenshot-Driven OrcaSlicer UI Restoration** - Phases 50-58 (2026-07-03; VERIFY-04 was not executed)
- Complete with residual gaps (carried to v3.8): **v3.7 Screenshot-Level UI Parity Closure** - Phases 59-64 (2026-07-04; RHI gizmo silent-dead and D3D12 segfault carried forward)

## Active Milestone: v3.8 RHI Gizmo Parity

**Goal:** Port the gizmo system (move/rotate/scale/cut), wipe tower, cut plane,
and precise object picking from the GLViewportRenderer (OpenGL path) to the
RhiViewportRenderer (D3D11 default path), so that the default RHI rendering
path is functionally complete and the GLViewport can be retired.

**Motivation:** v3.7 closed the visual parity gap but the default RHI path is
functionally incomplete — clicking Move/Rotate/Scale gizmo buttons sets the
gizmoMode property but nothing renders (RhiViewportRenderer has zero gizmo
support) and drag does nothing (no pickMoveAxis). The GLViewportRenderer
(2285 lines) is the only fully functional implementation and is gated behind
the OWZX_OPENGL=1 environment flag. This milestone makes the default path
fully interactive.

**Carry-forward from v3.7:**
- D3D12 segfault at setShaderResources (QRhi D3D12 backend deep issue; needs
  QRhi internal fix or Qt upgrade). D3D11 stays default throughout v3.8.
- v3.6 VERIFY-04 manual visual UAT (deferred since v3.6, still pending).

**Success criteria:**

- Move/Rotate/Scale gizmos render on the default D3D11 path when gizmoMode is set.
- User can drag a gizmo axis to transform the selected object (move along axis,
  rotate around axis, scale along axis) on the default path.
- Cut plane gizmo renders and is interactive (cut plane position adjustable).
- Wipe tower renders when present.
- Object picking uses ray-triangle (Möller-Trumbore) precision, matching GL path.
- GLViewportRenderer is removed; only RhiViewport + SoftwareViewport remain.
- All gizmo math (computeRay, pickMoveAxis, etc.) has unit tests.
- Build via `scripts/auto_verify_with_vcvars.ps1` passes (sanitized PATH workaround
  for the vcvars+VMware env issue is acceptable).

## Phases

- [x] **Phase 50:** Screenshot and Source-Truth Inventory (completed 2026-07-03; v3.6 closed)
- [x] **Phase 51:** Shell and Navigation Restoration (completed 2026-07-03; v3.6 closed)
- [x] **Phase 52:** Prepare Sidebar and Preset Controls (completed 2026-07-03; v3.6 closed)
- [x] **Phase 53:** Prepare Object, Plate, and Viewport Workflow (completed 2026-07-03; v3.6 closed)
- [x] **Phase 54:** Preview Layout, Sliders, and Right Panels (completed 2026-07-03; v3.6 closed)
- [x] **Phase 55:** G-code Preview Semantics and Rendering Stability (completed 2026-07-03; v3.6 closed)
- [x] **Phase 56:** Parameter Settings Dialogs Restoration (completed 2026-07-03; v3.6 closed)
- [x] **Phase 57:** Deprecated UI Removal and Architecture Cleanup (completed 2026-07-03; v3.6 closed)
- [x] **Phase 58:** End-to-End Visual and Functional Verification (completed 2026-07-03; v3.6 closed)
- [x] **Phase 65:** Gizmo math extraction + unit tests (completed 2026-07-04)
- [x] **Phase 66:** Gizmo geometry builders port (CPU vertex generation) (completed 2026-07-04)
- [x] **Phase 67:** RHI gizmo state wiring (synchronize + gizmoMode pipeline) (completed 2026-07-04)
- [x] **Phase 68:** Move gizmo RHI render (first visible gizmo) (implemented 2026-07-04; visual verification deferred to Phase 73)
- [x] **Phase 69:** Move gizmo pick + drag interaction loop (completed 2026-07-04)
- [x] **Phase 70:** Rotate + Scale gizmos (completed 2026-07-04)
- [ ] **Phase 71:** Cut plane + wipe tower
- [ ] **Phase 72:** Precise object picking (ray-triangle)
- [ ] **Phase 73:** Retire GLViewport + verification

### Phase 65: Gizmo Math Extraction + Unit Tests

**Goal:** Extract pure-math gizmo functions into a testable library with zero render dependencies, establishing a verified foundation before any rendering work.

**Requirements:** `GMATH-01`, `GMATH-02`, `GMATH-03`.

**Deliverables:**
- New class `GizmoMath` (or namespace) extracting: `computeRay`, `rayToAxisT`, `computeRotateAngle`, `pickMoveAxis`, `pickRotateAxis`, `pickScaleAxis`, `rayXZIntersect`. Inputs parameterized (no member-variable coupling).
- Unit tests covering each function (current coverage is 0).
- GLViewportRenderer updated to call the extracted functions (equivalence verified).

**Success criteria:**
1. All pure-math functions extracted and unit-tested.
2. GLViewportRenderer still works identically (calls the extracted library).
3. Ray-axis pick precision matches GL path for known test rays.

**Plans:** 1/1 plans complete

Plans:
- [x] 65-01-PLAN.md — Wave 1: extract 7 pure-math gizmo functions (computeRay, rayXZIntersect, rayToAxisT, pickMoveAxis, pickRotateAxis, pickScaleAxis, computeRotateAngle) into static `GizmoMath` class at `src/core/rendering/`, fully parameterized; GLViewportRenderer delegates with state passed as args; 15-slot `GizmoMathTests` (hand-derived expected values, all pass) (SUMMARY: 65-01-SUMMARY.md)

### Phase 66: Gizmo Geometry Builders Port

**Goal:** Port the gizmo geometry construction (move arrows, rotate torus, scale shaft+box) as pure CPU vertex generators, decoupled from any GL/RHI calls.

**Requirements:** `GGEO-01`, `GGEO-02`, `GGEO-03`.

**Deliverables:**
- `buildMoveGizmoVertices()` / `buildRotateGizmoVertices()` / `buildScaleGizmoVertices()` returning `QVector<Vertex>` (剥离 the trailing glGenBuffers/glBufferData).
- Snapshot tests asserting vertex counts and bounding ranges match GL originals.
- Per-axis color baked into vertices (X=red, Y=green, Z=blue) for the per-vertex color approach.

**Success criteria:**
1. Geometry generators produce identical vertex counts to GL originals.
2. No GL or RHI calls in the geometry layer.
3. Snapshot tests pin the vertex layout.

**Plans:** 1/1 plans complete

Plans:
- [x] 66-01-PLAN.md — Wave 1: extract 3 gizmo geometry builders (move arrows 114v, rotate torus 5184v, scale shafts+boxes 114v) into static `GizmoGeometry` class at `src/core/rendering/`, returning `QVector<GizmoVertex>` with per-axis RGBA color baked in; shared POD `GizmoVertex.h` extracted so `RhiViewportRenderer::Vertex` becomes a `using = GizmoVertex` alias; GL delegates + shaders consume per-vertex color (location 1); 14-slot `GizmoGeometryTests` (counts, colors, bounding boxes, offsets) all pass (SUMMARY: 66-01-SUMMARY.md)

### Phase 67: RHI Gizmo State Wiring

**Goal:** Connect the broken gizmo state pipeline — RhiViewportRenderer::synchronize must read gizmoMode/cutAxis/cutPosition/gizmoCenter, and setGizmoMode must trigger update().

**Requirements:** `GWIRE-01`, `GWIRE-02`.

**Deliverables:**
- `RhiViewportRenderer::synchronize` reads viewport->gizmoMode(), cutAxis(), cutPosition(), and computes gizmoCenter from the selected object's AABB.
- `RhiViewport::setGizmoMode` calls `update()` so the renderer re-syncs.
- Diagnostic logging confirms state arrives at the renderer on mode change.

**Success criteria:**
1. Setting gizmoMode in QML produces the correct value in the renderer's next synchronize.
2. cutAxis/cutPosition changes propagate.
3. gizmoCenter tracks the selected object.

**Plans:** 1/1 plans complete

Plans:
- [x] 67-01-PLAN.md — Wave 1: add m_gizmoMode/m_cutAxis/m_cutPosition/m_gizmoCenter members to RhiViewportRenderer; extend synchronize() to read viewport state + compute gizmoCenter via free function `GizmoCenter::fromSelectedBatch()` (extracted to `src/core/rendering/GizmoCenter.{h,cpp}` for testability); diagnostic `[RHI] gizmo state:` qInfo log on delta; 5-slot `GizmoStateWiringTests` (no-selection, not-found, single/multi-batch, negative-ranges) all pass (SUMMARY: 67-01-SUMMARY.md)

### Phase 68: Move Gizmo RHI Render

**Goal:** First visible gizmo on the D3D11 path — render the X/Y/Z move arrows using a new gizmo shader and dedicated pipeline.

**Requirements:** `GMOV-01`, `GMOV-02`.

**Deliverables:**
- New `gizmo.vert.qsb` (GLSL 440 with uGizmoCenter/uGizmoScale displacement) + frag shader.
- Gizmo uniform buffer (camera MVP + gizmoCenter + gizmoScale) or per-vertex color approach for axis colors.
- Move gizmo vertex buffer + lines pipeline (arrows) + triangles pipeline (cones).
- render() draws the 3 axes when gizmoMode == GizmoMove.

**Success criteria:**
1. Setting gizmoMode to Move renders three colored axis arrows at the selected object.
2. Gizmo renders on top of geometry (depth clear or no-depth-write for gizmo).
3. Build passes; no regression on Prepare bed grid.

### Phase 69: Move Gizmo Pick + Drag Interaction

**Goal:** Close the interaction loop — user can click an axis and drag to move the object along it.

**Requirements:** `GMOV-03`, `GMOV-04`.

**Deliverables:**
- RhiViewport mouse handlers → computeRay → GizmoMath::pickMoveAxis (Phase 65).
- Drag delta computation → EditorViewModel object translation.
- Visual feedback (hover highlight on the picked axis, optional).

**Success criteria:**
1. Clicking the X axis and dragging moves the object along X only.
2. Y and Z axes work analogously.
3. No camera orbit triggered during gizmo drag (input event consumed).

**Plans:** 1/1 plans complete

Plans:
- [x] 69-01-PLAN.md - Wave 1: wire RHI move-gizmo axis picking and dragging through `RhiViewport`, `GizmoMath`, `EditorViewModel`, and the thin `PreparePage.qml` bridge; object translation applies per-frame while undo records one independent drag entry; focused viewmodel/QML audit tests and canonical verification pass (SUMMARY: 69-01-SUMMARY.md)

### Phase 70: Rotate + Scale Gizmos

**Goal:** Port the rotate (torus rings) and scale (shaft+box) gizmos, reusing the Phase 68 pipeline skeleton.

**Requirements:** `GROT-01`, `GROT-02`, `GSCA-01`, `GSCA-02`.

**Deliverables:**
- Rotate torus vertex buffer + render + pickRotateAxis + rotation interaction.
- Scale shaft+box vertex buffer + render + pickScaleAxis + scale interaction.

**Success criteria:**
1. Rotate gizmo: drag a ring to rotate the object around that axis.
2. Scale gizmo: drag a box handle to scale along that axis.
3. Both render correctly and pick precisely.

**Plans:** 1/1 plans complete

Plans:
- [x] 70-01-PLAN.md - Wave 1: upload and draw RHI rotate torus rings plus scale shafts/box handles from `GizmoGeometry`; generalize `RhiViewport` gizmo pick/drag through `GizmoMath::pickRotateAxis`, `computeRotateAngle`, `pickScaleAxis`, and `rayToAxisT`; apply rotate/scale deltas in `EditorViewModel` with one undo entry per drag; thin `PreparePage.qml` bridge and focused viewmodel/QML audit coverage pass with canonical verification (SUMMARY: 70-01-SUMMARY.md)

### Phase 71: Cut Plane + Wipe Tower

**Goal:** Port the cut plane gizmo and wipe tower rendering, completing the functional parity with the GL path.

**Requirements:** `GCUT-01`, `GWT-01`.

**Deliverables:**
- Cut plane render (translucent quad + outline, 2 pipelines) + cutAxis/cutPosition interaction.
- Wipe tower geometry + render (translucent box pipeline).
- synchronize extended to read wipe tower properties.

**Success criteria:**
1. Cut plane renders and adjusts with cutAxis/cutPosition.
2. Wipe tower renders when present in the scene.

### Phase 72: Precise Object Picking

**Goal:** Replace the AABB-screen-rectangle picking with ray-triangle (Möller-Trumbore) precision matching the GL path.

**Requirements:** `GPICK-01`.

**Deliverables:**
- cpuVerts passed through to RhiViewportRenderer.
- pickObject ported (ray-AABB prefilter + ray-triangle) from GizmoMath.

**Success criteria:**
1. Clicking a model selects the exact mesh under the cursor (not just the AABB bounding box).
2. Picking precision matches the GL path.

### Phase 73: Retire GLViewport + Verification

**Goal:** Remove the now-redundant GLViewportRenderer (2285 lines) and its dependencies; verify the RHI path is the sole functional path.

**Requirements:** `GRET-01`, `GRET-02`.

**Deliverables:**
- Delete GLViewportRenderer.{cpp,h}, GLViewport.{cpp,h} (QML item + renderer), GCodeRenderer's GLViewport dependency decoupled.
- Remove OWZX_OPENGL branch from main_qml.cpp registration (only RhiViewport + SoftwareViewport remain).
- Update CMakeLists.txt, qml.qrc, QmlUiAuditTests.
- Final verification: build passes, all gizmo interactions work on the default path, no regressions.

**Success criteria:**
1. GLViewportRenderer and GLViewport files removed; build succeeds without them.
2. All gizmo/pick/wipe-tower/cut-plane functionality works on the default D3D11 path.
3. OWZX_OPENGL environment flag no longer has any effect (no GL path to activate).
4. Codebase reduced by ~2285 lines.

### Phase 50: Screenshot and Source-Truth Inventory

**Goal:** Build the detailed visual/behavior contract before modifying UI code.

**Requirements:** `INV-01`, `INV-02`, `INV-03`, `INV-04`, `INV-05`.

**Deliverables:**

- Inventory table for `shotScreen/准备页.png`, `shotScreen/预览页.png`, `shotScreen/打印机参数设置页.png`, and `shotScreen/材料参数设置页.png`.
- Prepare module mapping against upstream `Plater.*`, `GLCanvas3D.*`, `GUI_ObjectList.*`, `GUI_ObjectSettings.*`, and `Gizmos/*`.
- Preview module mapping against upstream `GUI_Preview.*`, `GCodeViewer.*`, `GLCanvas3D.*`, and `libslic3r/GCode/*`.
- Settings module mapping against upstream `Tab.*`, `PresetComboBoxes.*`, `ConfigManipulation.*`, `UnsavedChangesDialog.*`, `CreatePresetsDialog.*`, `PrintConfig.*`, `Preset.*`, and `PresetBundle.*`.
- Modify-vs-replace decision per module, including cleanup checklist for each replacement.

**Success criteria:**

1. Every screenshot-visible region has a Qt target, upstream source target, status, and verification method.
2. No implementation phase starts with unmapped screenshot-visible controls in its scope.
3. Replacement decisions explicitly identify old files, routes, resources, registrations, imports, and tests to remove.
4. Open behavior gaps are classified as Real, Hybrid, Mock, Blocked, Placeholder, or Superseded.

### Phase 51: Shell and Navigation Restoration

**Goal:** Restore the application-level shell and workflow actions so Prepare, Preview, and settings sit inside the right OrcaSlicer-like frame.

**Requirements:** `SHELL-01`, `SHELL-02`, `SHELL-03`, `SHELL-04`, `SHELL-05`.

**Deliverables:**

- Source-truth comparison for top navigation, menus, page switching, and high-priority actions.
- Restored top shell and navigation QML structure, replacing off-design routes where needed.
- C++ viewmodel state for action enablement, loading, error, and blocked states.
- Notification/error placement that does not block critical viewport/sidebar controls.
- Removal plan for shell-level legacy pages/routes that conflict with the restored workflow.

**Success criteria:**

1. User can navigate Prepare, Preview, Device/Monitor, Project, and settings actions through a screenshot-aligned shell.
2. Import, slice, preview, export, save, undo/redo, and settings actions are driven by C++ state.
3. Page switching preserves relevant Prepare and Preview state.
4. No obsolete placeholder route remains in the restored local workflow.

### Phase 52: Prepare Sidebar and Preset Controls

**Goal:** Restore the Prepare left sidebar and preset/settings controls as the main configuration entry point.

**Requirements:** `PREPSB-01`, `PREPSB-02`, `PREPSB-03`, `PREPSB-04`, `PREPSB-05`.

**Deliverables:**

- Screenshot-aligned Prepare left sidebar structure for printer, material/filament, and process selections.
- Sidebar-to-settings dialog entry points for printer, material, and process categories.
- Compatibility, dirty, read-only, warning, and modified-state viewmodel bindings.
- Basic/advanced and search/filter behavior backed by C++ option/config models.
- Slice invalidation and merged-config integration for sidebar changes.

**Success criteria:**

1. User can select and inspect printer/material/process state from the Prepare sidebar.
2. Settings buttons open the correct restored parameter settings category.
3. Sidebar warning/dirty/compatibility state updates without placeholder text.
4. Sidebar changes invalidate stale slice/preview/export state through C++ paths.

### Phase 53: Prepare Object, Plate, and Viewport Workflow

**Goal:** Restore the Prepare workspace around object operations, plate operations, viewport controls, and gizmo entry points.

**Requirements:** `PREPWF-01`, `PREPWF-02`, `PREPWF-03`, `PREPWF-04`, `PREPWF-05`, `PREPWF-06`.

**Deliverables:**

- Source-truth comparison for object list/actions, plate controls, context menus, camera controls, and gizmos.
- Import/project load wiring from restored Prepare flow to object list, plate state, and renderer state.
- Object actions for select, rename, duplicate, delete, arrange, lock/unlock, printable state, and volume inspection where supported.
- Plate switching/management UI aligned with screenshot and current PartPlate behavior.
- View controls for camera orientation, zoom, bed/grid, vertical tools, and supported gizmo entry points.
- Renderer stability checks for selection, camera movement, plate switching, page return, and tool changes.

**Success criteria:**

1. User can import a model and immediately see consistent object, plate, and viewport state.
2. Object and plate actions either work through source-truth behavior or are honestly classified and gated.
3. Viewport controls do not overlap, resize unexpectedly, or leave stale selected/tool state.
4. Prepare renderer remains visible through the core interaction sequence.

### Phase 54: Preview Layout, Sliders, and Right Panels

**Goal:** Restore the Preview page layout and interaction surfaces around the existing G-code preview workflow.

**Requirements:** `PREVLAY-01`, `PREVLAY-02`, `PREVLAY-03`, `PREVLAY-04`, `PREVLAY-05`.

**Deliverables:**

- Screenshot-aligned Preview layout with left summary, center viewport, vertical layer slider, bottom move slider, and right panels.
- Layer and move slider viewmodel bindings that preserve camera and renderer state.
- Preview camera interaction handling for rotate, pan, and zoom.
- Plate thumbnail/summary, current layer/move, print time, filament usage, and warning displays.
- Panel collapse/resize behavior where supported by screenshot/source truth.

**Success criteria:**

1. User sees the same major Preview structure shown in the screenshot.
2. Dragging the layer slider does not clear or hide the preview.
3. Dragging the camera after slicing does not clear or hide the preview.
4. Right-side panels display real preview/print statistics where data is available.

### Phase 55: G-code Preview Semantics and Rendering Stability

**Goal:** Complete the G-code preview behavior behind the restored Preview UI and prevent renderer regressions.

**Requirements:** `GCODE-01`, `GCODE-02`, `GCODE-03`, `GCODE-04`, `GCODE-05`.

**Deliverables:**

- Source-truth comparison for G-code viewer data, color modes, line-type filters, layer/move semantics, and current-line sync.
- Real G-code segment/layer metadata path from slicing/export into Preview for normal local workflows.
- Color-mode and line-type filter APIs and UI bindings.
- Layer/move filtering integration with GPU draw ranges, legend values, and G-code text/current-line state.
- D3D11 QRhi path verification and regression tests for camera drag, layer drag, move drag, page switch, reslice, and export.

**Success criteria:**

1. Normal Preview does not rely on placeholder segment data after a real local slice.
2. Color modes and line-type filters update visible rendering and legend state coherently.
3. Layer/move filters update draw ranges and text/current-line state together.
4. Restored Preview does not regress to `SoftwareViewport` unless explicitly classified as fallback.

**Plans:** 5/5 plans complete

Plans:
**Wave 1**

- [x] 55-01-PLAN.md — Wave 0 foundation: OrcaSlicer-style .gcode fixture + PreviewParserTests RED scaffold
- [x] 55-02-PLAN.md — Wave 1 data semantics: 20-role ;TYPE: parser, 17 EViewType modes, GCV1 wire-format role field, render-side per-role filtering (no repack)

**Wave 2** *(blocked on Wave 1 completion)*

- [x] 55-03-PLAN.md — Wave 2 UI: VisibilityFilter.qml (right-panel per-role checkbox group) + PreviewPage roleVisibility binding
- [x] 55-04-PLAN.md — Wave 2 regression tests: GCODE-01 no-placeholder RED, GCODE-03 legend/atomicity, GCODE-04 SoftwareViewport/role-skip/sizeof audit guards, GCODE-05 reslice/export/page-switch invariants

**Wave 3** *(blocked on Wave 2 completion)*

- [x] 55-05-PLAN.md — Wave 3 D3D11 startup-policy audit guard + 55-VALIDATION.md sign-off

### Phase 56: Parameter Settings Dialogs Restoration

**Goal:** Restore printer, material, and process settings as independent OrcaSlicer-like settings workflows.

**Requirements:** `SETTINGS-01`, `SETTINGS-02`, `SETTINGS-03`, `SETTINGS-04`, `SETTINGS-05`, `SETTINGS-06`, `SETTINGS-07`.

**Deliverables:**

- Screenshot-aligned settings dialog/page structure for printer and material screenshots, plus process settings by upstream parity.
- Top tabs, option groups, scroll behavior, labels, controls, and density matched to the screenshots.
- C++ typed option model coverage for boolean, numeric, enum, string, unit, nullable, and multi-value fields where supported.
- Dirty state, modified indicators, value source, read-only state, validation warnings, and blocking errors.
- Save, Save As, reset option/group/all, discard, cancel, and unsaved-change behavior.
- Search and basic/advanced filtering.
- Integration with Prepare sidebar, slice invalidation, merged slicing config, and project save/restore.

**Success criteria:**

1. User opens printer/material/process settings from the restored workflow without Project-page embedding.
2. Settings dialogs visually match the supplied screenshots at the module and density level.
3. Edits are model-driven and update dirty, validation, and sidebar state.
4. Save/reset/unsaved-change actions follow upstream-compatible behavior.

**Plans:** 4/4 plans complete

Plans:
**Wave 1**

- [x] 56-01-PLAN.md -- Wave 0 foundation: ConfigOptionModel 7 typed kinds (nullable/isVector/percent/sidetext) + CxSpinBox suffix + RED test scaffolds for SETTINGS-01..07

**Wave 2** *(blocked on Wave 1 completion)*

- [x] 56-02-PLAN.md -- Wave 2 C++ backend: static page/group tables per tier, ConfigViewModel per-tier/per-group ops, BackendContext.openSettings active forwarder; SETTINGS-02/04/05/06 ViewModelSmokeTests GREEN

**Wave 3** *(blocked on Wave 2 completion)*

- [x] 56-03-PLAN.md -- Wave 3 QML dialogs: OptionRow (typed dispatch, TextInput+DoubleValidator for float), GroupNavSidebar, parameterized SettingsDialog shell, main.qml settingsRequested wiring, QmlUiAuditTests GREEN

**Wave 4** *(blocked on Wave 3 completion)*

- [x] 56-04-PLAN.md -- Wave 4 integration + verification: SETTINGS-07 E2E (option edit -> slice invalidation, dirty overrides persist via 3MF scoped-config), flip remaining Wave 0 scaffolds GREEN, finalize 56-VALIDATION.md

### Phase 57: Deprecated UI Removal and Architecture Cleanup

**Goal:** Remove replaced/off-design UI code and enforce clean architecture boundaries after restoration work.

**Requirements:** `CLEAN-01`, `CLEAN-02`, `CLEAN-03`, `CLEAN-04`.

**Deliverables:**

- Audit of all replaced Prepare, Preview, Settings, shell, sidebar, dialog, and renderer files.
- Deletion or update of obsolete QML/C++ files, `qml.qrc` entries, type registrations, routes, imports, tests, docs, and references.
- QML business-logic audit with migration of durable behavior into C++ services/viewmodels.
- Encoding and comment cleanup for changed files.

**Success criteria:**

1. No active UI path retains old/deprecated/disconnected copies of replaced components.
2. Resource files and type registrations contain only live components.
3. Workflow behavior, validation, settings state, preset state, and preview filtering live in C++.
4. Encoding guard and diff checks show no new BOM/mojibake/comment-rule issues.

**Plans:** 2/2 plans complete

Plans:
**Wave 1**

- [x] 57-01-PLAN.md -- Wave 1 reference audit + safe deletion of 4 locked Settings files (SettingsPage/ConfigPage/ParamsPage/SearchDialog) + 3 legacy sidebar panels (Sidebar/FilamentPanel/PrintSettings) + removal of the 3 named routes (canLeaveSettingsPage / requestConfigPageExitIfNeeded / requestLeaveSettingsPage) and the dead deferred-config-exit machinery; pending-unsaved queue for preset/scope actions retained; ProjectPage tab untouched (SUMMARY: 57-01-SUMMARY.md)

**Wave 2** *(blocked on Wave 1 completion)*

- [x] 57-02-PLAN.md -- Wave 2 doc rewrite (CrealityPrint -> OrcaSlicer in 2 locked docs) + compiled QmlUiAudit regression test locking the deletions as a permanent invariant + broader dead-UI sweep (CLEAN-02 audit) + global encoding guard on all phase-touched files (CLEAN-04) (SUMMARY: 57-02-SUMMARY.md)

### Phase 58: End-to-End Visual and Functional Verification

**Goal:** Prove the restored workflow works visually and functionally from import through G-code export.

**Requirements:** `VERIFY-01`, `VERIFY-02`, `VERIFY-03`, `VERIFY-04`, `VERIFY-05`.

**Deliverables:**

- Automated tests for inventory completeness, viewmodel state, QML route/resource registration, and cleanup guards.
- Automated tests for import -> configure -> prepare -> slice -> preview -> export state transitions.
- Regression tests or deterministic harnesses for Preview layer/move/camera disappearing bugs.
- Manual UAT checklist comparing Prepare, Preview, printer settings, and material settings against screenshots and upstream behavior.
- Canonical verification run or classified failure report.

**Success criteria:**

1. Requirements traceability shows 47/47 requirements mapped and verified or honestly classified.
2. The restored app can run the full local workflow without using deprecated UI paths.
3. Preview remains visible through layer slider, move slider, and camera interaction.
4. Manual visual/UAT checklist is ready for the user and records exact remaining gaps if any.
5. `scripts/auto_verify_with_vcvars.ps1` passes, or failure is documented with root cause and owner.

**Plans:** 2/2 plans complete

Plans:
- [x] 58-01-PLAN.md -- VERIFY-01: encode the 9 Phase 50 §2 deterministic inventory checks as a new tests/InventoryAuditTests.cpp Qt Test target (region counts/schema/enums/ID format/INV-02..04 anchors/cleanup format/no-blank-upstream) running against both docs/v3.6-ui-inventory.md (canonical) and the 50-INVENTORY.md snapshot, plus CMakeLists.txt registration mirroring QmlUiAuditTests (completed 2026-07-03, commit 592c4ef)
- [x] 58-02-PLAN.md -- VERIFY-02/03/04/05: audit existing E2E/Preview tests for transition + Preview-stability coverage (add a gap test only if a real gap is found), write 58-UAT.md manual checklist against the 4 screenshots mapping every region to a requirement, run canonical verify + classify every ctest failure (carry-forward CliTests missing-fixture + auto_verify Qt-mismatch flake) with file/command/cause/owner (completed 2026-07-03, commit 760374a; audit conclusion: NO NEW TEST CODE NEEDED)

## Deferred Backlog

- v3.4 Phase 43 manual UAT for import -> Prepare -> slice -> Preview -> export remains pending until the user can verify it.
- v3.5 Phase 47-49 are superseded by v3.6 and should not be resumed as standalone work unless explicitly reopened.
- Device send/upload/cloud print and Monitor task workflow.
- AssembleView.
- Auto filament-map recommendation and wipe-tower geometry/rendering.
- Real thumbnail capture and 3MF pixel round-trip.
- D3D12 crash root cause and future Vulkan/D3D12 backend promotion.
- ModelMall/Home WebView and cloud workflows.
- Full hardware calibration completion beyond preserving existing implemented preset read/write paths.

## Next Step

Start Phase 55:

```text
$gsd-plan-phase 55
```

or execute the milestone autonomously:

```text
$gsd-autonomous --from 55
```

---

*Last updated: 2026-07-01 after Phase 54 completion.*
