---
phase: 13
phase_name: Hybrid Integration Verification
status: passed
verified: 2026-06-25
requirements:
  INT-01: passed
  INT-02: passed
  INT-03: passed
  INT-04: passed
  INT-05: passed
  INT-06: passed
manual_verification_required:
  mqtt_live_publish: true
  ftp_live_upload: true
  rtsp_live_decode: true
---

# Phase 13 Verification

## Result

Status: passed

## Checks

| Check | Result | Evidence |
|---|---|---|
| INT-01 SSDP parser fixtures | PASS | `ViewModelSmokeTests::int01_SsdpDiscoveryParsesMockResponse()` asserts Bambu and Creality datagrams, sender fallback, Bambu port `8883`, non-Bambu port `1883`, serial normalization, and deterministic timeout signal behavior. |
| INT-02 MQTT telemetry/control payloads | PASS | `ViewModelSmokeTests::int04_MqttConnectionParamsAndTelemetryFields()` covers telemetry fields and filtered-index routing; `int05_MqttCommandConstructionAndControlFlow()` covers pause/resume/stop/gcode envelopes and topic construction. |
| INT-03 FTP send-print routing | PASS | `ViewModelSmokeTests::int06_FtpUrlAndSendPrintRouting()` covers access-code URL encoding, remote path construction, refusal without MQTT, and missing local file error signal. |
| INT-04 Camera no-stream/error behavior | PASS | `ViewModelSmokeTests::int03_CameraStateMachineAndFrameToken()` covers no-camera error state, default RTSP URL derivation, state transitions, and frame token updates without requiring a live stream. |
| INT-05 Software viewport fallback | PASS | `QmlUiAuditTests::mainRegistersSoftwareViewportByDefault()` asserts `main_qml.cpp` defaults to `SoftwareViewport`, keeps OpenGL behind `OWZX_OPENGL`, and the canonical script does not set `OWZX_OPENGL`. |
| INT-06 App settings / bed shape persistence | PASS | `ViewModelSmokeTests::appSettingsAndEditorBedShapePersistDeterministically()` uses scoped `QSettings` restoration and verifies AppSettings clamping/persistence plus EditorViewModel bed-shape reload. |
| Full ViewModel smoke tests | PASS | `build\ViewModelSmokeTests.exe -o build\ViewModelSmokeTests.phase13-full.txt,txt` reported `32 passed, 0 failed`. |
| QML UI audit | PASS | `build\QmlUiAuditTests.exe -o build\QmlUiAuditTests.phase13-targeted.txt,txt` reported `6 passed, 0 failed`. |
| Source diff hygiene | PASS | `git diff --check` passed for touched source and test files; only Git LF/CRLF warnings were printed. |
| Canonical verification | PASS | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` exited 0 and reported QML UI audit and E2E pipeline success. |

## Commands Run

```powershell
cmd.exe /d /s /c "call ""C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"" && cmake --build build --target ViewModelSmokeTests QmlUiAuditTests --config Release"
build\ViewModelSmokeTests.exe int01_SsdpDiscoveryParsesMockResponse int04_MqttConnectionParamsAndTelemetryFields int05_MqttCommandConstructionAndControlFlow int06_FtpUrlAndSendPrintRouting int03_CameraStateMachineAndFrameToken appSettingsAndEditorBedShapePersistDeterministically -o build\ViewModelSmokeTests.phase13-targeted.txt,txt
build\QmlUiAuditTests.exe -o build\QmlUiAuditTests.phase13-targeted.txt,txt
build\ViewModelSmokeTests.exe -o build\ViewModelSmokeTests.phase13-full.txt,txt
git diff --check -- src/core/services/SsdpDiscovery.cpp src/core/services/SsdpDiscovery.h src/core/services/DeviceServiceMock.cpp src/core/services/DeviceServiceMock.h src/core/services/CameraServiceMock.cpp src/core/services/CameraServiceMock.h src/core/services/FtpUploader.cpp tests/ViewModelSmokeTests.cpp tests/QmlUiAuditTests.cpp
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1 *> build\phase13-canonical-verify.log
```

## Full Build Notes

The canonical script completed successfully on 2026-06-25. It rebuilt the Qt6 executable, built `ViewModelSmokeTests.exe`, `QmlUiAuditTests.exe`, CLI targets, and E2E tests, then reported:

- `[UI] QML UI audit tests passed`
- `[E2E] All pipeline tests passed`

An earlier run failed at post-build DLL copy for `ffmpeg/freetype.dll`. Root-cause checks showed the source and target DLLs were accessible and `cmake -E copy_if_different` succeeded when run directly; rerunning the canonical script completed successfully. No source change was required for that transient environment failure.

Known existing warnings remain from third-party/source build inputs, including CGAL data-dir warnings, Qt minimum-CMake warnings, MSVC codepage warnings in dependency/upstream headers, and existing discarded `QFuture` warnings.

## Manual Verification Notes

Automated Phase 13 coverage proves protocol parsing, payload construction, routing, fallback behavior, and persistence contracts. It does not prove live printer transport:

- MQTT live publish still requires a reachable printer/broker, a valid device serial, and LAN access code.
- FTP live upload still requires a printer accepting Bambu LAN FTP credentials.
- RTSP live decode still requires a camera-capable printer or controlled RTSP fixture plus compatible runtime codec support.
