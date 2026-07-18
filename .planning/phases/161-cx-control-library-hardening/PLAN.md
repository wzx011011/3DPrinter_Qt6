# Phase 161: Cx* Control Library Hardening

**Status:** Ready to execute
**Workstream:** DS
**Requirements:** DS-02, DS-03

## Goal

Harden the Cx* primitives as the design-system carrier. Audit found the library
in better shape than expected — only 3 real defects to fix + targeted state
backfills. Most Cx* controls already use Theme tokens (zero hex literals,
fontSize tokens throughout).

## Evidence-based scope (much narrower than audit suggested)

From direct grep of src/qml_gui/controls/:
- **Zero hardcoded hex literals** in Cx* controls (audit was wrong about this)
- **fontSize tokens already used** in all 16 controls (audit was wrong about this)
- **3 real defects**:
  1. CxButton.qml:31-32 — `Qt.darker(Theme.statusError, 1.5/1.1)` × 2 (Danger variant)
  2. CxIconButton.qml:48 — `Qt.darker(Theme.accentSubtle, 1.1)` (selected pressed)
  3. CxSpinBox.qml:63,72 — `fontSizeXS - 2` = 8px below XS=10 floor (▲/▼ spinner arrows)
- **State backfills** (real but lighter):
  - CxButton has no press-scale (CxIconButton does) — add subtle 0.96
  - CxButton/CxPillAction have no focus border
  - CxButton has no ToolTip support (CxIconButton does)
- **Disabled pattern** — actually consistent (opacity 0.45 is universal); audit overstated

## Plan

### Wave 1 — Fix the 3 real defects

1. `src/qml_gui/controls/CxButton.qml`:
   - Replace `Qt.darker(Theme.statusError, 1.5)` → `Theme.statusErrorDark`
   - Replace `Qt.darker(Theme.statusError, 1.1)` → `Theme.statusErrorPressed`
   - Add press-scale `scale: pressed ? 0.96 : 1.0` with Behavior
   - Add `property string toolTipText` + ToolTip binding (mirror CxIconButton)
   - Add focus border on `activeFocus` for accessibility
2. `src/qml_gui/controls/CxIconButton.qml`:
   - Replace `Qt.darker(Theme.accentSubtle, 1.1)` → `Theme.accentSubtlePressed`
3. `src/qml_gui/controls/CxSpinBox.qml`:
   - Replace `Theme.fontSizeXS - 2` → `Theme.fontSizeXS` (10px, the floor)
4. `src/qml_gui/controls/CxPillAction.qml`:
   - Add focus border + press-scale for parity with CxButton

### Wave 2 — Regression anchor

5. `tests/QmlUiAuditTests.cpp`: add `v52ControlLibraryHardened` slot asserting:
   - No `Qt.darker` / `Qt.lighter` in src/qml_gui/controls/
   - No `fontSizeXS - 2` (or any font size below XS floor) in CxSpinBox
   - CxButton has press-scale + toolTipText + focus border
   - All Cx* controls have disabled-state coverage

## Verification

- Canonical build exits 0
- 5/5 ctest groups PASS
- New `v52ControlLibraryHardened` slot passes
