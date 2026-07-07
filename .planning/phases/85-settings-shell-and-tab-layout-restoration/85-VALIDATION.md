# Phase 85 Validation: Settings Shell And Tab Layout Restoration

**Date:** 2026-07-07
**Validation model:** Nyquist-style source/behavior sampling for a UI shell
phase; final runtime screenshots are deferred to Phase 88 by milestone design.

## Requirement Sampling

| Requirement | Phase 85 Evidence |
|---|---|
| `SETLAYOUT-01` | `SettingsDialog.qml` remains a 736x593 non-modal `ApplicationWindow`; `main.qml` keeps three dialog instances; `LeftSidebar.qml` and existing smoke tests keep dispatch through `forwardSettingsRequest`. |
| `SETLAYOUT-02` | Printer/material titles and tab labels are restored to screenshot text/order; process reuses the same shell without a separate visual system. |
| `SETLAYOUT-03` | Source audits reject mojibake labels, raw internal strings in the shell, text-heavy save buttons, duplicate in-row close, and visible `GroupNavSidebar` usage. |

## Static Verification

- `tests/QmlUiAuditTests.cpp` must assert the restored shell contract:
  - clean Chinese titles and tab labels exist;
  - old mojibake title/tab substrings are absent from `SettingsDialog.qml`;
  - no visible `GroupNavSidebar` instance is used in `SettingsDialog.qml`;
  - no `filterIndicesByGroup`/`selectedGroup` shell dependency remains in the
    dialog;
  - no top-row `Save` / `Save As...` text buttons remain;
  - no manual in-row close `MouseArea` remains;
  - compact icon buttons, search reveal, advanced toggle, and save/save-as
    actions remain wired.
- `settingsDialogMainQmlDispatchStructural()` continues to prove `main.qml`
  dispatches all three dialog instances.
- Existing `ViewModelSmokeTests::testSettingsDialogOpenFromSidebar()` continues
  to prove the backend signal path and active preset tier update.

## Command Verification

Phase 85 should run:

```powershell
python $env:USERPROFILE\.coding-encoding-guard\encoding_guard.py <changed files>
git diff --check
```

If C++ audit tests are edited, run the built test binary when available or run
the canonical build command if a fresh binary/configure is required:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

## Deferred Runtime Validation

Phase 88 owns:

- app launch proof;
- printer/material/process dialog runtime interaction capture;
- final screenshot comparison against `shotScreen/打印机参数设置页.png` and
  `shotScreen/材料参数设置页.png`.
