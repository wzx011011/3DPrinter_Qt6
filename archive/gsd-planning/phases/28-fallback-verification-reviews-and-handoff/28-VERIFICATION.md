---
phase: 28
slug: fallback-verification-reviews-and-handoff
status: passed
verified: 2026-06-28
requirements: [PREP-06, PERF-03, PERF-04, PERF-06]
plans: [28-01]
---

# Phase 28 Verification — Fallback, Verification, Reviews, And Handoff

**Status: passed.** v3.1 milestone complete.

## Must-haves vs. Codebase

| Must-have | Evidence | Result |
|---|---|---|
| QRhi init failure → software viewport (no crash) | `OWZX_RHI_RENDERER=invalid_backend` → process alive, software backend; startup log `selected=<none> failures=[...]` | ✓ PASS |
| Canonical verify passes (build clean) | OWzxSlicer.exe + all targets linked, 0 errors | ✓ PASS |
| Benchmark disabled by default | owzx-render-bench is a separate exe (CMakeLists:390); app startup doesn't run it | ✓ PASS |
| UI audit/smoke guards fallback | QmlUiAudit tests 8 passed; GLViewport name parity across GL/Rhi/Software | ✓ PASS |
| Handoff documents backend choice + next milestone | This SUMMARY: D3D11 measured, D3D12 deferred, next = v3.2 AssembleView | ✓ PASS |

## Requirements Coverage

| REQ-ID | Requirement | Status | Evidence |
|---|---|---|---|
| PREP-06 | QRhi failure → fallback with diagnostic | ✓ satisfied | invalid_backend test: software viewport, no crash |
| PERF-03 | Canonical verify with QRhi code present | ✓ satisfied | Build compiles clean |
| PERF-04 | Benchmark opt-in from canonical script | ✓ satisfied | Separate exe; not in app startup |
| PERF-06 | UI audit guards fallback + QRhi gated | ✓ satisfied | QmlUiAudit 8 passed; GLViewport name maps by gate |

## v3.1 Milestone Final Status

- **Phases:** 6/6 complete (23-28)
- **Requirements:** 26/26 satisfied (RHI-01..06, PREP-01..07, PREV-01..07, PERF-01..06)
- **Measured backend:** D3D11 (safe default)
- **D3D12 status:** Crashes in Prepare render; D3D11-first workaround in place; deep investigation deferred
- **Performance:** 1M segments 0.36ms median, 5M segments 0.91ms median (D3D11)

Ready for `/gsd-audit-milestone v3.1`.
