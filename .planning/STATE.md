---
gsd_state_version: 1.0
milestone: v5.7
milestone_name: D3D12 Backend Investigation
status: completed
last_updated: "2026-07-24T15:15:00.000Z"
last_activity: 2026-07-24
progress:
  total_phases: 6
  completed_phases: 6
  total_plans: 6
  completed_plans: 6
  percent: 100
---

# Project State

**Latest completed milestone:** v5.7 D3D12 Backend Investigation, closed on
2026-07-24.

**Completion evidence:** D3D12 promotion attempted, verified on real hardware,
found to crash on AMD Radeon APU (mainstream integrated graphics), and
**reverted** — D3D11 remains the default. Phase 207-210 deliverables
(diagnostics + seam A/B/C mitigations) retained. Canonical build (D3D11
default) exited `0`, `APP_RUNNING_PID=80404`, all ctest + E2E passed. See
`.planning/milestones/v5.7-MILESTONE-AUDIT.md`.

**Next step:** select the next milestone. D3D12 default promotion is closed
with hard evidence (unsafe on AMD APU until a swapchain workaround exists).

## Latest Milestone: v5.7 D3D12 Backend Investigation

**Result:** D3D12 default promotion is **not viable** on AMD Radeon APU
(confirmed real-machine crash at QQuickWindow swapchain init). D3D11-first
default restored. Phase 207-210 retained: env-gated diagnostics, seam A/B/C
mitigations (correct QRhi-usage improvements), render_bench verification
isolating the failure to the swapchain path. The "should we promote D3D12?"
question (deferred since v4.5) is now closed with evidence: **no**.

| Phase | Name | Status | Requirement |
|---|---|---|---|
| 206 | Planning State Reconciliation | Complete | DOC-01 |
| 207 | Diagnostic Hardening + In-Environment Reproduction | Complete | REPRO-01 |
| 208 | Seam A Mitigation (readback fold) | Complete | MIT-01 |
| 209 | Seam B Mitigation (camera UBO pack) | Complete | MIT-02 |
| 210 | Seam C Mitigation (first-frame force + initialize reset) | Complete | MIT-03 |
| 211 | D3D12 Promotion (attempted) → Reverted to D3D11-first | Complete (reverted) | PROMO-01 |

## Project Reference

See:

- `.planning/PROJECT.md`
- `.planning/ROADMAP.md`
- `.planning/REQUIREMENTS.md`
- `.planning/milestones/v5.7-ROADMAP.md`
- `.planning/milestones/v5.7-REQUIREMENTS.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth.
