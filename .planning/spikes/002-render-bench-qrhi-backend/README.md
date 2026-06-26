---
spike: 002
name: render-bench-qrhi-backend
type: standard
validates: "Given the current Windows/Qt 6.10 environment, when benchmarking a QRhi renderer with high-volume Preview-like line segments, then we can identify the fastest backend that actually initializes and renders in automation."
verdict: VALIDATED
related: [001-rendering-performance-architecture]
tags: [rendering, qrhi, d3d12, d3d11, benchmark, preview]
---

# Spike 002: QRhi Backend Render Benchmark

## What This Validates

This spike validates the practical backend choice for the v3.1 high-performance renderer. It uses synthetic Preview-like line segments and renders them through Qt QRhi without entering the main OWzx UI.

## Research

Spike 001 selected Qt RHI as the right architecture. This spike checked the environment-specific detail that matters on Windows: which backend actually initializes and performs under automation.

Initial `QQuickRhiItem + Vulkan` window testing failed with:

```text
Failed to initialize graphics backend for Vulkan.
```

Root cause: the prototype forced Qt Quick's Vulkan scene graph through `QQuickWindow::setGraphicsApi(Vulkan)`. In the current session, Qt Quick cannot initialize that Vulkan backend. The benchmark was changed to use offscreen QRhi with backend fallback, so it does not depend on a visible window or scene graph exposure.

Follow-up check: the installed `E:/Qt6.10` package reports `vulkan` in `QT_DISABLED_PUBLIC_FEATURES` for QtGui and does not provide the public `QVulkanInstance` header. That means this environment cannot compile or initialize a true QRhi Vulkan backend from the current Qt SDK. The benchmark now reports that explicitly instead of treating it as a generic failure.

## How To Run

Use the canonical script with the benchmark flag:

```powershell
$env:OWZX_RENDER_BENCH='1'
$env:OWZX_RENDER_BENCH_SEGMENTS='1000000'
$env:OWZX_RENDER_BENCH_FRAMES='120'
$env:OWZX_RENDER_BENCH_BACKEND='auto'
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

For quick local iteration after configure, using the existing `build/` directory only:

```powershell
build\owzx-render-bench.exe --segments 1000000 --frames 120 --backend auto
build\owzx-render-bench.exe --segments 1000000 --frames 120 --backend all
```

## Implementation

- Added `owzx-render-bench`, guarded by `OWZX_RENDER_BENCH=OFF` by default.
- Added QRhi shaders through Qt Shader Tools (`.qsb` resources).
- Uses QRhi offscreen frames to avoid window exposure and remote-session failures.
- Auto backend order is D3D12, then D3D11.
- `--backend all` compares D3D12, Vulkan, and D3D11 when the Qt build supports those backends.
- Vulkan is intentionally not the default path because the current Qt build/session failed to initialize Vulkan scene graph.

## Results

Measured in the current environment:

Comparison run:

```powershell
build\owzx-render-bench.exe --segments 1000000 --frames 120 --backend all
```

```json
{"failures":{"vulkan":"QtGui was built without public Vulkan support"},"frames":120,"requestedBackend":"all","results":[{"backend":"d3d12","firstFrameMs":1.3214,"frames":120,"initialized":true,"medianFrameMs":0.364,"p95FrameMs":0.4134,"requestedBackend":"all","segments":1000000,"totalMs":46.6661,"uploadMs":27.9138,"vertices":2000000},{"backend":"d3d11","firstFrameMs":5.164,"frames":120,"initialized":true,"medianFrameMs":0.3422,"p95FrameMs":0.3947,"requestedBackend":"all","segments":1000000,"totalMs":45.73,"uploadMs":26.0457,"vertices":2000000}],"segments":1000000}
```

Earlier D3D12-only runs:

```json
{"backend":"d3d12","firstFrameMs":1.0581,"frames":120,"initialized":true,"medianFrameMs":0.3318,"p95FrameMs":0.393,"requestedBackend":"auto","segments":1000000,"totalMs":39.7047,"uploadMs":26.1339,"vertices":2000000}
```

```json
{"backend":"d3d12","firstFrameMs":2.3129,"frames":120,"initialized":true,"medianFrameMs":0.8894,"p95FrameMs":1.319,"requestedBackend":"auto","segments":5000000,"totalMs":118.1176,"uploadMs":115.4426,"vertices":10000000}
```

Verdict: `VALIDATED`.

The best known feasible route for this Windows Qt 6.10 build is **QRhi/D3D12 first, QRhi/D3D11 fallback**. Vulkan remains a desirable explicit API option, but this installed Qt SDK cannot provide a valid QRhi Vulkan backend. Vulkan should only be considered after switching to a Qt build with public Vulkan support and rerunning the same benchmark.

## Build Signal

For v3.1 renderer planning:

- Use QRhi as the renderer abstraction.
- Prefer D3D12 on Windows for the performance path.
- Keep D3D11 fallback.
- Do not force Vulkan as the default on Windows with the current Qt SDK.
- Re-evaluate Vulkan only with a Qt build where QtGui has Vulkan enabled.
- Do not use `QQuickFramebufferObject` for the performance renderer.
- Keep QML overlays separate from the GPU geometry renderer.
