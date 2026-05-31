# Requirements: Milestone v1.1 — End-to-End Slicing Workflow

## Scope

Verify and complete the full slicing workflow: STL/3MF import → machine/filament/process parameter settings → slice execution → G-code export → post-slice preview. Ensure no mock breakpoints remain in the critical path. Polish involved UI pages to usable state.

## Active Requirements

### PRESET-01: Preset Loading Completeness

**Priority:** P0
**Status:** Partial — real vendor presets load when HAS_LIBSLIC3R; falls back to 8 hardcoded presets

Real upstream vendor presets (Creality printers, filaments, process settings) must load correctly from `third_party/CrealityPrint/resources/profiles/`. The 3-tier chain (system → user → modified) must resolve correctly. When a user selects a printer, compatible filaments and processes must be filtered and displayed.

**Acceptance:**
- Vendor JSON parsed with full machine/filament/process lists
- Inheritance chains resolved (base → vendor → user overrides)
- Compatibility filtering works (nozzle diameter, temperature range)
- Current preset selection persists across model imports
- All 3 categories populated in parameter panel with real values

**Upstream reference:** `third_party/CrealityPrint/src/slic3r/GUI/PresetBundle.cpp`, `Tab.cpp`

### FLOW-01: E2E Workflow Integration

**Priority:** P0
**Status:** Partial — all components are REAL individually, but the full chain needs verification

The complete workflow from import to preview must work without any mock breakpoints. Each transition must carry real data.

**Acceptance:**
- Import STL/3MF → model appears in viewport with correct mesh
- Select printer → filament → process → all parameter values update
- Click Slice → libslic3r executes, progress bar updates, no crash
- Slice completes → G-code file written to disk, result summary shows real stats
- Auto-switch to Preview → G-code parsed, layer slider works, color mapping renders
- Export G-code → file dialog saves to user-specified path

**Upstream reference:** `third_party/CrealityPrint/src/slic3r/GUI/Plater.cpp` (on_slice_complete, on_process_completed)

### FLOW-02: Slice-to-Preview Data Continuity

**Priority:** P0
**Status:** Real — needs verification

After slicing, the G-code file path must flow to PreviewViewModel, and the preview must render the sliced result without requiring user intervention.

**Acceptance:**
- sliceFinished signal carries real output path
- PreviewViewModel.rebuildFromGCode() parses real G-code data
- Layer count, filament usage, print time match slice result summary
- Color mapping modes show distinct per-segment colors
- Stats panel displays real computed statistics

**Upstream reference:** `third_party/CrealityPrint/src/slic3r/GUI/GCodeViewer.cpp`

### UI-01: Prepare Page Polish

**Priority:** P1
**Status:** Polish needed — no TODOs, but needs visual QA pass

Prepare page must present a professional, usable interface matching upstream appearance. All interactive elements must function correctly.

**Acceptance:**
- Object list shows correct hierarchy with icons
- Toolbar buttons have correct enabled/disabled states
- Right panel sections collapse/expand correctly
- Slice button is prominent and clearly indicates state (idle/slicing/complete)
- Parameter search works across all options
- No visual glitches, overlapping elements, or clipped text

### UI-02: Preview Page Polish

**Priority:** P1
**Status:** Polish needed — no TODOs, but needs visual QA pass

Preview page must present sliced results clearly with all interactive controls working.

**Acceptance:**
- Layer slider snaps to valid layer boundaries
- Color mode selector shows all modes with correct labels
- Stats panel aligns data in readable format
- Legend panel updates correctly for each color mode
- Camera controls (zoom, rotate, pan) are smooth

### UI-03: Slice Progress and Export Polish

**Priority:** P1
**Status:** Polish needed — no TODOs, but needs visual QA pass

Slice progress panel must clearly communicate slicing state and results.

**Acceptance:**
- Progress bar accurately reflects libslic3r status callbacks
- Result summary shows all statistics in readable format
- Post-slice actions (Preview, Export) are clearly accessible
- Error states are displayed with actionable messages

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| PRESET-01 | Phase 1 | Partial |
| FLOW-01 | Phase 2 | Partial |
| FLOW-02 | Phase 2 | Partial |
| UI-01 | Phase 3 | Polish needed |
| UI-02 | Phase 3 | Polish needed |
| UI-03 | Phase 3 | Polish needed |

---
*Last updated: 2026-05-31 for milestone v1.1*
