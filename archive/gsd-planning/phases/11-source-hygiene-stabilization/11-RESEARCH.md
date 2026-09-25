# Phase 11: Source Hygiene Stabilization - Research

**Researched:** 2026-06-25
**Status:** Complete

## Findings

- `src/core/services/CalibrationServiceMock.cpp` contains a behavior-affecting literal `\r\n` artifact in `startCalibration()`. Because it is embedded in a `//` comment line, the intended `if (!m_sliceService) { m_timer->start(); }` fallback branch is commented out.
- `src/core/services/SliceService.cpp` and `src/core/services/SliceService.h` contain encoding-damaged comments and user-visible strings introduced in the dirty baseline. Several `QObject::tr()` and `QStringLiteral()` labels are unreadable.
- `src/core/services/SliceService.cpp.backup` is an untracked backup file under active source. Repository and build searches do not show it referenced by CMake or source includes.
- Untracked implementation files are not all external junk:
  - `src/core/services/AppSettingsService.*` are referenced from `CMakeLists.txt`, `BackendContext`, and `SliceService`.
  - `src/qml_gui/Renderer/SoftwareViewport.*` are referenced from `CMakeLists.txt` and `main_qml.cpp`.
  - `tests/QmlUiAuditTests.cpp` is referenced from `CMakeLists.txt` and `.planning/quick/260625-0cz-ui`.
- The phase should not remove or revert those implementation files. They should be classified as intentional implementation work pending later verification, while the source-adjacent backup file should be removed from active `src/`.

## Verification Targets

- Literal artifact scan should only return intentional protocol strings such as SSDP CRLF literals and planning/audit mentions.
- Encoding-damage scan should no longer find damaged tokens in the repaired active source files.
- `SliceService.cpp.backup` should no longer exist under `src/`.
- Canonical full build command should be attempted after edits: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.

## RESEARCH COMPLETE
