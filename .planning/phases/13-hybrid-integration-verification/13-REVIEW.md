---
phase: 13
phase_name: Hybrid Integration Verification
status: reviewed
reviewed: 2026-06-25
scope:
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

# Phase 13 Code Review

## Findings

No open blocking findings remain.

## Fixed During Review

- `src/core/services/SsdpDiscovery.h`: removed the stale private `parseResponse(...)` declaration after extracting the public static parser.
- `src/core/services/DeviceServiceMock.cpp`: made `stopPrint()` publish a stop command when MQTT is connected.
- `src/core/services/DeviceServiceMock.cpp`: moved pause/resume/stop MQTT publish before local `printJobs_` checks so live printer commands are not blocked by mock-only job state.
- `src/core/services/DeviceServiceMock.cpp`: fixed `connectViaMqtt()` to store the real selected device index through `selectedDeviceIndex()` rather than the filtered list index.
- `tests/ViewModelSmokeTests.cpp`: added resume/stop command envelope assertions and filtered-device telemetry regression coverage.

## Review Notes

- SSDP fixture parsing now uses production parser code and no longer depends on multicast or ambient LAN devices.
- MQTT telemetry parsing, print command envelope construction, and print topic construction are deterministic and covered through production helpers.
- FTP URL construction now avoids a chained `QString::arg()` hazard where percent-encoded sequences could be interpreted as later placeholders.
- QSettings tests use scoped snapshot/restore so local user settings are not left modified after the test process exits.
- QML startup audit is static by design; the canonical app smoke covers runtime startup without `OWZX_OPENGL`.

## Residual Risk

- The device integration services remain Hybrid. Protocol-level evidence is automated, but real MQTT broker publish, FTP upload, and RTSP decode still need hardware or controlled fixtures.
- `DeviceServiceMock` still carries a legacy `*Mock` name while containing hybrid real paths. Phase 13 documents and tests the behavior but does not rename the service.
- Existing source files still contain unrelated historical mojibake comments outside the Phase 13 behavior path; broader visible UI/text cleanup remains separate work.
