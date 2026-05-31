# Phase 2: Settings and Preset Inheritance — Context

## Current State

### What Already Works
- **ConfigOptionModel::loadFromUpstreamSchema()** — under `#ifdef HAS_LIBSLIC3R`, loads option definitions from `Slic3r::print_config_def` with type, range, defaults, tooltip, sidetext, mode, enums. Already called in ConfigViewModel constructor.
- **ConfigViewModel** — full 3-tier preset merge (printer→filament→print), scope management (global/object/volume/plate), value source tracking, dirty detection, search, layer ranges. All Q_PROPERTY wired.
- **PrintSettings.qml** (1723 lines) — 3-section sidebar with printer/filament/print preset combos, scope tabs, category-grouped option list, value source dots, search bar, layer range editor.
- **SettingsPage.qml** (407 lines) — three-column layout with category tree, parameter list, search.
- **SearchDialog.qml** (423 lines) — full-text search with fuzzy matching, grouped results, keyboard nav.
- **PresetServiceMock** — 3-category storage (Printer/Filament/Print), CRUD, compatibility check. Currently hardcoded with 8 mock presets.

### Gap Analysis
1. **PresetServiceMock uses hardcoded mock data** — 8 presets with ~12-16 keys each. Upstream has 100+ presets with 200+ keys each loaded from JSON vendor profile files.
2. **kDesiredKeys has ~100 keys** — may be missing some upstream keys needed for full schema coverage.
3. **Page grouping** — upstream Tab.cpp organizes options into Pages (Quality, Speed, Infill, Support, etc.) which contain Categories. Current implementation has categories but no page-level grouping.
4. **Preset inheritance chain** — upstream presets use `inherits` field (e.g., `fdm_process_creality_common` → `0.20mm Standard @Creality K1C`). Current mock stores flat values without inheritance resolution.

## Key Upstream APIs

- **`PresetBundle::load_vendor_configs_from_json(dir, vendor_name, flags, compat_rule)`** — loads vendor JSON presets into PresetBundle's printer/filament/print collections
- **`PresetBundle::prints/filaments/printers`** (PresetCollection) — stores preset hierarchy with inheritance
- **`print_config_def`** — global ConfigDef with all option definitions
- **`PresetCollection::first_visible()`** — get default preset
- **`DynamicPrintConfig::full_print_config()`** — complete config with all defaults

## Approach

Use `#ifdef HAS_LIBSLIC3R` to add real preset loading alongside existing mock:
1. Instantiate `PresetBundle` and load Creality vendor presets from `resources/profiles/Creality/`
2. Resolve preset inheritance chains to get effective values
3. Feed resolved values into existing PresetServiceMock storage
4. Extend ConfigOptionModel with page grouping from upstream
5. Verify end-to-end: select preset → options update → values correct
