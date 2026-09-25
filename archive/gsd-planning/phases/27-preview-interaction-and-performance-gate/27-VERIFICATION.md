---
phase: 27
slug: preview-interaction-and-performance-gate
status: passed
verified: 2026-06-28
requirements: [PREV-05, PERF-01, PERF-02]
plans: [27-01]
---

# Phase 27 Verification — Preview Interaction + Performance Gate

**Status: passed.** Performance gate met; playback no-re-upload proven; timing instrumentation added.

## Must-haves vs. Codebase

| Must-have | Evidence | Result |
|---|---|---|
| Preview playback/scrub no full-buffer reupload | `RhiViewportRenderer.cpp:81-87`: only m_previewData change triggers re-parse; layerMin/Max/moveEnd copied without invalidating uploaded flag | ✓ PASS |
| Frame/upload timing captured | `RhiViewportRenderer.cpp` render(): QElapsedTimer + qInfo [RHI-PERF] | ✓ PASS |
| 1M D3D11 benchmark interactive | median 0.36ms, p95 0.60ms (≈2800fps) | ✓ PASS |
| 5M D3D11 benchmark | median 0.91ms, p95 1.59ms (≈1100fps); upload 198ms one-shot | ✓ PASS |
| render_bench D3D11-first aligned | `tools/render_bench/main.cpp:156-160` D3D11 before D3D12 | ✓ PASS |

## Benchmark JSON Evidence

```json
1M: {"backend":"d3d11","segments":1000000,"uploadMs":46.1,"medianFrameMs":0.36,"p95FrameMs":0.60,"firstFrameMs":5.0}
5M: {"backend":"d3d11","segments":5000000,"uploadMs":198.1,"medianFrameMs":0.91,"p95FrameMs":1.59,"firstFrameMs":19.0}
```

## Conclusion
Phase 27 complete. Preview pipeline validated at 1M/5M segments (both interactive). No-re-upload playback/scrub proven. Timing instrumentation added. Ready for Phase 28.
