# Roadmap: OWzx Slicer

## Milestones

- Complete: **v2.9** - Phases 10-15 (shipped 2026-06-25)
- Complete: **v3.0 PartPlate Core** - Phases 16-22 (shipped 2026-06-26)
- Complete: **v3.1 QRhi Rendering** - Phases 23-28 (shipped 2026-06-28)
- Complete with tech debt: **v3.2 Multi-Plate Data Polish** - Phases 29-32 (audited 2026-06-28)
- Complete at MVP level, superseded for completeness: **v3.3 Slice Preview Main Flow MVP** - Phases 33-36
- Automated verification passed, manual UAT deferred: **v3.4 Import to G-code Complete Workflow** - Phases 37-43
- Superseded after Phase 46: **v3.5 Preset Authoring Complete Workflow** - Phases 44-49
- Active: **v3.6 Screenshot-Driven OrcaSlicer UI Restoration** - Phases 50-58

## Active Milestone: v3.6 Screenshot-Driven OrcaSlicer UI Restoration

**Goal:** Restore the Prepare page, Preview page, and parameter settings workflows as complete OrcaSlicer-equivalent user flows, using screenshots as visual/layout truth and OrcaSlicer source as behavior truth.

**Success criteria:**
- Every visible screenshot module has a recorded mapping to Qt target files and upstream OrcaSlicer source files.
- Prepare page layout and behavior support import, preset selection, object/plate actions, viewport controls, and gizmo entry points without placeholder-only workflows.
- Preview page layout and behavior support G-code inspection, layer/move sliders, camera interaction, right-side legend/stat panels, and G-code text sync without disappearing-render regressions.
- Printer, material, and process settings are restored as independent source-truth-aligned dialogs/pages with typed option editing, dirty state, validation, save/reset, and compatibility behavior.
- Replaced pages/components leave no abandoned files, routes, resource entries, registrations, imports, or tests behind.
- The full local workflow import -> configure -> prepare -> slice -> preview -> export is verified with automated checks and manual visual/UAT evidence.

## Phases

- [x] **Phase 50:** Screenshot and Source-Truth Inventory (completed 2026-06-30)
- [x] **Phase 51:** Shell and Navigation Restoration (completed 2026-07-01)
- [x] **Phase 52:** Prepare Sidebar and Preset Controls (completed 2026-07-01)
- [x] **Phase 53:** Prepare Object, Plate, and Viewport Workflow (completed 2026-07-01)
- [x] **Phase 54:** Preview Layout, Sliders, and Right Panels (completed 2026-07-01)
- [ ] **Phase 55:** G-code Preview Semantics and Rendering Stability
- [ ] **Phase 56:** Parameter Settings Dialogs Restoration
- [ ] **Phase 57:** Deprecated UI Removal and Architecture Cleanup
- [ ] **Phase 58:** End-to-End Visual and Functional Verification

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
