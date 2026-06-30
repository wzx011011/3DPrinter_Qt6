---
phase: 45
status: clean
depth: standard
files_reviewed: 6
findings:
  critical: 0
  warning: 0
  info: 0
  total: 0
reviewed: 2026-06-30
---

# Phase 45 Code Review

## Scope

- `src/core/services/PresetServiceMock.h`
- `src/core/services/PresetServiceMock.cpp`
- `src/core/viewmodels/ConfigViewModel.h`
- `src/core/viewmodels/ConfigViewModel.cpp`
- `src/qml_gui/panels/PrintSettings.qml`
- `tests/ViewModelSmokeTests.cpp`

## Review Result

No remaining blocking findings after review fixes.

## Finding Fixed During Review

### Invalid current selections disappeared from QML selector models

Severity: Warning

The service-level compatible preset list correctly excluded an incompatible current selection, but `ConfigViewModel::compatibleFilamentPresetNames()` and `compatiblePrintPresetNames()` passed that strict list directly to QML. This could make the current invalid selector display blank, conflicting with the Phase 45 requirement to keep invalid selections visibly blocked instead of hiding them.

Fix: the service API remains strict, while the viewmodel-facing lists prepend the current filament/process selection when it is invalid and absent from the compatible list.

Regression: `configKeepsInvalidSelectionWhenNoCompatibleFallback` now asserts the service strict list excludes the incompatible filament while the viewmodel list still contains the current invalid filament for UI visibility.

## Verification After Review

- `ninja -j16 OWzxSlicer.exe ViewModelSmokeTests`: exit 0.
- `ctest --output-on-failure -R ViewModelSmokeTests`: exit 0, 1/1 passed.
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`: exit 0.
- `git diff --check`: exit 0, CRLF conversion warnings only.
