---
phase: 15
phase_name: Verification and Handoff
status: ready
created: 2026-06-25
autonomous: true
requirements:
  - VERIFY-01
  - VERIFY-02
  - VERIFY-03
---

# Phase 15 Context: Verification and Handoff

## Objective

Close milestone v2.9 by proving the stabilized baseline still builds and by making the final planning handoff match the real implementation state.

## Inputs

- Phase 10 reset the active planning entry files and restored missing `.Codex/rules/*` references.
- Phase 11 cleaned source hygiene issues and classified untracked implementation files.
- Phase 12 wired implemented calibration modes and added deterministic ViewModel coverage.
- Phase 13 added deterministic hybrid integration evidence for SSDP, MQTT, FTP, camera, software viewport, and settings persistence.
- Phase 14 removed or disabled targeted visible placeholder/no-op UI paths and added static QML audit coverage.

## Constraints

- Use the project canonical verification command:
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- Use the existing `build/` directory only.
- Do not introduce product behavior in Phase 15.
- Do not claim real-printer, WebView, OpenVDB, FFmpeg, or WebRTC coverage beyond the evidence already available.

## Known External Artifacts

The following untracked root files/directories remain outside this milestone's implementation scope:

- `.agents/`
- `AGENTS.md`
- `IMPLEMENTATION_SUMMARY.md`

## Handoff Target

The recommended next milestone is v3.0 PartPlate and AssembleView because the stabilized baseline still classifies PartPlate/AssembleView as Hybrid/Placeholder and it is the next large source-truth module in `REMAINING_MIGRATION_PLAN.md`.
