# Roadmap: CrealityPrint Qt6/QML Migration

## Overview

Migrate the CrealityPrint v7.0.1 3D printer slicer GUI from C++/wxWidgets to C++/Qt6/QML, closing the remaining gap between the established Qt6 architecture and upstream source-truth behavior. The project has already completed build infrastructure, core rendering, object CRUD, slicing pipeline, and 8 gizmo real APIs. This milestone delivers full workspace alignment, preset inheritance, GL rendering for all gizmos, device interaction, and release readiness.

## Phases

**Phase Numbering:**
- Integer phases (1-8): Planned milestone work
- Decimal phases (e.g., 2.1): Urgent insertions (marked with INSERTED)

- [ ] **Phase 1: Prepare Workspace Alignment** -- Plate filament allocation, right panel E2E, slicing state machine
- [ ] **Phase 2: Settings and Preset Inheritance** -- Upstream Tab data, PresetBundle, ConfigOptionDef schema
- [ ] **Phase 3: Preview Workspace Verification** -- 13 color mappings, StatsPanel real data, slider interactions
- [ ] **Phase 4: Project Workflow** -- Save/load complete workflow, file import format alignment
- [ ] **Phase 5: Gizmo GL Rendering** -- All gizmo interactive GL rendering and painting
- [ ] **Phase 6: Device and Calibration Depth** -- Device state machine, calibration real workflow
- [ ] **Phase 7: Mall and Multi-Machine** -- QtWebEngine mall, MQTT/SSDP multi-machine
- [ ] **Phase 8: Release Readiness** -- Regression baseline, automation, I18N verification

## Phase Details

### Phase 1: Prepare Workspace Alignment
**Mode**: MVP
**Goal**: Users can operate the Prepare workspace with real data flowing through the right panel, object tree, and slicing pipeline, matching upstream behavior.
**Depends on**: Nothing (first phase -- builds on validated foundation)
**Requirements**: PREP-01, PREP-02, PREP-03
**Success Criteria** (what must be TRUE):
  1. Object tree shows correct hierarchy (objects, volumes, instances) with real data from ProjectServiceMock, matching upstream Plater object list behavior
  2. Right panel displays slice result summary with real statistics (time, filament usage, cost) after a successful slice
  3. Right panel parameter sections (print/filament/printer) show real values from the current preset, and arrange settings panel shows real bed dimensions and spacing
  4. Background slicing process state machine correctly transitions through Idle->Slicing->Exporting->Completed with progress, cancel, and error states aligned with upstream BackgroundSlicingProcess
  5. Toolbar and sidebar visual elements (icons, spacing, enabled/disabled states) match upstream toolbar appearance at pixel-level fidelity
**Plans**: CONTEXT.md, PLAN.md
**UI hint**: yes

### Phase 2: Settings and Preset Inheritance
**Mode**: MVP
**Goal**: Users can browse and modify print settings through the complete upstream preset hierarchy with real data, matching upstream Tab behavior.
**Depends on**: Phase 1
**Requirements**: SETT-01
**Success Criteria** (what must be TRUE):
  1. Settings page loads real upstream Tab category hierarchy (Quality, Speed, Infill, Support, etc.) with correct page grouping and option visibility rules from upstream Tab.cpp
  2. PresetBundle loads the 3-tier preset chain (system -> user -> modified) with real inheritance, and switching presets correctly updates all dependent config options
  3. ConfigOptionDef schema provides complete type info (int, float, enum, string, point, bool), valid ranges, defaults, and tooltip text for all ~110 upstream config keys
  4. Settings search filters options across all categories and shows matching results with correct labels
**Plans**: TBD
**UI hint**: yes

### Phase 3: Preview Workspace Verification
**Mode**: MVP
**Goal**: Users can inspect sliced G-code in Preview with all 13 color mappings, real statistics, and full slider interaction, matching upstream GCodeViewer behavior.
**Depends on**: Phase 1
**Requirements**: PREV-01
**Success Criteria** (what must be TRUE):
  1. All 13 G-code color mapping modes (Line Type, Feature Type, Tool, Height, Width, Speed, Fan Speed, Temperature, Volumetric Flow Rate, Layer Time, Layer Height, Extrusion Role, Color Print) render correctly with distinct per-segment colors matching upstream GCodeViewer
  2. StatsPanel displays real G-code statistics (total time, total filament, layer count, estimated cost) computed from the parsed G-code data
  3. LayerSlider navigates between layers and shows the correct layer boundary; MoveSlider scrubs through moves within a layer; both update the 3D view in real time
  4. Legend panel updates to show correct labels and color swatches for the currently selected color mode
**Plans**: TBD
**UI hint**: yes

### Phase 4: Project Workflow
**Mode**: MVP
**Goal**: Users can save and load complete 3MF projects and import all supported file formats with full fidelity, matching upstream file workflow.
**Depends on**: Phase 2
**Requirements**: PROJ-01
**Success Criteria** (what must be TRUE):
  1. Save project produces a valid 3MF file containing all objects, transforms, plate assignments, config overrides, and metadata; loading that file restores the complete workspace state
  2. Import supports all upstream formats (STL, OBJ, 3MF, STEP, AMF) with correct mesh loading and coordinate transform application
  3. Save/load round-trip preserves plate assignments, per-object config overrides, and gizmo-specific volume data (Emboss text, SVG paths, Cut parameters)
  4. File open dialog filters match upstream supported extensions and the drag-drop import path works for all supported formats
**Plans**: TBD
**UI hint**: yes

### Phase 5: Gizmo GL Rendering
**Mode**: MVP
**Goal**: Users can interact with all gizmos in the 3D viewport with visual feedback matching upstream GLGizmo behavior -- handles, previews, painting overlays, and real-time mesh updates.
**Depends on**: Phase 1
**Requirements**: GIZM-01, GIZM-02, GIZM-03, GIZM-04
**Success Criteria** (what must be TRUE):
  1. Text/Emboss/SVG gizmos render interactive GL handles (position, rotation, font size sliders) and show real-time text/shape preview on the mesh surface in the viewport
  2. Simplify/Drill/MeshBoolean/AdvancedCut gizmos render GL selection handles and show preview geometry (drill cylinder, cut plane, boolean result outline) before applying the operation
  3. Support Paint and Seam Paint gizmos render per-triangle color overlay on the mesh surface showing painted/unpainted regions, with brush size indicator following the cursor
  4. Hollow gizmo renders a semi-transparent hollow preview shell around the selected object with draggable drain-hole handles, using CGAL-based offset mesh generation (non-OpenVDB approach)
  5. MmuSegmentation gizmo renders per-triangle color mapping showing assigned extruders on the mesh surface, with per-face painting via cursor interaction (non-TriangleSelectorPatch approach)
**Plans**: TBD
**UI hint**: yes

### Phase 6: Device and Calibration Depth
**Mode**: MVP
**Goal**: Users can connect to real printers, monitor print status, and run calibration workflows with real device parameters, matching upstream device interaction.
**Depends on**: Phase 2
**Requirements**: DEVC-01, CALI-01
**Success Criteria** (what must be TRUE):
  1. DeviceManager state machine transitions through Discovered->Connecting->Connected->Ready->Printing->Completed with correct event handling at each transition, matching upstream DeviceManager
  2. Monitor page displays real device status (temperature, fan speed, print progress, HMS alerts) when a device is connected via the protocol layer
  3. Calibration page presents the full workflow (device selection -> preset selection -> real parameter editing -> start calibration -> view results history) with data flowing from DeviceService and PresetService
  4. Calibration history records are persisted and viewable, showing date, calibration type, parameters used, and results
**Plans**: TBD
**UI hint**: yes

### Phase 7: Mall and Multi-Machine
**Mode**: MVP
**Goal**: Users can browse the model mall via embedded WebView and manage multiple printers simultaneously, matching upstream multi-device behavior.
**Depends on**: Phase 6
**Requirements**: MALL-01, MULT-01
**Success Criteria** (what must be TRUE):
  1. Model Mall page renders the upstream mall web content inside a QtWebEngine WebView with working navigation, search, and model download interaction
  2. Multi-machine page displays a list of discovered printers (via SSDP/MQTT discovery) with connection status, and users can connect to multiple printers simultaneously
  3. Switching between connected devices in the multi-machine view updates the monitor page to show the selected device's real-time status
**Plans**: TBD
**UI hint**: yes

### Phase 8: Release Readiness
**Mode**: MVP
**Goal**: The application passes systematic regression verification against upstream behavior across all pages, with automated testing and complete I18N coverage.
**Depends on**: Phase 7
**Requirements**: REL-01
**Success Criteria** (what must be TRUE):
  1. Upstream behavior regression baseline document exists with pass/fail criteria for every upstream workflow (import, slice, preview, settings, save, gizmo, device, calibration)
  2. Automated regression tests exercise all pages in both Mock and HAS_LIBSLIC3R modes, and any test failure blocks the build
  3. All 6 language files (zh_CN, en, ja, ko, de, fr) have complete translations with no missing keys relative to the English source, and language switching applies correctly across all pages without restart
  4. Full-page regression automation runs as part of the build verification script and reports per-page pass/fail status
**Plans**: TBD

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3 -> 4 -> 5 -> 6 -> 7 -> 8

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Prepare Workspace Alignment | 1/1 | Completed | 2026-05-31 |
| 2. Settings and Preset Inheritance | 1/1 | Completed | 2026-05-31 |
| 3. Preview Workspace Verification | 1/1 | Completed | 2026-05-31 |
| 4. Project Workflow | 0/TBD | Not started | - |
| 5. Gizmo GL Rendering | 0/TBD | Not started | - |
| 6. Device and Calibration Depth | 0/TBD | Not started | - |
| 7. Mall and Multi-Machine | 0/TBD | Not started | - |
| 8. Release Readiness | 0/TBD | Not started | - |
