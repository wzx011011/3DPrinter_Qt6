---
phase: 14
phase_name: Visible Placeholder Triage
status: reviewed
reviewed: 2026-06-25
scope:
  - src/qml_gui/pages/ModelMallPage.qml
  - src/qml_gui/pages/PreferencesPage.qml
  - src/qml_gui/panels/LeftSidebar.qml
  - tests/QmlUiAuditTests.cpp
---

# Phase 14 Code Review

## Findings

No open blocking findings remain.

## Fixed During Review

- `src/qml_gui/pages/PreferencesPage.qml`: removed the empty update-check click handler and marked the control disabled.
- `src/qml_gui/panels/LeftSidebar.qml`: hid advanced/compare/object-table placeholder controls and removed runtime TODO markers for unavailable layer/parameter sections.
- `src/qml_gui/pages/ModelMallPage.qml`: disabled publish navigation and replaced fake marketplace copy with unavailable/local-preview copy.
- `tests/QmlUiAuditTests.cpp`: added a regression test that rejects empty handlers, fake ModelMall copy, and runtime sidebar placeholder markers.

## Review Notes

- Phase 14 intentionally did not implement full ModelMall, cloud account, AssembleView, variable layer editor, or preset diff workflows.
- Existing export/preferences/calibration paths were already wired and were preserved.
- The QML audit is static but is run by the canonical verification script, so the contract is covered in the normal full verification path.

## Residual Risk

- Some active QML files still contain historical mojibake comments outside the visible Phase 14 runtime behavior path.
- ModelMall remains a blocked/preview page rather than a real WebView-backed marketplace.
- The Preferences update service remains unavailable until a real update backend is introduced.
