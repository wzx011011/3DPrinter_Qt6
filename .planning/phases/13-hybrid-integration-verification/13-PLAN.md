---
phase: 13
phase_name: Hybrid Integration Verification
plan_id: 13-01
title: Add deterministic hybrid integration evidence
status: planned
wave: 1
type: tdd
autonomous: true
requirements_addressed:
  - INT-01
  - INT-02
  - INT-03
  - INT-04
  - INT-05
  - INT-06
files_modified:
  - src/core/services/SsdpDiscovery.cpp
  - src/core/services/SsdpDiscovery.h
  - src/core/services/DeviceServiceMock.cpp
  - src/core/services/DeviceServiceMock.h
  - src/core/services/CameraServiceMock.cpp
  - src/core/services/CameraServiceMock.h
  - tests/ViewModelSmokeTests.cpp
  - tests/QmlUiAuditTests.cpp
  - .planning/phases/13-hybrid-integration-verification/13-SUMMARY.md
  - .planning/phases/13-hybrid-integration-verification/13-VERIFICATION.md
  - .planning/phases/13-hybrid-integration-verification/13-REVIEW.md
  - .planning/REQUIREMENTS.md
  - .planning/ROADMAP.md
  - .planning/STATE.md
---

# Plan 13-01: Add deterministic hybrid integration evidence

<objective>
Turn SSDP, MQTT, FTP, camera, software viewport, and settings/bed-shape hybrid surfaces from fallback/no-crash confidence into deterministic automated evidence, while documenting which real-printer behaviors still require live hardware.
</objective>

<tasks>

1. Write failing deterministic integration tests first.
   - Files: `tests/ViewModelSmokeTests.cpp`, `tests/QmlUiAuditTests.cpp`
   - Action: Replace the LAN-dependent SSDP test with fixture parser assertions for Bambu and Creality responses plus deterministic timeout/error-path coverage.
   - Action: Add MQTT tests for telemetry JSON parsing and print command envelope/topic construction for `pause`, `resume`, `stop`, and `gcode_file`.
   - Action: Add FTP tests for access-code URL encoding, missing-local-file error signal, and send-print refusal without MQTT.
   - Action: Add camera tests for offline/no-camera error behavior and default RTSP URL derivation/state-machine coverage without requiring a real stream.
   - Action: Add AppSettings/EditorViewModel QSettings persistence tests with scoped restoration of original values.
   - Action: Add static QML audit coverage proving default startup registers `SoftwareViewport` unless `OWZX_OPENGL` selects OpenGL.
   - Verify: Build and run the targeted tests and confirm they fail for missing APIs or insufficient behavior before production edits.
   - Acceptance criteria: INT-01..INT-06 each have a failing test that describes the intended evidence.

2. Extract and reuse production helpers for protocol parsing and payload construction.
   - Files: `src/core/services/SsdpDiscovery.h`, `src/core/services/SsdpDiscovery.cpp`, `src/core/services/DeviceServiceMock.h`, `src/core/services/DeviceServiceMock.cpp`
   - Action: Add a public static SSDP datagram parser and make `onReadyRead()` use it.
   - Action: Add MQTT telemetry application and print command payload/topic helpers, and make `connectViaMqtt()` / `publishPrintCommand()` use them.
   - Action: Ensure helper behavior is source-truth aligned enough for protocol-level regression: Bambu port `8883`, non-Bambu port `1883`, Bambu JSON envelope under `print`, and `device/<serial>/request` topics.
   - Verify: RED tests from task 1 turn green without connecting to a live printer.
   - Acceptance criteria: INT-01 and INT-02 have deterministic parser/payload coverage through production code.

3. Close FTP, camera, and settings evidence gaps without overstating real hardware support.
   - Files: `src/core/services/CameraServiceMock.h`, `src/core/services/CameraServiceMock.cpp`, `tests/ViewModelSmokeTests.cpp`
   - Action: Expose a small camera URL helper if needed to assert default RTSP URL derivation without inspecting private state indirectly.
   - Action: Keep FTP tests on URL/error/routing behavior; do not fake a successful FTPS upload.
   - Action: Verify AppSettings clamping/persistence and EditorViewModel bed-shape persistence; add `QSettings::sync()` in bed-shape setters only if RED test proves persistence is not reliable.
   - Verify: Targeted tests pass and no live printer/RTSP dependency is introduced.
   - Acceptance criteria: INT-03, INT-04, and INT-06 have deterministic automated coverage and clear manual gap notes.

4. Run verification and write Phase 13 closeout artifacts.
   - Files: `.planning/phases/13-hybrid-integration-verification/13-SUMMARY.md`, `.planning/phases/13-hybrid-integration-verification/13-VERIFICATION.md`, `.planning/phases/13-hybrid-integration-verification/13-REVIEW.md`, `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/STATE.md`
   - Action: Run targeted tests, full `ViewModelSmokeTests.exe`, `QmlUiAuditTests.exe`, `git diff --check`, and the canonical PowerShell verification command.
   - Action: Document real-printer/manual verification requirements separately from automated regression evidence.
   - Action: Mark INT-01..INT-06 complete only with evidence or explicit deferral recorded.
   - Verify: `gsd-sdk query roadmap.analyze` identifies Phase 14 as next.
   - Acceptance criteria: Phase 13 artifacts are complete and traceability is updated.

</tasks>

<verification>

- Targeted RED/green tests:
  `build\\ViewModelSmokeTests.exe int01_SsdpDiscoveryParsesMockResponse int04_MqttConnectionParamsAndTelemetryFields int05_MqttCommandConstructionAndControlFlow int06_FtpUrlAndSendPrintRouting int03_CameraStateMachineAndFrameToken -o build\\ViewModelSmokeTests.phase13-targeted.txt,txt`
- Full ViewModel smoke tests:
  `build\\ViewModelSmokeTests.exe -o build\\ViewModelSmokeTests.phase13-full.txt,txt`
- Static UI audit:
  `build\\QmlUiAuditTests.exe -o build\\QmlUiAuditTests.phase13-qml.txt,txt`
- Source check:
  `git diff --check -- src/core/services/SsdpDiscovery.cpp src/core/services/SsdpDiscovery.h src/core/services/DeviceServiceMock.cpp src/core/services/DeviceServiceMock.h src/core/services/CameraServiceMock.cpp src/core/services/CameraServiceMock.h tests/ViewModelSmokeTests.cpp tests/QmlUiAuditTests.cpp .planning/phases/13-hybrid-integration-verification`
- Canonical verification:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

</verification>

<success_criteria>

- INT-01 through INT-06 are all addressed by deterministic tests, explicit manual notes, or documented deferral.
- No Phase 13 automated test requires a live printer, MQTT broker, FTPS server, RTSP stream, multicast LAN response, or external internet.
- Existing production call sites use the extracted helpers instead of duplicating test-only logic.
- Software viewport default startup evidence is tied to `main_qml.cpp` and canonical smoke behavior.
- Canonical verification passes and evidence is recorded.

</success_criteria>

## PLANNING COMPLETE
