# Roadmap: Milestone v1.1 — End-to-End Slicing Workflow

## Overview

Complete and verify the full slicing workflow from model import through G-code preview. The codebase already has real implementations for each component — this milestone focuses on closing the remaining preset gaps, verifying the E2E chain works without mock breaks, and polishing involved UI pages to production quality.

## Phases

- [ ] **Phase 1: Preset System Completion** — Ensure real vendor presets load completely and the 3-tier parameter chain flows correctly to the UI
- [ ] **Phase 2: E2E Workflow Verification** — Run through the complete slice workflow, fix any data flow breaks or crashes
- [ ] **Phase 3: UI Polish Pass** — Visual QA and polish on Prepare, Preview, and Slice Progress pages

## Phase Details

### Phase 1: Preset System Completion
**Mode**: MVP
**Goal**: Users can select a real Creality printer, compatible filament, and process preset, and see real parameter values in the right panel.
**Depends on**: Nothing (builds on validated foundation)
**Requirements**: PRESET-01
**Success Criteria**:
  1. Vendor JSON files from `third_party/CrealityPrint/resources/profiles/Creality/` parse correctly and populate all 3 preset categories (printer, filament, process)
  2. Inheritance chains resolve: system defaults → vendor overrides → user modifications
  3. Filament compatibility filtering works — only filaments matching the selected printer's nozzle diameter and temperature range appear
  4. Selecting a preset updates all parameter values in the right panel (PrintSettings.qml)
  5. Value source indicators correctly show which tier provides each value
**Plans**: 1 plan
Plans:
- [ ] v11-01-01-PLAN.md — Fix vendor path, store compatibility metadata, wire upstream defaults into hierarchy merge

### Phase 2: E2E Workflow Verification
**Mode**: MVP
**Goal**: The complete import → configure → slice → export → preview workflow runs without any mock breakpoints or data loss.
**Depends on**: Phase 1
**Requirements**: FLOW-01, FLOW-02
**Success Criteria**:
  1. Importing an STL file produces a visible mesh in the Prepare viewport
  2. Selecting printer/filament/process and clicking Slice triggers libslic3r, progress bar updates in real time, and slice completes without error
  3. Slice result summary shows real statistics (time, filament, cost) matching libslic3r output
  4. G-code file is written to disk and can be re-loaded
  5. Auto-switch to Preview renders the G-code with at least Line Type color mapping working
  6. Export G-code dialog saves the file to a user-chosen location
  7. Any data flow breaks between components are identified and fixed
**Plans**: 1 plan
Plans:
- [ ] v11-02-01-PLAN.md — Inject preset config into slice engine, add E2E workflow test coverage

### Phase 3: UI Polish Pass
**Mode**: MVP
**Goal**: All pages involved in the slicing workflow present a professional, usable interface with no visual glitches.
**Depends on**: Phase 2
**Requirements**: UI-01, UI-02, UI-03
**Success Criteria**:
  1. Prepare page: object list, toolbar, right panel all render correctly with proper spacing, alignment, and theme colors
  2. Preview page: sliders, stats, legend, and color mode selector all function smoothly
  3. Slice progress panel: progress bar, result summary, and action buttons are clearly readable
  4. No clipped text, overlapping elements, or broken layouts on any involved page
  5. Consistent theme token usage across all pages (Theme.bgBase, Theme.accent, etc.)
**Plans**: 3 plans
Plans:
- [ ] v11-03-01-PLAN.md — Add 6 Theme tokens, tokenize SliceProgress + PreviewPage + LayerSlider + StatsPanel + MoveSlider
- [ ] v11-03-02-PLAN.md — Tokenize PrintSettings + Sidebar
- [ ] v11-03-03-PLAN.md — Tokenize PreparePage (195 hardcoded colors)

## Progress

**Execution Order:**
Phases execute in numeric order: 1 -> 2 -> 3

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 1. Preset System Completion | 0/1 | Planning complete | - |
| 2. E2E Workflow Verification | 0/1 | Planning complete | - |
| 3. UI Polish Pass | 0/3 | Planning complete | - |
