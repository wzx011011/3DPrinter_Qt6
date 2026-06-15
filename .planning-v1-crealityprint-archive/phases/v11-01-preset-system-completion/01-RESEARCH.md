# Phase 1: Preset System Completion - Research

**Researched:** 2026-06-01
**Domain:** Preset loading, inheritance, compatibility, and configuration schema in CrealityPrint Qt6 migration
**Confidence:** HIGH

## Summary

The Qt6 preset system has a solid architectural foundation -- the 3-tier hierarchy (printer/filament/print), the `PresetServiceMock` with category-aware storage, the `ConfigViewModel` with scope management, and the `ConfigOptionModel` with upstream schema loading are all structurally sound. However, there are **critical gaps** that prevent the system from functioning correctly with real vendor presets.

The most critical issue is that `PresetServiceMock::loadVendorPresets()` searches for `Creality.json` inside `Creality/` subdirectory, but the actual vendor index file `Creality.json` lives one level up at `resources/profiles/Creality.json`. This means `loadVendorPresets()` always returns false, and the system falls back to 8 hardcoded presets instead of loading the 138 machine, 1202 filament, and 258 process presets from the upstream vendor bundle.

Additional gaps include: upstream defaults loaded but never consumed in the hierarchy merge, filament compatibility checks that are overly simplistic compared to upstream `compatible_printers` array matching, missing `machine_model_list` parsing, and no `default_print_profile` / `default_materials` resolution from machine presets.

**Primary recommendation:** Fix the vendor index path resolution, then complete the remaining gaps in inheritance resolution and compatibility checking. The existing architecture is sound -- this is primarily a path fix and data plumbing completion task, not an architectural redesign.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Vendor preset loading | API / Backend (C++) | -- | PresetServiceMock reads JSON files from disk, no UI involvement |
| Config schema definition | API / Backend (libslic3r) | -- | print_config_def provides canonical option definitions |
| 3-tier hierarchy merge | API / Backend (C++) | -- | ConfigViewModel.mergePresetHierarchy() is pure business logic |
| Compatibility checking | API / Backend (C++) | -- | PresetServiceMock.isFilamentCompatibleWithPrinter() |
| Preset selection UI | Browser / Client (QML) | -- | PrintSettings.qml renders preset dropdowns |
| Config option rendering | Browser / Client (QML) | -- | PrintSettings.qml renders individual option controls |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt6::Qml | 6.10 | Q_PROPERTY exposure, Q_INVOKABLE API | Project framework |
| libslic3r (PrintConfig) | v7.0.1 | print_config_def schema, ~200 config keys | Upstream source truth |
| Qt6::Concurrent | 6.10 | Background preset loading (future) | Project framework |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| nlohmann/json (via QJsonDocument) | Qt built-in | JSON parsing for vendor presets | All preset file loading |
| QSettings | 6.10 | User preset persistence | Custom preset save/load |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Custom JSON parsing in PresetServiceMock | Direct PresetBundle C++ API | PresetBundle requires wxWidgets; custom parsing is correct approach for Qt6 |
| QHash<QString, QVariant> preset storage | Slic3r::DynamicPrintConfig | DynamicPrintConfig would couple Qt6 to libslic3r internals; QHash is cleaner boundary |

## Architecture Patterns

### System Architecture Diagram

```
[Creality.json vendor index]
         |
         v
[PresetServiceMock::loadVendorPresets()]
    |              |              |
    v              v              v
[machine/*.json] [filament/*.json] [process/*.json]
    |              |              |
    +--[resolveInheritance()]-----+
         |              |
         v              v
[m_presetStore: QHash<presetName, QHash<key, value>>]
         |
         v
[ConfigViewModel::mergePresetHierarchy()]
    |           |           |
    v           v           v
[Printer]  [Filament]  [Print]   --> [globalOptionValues_]
    tier1      tier2      tier3          |
                                          v
                                   [applyScopeValues()]
                                          |
                                    [printOptions_ model]
                                          |
                                    [QML PrintSettings]
```

### Recommended Project Structure
```
src/core/services/PresetServiceMock.h/.cpp   -- already exists, needs fixes
src/core/viewmodels/ConfigViewModel.h/.cpp    -- already exists, needs fixes
src/qml_gui/Models/ConfigOptionModel.h/.cpp   -- already exists, working
src/qml_gui/Models/PresetListModel.h/.cpp     -- already exists, working
src/qml_gui/panels/PrintSettings.qml           -- already exists, minimal changes
```

### Pattern 1: Vendor Index Parsing
**What:** Read `Creality.json` to get machine_list/filament_list/process_list arrays with name+sub_path pairs
**When to use:** Application startup preset loading
**Example:**
```json
// resources/profiles/Creality.json (upstream format)
{
  "name": "Creality",
  "version": "25.12.26.17",
  "machine_list": [
    {"name": "fdm_creality_common", "sub_path": "machine/fdm_creality_common.json"},
    {"name": "Creality K1C 0.4 nozzle", "sub_path": "machine/Creality K1C 0.4 nozzle.json"}
  ],
  "filament_list": [...],
  "process_list": [...]
}
```

### Pattern 2: Preset Inheritance Resolution
**What:** Recursively resolve `inherits` field to build merged config (child overrides parent)
**When to use:** Loading any preset that has `inherits` field
**Example:**
```json
// machine/Creality K1C 0.4 nozzle.json
{
  "type": "machine",
  "inherits": "fdm_creality_common",
  "instantiation": "true",
  "nozzle_diameter": "0.4",
  ...
}
// machine/fdm_creality_common.json
{
  "type": "machine",
  "inherits": "fdm_machine_common",
  "instantiation": "false",  // template, not a selectable preset
  "gcode_flavor": "marlin",
  ...
}
```

### Anti-Patterns to Avoid
- **Skipping `instantiation: false` filter:** Template presets must NOT appear in user-selectable lists. The current code handles this correctly.
- **Hardcoding search paths:** The search path list must match how the build output finds resources. Currently searches `QDir::currentPath()/third_party/CrealityPrint/resources/profiles/Creality` which looks for `Creality.json` inside the wrong directory.
- **Ignoring `compatible_printers` arrays:** Filament presets carry `compatible_printers: ["Creality K1C 0.4 nozzle"]` arrays. The current compatibility check only uses nozzle diameter ranges, which is insufficient.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Config option schema | Custom option definitions | `Slic3r::print_config_def` via `ConfigOptionModel::loadFromUpstreamSchema()` | Already implemented, provides ~200 options with labels, ranges, tooltips, modes |
| Value type coercion | Custom string-to-number parsing | Qt's `QJsonValue` type dispatch + `toDouble()` | Preset JSON files store all values as strings, need careful type conversion |

**Key insight:** The existing `resolveInheritance()` function handles string-to-number coercion correctly. The main gaps are in path resolution and the upstream defaults consumption.

## Common Pitfalls

### Pitfall 1: Wrong vendor index search path
**What goes wrong:** `loadVendorPresets()` searches for `Creality.json` inside `resources/profiles/Creality/` but the file is at `resources/profiles/Creality.json`
**Why it happens:** The code assumes vendor index is inside the vendor subdirectory, matching the upstream `load_vendor_configs_from_json()` pattern where vendor_name subdirectory contains the index. But Creality's resource layout puts the index at the parent level.
**How to avoid:** Fix search paths to include `resources/profiles/` (parent of Creality directory) where `Creality.json` actually lives. The vendor directory for sub_path resolution is `resources/profiles/Creality/`.
**Warning signs:** `loadVendorPresets()` returns false, only 8 hardcoded presets available.

### Pitfall 2: Upstream defaults not consumed
**What goes wrong:** `PresetServiceMock` loads `print_config_def` defaults into `__upstream_defaults__` but `ConfigViewModel::mergePresetHierarchy()` never reads this store. This means options not present in any preset have no value.
**Why it happens:** The merge function starts from `printOptions_->resetToDefaults()` which uses hardcoded defaults or `loadFromUpstreamSchema()` defaults. But the `__upstream_defaults__` store was intended to provide the full set of ~200 upstream keys as the base tier.
**How to avoid:** In `mergePresetHierarchy()`, start with `__upstream_defaults__` as the base instead of the model's reduced defaults, OR ensure `loadFromUpstreamSchema()` already provides all needed keys.
**Warning signs:** Missing option values when switching presets; options showing 0 or empty instead of upstream defaults.

### Pitfall 3: Simplified compatibility check
**What goes wrong:** `isFilamentCompatibleWithPrinter()` only checks `compatible_nozzle_min/max` and `nozzle_temp_range_max/max_nozzle_temp`. But upstream filament presets carry explicit `compatible_printers` arrays.
**Why it happens:** The compatibility check was written before real vendor presets were available for testing.
**How to avoid:** Check `compatible_printers` array first (if present and non-empty, require printer name to be in the list). Fall back to nozzle/temp range checks only when array is absent or empty.
**Warning signs:** Wrong filament presets shown as "compatible" for a printer; user can select PLA for a printer that only supports ABS.

### Pitfall 4: `compatible_printers` and `compatible_prints` skipped in resolveInheritance
**What goes wrong:** The `resolveInheritance()` function explicitly skips these keys: `if (key == "compatible_printers" || key == "compatible_prints") continue;`
**Why it happens:** They were treated as metadata, but they are essential for compatibility filtering.
**How to avoid:** Store `compatible_printers` and `compatible_prints` in the preset values so the compatibility check can use them.
**Warning signs:** No filament presets filtered out when switching printers.

### Pitfall 5: Machine model list not parsed
**What goes wrong:** `Creality.json` has both `machine_list` (machine configs with `instantiation` flag) and `machine_model_list` (machine models with bed dimensions, images, nozzle diameters). The Qt6 code only parses `machine_list` and ignores `machine_model_list`.
**Why it happens:** Machine model info (bed dimensions, build plate textures) is needed for 3D viewport rendering, not for preset settings. It was deferred.
**How to avoid:** Parse `machine_model_list` in a future phase. For preset system completion, focus on `machine_list`/`filament_list`/`process_list` only.
**Warning signs:** Printer names appear correctly but bed dimensions are wrong.

## Code Examples

### Fixed search path for vendor index
```cpp
// PresetServiceMock::loadVendorPresets() -- CORRECTED search paths
const QStringList searchPaths = {
    // Vendor index is at profiles/Creality.json, NOT profiles/Creality/Creality.json
    QDir::currentPath() + QStringLiteral("/third_party/CrealityPrint/resources/profiles"),
    QCoreApplication::applicationDirPath() + QStringLiteral("/resources/profiles"),
};

for (const auto &path : searchPaths) {
    if (QFileInfo::exists(path + QStringLiteral("/Creality.json"))) {
        vendorDir = path + QStringLiteral("/Creality");  // sub-path root
        indexFile = path + QStringLiteral("/Creality.json");
        break;
    }
}
```

### Upstream-compatible filament compatibility check
```cpp
bool PresetServiceMock::isFilamentCompatibleWithPrinter(
    const QString &filamentName, const QString &printerName) const
{
    const auto filVals = m_presetStore.value(filamentName);
    const auto prnVals = m_presetStore.value(printerName);

    // 1. Check explicit compatible_printers array (upstream method)
    const auto cp = filVals.value(QStringLiteral("compatible_printers"));
    if (cp.isValid() && cp.type() == QVariant::List) {
        const auto list = cp.toStringList();
        if (!list.isEmpty()) {
            return list.contains(printerName);
        }
    }

    // 2. Fallback: nozzle diameter + temp range checks
    // ... (existing logic)
}
```

### Consuming upstream defaults in mergePresetHierarchy
```cpp
void ConfigViewModel::mergePresetHierarchy()
{
    // Start with upstream schema defaults (full ~200 keys)
    QHash<QString, QVariant> merged;
    if (presetService_->hasPreset(QStringLiteral("__upstream_defaults__"))) {
        merged = presetService_->presetValues(QStringLiteral("__upstream_defaults__"));
    } else {
        printOptions_->resetToDefaults();
        merged = printOptions_->valuesByKey();
    }

    // Initialize value sources
    valueSources_.clear();
    for (auto it = merged.begin(); it != merged.end(); ++it)
        valueSources_[it.key()] = QStringLiteral("default");

    // Layer printer -> filament -> print presets on top
    // ... (existing tier logic)
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| 8 hardcoded presets | Real vendor preset loading via JSON parsing | Phase 2 (Settings/Preset) | Loads 138+ printers, 1200+ filaments, 258+ processes |
| Mock-only preset names | Category-aware PresetServiceMock | Phase 2 | 3-tier hierarchy implemented |
| QML inline config options | ConfigOptionModel with upstream schema | Phase 2 | ~200 config keys from print_config_def |

**Deprecated/outdated:**
- `initBuiltinDefaults()`: Hardcoded fallback that creates 2 printers, 3 filaments, 3 processes. Should only be used when `HAS_LIBSLIC3R` is not defined.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `Creality.json` at `resources/profiles/Creality.json` is the authoritative vendor index | Architecture | HIGH -- if wrong, preset loading path must be redesigned |
| A2 | Upstream `compatible_printers` array in filament presets is the primary compatibility mechanism | Compatibility | MEDIUM -- if wrong, compatibility logic needs different approach |
| A3 | `__upstream_defaults__` store should be consumed as the base tier in hierarchy merge | Merge Logic | MEDIUM -- if wrong, ConfigOptionModel defaults may be sufficient |
| A4 | The `loadFromUpstreamSchema()` already provides all needed keys for the preset values to overlay | Schema | LOW -- verified by reading code, it iterates kDesiredKeys from print_config_def |

## Open Questions

1. **Should `machine_model_list` be parsed in this phase?**
   - What we know: Machine model entries contain bed_model, bed_texture, nozzle_diameter, family, model_id, default_materials
   - What's unclear: Whether the bed dimensions are needed for the viewport before machine model parsing is complete
   - Recommendation: Defer machine_model_list to a separate phase; focus on machine_list/filament_list/process_list only

2. **Should user preset save/load to disk be part of this phase?**
   - What we know: `createCustomPreset()` exists but only stores in memory
   - What's unclear: Whether upstream expects user presets to persist across sessions via filesystem
   - Recommendation: Defer filesystem persistence to a later phase; focus on vendor preset loading correctness

3. **How should `default_print_profile` from machine presets be used?**
   - What we know: Each machine preset has `"default_print_profile": "0.20mm Standard @Creality K1C 0.4 nozzle"` which links to a specific process preset
   - What's unclear: Whether changing printer should auto-select the linked process preset
   - Recommendation: Implement as a nice-to-have; when switching printer, look up `default_print_profile` and auto-select matching process preset

## Environment Availability

Step 2.6: SKIPPED (no external dependencies beyond libslic3r which is already linked)

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Qt Test (QtTest) |
| Config file | None -- tests in `tests/ViewModelSmokeTests.cpp` |
| Quick run command | `cmake --build build --target ViewModelSmokeTests && build/ViewModelSmokeTests.exe` |
| Full suite command | Same as quick (single test binary) |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PS-01 | loadVendorPresets finds Creality.json and parses vendor index | unit | `ViewModelSmokeTests -ps01` | Wave 0 |
| PS-02 | Correct number of printers/filaments/processes loaded | unit | `ViewModelSmokeTests -ps02` | Wave 0 |
| PS-03 | Inheritance chain resolves fdm_creality_common -> fdm_machine_common | unit | `ViewModelSmokeTests -ps03` | Wave 0 |
| PS-04 | mergePresetHierarchy produces correct merged values | unit | `ViewModelSmokeTests -ps04` | Wave 0 |
| PS-05 | Filament compatibility checks use compatible_printers array | unit | `ViewModelSmokeTests -ps05` | Wave 0 |

### Sampling Rate
- **Per task commit:** `cmake --build build --config Release`
- **Per wave merge:** Full build + smoke test
- **Phase gate:** 274/274 compilation, 0 QML warnings, correct preset count in UI

### Wave 0 Gaps
- [ ] `tests/PresetLoadingTests.cpp` -- covers PS-01 through PS-05
- [ ] Test fixture for vendor preset directory path resolution

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V5 Input Validation | yes | QJsonValue type checking, string-to-number conversion with validation |
| V4 Access Control | no | No user auth in this domain |
| V6 Cryptography | no | No encryption in this domain |

### Known Threat Patterns for Preset Loading

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malformed JSON in vendor preset files | Tampering | QJsonParseError checking, graceful fallback to initBuiltinDefaults() |
| Path traversal in sub_path values | Tampering | Validate sub_path doesn't contain `..` or absolute paths |
| Integer overflow in string-to-number conversion | Tampering | Qt's toDouble() with ok flag check |

## Sources

### Primary (HIGH confidence)
- `src/core/services/PresetServiceMock.cpp` -- read and analyzed line by line
- `src/core/viewmodels/ConfigViewModel.cpp` -- read and analyzed line by line
- `src/qml_gui/Models/ConfigOptionModel.cpp` -- read and analyzed
- `third_party/CrealityPrint/resources/profiles/Creality.json` -- vendor index file verified at 6393 lines
- `third_party/CrealityPrint/src/libslic3r/PresetBundle.cpp` -- upstream load_system_presets_from_json and load_vendor_configs_from_json analyzed

### Secondary (MEDIUM confidence)
- `third_party/CrealityPrint/src/libslic3r/Preset.hpp` -- BBL_JSON_KEY_* defines verified
- `third_party/CrealityPrint/resources/profiles/Creality/machine/` -- 138 JSON files verified
- `third_party/CrealityPrint/resources/profiles/Creality/filament/` -- 1202 JSON files verified
- `third_party/CrealityPrint/resources/profiles/Creality/process/` -- 258 JSON files verified

### Tertiary (LOW confidence)
- None

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - all libraries verified in codebase
- Architecture: HIGH - existing architecture is sound, gaps are in data plumbing
- Pitfalls: HIGH - path issue confirmed by direct file system inspection

**Research date:** 2026-06-01
**Valid until:** 2026-07-01 (stable -- project tech stack is locked)
