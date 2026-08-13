# Requirements: OWzx Slicer

**Milestone:** v5.11 Process Settings Source-Truth Convergence
**Defined:** 2026-08-03
**Core Value:** OrcaSlicer upstream behavior is the product source of truth;
Qt6 code must inherit that behavior and must not invent new product behavior
without an explicit upstream mapping or documented block.

## v5.11 Requirements

### HIER: Process Hierarchy And Navigation

- [x] **HIER-01**: User can see the Process pages in the upstream order:
  Quality, Strength, Speed, Support, Multimaterial, and Others.
- [x] **HIER-02**: User can reach every supported Process option from its
  upstream-derived page and group.
- [ ] **HIER-03**: User can see the visible-option and dirty-option count for
  each Process group on the active page.

### DISC: Group Disclosure

- [ ] **DISC-01**: User can toggle one Process group with a pointer or keyboard
  without changing another group.
- [ ] **DISC-02**: User retains each Process group state while changing page,
  search text, Advanced mode, preset, or dialog visibility during the session.
- [ ] **DISC-03**: User can see search matches in a collapsed Process group;
  clearing search restores its saved disclosure state.
- [ ] **DISC-04**: User sees only non-empty Process groups and can identify a
  group's real expanded/collapsed state through its glyph, focus, and accessible
  name.

### FILT: Search And Mode Behavior

- [ ] **FILT-01**: User can search option keys and labels within the active
  Process page, and the query persists while the user changes page or mode.
- [ ] **FILT-02**: User sees every Simple Process option in Advanced mode plus
  the additional upstream Advanced and Develop options.
- [ ] **FILT-03**: User sees Process rows and counts refresh without reopening
  the dialog after an edit, reset, preset switch, or read-only-state change.

### ROW: Typed Option Rows

- [ ] **ROW-01**: User can identify an option's label, value, unit, source,
  inheritance, read-only state, dirty state, and help information without a
  layout regression.
- [ ] **ROW-02**: User can reset a writable dirty Process option to its current
  effective reference value.
- [ ] **ROW-03**: User can reset the dirty options of a Process group without
  affecting an identically named group on another page.
- [ ] **ROW-04**: User can submit only supported typed, bounded, and enumerated
  Process values and receives a clear validation result when input is rejected.
- [ ] **ROW-05**: User sees an honest limited representation for unsupported
  vector Process options; the UI does not present them as multi-nozzle editors.

### PSET: Preset Feedback And Workflow Safety

- [ ] **PSET-01**: User can distinguish dirty, compatible, incompatible, and
  read-only Process preset states.
- [ ] **PSET-02**: User retains correct save, Save As, discard, close-guard,
  and slice-invalidation behavior after Process navigation and edits.

### VER: Workflow Evidence

- [ ] **VER-01**: Automated and visual verification proves Process hierarchy,
  disclosure, search, mode, reset, preset feedback, and slice invalidation.

## Future Requirements

### Search And Configuration Scope

- **FUT-01**: User can navigate search results across Process pages with a
  source-mapped page/group destination.
- **FUT-02**: User receives equivalent source-truth hierarchy improvements for
  Printer and Material settings after separate scope approval.
- **FUT-03**: User can edit full vector/per-extruder values after multi-nozzle
  product scope is explicitly approved.

## Out Of Scope

| Feature | Reason |
|---------|--------|
| H2C/A2L, multi-nozzle, AMS, or per-extruder editors | Separate product decision and architecture scope. |
| Printer and Material settings feature expansion | v5.11 is limited to the Process tier. |
| Device, hardware, network, cloud, camera, or monitor behavior | Removed product scope unless explicitly reopened. |
| SLA settings or slicing | Permanently removed from scope on 2026-07-26. |
| libslic3r algorithm changes | This is a GUI migration milestone. |
| Unmapped cross-option auto-correction | Requires its own OrcaSlicer source mapping and behavior tests. |
| Persisting disclosure state in presets, projects, or QSettings | v5.11 disclosure state is session-only presentation state. |

## Traceability

| Requirement | Phase | Status |
|-------------|-------|--------|
| HIER-01 | Phase 226 | Complete |
| HIER-02 | Phase 226 | Complete |
| HIER-03 | Phase 227 | Pending |
| DISC-01 | Phase 227 | Pending |
| DISC-02 | Phase 227 | Pending |
| DISC-03 | Phase 227 | Pending |
| DISC-04 | Phase 227 | Pending |
| FILT-01 | Phase 227 | Pending |
| FILT-02 | Phase 227 | Pending |
| FILT-03 | Phase 227 | Pending |
| ROW-01 | Phase 228 | Pending |
| ROW-02 | Phase 228 | Pending |
| ROW-03 | Phase 228 | Pending |
| ROW-04 | Phase 228 | Pending |
| ROW-05 | Phase 228 | Pending |
| PSET-01 | Phase 229 | Pending |
| PSET-02 | Phase 229 | Pending |
| VER-01 | Phase 230 | Pending |

**Coverage:**
- v5.11 requirements: 18 total
- Mapped to phases: 18
- Unmapped: 0

---
*Requirements defined: 2026-08-03*
*Last updated: 2026-08-03 after v5.11 roadmap creation*
