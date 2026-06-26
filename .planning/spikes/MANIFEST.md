# Spike Manifest

## Spikes

| # | Name | Type | Validates | Verdict | Tags |
|---|---|---|---|---|---|
| 001 | rendering-performance-architecture | research | Given v3.1 needs high-performance Prepare and Preview rendering, when comparing Qt OpenGL/FBO, Qt RHI/Vulkan, QSGRenderNode/native Vulkan, and upstream libvgcode, then we can choose the highest-performance feasible route. | PARTIAL | rendering, vulkan, qrhi, preview, prepare, performance |

## Decision Notes

- Performance is the primary selection criterion for v3.1 rendering architecture.
- Functional behavior must still align with upstream OrcaSlicer visible workflows.
- Rendering internals may diverge from upstream when Qt-native or lower-level GPU paths produce better performance.
- Current software and OpenGL viewports remain compatibility references, not the performance target.
