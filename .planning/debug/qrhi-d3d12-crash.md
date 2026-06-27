---
status: fixing
trigger: "OWZX_RHI_RENDERER auto launch becomes unresponsive and crashes"
created: 2026-06-27
updated: 2026-06-27
---

# QRhi D3D12 Crash Debug Session

## Symptoms

- Expected behavior: launching the app with the QRhi renderer should open the Prepare page and keep running so the user can inspect the renderer.
- Actual behavior: `OWZX_RHI_RENDERER=1` selected D3D12, the app became unresponsive and exited.
- Error messages: crash dump recorded `0xc0000005` access violation.
- Timeline: started after Phase 25 QRhi rendering was launched manually for visual inspection.
- Reproduction: launch `build/OWzxSlicer.exe` from `build/` with `OWZX_RHI_RENDERER=1` or `OWZX_RHI_RENDERER=d3d12`.

## Current Focus

- hypothesis: D3D12 is not currently a feasible default backend on this Windows/Qt 6.10 runtime; D3D11 is the highest-performing verified QRhi backend that remains stable.
- test: compare default software, explicit D3D11, and explicit D3D12 launches under the same executable and page.
- expecting: D3D11 stays alive; D3D12 reproduces the access violation.
- next_action: change app auto policy to prefer D3D11 and keep D3D12 available only through explicit opt-in.

## Evidence

- 2026-06-27T22:36:19: startup log showed `QRhi backend selection: enabled=true requested=auto selected=d3d12 attempts=[d3d12:ok]`.
- 2026-06-27T22:36:22: crash stack log recorded `Unhandled exception code=0xc0000005`.
- 2026-06-27T22:43:09: default software path stayed alive until the controlled timeout.
- 2026-06-27T22:43:22: explicit `OWZX_RHI_RENDERER=d3d11` stayed alive until the controlled timeout.
- 2026-06-27T22:43:57: explicit `OWZX_RHI_RENDERER=d3d12` exited with `-1073741819` and wrote a new minidump.

## Resolution

- root_cause: pending source fix verification.
- fix: pending.
- verification: pending.
- files_changed: pending.
