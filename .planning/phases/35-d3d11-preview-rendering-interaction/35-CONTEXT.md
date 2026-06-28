# Phase 35: D3D11 Preview Rendering Interaction - Context

**Gathered:** 2026-06-28
**Status:** Ready for planning
**Source:** v3.3 ROADMAP, REQUIREMENTS, and Phase 34 parser completion

<domain>
## Phase Boundary

Phase 35 makes the existing Qt RHI Preview viewport visibly useful and interactive on the Windows D3D11 path. It consumes the Phase 34 `GCV1` preview payload and hardens renderer-side range handling for layer, move, travel visibility, and color mode MVP controls. It does not promote Vulkan, D3D12, or SoftwareViewport as the normal path.
</domain>

<decisions>
## Implementation Decisions

### Rendering Path
- D-35-01: Keep Windows default Preview on the existing QRhi/D3D11-capable `RhiViewport` path registered as `GLViewport`; `SoftwareViewport` is a fallback only, not the normal Preview route.
- D-35-02: Treat the `PreviewViewModel` `GCV1` payload as the source of visible preview segments. Phase 34 already repacks the payload when travel visibility or color mode changes.
- D-35-03: Renderer range selection must use packed segment layer and move indices exactly; it must not approximate move playback by scaling vertex counts.

### Interaction Scope
- D-35-04: Layer slider, move slider, travel toggle, and color mode MVP changes must flow through existing QML bindings into `RhiViewport` without resizing or switching viewport classes.
- D-35-05: The renderer may keep one contiguous draw range for the MVP because layer ranges are continuous and travel filtering is applied before packing.

### Verification
- D-35-06: Add deterministic source/audit coverage for the Preview RHI path and exact range logic before changing renderer code.
- D-35-07: Canonical verification remains the only accepted build/test command: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.

### the agent's Discretion
- Exact internal struct names and helper placement inside `RhiViewportRenderer`.
- Whether range calculation remains a helper returning `firstVertex`/`vertexCount` or is split into smaller helpers, provided the behavior is exact and covered.
</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `src/qml_gui/pages/PreviewPage.qml` already instantiates `GLViewport` for Preview and binds `previewData`, layer bounds, `moveEnd`, `showTravelMoves`, and `gcodeViewMode`.
- `src/qml_gui/Renderer/RhiViewport.h` exposes the matching QML-facing properties.
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp` already parses `GCV1` payloads, uploads segment vertices to QRhi buffers, and draws lines for `CanvasPreview`.
- `tests/QmlUiAuditTests.cpp` is the existing source-audit suite for QML and renderer wiring.

### Established Patterns
- QML remains presentation-only; range state comes from `PreviewViewModel`.
- Renderer code uses `qWarning`/`qInfo` tagged logs and local structs in `RhiViewportRenderer`.
- Existing audit tests protect source-level architectural constraints that are hard to assert in Qt headless runtime.

### Integration Points
- `PreviewViewModel::gcodePreviewData` changes after parsing, travel toggle, and color mode changes.
- `PreviewPage.qml` passes `previewVm.currentLayerMin`, `currentLayerMax`, and `currentMove` into the viewport.
- `main_qml.cpp` selects QRhi graphics API and registers `RhiViewport` as `GLViewport` unless explicitly configured otherwise.
</code_context>

<specifics>
## Specific Ideas

- Replace proportional move clipping with per-segment draw spans keyed by packed `move` and `layer`.
- Remove the renderer-side assumption that `move == 0` means travel; `move` is a playback index.
- Keep the Phase 34 `GCV1` format stable unless an implementation blocker proves a schema change is necessary.
</specifics>

<deferred>
## Deferred Ideas

- Full upstream Preview parity beyond MVP range/toolpath rendering.
- Vulkan or D3D12 promotion.
- Real pixel-based visual regression capture for G-code preview, unless needed for Phase 36 final UAT.
</deferred>

---

*Phase: 35-d3d11-preview-rendering-interaction*
*Context gathered: 2026-06-28 via autonomous lifecycle defaults*
