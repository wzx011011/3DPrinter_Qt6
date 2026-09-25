# Phase 203 Plan: D3D12 Root-Cause Confirmation (documentation-only)

**Milestone:** v5.6
**Scope:** Documentation only. This phase produces a root-cause confirmation
report for the historical D3D12 `0xC0000005` access violation. It does NOT
promote D3D12 to the default backend, does NOT change `RhiBackendSelector.cpp`,
does NOT touch `RhiViewportRenderer.cpp`, and does NOT relax the
`d3d12StaysOptInBehindEnvFlag` regression slot. Per the user decision, this
phase is explicitly **root-cause confirmation only, no default promotion**.

## Goal

Carry forward the v4.5 time-boxed D3D12 root-cause investigation
(`.planning/research/D3D12-CRASH-ROOT-CAUSE.md`, phase 106-01-01) into v5.6 by:

1. Re-reading the v4.5 conclusions.
2. Re-confirming the current code state (the BUG-V31-1 fix, the three candidate
   seams A/B/C, and the D3D11-first selector order) at their v5.6 line offsets,
   which drifted since phase 106 due to Phase 95 (THUMBCAP) and later
   view-renderer additions.
3. Producing an honest v5.6 confirmation report that states plainly: the root
   cause is **still not confirmed**, the crash **still does not reproduce** in
   the headless test environment, and D3D12 therefore **remains opt-in** behind
   `OWZX_RHI_RENDERER=d3d12`.

This is the documented acceptable outcome. Phase 203 does NOT fabricate a root
cause. "Not confirmed, D3D11-first retained" IS the correct deliverable for a
phase whose charter is root-cause confirmation without the tooling (original
machine + native debugger + minidump symbols) that a confirmed isolation
requires.

## Background

The D3D12 crash history is fully documented in
`.planning/research/D3D12-CRASH-ROOT-CAUSE.md`:

- **Crash signature:** exit code `-1073741819` = `0xC0000005`
  (STATUS_ACCESS_VIOLATION).
- **Original trigger (2026-06-27):** `OWZX_RHI_RENDERER=1` (normalized to
  `auto`, which under the OLD candidate order picked D3D12 first); fired ~3s
  after launch on the developer's workstation.
- **v4.5 repro attempts (2026-07-12):** two runs in the headless test
  environment (12s and 20s waits, D3D12 debug layer + QSG_RHI_DEBUG both
  active). The app survived both. No new minidump. The crash does NOT
  reproduce headlessly.
- **Three candidate seams (unconfirmed):** A = `cb->resourceUpdate()` outside a
  pass (thumbnail readback); B = three sub-range `updateDynamicBuffer` writes
  into the 256-byte camera UBO; C = first-frame SRB/buffer readiness race.
- **Decision rule:** D3D12-03 (DR-05) hard rule -- D3D12 stays opt-in until a
  confirmed root cause AND a clean repro on the original machine after the
  matching mitigation probe.

The current default policy is D3D11-first
(`RhiBackendSelector.cpp:56-65`), D3D12 is opt-in via
`OWZX_RHI_RENDERER=d3d12` plus `OWZX_D3D12_DEBUG`, and the regression slot
`d3d12StaysOptInBehindEnvFlag` (`tests/QmlUiAuditTests.cpp:4425`) locks that
order plus the no-Vulkan constraint plus the rationale comment.

## Hard scope constraints (do NOT cross)

1. **No code changes.** No `.cpp`, `.h`, `.qml`, CMake, or test file is modified
   by this phase. This is a pure documentation deliverable.
2. **No D3D12 default promotion.** D3D11 stays first in
   `defaultWindowsCandidates()`. The user decision for v5.6 is "root-cause
   confirmation only, no default promotion."
3. **No fabricated root cause.** If the crash cannot be reproduced and the
   tooling gap is unchanged, the report says so plainly. Inventing a confirmed
   root cause would violate the D3D12-03 contract and the regression slot.
4. **No regression-slot weakening.** `d3d12StaysOptInBehindEnvFlag` is verified
   intact at the end of the phase; it is not edited, deleted, or relaxed.
5. **ASCII-only English** for all added comment/report text (no non-ASCII).

## Deliverables

1. `.planning/milestones/v5.6-phases/203-d3d12-root-cause-confirmation/PLAN.md`
   (this file).
2. `.planning/milestones/v5.6-phases/203-d3d12-root-cause-confirmation/ROOT-CAUSE-REPORT.md`
   -- the v5.6 confirmation report that:
   a. References the v4.5 original conclusions.
   b. Re-confirms the v5.6 code state (BUG-V31-1 fix present; A/B/C seam
      locations at their drifted v5.6 offsets; D3D11-first order intact).
   c. States the honest conclusion: root cause still NOT confirmed, crash
      still NOT reproducible headlessly, D3D12 stays opt-in.
   d. Lists the prerequisites a confirmed isolation would need (original
      machine + VS/WinDbg/PIX + symbols + minidump analysis + A/B/C
      mitigation probe).
   e. Documents the A/B/C mitigation-probe procedures to run IF the original
      machine becomes available.

## Acceptance

- ROOT-CAUSE-REPORT.md exists and is ASCII-only English.
- `RhiBackendSelector.cpp` is byte-identical before/after this phase (verified
  by re-reading, not by edit -- no edit happens).
- `RhiViewportRenderer.cpp` is byte-identical before/after this phase.
- `tests/QmlUiAuditTests.cpp::d3d12StaysOptInBehindEnvFlag` still asserts:
  D3D11 before D3D12 in `defaultWindowsCandidates()`, the `D3D11-first`
  rationale comment present, `OWZX_RHI_RENDERER` remains the opt-in gate, and
  no `QRhi::Vulkan` in the default candidate list.
- No `.cpp`/`.h`/`.qml` file is modified by this phase.

## Non-goals (explicit)

- Promoting D3D12 to default. (Out of scope; blocked by D3D12-03 / DR-05.)
- Running a new repro attempt. (Headless box cannot reach the crashing frame;
  the v4.5 repro verdict stands. No new attempt is made in this phase.)
- Refactoring candidates A/B/C. (Those are mitigation probes for a future
  on-original-machine session, not v5.6 deliverables.)
- Replacing the v4.5 report. The v4.5
  `.planning/research/D3D12-CRASH-ROOT-CAUSE.md` stays as the historical
  record; this phase's ROOT-CAUSE-REPORT.md is the v5.6 re-confirmation layer.
