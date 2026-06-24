# Code Drift Audit

Date: 2026-06-23

Purpose: compare the current Qt6 implementation against the planning state and identify where the backlog has drifted from real code.

This audit is evidence for `.planning/REMAINING_MIGRATION_PLAN.md`. It is not an implementation plan by itself.

## Summary

The implementation is ahead of the old plan in several backend areas, but the product status is less complete than some milestone wording implies.

Main drift pattern:

- Several `*Mock` services now contain real transport or libslic3r paths plus mock fallback behavior.
- Some UI surfaces are present but disabled, silent no-ops, or wired only to local mock state.
- Several tests validate fallback/no-crash behavior rather than source-truth behavior.
- Planning must therefore use `Real / Hybrid / Mock / Blocked`, not only `complete / incomplete`.

## Service And Workflow Drift Matrix

| Area | Code status | Planning drift | Evidence |
|---|---|---|---|
| Composition root | Hybrid services are still injected as mock-named services | Docs can overstate completion if `*Mock` suffix is ignored | `src/qml_gui/BackendContext.cpp` constructs `CalibrationServiceMock`, `PresetServiceMock`, `DeviceServiceMock`, `NetworkServiceMock`, `CameraServiceMock`, `CloudServiceMock` |
| Calibration | Hybrid/partial | Plan said calibration was deferred, but code has partial real SliceService integration; UI topbar remains disabled | `CalibrationServiceMock::startCalibration()` maps PA/Flow/Temp to `SliceService::setCalibParams()` and `startSlice()`; `BBLTopbar.qml` calibration menu entries are disabled |
| Calibration fallback | Bug risk | Literal `\r\n` text appears inside a `//` comment before fallback timer code, likely commenting out mock fallback code | `src/core/services/CalibrationServiceMock.cpp`, bottom of `startCalibration()` |
| SSDP discovery | Hybrid | Real UDP discovery exists, but device list is still mostly separate mock state | `SsdpDiscovery` uses `QUdpSocket`; `NetworkServiceMock::discoverDevices()` runs discovery; `DeviceServiceMock` still builds mock devices |
| Camera | Hybrid | Real FFmpeg/RTSP path exists, but tests mainly exercise empty-url mock state machine | `CameraStream` uses FFmpeg input/decode APIs; `CameraServiceMock::startStream()` falls back when no camera URL exists |
| MQTT/FTP/print send | Hybrid | Real wrappers exist, but current tests validate disconnected fallback and mock job state more than live printer behavior | `DeviceServiceMock::connectViaMqtt()`, `sendPrintViaFtp()`, `publishPrintCommand()`, `startPrint()` |
| Preset import/export | Partial | Planning may call preset IO real, but implementation is simplified JSON bundle, not upstream-compatible `.zip/.bbscfg` behavior | `PresetServiceMock::exportBundle()` writes JSON; `importBundle()` reads JSON `presets` array |
| PartPlate / plate scoped config | Mock/partial | Visual plate support exists, but upstream PartPlate semantics are not real enough | `ProjectServiceMock::plateScopedOptionValue()` has fallback/TODO behavior; `setPlateScopedOption()` is not a real upstream path |
| AssembleView | Placeholder | UI/router has an AssembleView enum but no source-truth implementation | `BackendContext::ViewMode::AssembleView` is documented as placeholder |
| Prepare / Preview top-level actions | Partial | Visible actions still contain no-op/TODO handlers | `main.qml` export project/model/preferences handlers are TODO; `GLToolbars.qml` layer editing is disabled |
| Preset/sidebar QML | Partial | Some controls are visual placeholders or carry logic in QML instead of C++ model/viewmodel | `LeftSidebar.qml`, `ParamsPage.qml`, preset filter/compare/settings placeholders |
| Model mall / web | Mock/blocked | WebView is hardcoded unavailable while pages exist | `ModelMallViewModel::webViewAvailable()` returns false |
| Multi-machine | Mock/partial | UI has broad coverage but state is mock/local rather than real device task state | `MultiMachineViewModel` contains mock data and mock messages |

## Code-Level Findings

### 1. Calibration is not simply "deferred"

`CalibrationServiceMock::startCalibration()` already dispatches several calibration modes through `SliceService`:

- `flow_dynamics` -> mode `1`
- `flow_rate` -> mode `5`
- `temp_tower` -> mode `6`

The topbar calibration entries are still disabled, and other calibration types do not have a `CalibMode` mapping. The remaining plan should be "complete and verify partial real calibration", not "start calibration from zero".

Action needed:

- Fix or confirm the literal `\r\n` comment issue around fallback timer code.
- Wire implemented modes into the UI.
- Add deterministic regression tests that do not skip when environment slicing is unavailable.
- Mark unmapped modes explicitly as Pending or Blocked.

### 2. Device integration is real-path plus mock fallback

The codebase now has real-ish transport pieces:

- SSDP discovery through UDP multicast.
- MQTT wrapper and Bambu-style status/control topics.
- FTP upload path for send-to-printer.
- RTSP/FFmpeg camera stream path.

However, the UI and tests still lean heavily on mock fallback:

- `DeviceServiceMock` builds static devices.
- MQTT tests check disconnected behavior and command fallback.
- Camera tests use empty-url/no-decoder paths.

Action needed:

- Add deterministic simulator/fixture tests for SSDP, MQTT payload parsing, FTP error paths, and camera frame decode or explicit camera blocking.
- Split each service contract into real transport, fallback state, and visible support status.

### 3. Preset bundle behavior is simplified

Current import/export uses JSON written by `PresetServiceMock`, while the UI text mentions upstream `.zip/.bbscfg` style behavior. This is acceptable as a temporary bridge, but it cannot be counted as source-truth complete.

Action needed:

- Decide whether Qt6 must support upstream bundle formats directly in v3.0.
- Add compatibility tests for upstream-like sample bundles.
- Move filtering/dirty-state logic out of QML into C++ model/viewmodel where it affects behavior.

### 4. Placeholder UI is still user-visible

Examples include:

- Export project/model/preferences handlers in `main.qml`.
- Disabled account/model store/publish buttons in `BBLTopbar.qml`.
- Disabled calibration menu entries in `BBLTopbar.qml`.
- Layer editing disabled in `GLToolbars.qml`.
- Sidebar Simple/Advanced, Compare, object layers, and params placeholder paths.
- `ModelMallViewModel::webViewAvailable()` hardcoded `false`.

Action needed:

- Treat visible disabled/no-op UI as migration debt, even when the surrounding page exists.
- Do not mark a workflow `[x]` until the button/menu path reaches a viewmodel/service or is explicitly classified blocked.

### 5. Planning source files are not fully aligned with repo instructions

The repo instructions reference `.Codex/rules/source-truth-migration.md` and `.Codex/rules/build-rules.md`, but those files are missing in this checkout.

Action needed:

- Restore those files from canonical source or replace the references with the active equivalent.
- Keep `AGENTS.md`, `.planning/INDEX.md`, and `.planning/REMAINING_MIGRATION_PLAN.md` aligned.

## Revised Classification Rule

Use these status terms going forward:

- `Real`: source-truth behavior implemented and verified with deterministic evidence.
- `Hybrid`: real path exists, but fallback/mock behavior remains or verification is incomplete.
- `Mock`: UI/service is local simulation only.
- `Blocked`: source-truth behavior requires unavailable dependency, protocol, credential, or product decision.
- `Placeholder`: visible UI or enum exists but has no meaningful backend behavior.

Do not use milestone completion language as product completion evidence.

