# Phase 24: Prepare Scene Data And Plate Rendering - Research

**Researched:** 2026-06-27
**Domain:** Qt QRhi Prepare scene cache, bed/plate rendering, and plate context synchronization
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Keep the technical route Qt-native: `QQuickRhiItem` + QRhi resources, using the Phase 23 D3D12-first/D3D11-fallback selector behind `OWZX_RHI_RENDERER=1`.
- Do not attempt Vulkan in Phase 24. The Phase 23 benchmark proved the installed QtGui lacks public Vulkan support, so Vulkan is not a viable app backend until the Qt SDK changes.
- Optimize for steady-frame cost: create/update QRhi buffers only when explicit scene dirty flags change, not every frame.
- Keep default startup stable. Normal launches still use the stable SoftwareViewport path unless the QRhi gate is explicitly enabled.
- Add a C++ scene/cache layer for Prepare data, colocated with the QRhi renderer boundary, that stores bed dimensions, grid/axis vertices, active plate metadata, and future mesh/cache hooks.
- Track dirty categories explicitly: bed geometry, plate context, mesh data, visibility flags, and GPU resource state. Phase 24 acceptance depends on proving only dirty ranges/resources are uploaded.
- Treat QML as binding-only. QML may pass existing viewmodel properties to the viewport, but scene conversion, dirty decisions, and GPU ownership stay in C++.
- Render active bed/plate context through QRhi: bed surface or outline, grid lines, border, origin cues/axes, and visual state tied to the selected plate.
- Start from existing Qt6/OpenGL functional expectations where useful: 220x220 fallback bed, 10 mm fine grid, 50 mm coarse grid, origin/axis cues, and a ground plane slightly below model height.
- Use `EditorViewModel::currentPlateIndex`, `plateCount`, and `ProjectServiceMock`/`PartPlateList` as the C++ source of truth for plate state.
- Active-plate switching must mark scene data dirty and update rendered plate context without leaking inactive-plate object data into the active scene.
- Technical implementation intentionally diverges from upstream wx/OpenGL/ImGui internals. The parity requirement is functional: the active bed/plate context and plate switching behavior should match the user-visible workflow.
- Use TDD for implementation: create focused failing tests/audits before source changes.
- Verification must use the canonical command only: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.

### the agent's Discretion
Keep the first implementation vertically thin but architecturally real: a reusable scene/cache class, QRhi buffer upload ownership, and a visible QRhi bed/plate result are more valuable than broad but temporary rendering code. Defer polish that does not affect performance, correctness, or Phase 25 extensibility.

### Deferred Ideas (OUT OF SCOPE)
- Full Prepare model mesh rendering, active object selection, hover, picking, gizmos, and camera behavior are Phase 25.
- Preview G-code segment buffers, color modes, layer range, and legend/stat sync are Phase 26-27.
- Vulkan promotion remains deferred until QtGui public Vulkan support is available and benchmark evidence beats D3D12/D3D11.
- Bed textures/logos, calibration graphics, exclusion zones, and full upstream plate icon rendering are deferred unless later requirements make them user-visible blockers.
</user_constraints>

## Summary

Phase 24 should create a pure C++ Prepare scene data object before touching QRhi buffers. The object should own bed metrics, active plate context, visible object indices, dirty flags, and generated bed/grid/axis geometry. That gives the renderer a deterministic input contract and makes the performance requirement testable without starting the app. [VERIFIED: context + codebase]

The current QRhi viewport is a valid host but still diagnostic-only: `RhiViewportRenderer` uploads one static triangle and draws it every frame. Phase 24 should replace that diagnostic primitive with bed fill, border, grid, and origin/axis geometry derived from the scene data object. QRhi resources should be allocated once and re-uploaded only when the scene dirty flags report a bed, plate, mesh, or visibility change. [VERIFIED: `src/qml_gui/Renderer/RhiViewportRenderer.cpp`]

Plate state already has a C++ truth source. `PartPlateList` owns current-plate state and object membership; `ProjectServiceMock` exposes `currentPlateObjectIndices()` and `plateObjectIndices()`, and `EditorViewModel::setCurrentPlateIndex()` already refreshes mesh/cache state. The missing contract is a typed renderer-facing active-plate context; relying on QML or packed mesh bytes would be fragile because current `meshData()` packs object batches but not a clear plate-membership contract for QRhi filtering. [VERIFIED: `src/core/model/PartPlateList.*`, `ProjectServiceMock.*`, `EditorViewModel.*`]

**Primary recommendation:** Add `PrepareSceneData` as a pure, testable C++ scene/dirty-tracking class, expose bed/plate context properties on `EditorViewModel` and `RhiViewport`, bind them in `PreparePage.qml`, and then make `RhiViewportRenderer` render bed/grid/axis buffers from `PrepareSceneData` with dirty-gated QRhi uploads.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|--------------|----------------|-----------|
| Bed/plate source truth | C++ viewmodel/service/domain | QML binding | `EditorViewModel` and `PartPlateList` already own plate and bed state; QML should not compute it. |
| Scene dirty tracking | Pure C++ scene data helper | QRhi renderer | Dirty flags must be unit-testable and independent from GPU object lifetime. |
| QRhi GPU buffer ownership | `RhiViewportRenderer` | `RhiViewport` synchronized state | QRhi buffers, pipelines, and uploads belong to the render thread item renderer. |
| Prepare page binding | QML presentation | C++ viewmodel | QML may bind existing properties into the viewport but must not filter objects or build geometry. |
| Source-truth mapping | Planning/code comments | Renderer/service implementation | Functional behavior maps to upstream `3DBed`, `PartPlate`, `PartPlateList`, `GLCanvas3D`, and `Plater`. |

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| RHI-04 | Renderer can keep Prepare mesh data and Preview G-code segment data in GPU-resident buffers and update only dirty ranges. | Phase 24 covers the Prepare half: explicit dirty flags and QRhi bed/plate buffers, leaving Preview G-code to Phase 26. |
| PREP-01 | User can see the active bed/plate rendered through the QRhi path with correct bed dimensions, grid, origin cues, and plate selection context. | `EditorViewModel` already exposes bed metrics and plate state; QRhi renderer can draw bed/grid/axes from these. |
| PREP-05 | User can switch plates and see the QRhi viewport update to the selected plate without leaking objects from inactive plates. | `PartPlateList` and `ProjectServiceMock` expose active-plate membership; add a typed renderer-facing active object index list. |
</phase_requirements>

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt Quick `QQuickRhiItem` | Qt 6.10.2 local SDK | QML-hosted QRhi item | Already introduced in Phase 23 and verified in local headers. |
| Qt Gui QRhi private headers (`rhi/qrhi.h`) | Qt 6.10.2 local SDK | Buffers, pipelines, command buffers, resource updates | Phase 23 renderer and benchmark already use QRhi APIs. |
| Qt Shader Tools | Qt 6.10.2 local SDK | `.qsb` shader resources | Already wired for app `rhi_viewport_shaders`. |
| Qt Test | Qt 6.10.2 local SDK | Unit/static tests | Existing `ViewModelSmokeTests` and `QmlUiAuditTests` use this. |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Existing `PartPlateList` | project implementation | Plate state and object membership | Active plate context and inactive object isolation. |
| Existing `EditorViewModel` | project implementation | UI-facing bed/plate state | Bind bed metrics and active plate indices into `RhiViewport`. |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Pure `PrepareSceneData` helper | Put dirty flags directly in `RhiViewportRenderer` | Harder to unit test and mixes render-thread GPU lifetime with CPU scene rules. |
| Typed active plate indices | Parse/filter from packed `meshData` only | Current packed IDs are renderer object IDs, not a reliable plate-membership API. |
| QRhi D3D12/D3D11 | Vulkan | Current QtGui public Vulkan support is disabled. |

**Installation:** No external packages are installed in this phase.

## Package Legitimacy Audit

No external packages are introduced. No registry or slopcheck audit is required.

## Architecture Patterns

### System Architecture Diagram

```text
EditorViewModel
  | bedWidth/bedDepth/origin/shape + currentPlateIndex/plateCount + active object indices
  v
PreparePage.qml (bindings only)
  v
RhiViewport (QQuickRhiItem state mirror)
  v synchronize()
PrepareSceneData (dirty flags + generated bed/grid/axis geometry)
  v dirty flags consumed by renderer
RhiViewportRenderer (QRhi buffers + pipelines + draw calls)
```

### Recommended Project Structure

```text
src/qml_gui/Renderer/
  PrepareSceneData.h          # pure scene model, dirty flags, generated geometry
  PrepareSceneData.cpp
  RhiViewport.h/.cpp          # QML properties for bed/plate context
  RhiViewportRenderer.h/.cpp  # QRhi resources and dirty-gated uploads
tests/
  PrepareSceneDataTests.cpp   # focused RED/GREEN unit tests for dirty flags and geometry
  QmlUiAuditTests.cpp         # static QML/C++ boundary guards
  ViewModelSmokeTests.cpp     # active plate context behavior
```

### Pattern 1: Pure Scene Contract Before GPU Resources
**What:** Build and test `PrepareSceneData` without QRhi objects, then let `RhiViewportRenderer` consume it.
**When to use:** Bed/grid geometry, dirty flag transitions, plate context isolation.
**Why:** It makes RHI-04 provable without a graphics backend and keeps render-thread code smaller.

### Pattern 2: Compatible Viewport Properties
**What:** Add properties to `RhiViewport` only where current `GLViewport`/`SoftwareViewport` compatibility or Phase 24 binding needs them.
**When to use:** `bedWidth`, `bedDepth`, `bedOriginX`, `bedOriginY`, `bedShapeType`, `bedDiameter`, `plateCount`, `currentPlateIndex`, `activePlateObjectIndices`.
**Why:** Existing `PreparePage.qml` can continue instantiating `GLViewport` while the registered implementation changes under the QRhi gate.

### Pattern 3: Dirty-Gated QRhi Uploads
**What:** `synchronize()` updates the CPU scene and sets dirty flags; `render()` allocates/reuploads QRhi buffers only when those flags are present.
**When to use:** Bed geometry changes, active plate switch, show-bed visibility, mesh byte/context changes.
**Why:** This is the core performance requirement for Phase 24.

### Anti-Patterns to Avoid
- **QML filtering active plate objects:** violates the project boundary and makes PREP-05 hard to test.
- **Per-frame geometry rebuild/upload:** violates RHI-04 and hides performance regressions.
- **Claiming full mesh rendering:** belongs to Phase 25 and should not be represented as complete in Phase 24.
- **Replacing SoftwareViewport default:** violates the Phase 23 startup contract.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| QRhi item lifecycle | Custom scenegraph plumbing | `QQuickRhiItem` / `QQuickRhiItemRenderer` | Phase 23 already proved the Qt-native path. |
| Plate membership source | QML arrays computed from UI delegates | `PartPlateList` via `ProjectServiceMock` / `EditorViewModel` | Maintains source-truth ownership and testability. |
| Shader pipeline | Runtime shader strings | Existing `.qsb` resources via `qt_add_shaders` | Consistent with QRhi and Phase 23. |

## Common Pitfalls

### Pitfall 1: Current `meshData()` Is Not Plate-Scoped Enough
**What goes wrong:** Renderer draws batches from inactive plates because packed mesh data lacks a typed active-plate membership contract.
**How to avoid:** Expose active plate object indices separately and store them in `PrepareSceneData`; Phase 25 can use that contract when mesh rendering is added.

### Pitfall 2: Dirty Flags Cleared Too Early
**What goes wrong:** Scene changes are acknowledged in `synchronize()` but buffers are not updated in `render()`.
**How to avoid:** `PrepareSceneData` should expose a consume/peek workflow so tests can verify flags remain visible until renderer upload.

### Pitfall 3: QRhi Resource Recreate Every Frame
**What goes wrong:** Bed/grid buffers are destroyed and recreated during every `render()`, defeating the performance goal.
**How to avoid:** Keep persistent `QRhiBuffer` members, compare required byte sizes, and upload only when dirty or resized.

### Pitfall 4: Over-implementing Upstream Plate Polish
**What goes wrong:** Logos, bed textures, calibration markers, icons, and height-limit visualization pull Phase 24 into a broad upstream rendering clone.
**How to avoid:** Implement bed fill/outline/grid/origin/active context only; record advanced plate decoration as out of scope for this phase.

## Code Examples

### QRhi Resource Updates

```cpp
// Source: E:/Qt6.10/include/QtGui/6.10.2/QtGui/rhi/qrhi.h
void QRhiResourceUpdateBatch::uploadStaticBuffer(QRhiBuffer *buf, quint32 offset, quint32 size, const void *data);
void QRhiCommandBuffer::beginPass(QRhiRenderTarget *rt, const QColor &color, const QRhiDepthStencilClearValue &depthStencilClearValue, QRhiResourceUpdateBatch *resourceUpdates = nullptr);
void QRhiCommandBuffer::draw(quint32 vertexCount, quint32 instanceCount = 1, quint32 firstVertex = 0, quint32 firstInstance = 0);
```

### Existing Bed Geometry Expectations

```cpp
// Source: src/qml_gui/Renderer/GLViewportRenderer.cpp
const float P = 220.f;
// border rectangle, 10 mm fine grid, 50 mm coarse grid, and 30 mm origin axes
```

### Upstream Bed Ground Plane And Axes

```cpp
// Source: third_party/OrcaSlicer/src/slic3r/GUI/3DBed.cpp
static const float GROUND_Z = -0.04f;
// Bed3D::set_shape updates build volume and axes origin.
// Bed3D::render_internal renders axes and bed model/custom background.
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `QQuickFramebufferObject` OpenGL viewport | `QQuickRhiItem` gated viewport | Phase 23 | Phase 24 should extend QRhi path, not GL FBO. |
| Diagnostic QRhi triangle | Bed/plate scene buffers | Phase 24 | Renderer becomes functionally useful for Prepare context. |
| UI-visible plate cards only | Plate context mirrored into QRhi scene data | Phase 24 | Active plate switching affects viewport rendering state. |

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|-------------|-----------|---------|----------|
| Qt Quick `QQuickRhiItem` | `RhiViewport` | yes | Qt 6.10.2 local SDK | SoftwareViewport |
| QRhi D3D12 | preferred app backend | yes by Phase 23 benchmark | Qt QRhi D3D12 | D3D11 |
| QRhi D3D11 | fallback app backend | yes by Phase 23 benchmark | Qt QRhi D3D11 | SoftwareViewport |
| QRhi Vulkan | not required | no public QtGui support | disabled | D3D12/D3D11 |
| Qt Test | scene/viewmodel/audit tests | yes | Qt 6.10.2 | static `rg` checks during early RED |

**Missing dependencies with no fallback:** None for Phase 24.

**Missing dependencies with fallback:** Vulkan unavailable; D3D12/D3D11 remain the performance path.

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Qt Test (`PrepareSceneDataTests`, `ViewModelSmokeTests`, `QmlUiAuditTests`) |
| Config file | root `CMakeLists.txt` |
| Quick run command | `build/PrepareSceneDataTests.exe` and `build/QmlUiAuditTests.exe` |
| Full suite command | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` |

### Phase Requirements To Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| RHI-04 | Dirty categories drive QRhi buffer upload state | unit/static | `build/PrepareSceneDataTests.exe`; `build/QmlUiAuditTests.exe` | new |
| PREP-01 | Bed dimensions/grid/origin/plate context reach QRhi renderer | unit/static/build | `build/PrepareSceneDataTests.exe`; `build/QmlUiAuditTests.exe` | new/partial |
| PREP-05 | Active plate context changes and inactive object indices are not in active context | unit/static | `build/ViewModelSmokeTests.exe`; `build/PrepareSceneDataTests.exe` | partial |

### Sampling Rate
- **Per task commit:** focused Qt test or static `rg` check from the task.
- **Per wave merge:** canonical full command when code has compiled.
- **Phase gate:** canonical full command plus optional `OWZX_RHI_RENDERER=1` smoke/log check if safe in the environment.

### Wave 0 Gaps
- [ ] `tests/PrepareSceneDataTests.cpp` for dirty flags and geometry.
- [ ] `PrepareSceneData` source files in `src/qml_gui/Renderer/`.
- [ ] QML/static audit for Prepare bed/plate bindings.

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | N/A |
| V3 Session Management | no | N/A |
| V4 Access Control | no | N/A |
| V5 Input Validation | yes | Clamp/validate renderer-facing bed dimensions, plate indices, and list sizes before buffer generation. |
| V6 Cryptography | no | N/A |

### Known Threat Patterns for Renderer Scene Data

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Invalid bed dimensions create huge buffers | Denial of Service | Clamp dimensions and skip invalid geometry. |
| Invalid plate index leaks stale context | Information Disclosure | Treat out-of-range active plate as empty context and mark plate dirty. |
| Large active object list causes unbounded memory use | Denial of Service | Bound list conversion and keep indices typed as ints. |

## Open Questions (RESOLVED)

1. **Should Phase 24 render full model meshes?** RESOLVED: No. Phase 24 creates scene/cache and bed/plate rendering; model mesh rendering is Phase 25.
2. **Should QML compute active plate object filtering?** RESOLVED: No. Filtering context stays in C++ viewmodel/scene data.
3. **Should Vulkan be tested again in Phase 24?** RESOLVED: No. Phase 23 already proved the current Qt SDK lacks public Vulkan support.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | A top-down orthographic bed/grid render is sufficient for Phase 24 before Phase 25 camera completion. | Architecture Patterns | If user-visible acceptance demands full 3D camera framing in Phase 24, part of Phase 25 must be pulled forward. |
| A2 | Active object indices are sufficient to prove no inactive object context leaks before full mesh rendering lands. | Phase Requirements | If PREP-05 requires rendered model geometry immediately, Phase 24 must be split or expanded into Phase 25 scope. |

## Sources

### Primary (HIGH confidence)
- `src/qml_gui/Renderer/RhiViewport.h/.cpp` - QQuickRhiItem host and existing property surface.
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp` - current diagnostic QRhi pipeline.
- `src/qml_gui/Renderer/GLViewportRenderer.cpp` - existing bed/grid/axis visual expectations.
- `src/core/viewmodels/EditorViewModel.h/.cpp` - bed metrics, current plate, mesh cache refresh, plate switching.
- `src/core/services/ProjectServiceMock.h/.cpp` - plate object index APIs and mesh packing.
- `src/core/model/PartPlate.h/.cpp`, `PartPlateList.h/.cpp` - plate membership source truth.
- `third_party/OrcaSlicer/src/slic3r/GUI/3DBed.cpp` - upstream bed shape, ground plane, axes, rendering.
- `third_party/OrcaSlicer/src/slic3r/GUI/PartPlate.cpp/.hpp` - upstream plate rendering and current-only filtering.
- `E:/Qt6.10/include/QtGui/6.10.2/QtGui/rhi/qrhi.h` - QRhi buffer updates and draw APIs.

### Secondary (MEDIUM confidence)
- `gsd-sdk query history-digest` - prior Phase 16-23 implementation dependencies and patterns.

### Tertiary (LOW confidence)
- None.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - confirmed from local Qt SDK and existing Phase 23 code.
- Architecture: HIGH - follows current viewmodel/service/rendering boundaries.
- Pitfalls: MEDIUM - based on current packed mesh contract and expected Phase 25 dependency.

**Research date:** 2026-06-27
**Valid until:** 2026-07-27 or until Qt SDK/rendering architecture changes.
