---
spike: 001
name: rendering-performance-architecture
type: research
validates: "Given v3.1 needs high-performance Prepare and Preview rendering, when comparing Qt OpenGL/FBO, Qt RHI/Vulkan, QSGRenderNode/native Vulkan, and upstream libvgcode, then we can choose the highest-performance feasible route."
verdict: PARTIAL
related: []
tags: [rendering, vulkan, qrhi, preview, prepare, performance]
---

# Spike 001: Rendering Performance Architecture

## Context

The next milestone is expected to cover complete Prepare page model/plate rendering and post-slice Preview rendering. The product behavior still follows upstream OrcaSlicer, but the rendering implementation should choose the fastest feasible Qt-native/GPU route. Complexity is not a deciding constraint.

This spike is research-only. It does not implement the renderer or modify source code.

## Current Qt6 Baseline

- `src/qml_gui/main_qml.cpp` currently forces `QT_QUICK_BACKEND=software` unless `OWZX_OPENGL` is set.
- `GLViewport` and `GCodeRenderer` are implemented with `QQuickFramebufferObject`, so they are tied to OpenGL.
- `SoftwareViewport` is the default runtime viewport for stability.
- `tests/QmlUiAuditTests.cpp` asserts that the OpenGL viewport remains behind `OWZX_OPENGL` and that the canonical verify script does not enable it.
- `CMakeLists.txt` links `Qt6::OpenGL` but does not include Vulkan, ShaderTools, or a QRhi-based renderer target.

This means the existing renderer is a compatibility/functional baseline, not a viable maximum-performance target.

## Official Qt Constraints

- Qt documents `QQuickFramebufferObject` as an OpenGL/FBO integration class and warns that it is not compatible with Vulkan or Metal. It is described as a legacy class kept for Qt 5 source compatibility.
- Qt documents `QQuickRhiItem` as the portable replacement for `QQuickFramebufferObject`. It integrates custom rendering through QRhi and can run with Vulkan, Metal, Direct3D 11/12, and OpenGL/OpenGL ES.
- `QQuickRhiItem` is not functional with Qt Quick's software scene graph backend, so the performance path must stop forcing `QT_QUICK_BACKEND=software`.
- `QQuickWindow`/Qt Quick Scene Graph can render with accelerated APIs such as OpenGL or Vulkan and supports explicit graphics API selection via `QQuickWindow::setGraphicsApi()`.
- Khronos describes Vulkan as a modern explicit graphics and compute API with cross-vendor access to modern GPUs; it gives the application more responsibility in exchange for more control and potential performance gains.

References:

- Qt: https://doc.qt.io/qt-6/qquickframebufferobject.html
- Qt: https://doc.qt.io/qt-6/qquickrhiitem.html
- Qt: https://doc.qt.io/qt-6/qquickwindow.html
- Khronos: https://docs.vulkan.org/guide/latest/what_is_vulkan.html

## Candidate Routes

| Route | Performance Ceiling | Feasibility | Verdict |
|---|---:|---:|---|
| Keep current `QQuickFramebufferObject` and optimize OpenGL | Medium | High | Reject as target. It is OpenGL-only, legacy in Qt 6, and cannot use Vulkan/modern scene graph backends. |
| `QQuickRhiItem` + QRhi + Vulkan backend | High | High | Recommended. It stays inside Qt Quick, allows GPU-resident resources, and gives a portable path across Vulkan/D3D/Metal/OpenGL if needed. |
| `QSGRenderNode` or under-QML native Vulkan commands | Very high | Medium | Keep as escalation. Use if `QQuickRhiItem` offscreen composition or QRhi abstraction becomes a measured bottleneck. |
| Separate `QVulkanWindow`/native Vulkan viewport embedded beside QML | Very high raw renderer control | Medium-low | Not preferred. Window embedding, input, z-ordering, overlays, and QML composition become costly integration points. |
| Port upstream `libvgcode` renderer directly | Medium-high for Preview only | Medium | Not preferred as primary route. It aligns internals but does not solve Prepare rendering and may keep old GL assumptions. Reuse algorithms/data ideas, not the rendering stack. |

## Recommended Architecture

Use a new high-performance viewport path:

1. Add a performance renderer item implemented as `QQuickRhiItem` with a `QQuickRhiItemRenderer`.
2. Select `QSGRendererInterface::Vulkan` for the performance mode on Windows.
3. Keep `SoftwareViewport` as a compatibility fallback and keep the current OpenGL path only as a transitional fallback.
4. Add Qt Shader Tools and compile renderer shaders to `.qsb`.
5. Move render payload generation into C++ model/cache classes, not QML:
   - Prepare: immutable mesh vertex/index buffers, per-object transform/material buffers, plate/bed geometry buffers, wipe tower buffers.
   - Preview: compact G-code segment buffers with per-layer and per-move ranges.
6. Upload large geometry once and update dirty ranges only.
7. Render Prepare models with batched or instanced draws.
8. Render Preview by drawing indexed layer/move ranges instead of filtering all segments on the CPU each frame.
9. Do color modes, travel visibility, toolhead marker, and layer clipping through uniforms/storage buffers/shader logic where feasible.
10. Keep QML responsible for controls, legends, sliders, and overlays only.

## Performance Priorities

1. Avoid per-frame CPU parsing, filtering, and full-buffer uploads.
2. Keep mesh and G-code buffers GPU-resident after initial upload.
3. Batch objects by material/pipeline and use instancing for transforms.
4. Store layer and move offsets so Preview scrubbing changes draw ranges rather than rebuilding data.
5. Use GPU picking or a CPU BVH rather than per-object brute-force tests when model count grows.
6. Measure frame time, upload time, memory use, and interaction latency separately.

## Implementation Risks

- QRhi classes have limited compatibility guarantees and require linking Qt GUI private APIs for direct use. This is acceptable for the performance path because the project already targets a pinned Qt 6.10 toolchain.
- The performance path requires an accelerated scene graph backend. It must not run under the current forced software backend.
- Vulkan availability depends on Windows GPU drivers and runtime support. The app still needs a software fallback for machines or remote sessions without a working accelerated backend.
- Existing UI audit tests currently require the OpenGL viewport to stay behind `OWZX_OPENGL`; implementation must replace those expectations with a three-way policy: Vulkan performance path, software fallback, and transitional OpenGL fallback.
- Shader build support needs to be added to CMake, likely through Qt Shader Tools and checked-in or generated `.qsb` assets.

## Follow-up Benchmark Gate

Before implementing the full v3.1 renderer, build a micro-prototype that compares:

- Current `GLViewport/GCodeRenderer` OpenGL path.
- New `QQuickRhiItem` Vulkan path.
- Optional `QSGRenderNode` native Vulkan path if `QQuickRhiItem` shows composition overhead.

The benchmark should use synthetic and real workloads:

- 1 plate with small model count.
- 16 plates with multiple model instances.
- 1 million Preview segments.
- 5 million Preview segments if memory allows.
- Fast layer-range scrubbing.
- View rotation/pan/zoom under continuous input.

Minimum success criteria:

- Vulkan path has lower median and p95 frame time than current OpenGL path on the same machine.
- Preview layer scrubbing avoids full CPU reparse/reupload.
- No blank-window regression in the canonical software fallback.
- Visual behavior remains source-truth compatible with upstream OrcaSlicer workflows.

## Result

Verdict is `PARTIAL` because research strongly selects `QQuickRhiItem + QRhi + Vulkan` as the best feasible Qt-native route, but no empirical benchmark has been run yet. The next implementation step should be a small renderer prototype and benchmark, not a full feature migration.

Update 2026-06-27: Spike 002 supersedes the default backend choice for the current Windows Qt 6.10 environment. Keep QRhi as the renderer architecture, but use QRhi/D3D12 first and QRhi/D3D11 fallback on Windows. The installed Qt SDK has QtGui Vulkan disabled, so Vulkan requires a different Qt build before it can be benchmarked as a real candidate.
