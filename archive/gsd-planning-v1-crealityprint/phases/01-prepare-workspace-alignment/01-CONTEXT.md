# Phase 1 Context: Prepare Workspace Alignment

**Phase**: 1 / Prepare Workspace Alignment
**Requirements**: PREP-01, PREP-02, PREP-03
**Date**: 2026-05-31

## Current State Assessment

Phase 1 covers the Prepare workspace — the primary user-facing view where users import models, arrange them on plates, configure print settings, and trigger slicing. This is the most-used workspace in the application.

### Implementation Inventory

| Area | Qt6 Files | Lines | Upstream Reference | Gap Level |
|------|-----------|-------|--------------------|-----------|
| Object tree | `ObjectList.qml` + `EditorViewModel` | ~4,700 | `GUI_ObjectList.cpp` (8,797) | Low |
| Left panel / plates | `PreparePage.qml` leftPanel section | ~1,300 | `Plater.cpp` sidebar + `PartPlateList` | Medium |
| Right panel (3 tabs) | `Sidebar.qml` + `SliceProgress.qml` + `PrintSettings.qml` | ~2,850 | `Plater.cpp` sidebar (lines 381-1334) | Medium |
| Toolbar | `PreparePage.qml` inline toolbar | ~300 | `Plater.cpp` GLToolbar handlers | Low |
| Background slicing | `SliceService.h/.cpp` + `EditorViewModel` slicing bridge | ~620 | `BackgroundSlicingProcess.cpp` (1,173) | High |
| Plate management | Distributed in `EditorViewModel` + `ProjectServiceMock` | ~800 | `PartPlate.cpp` (5,855) | High |
| Config/Preset | `ConfigViewModel` + `PresetListModel` + `ConfigOptionModel` | ~1,200 | `Tab.cpp` (~7,000) | High |

### What's Already Working

1. **ObjectList.qml** (1,166 lines): Full hierarchical tree with expand/collapse, drag reorder, context menus, volume type badges, inline rename, printable toggle, Ctrl-click multi-select, add volume/primitive/text/SVG submenus, cut/copy/paste/clone
2. **PreparePage.qml** (3,736 lines): Complete left panel with plate cards, printer/bed combos, object list integration, plate settings dialog (bed type, print sequence, spiral mode, filament sequence with drag reorder), object info bar
3. **Sidebar.qml** (509 lines): 3-tab collapsible panel (Objects/Print/Slice)
4. **SliceProgress.qml** (620 lines): Comprehensive result display — progress bar, multi-plate status, model size, time, speed, layer count, filament weight/length/cost, per-extruder breakdown, export path, action buttons
5. **PrintSettings.qml** (1,723 lines): Full Page>Category>Option hierarchy, scope selector (Global/Object/Part/Plate), value chain popup, search dialog, modified options summary
6. **Toolbar**: All gizmo buttons with keyboard shortcuts matching upstream
7. **SliceService** (537 lines): Real libslic3r slicing in HAS_LIBSLIC3R mode with progress, cancel, result extraction

### Key Gaps vs Upstream

1. **Plate architecture**: No dedicated PartPlateList/PartPlate class — logic embedded in EditorViewModel + ProjectServiceMock. Upstream has each plate owning its own Print + GCodeProcessorResult.
2. **Background slicing state machine**: Simplified vs upstream (no INITIAL->IDLE->STARTED->RUNNING->FINISHED/CANCELED lifecycle, no config snapshot, no SEH exception handling, no 500ms debounce timer)
3. **Filament allocation**: QML filament UI exists but data path may flow through mock. Upstream has real per-plate/per-object filament assignment with flushing volumes and auto-mapping.
4. **Config data flow**: ConfigViewModel and PrintSettings.qml have the UI structure but real preset data loading depends on Phase 2 (SETT-01) work.
5. **Slice all orchestration**: EditorViewModel has m_sliceAllQueue but the sequential plate-by-plate slice with automatic plate switching is simplified vs upstream's multi-plate coordination.

## Grey Areas

### Grey Area 1: Plate Management Architecture

**Question**: Should we extract plate management into a dedicated PlateService, or keep the current distributed approach in EditorViewModel/ProjectServiceMock?

- **Option A — Keep distributed** (lower risk): Current approach already works. Plate CRUD, settings, thumbnails, drag-to-plate all functional. Risk: EditorViewModel is already ~3,500 lines.
- **Option B — Extract PlateService** (better architecture): Dedicated C++ service mirroring upstream PartPlateList. Cleaner separation but requires significant refactoring of EditorViewModel, PreparePage, and ObjectList bindings.

**Recommendation**: Option A — keep distributed. The current approach works and Phase 1 is about alignment verification, not architecture refactoring. PlateService extraction can be a separate refactoring task later.

### Grey Area 2: Background Slicing State Machine Completeness

**Question**: How closely should our SliceService replicate the upstream BackgroundSlicingProcess state machine?

- **Option A — Functional alignment** (pragmatic): Ensure states (Idle->Slicing->Completed with Cancel/Error) work correctly with real data. Don't replicate SEH handling, config snapshots, or UI task synchronization. The current QtConcurrent approach is idiomatic Qt6.
- **Option B — Full state machine replication** (upstream-faithful): Replicate the 6-state lifecycle, config snapshot mechanism, and error handling model. Higher engineering effort for marginally more robust behavior.

**Recommendation**: Option A — functional alignment. The upstream uses wxThread + condition variables because wxWidgets lacks async primitives. Qt6's QtConcurrent + signals/slots provides equivalent behavior with cleaner code. Match the *states* and *transitions*, not the implementation mechanism.

### Grey Area 3: Config/Preset Data Dependency

**Question**: Phase 1 success criteria require "real values from the current preset" in the right panel, but full preset inheritance (SETT-01) is Phase 2. How to handle this dependency?

- **Option A — Wire available data** (incremental): Connect existing PresetListModel/ConfigViewModel data that already works. Accept that some values may show defaults rather than real inherited values until Phase 2 completes.
- **Option B — Pull preset work forward** (comprehensive): Move preset loading into Phase 1, expanding scope significantly.

**Recommendation**: Option A — wire available data. Phase 2 explicitly handles the full preset chain. Phase 1 should verify that the *UI surface* correctly displays whatever data flows through, not implement the full preset system.

### Grey Area 4: Toolbar/Sidebar Visual Fidelity Bar

**Question**: Success criteria mention "pixel-level fidelity" for toolbar appearance. What's the practical target?

- **Option A — Functional alignment** (pragmatic): Same buttons, same icons, same enabled/disabled states, same keyboard shortcuts. Accept Qt6 theme differences (font rendering, spacing within reason).
- **Option B — Pixel-perfect** (high effort): Match exact pixel positions, colors, icon sizes, font sizes. Requires extensive custom styling and pixel measurements.

**Recommendation**: Option A — functional alignment. This is a Qt6/QML application, not a wxWidgets clone. Visual consistency matters but pixel-perfect matching of a different widget toolkit is impractical and not a good use of time.

## Recommended Approach

1. **Verify existing implementations** against upstream behavior for each PREP requirement
2. **Fix data flow gaps** where real data should flow but mock data is used (especially slice results)
3. **Align state machine** functionally (not implementation-for-implementation) with upstream BackgroundSlicingProcess
4. **Accept distributed plate architecture** as-is for this phase
5. **Defer full preset system** to Phase 2, wire what's available in Phase 1

## Files to Examine

### Qt6 Implementation
- `src/qml_gui/pages/PreparePage.qml` — main workspace page
- `src/qml_gui/panels/ObjectList.qml` — object tree
- `src/qml_gui/panels/Sidebar.qml` — right panel container
- `src/qml_gui/panels/SliceProgress.qml` — slice result display
- `src/qml_gui/panels/PrintSettings.qml` — parameter panel
- `src/core/viewmodels/EditorViewModel.h/.cpp` — main workspace ViewModel
- `src/core/services/SliceService.h/.cpp` — slicing service
- `src/core/services/ProjectServiceMock.h/.cpp` — backing data store
- `src/core/viewmodels/ConfigViewModel.h/.cpp` — config/preset ViewModel

### Upstream Reference
- `third_party/CrealityPrint/src/slic3r/GUI/Plater.cpp` — main workspace (19,409 lines)
- `third_party/CrealityPrint/src/slic3r/GUI/BackgroundSlicingProcess.cpp` — state machine (1,173 lines)
- `third_party/CrealityPrint/src/slic3r/GUI/GUI_ObjectList.cpp` — object tree (8,797 lines)
- `third_party/CrealityPrint/src/slic3r/GUI/PartPlate.cpp` — plate management (5,855 lines)

---
*Generated: 2026-05-31 during Phase 1 smart discuss*
