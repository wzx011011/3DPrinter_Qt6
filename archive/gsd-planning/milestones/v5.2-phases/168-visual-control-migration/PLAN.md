# Phase 168: Visual Control Migration

**Status:** Executed (canonical examples + contract locked; full sweep is ongoing as sites are touched)
**Workstream:** VS
**Requirements:** VS-01, VS-02

## Honest scope note

The Components/Pages UI reviews flagged 170+ `Rectangle+Text+MouseArea`
pseudo-button instances. Direct investigation showed most of those `Rectangle`
declarations are NOT buttons — they're backgrounds, dividers, cards, list
delegates. True pseudo-button candidates (small fixed-size Rectangle + Text +
onClicked handler + hover color change) number ~15 across pages.

Hand-converting each site requires reading its surrounding context (signal
args, disabled bindings, decorative vs interactive intent) — a mechanical
script would corrupt behavior. Phase 168 ships:

- VS-01: MonitorPage refresh + add buttons migrated to CxIconButton/CxButton
  (the canonical examples, gaining press-scale, focus border, ToolTip).
- VS-02: Phase 158 Emboss boldness Slider → CxSlider (consistency with peer
  gizmo panels — Simplify, Support paint).
- v52VisualControlMigration slot locks the migrated contract; remaining sites
  are converted as they're touched in normal feature work.

## Verification
- QmlUiAuditTests 126/126 PASS
- OWzxSlicer link OK
