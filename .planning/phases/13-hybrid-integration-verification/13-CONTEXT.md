# Phase 13: Hybrid Integration Verification - Context

**Gathered:** 2026-06-25
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 13 replaces confidence from fallback/no-crash behavior with deterministic evidence for the existing hybrid integration surfaces: SSDP discovery parsing, MQTT telemetry/control payloads, FTP send-print routing, camera no-stream/error handling, software viewport fallback startup, and AppSettings / bed-shape persistence. This phase verifies and lightly extracts existing behavior; it must not claim real printer, RTSP, MQTT broker, or FTPS success without hardware/credential evidence.

</domain>

<decisions>
## Implementation Decisions

### Deterministic Protocol Evidence
- Replace the current LAN-dependent SSDP smoke test with Bambu/Creality fixture parsing and a deterministic timeout/error-path check.
- Extract parser/build helpers only where they are production-useful: SSDP datagram parsing, MQTT report payload application, MQTT print command envelope/topic construction, and FTP remote-path/URL construction.
- Keep tests independent of live printers, brokers, RTSP streams, multicast-capable LANs, and internet availability.
- Treat malformed payloads and missing local files as explicit error-path coverage, not as skipped environmental cases.

### Hybrid Boundary
- Do not introduce fake "connected printer" success that would make MQTT/FTPS look Real.
- Automated tests may prove protocol-level construction and service state transitions; live publish, upload, and video decode remain manual/hardware verification.
- Keep `*Mock` service names as current architecture, but classify each verified surface as Hybrid unless a real end-to-end path is proven.
- Preserve existing fallback behavior where it is explicitly mock/demo behavior.

### Source-Truth Anchors
- Use OrcaSlicer upstream files as references for user-visible integration semantics:
  `BonjourDialog.*`, `DeviceManager.*`, `SendToPrinter.cpp`, `StatusPanel.cpp`, `MediaPlayCtrl.*`, `wxMediaCtrl2.*`, and `Tab.cpp` printable-area / bed-shape handling.
- Do not modify libslic3r or upstream third-party sources.
- Keep protocol behavior in C++ services/viewmodels; QML must not own parsing, persistence, or transport semantics.
- Document blocked manual requirements separately from automated regression checks.

### Verification Shape
- Add focused tests to existing Qt test targets where they naturally fit: `ViewModelSmokeTests` for service/viewmodel behavior and `QmlUiAuditTests` for static startup fallback evidence.
- Run targeted RED tests before production edits, then rerun targeted and full tests after implementation.
- Final verification must include the canonical PowerShell command from the build rules.
- Completion evidence must map INT-01 through INT-06 individually.

### the agent's Discretion
- Choose the smallest stable C++ helper signatures needed for deterministic tests and reuse them from production call sites.
- Prefer readable JSON assertions over string-substring payload checks when verifying MQTT envelopes.
- If AppSettings and Editor bed-shape persistence overlap through QSettings, verify the current contract and record remaining unification work rather than broad refactoring in Phase 13.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/core/services/SsdpDiscovery.*` owns UDP discovery and response parsing but currently hides the parser behind a private method.
- `src/core/services/DeviceServiceMock.*` owns MQTT telemetry parsing, print-control publish routing, FTP send-print routing, and mock print state.
- `src/core/services/FtpUploader.*` owns Bambu-style FTP URL construction and missing-local-file error handling.
- `src/core/services/CameraServiceMock.*` owns camera availability, mock stream state, RTSP decoder start/stop, and no-camera error state.
- `src/core/services/AppSettingsService.*` and `src/core/viewmodels/EditorViewModel.*` persist bed dimensions through `QSettings`.
- `src/qml_gui/main_qml.cpp` registers `SoftwareViewport` as the default `OWzxGL.GLViewport` implementation unless `OWZX_OPENGL` is set.

### Established Patterns
- Existing Qt service tests live in `tests/ViewModelSmokeTests.cpp` and use direct construction plus `QSignalSpy`.
- QML/static UI checks live in `tests/QmlUiAuditTests.cpp`.
- Full verification must use `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` and build only in `build/`.
- GSD artifacts track requirement completion in `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, and `.planning/STATE.md`.

### Integration Points
- `NetworkServiceMock::discoverDevices()` consumes `SsdpDiscovery` output.
- `MonitorViewModel` forwards `DeviceServiceMock` telemetry and camera fields to QML.
- `DeviceServiceMock::publishPrintCommand()` and `sendPrintViaFtp()` are the service boundary for MQTT/FTP print controls.
- `scripts/auto_verify_with_vcvars.ps1` starts `OWzxSlicer.exe` without setting `OWZX_OPENGL`, so its startup smoke covers the default software viewport registration path.

</code_context>

<specifics>
## Specific Ideas

Autonomous default selected: one focused implementation plan that first adds failing tests, then extracts production helpers and updates documentation. No user decision is needed because the roadmap scope is verification/stabilization, not a new feature design.

</specifics>

<deferred>
## Deferred Ideas

- Real printer MQTT publish verification requires printer IP, LAN access code, and a reachable broker.
- Real FTPS upload verification requires a printer accepting Bambu LAN FTP credentials.
- Real RTSP/video decode verification requires a camera-capable printer or RTSP fixture plus available runtime codec dependencies.
- Broad device-service renaming away from `*Mock` remains outside Phase 13.

</deferred>
