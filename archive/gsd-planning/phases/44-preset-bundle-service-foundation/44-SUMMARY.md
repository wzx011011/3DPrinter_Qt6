---
phase: 44
status: complete
completed: 2026-06-30
requirements_addressed: [PSET-01, PSET-02, PSET-03, PSET-04, PSET-05, PSET-06]
---

# Phase 44 Summary: Preset Bundle Service Foundation

## Delivered

- Added service-owned preset metadata in `PresetServiceMock` for category, built-in/system, read-only, user preset, vendor, setting id, inherited parent, and value count.
- Made fallback and vendor-loaded presets system/read-only, while user-created and imported presets are writable user presets.
- Added category-correct selected-preset persistence through `QSettings` for print, filament, and printer presets.
- Replaced placeholder all-preset JSON export with a validated local bundle format that exports user presets with category and metadata.
- Replaced mutating import behavior with full pre-validation; malformed bundles, duplicate names, invalid categories, missing values, and empty names fail without changing existing preset state.
- Added bundle kind/version validation so unsupported local bundle formats are rejected before import.
- Updated `ConfigViewModel` and `CalibrationViewModel` to use `PresetServiceMock` category constants instead of raw or reversed category integers.
- Tightened legacy `ConfigViewModel::setCurrentPreset()` to act only as a print/process preset selector and reject printer/filament names.
- Added category gates for rename/delete so a preset cannot be mutated through the wrong category path.
- Added regression coverage for category mapping, read-only/user metadata, persisted selections, malformed import no-op, user bundle round-trip, tier-aware saves, and wrong-category mutation rejection.

## Intentional Limits

- Phase 44 keeps the local JSON bundle deterministic and service-level. Full upstream dialog parity and broader import/export UI behavior remains Phase 48.
- Disk persistence of edited user preset files beyond in-memory service state remains Phase 47.
- Full compatibility-expression evaluation remains Phase 45.
- The canonical verification script did not complete within 15 minutes because `build/E2EWorkflowTests.exe` was still running; phase-specific preset and ViewModel verification passed separately.
