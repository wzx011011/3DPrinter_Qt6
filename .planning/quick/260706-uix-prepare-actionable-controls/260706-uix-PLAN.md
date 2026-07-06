---
quick_id: 260706-uix
slug: prepare-actionable-controls
status: complete
date: 2026-07-06
---

# Quick Task 260706-uix: Prepare actionable controls

## Goal

Make the Prepare page controls restored in the visual parity pass honest and actionable. Visible buttons must either execute a real Qt/backend action or be state-disabled by a real backend gate with a clear tooltip.

## Scope

- Prepare top chrome slice/export buttons.
- Prepare viewport top action toolbar.
- Existing QML UI audit coverage for these controls.

## Acceptance

- No visible GL toolbar button remains as a pure screenshot-density placeholder.
- The Prepare top "slice single plate" button directly requests current-plate slicing instead of opening a menu.
- The Prepare top export button is disabled unless `EditorViewModel::canExportGCode` is true.
- Focused QML audit passes.
- Encoding guard and `git diff --check` pass.
- Canonical verifier passes before completion.

## Result

- Removed screenshot-density placeholder buttons from the Prepare viewport top action toolbar.
- Wired restored action buttons to real `EditorViewModel` actions: delete, copy, paste, mirror, center, repair, and object settings.
- Changed the Prepare top "slice single plate" button to call `EditorViewModel::requestSlice()` directly through a dedicated topbar signal.
- Bound the Prepare top export button to `EditorViewModel::canExportGCode` and backend readiness tooltip text.
- Gated high-risk gizmos that currently cannot complete with available backend support:
  - Mesh Boolean and Drill stay disabled while CGAL MeshBoolean is unavailable.
  - Support Paint and Seam Paint stay disabled until viewport triangle picking is wired.

## Verification

- `build\QmlUiAuditTests.exe prepareRestoredControlsAreActionable -o build\qml-uix-actionable.txt,txt` passed before the full rebuild.
- `E:\Qt6.10\bin\qmllint.exe src\qml_gui\BBLTopbar.qml src\qml_gui\components\GLToolbars.qml src\qml_gui\main.qml` passed with existing warnings.
- `python %USERPROFILE%\.coding-encoding-guard\encoding_guard.py` passed.
- `git diff --check` passed.
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` passed.
- `build\OWzxSlicer.exe` launched and responded as `OWzx Slicer`; Windows screenshot capture failed with `SetIsBorderRequired failed: 不支持此接口 (0x80004002)`, so no screenshot artifact was recorded from Computer Use.

## Commit

- `f950545 fix(prepare): make restored controls actionable`
