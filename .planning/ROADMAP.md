# Roadmap: OWzx Slicer

## Overview

v5.11 converges the FFF Process (`print`) SettingsDialog on OrcaSlicer source
truth. It replaces the flat decorative grouping with a source-mapped Process
projection, stable session disclosure, safe typed editing, preserved preset
workflow semantics, and end-to-end evidence. Printer and Material expansion,
multi-nozzle and per-extruder editing, device/network/cloud, SLA, libslic3r
changes, and unmapped auto-correction are outside this milestone.

## Milestone History

- Historical milestones v2.9 through v5.7 are complete; see
  `.planning/MILESTONES.md` for their archived history.
- Complete: v5.8 Submodule Baseline Alignment + Hollow Editing - Phases 212-215.
- Complete: v5.9 ConfigWizard Multi-Vendor Selection - Phases 216-219.
- Complete: v5.10 Polish & Source-Truth Consolidation - Phases 220-225,
  recorded complete in `.planning/STATE.md` on 2026-07-26.
- Planned: v5.11 Process Settings Source-Truth Convergence - Phases 226-230.

## Phases

- [x] **Phase 226: Source-Mapped Process Hierarchy** - Establish the exact (completed 2026-08-11)
  upstream Process page and group navigation contract.
- [ ] **Phase 227: Process Projection, Disclosure, And Filtering** - Deliver
  stable page-qualified group rows, session disclosure, counts, search, and mode behavior.
- [ ] **Phase 228: Typed Process Editing** - Make Process option rows clear,
  resettable, validated, and honest about unsupported vector values.
- [ ] **Phase 229: Preset Feedback And Workflow Safety** - Preserve visible
  Process preset state and lifecycle behavior through navigation and edits.
- [ ] **Phase 230: Process Workflow Evidence** - Prove the complete Process
  workflow through automated, end-to-end, and fixed-size visual verification.

## Phase Details

### Phase 226: Source-Mapped Process Hierarchy
**Goal**: Users can navigate the FFF Process settings through the exact upstream
page and group hierarchy rather than a divergent flat or fallback list.
**Depends on**: Nothing (first v5.11 phase)
**Requirements**: HIER-01, HIER-02
**Success Criteria** (what must be TRUE):
  1. User sees the Process pages in this upstream order: Quality, Strength, Speed, Support, Multimaterial, and Others.
  2. User can reach every supported Process option from its source-mapped page and group, with no alphabetical or category fallback moving an unmapped option.
**Plans**: 1 plan
**UI hint**: yes

### Phase 227: Process Projection, Disclosure, And Filtering
**Goal**: Users can scan, search, and disclose stable Process groups whose
identity and counts remain correct as the current page, mode, and data change.
**Depends on**: Phase 226
**Requirements**: HIER-03, DISC-01, DISC-02, DISC-03, DISC-04, FILT-01, FILT-02, FILT-03
**Success Criteria** (what must be TRUE):
  1. User sees only non-empty groups on the active Process page, each with accurate visible-option and dirty-option counts.
  2. User can independently expand or collapse one group by pointer or keyboard, and its glyph, focus behavior, and accessible name report its actual state.
  3. User retains a group's session disclosure state while changing page, query, Advanced mode, preset, or dialog visibility; search temporarily reveals matches without overwriting that saved state.
  4. User can search option keys and labels on the active Process page, retains the query across page or mode changes, and sees Advanced include every Simple option plus upstream Advanced and Develop options.
  5. User sees rows and group counts refresh without reopening the dialog after an edit, reset, preset change, or read-only-state change.
**Plans**: 2 plans
**UI hint**: yes

### Phase 228: Typed Process Editing
**Goal**: Users can understand and safely change supported Process values with
the same C++ configuration and effective-value route that already owns preset behavior.
**Depends on**: Phase 227
**Requirements**: ROW-01, ROW-02, ROW-03, ROW-04, ROW-05
**Success Criteria** (what must be TRUE):
  1. User can identify a Process option's label, value, unit, source, inheritance, read-only state, dirty state, and help information in the compact dialog layout.
  2. User can reset a writable dirty option to its current effective reference and reset a dirty group without affecting an identically named group on another Process page.
  3. User can submit only supported typed, bounded, and enumerated values and receives a clear result when validation or read-only protection rejects input.
  4. User sees unsupported vector Process values as explicitly limited scalar/status presentation, never as a multi-nozzle or per-extruder editor.
**Plans**: 2 plans
**UI hint**: yes

### Phase 229: Preset Feedback And Workflow Safety
**Goal**: Users retain accurate Process preset feedback and lifecycle safeguards
while navigating the restored hierarchy and editing Process values.
**Depends on**: Phase 228
**Requirements**: PSET-01, PSET-02
**Success Criteria** (what must be TRUE):
  1. User can distinguish dirty, compatible, incompatible, and read-only Process preset states before choosing an action.
  2. User retains correct Save, Save As, discard, and close-guard behavior after navigating pages and groups or editing Process values.
  3. User sees the existing slice-invalidation behavior after a Process edit or accepted preset workflow action, without losing the current preset selection or compatibility feedback.
**Plans**: 1 plan
**UI hint**: yes

### Phase 230: Process Workflow Evidence
**Goal**: Reviewers can verify that the restored Process Settings workflow is
source-aligned, interactive, and visually stable without widening the milestone scope.
**Depends on**: Phase 229
**Requirements**: VER-01
**Success Criteria** (what must be TRUE):
  1. A reviewer can run automated checks that prove the source order, page-qualified grouping, disclosure, search reveal, Advanced mode, reset, preset feedback, and slice invalidation behavior.
  2. A reviewer can exercise the Process dialog end to end and observe correct behavior after navigation, edits, resets, preset changes, and guarded close or discard actions.
  3. A reviewer can inspect the fixed-size Process dialog and see stable tabs, group headers, long metadata, read-only and dirty states, reset affordances, and compatibility feedback without clipping or overlap.
**Plans**: 1 plan
**UI hint**: yes

## Progress

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 226. Source-Mapped Process Hierarchy | 1/1 | Complete    | 2026-08-13 |
| 227. Process Projection, Disclosure, And Filtering | 0/2 | Not started | - |
| 228. Typed Process Editing | 0/2 | Not started | - |
| 229. Preset Feedback And Workflow Safety | 0/1 | Not started | - |
| 230. Process Workflow Evidence | 0/1 | Not started | - |
