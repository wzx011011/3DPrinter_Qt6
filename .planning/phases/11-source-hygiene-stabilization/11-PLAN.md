---
phase: 11
phase_name: Source Hygiene Stabilization
plan_id: 11-01
title: Repair source hygiene and classify dirty baseline files
status: ready
wave: 1
type: standard
autonomous: true
requirements_addressed:
  - HYGIENE-01
  - HYGIENE-02
  - HYGIENE-03
  - HYGIENE-04
files_modified:
  - src/core/services/CalibrationServiceMock.cpp
  - src/core/services/SliceService.cpp
  - src/core/services/SliceService.h
  - src/core/services/SliceService.cpp.backup
  - .planning/phases/11-source-hygiene-stabilization/11-SUMMARY.md
  - .planning/phases/11-source-hygiene-stabilization/11-VERIFICATION.md
  - .planning/phases/11-source-hygiene-stabilization/11-REVIEW.md
  - .planning/STATE.md
  - .planning/ROADMAP.md
  - .planning/REQUIREMENTS.md
---

# Plan 11-01: Repair source hygiene and classify dirty baseline files

<objective>
Make the active dirty baseline readable and unambiguous without reverting unrelated implementation work. Fix behavior-affecting literal escape damage, repair deterministically recoverable encoding damage in active source files, remove the unreferenced source-adjacent backup file, and classify untracked implementation files.
</objective>

<tasks>

1. Fix the `CalibrationServiceMock.cpp` fallback timer artifact.
   - Files: `src/core/services/CalibrationServiceMock.cpp`
   - Action: Replace literal `\r\n` text with real line breaks so the mock timer branch executes only when no `SliceService` is installed.
   - Verify: `rg -n --fixed-strings '\r\n' src/core/services/CalibrationServiceMock.cpp` returns no matches.
   - Acceptance criteria: HYGIENE-01 is satisfied for the known behavior-affecting artifact.

2. Repair encoding-damaged `SliceService` comments and user-visible strings.
   - Files: `src/core/services/SliceService.cpp`, `src/core/services/SliceService.h`
   - Action: Replace unreadable `QObject::tr()` / `QStringLiteral()` text with clear Chinese labels and repair comments whose intent is clear from surrounding code and Phase 10 evidence.
   - Verify: targeted scan for damaged tokens in the repaired files returns no matches.
   - Acceptance criteria: HYGIENE-02 is satisfied for the active `SliceService` damage identified by Phase 11 research.

3. Remove the unreferenced backup source artifact.
   - Files: `src/core/services/SliceService.cpp.backup`
   - Action: Confirm no build/source references exist, then remove it from active `src/`.
   - Verify: `Test-Path src/core/services/SliceService.cpp.backup` returns false.
   - Acceptance criteria: HYGIENE-03 is satisfied for the known backup file.

4. Classify untracked baseline implementation files.
   - Files: `.planning/phases/11-source-hygiene-stabilization/11-SUMMARY.md`, `.planning/STATE.md`
   - Action: Record that `AppSettingsService.*`, `SoftwareViewport.*`, and `QmlUiAuditTests.cpp` are intentional implementation/test files because they are referenced by CMake or active source; leave unrelated untracked docs/agent folders untouched.
   - Verify: summary and state include classification evidence.
   - Acceptance criteria: HYGIENE-04 is satisfied without reverting or deleting intentional dirty work.

5. Verify and close Phase 11.
   - Files: `.planning/phases/11-source-hygiene-stabilization/11-VERIFICATION.md`, `.planning/REQUIREMENTS.md`, `.planning/ROADMAP.md`, `.planning/STATE.md`
   - Action: Run targeted scans, attempt canonical verification, write evidence, mark HYGIENE requirements complete, and commit only Phase 11 files plus intended source hygiene edits.
   - Verify: `git diff --check` passes for touched files; canonical command result is recorded.
   - Acceptance criteria: Phase 11 has SUMMARY, VERIFICATION, REVIEW, requirements traceability, and roadmap/state updates.

</tasks>

<verification>

- `rg -n --fixed-strings '\r\n' src/core/services/CalibrationServiceMock.cpp src/core/services/SliceService.cpp src/core/services/SliceService.h`
- `rg -n "鈥|鈫|鉁|馃|�|Ã|Â|涓|鍙|鏂|璺|鎴|鏈|ä|å|æ|锟" src/core/services/CalibrationServiceMock.cpp src/core/services/SliceService.cpp src/core/services/SliceService.h`
- `Test-Path src/core/services/SliceService.cpp.backup`
- `rg -n "SliceService\.cpp\.backup|AppSettingsService|SoftwareViewport|QmlUiAuditTests" CMakeLists.txt src tests .planning -S --glob '!build/**'`
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- `git diff --check -- src/core/services/CalibrationServiceMock.cpp src/core/services/SliceService.cpp src/core/services/SliceService.h .planning/REQUIREMENTS.md .planning/ROADMAP.md .planning/STATE.md .planning/phases/11-source-hygiene-stabilization`

</verification>

<success_criteria>

- HYGIENE-01 through HYGIENE-04 are all covered by this plan.
- Behavior-affecting literal escape damage is removed from active source.
- Repaired source strings are readable and preserve existing slicing/calibration intent.
- The backup source artifact is no longer present under `src/`.
- Intentional untracked implementation files are classified rather than silently discarded.
- Verification evidence is recorded even if the canonical build is blocked by pre-existing dirty baseline issues.

</success_criteria>

## PLANNING COMPLETE
