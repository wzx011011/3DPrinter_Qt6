---
phase: 28-fallback-verification-reviews-and-handoff
plan: 01
subsystem: verification
tags: [qrhi, fallback, verification, milestone-handoff, v3.1-final]

requires:
  - phase: 27
    provides: "performance gate + timing instrumentation"
provides:
  - "QRhi fallback hardening verified (PREP-06)"
  - "v3.1 milestone handoff documentation"
affects: []

key-files:
  created: []
  modified:
    - src/qml_gui/main_qml.cpp

requirements-completed: [PREP-06, PERF-03, PERF-04, PERF-06]

duration: 15min
completed: 2026-06-28
---

# Plan 28-01: Fallback, Verification, Reviews, And Handoff Summary

**v3.1 final hardening: QRhi fallback verified, benchmark opt-in confirmed, milestone handoff documented. v3.1 QRhi High-Performance Prepare/Preview Rendering is delivery-ready.**

## Performance
- **Duration:** ~15 min
- **Files modified:** 1 (main_qml.cpp fallback diagnostic)

## Accomplishments
- **PREP-06:** QRhi init failure falls back to software viewport — verified with `OWZX_RHI_RENDERER=invalid_backend` (process alive, software viewport active, no crash).
- **PERF-03:** Build compiles clean (OWzxSlicer + all targets linked).
- **PERF-04:** Benchmark is opt-in (owzx-render-bench is a separate exe; app startup doesn't run it).
- **PERF-06:** QmlUiAudit/smoke guards fallback viewport property parity (Phase 25 pattern).
- **v3.1 measured backend:** D3D11 (safe default). D3D12 crashes in Prepare render (deferred). D3D11 interactive at 1M/5M segments (Phase 27 benchmark).

## v3.1 Milestone Summary (across phases 23-28)

| Phase | Focus | Status |
|---|---|---|
| 23 | QRhi Renderer Foundation + Backend Gate | ✓ Complete |
| 24 | Prepare Scene Data + Plate Rendering | ✓ Complete |
| 25 | Prepare Model Mesh + Camera + Selection | ✓ Complete |
| 26 | Preview G-Code GPU Pipeline | ✓ Complete |
| 27 | Preview Interaction + Performance Gate | ✓ Complete |
| 28 | Fallback + Verification + Handoff | ✓ Complete |

## v3.1 Measured Results
- **Backend:** D3D11 (safe default; D3D12 crash deferred)
- **Prepare rendering:** bed grid + model mesh + selection/hover through QRhi (Phases 24-25)
- **Preview rendering:** G-code segments via segment-buffer pipeline + draw-range layer filtering (Phase 26)
- **Performance:** 1M segments median 0.36ms, 5M segments median 0.91ms (Phase 27)
- **Fallback:** QRhi failure → software viewport (verified, Phase 28)

## Next Milestone: v3.2 AssembleView + Multi-Plate Polish
- AssembleView non-placeholder bird's-eye multi-plate layout
- Multi-plate arrangement, wipe-tower geometry, multi-thumbnail, filament-map UI
- PLATE-09 real-model 3MF fixture closure
- D3D12 crash root-cause investigation (optional)

---
*Phase: 28-fallback-verification-reviews-and-handoff*
*Completed: 2026-06-28*
