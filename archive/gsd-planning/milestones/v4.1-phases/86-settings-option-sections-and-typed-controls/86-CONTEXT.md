# Phase 86: Settings Option Sections And Typed Controls - Context

**Gathered:** 2026-07-07
**Status:** Ready for planning
**Mode:** Autonomous smart discuss defaults

<domain>
## Phase Boundary

Phase 86 restores the visible option-content region inside the already restored
settings dialog shell. It owns section headers, row density, typed row visuals,
unit/sidetext presentation, paired min/max range rows, and compact state
indicators for printer, material, and process settings.

In scope:

- `OptionRow.qml` section-header and row renderer visuals.
- Settings-dialog delegate sizing/metadata passed into `OptionRow.qml`.
- Source/QML audits that lock section, typed-control, and state-indicator
  structure.
- Targeted viewmodel smoke regressions needed to prove existing edit wiring is
  still routed through C++ models.

Out of scope:

- Reworking the Phase 85 settings shell, top action row, or tab labels.
- Preset save/reset/discard/unsaved-close semantic hardening beyond preserving
  current wiring; Phase 87 owns that work.
- Final runtime screenshot comparison and stale-path cleanup; Phase 88 owns the
  full verification bundle.
- LAN/device/cloud/network, Monitor, ModelMall/Home WebView, camera streams,
  AssembleView, D3D12/Vulkan, and full upstream preset-bundle compatibility.

</domain>

<decisions>
## Implementation Decisions

### Section Flow

- **D-01:** Keep the Phase 85 shell intact and restore only the option-content
  region. `SettingsDialog.qml` remains the host; `OptionRow.qml` remains the
  renderer.
- **D-02:** Use screenshot-like compact section headers: small cyan icon/glyph
  rail, bold section title, and a horizontal divider that continues through the
  content pane. Do not introduce nested cards.
- **D-03:** Drive sections from existing `ConfigOptionModel::optGroup()` and
  existing filtered index order. Do not add a new QML-only grouping model.
- **D-04:** Preserve stable scroll behavior by keeping delegate height derived
  from `OptionRow.totalHeight`; hover, status labels, and dynamic indicators
  must not resize rows after creation.

### Typed Controls

- **D-05:** Preserve the existing C++ edit path: all edits route through
  `optionModel.setValue(optIdx, value)`. QML may format and clamp values for
  presentation only.
- **D-06:** Use project controls for visible editor widgets: `CxCheckBox` for
  bool rows, `CxSpinBox` or compact numeric edit frames for numeric rows,
  `CxComboBox` for enum rows, `CxTextField`/`CxTextArea` for string rows, and
  a compact color button for color-like string keys.
- **D-07:** Prefer upstream `sidetext` from `ConfigOptionModel::optSidetext()`;
  fall back to `optUnit()` only when sidetext is empty. The unit text must sit
  inside the fixed control cluster so rows do not jump.
- **D-08:** Render paired min/max rows for range-like keys detected by existing
  key/name patterns such as `_range`, `_min`, `_max`, `fan_min_speed`,
  `fan_max_speed`, and `nozzle_temperature_range`. This phase may present
  one option's min/max bounds compactly; deeper data-model pairing belongs to a
  later source-truth preset milestone if true multi-key editing is required.
- **D-09:** Material color-like rows should show a swatch/button affordance
  while preserving the current string value path. Do not add a new color
  persistence backend in QML.

### State Indicators

- **D-10:** Dirty, value-source, read-only, nullable/inherit, vector/per-extruder,
  and validation states must be visible in a fixed metadata lane. They may be
  compact badges/icons, but they cannot appear as variable-height text blocks.
- **D-11:** Disabled/read-only controls should reduce interaction affordance
  without hiding the label, unit, or current value.
- **D-12:** Validation visibility for numeric bounds should be explicit at the
  row edge using the model's min/max metadata, even if backend validation is
  still deeper than Phase 86.

### Verification Routing

- **D-13:** Add RED-first QML source audit coverage for the Phase 86 row
  contract before production QML edits.
- **D-14:** Run targeted `QmlUiAuditTests` and relevant `ViewModelSmokeTests`
  before the final canonical verifier.
- **D-15:** Run the global encoding guard for touched QML, C++, and planning
  docs before committing.

### the agent's Discretion

- The implementation may use small local QML helper functions/properties for
  visual classification, as long as business semantics remain in C++ models.
- The exact badge copy and glyphs may be chosen from existing Theme/assets when
  the screenshots do not specify an exact symbol.
- The plan may add static source audits instead of fragile screenshot automation
  because Phase 88 owns final runtime visual evidence.

</decisions>

<canonical_refs>
## Canonical References

### Planning Scope

- `.planning/REQUIREMENTS.md` - `SETCTRL-01`, `SETCTRL-02`, and `SETCTRL-03`.
- `.planning/ROADMAP.md` - Phase 86 success criteria.
- `.planning/STATE.md` - current v4.1 scope guard and removed network/device
  scope.
- `.planning/phases/84-settings-source-truth-gap-audit/84-GAP-MATRIX.md` -
  `SET-SECTIONS`, `SET-TYPED-ROWS`, and `SET-STATE-INDICATORS`.
- `.planning/phases/85-settings-shell-and-tab-layout-restoration/85-CONTEXT.md`
  - Phase 85 shell decisions to preserve.
- `.planning/phases/85-settings-shell-and-tab-layout-restoration/85-01-SUMMARY.md`
  - restored shell handoff.

### Visual Truth

- `shotScreen/打印机参数设置页.png` - printer settings target.
- `shotScreen/材料参数设置页.png` - material settings target.

### Current Qt Targets

- `src/qml_gui/components/OptionRow.qml` - primary Phase 86 target.
- `src/qml_gui/dialogs/SettingsDialog.qml` - delegate host and compact sizing.
- `src/qml_gui/panels/LeftSidebar.qml` - secondary `OptionRow` consumer.
- `src/qml_gui/Models/ConfigOptionModel.h`
- `src/qml_gui/Models/ConfigOptionModel.cpp`
- `src/core/viewmodels/ConfigViewModel.h`
- `src/core/viewmodels/ConfigViewModel.cpp`
- `tests/QmlUiAuditTests.cpp`
- `tests/ViewModelSmokeTests.cpp`

### OrcaSlicer Source Truth

- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.hpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/OptionsGroup.hpp`
- `third_party/OrcaSlicer/src/slic3r/GUI/OptionsGroup.cpp`
- `third_party/OrcaSlicer/src/libslic3r/PrintConfig.hpp`
- `third_party/OrcaSlicer/src/libslic3r/PrintConfig.cpp`

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets

- `OptionRow.qml` already exposes typed dispatch for bool, int, double,
  percent, enum, and string rows.
- `SettingsDialog.qml` already computes filtered option indices and passes
  `showGroupHeader`, `oGroup`, `valueSource`, and compact width knobs into
  `OptionRow`.
- `LeftSidebar.qml` also consumes `OptionRow`, so `OptionRow` changes must stay
  compatible with a narrower sidebar.
- `ConfigOptionModel` exposes key, label, type, min, max, step, enum labels,
  group, page, read-only, dirty, tooltip, nullable, vector, and sidetext/unit
  metadata.
- `ConfigViewModel` exposes `valueSourceForKey`, reset/value-source helpers,
  preset dirty state, and per-tier option models.

### Established Patterns

- QML is presentation and wiring only; durable settings behavior remains in
  C++ viewmodels/services.
- Custom controls use the `Cx*` family.
- Source/QML audit tests are accepted in this codebase for locking visual
  contracts that will later be proven at runtime.
- Screenshot-driven milestones use screenshots for visual truth and OrcaSlicer
  source for behavior truth.

### Integration Points

- `SettingsDialog.qml` delegates rows through `OptionRow` and binds
  `optionModel.setValue`.
- `ConfigOptionModel::optSidetext()` and `optUnit()` provide unit display
  without QML regex guessing.
- `ConfigOptionModel::optNullable()` and `optIsVector()` already expose the
  state metadata required by Phase 86.
- `ConfigViewModel::valueSourceForKey()` provides the current value-source lane.

</code_context>

<specifics>
## Specific Ideas

- Printer screenshot section headers show a cyan icon at the left, title text,
  and a divider line continuing across the pane.
- Material screenshot rows use a tight label column, compact editor column, and
  unit suffixes directly adjacent to values.
- Motion/temperature examples show min/max labels and two numeric fields on one
  row; Phase 86 should restore that visual pattern where existing metadata
  allows it.
- Existing QML strings contain encoding artifacts; Phase 86 should avoid adding
  new user-visible mojibake.

</specifics>

<deferred>
## Deferred Ideas

- Full preset save/reset/discard/close workflow hardening belongs to Phase 87.
- Runtime screenshot comparison and stale settings path cleanup belong to
  Phase 88.
- New multi-key range persistence, if required beyond visual restoration, should
  be planned as a later preset/model-source milestone.
- D3D12/Vulkan and all LAN/device/cloud/network work remain outside v4.1.

</deferred>

---

*Phase: 86-Settings Option Sections And Typed Controls*
*Context gathered: 2026-07-07*
