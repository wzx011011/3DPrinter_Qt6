# Phase 27: Preview Interaction And Performance Gate - Context

**Gathered:** 2026-06-28
**Status:** Ready for planning

<domain>
## Phase Boundary

Validate the Phase 26 Preview pipeline against the no-full-reupload design goal: (1) run render_bench to capture 1M/5M segment performance under D3D11, (2) add in-app frame/upload timing to RhiViewportRenderer (currently absent — PERF-01 gap), (3) verify playback/layer-scrubbing updates the draw range without buffer re-upload (PREV-05 — already implemented structurally, needs audit/instrumentation proof).

**Scout finding:** ~70% "run existing tools + capture evidence", ~30% "small new code" (frame/upload timing in renderer). No new pipelines, shaders, or infrastructure. The draw-range design (D-26-02) already eliminates per-frame CPU filter and re-upload. render_bench already validated 1M/5M segments (spike 002 has prior D3D12 results; needs D3D11 re-run after the backend-order fix).

**In scope:** render_bench D3D11 1M/5M benchmark runs; in-app frame/upload timing capture (QElapsedTimer in RhiViewportRenderer); PREV-05 playback/scrub audit (verify no re-upload path); structured output.
**Out of scope:** Per-move offset index for precise playback (proportional clamp is acceptable for Phase 27; precise per-move is a follow-up if visually problematic); GPU timestamp queries (D3D11 timestamp queries are complex; wall-clock QElapsedTimer is sufficient for Phase 27).

</domain>

<decisions>
## Implementation Decisions

### Frame/upload timing in RhiViewportRenderer (D-27-01)
- **D-27-01:** Add `QElapsedTimer` based timing to RhiViewportRenderer for Preview rendering: measure segment-buffer upload time (ms, from `uploadStaticBuffer` call to batch completion) and frame time (ms, per `render()` call for CanvasPreview). Log via `qInfo("[RHI-PERF]")` tagged output to startup_diagnostics.log when `QML_DEBUG_LOG` is set. Capture first-frame separately. This satisfies PERF-01's "frame timing, first-frame timing, upload timing, selected backend, segment counts" requirement.

### render_bench re-run with D3D11 (D-27-02)
- **D-27-02:** Run `owzx-render-bench --backend d3d11 --segments 1000000` and `--segments 5000000` to capture current D3D11 performance (spike 002 has D3D12 numbers; D3D11 is the new safe default). Also align render_bench's `stableAuto` order to D3D11-first to match the app's crash workaround. Record results in VERIFICATION.md.

### PREV-05 playback audit (D-27-03)
- **D-27-03:** Audit the QRhi Preview path to confirm: layer-range change (setLayerRange/moveLayerRange) and playback (setCurrentMove/playAnimation) do NOT trigger buffer re-upload. The structural design (D-26-02 draw-range) already prevents re-upload; Phase 27 adds code-level proof: synchronize() only re-parses on m_previewData change; layerMin/Max/moveEnd are copied without invalidating m_previewSegmentBufferUploaded. Document this in VERIFICATION.md with code citations.

### Claude's Discretion
- Timing precision (QElapsedTimer vs std::chrono; QElapsedTimer is Qt-idiomatic).
- Whether to also add timing to Prepare render path (optional; Preview is the focus).
- Output format (tagged qInfo vs structured JSON file; tagged qInfo is simpler).

</decisions>

<canonical_refs>
## Canonical References
- `tools/render_bench/main.cpp` — CLI args (50-87), runOffscreenBenchmark (224-334), JSON output (336-350)
- `tools/render_bench/main.cpp:156-159` — stableAuto order (D3D12-first; align to D3D11-first per D-27-02)
- `.planning/spikes/002-render-bench-qrhi-backend/README.md` — prior 1M/5M benchmark results (D3D12)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:76-86` — synchronize: only re-parse on data change (PREV-05 proof)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:633-672` — computePreviewDrawRange: proportional moveEnd clamp
- `src/core/viewmodels/PreviewViewModel.cpp:294-302` — setCurrentMove: no re-pack (PREV-05 proof)

</canonical_refs>

<deferred>
## Deferred Ideas
- Per-move offset index for precise playback boundary (proportional clamp is acceptable for Phase 27)
- GPU timestamp queries (wall-clock QElapsedTimer is sufficient)
- Prepare render path timing (Preview is the perf-sensitive path; Prepare timing optional)

</deferred>

---

*Phase: 27-preview-interaction-and-performance-gate*
*Context gathered: 2026-06-28*
