---
phase: 15
phase_name: Verification and Handoff
plan_id: 15-01
title: Final v2.9 verification and handoff evidence
status: complete
wave: 1
type: verification
autonomous: true
requirements_addressed:
  - VERIFY-01
  - VERIFY-02
  - VERIFY-03
files_modified:
  - .planning/phases/15-verification-and-handoff/15-SUMMARY.md
  - .planning/phases/15-verification-and-handoff/15-VERIFICATION.md
  - .planning/phases/15-verification-and-handoff/15-REVIEW.md
  - .planning/INDEX.md
  - .planning/MILESTONES.md
  - .planning/REMAINING_MIGRATION_PLAN.md
  - .planning/REQUIREMENTS.md
  - .planning/ROADMAP.md
  - .planning/STATE.md
---

# Plan 15-01: Final v2.9 Verification and Handoff Evidence

<objective>
Run final verification after all v2.9 work, account for explicit smoke-test coverage, and update milestone traceability so the next source-truth milestone starts from an aligned baseline.
</objective>

<tasks>

1. Run explicit smoke/audit verification.
   - Command: `build\ViewModelSmokeTests.exe -o build\ViewModelSmokeTests.phase15.txt,txt`
   - Command: `build\QmlUiAuditTests.exe -o build\QmlUiAuditTests.phase15.txt,txt`
   - Acceptance criteria: both available test executables pass or any built-only status is documented with a concrete reason.

2. Run canonical full verification.
   - Command: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
   - Acceptance criteria: command exits 0 and reports build/startup/QML/E2E success.

3. Write final evidence.
   - Files: `15-SUMMARY.md`, `15-VERIFICATION.md`, `15-REVIEW.md`
   - Action: Link VERIFY-01..VERIFY-03 to the commands and evidence.
   - Action: Summarize residual manual verification needs for hardware/WebView/blocked dependencies.
   - Acceptance criteria: no requirement remains without evidence or explicit deferred/manual status.

4. Align milestone handoff documents.
   - Files: `.planning/INDEX.md`, `.planning/MILESTONES.md`, `.planning/REMAINING_MIGRATION_PLAN.md`, `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/STATE.md`
   - Action: Mark Phase 15 and v2.9 verification complete.
   - Action: Set the next recommended milestone to v3.0 PartPlate and AssembleView.
   - Acceptance criteria: planning entry files agree on current milestone completion and next work.

</tasks>

<verification>

- `build\ViewModelSmokeTests.exe -o build\ViewModelSmokeTests.phase15.txt,txt`
- `build\QmlUiAuditTests.exe -o build\QmlUiAuditTests.phase15.txt,txt`
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- `git diff --check`

</verification>

<success_criteria>

- VERIFY-01, VERIFY-02, and VERIFY-03 are complete.
- Phase 15 is marked complete in roadmap/state/requirements.
- v2.9 has final verification evidence and a next-milestone handoff.
- No product source files are changed in Phase 15.

</success_criteria>
