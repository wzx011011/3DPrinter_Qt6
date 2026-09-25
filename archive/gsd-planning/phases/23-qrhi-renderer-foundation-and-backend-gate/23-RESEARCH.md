# Phase 23: QRhi Renderer Foundation And Backend Gate - Research

**Researched:** 2026-06-27
**Domain:** Qt Quick QRhi viewport integration and gated renderer startup
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Use `OWZX_RHI_RENDERER=1` as the explicit runtime gate. Keep QRhi code compiled with the app where practical, but disabled by default.
- Keep the default viewport path stable: normal startup continues to register/use `SoftwareViewport`; QRhi only replaces or hosts the viewport when explicitly enabled.
- On QRhi initialization failure, log diagnostics and fall back to the stable viewport path without a blocking dialog, process exit, crash, or blank viewport.
- Preserve the existing `OWZX_OPENGL` legacy path as a separate gate. QRhi is a new path and must not reuse or extend `QQuickFramebufferObject` as the long-term architecture.
- On Windows, attempt QRhi D3D12 first and D3D11 fallback second. Vulkan is not a v3.1 default candidate with the installed Qt SDK because QtGui public Vulkan support is disabled.
- Expose backend failures through structured diagnostics containing attempted backend, failure reason, and selected backend.
- Phase 23 benchmark scope remains synthetic segment rendering: shader `.qsb` resource use, D3D12/D3D11 initialization, and explicit Vulkan-disabled reporting.
- Phase 23 establishes repeatable timing output and benchmark invocation, not final FPS/throughput acceptance.
- Register a new QML item/type for the QRhi viewport while keeping the property/API surface as compatible as possible with the current `GLViewport` bindings used by Prepare and Preview.
- Keep rendering decisions, backend selection, data conversion, fallback state, and diagnostics in C++ renderer/service/viewmodel layers. QML only hosts the item and binds existing viewmodel properties.
- Phase 23 does not need complete real bed/model/G-code rendering.
- Prioritize tests and audits that prove default fallback remains available, QRhi is explicitly gated and off by default, and benchmark enablement does not alter normal app startup.

### the agent's Discretion
Use existing local naming and QML registration patterns where they reduce churn. Keep implementation minimal enough for Phase 23 while leaving clear extension points for plate/model scene data in Phase 24 and Preview buffers in Phase 26.

### Deferred Ideas (OUT OF SCOPE)
- Real Prepare plate/bed rendering is Phase 24.
- Real Prepare model mesh rendering, selection, hover, and camera interaction completion is Phase 25.
- Real Preview G-code segment pipeline, color modes, toggles, layer range, and legend/statistics synchronization are phases 26-27.
- Vulkan backend promotion is future work after replacing or rebuilding Qt with public QtGui Vulkan support enabled and re-running benchmark evidence.
</user_constraints>

## Summary

Phase 23 should use Qt 6.10's public `QQuickRhiItem` / `QQuickRhiItemRenderer` API for the app-hosted QRhi viewport rather than extending the legacy `QQuickFramebufferObject` path. The installed SDK exposes `QQuickRhiItem` in `E:/Qt6.10/include/QtQuick/qquickrhiitem.h`, with `createRenderer()`, `initialize(QRhiCommandBuffer*)`, `synchronize(QQuickRhiItem*)`, and `render(QRhiCommandBuffer*)` hooks. [VERIFIED: local Qt 6.10 SDK headers]

The current app startup is deliberately conservative: `src/qml_gui/main_qml.cpp` sets `QT_QUICK_BACKEND=software` and registers `SoftwareViewport` as `OWzxGL.GLViewport` unless `OWZX_OPENGL` is set. Phase 23 must preserve this path and introduce `OWZX_RHI_RENDERER` as a separate gate that selects a QRhi scenegraph API before `QGuiApplication` construction and registers a compatible QRhi item only when the gate can be satisfied. [VERIFIED: codebase]

The existing offscreen benchmark already proves the QRhi backend reality on this machine: D3D12 and D3D11 initialize and render; Vulkan reports "QtGui was built without public Vulkan support." App-side backend policy should mirror that evidence by trying Direct3D12 first, Direct3D11 second, and treating Vulkan as a future SDK prerequisite rather than a runtime default. [VERIFIED: codebase + local benchmark output]

**Primary recommendation:** Implement a minimal `RhiViewport` based on `QQuickRhiItem`, register it behind `OWZX_RHI_RENDERER=1`, force Qt Quick to `Direct3D12` with fallback to `Direct3D11` using a preflight helper, and guard the startup contract with `QmlUiAuditTests`.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|--------------|----------------|-----------|
| Runtime renderer gate | C++ app entry (`main_qml.cpp`) | QML type registration | Backend choice must happen before scenegraph initialization; QML should only receive a registered item. |
| QRhi viewport host | C++ renderer item (`src/qml_gui/Renderer`) | QML page binding | `QQuickRhiItem` owns QRhi render lifecycle; existing pages bind properties. |
| Backend preflight and diagnostics | C++ helper/service-style utility | Benchmark tool | Backend attempts, failure reasons, and selected API must be structured and reusable. |
| Shader pipeline | CMake + Qt Shader Tools | Renderer resource loading | `.qsb` resources should be compiled through canonical `build/` and loaded by renderer/benchmark. |
| Fallback UX | C++ startup registration + existing `SoftwareViewport` | Backend notification/logging | QRhi failure must choose stable viewport without modal failure or blank app. |

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| RHI-01 | Stable viewport remains default while QRhi is gated. | Existing `main_qml.cpp` default software registration can be preserved and extended with a new gate. |
| RHI-02 | Windows QRhi uses D3D12 first with D3D11 fallback. | `QSGRendererInterface` supports `Direct3D12` and `Direct3D11`; benchmark helper already models this order. |
| RHI-03 | Shaders compile to `.qsb` through canonical build tree. | `OWZX_RENDER_BENCH` already uses `qt_add_shaders`; app renderer can add a separate shader group. |
| RHI-05 | QML hosts QRhi viewport without business/rendering logic in QML. | `QQuickRhiItem` can expose a QML item with existing `GLViewport`-compatible properties. |
| RHI-06 | Optional benchmark reports backend metrics. | `tools/render_bench/main.cpp` already emits JSON for D3D12/D3D11 and Vulkan-disabled status. |
| PERF-05 | Backend choice and timing evidence are captured. | Structured diagnostics can be shared between app gate logs and benchmark JSON. |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt Quick `QQuickRhiItem` | Qt 6.10.2 local SDK | Embed QRhi-rendered content as a QML item | Public Qt Quick API for custom QRhi rendering inside the scenegraph. |
| Qt Gui QRhi private headers (`rhi/qrhi.h`) | Qt 6.10.2 local SDK | QRhi pipeline, buffers, command buffer, shader resources | Existing benchmark already uses these APIs; required for renderer internals. |
| Qt Shader Tools | Qt 6.10.2 local SDK | Compile `.vert` / `.frag` into `.qsb` resources | Already wired for `owzx-render-bench`; matches Qt RHI shader path. |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Qt Test | Qt 6.10.2 local SDK | Static UI/startup contract audit | Extend `QmlUiAuditTests` to guard QRhi gate and fallback. |
| Existing `SoftwareViewport` | project implementation | Stable fallback | Default runtime and QRhi failure fallback. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| `QQuickRhiItem` | `QSGRenderNode` | Lower-level and more error-prone; unnecessary because Qt 6.10 exposes `QQuickRhiItem`. |
| `QQuickRhiItem` | `QQuickFramebufferObject` | OpenGL-only and conflicts with the QRhi performance direction. |
| App-side D3D12/D3D11 | Vulkan | Current QtGui has public Vulkan disabled, so Vulkan cannot be a v3.1 default backend. |

**Installation:** No external packages are installed in this phase.

## Architecture Patterns

### System Architecture Diagram

```text
Process env
  |
  |-- no OWZX_RHI_RENDERER --> QT_QUICK_BACKEND=software --> register SoftwareViewport as OWzxGL.GLViewport
  |
  |-- OWZX_OPENGL ---------> QSG OpenGL API -------------> register legacy GLViewport
  |
  `-- OWZX_RHI_RENDERER --> preflight D3D12 -> D3D11
                              |
                              |-- success --> QQuickWindow::setGraphicsApi(selected) + register RhiViewport
                              `-- failure --> log diagnostics + register SoftwareViewport
```

### Recommended Project Structure

```text
src/qml_gui/Renderer/
  RhiBackendSelector.{h,cpp}   # backend candidate order, selected API, diagnostics
  RhiViewport.{h,cpp}          # QQuickRhiItem-compatible GLViewport property surface
  RhiViewportRenderer.{h,cpp}  # QQuickRhiItemRenderer minimal pipeline / clear / diagnostics
  shaders/rhi_viewport.vert    # app renderer shader compiled to qsb
  shaders/rhi_viewport.frag
tests/
  QmlUiAuditTests.cpp          # static gate/fallback audit
```

### Pattern 1: Compatible QML Type Swap
**What:** Keep QML importing `OWzxGL 1.0` and instantiating `GLViewport`, but register one of `SoftwareViewport`, legacy `GLViewport`, or new `RhiViewport` under that name based on environment and preflight. [VERIFIED: current `main_qml.cpp` pattern]
**When to use:** Phase 23, because the UI-SPEC says pages should not shift or branch heavily.

### Pattern 2: Renderer Internals In C++
**What:** Expose QML properties only; copy/synchronize state into the renderer in `synchronize()`, allocate QRhi resources in `initialize()`, draw in `render()`. [VERIFIED: `QQuickRhiItemRenderer` public hooks]
**When to use:** All QRhi rendering milestones.

### Anti-Patterns to Avoid
- **Extending `QQuickFramebufferObject`:** keeps the code tied to OpenGL and does not satisfy the QRhi architecture decision.
- **QML backend selection logic:** violates project boundary rules and makes fallback hard to audit.
- **Vulkan-first startup:** repeats the earlier user-visible Vulkan initialization failure with the current SDK.
- **Changing default startup:** violates RHI-01 and existing QmlUiAudit coverage.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| QRhi item lifecycle | Custom scenegraph node from scratch | `QQuickRhiItem` | Qt already owns texture, render target, and render scheduling integration. |
| Shader compilation | Runtime string shader compilation | `qt_add_shaders` / `.qsb` resources | Required by QRhi portable shader path and already proven by benchmark. |
| Backend availability proof | UI-only flag | Preflight helper + benchmark JSON | Verification needs structured evidence, not visual inference. |

## Common Pitfalls

### Pitfall 1: Setting Scenegraph API Too Late
**What goes wrong:** Calling `QQuickWindow::setGraphicsApi()` after Qt Quick scenegraph initialization has no effect or behaves inconsistently.
**How to avoid:** Resolve `OWZX_RHI_RENDERER` / `OWZX_OPENGL` before `QGuiApplication` and before loading QML.

### Pitfall 2: Software Backend With QRhi Item
**What goes wrong:** The current default `QT_QUICK_BACKEND=software` cannot host QRhi rendering.
**How to avoid:** Only set `QT_QUICK_BACKEND=software` when neither `OWZX_OPENGL` nor successful `OWZX_RHI_RENDERER` is active.

### Pitfall 3: Treating Benchmark As App Startup
**What goes wrong:** Offscreen benchmark success does not prove QML item integration or fallback.
**How to avoid:** Add app-side static audit and a lightweight runtime smoke path in addition to `owzx-render-bench`.

### Pitfall 4: Scope Creep Into Real Mesh/G-code Rendering
**What goes wrong:** Phase 23 grows into Phase 24-27 data pipeline work.
**How to avoid:** Minimal QRhi renderer may draw a diagnostic clear/triangle only; real plate/model/G-code buffers are planned later.

## Code Examples

### Public QQuickRhiItem Hooks

```cpp
// Source: E:/Qt6.10/include/QtQuick/qquickrhiitem.h
class QQuickRhiItemRenderer {
protected:
    virtual void initialize(QRhiCommandBuffer *cb) = 0;
    virtual void synchronize(QQuickRhiItem *item) = 0;
    virtual void render(QRhiCommandBuffer *cb) = 0;
};
```

### Existing Startup Gate Pattern

```cpp
// Source: src/qml_gui/main_qml.cpp
const bool useOpenGLViewport = qEnvironmentVariableIsSet("OWZX_OPENGL");
if (!useOpenGLViewport) {
  qputenv("QT_QUICK_BACKEND", "software");
} else {
  QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
}
```

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| Qt Quick `QQuickRhiItem` | RhiViewport | yes | Qt 6.10.2 local SDK | none needed |
| Qt Shader Tools | `.qsb` app shaders | yes | Qt 6.10.2 local SDK | none needed |
| QRhi D3D12 | Preferred backend | yes by benchmark | Qt QRhi D3D12 | D3D11 |
| QRhi D3D11 | Fallback backend | yes by benchmark | Qt QRhi D3D11 | SoftwareViewport |
| QRhi Vulkan | Future backend | no public QtGui support | disabled in QtGui | D3D12/D3D11 |

**Missing dependencies with no fallback:** None for Phase 23.

**Missing dependencies with fallback:** Vulkan is unavailable; D3D12/D3D11 cover v3.1.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Qt Test via `QmlUiAuditTests`; optional benchmark executable |
| Config file | root `CMakeLists.txt` |
| Quick run command | `build/QmlUiAuditTests.exe` |
| Full suite command | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` |

### Phase Requirements To Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| RHI-01 | Default startup remains SoftwareViewport and no canonical script QRhi env override | static Qt Test | `build/QmlUiAuditTests.exe` | yes |
| RHI-02 | D3D12/D3D11 backend order visible in app code and benchmark | static Qt Test + benchmark | `build/QmlUiAuditTests.exe`; `build/owzx-render-bench.exe --backend all --segments 1000 --frames 5` | partial |
| RHI-03 | App/benchmark shaders compile via `qt_add_shaders` | configure/build | canonical full command | partial |
| RHI-05 | QML host item is C++ registered and compatible; QML does no backend logic | static Qt Test | `build/QmlUiAuditTests.exe` | partial |
| RHI-06 | Optional benchmark reports JSON metrics | benchmark | `build/owzx-render-bench.exe --backend all --segments 1000 --frames 5` | yes |
| PERF-05 | Structured backend diagnostics are emitted | unit/static + benchmark | `build/QmlUiAuditTests.exe`; benchmark command | partial |

### Sampling Rate
- **Per task commit:** `build/QmlUiAuditTests.exe` when build artifacts exist; otherwise the relevant `rg` static checks from the task.
- **Per wave merge:** canonical full command.
- **Phase gate:** canonical full command plus optional benchmark with `OWZX_RENDER_BENCH=1`.

### Wave 0 Gaps
- [ ] Extend `tests/QmlUiAuditTests.cpp` with QRhi gate/fallback assertions.
- [ ] Add app-side QRhi source files and shader resources so CMake can compile them.

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | N/A |
| V3 Session Management | no | N/A |
| V4 Access Control | no | N/A |
| V5 Input Validation | yes | Treat environment variable values as untrusted strings; accept only known gate/backend values. |
| V6 Cryptography | no | N/A |

### Known Threat Patterns for Renderer Gate

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Invalid env var selects unsupported renderer path | Denial of Service | Strict allowlist and fallback to stable viewport. |
| Diagnostic log hides backend failure | Repudiation | Structured diagnostics include attempted backend and failure reason. |

## Open Questions (RESOLVED)

1. **Should Phase 23 use `QQuickRhiItem` or lower-level scenegraph APIs?** RESOLVED: Use `QQuickRhiItem`, verified in local Qt 6.10.2 public headers.
2. **Should Vulkan be attempted by default?** RESOLVED: No; Vulkan remains future work because QtGui public Vulkan support is disabled in this SDK.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | App-side QRhi preflight can reliably choose D3D12 before QQuickWindow creates the scenegraph. | Architecture Patterns | If Qt Quick D3D12 scenegraph creation still fails despite preflight, fallback logic must catch runtime failure and revert to SoftwareViewport. |

## Sources

### Primary (HIGH confidence)
- `E:/Qt6.10/include/QtQuick/qquickrhiitem.h` - public `QQuickRhiItem` / renderer hooks.
- `E:/Qt6.10/include/QtQuick/qsgrendererinterface.h` - `Direct3D12`, `Direct3D11`, `Software`, `OpenGL` graphics API enum values.
- `src/qml_gui/main_qml.cpp` - current default SoftwareViewport / `OWZX_OPENGL` gate.
- `src/qml_gui/Renderer/GLViewport.h` and `SoftwareViewport.h` - compatible property surfaces.
- `tools/render_bench/main.cpp` - current QRhi D3D12/D3D11/Vulkan-disabled benchmark implementation.

### Secondary (MEDIUM confidence)
- Prior v2.9/v3.0 phase summaries via `gsd-sdk query history-digest`.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - confirmed from local Qt SDK and current build files.
- Architecture: HIGH - matches current app startup and Qt Quick public API.
- Pitfalls: MEDIUM - based on Qt scenegraph lifecycle plus local startup behavior; runtime validation remains required.

**Research date:** 2026-06-27
**Valid until:** 2026-07-27 or until Qt SDK changes.
