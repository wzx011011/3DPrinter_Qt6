# Phase 56: Parameter Settings Dialogs Restoration - Research

**Researched:** 2026-07-02
**Domain:** C++17/Qt6.10/QML desktop settings dialogs driven by libslic3r `print_config_def` metadata
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Three independent non-modal settings dialogs (printer, material, process) -- each a separately-focusable ApplicationWindow, matching upstream Tab.cpp. Not modal popups, not embedded pages.
- Separate dialogs, not one combined. One per PresetCollection scope.
- Open path: Prepare left-sidebar per-section "Setting" button via existing `settingsRequested(category)` signal (Phase 52 wired as logged no-op). BackendContext forwards to open correct dialog.
- Close path: X button or Esc. On close/tab-switch/preset-switch when dirty -> show UnsavedChangesDialog (save/discard/cancel). Do NOT auto-discard.
- Process settings built by upstream Tab.cpp print-process parity (no screenshot). Same shell, option-group nav, typed option model, save/reset semantics.
- Option list source: driven from real upstream `Slic3r::print_config_def`, filtered by settings category and basic/advanced level. No hand-curated subset.
- Typed-control coverage: all seven option types -- bool, int, float, enum, string, percent+unit, nullable (inherit), multi-value (per-extruder).
- Units: read from print_config_def option metadata (sidetext), rendered as suffix on control.
- Null/inheritance: null option value = inherit-from-parent-preset. UI shows inherited value with value-source indicator.
- Dirty granularity: per-option dirty + per-preset global dirty, with inline modified-option markers. Mirrors upstream Tab.cpp optcache diff.
- Value source / read-only: surface inherited-vs-overridden source, gate builtin presets read-only.
- Reset levels: three -- reset-option, reset-group, reset-all.
- Save vs Save As: Save overwrites user preset; builtin preset Save blocked, forces Save As.
- Unsaved-change guard: existing UnsavedChangesDialog guards close/preset-switch/category-switch.
- Search scope: per-dialog. Filtered/no-match states useful and source-truth-aligned.
- Basic/Advanced toggle: driven from print_config_def option mode (advanced vs basic).
- Slice invalidation: reuse Phase-52 connect (ConfigViewModel::stateChanged -> EditorViewModel::invalidateSliceResultsForAllPlates). Any option edit or preset change invalidates stale slice results.
- Project save/restore: dirty user-overrides persist via existing 3MF scoped-config-override path. Users NOT forced to save-as-preset before project save.

### Claude's Discretion
- Exact QML dialog component decomposition (one parameterized shell vs three near-identical files) -- prefer single parameterized shell + per-category category/group model injection.
- Exact ConfigOptionModel extension shape (role-based vs type-based dispatch) for the 7 option types, as long as all 7 covered.
- Whether PresetListModel becomes canonical model for new dialogs or ConfigOptionModel is extended.
- Signal/slot wiring for settingsRequested(category) -> dialog open in BackendContext.
- Test file placement (extend ViewModelSmokeTests.cpp / E2EWorkflowTests.cpp vs new SettingsDialogTests.cpp).

### Deferred Ideas (OUT OF SCOPE)
- Removal of replaced embedded surface (SettingsPage.qml, ConfigPage.qml, ParamsPage.qml, SearchDialog.qml + qrc/routes/imports) -> Phase 57.
- Filament color metadata wiring.
- "Compare presets" and per-object advanced-settings overrides inside the dialog.
- Cross-category global search.
- Full hardware-calibration completion beyond preset read/write.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SETTINGS-01 | Open independent printer/material/process settings dialogs that match screenshots, not off-design embedding | CONTEXT.md dialog architecture locked; 50-INVENTORY region IDs frozen; BackendContext::settingsRequested signal already exists |
| SETTINGS-02 | Navigate top tabs and option groups for printer/material settings with screenshot-aligned density | Upstream Tab.cpp page/group names documented in Research (TabPrint/TabFilament/TabPrinter); ConfigOptionModel pageNames()/filterIndicesByPage() already exist |
| SETTINGS-03 | Edit typed config options through C++ option models covering bool/num/enum/string/units/nullable/multi-value | ConfigOptionDef struct fully documented; ConfigOptionModel already has roles for key/label/type/value/min/max/step/enum/category/group/page/readonly/dirty/tooltip/mode; extensions needed for string/percent/nullable/multi-value |
| SETTINGS-04 | See dirty state, modified-option indicators, inherited/default value source, read-only, validation warnings | ConfigOptionModel has dirtyKeys/DirtyRole/referenceValues; ConfigViewModel has valueSourceForKey/globalModifiedCount/resetGlobalOption/resetAllGlobalOptions; need per-group dirty count |
| SETTINGS-05 | Save, Save As, reset option/group/all, discard, cancel unsaved changes | ConfigViewModel has requestSavePendingChanges/requestDiscardPendingChanges/resetOptionToLevel/resetGlobalOption/resetAllGlobalOptions/saveCurrentPreset/createCustomPreset; need resetGroup |
| SETTINGS-06 | Search settings, toggle basic/advanced, see filtered/no-match states | ConfigViewModel has filterOptionIndices/searchOptions with fuzzy matching; ConfigOptionModel has mode field; CxSwitch control available |
| SETTINGS-07 | Settings changes update Prepare sidebar state, slice invalidation, merged slicing config, project save/restore | Phase-52 connect (ConfigViewModel::stateChanged -> EditorViewModel::invalidateSliceResultsForAllPlates) already wired; mergedConfigValues() exists; applyProjectConfig() exists |
</phase_requirements>

## Summary

Phase 56 builds three independent non-modal `ApplicationWindow` settings dialogs (printer, material, process) that replace the off-design embedded `SettingsPage.qml`. The dialogs are driven by the upstream `Slic3r::print_config_def` metadata schema, exposed through an extended `ConfigOptionModel` that already handles bool/int/float/enum but needs string/percent/nullable/multi-value coverage. The upstream `ConfigOptionDef` struct exposes type, category, mode (basic/advanced), min/max, enum values/labels, tooltip, sidetext (unit), readonly flag, nullable flag, and default value -- but NO step field (step is synthesized). The `ConfigViewModel` already provides save/saveas/discard/reset/valueSource/dirty infrastructure, but needs per-group reset.

The biggest technical gap is the **page-to-group navigation** mapping: upstream Tab.cpp uses `add_options_page(title)` to create pages (tabs) and `page->new_optgroup(title)` to create option groups within each page, but options are **manually placed** into specific groups via `optgroup->append_single_option_line()` calls -- there is no metadata field on `ConfigOptionDef` that maps an option to a specific group. The Qt side must either (a) hard-code the page/group mapping per category (like the existing `kMachineKeys[]`/`kFilamentKeys[]` arrays) or (b) extract it from upstream source at build time. Given the UI-SPEC already locks tab names and group names, option (a) is the pragmatic path.

**Primary recommendation:** Extend `ConfigOptionModel` with string type support, nullable flag, sidetext from upstream, and a `group` field per option that is populated from a static mapping table derived from upstream `Tab.cpp` `new_optgroup` / `append_single_option_line` calls. Build a single parameterized `SettingsDialog.qml` shell with C++ backend models driving the three category scopes.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Option schema extraction (print_config_def -> typed model) | API / Backend (C++ ConfigOptionModel) | -- | Data mapping from libslic3r to Qt, no UI concern |
| Dirty / save / reset / value-source semantics | API / Backend (C++ ConfigViewModel) | -- | Business logic for preset lifecycle |
| Page/group navigation model | API / Backend (C++ ConfigOptionModel) | -- | Structure derived from upstream Tab.cpp |
| Preset CRUD (save/saveas/delete/reset) | API / Backend (C++ PresetServiceMock + ConfigViewModel) | -- | Preset lifecycle is backend state |
| Builtin read-only gating | API / Backend (C++ ConfigViewModel) | -- | Preset access control |
| Search / filter (fuzzy match) | API / Backend (C++ ConfigViewModel) | -- | Search logic belongs in C++ per qml-boundaries rule |
| Dialog shell (ApplicationWindow) | Frontend (QML) | -- | Pure presentation frame |
| Typed option rendering (bool/num/enum/string) | Frontend (QML) | -- | Visual control dispatch based on option type |
| Group nav / tab switching | Frontend (QML) | Backend (C++ filterIndicesByPage) | UI interaction driven by C++ model |
| Unsaved-changes guard dialog | Frontend (QML UnsavedChangesDialog) | Backend (C++ requestSavePendingChanges) | Existing reusable dialog |
| Slice invalidation on edit | API / Backend (C++ ConfigViewModel -> EditorViewModel) | -- | Already wired Phase 52 connect |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt 6.10 QML | 6.10 | ApplicationWindow for non-modal dialogs, ListView for option lists | Project locked to Qt 6.10 |
| Qt Quick Controls 2 | 6.10 | SpinBox, ComboBox, Switch, TextField base controls | Via Cx* wrappers |
| libslic3r PrintConfig.hpp | upstream v7.0.1 | ConfigOptionDef metadata schema (type, category, mode, min/max, enum, sidetext, nullable) | Source truth for all option metadata |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| CxSpinBox | project-internal | Integer numeric input | Int option type (NOT float -- Qt6 SpinBox.stepSize is int-only) |
| CxComboBox | project-internal | Enum selection, preset selection | Enum option type |
| CxSwitch | project-internal | Boolean toggle | Bool option type |
| CxTextField | project-internal | String input | String option type |
| CxSlider | project-internal | Numeric range slider | Numeric option types alongside spinbox/input |
| CxButton | project-internal | Action buttons | Footer Save/Discard/Cancel, Reset, Save As |
| CxScrollView | project-internal | Scrollable group nav and option list | Group navigation sidebar |
| CxDialog | project-internal | Modal dialogs (UnsavedChanges, SavePreset, Delete confirm) | Guard and confirmation dialogs |
| Theme singleton | project-internal | All visual tokens | Every visual element |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| CxSpinBox (int only) + custom float TextInput for float options | QML SpinBox with custom step via int scaling | Qt6 SpinBox.stepSize accepts only int; float step requires either (a) TextInput + DoubleValidator, (b) int-scaled SpinBox with textFromValue/valueFromText, or (c) custom control. UI-SPEC already specifies TextInput for float. |
| Single parameterized SettingsDialog.qml | Three separate dialog QML files | Triplication of ~80% identical QML code. Parameterized shell avoids this. |
| Static group mapping table | Runtime reflection from Tab.cpp | Tab.cpp groups are manually constructed, no metadata to reflect. Static table is source-truth. |

**Installation:** No external packages to install. All dependencies are project-internal.

**Version verification:** Not applicable -- no new packages.

## Package Legitimacy Audit

> No new external packages are installed in this phase. All components are project-internal (Cx* controls, Theme, existing viewmodels/services). This section is not applicable.

## Architecture Patterns

### System Architecture Diagram

```
Prepare Left Sidebar                    Settings Dialog (printer/material/process)
[Setting button]                       +---------------------------------------+
       |                               | Title Bar: [Preset Name] [ComboBox]   |
       v                               |   [Modified Badge] [SaveAs] [X Close] |
BackendContext::openSettings(cat)      +---------------------------------------+
       |                               | Preset Bar: [ComboBox] [Modified]      |
       v                               |   [Save] [SaveAs] [Delete] [Reset All] |
ConfigViewModel::setActivePresetTier() +---------------------------------------+
       |                               | Tab Strip: [Quality] [Strength] [Speed]|
       v                               +---------------------------------------+
SettingsDialog(category=tier)          | Group Nav  | Option Editing Area         |
  - Creates 1x per category           | [All]      | [Group Header]               |
  - Owned by BackendContext             | [Group 1]  | [Dirty dot] Label Control    |
  - Shown/hidden, not destroyed       | [Group 2]  | [Dirty dot] Label Control    |
       |                               | [Group 3]  | [Dirty dot] Label Control    |
       +---> ConfigViewModel            +---------------------------------------+
              |                        | Search Bar: [Search...] [Advanced]    |
              +---> ConfigOptionModel   |   {N matched}  {M modified}           |
              |     (per tier)         +---------------------------------------+
              +---> PresetServiceMock   | Footer: [Save] [Discard] [Cancel]    |
                                       +---------------------------------------+

Option Edit Flow:
QML control.onChanged -> ConfigOptionModel.setValue(row, v)
  -> emit optionValueChanged(key, v)
    -> ConfigViewModel.handleOptionValueChanged(key, v)
      -> update tier-specific presetValues hash
        -> updateMergedPresetValues()
          -> applyScopeValues()
            -> emit stateChanged()
              -> [Phase-52 wired] EditorViewModel.invalidateSliceResultsForAllPlates()

Save Flow:
QML Save button -> ConfigViewModel.requestSavePendingChanges()
  -> if builtin: emit saveAsRequired() -> show SavePresetDialog
  -> else: saveCurrentPreset() -> PresetServiceMock.savePresetValues()

Dirty Guard Flow:
Dialog close / preset switch -> check ConfigViewModel.isPresetDirty()
  -> if dirty: show UnsavedChangesDialog
    -> Save: requestSavePendingChanges() then close
    -> Discard: requestDiscardPendingChanges() then close
    -> Cancel: abort, dialog stays open
```

### Recommended Project Structure
```
src/
  qml_gui/
    dialogs/
      SettingsDialog.qml          # Single parameterized non-modal ApplicationWindow
      SavePresetDialog.qml        # (existing, reused)
      UnsavedChangesDialog.qml    # (existing, reused)
    components/
      OptionRow.qml               # Typed option renderer (dispatches by type)
      GroupNavSidebar.qml          # Left option-group navigation list
      SearchBarPanel.qml          # Search + advanced toggle bar (or inline)
    controls/
      CxUnitSpinBox.qml           # NEW: SpinBox variant with unit suffix property
  core/
    viewmodels/
      ConfigViewModel.h/.cpp       # EXTEND: per-group reset, nullable per-option, groupNames()
      SettingsDialogController.h/.cpp  # NEW: owns dialog instances, wires open/close/save
    services/
      PresetServiceMock.h/.cpp    # (existing, reused as-is)
  Models/
    ConfigOptionModel.h/.cpp       # EXTEND: string type, nullable flag, per-group metadata
```

### Pattern 1: Parameterized Dialog Shell
**What:** Single `SettingsDialog.qml` with a `required property string presetTier` that the C++ backend sets to "printer"/"filament"/"print". The dialog reads all data from `ConfigViewModel` methods scoped by the active tier.
**When to use:** When three dialogs share 90%+ identical layout and behavior.
**Example:**
```qml
// SettingsDialog.qml
ApplicationWindow {
    id: root
    required property var configVm
    required property string presetTier  // "printer" | "filament" | "print"
    title: presetTier === "printer" ? qsTr("Printer Settings")
          : presetTier === "filament" ? qsTr("Material Settings")
          : qsTr("Process Settings")
    // ... all layout uses configVm methods scoped by presetTier
}
```

### Pattern 2: Typed Option Row Dispatch
**What:** A single `OptionRow.qml` component that checks `optType` and renders the appropriate Cx* control (CxSwitch for bool, CxSpinBox+Slider for int, TextInput for float, CxComboBox for enum, TextArea for string).
**When to use:** When a list view must render heterogeneous option types.
**Example:**
```qml
// OptionRow.qml
Loader {
    id: controlLoader
    sourceComponent: {
        switch(optType) {
        case "bool":    return boolComponent
        case "int":     return intComponent
        case "double":  return doubleComponent
        case "enum":    return enumComponent
        case "string":  return stringComponent
        default:        return doubleComponent
        }
    }
}
```

### Pattern 3: Float Step via TextInput (not SpinBox)
**What:** Qt6 QML SpinBox.stepSize only accepts int. For float options with non-integer step (e.g., 0.01), use a `TextInput` with `DoubleValidator` instead of `CxSpinBox`. This is already documented in the debugging rules and aligned with the UI-SPEC.
**When to use:** All float/double option types.
**Example:**
```qml
// Float numeric control (from UI-SPEC)
Rectangle {
    width: 64; height: 24; radius: Theme.radiusSM
    color: Theme.bgInset
    border.color: control.activeFocus ? Theme.borderFocus : Theme.borderInput
    TextInput {
        anchors.fill: parent
        anchors.margins: 6
        validator: DoubleValidator { decimals: 3 }
        horizontalAlignment: TextInput.AlignRight
        onEditingFinished: { optionModel.setValue(row, text) }
    }
}
```

### Anti-Patterns to Avoid
- **Using QML SpinBox for float step**: Qt6 SpinBox.stepSize is int-only. Must use TextInput + DoubleValidator for float options.
- **Hand-curating option lists instead of driving from print_config_def**: The Preset Rule requires driving from upstream schema. The existing `loadSchemaFromKeys()` pattern is correct.
- **Putting save/reset business logic in QML**: All preset lifecycle logic belongs in ConfigViewModel/PresetServiceMock per qml-boundaries rule.
- **Creating three separate QML dialog files**: Triplication. Use single parameterized shell.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Fuzzy search algorithm | Custom string matching | `ConfigViewModel::searchOptions()` + `filterOptionIndices()` | Already implemented with scoring-based fuzzy matching aligned to upstream fts_fuzzy_match |
| Enum-to-label mapping | Manual switch per option key | `ConfigOptionModel::optEnumLabelsList()` + upstream `ConfigOptionDef::enum_labels` | Already mapped from upstream metadata |
| Unit detection heuristics | QML key-name regex | `ConfigOptionModel::optUnit()` with upstream `sidetext` fallback | Already reads sidetext from ConfigOptionDef; heuristic is fallback only |
| Preset dirty detection | Manual value comparison | `ConfigOptionModel::m_dirtyKeys` + `ConfigViewModel::isPresetDirty()` | Already implemented with reference-value diff |
| Unsaved changes dialog | New modal QML | `UnsavedChangesDialog.qml` (existing) | Already exists with save/discard/cancel pattern |
| Save preset dialog | New modal QML | `SavePresetDialog.qml` (existing) | Already exists with name input + duplicate check |
| Basic/Advanced filtering | QML visibility toggle per option | `ConfigOptionModel::optMode()` + `ConfigViewModel::filterOptionIndices(advancedMode)` | Already implemented with upstream ConfigOptionMode |

**Key insight:** The existing ConfigOptionModel + ConfigViewModel already provide 70% of the infrastructure needed. Phase 56's job is (a) building the QML dialog shell, (b) extending the option model for missing types, (c) adding per-group reset, and (d) wiring the open/close/save flows through BackendContext.

## Common Pitfalls

### Pitfall 1: Qt6 SpinBox stepSize is int-only
**What goes wrong:** Attempting `SpinBox { stepSize: 0.01 }` compiles but stepSize silently rounds to 0 or 1. Float options get no stepping.
**Why it happens:** Qt6 QML SpinBox.stepSize type is `real` but the internal stepper uses integer arithmetic. This is a known Qt6 limitation documented in the project's debugging rules.
**How to avoid:** Use `TextInput` + `DoubleValidator` for all float/double option types. Use `CxSpinBox` only for int options. The UI-SPEC already specifies this pattern.
**Warning signs:** Float options step by 1.0 or not at all in the UI.

### Pitfall 2: ConfigOptionDef has NO step field
**What goes wrong:** Trying to read `def->step` to set step size for numeric controls.
**Why it happens:** Upstream `ConfigOptionDef` (Config.hpp:2217-2527) has `min`, `max`, `mode`, `category`, `enum_values`, `enum_labels`, `tooltip`, `sidetext`, `readonly`, `nullable` -- but no `step` field.
**How to avoid:** Synthesize step from option type: float options default to 0.01, int options default to 1. The existing `loadSchemaFromKeys()` already does this (line 993-994 of ConfigOptionModel.cpp).
**Warning signs:** Compiler error `class ConfigOptionDef has no member named step`.

### Pitfall 3: ConfigOptionMode has FOUR levels, not two
**What goes wrong:** Treating basic/advanced as a boolean toggle. Upstream has comSimple(0), comAdvanced(1), comExpert(2), comDevelop(3).
**Why it happens:** The UI-SPEC says "Basic/Advanced toggle" but upstream has 4 levels. Current `filterOptionIndices()` already handles this by hiding mode>=1 when `advancedMode=false` and hiding mode==0 when `advancedMode=true`.
**How to avoid:** Use the existing filterOptionIndices(category, searchText, advancedMode) logic which correctly handles the 4-level enum. The toggle is effectively "show simple-only" vs "show advanced+expert+develop".
**Warning signs:** Options visible in upstream "Advanced" tab not appearing in Qt dialog.

### Pitfall 4: Nullable options need special representation
**What goes wrong:** Treating nullable options as regular options. A null value means "inherit from parent preset" -- the control must show the inherited value with a source indicator, not a blank or zero.
**Why it happens:** `ConfigOptionDef::nullable = true` is a separate concept from the option type. `ConfigOptionFloatOrPercent` is not the same as nullable. Nullable is a flag on `coFloats`, `coInts`, `coPercents`, `coBools`, `coEnums`, `coFloatsOrPercents`.
**How to avoid:** The `ConfigOption` struct needs a `bool nullable` field. When nullable=true and the value is nil, the QML control shows the inherited value from the parent tier (via `valueSourceForKey()`) with a "Default" or "Inherited from ..." indicator.
**Warning signs:** Nullable options showing "0" or empty instead of inherited value.

### Pitfall 5: Per-extruder multi-value options
**What goes wrong:** Options like `nozzle_temperature` that have one value per extruder. These are `coFloats` (vector type), not `coFloat` (scalar). The existing `loadSchemaFromKeys()` maps `coFloats` to type "double" which loses the vector dimension.
**Why it happens:** `ConfigOptionType` has a `coVectorType = 0x4000` flag. `coFloats = coFloat + coVectorType`. The `mapType()` function in ConfigOptionModel.cpp (line 588-611) maps both `coFloat` and `coFloats` to "double", losing the vector information.
**How to avoid:** Add a `bool isVector` field to `ConfigOption`. When `isVector=true`, the QML control renders an extruder selector above the option row. The `ConfigOptionDef::is_scalar()` method distinguishes scalar vs vector.
**Warning signs:** Multi-extruder printers show only one value for per-extruder options.

### Pitfall 6: Non-modal ApplicationWindow alongside main ApplicationWindow
**What goes wrong:** The settings dialog (ApplicationWindow) may not receive keyboard focus correctly, or Esc key may close both windows, or the dialog may appear behind the main window.
**Why it happens:** Qt6 QML supports multiple ApplicationWindows but focus/keyboard routing needs care. The main window must not be modal while the settings dialog is open.
**How to avoid:** Use `requestActivate()` when showing the dialog. Ensure the main window does not claim focus back. Test Esc key handling to ensure it only closes the dialog, not the main window.
**Warning signs:** Dialog appears but cannot receive keyboard input; Esc closes the whole app.

### Pitfall 7: Page-to-group mapping has no upstream metadata
**What goes wrong:** Trying to read a "group" field from `ConfigOptionDef` to determine which optgroup an option belongs to. No such field exists.
**Why it happens:** Upstream Tab.cpp manually places options into groups via `optgroup->append_single_option_line("option_key")`. The assignment is procedural, not declarative. `ConfigOptionDef::category` maps to the page/tab, not to the group within the page.
**How to avoid:** Build a static `QHash<QString, QString>` mapping (option_key -> group_name) derived from upstream Tab.cpp source for each category. This is already partially done for the print category in the hardcoded option list (the `group` field in `ConfigOption` constructor). For machine and filament, the same pattern needs to be applied based on `TabPrinter::build_fff()` and `TabFilament::build()` source.
**Warning signs:** All options appear in one giant group, or group names don't match upstream.

## Code Examples

### ConfigOptionDef struct (upstream source truth)
```cpp
// Source: third_party/OrcaSlicer/src/libslic3r/Config.hpp:2217-2527
class ConfigOptionDef
{
public:
    t_config_option_key   opt_key;           // e.g., "layer_height"
    ConfigOptionType     type = coNone;      // coFloat, coInt, coBool, coEnum, coString, etc.
    bool                  nullable = false;  // accepts nil value (inherit from parent)
    clonable_ptr<const ConfigOption> default_value;  // default ConfigOption instance

    GUIType               gui_type { GUIType::undefined };  // i_enum_open, f_enum_open, color, etc.
    std::string           gui_flags;         // "serialized", "show_value"
    std::string           label;             // short label for grouped field
    std::string           full_label;        // stand-alone field label
    PrinterTechnology     printer_technology = ptUnknown;
    std::string           category;          // e.g., "Layers and Perimeters", "Infill", "Speed"
    std::string           tooltip;           // GUI tooltip text
    std::string           sidetext;          // text right of input field, usually unit (mm, %, C)
    std::string           cli;
    std::string           cli_params;
    t_config_option_key   ratio_over;        // for coFloatOrPercent
    bool                  multiline = false;
    bool                  full_width = false;
    bool                  is_code = false;
    bool                  readonly = false;
    int                   height = -1;
    int                   width = -1;
    float                 min = -FLT_MAX;    // min for numeric input
    float                 max = FLT_MAX;     // max for numeric input
    double                max_literal = 1;   // for coFloatOrPercent
    ConfigOptionMode      mode = comSimple;  // 0=simple, 1=advanced, 2=expert, 3=develop

    std::vector<t_config_option_key> aliases;
    std::vector<t_config_option_key> shortcut;
    std::vector<std::string>  enum_values;   // enum value strings
    std::vector<std::string>  enum_labels;   // enum display labels
    const t_config_enum_values *enum_keys_map = nullptr;
};
```

### ConfigOptionType enum (upstream)
```cpp
// Source: third_party/OrcaSlicer/src/libslic3r/Config.hpp:158-197
enum ConfigOptionType {
    coVectorType = 0x4000,
    coNone = 0,
    coFloat = 1,         coFloats = coFloat + coVectorType,
    coInt = 2,           coInts = coInt + coVectorType,
    coString = 3,        coStrings = coString + coVectorType,
    coPercent = 4,        coPercents = coPercent + coVectorType,
    coFloatOrPercent = 5, coFloatsOrPercents = coFloatOrPercent + coVectorType,
    coPoint = 6,         coPoints = coPoint + coVectorType,
    coPoint3 = 7,
    coBool = 8,          coBools = coBool + coVectorType,
    coEnum = 9,          coEnums = coEnum + coVectorType,
    coPointsGroups = 10 + coVectorType,
    coIntsGroups = 11 + coVectorType
};
```

### ConfigOptionMode enum (upstream)
```cpp
// Source: third_party/OrcaSlicer/src/libslic3r/Config.hpp:199-204
enum ConfigOptionMode {
    comSimple = 0,
    comAdvanced,
    comExpert,
    comDevelop,
};
```

### Current ConfigOptionModel roles and methods (what exists)
```cpp
// Source: src/qml_gui/Models/ConfigOptionModel.h (read from file)
// Roles: KeyRole, LabelRole, TypeRole, ValueRole, MinRole, MaxRole, StepRole,
//        EnumLabelsRole, CategoryRole, GroupRole, PageRole, ReadonlyRole, DirtyRole,
//        TooltipRole, ModeRole (16 roles)
// Types handled: "double", "int", "bool", "enum" (4 of 7 required)
// MISSING types: "string", "percent" (coPercent/coPercents), nullable (inherit flag)
// MISSING fields: isVector (multi-value per-extruder)
// Methods: setValue, resetOption, indexOfKey, countForCategory, indicesForCategory,
//          pageNames, filterIndicesByPage, optUnit, optTooltip, optMode, optIsDirty,
//          dirtyCount, valuesByKey, defaultValuesByKey, resetToDefaults, applyValues,
//          setReferenceValues, setReadonlyKeys
// #ifdef HAS_LIBSLIC3R: loadFromUpstreamSchema, loadMachineSchema, loadFilamentSchema, loadSchemaFromKeys
```

### Current ConfigViewModel API (what exists vs what's needed)
```cpp
// EXISTS (sufficient for SETTINGS-04/05):
//   Q_PROPERTY: isPresetDirty, globalModifiedCount, currentPresetCombinationValid,
//              currentPresetCompatibilityMessage, hasPendingUnsavedChanges
//   Q_INVOKABLE: requestSavePendingChanges, requestDiscardPendingChanges,
//                requestCancelPendingChanges, resetOptionToLevel(key, level),
//                resetGlobalOption(key), resetAllGlobalOptions,
//                valueSourceForKey(key), saveCurrentPreset, createCustomPreset,
//                deletePreset, canDeletePreset, isPresetDirty,
//                filterOptionIndices(category, searchText, advancedMode),
//                searchOptions(query), mergedConfigValues, applyProjectConfig

// NEEDS ADDITION for SETTINGS-05:
//   resetGroup(tier, groupName) -- reset all options in a specific group to reference values
//   Per-group modified count for the group-nav dirty badges
//   Per-option nullable flag access (optNullable(int i))
//   Per-option isVector flag access (optIsVector(int i))
//   Per-extruder value access for multi-value options

// ALREADY EXISTS but may need extension:
//   filterOptionIndices() currently only operates on printOptions_ model
//   -> needs to accept a tier parameter to filter machine/filament models too
```

### Upstream Tab page and group names (source truth)

**TabPrint (Process) pages and groups:**
```cpp
// Source: third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2311-2788
// Page "Quality":
//   Groups: "Layer height", "Line width", "Seam", "Precision", "Ironing",
//           "Z contouring", "Wall generator", "Walls and surfaces", "Bridging", "Overhangs"
// Page "Strength":
//   Groups: "Walls", "Top/bottom shells", "Infill", "Advanced"
// Page "Speed":
//   Groups: "First layer speed", "Other layers speed", "Overhang speed",
//           "Travel speed", "Acceleration", "Junction Deviation", "Jerk(XY)", "Advanced"
// Page "Support":
//   Groups: "Support", "Raft", "Support filament", "Support ironing",
//           "Advanced", "Tree supports"
// Page "Multimaterial":
//   Groups: "Prime tower", "Filament for Features", "Ooze prevention", "Flush options", "Advanced"
// Page "Others":
//   Groups: "Skirt", "Brim", "Special mode", "Fuzzy Skin", "G-code output",
//           "Change extrusion role G-code", "Post-processing Scripts", "Notes"
```

**TabFilament (Material) pages and groups:**
```cpp
// Source: third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:3892-4216
// Page "Filament":
//   Groups: "Basic information", "Flow ratio and Pressure Advance",
//           "Print chamber temperature", "Print temperature", "Bed temperature",
//           "Temperature per filament" (extruder pages), "Volumetric speed limitation"
// Page "Cooling":
//   Groups: "Cooling for specific layer", "Part cooling fan",
//           "Auxiliary part cooling fan", "Exhaust fan"
// Page "Advanced":
//   Groups: "Filament start G-code", "Change extrusion role G-code", "Filament end G-code"
// Page "Multimaterial":
//   Groups: "Wipe tower parameters", "Multi Filament",
//           "Tool change parameters with single extruder MM printers",
//           "Tool change parameters with multi extruder MM printers"
// Page "Dependencies":
//   Groups: "Compatible printers", "Compatible process profiles"
// Page "Notes":
//   Groups: "Notes"
```

**TabPrinter (Printer) pages and groups:**
```cpp
// Source: third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:4438-4900
// Page "Basic information":
//   Groups: "Printable space", "Advanced", "Cooling Fan",
//           "Extruder Clearance", "Adaptive bed mesh", "Accessory"
// Page "Machine G-code":
//   Groups: "File header G-code", "Machine start G-code", "Machine end G-code",
//           "Printing by object G-code", "Before layer change G-code",
//           "Layer change G-code", "Timelapse G-code", "Clumping Detection G-code",
//           "Change filament G-code", "Change extrusion role G-code",
//           "Pause G-code", "Template Custom G-code"
// Page "Notes":
//   Groups: "Notes"
// Page "Motion ability" (extruder pages, is_extruder_pages=true):
//   Groups: "Advanced", "Resonance Compensation", "Speed limitation",
//           "Acceleration limitation", "Jerk limitation"
// Extruder page:
//   Groups: "Size", "Retraction"
```

### Upstream save_preset flow
```cpp
// Source: third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:6643-6710
// 1. Validate filament temperature pairs (filament only)
// 2. If name empty: show SavePresetDialog modal -> get name, detach flag
// 3. If preset already exists: overwrite (exist_preset=true)
// 4. If new preset + system/filament: set compatible_printers to current printer
// 5. m_presets->save_current_preset(name, detach, save_to_project)
// 6. Update edited preset to the newly saved one
// 7. Reload preset list, update selector

// Qt side mirrors:
//   ConfigViewModel::requestSavePendingChanges() -> checks isReadOnlyPreset -> if builtin, emit saveAsRequired()
//   ConfigViewModel::saveCurrentPreset() -> PresetServiceMock::savePresetValues(name, tierValues)
//   ConfigViewModel::createCustomPreset(category, name) -> PresetServiceMock::createCustomPreset()
```

### BackendContext settingsRequested wiring
```cpp
// Source: src/qml_gui/BackendContext.h:417-419
// Signal: settingsRequested(const QString &category)  // "printer"/"filament"/"process"

// Source: src/qml_gui/BackendContext.cpp:573-578 (current no-op)
void BackendContext::openSettings(const QString &category)
{
    qInfo("[Backend] settingsRequested(%s) -- settings dialog pending Phase 56", qPrintable(category));
    emit settingsRequested(category);
}
// Phase 56 replaces this no-op with: create/show the correct SettingsDialog instance
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Hand-curated option list (30 hardcoded options) | Schema-driven from `print_config_def` (200+ options) | Phase 46 (ConfigOptionModel::loadSchemaFromKeys) | All upstream options automatically available |
| Embedded settings in SettingsPage.qml | Independent non-modal dialogs | Phase 56 (this phase) | Matches upstream Tab.cpp workflow |
| No per-option dirty tracking | Per-option dirty via m_dirtyKeys + referenceValues | Phase 46 | Accurate dirty indicators |
| No value-source tracking | valueSourceForKey() with 3-tier inheritance chain | Phase 46 | Inherited-vs-overridden display |
| QML-only search | C++ fuzzy search (filterOptionIndices + searchOptions) | Phase 46 | Proper fuzzy matching aligned to upstream |

**Deprecated/outdated:**
- `SettingsPage.qml` tier bar approach (category switching via tabs inside embedded page) -- replaced by independent dialogs.
- `ParamsPage.qml` category list as the group navigation -- replaced by proper per-tab group nav derived from upstream Tab.cpp.
- `ConfigOptionModel` hardcoded option list (lines 12-103) -- replaced by `loadSchemaFromKeys()` when HAS_LIBSLIC3R is defined. The hardcoded list is the fallback for non-libslic3r builds.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | UI-SPEC tab names for process ("Quality/Strength/Speed/Support/Multimaterial/Others") match upstream TabPrint pages | Architecture Patterns / Upstream Tab page and group names | Mismatch would cause wrong tab labels |
| A2 | The static page-to-group mapping approach (hardcoded QHash derived from Tab.cpp) is acceptable vs runtime extraction | Architecture Patterns | If upstream restructures groups, Qt side drifts |
| A3 | CxSpinBox can be used for int options without modification (stepSize=1 is default) | Don't Hand-Roll | If CxSpinBox has bugs with int step, need custom fix |
| A4 | Three ApplicationWindow instances can coexist with the main ApplicationWindow in Qt6.10 QML on Windows | Common Pitfalls #6 | If focus/keyboard routing fails, dialog unusable |
| A5 | The existing ConfigViewModel::filterOptionIndices() can be extended to filter machine/filament models by adding a tier parameter | Current ConfigViewModel API | If the method only works on printOptions_, needs refactoring |
| A6 | The UnsavedChangesDialog and SavePresetDialog can be reused from the new dialog context (they take `configVm` as required property) | Reusable Components | If they have hardcoded assumptions about being called from SettingsPage, they won't work |

**Validation note:** A1 is verified against upstream Tab.cpp source (lines 2311-2788) -- the page names match. A4 is standard Qt6 functionality but needs runtime verification on Windows.

## Open Questions

1. **Page-to-group mapping data format**
   - What we know: Upstream Tab.cpp manually places options into groups. No declarative metadata exists on ConfigOptionDef.
   - What's unclear: Should the mapping be (a) a C++ static QHash in ConfigOptionModel, (b) a JSON sidecar file, or (c) extracted at cmake configure time from upstream source?
   - Recommendation: Option (a) -- a static QHash per tier in ConfigOptionModel.cpp, derived from upstream Tab.cpp source. This is maintainable and build-time verifiable.

2. **CxUnitSpinBox -- extend CxSpinBox or new control?**
   - What we know: UI-SPEC identifies this as a gap. Current CxSpinBox has no suffix/unit property.
   - What's unclear: Whether to add a `property string suffix` to CxSpinBox or create a new CxUnitSpinBox wrapping it.
   - Recommendation: Add a `property string suffix` to CxSpinBox (minimal change, avoids new control registration). The suffix renders as a Text element anchored to the right of the spinbox content area.

3. **ConfigViewModel per-tier filterOptionIndices**
   - What we know: Current `filterOptionIndices()` only filters `printOptions_`. Machine and filament need the same capability.
   - What's unclear: Should the method accept a tier parameter, or should each option model have its own filter method?
   - Recommendation: Add a tier parameter to `filterOptionIndices()` that selects the correct ConfigOptionModel internally via `optionModelForTier()`.

## Environment Availability

Step 2.6: Not applicable (no new external dependencies -- all components are project-internal C++/QML).

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Qt Test (QtTest) |
| Config file | CMakeLists.txt (CTest integration, include(CTest)) |
| Quick run command | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` (includes cmake configure + build + smoke test) |
| Full suite command | `ctest --test-dir build --output-on-failure` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SETTINGS-01 | Dialog opens from sidebar button, is non-modal, has correct title | smoke | `ctest --test-dir build -R SmokeTest -N` (manual verify) | Extend ViewModelSmokeTests.cpp |
| SETTINGS-02 | Tabs and group nav populate correctly for printer/material/process | unit | `ctest --test-dir build -R ViewModelSmokeTest` | Extend ViewModelSmokeTests.cpp |
| SETTINGS-03 | ConfigOptionModel returns correct type/unit/enum for all 7 types | unit | `ctest --test-dir build -R ViewModelSmokeTest` | Extend ViewModelSmokeTests.cpp |
| SETTINGS-04 | Per-option dirty detection, value-source tracking, read-only gating | unit | `ctest --test-dir build -R ViewModelSmokeTest` | Extend ViewModelSmokeTests.cpp |
| SETTINGS-05 | Save/SaveAs/reset-option/reset-group/reset-all flow correctly | unit | `ctest --test-dir build -R ViewModelSmokeTest` | Extend ViewModelSmokeTests.cpp |
| SETTINGS-06 | Search returns matching options, basic/advanced toggle filters correctly | unit | `ctest --test-dir build -R ViewModelSmokeTest` | Extend ViewModelSmokeTests.cpp |
| SETTINGS-07 | Option edit triggers slice invalidation, mergedConfigValues includes changes | integration | `ctest --test-dir build -R E2E` | Extend E2EWorkflowTests.cpp |

### Sampling Rate
- **Per task commit:** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` (full build verify)
- **Per wave merge:** `ctest --test-dir build --output-on-failure` (full test suite)
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `tests/ViewModelSmokeTests.cpp` -- add testSettingsDialogOpen(), testPerOptionDirty(), testGroupReset(), testNullableOption(), testSearchFilterByTier()
- [ ] `tests/E2EWorkflowTests.cpp` -- add testSettingsEditInvalidatesSlice() (SETTINGS-07)
- [ ] `src/qml_gui/controls/CxSpinBox.qml` -- add `property string suffix: ""` for unit suffix rendering
- [ ] No framework install needed (Qt Test already available)

## Security Domain

Not applicable for this phase. Settings dialogs are a desktop-only UI feature with no network input, no authentication, no data persistence beyond local preset files (already handled by PresetServiceMock). No ASVS categories apply.

## Sources

### Primary (HIGH confidence)
- `third_party/OrcaSlicer/src/libslic3r/Config.hpp:158-2527` -- ConfigOptionType enum, ConfigOptionMode enum, ConfigOptionDef class (all fields documented)
- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:2311-2788` -- TabPrint::build() page and group names
- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:3892-4216` -- TabFilament::build() page and group names
- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:4438-4900` -- TabPrinter::build_fff() page and group names
- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp:6643-6710` -- Tab::save_preset() flow
- `src/qml_gui/Models/ConfigOptionModel.h` -- current roles, methods, ConfigOption struct (read from file)
- `src/qml_gui/Models/ConfigOptionModel.cpp` -- current implementation, loadSchemaFromKeys, type mapping (read from file)
- `src/core/viewmodels/ConfigViewModel.h` -- current API surface (read from file)
- `src/core/viewmodels/ConfigViewModel.cpp` -- current implementation (read from file)
- `src/qml_gui/BackendContext.h:417-419` -- settingsRequested signal (read from file)
- `src/qml_gui/BackendContext.cpp:573-578` -- current no-op wiring (read from file)
- `.claude/rules/debugging.md` -- SpinBox stepSize int-only trap (project rule)

### Secondary (MEDIUM confidence)
- `56-CONTEXT.md` -- locked decisions for dialog architecture, option model, dirty/reset semantics
- `56-UI-SPEC.md` -- frozen visual contract, typed control matrix, region specs
- `50-INVENTORY.md` -- frozen region IDs and modify/replace decisions
- `src/qml_gui/controls/CxSpinBox.qml` -- current CxSpinBox implementation (read from file)

### Tertiary (LOW confidence)
- None -- all findings verified against upstream source or existing codebase.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all project-internal, no external packages
- Architecture: HIGH -- ConfigOptionDef, Tab.cpp page/group structure verified against upstream source
- Pitfalls: HIGH -- SpinBox int-only trap documented in project rules, ConfigOptionMode 4-level verified from enum, nullable flag verified from ConfigOptionDef

**Research date:** 2026-07-02
**Valid until:** 60 days (stable upstream API, no external dependencies)
