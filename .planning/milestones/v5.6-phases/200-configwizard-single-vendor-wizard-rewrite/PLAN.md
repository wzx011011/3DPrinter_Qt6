# Phase 200 — ConfigWizard Single-Vendor Wizard Rewrite

## Goal

Rewrite `src/qml_gui/dialogs/ConfigWizardDialog.qml` so the printer /
filament / bed-surface pickers are driven by the Phase 199 enumeration API
instead of hard-coded lists. Keep the wizard single-vendor for now and
close the first-run loop on completion.

## Scope

- IN: dynamic printer / filament / bed lists via
  `backend.presetServiceMock`.
- IN: temperature card backed by live preset values (no switch-case).
- IN: documented completion loop (`configWizardCompleted = true`).
- OUT: multi-vendor selection / check-all flow (Deferred).
- OUT: `PresetUpdater` / `AppConfig` persistence of wizard selections
  beyond the existing `wizard/completed` flag (Deferred).

## Structure preserved

The original dialog already had a 4-page SwipeView
(Welcome -> Printer -> Filament -> Done) and already closed the loop by
setting `backend.configWizardCompleted = true` on Finish. This phase keeps
that structure and that loop; it only replaces the data sources and
documents the loop explicitly. (The brief hypothesised a 3-page wizard and
a missing setter, but the on-disk file already had both — verified against
`BackendContext.cpp:481 setConfigWizardCompleted` and `main.qml:601`.)

## Data binding (single vendor)

QML resolves the service through a null-safe guard matching the
`PreparePage.qml` convention:

```qml
readonly property var presetSvc: typeof backend !== "undefined" && backend
    ? backend.presetServiceMock : null
readonly property string activeVendor: vendorList.length > 0 ? vendorList[0] : ""
readonly property var printerModelList: presetSvc && activeVendor.length > 0
    ? presetSvc.printerModelsForVendor(activeVendor) : []
readonly property var materialList: ... materialsForVendor(activeVendor) ...
readonly property var bedTypeList: presetSvc ? presetSvc.defaultBedTypes() : []
```

`activeVendor` is `vendors()[0]`. With the built-in bundle only,
`vendors()` returns `["OWzx Builtin"]`; once `Creality.json` is loaded it
returns `["Creality", "OWzx Builtin"]` (sorted), so the wizard
auto-promotes to the real vendor when the data is present — no QML change
required.

## Replacement of hard-coded mock

| Picker | Before (hard-coded) | After (Phase 199 API) |
|--------|---------------------|-----------------------|
| Printer combo | 4 Creality names | `printerModelsForVendor(activeVendor)` |
| Bed combo | 4 surfaces | `defaultBedTypes()` |
| Filament combo | `["PLA","ABS","PETG","TPU","ASA"]` | `materialsForVendor(activeVendor)` |
| Nozzle temp | switch on combo index | `presetValue(name, "nozzle_temp")` |
| Bed temp | switch on combo index | `presetValue(name, "bed_temp")` |
| Hint text | switch on combo index | name-substring heuristic |

The material description reverted from a switch-case to a name-substring
heuristic (`indexOf("PLA")`, etc.) so it still works when the preset name
is the vendor's full label (e.g. `Creality Generic PLA`) rather than the
bare token.

## Empty-state handling

Every picker is wrapped in `visible: <list>.length > 0` and paired with a
warning `Text` so the wizard degrades gracefully when no presets are
loaded (e.g. a build with no vendor JSON and no built-in defaults). The
Done page summary rows are likewise hidden when their data is empty.

## Completion loop (closed and documented)

```qml
backend.configWizardCompleted = true;  // WRITE -> setConfigWizardCompleted(true)
root.wizardFinished();
root.close();
```

This invokes `BackendContext::setConfigWizardCompleted` which writes
`wizard/completed` to QSettings (BackendContext.cpp:489) and emits
`configWizardCompletedChanged`. `main.qml:601` gates auto-open on
`!backend.configWizardCompleted`, so the wizard will not re-open on next
launch. This was already wired in the original file; this phase keeps it
intact and adds an explanatory comment.

## Header change (minimal)

`PresetServiceMock::presetValue` was promoted to `Q_INVOKABLE` so QML can
read the per-material nozzle/bed temperatures. It is a read-only
accessor; no behavior change.

## Files changed

- `src/qml_gui/dialogs/ConfigWizardDialog.qml` — full rewrite (411 ->
  ~490 lines): dynamic pickers, empty states, preset-backed temperature
  card, documented completion loop.
- `src/core/services/PresetServiceMock.h` — `presetValue` now
  `Q_INVOKABLE` (QML temperature reads).

## Non-regression

- Visual style unchanged: same `CxDialog`, `Theme.*` tokens, page dots,
  navigation buttons, success screen.
- `selectedPrinter` / `selectedBedType` / `selectedFilament` /
  `selectedNozzle` public properties and the `wizardFinished()` signal are
  preserved for external callers.
- No change to `BackendContext` semantics beyond the Phase 199 Q_PROPERTY.

## Verification

- Bracket balance: QML passes the string/comment-aware checker.
- Manually verified all `Theme.*` tokens used exist in `Theme.qml`
  (including `accentDark`, already used by the prior file).
- `CxComboBox` accepts a list `model:` (same usage as the original file).
- Empty-model paths degrade to informative warnings instead of crashing.

## Deferred follow-ups

- Multi-vendor picker (check-all vendors, per-vendor model lists).
- `PresetUpdater` integration + `AppConfig` vendor enable list.
- Per-model bed-surface metadata (requires extending the machine JSON
  parse in `loadVendorPresets`).
- Persisting the user's wizard selections as the active presets
  (`setSelectedPresetForCategory`) rather than only recording completion.
