# Roadmap: Milestone v2.9 - Implementation Realignment and Stabilization

## Overview

v2.9 is a stabilization and truth-reset milestone. Its job is to make the project plan, current implementation, visible UI, and verification evidence agree before the next large source-truth migration module starts.

**Status:** Planning
**Started:** 2026-06-24
**Phase numbering:** continues from prior milestone artifacts, starting at Phase 10.

## Phase 10: Planning Truth Reset - Complete

**Goal:** Make `.planning` a trustworthy entry point again.
**Completed:** 2026-06-25

**Requirements:** PLAN-01, PLAN-02, PLAN-03, PLAN-04, PLAN-05

**Scope:**
- Reconcile `PROJECT.md`, `STATE.md`, `REQUIREMENTS.md`, `ROADMAP.md`, `INDEX.md`, and `REMAINING_MIGRATION_PLAN.md`.
- Reflect v2.7/v2.8 git history as already-landed work rather than future scope.
- Add and maintain the Real/Hybrid/Mock/Blocked/Placeholder classification matrix.
- Resolve or redirect missing `.Codex/rules/*` references used by AGENTS.
- Classify legacy CrealityPrint references as historical evidence, vendor data, or cleanup debt.

**Success criteria:**
1. All active planning entry files name v2.9 as the current milestone.
2. Every v2.9 requirement maps to exactly one roadmap phase.
3. Service/workflow classification is present and evidence-based.
4. AGENTS rule references resolve to `.Codex/rules/*`.

## Phase 11: Source Hygiene Stabilization

**Goal:** Remove source-level ambiguity introduced by dirty local work and encoding damage.

**Requirements:** HYGIENE-01, HYGIENE-02, HYGIENE-03, HYGIENE-04

**Scope:**
- Fix literal escape artifacts such as `\r\n` inside active source comments when they affect behavior.
- Repair encoding-damaged comments and user-visible strings in active files touched by recent implementation work.
- Confirm ownership of `src/core/services/SliceService.cpp.backup`, then remove it or move it outside active source.
- Classify untracked baseline files such as `AppSettingsService.*` and `SoftwareViewport.*` as intentional implementation, external artifact, or deferred cleanup.

**Success criteria:**
1. Broad source searches no longer surface behavior-affecting literal escape damage in active files.
2. No unexplained backup source file remains under `src/`.
3. The dirty baseline is reduced to intentional tracked changes or documented external files.
4. Canonical verification still builds after hygiene changes.

## Phase 12: Calibration Closure for Implemented Modes

**Goal:** Turn the existing hybrid PA/Flow/Temp calibration work into visible, deterministic, verified behavior.

**Requirements:** CAL-01, CAL-02, CAL-03, CAL-04, CAL-05

**Scope:**
- Wire implemented calibration modes from BBLTopbar or equivalent visible UI paths to `CalibrationViewModel`.
- Add deterministic coverage for PA, Flow Rate, and Temp Tower job creation or slice request generation.
- Preserve mock fallback behavior when `SliceService` is not available.
- Ensure progress comes from `SliceService` in real mode and only from the timer in mock mode.
- Mark remaining calibration modes Pending or Blocked with upstream references.

**Success criteria:**
1. Implemented calibration modes are no longer disabled placeholders in the topbar path.
2. Three implemented calibration modes have deterministic regression coverage.
3. Mock fallback and real slice progress paths are separately verified.
4. Unimplemented calibration modes are explicit future work, not silent gaps.

## Phase 13: Hybrid Integration Verification

**Goal:** Replace transport fallback confidence with deterministic evidence.

**Requirements:** INT-01, INT-02, INT-03, INT-04, INT-05, INT-06

**Scope:**
- Add or update tests/fixtures for SSDP response parsing, MQTT status/control payloads, FTP send-print success/error handling, and camera no-stream/error paths.
- Document real-printer/manual verification requirements where live hardware is unavoidable.
- Verify software viewport and OpenGL fallback startup behavior.
- Verify or defer AppSettings and bed-shape persistence behavior introduced by recent local work.

**Success criteria:**
1. Network/device tests do not require a live printer for protocol-level coverage.
2. Real hardware requirements are documented separately from automated regression checks.
3. Startup smoke evidence covers the selected viewport mode.
4. Canonical verification remains green.

## Phase 14: Visible Placeholder Triage

**Goal:** Stop counting visible placeholders as completed product workflows.

**Requirements:** UI-01, UI-02, UI-03, UI-04, UI-05

**Scope:**
- Wire export project/model and preferences menu actions to real backend behavior where feasible.
- Enable and route calibration menu entries for implemented modes.
- Reclassify account, model store, publish, layer editing, AssembleView, and ModelMall/WebView surfaces as Real, Hybrid, Placeholder, or Blocked.
- Identify QML-side durable behavior that should move into C++ viewmodels/services.

**Success criteria:**
1. Top-level menu actions are not silent no-ops.
2. Implemented calibration entries are visible and functional from the topbar path.
3. Placeholder surfaces are documented as such in requirements/planning.
4. Any QML business-logic extraction needed for v2.9 is either done or scoped to a future phase with evidence.

## Phase 15: Verification and Handoff

**Goal:** Produce the final v2.9 evidence bundle and prepare the next source-truth milestone.

**Requirements:** VERIFY-01, VERIFY-02, VERIFY-03

**Scope:**
- Run the canonical full verification command.
- Run `ViewModelSmokeTests.exe` explicitly or document why it remains built-only.
- Update traceability statuses and final evidence.
- Propose the next major milestone based on the stabilized baseline, likely PartPlate/AssembleView or Preset System Completion.

**Success criteria:**
1. Canonical verification passes after all v2.9 work.
2. Built-only tests are explicitly accounted for.
3. Each completed v2.9 requirement has source, test, or manual verification evidence.
4. The next milestone starts from a clean, aligned planning baseline.

## Coverage

| Phase | Name | Requirements | Count |
|---|---|---|---:|
| 10 | Planning Truth Reset | PLAN-01..PLAN-05 | 5 (Complete) |
| 11 | Source Hygiene Stabilization | HYGIENE-01..HYGIENE-04 | 4 |
| 12 | Calibration Closure for Implemented Modes | CAL-01..CAL-05 | 5 |
| 13 | Hybrid Integration Verification | INT-01..INT-06 | 6 |
| 14 | Visible Placeholder Triage | UI-01..UI-05 | 5 |
| 15 | Verification and Handoff | VERIFY-01..VERIFY-03 | 3 |

**Total requirements:** 28
**Mapped:** 28
**Unmapped:** 0

## Next Step

Start with:

```text
$gsd-discuss-phase 11
```

or, for a direct execution plan:

```text
$gsd-plan-phase 11
```

---

*Last updated: 2026-06-25 after Phase 10 completion.*
