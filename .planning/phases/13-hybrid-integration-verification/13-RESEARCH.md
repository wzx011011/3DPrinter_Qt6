# Phase 13 Research: Hybrid Integration Verification

## Upstream Source Truth

- `third_party/OrcaSlicer/src/slic3r/GUI/BonjourDialog.*`
  - Upstream discovery UI is network-backed and can receive discovered printer data asynchronously.
- `third_party/OrcaSlicer/src/slic3r/GUI/DeviceManager.*`
  - Device state, command routing, and machine records are upstream-owned integration behavior.
- `third_party/OrcaSlicer/src/slic3r/GUI/SendToPrinter.cpp`
  - Send-to-printer flow contains FTP/local-network handling and Bambu access-code URL construction (`bambu:///local/...user=bblp&passwd=...`).
- `third_party/OrcaSlicer/src/slic3r/GUI/StatusPanel.cpp`
  - Pause/resume/abort UI delegates to machine-object command methods and owns visible camera/monitoring controls.
- `third_party/OrcaSlicer/src/slic3r/GUI/MediaPlayCtrl.*` and `third_party/OrcaSlicer/src/slic3r/GUI/wxMediaCtrl2.*`
  - Upstream camera playback is a media-control/RTSP integration surface with real runtime dependencies.
- `third_party/OrcaSlicer/src/slic3r/GUI/Tab.cpp`
  - `printable_area` / bed-shape configuration is edited through a bed-shape widget and persisted through config state.

## Current Qt6 Implementation

- `src/core/services/SsdpDiscovery.*`
  - Real UDP M-SEARCH exists, but `tests/ViewModelSmokeTests.cpp::int01_SsdpDiscoveryParsesMockResponse()` currently starts real LAN discovery and skips if no devices respond.
  - Parser is private, so protocol-level coverage is not deterministic.
- `src/core/services/DeviceServiceMock.*`
  - MQTT connection wrapper exists through `MqttClient`.
  - Telemetry parsing is currently embedded in the `messageReceived` lambda.
  - `publishPrintCommand()` constructs Bambu-style JSON only after a real MQTT connection, which prevents deterministic payload assertions.
  - Pause/resume currently publish only when connected; stop uses mock state only.
- `src/core/services/FtpUploader.*`
  - `buildFtpUrl()` is deterministic and already percent-encodes the access code.
  - `uploadFile()` has deterministic missing-file and already-uploading error paths.
  - `DeviceServiceMock::sendPrintViaFtp()` refuses to send when MQTT is not connected, then builds `/mnt/sdcard/<base>.gcode` on connected paths.
- `src/core/services/CameraServiceMock.*`
  - No-camera/offline start path sets an error message.
  - Online path auto-builds a Bambu RTSP URL and advances through mock Connecting / Connected / Streaming state before attempting a decoder.
  - Real RTSP decode still requires camera-capable hardware or a controlled RTSP fixture.
- `src/core/services/AppSettingsService.*`
  - Persists `Bed/Width` and `Bed/Depth`, clamps to `50..2000`, and syncs QSettings.
- `src/core/viewmodels/EditorViewModel.*`
  - Persists UI bed-shape values under `bed/width`, `bed/depth`, `bed/maxHeight`, `bed/originX`, `bed/originY`, `bed/shapeType`, and `bed/diameter`.
  - Setters do not currently call `QSettings::sync()`; tests should verify current behavior and identify any missing sync if it fails.
- `src/qml_gui/main_qml.cpp`
  - Default startup registers `SoftwareViewport` as `OWzxGL.GLViewport`; setting `OWZX_OPENGL` selects `GLViewport`.
  - The canonical script starts the app without `OWZX_OPENGL`, so its smoke launch covers the default software path.

## Risks and Pitfalls

- A test that starts UDP multicast or waits for LAN devices does not satisfy INT-01.
- A test that checks "no crash" without parsing JSON payloads does not satisfy INT-02.
- Forcing `DeviceServiceMock` into a fake connected state could overstate real MQTT/FTPS completion; prefer pure helper construction tests.
- `QSettings` tests must restore original user settings to avoid damaging the developer's local OWzx preferences.
- Camera tests should not depend on FFmpeg/RTSP being available in CI.
- Startup smoke evidence should state whether it covers default software viewport or explicit OpenGL mode.

## Implementation Direction

- Add public/static parser and payload helpers with production-oriented names:
  - `SsdpDiscovery::parseResponseDatagram(...)`
  - `DeviceServiceMock::applyMqttReportPayload(...)`
  - `DeviceServiceMock::buildPrintCommandEnvelope(...)`
  - `DeviceServiceMock::buildPrintCommandTopic(...)`
  - optional remote-path helper for FTP if needed.
- Rewire existing production code to use the helper methods so tests cover real code paths, not duplicated logic.
- Add tests:
  - SSDP Bambu and Creality fixture parsing; deterministic timeout via slot invocation or zero-response path.
  - MQTT telemetry parsing from nested/direct `print` JSON and command envelope/topic construction for pause/resume/stop/gcode_file.
  - FTP URL percent encoding, missing-file `uploadFinished(false, ...)`, and send-print refusal without MQTT.
  - Camera offline/no-camera rejection and online default RTSP URL state-machine path.
  - AppSettings persistence/clamping and EditorViewModel bed-shape persistence with scoped QSettings restoration.
  - Static audit that `main_qml.cpp` defaults to `SoftwareViewport` unless `OWZX_OPENGL` is set.

## Verification Targets

- RED targeted tests before production edits.
- Targeted `ViewModelSmokeTests` after helper implementation.
- Full `ViewModelSmokeTests.exe`.
- `QmlUiAuditTests.exe`.
- `git diff --check` on changed files.
- Canonical command:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
