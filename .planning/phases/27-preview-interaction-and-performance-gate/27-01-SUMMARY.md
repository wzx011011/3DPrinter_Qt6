---
phase: 27-preview-interaction-and-performance-gate
plan: 01
subsystem: rendering
tags: [qrhi, preview, performance, benchmark, d3d11, timing-instrumentation]

requires:
  - phase: 26
    provides: "Preview segment pipeline + draw-range layer filtering"
provides:
  - "Frame/upload timing in RhiViewportRenderer Preview path (PERF-01)"
  - "D3D11 1M/5M benchmark evidence (PERF-02)"
  - "PREV-05 audit: no-re-upload on playback/scrub (code-level proof)"
affects: [28-fallback-verification-reviews-and-handoff]

key-files:
  created: []
  modified:
    - src/qml_gui/Renderer/RhiViewportRenderer.h
    - src/qml_gui/Renderer/RhiViewportRenderer.cpp
    - tools/render_bench/main.cpp

key-decisions:
  - "D-27-01: QElapsedTimer wall-clock timing (GPU timestamp queries deferred)"
  - "D-27-02: render_bench D3D11-first aligned with app crash workaround"

requirements-completed: [PREV-05, PERF-01, PERF-02]

duration: 20min
completed: 2026-06-28
---

# Plan 27-01: Preview Interaction + Performance Gate Summary

**Validated Phase 26 Preview pipeline: 1M/5M D3D11 benchmarks interactive, playback/scrub no-re-upload proven, frame timing instrumentation added.**

## Benchmark Results (D3D11)

| Metric | 1M segments | 5M segments |
|---|---|---|
| Vertices | 2,000,000 | 10,000,000 |
| Upload (one-shot) | 46.1ms | 198.1ms |
| Median frame | 0.36ms | 0.91ms |
| P95 frame | 0.60ms | 1.59ms |
| First frame | 5.0ms | 19.0ms |

Both workloads are **highly interactive** (median frame <1ms even at 5M segments = ~1100fps render capacity).

## PREV-05 Audit
synchronize() (`RhiViewportRenderer.cpp:83-87`) copies layerMin/Max/moveEnd/showTravelMoves directly WITHOUT invalidating m_previewSegmentBufferUploaded. Only m_previewData change (line 81) triggers re-parse. Playback and layer-scrubbing use draw-range only (computePreviewDrawRange) — zero buffer re-upload. **Code-level proven.**

## Task Commits
- All tasks: commit (feat 27)

## Next Phase Readiness
- Phase 27 complete. Ready for Phase 28 (fallback hardening + review + handoff).

---
*Phase: 27-preview-interaction-and-performance-gate*
*Completed: 2026-06-28*
