# Phase 167: Component Coherence

**Status:** Executed
**Workstream:** Cmp
**Requirements:** Cmp-01, Cmp-02 (Cmp-03 deferred — OptionRow promotion + MoveSlider/PreviewLayerRail unification are deeper refactors)

## Result

- Cmp-01: NotificationCenter's private 9-level severity→color and severity→icon
  switch tables collapsed to lookups against Theme.severityColors /
  Theme.severityIcons (Phase 160 canonical palettes). Single source of truth
  for the notification system (was duplicated across ErrorBanner/ErrorToast/
  NotificationCenter per Components-UI-REVIEW). The Phase 162 color sweep had
  already migrated ErrorBanner/ErrorToast to Theme tokens; Phase 167 finishes
  the consolidation at the table level.
- Cmp-02: 4 orphan components removed from the qrc bundle (CxPanel,
  CxSectionHeader, FilamentSlot, GroupNavSidebar — confirmed zero QML
  consumers per Components-UI-REVIEW). Source files kept on disk for
  potential future use; just no longer compiled into the binary.

Cmp-03 deferred — OptionRow inline NumericEdit/Badge promotion and the
MoveSlider/PreviewLayerRail step-button idiom unification are deeper refactors
that warrant their own focused phase, not a sweep.

## Verification
- QmlUiAuditTests 125/125 PASS
- OWzxSlicer link OK
