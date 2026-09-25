# UI Design Contract — Phase 117: IMSlider Integration And Tick Rendering

**Phase:** 117 — IMSlider Integration And Tick Rendering
**Created:** 2026-07-15
**Mode:** Inline (integration phase — design space locked by upstream IMSlider + existing LayerSlider.qml)
**Status:** APPROVED (design contract is upstream-mandated, not free design)

## Design Philosophy

This phase is a **source-truth integration**, not a free-design phase. The visual and interaction contract is locked by two deterministic sources:

1. **Upstream truth:** `third_party/OrcaSlicer/src/slic3r/GUI/IMSlider.cpp` / `.hpp` — the canonical OrcaSlicer layer slider with tick marks, right-click menus, and color coding.
2. **Existing implementation:** `src/qml_gui/components/LayerSlider.qml` — a functionally-complete Qt6 port of the IMSlider that is currently orphaned (never instantiated). It already implements tick rendering, color coding, right-click add/edit/delete menus, and the CustomGcodeDialog instantiation.

**The design work is therefore: wire the existing component into `PreviewPage`, do NOT redesign it.** Any deviation from the existing `LayerSlider.qml` behavior requires an explicit upstream mapping.

## Visual Contract

### Tick Mark Color Coding (locked by upstream + existing LayerSlider.qml:81-124)

| Tick Type | Enum (`TickCodeTypes.h:8-12`) | Color | Upstream Anchor |
|---|---|---|---|
| PausePrint | `PausePrint=0` | Orange | `IMSlider` pause tick |
| ToolChange (filament change) | `ToolChange=3` | Blue | `IMSlider` tool-change tick |
| ColorChange | `ColorChange=4` | Green | `IMSlider` color-change tick |
| CustomGcode | `CustomGcode=1` | Teal | `IMSlider` custom tick |
| Template | `Template=2` | (existing default) | `IMSlider` template tick |

These colors are already implemented in `LayerSlider.qml:81-124` (Repeater over `previewVm.tickMarks` with per-type color). The integration must preserve them exactly — no re-picking colors.

### Tick Position

- Tick marks render at the layer position on the slider rail corresponding to their `tick` field (layer number) in `TickCode` (`TickCodeTypes.h:15-24`).
- Position-to-pixel mapping is already handled by `LayerSlider.qml`'s Repeater + the slider's `from`/`to` range. Preserve it.

### Slider Geometry

- The right-side layer rail (`PreviewLayerRail.qml` today) and/or the bottom move slider (`MoveSlider.qml`) are the integration targets.
- Upstream IMSlider is a single dual-thumb slider on the right side. The existing `LayerSlider.qml` mirrors this. Decide during planning whether to:
  - **(A) Replace `PreviewLayerRail.qml` with `LayerSlider.qml`** directly (fewest orphaned files), or
  - **(B) Consolidate** LayerSlider's tick rendering into the current slider set.
- Prefer **(A)** — it removes the orphan and leaves zero dead components (No-Deprecated-UI rule).

## Interaction Contract (locked by upstream + existing LayerSlider.qml)

### Right-Click on Empty Track Area (LayerSlider.qml:531-557)

Context menu appears with:
- "Add Pause" — adds a PausePrint tick at the clicked layer
- "Add Custom G-code..." — opens `CustomGcodeDialog.qml` in add mode

### Right-Click on Existing Tick (LayerSlider.qml:561-624)

Context menu appears with:
- "Edit" — opens `CustomGcodeDialog.qml` in edit mode (for CustomGcode/ColorChange/ToolChange ticks)
- "Delete" — removes the tick via `previewVm.removeTickAtLayer`

### CustomGcodeDialog (CustomGcodeDialog.qml:55-62)

- Already functional: calls `previewVm.editCustomGcodeAtLayer(...)` (edit mode) or `previewVm.addCustomGcodeAtLayer(...)` (add mode) on confirm.
- **NOTE for Phase 117 scope:** these ViewModel methods currently mutate `tickMarks_` in memory only — they do NOT write back to `custom_gcode_per_print_z` (that is Phase 118's scope). Phase 117 wires the UI so the menus/dialog are reachable; the in-memory mutation is the correct Phase-117 behavior. The closed loop lands in Phase 118.

## Out of Scope (explicit)

- **Write-back to `custom_gcode_per_print_z`** → Phase 118.
- **Re-slice on tick edit** → Phase 118.
- **Drag-to-relocate ticks (`moveTick`)** → Phase 119.
- **Template type full UI** → Phase 119 (round-tripped at data level if feasible).
- **New tick types beyond the 5 upstream** → never (source-truth rule).

## Design Decisions Locked

1. **Color palette:** per-type colors from existing `LayerSlider.qml` (Orange/Blue/Green/Teal) — upstream-mandated, not a free choice.
2. **Component strategy:** wire in the existing `LayerSlider.qml` (option A) to avoid orphaned components; remove `PreviewLayerRail.qml` if it becomes unused.
3. **No new styling:** this phase adds zero new color tokens, spacing values, or typography. It reuses the existing slider component's styling verbatim.
4. **Menu copy:** reuse the existing menu text from `LayerSlider.qml:531-624` (already English/ASCII).

## Verification (visual)

- A sliced G-code with `;...COLOR_CHANGE` / `PAUSE_PRINT` / `CUSTOM_GCODE` / `MANUAL_TOOL_CHANGE` comments must show colored ticks at the correct layers in the running Preview.
- Right-click on the rail's empty area shows the Add menu; right-click on a tick shows Edit/Delete.
- No empty/broken slider remains (the old tick-less `PreviewLayerRail` is replaced or gone).

---
*UI-SPEC created inline for Phase 117 — integration phase with design locked by upstream IMSlider + existing LayerSlider.qml.*
