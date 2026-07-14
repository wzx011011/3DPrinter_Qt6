# Phase 119 Summary: Tick Type Coverage And Drag Relocation

**Phase:** 119 — Tick Type Coverage And Drag Relocation (WS1, TICK-04 + TICK-05)
**Status:** Complete
**Date:** 2026-07-15

## What Shipped
Closed TICK-04 + TICK-05 (WS1 final phase). All 5 upstream tick types (PausePrint/CustomGcode/Template/ToolChange/ColorChange) are now addable from the UI and round-trip via the Phase 118 write-back loop. Ticks are drag-relocatable on the Preview rail (matching upstream IMSlider).

## Changes
| File | Change |
|---|---|
| `PreviewViewModel.h` | Added addColorChangeAtLayer/addTemplateAtLayer/moveTick Q_INVOKABLE decls. |
| `PreviewViewModel.cpp` | Implemented the 3 methods following Phase 118 CRUD pattern (dedup → append → sort → emit → writeTicksToModel). moveTick returns bool (false on missing source/occupied target). |
| `PreviewLayerRail.qml` | Tick delegate drag handler (vertical, follows cursor, computes target layer on release, calls moveTick, snap-back on false). sliderAddMenu gains "Add Color Change" + "Add Template" items. |
| `QmlUiAuditTests.cpp` | Added tickTypeCoverageAndDragRelocation source-audit slot. |

## Verification
- Canonical build (j6): exit 0; all targets linked.
- All 5 ctest groups passed (PrepareScene/PartPlate/ViewModel/QmlUiAudit/PreviewParser).
- APP_RUNNING_PID=15588 (app launched live).
- QmlUiAuditTests incl. new tickTypeCoverageAndDragRelocation slot: passed.

## Carry-Forward
- WS1 complete (Phases 117-119). ColorChange uses default extruder 1 + #FF0000; a dedicated color/extruder picker is a future UI enhancement.
