# Plan / Implementation Alignment Audit

Date: 2026-06-24  
Purpose: re-audit all active planning artifacts against real implementation, then define the next milestone from code evidence rather than stale roadmap labels.

## Summary

The planning tree and implementation had drifted:

- `.planning/PROJECT.md`, `STATE.md`, and `ROADMAP.md` still identified v2.6 as current.
- Git history already contained v2.7 and v2.8 work on `main`.
- Current local changes include additional implementation work around calibration, slicing, app settings, software viewport fallback, and QML startup behavior.
- Some active files contain encoding damage or literal escape artifacts.
- Several broad UI surfaces exist visually but are disabled, placeholder, or silent no-op workflows.

The correct next milestone is therefore **v2.9: Implementation Realignment and Stabilization**, not a stale v2.7 continuation.

## Verification Performed

Canonical verification command:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Observed result:

- Build completed successfully.
- `OWzxSlicer.exe` startup smoke passed.
- E2E pipeline tests passed.
- `ViewModelSmokeTests.exe` was built but not run by the script.

## Git Evidence

Recent mainline history includes:

- `567b01c fix(review,v2.8): 2 critical + 2 warning bugs from code review`
- `6ab72e3 feat(ftp,v2.8 P2-C): send print job via FTPS upload + MQTT print command`
- `ca6bced feat(mqtt,v2.7 P2-B): print control commands via MQTT publish`
- `227ad43 feat(mqtt,v2.7 P2-A): real MQTT connect + live telemetry + access-code UI`
- `0ed40f3 feat(calib,v2.7 P1): Calibration generates real PA/Flow/Temp G-code via slice pipeline`
- `4a5898b fix(slice,v2.7 P0): bed_shape + loadFinished wait - slice E2E pipeline now works`

These commits mean v2.7/v2.8 cannot be treated as future planning scope.

## Planning Findings

### P0: Active milestone was stale

Planning said v2.6 was current and complete, while code history had already landed v2.7/v2.8 work.

Action taken:

- Rewrote `PROJECT.md`, `STATE.md`, `REQUIREMENTS.md`, `ROADMAP.md`, `INDEX.md`, `MILESTONES.md`, and `REMAINING_MIGRATION_PLAN.md` around v2.9.

### P0: Completion language was ambiguous

Several areas are phase-complete but not product-complete. Real paths exist in services that still include fallback/mock behavior.

Action taken:

- Added Real/Hybrid/Mock/Blocked/Placeholder status terms.
- Added a current baseline classification matrix to `REQUIREMENTS.md`.

### P1: Missing rule references

AGENTS references `.Codex/rules/source-truth-migration.md` and `.Codex/rules/build-rules.md`, but those files are missing in this checkout.

Action required in v2.9:

- Restore the rule files or redirect AGENTS to existing canonical rules.

## Implementation Findings

### P0: Dirty baseline needs classification

Current worktree includes modified implementation files and untracked files such as `AppSettingsService.*`, `SoftwareViewport.*`, `.planning/INDEX.md`, audits, and `SliceService.cpp.backup`.

Action required in v2.9:

- Classify each dirty/untracked file as intentional implementation, planning artifact, external artifact, or cleanup candidate.

### P0: Behavior-affecting source hygiene issue

`src/core/services/CalibrationServiceMock.cpp` contains literal `\r\n` text near fallback timer logic. Because the text appears in a `//` comment line, it may comment out intended fallback code.

Action required in v2.9:

- Convert literal escapes to real line breaks and verify real-mode vs mock-mode calibration progress behavior.

### P1: Encoding damage

Several active files and planning files contained mojibake or replacement characters. Active source comments and user-visible strings must be repaired before counting affected workflows complete.

Action required in v2.9:

- Repair active source comments/strings or explicitly track the remaining damage.

### P1: Calibration is hybrid, not deferred

PA, Flow Rate, and Temp Tower paths now dispatch through `SliceService`, but topbar calibration entries remain disabled and deterministic tests are incomplete.

Action required in v2.9:

- Wire implemented calibration modes to visible UI and add deterministic regression coverage.

### P1: Hybrid integrations need deterministic verification

SSDP, MQTT, FTP, and camera code paths exist, but tests still lean on fallback/no-crash behavior or environment-dependent paths.

Action required in v2.9:

- Add deterministic protocol fixtures and document live-hardware requirements separately.

### P1: Visible placeholders remain

Examples:

- `main.qml` export project/model/preferences handlers are TODO/no-op.
- `BBLTopbar.qml` calibration menu entries are disabled.
- `Plater.qml` AssembleView is a placeholder.
- `ModelMallViewModel::webViewAvailable()` returns false.
- Preset bundle import/export is simplified JSON, not upstream-compatible bundle behavior.
- Plate-scoped config in `ProjectServiceMock` still contains TODO/fallback behavior.

Action required in v2.9:

- Wire feasible top-level actions or classify them as Placeholder/Blocked/Future in planning.

## New Milestone Decision

The active milestone is:

**v2.9 - Implementation Realignment and Stabilization**

This milestone is intentionally scoped before PartPlate, Preset System, WebView, or other large migrations. The project needs a clean, evidence-backed baseline first.

## Files Updated

- `.planning/PROJECT.md`
- `.planning/STATE.md`
- `.planning/REQUIREMENTS.md`
- `.planning/ROADMAP.md`
- `.planning/INDEX.md`
- `.planning/MILESTONES.md`
- `.planning/REMAINING_MIGRATION_PLAN.md`
- `.planning/audits/2026-06-24-plan-implementation-alignment.md`

## Next Step

Start Phase 10:

```text
$gsd-discuss-phase 10
```

or:

```text
$gsd-plan-phase 10
```
