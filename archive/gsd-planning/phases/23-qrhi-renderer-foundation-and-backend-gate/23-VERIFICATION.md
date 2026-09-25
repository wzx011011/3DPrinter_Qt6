---
phase: 23-qrhi-renderer-foundation-and-backend-gate
status: passed
verified_at: 2026-06-27T10:57:12+08:00
requirements: [RHI-01, RHI-02, RHI-03, RHI-05, RHI-06, PERF-05]
---

# Phase 23 Verification: QRhi Renderer Foundation And Backend Gate

## Result

**Status:** passed

Phase 23 establishes a gated Qt-native QRhi foundation without changing the default viewport startup path. Real Prepare plate/model rendering and real Preview G-code rendering remain pending for Phases 24-26.

## Automated Checks

### Canonical Verification

Command:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Result: exit 0.

Evidence from `build/auto_verify_23_03_green_attempt1.log`:

- `QML UI audit tests passed`
- `APP_RUNNING_PID=28736`
- `All pipeline tests passed`

### Optional QRhi Benchmark Through Canonical Script

Command:

```powershell
$env:OWZX_RENDER_BENCH='1'
$env:OWZX_RENDER_BENCH_SEGMENTS='1000'
$env:OWZX_RENDER_BENCH_FRAMES='5'
$env:OWZX_RENDER_BENCH_BACKEND='all'
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Result: exit 0.

Evidence from `build/auto_verify_23_03_bench_all.log`:

- `RenderBench] Benchmark completed`
- `QML UI audit tests passed`
- `APP_RUNNING_PID=34076`
- `All pipeline tests passed`

Benchmark JSON summary:

```json
{"attemptedBackends":["d3d12","vulkan","d3d11"],"failures":{"vulkan":"QtGui was built without public Vulkan support"},"frames":5,"requestedBackend":"all","results":[{"attemptedBackends":["d3d12"],"backend":"d3d12","firstFrameMs":25.9842,"frames":5,"initialized":true,"medianFrameMs":0.131,"p95FrameMs":25.9842,"requestedBackend":"all","segments":1000,"selectedBackend":"d3d12","totalMs":26.7167,"uploadMs":1.8227,"vertices":2000},{"attemptedBackends":["d3d12","vulkan","d3d11"],"backend":"d3d11","firstFrameMs":2.6645,"frames":5,"initialized":true,"medianFrameMs":0.128,"p95FrameMs":2.6645,"requestedBackend":"all","segments":1000,"selectedBackend":"d3d11","totalMs":3.1558,"uploadMs":1.3143,"vertices":2000}],"segments":1000}
```

### Focused QML UI Audit

Command:

```powershell
build\QmlUiAuditTests.exe -o build\qml_audit_23_03_green.txt,txt
```

Result: exit 0, `10 passed, 0 failed`.

Covered checks include:

- default `SoftwareViewport` registration
- explicit `OWZX_RHI_RENDERER` QRhi viewport gate
- app QRhi selector D3D12-first and D3D11 fallback
- no app-side Vulkan default while QtGui public Vulkan support is disabled
- QRhi item host and `.qsb` shader resource wiring
- optional render benchmark contract and canonical script benchmark gate

### Direct Benchmark Confirmation

Command:

```powershell
build\owzx-render-bench.exe --segments 1000 --frames 5 --backend all
```

Result: exit 0.

Evidence from `build/render_bench_23_03_direct.json`:

- `requestedBackend=all`
- `attemptedBackends=["d3d12","vulkan","d3d11"]`
- `selectedBackend=d3d12` result present
- `selectedBackend=d3d11` result present
- `failures.vulkan="QtGui was built without public Vulkan support"`

## Requirement Traceability

| Requirement | Status | Evidence |
|---|---|---|
| RHI-01 | Passed | `main_qml.cpp` keeps default/failure `SoftwareViewport`; canonical script does not set `OWZX_RHI_RENDERER`; canonical app smoke passed. |
| RHI-02 | Passed | `RhiBackendSelector.cpp` tries D3D12 then D3D11 for app QRhi selection; `QmlUiAuditTests` guards both names. |
| RHI-03 | Passed | `qt_add_shaders(OWzxSlicer "rhi_viewport_shaders")` builds app `.qsb` resources; canonical build passed. |
| RHI-05 | Passed | `RhiViewport` is a C++ `QQuickRhiItem` registered under `OWzxGL.GLViewport`; QML pages keep binding properties only. |
| RHI-06 | Passed | Optional `owzx-render-bench` built and ran through canonical script and direct command, reporting structured backend JSON. |
| PERF-05 | Passed | Benchmark records upload/first-frame/median/p95/total timing fields for D3D12 and D3D11. |

## Manual Verification

No manual verification required for Phase 23. The QRhi app viewport remains opt-in and the default launch path is covered by automated startup smoke.

## Deferred Scope

- GPU-resident Prepare mesh/plate buffers are Phase 24/25.
- Real model mesh rendering and camera interaction are Phase 25.
- Real Preview G-code segment rendering and layer controls are Phase 26/27.
- Vulkan app support remains deferred until QtGui is rebuilt or replaced with public Vulkan support.

