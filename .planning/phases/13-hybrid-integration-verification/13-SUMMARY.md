---
phase: 13
phase_name: Hybrid Integration Verification
plan_id: 13-01
status: complete
completed: 2026-06-25
implementation_commit: 8cb04d4
requirements_completed:
  - INT-01
  - INT-02
  - INT-03
  - INT-04
  - INT-05
  - INT-06
key_files:
  modified:
    - src/core/services/SsdpDiscovery.cpp
    - src/core/services/SsdpDiscovery.h
    - src/core/services/DeviceServiceMock.cpp
    - src/core/services/DeviceServiceMock.h
    - src/core/services/FtpUploader.cpp
    - src/core/services/CameraServiceMock.cpp
    - src/core/services/CameraServiceMock.h
    - tests/ViewModelSmokeTests.cpp
    - tests/QmlUiAuditTests.cpp
---

# Phase 13 Summary: Hybrid Integration Verification

## What Changed

- Replaced the LAN-dependent SSDP smoke check with deterministic Bambu and Creality response fixtures.
- Extracted `SsdpDiscovery::parseResponseDatagram()` and reused it from the live UDP receive path.
- Extracted MQTT report and command helpers in `DeviceServiceMock`:
  - `applyMqttReportPayload()`
  - `buildPrintCommandEnvelope()`
  - `buildPrintCommandTopic()`
  - `buildPrintRemotePath()`
- Covered MQTT telemetry fields for status, progress, nozzle/bed temperatures, layers, and remaining time.
- Covered pause, resume, stop, and G-code print command JSON envelopes without requiring a live broker.
- Fixed the MQTT connected-device index path so filtered device lists resolve to the real device index before telemetry or command routing.
- Fixed FTP URL construction so access codes containing spaces or slashes are percent-encoded correctly and are not corrupted by chained `QString::arg()` substitutions.
- Added deterministic FTP missing-file error and send-print refusal coverage without connecting to a printer.
- Added `CameraServiceMock::defaultRtspUrlForDevice()` and covered default RTSP URL derivation plus no-camera/state-machine behavior.
- Added scoped `QSettings` regression coverage for `AppSettingsService` bed-size clamping/persistence and `EditorViewModel` bed-shape persistence.
- Added static QML audit coverage proving `SoftwareViewport` is the default `GLViewport` registration unless `OWZX_OPENGL` selects the OpenGL renderer.

## Verification

- Phase 13 targeted tests passed: 8 passed, 0 failed.
- Full `ViewModelSmokeTests.exe` passed: 32 passed, 0 failed.
- QML UI audit passed: 6 passed, 0 failed.
- `git diff --check` passed for touched source and test files.
- Canonical verification passed:
  - `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
  - Release build completed.
  - QML UI audit tests passed.
  - E2E pipeline tests passed.

## Remaining Work

- Live MQTT publish verification still requires a reachable printer/broker, serial, and LAN access code.
- Live FTP upload verification still requires a printer accepting the Bambu LAN FTP credentials.
- Live RTSP verification still requires camera-capable hardware or a controlled RTSP fixture plus runtime codec support.
- Phase 14 remains responsible for visible placeholder/no-op UI triage.
