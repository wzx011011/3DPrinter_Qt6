# Phase 162: Color Hardcode Sweep

**Status:** Executed (script-based mechanical sweep)
**Workstream:** TK (Token Migration)
**Requirement:** TK-01

## Approach

Built hex→token map from Theme.qml. Swept all `*.qml` under `src/qml_gui/`
(excluding Theme.qml itself) replacing every quoted `"#rrggbb"` literal with
the nearest Theme token within a perceptual distance threshold. Skip-listed
intentional status colors (#ff0000 red, #8B5CF6 purple, etc.) and AARRGGBG
alpha-prefixed translucents.

## Result

- **695 hex literals → Theme tokens** across 37 files
- 29 intentional status/special colors preserved
- Worst offender PreferencesPage.qml: 129 → 0 (now Theme-token-driven)
- LeftSidebar private palette (6 colors) → Theme tokens
- PresetDiffDialog status badges → Theme.status*

## Verification
- QmlUiAuditTests 120/120 PASS
- OWzxSlicer link OK
