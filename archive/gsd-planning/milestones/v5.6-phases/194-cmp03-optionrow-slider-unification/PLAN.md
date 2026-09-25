# Phase 194 Plan: Cmp-03 OptionRow and Slider Unification

**Requirement:** UI-01
**Goal:** Promote the inline `NumericEdit`/`Badge` components in `OptionRow.qml`
into shared `Cx*` controls, and unify the parallel step-button idioms
(`MoveStepButton` in `MoveSlider.qml`, `RailButton` in `PreviewLayerRail.qml`)
into a single `CxStepButton.qml`. Also add `CxBusyIndicator.qml` for Phase 196.

## Files

- Create: `src/qml_gui/controls/CxBadge.qml`
- Create: `src/qml_gui/controls/CxNumericEdit.qml`
- Create: `src/qml_gui/controls/CxStepButton.qml`
- Create: `src/qml_gui/controls/CxBusyIndicator.qml`
- Modify: `src/qml_gui/components/OptionRow.qml` (remove inline Badge/NumericEdit;
  use the new Cx controls)
- Modify: `src/qml_gui/components/MoveSlider.qml` (remove inline MoveStepButton;
  use CxStepButton)
- Modify: `src/qml_gui/components/PreviewLayerRail.qml` (remove inline RailButton;
  use CxStepButton)

## Steps

- [x] Create `controls/CxBadge.qml` (extracted from OptionRow.qml:477-497).
- [x] Create `controls/CxNumericEdit.qml` (extracted from OptionRow.qml:499-530).
- [x] Create `controls/CxStepButton.qml` (unified from MoveSlider.qml:181-212
      and PreviewLayerRail.qml:357-393).
- [x] Create `controls/CxBusyIndicator.qml` (new, for Phase 196 Emboss spinner).
- [x] Refactor `OptionRow.qml`: replace inline `Badge`/`NumericEdit` with the
      new Cx controls; delete the inline component declarations.
- [x] Refactor `MoveSlider.qml`: replace inline `MoveStepButton` with
      `CxStepButton`; delete the inline component declaration.
- [x] Refactor `PreviewLayerRail.qml`: replace inline `RailButton` with
      `CxStepButton`; delete the inline component declaration.

## Verify

- [ ] `grep -rn "component Badge\|component NumericEdit\|component MoveStepButton\|component RailButton" src/qml_gui` returns no inline definitions.
- [ ] Canonical build exits 0.
- [ ] QmlUiAuditTests PASS.
- [ ] ViewModelSmokeTests PASS.
- [ ] E2E workflow PASS.
- [ ] `OWzxSlicer.exe` launch liveness confirmed.
