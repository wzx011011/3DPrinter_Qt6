---
phase: 84
slug: settings-source-truth-gap-audit
status: approved
nyquist_compliant: true
wave_0_complete: true
created: 2026-07-07
---

# Phase 84 - Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|---|---|
| **Framework** | Documentation/source audit |
| **Config file** | none |
| **Quick run command** | `git diff --check` |
| **Full suite command** | `python %USERPROFILE%\.coding-encoding-guard\encoding_guard.py .planning/phases/84-settings-source-truth-gap-audit/84-GAP-MATRIX.md .planning/phases/84-settings-source-truth-gap-audit/84-VERIFICATION.md .planning/phases/84-settings-source-truth-gap-audit/84-01-SUMMARY.md` |
| **Estimated runtime** | ~10 seconds |

---

## Sampling Rate

- **After every task commit:** Run `git diff --check`.
- **After every plan wave:** Run the encoding guard on Phase 84 artifacts.
- **Before phase verification:** Confirm the gap matrix covers region IDs,
  source anchors, Qt targets, Phase 56 residuals, requirements, owners, and
  verification methods.
- **Max feedback latency:** 10 seconds.

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---|---|---|---|---|---|---|---|---|
| 84-01-01 | 01 | 1 | SETAUDIT-01 | N/A | N/A | source/doc | `rg -n "SET-SHELL|SET-PRESET-ACTIONS|SET-TABS|SET-SECTIONS|SET-TYPED-ROWS" .planning/phases/84-settings-source-truth-gap-audit/84-GAP-MATRIX.md` | green after execution | green |
| 84-01-02 | 01 | 1 | SETAUDIT-01 | N/A | N/A | source/doc | `rg -n "SettingsDialog.qml|OptionRow.qml|GroupNavSidebar.qml|Tab.cpp|PresetComboBoxes.cpp|PrintConfig.cpp" .planning/phases/84-settings-source-truth-gap-audit/84-GAP-MATRIX.md` | green after execution | green |
| 84-01-03 | 01 | 1 | SETAUDIT-02 | N/A | N/A | source/doc | `rg -n "Phase 56|visual parity|typed-control rendering|cross-window live edit|Phase 85|Phase 86|Phase 87|Phase 88" .planning/phases/84-settings-source-truth-gap-audit/84-GAP-MATRIX.md` | green after execution | green |
| 84-01-04 | 01 | 1 | SETAUDIT-01, SETAUDIT-02 | N/A | N/A | source/doc | `git diff --check` | green after execution | green |

---

## Wave 0 Requirements

Existing infrastructure covers all Phase 84 requirements. No test harness or
fixture creation is needed because the phase is documentation/source audit only.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|---|---|---|---|
| Final printer/material/process runtime visual evidence | SETVERIFY-02 | Belongs to Phase 88 after implementation phases land | Phase 84 must route this to Phase 88; do not claim it here. |

---

## Validation Sign-Off

- [x] All tasks have automated verify commands or clear document assertions.
- [x] Sampling continuity: no 3 consecutive tasks without automated verify.
- [x] Wave 0 covers all missing references.
- [x] No watch-mode flags.
- [x] Feedback latency < 10s.
- [x] `nyquist_compliant: true` set in frontmatter.

**Approval:** approved 2026-07-07
