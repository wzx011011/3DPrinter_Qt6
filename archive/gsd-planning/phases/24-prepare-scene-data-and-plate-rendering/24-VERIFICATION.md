---
phase: 24
status: passed
verified: 2026-06-27
canonical_command: "powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1"
requirements: [RHI-04, PREP-01, PREP-05]
---

# Phase 24 Verification

## Scope

Phase 24 verifies the Prepare scene/cache and QRhi bed/plate rendering foundation:

- CPU-side `PrepareSceneData` bed/grid/origin vertices, active plate context, and dirty flags.
- `EditorViewModel` active plate membership exposed to the renderer without QML filtering.
- QRhi Prepare bed fill and grid/origin line rendering with private renderer-owned buffers and pipelines.
- Dirty-gated QRhi scene uploads for Prepare bed/plate state.

This phase does not claim full Prepare model mesh rendering, selection/hover, camera interaction, gizmos, or Preview G-code rendering. Those remain Phase 25-27 scope.

## Focused Checks

| Command | Exit | Evidence |
|---------|------|----------|
| `build\PrepareSceneDataTests.exe` | 0 | Scene data tests passed, including dirty flag consumption, invalid bed bounds, active plate isolation, and plate dirty transitions. |
| `build\ViewModelSmokeTests.exe activePlateObjectIndicesFollowCurrentPlateWithoutFallback` | 0 | Active plate renderer membership follows `ProjectServiceMock::currentPlateObjectIndices()` and does not use the show-all UI fallback. |
| `build\QmlUiAuditTests.exe` | 0 | QML boundary, default fallback registration, RHI gating, viewport property parity, and QRhi dirty-upload source guards passed. |

## Canonical Verification

Command:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Result: exit code 0 on 2026-06-27.

Key output lines:

- `[PrepareScene] Prepare scene data tests passed`
- `[UI] QML UI audit tests passed`
- `APP_RUNNING_PID=31500`
- `[E2E] All pipeline tests passed`

This was run from the repository root and used the required `build/` directory.

## Explicit QRhi Smoke

Command:

```powershell
$env:OWZX_RHI_RENDERER='1'; Start-Process .\OWzxSlicer.exe -WorkingDirectory build
```

Result: exit code 0 for the smoke wrapper; app stayed running for 5 seconds and was then stopped.

Evidence:

- `RHI_APP_RUNNING_PID=32916`
- `startup_diagnostics.log`: `QRhi backend selection: enabled=true requested=auto selected=d3d12 attempts=[d3d12:ok]`

## Requirement Evidence

| Requirement | Phase 24 Status | Evidence |
|-------------|-----------------|----------|
| RHI-04 | Complete for Prepare scene/cache; Preview segment buffers deferred to Phase 26 | `PrepareSceneData` dirty flags, `RhiViewportRenderer` `m_bedFillBuffer` / `m_bedLineBuffer`, dirty-gated `uploadSceneBuffers()`, `QmlUiAuditTests::rhiViewportRendererUsesPrepareSceneDataAndDirtyUploads()` |
| PREP-01 | Complete for active bed/plate QRhi rendering | `PrepareSceneData` bed fill/line vertices, `PreparePage.qml` bed property bindings, `RhiViewportRenderer` triangle fill and line grid/origin pipelines, explicit RHI d3d12 smoke |
| PREP-05 | Complete for active plate context isolation before full mesh rendering | `EditorViewModel::activePlateObjectIndices()`, `PrepareSceneData::setPlateContext()`, `ViewModelSmokeTests::activePlateObjectIndicesFollowCurrentPlateWithoutFallback()`, `PrepareSceneDataTests::plateContextDirtyFlagsOnlyChangeOnPlateDifferences()` |

## Residual Scope

- Full Prepare model mesh rendering, selection, hover, and camera behavior: Phase 25.
- Preview G-code segment buffers and layer-range draw control: Phase 26.
- QRhi fallback hardening and final milestone review gates: Phase 28.
