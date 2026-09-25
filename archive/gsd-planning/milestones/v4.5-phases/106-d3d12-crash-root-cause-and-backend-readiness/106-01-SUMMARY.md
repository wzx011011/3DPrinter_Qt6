---
phase: 106-d3d12-crash-root-cause-and-backend-readiness
plan: 01
subsystem: infra
tags: [d3d12, rhi, crash-triage, root-cause, time-boxed, source-audit, regression-lock, canonical-build, ctest, vulkan-sdk-blocked]

# Dependency graph
requires:
  - phase: 105-d3d12-debug-layer-wiring
    provides: OWZX_D3D12_DEBUG env flag + QSG_RHI_DEBUG forwarding (the triage tooling used by the repro attempts)
provides:
  - D3D12-CRASH-ROOT-CAUSE.md time-boxed root-cause report (repro procedure, static triage, BUG-V31-1 verdict, leading hypothesis, next steps, tooling gap)
  - D3D12-03 STATE.md policy entry (no default promotion until confirmed root cause; Vulkan SDK-blocked)
  - d3d12StaysOptInBehindEnvFlag source-audit slot locking the D3D11-first defaultWindowsCandidates() order (DR-05)
affects: [d3d12-default-promotion, vulkan-backend, rhi-backend-selector, qml-ui-audit-tests]

# Tech tracking
tech-stack:
  added: []  # investigation phase — no new libraries
  patterns:
    - "Time-boxed investigation report (DR-04): when a repro does not converge in the test env, ship the hypothesis + tooling gap instead of fabricating a root cause"
    - "Position-ordered defaultWindowsCandidates() source-audit (DR-05): slice the function body between its signature and the next function, assert Direct3D11 precedes Direct3D12 + QRhi::D3D11 precedes QRhi::D3D12 — reuses the Phase 55/73/105 boundary check"

key-files:
  created:
    - .planning/research/D3D12-CRASH-ROOT-CAUSE.md
  modified:
    - .planning/STATE.md
    - tests/QmlUiAuditTests.cpp

key-decisions:
  - "D3D12-02 closed as TIME-BOXED (DR-04 acceptable outcome): the 0xc0000005 access violation does NOT reproduce in the test environment (survived 20s with OWZX_RHI_RENDERER=d3d12 OWZX_D3D12_DEBUG=1, no new minidump). A confirmed root cause needs an interactive display + GPU debugger on the original machine."
  - "BUG-V31-1 verdict = SUPERSEDED as a live explanation: the cited ordering violation (camera uniform uploaded AFTER beginPass) is already fixed at RhiViewportRenderer.cpp:281-298 (updates folded into beginPass 4th arg). The blamed pattern no longer exists in source, so V31-1 cannot be the active cause of any crash today. Inconclusive as the historical cause (no debugger to confirm)."
  - "Leading hypothesis: three candidate seams ranked by plausibility — (A) cb->resourceUpdate() outside a pass at RhiViewportRenderer.cpp:414 (thumbnail readback); (B) three sub-range updateDynamicBuffer writes into the camera UBO at :1434/1449/1451; (C) first-frame SRB/buffer readiness race for CanvasView3D/AssembleView. None confirmed without a debugger."
  - "D3D12-03 documented in STATE.md: default-backend promotion stays OUT OF SCOPE until a confirmed root cause + clean repro on the original machine. D3D12 stays opt-in via OWZX_RHI_RENDERER=d3d12. Vulkan is SDK-blocked (Qt disables the 'vulkan' public feature, PROJECT.md:143), evaluation-only, NOT a v4.5 deliverable."
  - "DR-05 hard rule locked by source-audit: defaultWindowsCandidates() at RhiBackendSelector.cpp:56-65 keeps D3D11 first (d3d11 at :62 precedes d3d12 at :63). The d3d12StaysOptInBehindEnvFlag slot asserts this position-ordered + the D3D11-first comment + the OWZX_RHI_RENDERER opt-in gate + no Vulkan in the default selector."

patterns-established:
  - "Honest time-box (DR-04): a documented 'inconclusive with hypothesis X + needs tool Y' is the acceptable D3D12-02 outcome. Do NOT fabricate a root cause to look done."
  - "D3D12-03 regression lock pattern: position-ordered defaultWindowsCandidates() boundary check (slice between signature and next function, assert D3D11-before-D3D12) + D3D11-first comment presence + opt-in gate + no-Vulkan. A future refactor that flips the order fails deterministically."

requirements-completed: [D3D12-02, D3D12-03]

# Metrics
duration: ~50 min
completed: 2026-07-12
---

# Phase 106 Plan 01: D3D12 Crash Root-Cause And Backend Readiness Summary

**Time-boxed D3D12 crash investigation (DR-04): the 0xc0000005 access violation does NOT reproduce in the test env, so D3D12-02 ships a documented hypothesis + tooling gap (NOT a fabricated root cause); D3D12-03 documented in STATE.md and locked by a d3d12StaysOptInBehindEnvFlag source-audit slot that keeps D3D11-first in defaultWindowsCandidates(). Canonical build clean, regression 5/5 PASS.**

## Performance

- **Duration:** ~50 min (repro attempts + static triage + report ~25 min; STATE.md + source-audit slot ~5 min; canonical build + targeted test-build + ctest + liveness ~20 min)
- **Started:** 2026-07-12 (repro attempts first, then static triage, then report)
- **Completed:** 2026-07-12
- **Tasks:** 4
- **Files modified:** 3 (1 created, 2 modified)

## Accomplishments
- D3D12-02 closed as TIME-BOXED (DR-04): two repro attempts in the test environment (12s + 20s waits with OWZX_RHI_RENDERER=d3d12 OWZX_D3D12_DEBUG=1) both survived with NO crash and NO new minidump. The historical crash (0xc0000005, June 27/28 dumps) is environment-dependent and does not reproduce headless. Root-cause report at `.planning/research/D3D12-CRASH-ROOT-CAUSE.md` documents the repro procedure, the failed capture (debug layer writes to OutputDebugString, not stdout/stderr), the static triage of the render path, the BUG-V31-1 verdict, three ranked candidate seams, next steps, and the explicit tooling gap.
- BUG-V31-1 verdict = SUPERSEDED as a live explanation. The cited pattern (camera uniform uploaded AFTER beginPass) is already fixed at RhiViewportRenderer.cpp:281-298 — the `updates` batch carrying uploadCameraUniform (:295) and the preview-segment upload (:291-292) is folded into `cb->beginPass(...)` as its 4th argument at :298. The blamed ordering no longer exists, so V31-1 cannot be the active cause of any current crash (PITFALLS.md pitfall 5 was right to call it a hypothesis).
- D3D12-03 documented in STATE.md Deferred Items: D3D12 default-backend promotion stays OUT OF SCOPE until a confirmed root cause + clean repro on the original machine; D3D12 remains opt-in via OWZX_RHI_RENDERER=d3d12; Vulkan is SDK-blocked and evaluation-only.
- DR-05 hard rule locked: added `d3d12StaysOptInBehindEnvFlag` source-audit slot to QmlUiAuditTests asserting (a) defaultWindowsCandidates() keeps D3D11 before D3D12 (position-ordered, reusing the Phase 55/73/105 boundary check); (b) the D3D11-first rationale comment is present; (c) OWZX_RHI_RENDERER remains the opt-in gate; (d) no Vulkan in the default selector. A future reorder that puts D3D12 first fails this slot deterministically.
- Canonical build clean + regression 5/5 PASS (QmlUiAuditTests incl. the new slot, ViewModelSmokeTests, PartPlateTests, PrepareSceneDataTests, PreviewParserTests).

## Task Commits

Each task was committed atomically:

1. **Task 106-01-01: Repro + triage + root-cause report** - `e97b637` (investigation)
2. **Task 106-01-02: D3D12-03 in STATE.md** - `5f40805` (docs)
3. **Task 106-01-03: d3d12StaysOptInBehindEnvFlag source-audit slot** - `33f5625` (test)
4. **Task 106-01-04: Build + regression ctest** - this summary (no separate code commit; verification task)

**Plan metadata:** this SUMMARY commit (docs)

## Files Created/Modified
- `.planning/research/D3D12-CRASH-ROOT-CAUSE.md` - Time-boxed D3D12 root-cause report: repro procedure (DR-01), debug-layer capture (DR-02, none capturable + reason), static triage of the render path, BUG-V31-1 SUPERSEDED verdict, three ranked candidate seams, next steps, tooling gap (DR-04). [106-01-01, e97b637]
- `.planning/STATE.md` - D3D12-03 policy row in Deferred Items: no default promotion until confirmed root cause; D3D12 opt-in; Vulkan SDK-blocked. [106-01-02, 5f40805]
- `tests/QmlUiAuditTests.cpp` - `d3d12StaysOptInBehindEnvFlag` slot declaration + implementation asserting D3D11-first order, D3D11-first comment, OWZX_RHI_RENDERER opt-in gate, no Vulkan in default selector (DR-05). [106-01-03, 33f5625]

## Decisions Made
- **Time-box acceptance (DR-04).** The repro did not converge in the test environment (no display, no GPU debugger, different GPU/driver than the original machine). Per DR-04, the investigation ships the leading hypothesis + next steps + tooling gap rather than fabricating a root cause. This is the documented acceptable D3D12-02 outcome.
- **BUG-V31-1 = SUPERSEDED, not confirmed/refuted.** Static triage proved the cited fix is in place (updates folded into beginPass at :298), so V31-1 cannot be the active cause. But without a debugger the historical cause cannot be confirmed or refuted — only superseded as a live explanation.
- **Three candidate seams documented, not patched.** Per the phase plan (investigation, no production code unless a clear fix emerges), the three candidate seams (resourceUpdate outside pass at :414; sub-range UBO writes at :1434/1449/1451; first-frame readiness race) are documented as testable hypotheses for the next investigation, not refactored blind.
- **No default promotion (DR-05).** defaultWindowsCandidates() order unchanged (D3D11 first at :62, D3D12 second at :63). Locked by the new source-audit slot.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
- **D3D12 repro did not crash in the test environment.** Two attempts (12s, 20s) with OWZX_RHI_RENDERER=d3d12 OWZX_D3D12_DEBUG=1 both survived; no new minidump. Root cause: headless box has no interactive display (the crash fires in the live QQuickRhiItem render path which needs a composited swapchain), a different GPU/driver than the original workstation, and no debugger to capture the OutputDebugString-channel debug-layer text. This is the DR-04 documented acceptable outcome, not a failure of the investigation method.
- **Canonical verify wrapper killed mid test-target build.** `scripts/auto_verify_with_vcvars.ps1` completed the main build (`[236/236] Linking CXX executable OWzxSlicer.exe`) and began the test-target builds (`[1/4] Automatic MOC and UIC for target E2EWorkflowTests`) before the background budget elapsed. Per the project_specific_notes ("Fall back to targeted ninja if wrapper times out") and the Phase 105 precedent, the test-target build + ctest + regression gates were completed via the vcvars-preamble fallback (PATH-sanitize + vcvars + Windows-Kits fallback verbatim from the wrapper, then targeted ninja on the changed test targets + ctest). The main app build was clean in the wrapper run.
- **Two pre-existing ctest failures unrelated to this phase.** `InventoryAuditTests` fails because `.planning/phases/50-screenshot-and-source-truth-inventory/50-INVENTORY.md` was archived to milestones in commit 4da9eb3 (the test reads the old path). `CliTests` fails in testLoadHotend + testSliceBlock20XY (CLI model-loading output + libslic3r slicing exit code -3). Neither is in the canonical script's blocking regression gates, neither touches D3D12/STATE.md/QmlUiAuditTests, and neither was introduced by Phase 106-01. The 5 canonical blocking gates (QmlUiAuditTests, ViewModelSmokeTests, PartPlateTests, PrepareSceneDataTests, PreviewParserTests) all PASS.

## Verification

**Canonical build (wrapper, partial):** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` — OWzxSlicer.exe + libslic3r built clean (`[236/236] Linking CXX executable OWzxSlicer.exe`, zero errors). Configure passed (Windows-Kits fallback fired). Killed mid test-target build by the background budget; main app link succeeded.

**Targeted test-build (vcvars-preamble fallback):** `ninja E2EWorkflowTests ViewModelSmokeTests QmlUiAuditTests InventoryAuditTests PrepareSceneDataTests` — NINJA_EXIT=0. QmlUiAuditTests rebuilt with the new `d3d12StaysOptInBehindEnvFlag` slot (AUTOMOC re-ran: `[1/16] Automatic MOC and UIC for target QmlUiAuditTests`). All 5 test targets linked clean.

**Regression ctest (5/5 canonical blocking gates PASS, 0.07-1.23s each):**
- PrepareSceneDataTests: PASS
- PartPlateTests: PASS
- ViewModelSmokeTests: PASS
- QmlUiAuditTests: PASS (contains the new d3d12StaysOptInBehindEnvFlag slot)
- PreviewParserTests: PASS

**Full ctest (12 tests, 10 PASS / 2 pre-existing FAIL):** E2EWorkflowTests PASS, ViewModelSmokeTests PASS, QmlUiAuditTests PASS, PrepareSceneDataTests PASS, GizmoMathTests PASS, ObjectPickingTests PASS, GizmoGeometryTests PASS, PartPlateTests PASS, GizmoStateWiringTests PASS, PreviewParserTests PASS; InventoryAuditTests FAIL (Phase 50 doc archival, pre-existing), CliTests FAIL (CLI slicing behavior, pre-existing). Both failures pre-date Phase 106-01 and are unrelated to D3D12/STATE.md/QmlUiAuditTests.

**D3D12 repro attempts (test env):** `OWZX_RHI_RENDERER=d3d12 OWZX_D3D12_DEBUG=1`, two launches (12s, 20s). Both survived with no crash and no new minidump. The historical 0xc0000005 access violation does NOT reproduce in this environment (documented per DR-04).

## User Setup Required
None - no external service configuration required. The OWZX_D3D12_DEBUG and OWZX_RHI_RENDERER env flags are dev/debug-only opt-in gates (Phase 105 wiring), not user-facing configuration.

## Next Phase Readiness
- D3D12-02 is closed (time-boxed, DR-04). D3D12-03 is documented + source-audit-locked. Phase 106 plan 01 is complete.
- A future D3D12 root-cause confirmation phase would need: (1) repro on the original machine with Visual Studio or WinDbg attached to capture the debug-layer output + faulting frame; (2) open the existing June-27/28 `crash_dumps/*.dmp` in WinDbg with symbols; (3) probe candidate seam A (resourceUpdate outside pass at :414), B (sub-range UBO writes), C (first-frame readiness race) per the report's Section 5.
- D3D12 default promotion stays blocked behind that confirmation (D3D12-03 + DR-05 + the d3d12StaysOptInBehindEnvFlag slot).
- No blockers for the rest of the v4.5 roadmap: D3D12 is P2/P3 and fully decoupled from the WS1/WS2/WS3/WS5 workstreams.

## Self-Check: PASSED
- [x] `.planning/research/D3D12-CRASH-ROOT-CAUSE.md` exists with DR-02 sections (repro, debug output, static triage, hypothesis, BUG-V31-1 verdict, next steps, tooling gap).
- [x] BUG-V31-1 explicitly addressed (SUPERSEDED as live explanation; inconclusive as historical cause).
- [x] Honest about investigation limits (no fabricated root cause; DR-04 time-box documented).
- [x] D3D12-03 documented in STATE.md.
- [x] defaultWindowsCandidates() order unchanged (D3D11 first; DR-05).
- [x] d3d12StaysOptInBehindEnvFlag slot exists, deterministic, passes (QmlUiAuditTests PASS).
- [x] Canonical build clean (OWzxSlicer.exe link success); regression ctest 5/5 PASS.
- [x] Each task committed atomically; SUMMARY committed.

---
*Phase: 106-d3d12-crash-root-cause-and-backend-readiness*
*Completed: 2026-07-12*
