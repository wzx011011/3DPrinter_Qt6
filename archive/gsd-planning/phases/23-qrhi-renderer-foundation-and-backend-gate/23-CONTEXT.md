# Phase 23: QRhi Renderer Foundation And Backend Gate - Context

**Gathered:** 2026-06-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Phase 23 delivers the gated QRhi renderer foundation only: runtime/backend gate, QML-hostable viewport type, shader build pipeline, structured backend diagnostics, benchmark evidence, and fallback safety. It must not change default stable viewport startup and must not attempt full real Prepare plate/model rendering or Preview G-code rendering; those belong to phases 24-27.

</domain>

<decisions>
## Implementation Decisions

### Renderer Gate And Startup Contract
- Use `OWZX_RHI_RENDERER=1` as the explicit runtime gate. Keep QRhi code compiled with the app where practical, but disabled by default.
- Keep the default viewport path stable: normal startup continues to register/use `SoftwareViewport`; QRhi only replaces or hosts the viewport when explicitly enabled.
- On QRhi initialization failure, log diagnostics and fall back to the stable viewport path without a blocking dialog, process exit, crash, or blank viewport.
- Preserve the existing `OWZX_OPENGL` legacy path as a separate gate. QRhi is a new path and must not reuse or extend `QQuickFramebufferObject` as the long-term architecture.

### Backend Selection And Benchmark Evidence
- On Windows, attempt QRhi D3D12 first and D3D11 fallback second. Vulkan is not a v3.1 default candidate with the installed Qt SDK because QtGui public Vulkan support is disabled.
- Expose backend failures through structured diagnostics containing attempted backend, failure reason, and selected backend. App startup/fallback and `owzx-render-bench` should both produce evidence that can be checked in logs or JSON.
- Phase 23 benchmark scope remains synthetic segment rendering: shader `.qsb` resource use, D3D12/D3D11 initialization, and explicit Vulkan-disabled reporting. Real model mesh and G-code scene data are deferred to later phases.
- Phase 23 establishes repeatable timing output and benchmark invocation, not final FPS/throughput acceptance. Large workload pass/fail thresholds belong to Phase 27.

### QML Hosting And Responsibility Boundary
- Register a new QML item/type for the QRhi viewport while keeping the property/API surface as compatible as possible with the current `GLViewport` bindings used by Prepare and Preview.
- Keep rendering decisions, backend selection, data conversion, fallback state, and diagnostics in C++ renderer/service/viewmodel layers. QML only hosts the item and binds existing viewmodel properties.
- Phase 23 does not need complete real bed/model/G-code rendering. It needs host item viability, shader pipeline, backend/fallback behavior, and benchmark-backed QRhi evidence.
- Prioritize tests and audits that prove default fallback remains available, QRhi is explicitly gated and off by default, and benchmark enablement does not alter normal app startup.

### the agent's Discretion
Use existing local naming and QML registration patterns where they reduce churn. Keep implementation minimal enough for Phase 23 while leaving clear extension points for plate/model scene data in Phase 24 and Preview buffers in Phase 26.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/qml_gui/main_qml.cpp` already gates viewport registration: default startup sets `QT_QUICK_BACKEND=software` and registers `SoftwareViewport` as `OWzxGL.GLViewport`; `OWZX_OPENGL` registers the legacy `GLViewport` and forces `QSGRendererInterface::OpenGL`.
- `src/qml_gui/Renderer/GLViewport.h` and `src/qml_gui/Renderer/SoftwareViewport.h` share the QML-facing viewport property surface: `canvasType`, `meshData`, `previewData`, layer/move controls, bed/wipe tower toggles, marker properties, `gizmoMode`, `wireframeMode`, and thumbnail/camera invokables.
- `tools/render_bench/main.cpp` already contains an offscreen QRhi synthetic segment benchmark with D3D12/D3D11 support and explicit Vulkan disabled reporting for the current Qt SDK.
- `CMakeLists.txt` already has `OWZX_RENDER_BENCH` and Qt Shader Tools wiring for benchmark shaders under `tools/render_bench/shaders`.

### Established Patterns
- Runtime feature gates are environment-variable based for renderer startup (`OWZX_OPENGL`) and should remain non-invasive by default.
- QML pages import `OWzxGL` and instantiate `GLViewport` directly in `PreparePage.qml` and `PreviewPage.qml`; maintaining the existing property shape minimizes QML churn.
- Source-truth behavior should remain in C++ services/viewmodels/renderers, with QML acting as presentation and property wiring.
- Build verification must use the canonical `scripts/auto_verify_with_vcvars.ps1` command and `build/` directory.

### Integration Points
- Add the QRhi item under `src/qml_gui/Renderer/` and register it from `main_qml.cpp` only when `OWZX_RHI_RENDERER=1` is set and backend initialization policy permits it.
- Keep `OWZX_OPENGL` behavior independent so existing OpenGL fallback/debug path remains available.
- Extend or reuse benchmark diagnostics for app-side backend selection evidence, but do not make the benchmark run during normal startup.
- Add focused tests/audits near existing QML UI audit coverage to guard default SoftwareViewport registration and QRhi gate behavior.

</code_context>

<specifics>
## Specific Ideas

- The user explicitly wants the highest-performance viable rendering path and is open to Vulkan in principle, but current local evidence makes D3D12/D3D11 the viable v3.1 route until QtGui Vulkan support changes.
- Rendering implementation may diverge technically from upstream wx/OpenGL internals as long as user-visible behavior remains aligned; Qt-native rendering is acceptable and preferred here.
- Phase 23 should keep the app stable first: no startup regression, no blank view, no Vulkan error dialog.

</specifics>

<deferred>
## Deferred Ideas

- Real Prepare plate/bed rendering is Phase 24.
- Real Prepare model mesh rendering, selection, hover, and camera interaction completion is Phase 25.
- Real Preview G-code segment pipeline, color modes, toggles, layer range, and legend/statistics synchronization are phases 26-27.
- Vulkan backend promotion is future work after replacing or rebuilding Qt with public QtGui Vulkan support enabled and re-running benchmark evidence.

</deferred>
