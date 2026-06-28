# Phase 41 Verification

**Date:** 2026-06-29
**Status:** Passed

## RED Evidence

Command:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Result before implementation:

- Build completed.
- QML UI audit failed as expected after adding the RED test.
- Focused output in `build/qml_phase41_red.txt`:
  - `QmlUiAuditTests::previewRhiRendererResetsGpuStateAfterResourceRelease`
  - Failure: `RhiViewportRenderer must centralize Preview GPU state reset for QRhi resource rebuilds`

## Focused GREEN Evidence

Command:

```powershell
.\build\QmlUiAuditTests.exe -o build\qml_phase41_green.txt,txt
```

Result:

```text
Totals: 21 passed, 0 failed, 0 skipped, 0 blacklisted
```

## Canonical Verification

Command:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Result:

- Build completed successfully.
- Prepare scene data tests passed.
- PartPlate tests passed.
- QML UI audit tests passed.
- App smoke launched successfully with `APP_RUNNING_PID=36916`.
- E2E pipeline tests passed.

Relevant final output:

```text
[UI] QML UI audit tests passed
APP_RUNNING_PID=36916

[E2E] Running pipeline tests...
[E2E] All pipeline tests passed
```

## Whitespace Check

Command:

```powershell
git diff --check
```

Result:

```text
passed
```

## Requirement Evidence

- `PREVIEW-05`: Existing backend audits plus canonical app smoke confirm the normal path remains `RhiViewport`/QRhi with D3D11-first policy and no direct `SoftwareViewport` usage in `PreviewPage.qml`.
- `PREVIEW-06`: New audit coverage confirms layer/move/toggle setters preserve the same Preview payload and the renderer resets/reuploads GPU state after QRhi resource rebuild.
- `PREVIEW-07`: Camera fit/orbit/pan/zoom paths keep payload stable, upload camera uniform before `beginPass()`, and QRhi resource release now forces Preview buffer reupload.
- `PREVIEW-08`: Pure range/camera interactions no longer rebuild or reparse `GCV1`; they redraw over existing CPU/GPU segment data. Payload changes and resource rebuilds are the only reupload paths.
