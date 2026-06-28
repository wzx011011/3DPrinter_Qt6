---
phase: 36
plan: 01
type: verification
wave: 1
depends_on: []
files_modified:
  - .planning/phases/36-verification-and-handoff/36-SUMMARY.md
  - .planning/phases/36-verification-and-handoff/36-VERIFICATION.md
  - .planning/ROADMAP.md
  - .planning/REQUIREMENTS.md
  - .planning/STATE.md
autonomous: true
requirements:
  - TEST-01
  - TEST-02
  - TEST-03
---

# Verification and Handoff Plan

<objective>
Close v3.3 by producing deterministic verification evidence, reviewing the implemented main flow, launching the app for manual UAT, and updating planning/backlog state.
</objective>

<tasks>

## Task 1: Final automated verification

**Files:** `.planning/phases/36-verification-and-handoff/36-VERIFICATION.md`

**Action:**
- Stop any existing `OWzxSlicer` process.
- Run `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- Record suite outcomes and the latest QRhi backend selection log.
- Stop the smoke-test app process if the verifier leaves one running.

**Verify:**
- Canonical command exits 0.
- PrepareScene, PartPlate, QML UI audit, and E2E pipeline tests pass.

## Task 2: Review v3.3 main-flow code path

**Files:** `.planning/phases/36-verification-and-handoff/36-SUMMARY.md`

**Action:**
- Review Phase 33-35 commits and changed files for:
  - slice completion -> Preview navigation,
  - G-code parser edge cases,
  - travel/color payload repacking,
  - RHI Preview draw-range logic,
  - accidental normal-path `SoftwareViewport` fallback.
- Fix any blocking issue found during review.
- Record non-blocking residual risk explicitly.

**Verify:**
- Any fix runs the canonical command again.
- If no fix is needed, summary documents the review outcome.

## Task 3: Launch and handoff for manual UAT

**Files:** `.planning/ROADMAP.md`, `.planning/REQUIREMENTS.md`, `.planning/STATE.md`, `.planning/phases/36-verification-and-handoff/36-SUMMARY.md`

**Action:**
- Launch `build/OWzxSlicer.exe` after verification passes.
- Provide the user a concise UAT path:
  - load a model,
  - slice from Prepare,
  - confirm automatic Preview entry,
  - confirm non-empty Preview rendering,
  - exercise layer/move sliders, travel toggle, and color mode.
- Update roadmap/requirements/state to mark v3.3 complete or ready for final milestone audit.

**Verify:**
- The app process is running and responding after launch.
- Handoff notes name the exact executable and current known deferred backlog.

</tasks>

<verification>
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- Code review of v3.3 changed files.
- Runtime launch of `build/OWzxSlicer.exe` for user UAT.
</verification>

<success_criteria>
- Final canonical verification exits 0.
- v3.3 requirements have evidence or explicit deferral.
- No unintended active process remains except the intentionally launched app for user UAT.
- User can test the main load -> slice -> Preview path in the running app.
</success_criteria>
