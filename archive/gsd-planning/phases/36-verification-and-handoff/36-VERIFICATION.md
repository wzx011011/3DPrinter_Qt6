---
phase: 36
status: passed
verified_at: 2026-06-28
command: powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
requirements:
  - TEST-01
  - TEST-02
  - TEST-03
---

# Phase 36 Verification

## Result

The canonical verifier exited `0` on 2026-06-28 after the Phase 33-35 changes were committed.

## Automated Gate

Command:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Observed results:

- Build/configure completed in `build/` using the canonical script.
- `PrepareSceneDataTests.exe` passed.
- `PartPlateTests.exe` passed.
- `QmlUiAuditTests.exe` passed.
- `E2EWorkflowTests.exe` passed.
- Smoke launch reported `APP_RUNNING_PID=2104` before the verifier stopped that process.
- After verification cleanup, no stale `OWzxSlicer` process was left running.

## Rendering Backend Evidence

Latest startup diagnostic at verification time:

```text
2026-06-28T16:03:49.162 QRhi backend selection: enabled=true requested=auto selected=d3d11 attempts=[d3d11:ok]
```

## Requirement Coverage

| Requirement | Evidence | Status |
|---|---|---|
| TEST-01 | `E2EWorkflowTests::test_backend_switches_to_preview_after_slice` covers slice completion, output path, Preview data, layer count, move count, and Preview navigation. | Passed |
| TEST-02 | `E2EWorkflowTests::test_preview_parser_handles_extrusion_modes_and_travel_filter` covers absolute/relative extrusion, `G92 E`, layer Z, travel/extrude split, tool changes, and travel visibility filtering. | Passed |
| TEST-03 | `QmlUiAuditTests::previewRhiRendererBindsPreviewStateAndUsesExactDrawSpans` guards QRhi Preview bindings, exact draw spans, move cutoff, and no direct `SoftwareViewport` normal-path instantiation. | Passed |

## Manual UAT Handoff

The user-facing UAT path for the running app is:

1. Load a model in Prepare.
2. Start slicing from the normal Prepare workflow.
3. Confirm successful slicing enters Preview automatically.
4. Confirm Preview shows non-empty toolpath rendering.
5. Exercise layer range, move slider, travel toggle, and color mode.

Manual UAT remains the next human check; automated v3.3 gates passed.
