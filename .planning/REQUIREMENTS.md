# Requirements: OWzx Slicer v3.6 Screenshot-Driven OrcaSlicer UI Restoration

**Defined:** 2026-07-01
**Status:** Active - requirements defined, roadmap ready
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; screenshot-driven UI milestones also use screenshots as visual/layout truth.

## Scope Contract

v3.6 is not an MVP milestone. It restores the complete local Prepare, Preview, and parameter settings user flow shown in the supplied screenshots and defined by OrcaSlicer upstream behavior:

```text
Import model -> select/edit printer/material/process settings -> prepare model/plate -> slice -> inspect G-code preview -> export G-code
```

Screenshots under `shotScreen/` define visual layout, density, module placement, and visible controls. OrcaSlicer source under `third_party/OrcaSlicer` defines behavior, state transitions, validation, and workflow semantics.

If an existing Qt page is materially off-design or too simplified for this scope, replace it. Replacement is not complete until old files, routes, registrations, resource entries, imports, tests, and disconnected code paths are removed.

v3.5 Phase 47-49 are superseded by this milestone. v3.5 Phase 44-46 remain historical evidence and may be reused only when they support the v3.6 source-truth UI restoration.

## Status Terms

- **Real:** source-truth behavior is implemented and verified with deterministic evidence.
- **Hybrid:** a real path exists, but fallback/mock behavior remains or verification is incomplete.
- **Mock:** local simulation only.
- **Blocked:** requires an unavailable dependency, credential, protocol, product decision, or upstream feature that is not locally available.
- **Placeholder:** visible UI or enum exists but has no meaningful backend behavior.
- **Superseded:** previous-milestone scope is intentionally abandoned in favor of this milestone.

## v3.6 Requirements

### Screenshot and Source Inventory

- [ ] **INV-01:** Every visible region in `shotScreen/准备页.png`, `shotScreen/预览页.png`, `shotScreen/打印机参数设置页.png`, and `shotScreen/材料参数设置页.png` is cataloged with region name, visible controls, Qt target file/component, upstream source file, behavior status, and verification method.
- [ ] **INV-02:** Prepare page behavior is mapped against upstream `Plater.*`, `GLCanvas3D.*`, `GUI_ObjectList.*`, `GUI_ObjectSettings.*`, and `Gizmos/*` before implementation work claims parity.
- [ ] **INV-03:** Preview page behavior is mapped against upstream `GUI_Preview.*`, `GCodeViewer.*`, `GLCanvas3D.*`, and `libslic3r/GCode/*` before implementation work claims parity.
- [ ] **INV-04:** Printer/material/process settings behavior is mapped against upstream `Tab.*`, `PresetComboBoxes.*`, `ConfigManipulation.*`, `UnsavedChangesDialog.*`, `CreatePresetsDialog.*`, `PrintConfig.*`, `Preset.*`, and `PresetBundle.*`.
- [ ] **INV-05:** For each module, the plan records whether to modify existing Qt code or replace it, and replacement decisions include a cleanup checklist for obsolete files and references.

### Shell, Navigation, and Workflow Actions

- [ ] **SHELL-01:** User sees an OrcaSlicer-like top shell with menu actions, Prepare/Preview/Device/Project navigation, current workspace state, and high-priority workflow actions matching the screenshot layout.
- [ ] **SHELL-02:** Page switching preserves relevant Prepare/Preview state and does not route through obsolete placeholder pages or off-design intermediate views.
- [ ] **SHELL-03:** Import, slice, preview, export, save, undo/redo, and settings actions expose enabled/disabled/loading/error states through C++ viewmodels, not QML-only conditions.
- [ ] **SHELL-04:** Notifications, validation errors, and blocking workflow messages are visible in the restored shell without covering critical sidebar, viewport, or preview controls.
- [ ] **SHELL-05:** The app shell removes or hides legacy pages/routes that no longer belong to the restored local workflow.

### Prepare Sidebar and Preset Controls

- [ ] **PREPSB-01:** User can view and change printer, material/filament, and process selections in a left sidebar visually aligned with the Prepare screenshot.
- [ ] **PREPSB-02:** User can open printer, material, and process settings from the sidebar into the restored parameter settings dialogs with the correct active category.
- [ ] **PREPSB-03:** User can see compatibility, dirty, read-only, warning, and modified states for active printer/material/process presets without relying on placeholder text.
- [ ] **PREPSB-04:** User can switch basic/advanced mode, search/filter relevant settings, and see the same option group visibility semantics used by upstream where local data supports it.
- [ ] **PREPSB-05:** Sidebar controls feed Prepare readiness, slice invalidation, and SliceService merged configuration consistently with the C++ preset/config model.

### Prepare Object, Plate, and Viewport Workflow

- [x] **PREPWF-01:** User can import models/projects from the restored Prepare workflow and see model/object state update in the sidebar, plate controls, and viewport.
- [x] **PREPWF-02:** User can select, rename, duplicate, delete, arrange, lock/unlock, mark printable/unprintable, and inspect objects/volumes through source-truth-aligned object and context-menu flows.
- [x] **PREPWF-03:** User can switch and manage plates with screenshot-aligned plate controls and source-truth-aligned plate membership behavior.
- [x] **PREPWF-04:** User can use viewport camera controls, zoom/view orientation controls, bed/grid display, and vertical tool buttons without layout overlap or stale state.
- [x] **PREPWF-05:** User can use move, rotate, scale, place-on-face, cut, support/seam/paint, and related gizmo entry points only when backed by real or explicitly classified behavior.
- [x] **PREPWF-06:** Prepare renderer remains visible and stable while selecting objects, changing tools, rotating the camera, switching plates, and returning from Preview.

### Preview Layout, Controls, and Panels

- [x] **PREVLAY-01:** User sees a Preview page layout matching the supplied screenshot: left summary/sidebar, center G-code viewport, vertical layer slider, bottom move slider, and right legend/statistics/G-code panels.
- [x] **PREVLAY-02:** User can move the layer slider and move slider without the model or G-code preview disappearing, losing camera state, or resetting unrelated UI state.
- [x] **PREVLAY-03:** User can rotate/pan/zoom the Preview viewport after slicing without the preview disappearing or clearing draw ranges.
- [x] **PREVLAY-04:** User can view plate thumbnail/summary, current layer, current move, print time, filament usage, and slice result warnings where upstream exposes equivalent data.
- [x] **PREVLAY-05:** User can collapse or resize side panels where the screenshot/upstream workflow expects it, while preserving text fit and avoiding viewport overlap.

### G-code Preview Semantics and Rendering

- [ ] **GCODE-01:** Preview receives real G-code path/segment/layer metadata from the slicing/export path and does not depend on placeholder segment data for normal local workflows.
- [ ] **GCODE-02:** User can switch color modes and line-type filters for travel, perimeter, infill, support, skirt/brim, wipe tower, and other available upstream-equivalent paths.
- [ ] **GCODE-03:** Layer and move filtering update GPU draw ranges, legend values, and current-line/G-code text state consistently.
- [ ] **GCODE-04:** Renderer backend selection uses the Qt-native high-performance path already established for Windows, with D3D11 as the default and no regression to `SoftwareViewport` for restored Preview unless explicitly classified as fallback.
- [ ] **GCODE-05:** Preview interaction stability is regression-tested for camera drag, layer drag, move drag, page switch, reslice, and export.

### Parameter Settings Dialogs

- [ ] **SETTINGS-01:** User can open independent printer, material, and process settings dialogs/pages that visually match the supplied screenshots and do not rely on the off-design Project/Settings embedding.
- [ ] **SETTINGS-02:** User can navigate top tabs and option groups for printer and material settings with screenshot-aligned density, labels, controls, and scroll behavior.
- [ ] **SETTINGS-03:** User can edit typed config options through C++ option models covering booleans, numbers, enums, strings, units, nullable values, and multi-value fields where supported.
- [ ] **SETTINGS-04:** User can see dirty state, modified option indicators, inherited/default value source, read-only state, validation warnings, and blocking errors.
- [ ] **SETTINGS-05:** User can Save, Save As, reset option, reset group/all, discard, or cancel unsaved changes according to upstream settings/preset behavior.
- [ ] **SETTINGS-06:** User can search settings, toggle basic/advanced visibility, and see filtered/no-match states that are useful and source-truth-aligned.
- [ ] **SETTINGS-07:** Settings changes update Prepare sidebar state, slice invalidation, merged slicing config, and project save/restore behavior.

### Replacement and Cleanup

- [x] **CLEAN-01:** Every page/component replaced during v3.6 has its obsolete QML/C++ files, `qml.qrc` entries, registrations, routes, imports, tests, and documentation references removed or updated.
- [x] **CLEAN-02:** No active UI path keeps parallel `old`, `legacy`, `deprecated`, `unused`, placeholder-only, or disconnected copies of replaced Prepare, Preview, or Settings components.
- [x] **CLEAN-03:** QML remains presentation/wiring only; durable workflow behavior, validation, settings state, preset state, and preview filtering live in C++ services/viewmodels.
- [x] **CLEAN-04:** English ASCII-only comments and UTF-8-without-BOM are preserved across changed source, QML, Markdown, JSON, and CMake files.

### End-to-End Verification

- [x] **VERIFY-01:** Automated tests cover screenshot/source inventory completeness, critical C++ viewmodel state, QML route/resource registration, and cleanup of replaced UI code. (Phase 58-01: tests/InventoryAuditTests.cpp + QmlUiAuditTests)
- [x] **VERIFY-02:** Automated tests cover import -> configure -> prepare -> slice -> preview -> export state transitions, including slice invalidation after settings changes. (Phase 58-02 audit: covered by existing E2EWorkflowTests slots, no new code needed)
- [x] **VERIFY-03:** Automated tests or deterministic harnesses cover Preview layer/move/camera interactions so the disappearing-preview bug cannot regress. (Phase 58-02 audit: covered by existing E2EWorkflowTests + QmlUiAuditTests GCODE-05 slots)
- [ ] **VERIFY-04:** Manual UAT checklist validates visual parity against the four screenshots and behavior parity against the mapped upstream source for Prepare, Preview, and settings. (Phase 58-02: 58-UAT.md produced; awaiting human sign-off — status human_needed)
- [x] **VERIFY-05:** Canonical verification passes or any failure is classified with file, command, cause, and follow-up owner. (Phase 58-02: 58-VERIFICATION.md; 7/8 ctest PASS, single CliTests failure classified carry-forward)

## Future Requirements

- Device send/print workflows, including upload to printer, cloud printing, and Monitor task lifecycle.
- AssembleView source-truth completion.
- Auto filament-map recommendation and wipe-tower geometry/rendering.
- Real GL/QRhi-capture thumbnails and 3MF pixel round-trip.
- Full PLATE-09 save/reload assertions after shared 3MF writer integration is fixed.
- D3D12 or Vulkan default promotion after separate backend feasibility and stability work.
- ModelMall/Home WebView and cloud-related workflows.
- Full i18n translation coverage beyond strings touched by this milestone.
- Full hardware calibration completion beyond preserving existing implemented preset read/write paths.

## Out of Scope

| Feature | Reason |
|---|---|
| Device send, upload, cloud print, and Monitor print-job workflow | Separate workflow after local Prepare/Preview/export is visually and behaviorally restored. |
| Changing libslic3r slicing algorithms | GUI migration preserves libslic3r behavior. |
| Making D3D12 or Vulkan the default backend | D3D11 QRhi remains the known stable Qt-native Windows backend. |
| AssembleView, auto filament-map, and wipe-tower rendering | Separate source-truth workflows outside the supplied screenshots. |
| Claiming v3.4 manual UAT completion | User could not verify it earlier; it remains pending until evidence exists. |
| Resuming v3.5 Phase 47-49 as standalone work | Superseded by v3.6 full UI restoration. Relevant pieces are folded into Settings and E2E phases. |
| Keeping old UI code after replacement | User explicitly requires no abandoned/deprecated UI code in the project. |

## Traceability

| Requirement | Phase | Status |
|---|---|---|
| INV-01 | Phase 50 | Pending |
| INV-02 | Phase 50 | Pending |
| INV-03 | Phase 50 | Pending |
| INV-04 | Phase 50 | Pending |
| INV-05 | Phase 50 | Pending |
| SHELL-01 | Phase 51 | Pending |
| SHELL-02 | Phase 51 | Pending |
| SHELL-03 | Phase 51 | Pending |
| SHELL-04 | Phase 51 | Pending |
| SHELL-05 | Phase 51 | Pending |
| PREPSB-01 | Phase 52 | Pending |
| PREPSB-02 | Phase 52 | Pending |
| PREPSB-03 | Phase 52 | Pending |
| PREPSB-04 | Phase 52 | Pending |
| PREPSB-05 | Phase 52 | Pending |
| PREPWF-01 | Phase 53 | Complete |
| PREPWF-02 | Phase 53 | Complete |
| PREPWF-03 | Phase 53 | Complete |
| PREPWF-04 | Phase 53 | Complete |
| PREPWF-05 | Phase 53 | Complete |
| PREPWF-06 | Phase 53 | Complete |
| PREVLAY-01 | Phase 54 | Complete |
| PREVLAY-02 | Phase 54 | Complete |
| PREVLAY-03 | Phase 54 | Complete |
| PREVLAY-04 | Phase 54 | Complete |
| PREVLAY-05 | Phase 54 | Complete |
| GCODE-01 | Phase 55 | Complete |
| GCODE-02 | Phase 55 | Complete |
| GCODE-03 | Phase 55 | Pending |
| GCODE-04 | Phase 55 | Pending |
| GCODE-05 | Phase 55 | Pending |
| SETTINGS-01 | Phase 56 | Pending |
| SETTINGS-02 | Phase 56 | Complete |
| SETTINGS-03 | Phase 56 | Complete |
| SETTINGS-04 | Phase 56 | Complete |
| SETTINGS-05 | Phase 56 | Complete |
| SETTINGS-06 | Phase 56 | Complete |
| SETTINGS-07 | Phase 56 | Pending |
| CLEAN-01 | Phase 57 | Complete |
| CLEAN-02 | Phase 57 | Complete |
| CLEAN-03 | Phase 57 | Complete |
| CLEAN-04 | Phase 57 | Complete |
| VERIFY-01 | Phase 58 | Pending |
| VERIFY-02 | Phase 58 | Pending |
| VERIFY-03 | Phase 58 | Pending |
| VERIFY-04 | Phase 58 | Pending |
| VERIFY-05 | Phase 58 | Pending |

**Coverage:** 47 total; 47 mapped; 0 unmapped.

---

*Requirements defined: 2026-07-01*
*Last updated: 2026-07-01 for v3.6 milestone definition.*
