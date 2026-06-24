---
phase: 12
phase_name: Calibration Closure for Implemented Modes
status: passed
verified: 2026-06-25
requirements:
  CAL-01: passed
  CAL-02: passed
  CAL-03: passed
  CAL-04: passed
  CAL-05: passed
---

# Phase 12 Verification

## Result

Status: passed

## Checks

| Check | Result | Evidence |
|---|---|---|
| Visible calibration routing | PASS | `BBLTopbar.qml` routes Flow Dynamics, Flow Rate, and Temp Tower through `selectItemById()` using stable ids. |
| Deterministic calibration request coverage | PASS | `ViewModelSmokeTests::calibrationImplementedModesEmitSliceRequests()` asserted mode/range/step/project-name for PA, Flow Rate, and Temp Tower. |
| Mock fallback coverage | PASS | `ViewModelSmokeTests::calibrationFallbackAndSliceCallbacksDriveProgress()` completed fallback timer flow without `SliceService`. |
| Real callback progress coverage | PASS | The same test drove `onSliceProgressUpdated`, `onSliceFinished`, and `onSliceFailed` callbacks through the service state machine. |
| Unsupported modes explicit | PASS | `ViewModelSmokeTests::calibrationUnsupportedModesAreExplicitlyUnavailable()` asserted Bed Leveling, Vibration, and Max Volumetric Speed are not startable and carry reasons. |
| Full ViewModel smoke tests | PASS | `build\ViewModelSmokeTests.exe -o build\ViewModelSmokeTests.phase12-full.txt,txt` reported `31 passed, 0 failed`. |
| QML UI audit | PASS | `build\QmlUiAuditTests.exe -o build\QmlUiAuditTests.phase12.txt,txt` exited 0. |
| Canonical verification | PASS | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` exited 0. |

## Commands Run

```powershell
cmd.exe /d /s /c 'call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" && cmake --build build --target ViewModelSmokeTests --config Release && build\ViewModelSmokeTests.exe calibrationImplementedModesExposeStableRouting calibrationImplementedModesEmitSliceRequests calibrationUnsupportedModesAreExplicitlyUnavailable calibrationFallbackAndSliceCallbacksDriveProgress -o build\ViewModelSmokeTests.phase12-targeted.txt,txt'
build\ViewModelSmokeTests.exe -o build\ViewModelSmokeTests.phase12-full.txt,txt
cmd.exe /d /s /c 'call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" && cmake --build build --target ViewModelSmokeTests QmlUiAuditTests --config Release && build\ViewModelSmokeTests.exe calibrationImplementedModesExposeStableRouting calibrationImplementedModesEmitSliceRequests calibrationUnsupportedModesAreExplicitlyUnavailable calibrationFallbackAndSliceCallbacksDriveProgress -o build\ViewModelSmokeTests.phase12-targeted.txt,txt && build\QmlUiAuditTests.exe -o build\QmlUiAuditTests.phase12.txt,txt'
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

## Full Build Notes

The canonical script completed successfully on 2026-06-25. It rebuilt the Qt6 executable, built `ViewModelSmokeTests.exe`, `QmlUiAuditTests.exe`, CLI targets, and E2E tests, then reported:

- `[UI] QML UI audit tests passed`
- `[E2E] All pipeline tests passed`

Known existing warnings remain from third-party/source build inputs, including CGAL data-dir warnings, Qt minimum-CMake warnings, MSVC codepage warnings in dependency/upstream headers, and existing discarded `QFuture` warnings.
