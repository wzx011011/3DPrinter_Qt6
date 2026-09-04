# Phase 227: Process Projection, Disclosure, And Filtering - Context

**Gathered:** 2026-08-13
**Status:** Ready for planning

<domain>
## Phase Boundary

This phase restores the interaction layer for the FFF Process (`print`) branch
of `SettingsDialog`. It covers page-qualified group projection, session-only
disclosure, active-page search, Simple/Advanced mode filtering, and immediate
refresh after model or preset state changes.

It does not add typed editing, reset controls, validation feedback, preset
lifecycle UI, printer/material hierarchy, multi-nozzle or per-extruder editing.
</domain>

<decisions>
## Locked Decisions

### Ownership

- Add a Process-only C++ `QAbstractListModel` presentation projection exposed
  by `ConfigViewModel`.
- The projection owns active page, search text, Advanced mode, and session
  collapse keys. QML renders rows and forwards semantic interaction only.
- Collapse identity uses an untranslated page-qualified technical key. It is
  not persisted in presets, projects, or `QSettings`.

### Source and filtering contract

- `ConfigOptionModel` remains the source-mapped page/group/order authority.
- `ConfigViewModel::filterOptionIndices("print", ...)` remains the sole
  authority for query matching and mode visibility.
- Search is limited to the active Process page. It searches keys and labels,
  survives page and mode changes, and temporarily reveals matching collapsed
  groups without changing their saved collapse state.
- Advanced must be a strict superset of Simple using the existing C++ mode
  route, including upstream Advanced and Develop options.

### Interaction and visual contract

- Replace the decorative group marker with a dense, full-width Process group
  header. Pointer activation and Enter/Space activation both toggle exactly
  one group.
- The disclosure glyph, focus behavior, and accessible name must reflect the
  current effective expanded state.
- Only non-empty groups on the active page render. Headers report visible and
  dirty option counts from the C++ projection.
- Preserve the existing compact settings-dialog geometry and theme. Do not
  restore the retired `GroupNavSidebar`.

### Refresh contract

- Rebuild the projection after option values, references, read-only state,
  preset state, page, query, and mode changes. Do not cache copied values or
  stale option indices across a preset change.
</decisions>

<code_context>
## Implementation Surfaces

- Upstream hierarchy truth: `third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp`
  (`TabPrint::build`). Upstream has static group titles; interactive disclosure
  is an approved OWzx presentation requirement, not a claimed upstream widget.
- Schema/hierarchy truth: `src/qml_gui/Models/ConfigOptionModel.*`.
- Existing filter and preset state: `src/core/viewmodels/ConfigViewModel.*`.
- Process dialog: `src/qml_gui/dialogs/SettingsDialog.qml`.
- Existing row presentation: `src/qml_gui/components/OptionRow.qml`.
- New Process header and C++ projection must be registered through the normal
  CMake and QML resource paths.
</code_context>

<verification>
## Required Evidence

- Unit coverage for deterministic flattened rows, page-qualified collapse,
  search reveal, active-page filtering, Advanced superset, counts, and refresh.
- QML source/runtime audit for an accessible interactive header and Process-only
  projection usage.
- The canonical build command and existing application smoke/E2E suite pass.
</verification>
