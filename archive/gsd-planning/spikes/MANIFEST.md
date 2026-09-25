# Spike Manifest

## Spikes

| # | Name | Type | Validates | Verdict | Tags |
|---|---|---|---|---|---|
| 001 | rendering-performance-architecture | research | Given v3.1 needs high-performance Prepare and Preview rendering, when comparing Qt OpenGL/FBO, Qt RHI/Vulkan, QSGRenderNode/native Vulkan, and upstream libvgcode, then we can choose the highest-performance feasible route. | PARTIAL | rendering, vulkan, qrhi, preview, prepare, performance |
| 002 | render-bench-qrhi-backend | standard | Given the current Windows/Qt 6.10 environment, when benchmarking a QRhi renderer with high-volume Preview-like line segments, then we can identify the fastest backend that actually initializes and renders in automation. | VALIDATED | rendering, qrhi, d3d12, d3d11, benchmark, preview |

## Decision Notes

- Performance is the primary selection criterion for v3.1 rendering architecture.
- Functional behavior must still align with upstream OrcaSlicer visible workflows.
- Rendering internals may diverge from upstream when Qt-native or lower-level GPU paths produce better performance.
- Current software and OpenGL viewports remain compatibility references, not the performance target.
- In the current Windows environment, the measured performance backend is QRhi/D3D12 with QRhi/D3D11 fallback. The installed Qt 6.10 SDK has QtGui Vulkan disabled, so Vulkan is not a valid current backend until Qt is rebuilt or replaced with public Vulkan support.
