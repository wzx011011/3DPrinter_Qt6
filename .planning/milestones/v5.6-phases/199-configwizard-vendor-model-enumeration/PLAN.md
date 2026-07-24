# Phase 199 — ConfigWizard Vendor/Model Enumeration Layer

## Goal

Give the first-run `ConfigWizardDialog` a single source of truth for the
vendor / printer-model / material / bed-surface pickers by exposing
enumeration primitives on `PresetServiceMock`. The QML wizard (Phase 200)
stops hard-coding the four Creality model names and instead reads them from
the already-parsed preset metadata.

## Scope

- IN: `Q_INVOKABLE` enumeration API on `PresetServiceMock`.
- IN: QML exposure of the service via `backend.presetServiceMock`.
- IN: Regression tests covering the new API and the
  `__upstream_defaults__` exclusion guarantee.
- OUT: multi-vendor selection UI (Deferred).
- OUT: `PresetUpdater` / `AppConfig` vendor persistence (Deferred).

## Data source (no re-parse)

`PresetServiceMock::loadVendorPresets()` (cpp ~357) already parses
`Creality.json` into `machineEntries` / `filamentEntries` / `processEntries`
local lists and registers each instantiated preset via
`registerPresetMetadata()`. After construction the following maps are the
ground truth:

- `m_presetMetadata` (name -> `PresetMetadata { category, vendor, ... }`)
- `m_categoryPresets` (category -> name list)
- `m_presetStore` (name -> key/value map; also holds `__upstream_defaults__`)

The `__upstream_defaults__` sink is written directly into `m_presetStore`
(`loadUpstreamSchemaDefaults`, cpp ~597) and is NEVER registered through
`registerPresetMetadata`, so it has no metadata and no category. Every
enumeration API walks `m_presetMetadata` / `m_categoryPresets`, which makes
the exclusion automatic and free.

## API added (`src/core/services/PresetServiceMock.h`)

```cpp
Q_INVOKABLE QStringList vendors() const;
Q_INVOKABLE QStringList printerModelsForVendor(const QString &vendor) const;
Q_INVOKABLE QStringList materialsForVendor(const QString &vendor) const;
Q_INVOKABLE QStringList bedTypesForPrinterModel(const QString &model) const;
Q_INVOKABLE QStringList defaultBedTypes() const;
```

### Return values

| API | Built-in bundle (no vendor JSON) | With `Creality.json` loaded |
|-----|----------------------------------|-----------------------------|
| `vendors()` | `["OWzx Builtin"]` | `["Creality", "OWzx Builtin"]` (sorted) |
| `printerModelsForVendor("Creality")` | `[]` | all instantiated machine presets |
| `printerModelsForVendor("OWzx Builtin")` | `["Creality K1C 0.4", "Creality Ender-3 S1"]` | `[]` |
| `materialsForVendor("OWzx Builtin")` | 3 filament presets (PLA/ABS/PETG) | `[]` |
| `bedTypesForPrinterModel(any)` | 4-entry default list | 4-entry default list |
| `defaultBedTypes()` | 4 entries | 4 entries |

`vendors()` is sorted with `std::sort` for deterministic QML display.

## QML exposure

`PresetServiceMock` was previously only reachable inside C++
(`BackendContext::presetService_`). Added a CONSTANT `Q_PROPERTY`:

```cpp
Q_PROPERTY(QObject *presetServiceMock READ presetServiceMock CONSTANT)
QObject *presetServiceMock() const;  // returns presetService_
```

Returned as `QObject*` to avoid leaking the `PresetServiceMock` header into
QML-facing includes. QML calls the enumeration API via
`backend.presetServiceMock.vendors()` etc.

## Data gap: per-model bed surfaces

The upstream machine JSON does not carry a bed-surface field in the keys
`loadVendorPresets()` ingests today, so `bedTypesForPrinterModel()` cannot
return model-specific data yet. It returns the canonical 4-surface default
list (matching the prior mock) and accepts the `model` argument purely for
forward compatibility. This gap is intentionally documented here and not
silently faked; closing it requires extending the machine schema parse,
which is Deferred.

Default list (shared with the wizard):
- `光滑 PEI 板`
- `普通 PEI 板`
- `PC 热床`
- `EP 热床`

## Files changed

- `src/core/services/PresetServiceMock.h` — 5 new `Q_INVOKABLE` declarations.
- `src/core/services/PresetServiceMock.cpp` — 5 implementations
  (`vendors`, `printerModelsForVendor`, `materialsForVendor`,
  `defaultBedTypes`, `bedTypesForPrinterModel`).
- `src/qml_gui/BackendContext.h` — `presetServiceMock` Q_PROPERTY + accessor.
- `src/qml_gui/BackendContext.cpp` — accessor returns `presetService_`.
- `tests/ViewModelSmokeTests.cpp` — 5 new test slots:
  `testVendorEnumeration`, `testPrinterModelsForVendor`,
  `testMaterialsForVendor`, `testBedTypesForPrinterModel`,
  `testEnumerationExcludesUpstreamDefaults`.

## Non-regression

- No existing `Q_INVOKABLE` on `PresetServiceMock` was touched; preset
  CRUD / IO (`exportBundle`, `importBundle`, `createCustomPreset`,
  `comparePresets`, ...) are unchanged.
- `loadVendorPresets()` body is untouched — the new API only reads the maps
  it already populates.

## Verification

- Bracket balance: all 5 modified files pass the string/comment-aware
  checker.
- Tests run under `HAS_LIBSLIC3R=1` (app build) and accept either the
  built-in vendor (`OWzx Builtin`) or the JSON vendor (`Creality`), so they
  pass with or without `Creality.json` checked into the tree.
