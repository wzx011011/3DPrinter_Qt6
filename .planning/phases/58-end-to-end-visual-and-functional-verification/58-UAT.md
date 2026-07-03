# Phase 58 — Manual User Acceptance Test Checklist (VERIFY-04)

**Phase:** 58-end-to-end-visual-and-functional-verification
**Plan:** 58-02 (Task 2)
**Visual truth:** the 4 screenshots under `shotScreen/`
**Behavior truth:** upstream OrcaSlicer v7.0.1 (`third_party/OrcaSlicer`)
**Region reference:** `docs/v3.6-ui-inventory.md` (region IDs are immutable)

This is the human sign-off gate for the v3.6 milestone. A non-developer
(product owner) runs `build/OWzxSlicer.exe`, walks the v3.6 workflow, and
compares what they see against the 4 screenshots. Each item maps to one
screenshot-visible region (PREP-/PREV-/SETPRINT-/SETMAT-) AND one or more
requirements from `.planning/REQUIREMENTS.md`.

For every item, mark one of: **PASS** / **FAIL** / **NOT-AVAILABLE (N-A)**.
For any FAIL, add a one-line note describing what was wrong.

## Prerequisites

1. `build/OWzxSlicer.exe` exists and was built by the canonical verify
   script (`scripts/auto_verify_with_vcvars.ps1`). If not, build it first.
2. The 4 screenshots under `shotScreen/` are open in an image viewer so you
   can compare side-by-side:
   - `shotScreen/准备页.png` — Prepare page
   - `shotScreen/预览页.png` — Preview page
   - `shotScreen/打印机参数设置页.png` — Printer settings
   - `shotScreen/材料参数设置页.png` — Material settings
3. A small STL test model is available. The E2E tests use
   `third_party/OrcaSlicer/tests/data/test_3mf/Prusa.stl`; if you do not
   have that path, import any small STL you have on hand.

## How to Run

1. Launch `build/OWzxSlicer.exe`.
2. Walk sections A through E below in order.
3. For each item, compare what you see in the running app to the named
   screenshot quadrant, then mark PASS / FAIL / N-A. Add a one-line note
   for any FAIL describing the visible discrepancy.

---

## Section A — Prepare Page vs `shotScreen/准备页.png`

Open the Prepare page (the default landing page). Compare each region
below to the matching quadrant of `准备页.png`.

### A.1 — Top shell / menu bar (PREP-TOP)
- **Compare to:** `准备页.png` top edge — the menu bar (File / Edit / View etc.) and the page tabs (准备 / 预览).
- **Check:** the menu bar is visible, the 准备 tab is currently selected, the 预览 tab is present but disabled until a slice exists, and the window controls (minimize / maximize / close) are at the right.
- **Maps to:** SHELL-01, SHELL-02.
- **PASS / FAIL / N-A:** ____

### A.2 — Left preset sidebar (PREP-SIDEBAR)
- **Compare to:** `准备页.png` left edge — the vertical Printer / Filament / Process preset combos.
- **Check:** the sidebar shows a Printer preset combo, one or more Filament slots, and a Process preset combo. Each has an expand/gear button. Switching the Printer preset updates the displayed name.
- **Maps to:** PREPSB-01, PREPSB-03, PREPSB-05.
- **PASS / FAIL / N-A:** ____

### A.3 — Object list panel (PREP-OBJLIST)
- **Compare to:** `准备页.png` left side, below the preset sidebar — the imported-object rows.
- **Check:** after importing a model, a row appears with the model name. Right-click exposes add / select / delete / rename actions. Selecting a row highlights the corresponding mesh in the viewport.
- **Maps to:** PREPWF-01, PREPWF-02.
- **PASS / FAIL / N-A:** ____

### A.4 — 3D viewport / bed (PREP-VIEWPORT)
- **Compare to:** `准备页.png` center — the bed grid with the imported model on it.
- **Check:** a bed grid is rendered, the imported model is visible and selectable, the camera responds to mouse orbit / pan / zoom, and the plate boundary is drawn.
- **Maps to:** PREPWF-04, PREPWF-06.
- **PASS / FAIL / N-A:** ____

### A.5 — Vertical GL tool buttons (PREP-VTOOLBAR)
- **Compare to:** `准备页.png` left-of-viewport vertical strip — Move / Rotate / Scale / Cut / Place / Support / Seam / Paint icons.
- **Check:** the vertical tool strip is visible. Clicking Move / Rotate / Scale activates the corresponding gizmo on the selected object. Tools without a real backend (e.g. support/seam/paint where OpenVDB is unavailable) are visibly disabled or honestly classified, not silently dead.
- **Maps to:** PREPWF-05.
- **PASS / FAIL / N-A:** ____

### A.6 — Gizmo floating control panel (PREP-GIZMOFLOAT)
- **Compare to:** `准备页.png` center — the conditional floating input window that appears when a gizmo is active.
- **Check:** activating the Cut gizmo opens its floating input window (connector / cut plane controls). Closing the gizmo dismisses the window.
- **Maps to:** PREPWF-05.
- **PASS / FAIL / N-A:** ____

### A.7 — Plate / multi-plate strip (PREP-PLATEBAR)
- **Compare to:** `准备页.png` bottom-left or bottom edge — the plate thumbnail strip.
- **Check:** the plate strip is visible, the current plate is highlighted, and an Add-plate action is available. Switching plates updates the viewport.
- **Maps to:** PREPWF-03.
- **PASS / FAIL / N-A:** ____

### A.8 — Slice progress / status (PREP-SLICESTATUS)
- **Compare to:** `准备页.png` bottom edge — the slice progress bar and status text.
- **Check:** before slicing, the Slice button is enabled (when a model is on the bed) and the status text is idle. During slicing, a progress bar advances and the Slice button is disabled. After slicing, the status text shows the result.
- **Maps to:** SHELL-03, PREPWF-06.
- **PASS / FAIL / N-A:** ____

### A.9 — Arrange settings popup / view-orientation controls (PREP-VIEWOPTS)
- **Compare to:** `准备页.png` viewport corner — the camera preset buttons (Top / Front / Right / Iso) and the Arrange popup.
- **Check:** the view-orientation buttons re-orient the camera. The Arrange action opens a small popup with arrange settings (spacing / rotation) before arranging.
- **Maps to:** PREPWF-04.
- **PASS / FAIL / N-A:** ____

---

## Section B — Preview Page vs `shotScreen/预览页.png`

Switch to the Preview page (the 预览 tab becomes enabled after a successful
slice). Compare each region below to `预览页.png`.

### B.1 — Preview top shell / header bar (PREV-TOP)
- **Compare to:** `预览页.png` top edge — the "预览模式" label, view-mode combo, camera preset buttons, time/progress chip, layer/move summary chip.
- **Check:** the header bar shows the view-mode combo and the time/layer summary. Camera preset buttons (Top / Front / Right / Iso) re-orient the preview camera.
- **Maps to:** PREVLAY-01, GCODE-02.
- **PASS / FAIL / N-A:** ____

### B.2 — Left state / layer-slider panel (PREV-LEFT)
- **Compare to:** `预览页.png` left edge — the vertical layer slider and plate thumbnail/summary.
- **Check:** the left panel contains the vertical layer slider. The plate-thumbnail area may be a placeholder; if so it is honestly labeled, not silently empty.
- **Maps to:** PREVLAY-01, PREVLAY-04.
- **PASS / FAIL / N-A:** ____

### B.3 — G-code viewport (PREV-VIEWPORT)
- **Compare to:** `预览页.png` center — the G-code preview canvas.
- **Check:** the center canvas renders the sliced G-code (colored extrusion paths, tool marker). Real sliced data is shown — not a placeholder wireframe.
- **Maps to:** GCODE-01, GCODE-03.
- **PASS / FAIL / N-A:** ____

### B.4 — Vertical layer slider (PREV-VSLIDER) (regression-critical)
- **Compare to:** `预览页.png` right edge of the left panel — the vertical slider with layer handles.
- **Check:** drag the layer slider up and down. The Preview stays visible at every position (the disappearing-preview bug must NOT regress). The current-layer indicator updates.
- **Maps to:** PREVLAY-02, GCODE-05.
- **PASS / FAIL / N-A:** ____

### B.5 — Bottom horizontal move slider (PREV-MSLIDER) (regression-critical)
- **Compare to:** `预览页.png` bottom edge — the horizontal move slider with play / pause / step buttons.
- **Check:** drag the move slider. The Preview stays visible at every position. Play / pause / step buttons advance the current move. The current-move indicator updates.
- **Maps to:** PREVLAY-02, GCODE-05.
- **PASS / FAIL / N-A:** ____

### B.6 — Camera orbit / pan / zoom in Preview (PREV-VIEWPORT) (regression-critical)
- **Compare to:** `预览页.png` center — orbit the preview camera with the mouse.
- **Check:** orbit / pan / zoom the Preview viewport. The G-code preview does NOT disappear or reset its draw ranges during or after the camera interaction.
- **Maps to:** PREVLAY-03, GCODE-05.
- **PASS / FAIL / N-A:** ____

### B.7 — Right legend + statistics panel container (PREV-RIGHT)
- **Compare to:** `预览页.png` right edge — the right panel container holding the legend and statistics.
- **Check:** the right panel container is visible and holds the legend and statistics sub-regions.
- **Maps to:** PREVLAY-01, PREVLAY-05.
- **PASS / FAIL / N-A:** ____

### B.8 — Color legend (PREV-LEGEND)
- **Compare to:** `预览页.png` right panel — the collapsible color/role legend rows.
- **Check:** the legend lists the active color mode's role rows with colored swatches. The legend can be collapsed and re-expanded.
- **Maps to:** GCODE-02, GCODE-03.
- **PASS / FAIL / N-A:** ____

### B.9 — Statistics panel (PREV-STATS)
- **Compare to:** `预览页.png` right panel — the print time / filament usage / layer count statistics.
- **Check:** the statistics panel shows print time, filament usage, and layer count for the current slice. Values are non-zero and match the slice result.
- **Maps to:** PREVLAY-04, GCODE-01.
- **PASS / FAIL / N-A:** ____

### B.10 — Page switch Prepare -> Preview -> Prepare (regression-critical)
- **Check:** slice once, switch to Preview, switch back to Prepare, switch to Preview again. The G-code preview data is preserved across the page switches (no reload, no disappearance).
- **Maps to:** PREVLAY-02, GCODE-05.
- **PASS / FAIL / N-A:** ____

### B.11 — Reslice (regression-critical)
- **Check:** from Preview, trigger a reslice (e.g. by changing a slice-affecting setting and re-slicing). The Preview rebuilds with different bytes — the new slice result is reflected, not the stale one.
- **Maps to:** GCODE-05.
- **PASS / FAIL / N-A:** ____

### B.12 — Export while Preview visible (regression-critical)
- **Check:** with Preview visible, export the G-code to a temp path. The Preview stays visible and its data is intact after the export completes.
- **Maps to:** GCODE-05, SHELL-03.
- **PASS / FAIL / N-A:** ____

### B.13 — Tool position tooltip (PREV-TOOLTIP)
- **Compare to:** `预览页.png` bottom-left floating tooltip — the tool X/Y/Z position.
- **Check:** as you drag the move slider, a small tooltip near the bottom-left shows the current tool X/Y/Z position.
- **Maps to:** PREVLAY-04, GCODE-03.
- **PASS / FAIL / N-A:** ____

---

## Section C — Printer Settings vs `shotScreen/打印机参数设置页.png`

Open the Printer settings dialog from the left sidebar's printer gear
button. Compare each region below to `打印机参数设置页.png`.

### C.1 — Dialog shell / window chrome (SETPRINT-SHELL)
- **Compare to:** `打印机参数设置页.png` overall frame — the dialog title bar and window chrome (dark theme).
- **Check:** the printer settings open as an independent window (not embedded in the main window). The title bar shows the dialog title. The close button dismisses the dialog.
- **Maps to:** SETTINGS-01, INV-04.
- **PASS / FAIL / N-A:** ____

### C.2 — Preset selector bar (SETPRINT-PRESETBAR)
- **Compare to:** `打印机参数设置页.png` top of dialog — the current preset combo, dirty/save indicator, Save / Save As / Reset actions.
- **Check:** the preset combo shows the current printer preset. A dirty indicator appears when an option is edited. Save / Save As / Reset actions are present and enabled/disabled appropriately.
- **Maps to:** SETTINGS-04, SETTINGS-05.
- **PASS / FAIL / N-A:** ____

### C.3 — Top category tabs (SETPRINT-TABS)
- **Compare to:** `打印机参数设置页.png` top — the row of category tabs (Machine / Extruder printer-scope tabs).
- **Check:** the tab row lists the printer-scope categories. Switching tabs changes the visible option groups.
- **Maps to:** SETTINGS-02.
- **PASS / FAIL / N-A:** ____

### C.4 — Left option-group navigation (SETPRINT-GROUPNAV)
- **Compare to:** `打印机参数设置页.png` left column — the option-group list (General / Build Volume / Capabilities / Custom G-code etc.).
- **Check:** the left column lists option groups for the currently selected tab. Selecting a group scrolls the option area to that group.
- **Maps to:** SETTINGS-02.
- **PASS / FAIL / N-A:** ____

### C.5 — Main option editing area (SETPRINT-OPTIONS)
- **Compare to:** `打印机参数设置页.png` center — the scrollable area of checkboxes / number fields / combos / spin boxes with units (mm / % / °C / s).
- **Check:** options render with the correct typed control (checkbox for bool, number field for int/float with unit, combo for enum). Editing a value marks it dirty and shows the value-source indicator.
- **Maps to:** SETTINGS-03, SETTINGS-04.
- **PASS / FAIL / N-A:** ____

### C.6 — Search field + basic/advanced toggle (SETPRINT-SEARCH)
- **Compare to:** `打印机参数设置页.png` above the option area — the search box and Advanced toggle.
- **Check:** typing in the search box filters the visible options. Toggling Advanced shows/hides advanced-scope options.
- **Maps to:** SETTINGS-06, PREPSB-04.
- **PASS / FAIL / N-A:** ____

### C.7 — Footer action bar (SETPRINT-FOOTER)
- **Compare to:** `打印机参数设置页.png` bottom — the Save / Discard / Cancel buttons.
- **Check:** the footer shows Save / Discard / Cancel. With unsaved changes, attempting to close the dialog triggers the Unsaved Changes guard.
- **Maps to:** SETTINGS-05.
- **PASS / FAIL / N-A:** ____

### C.8 — Dirty / compat / warning indicators (SETPRINT-DIRTY)
- **Compare to:** `打印机参数设置页.png` inline — the modified-option markers, inheritance/source indicators, compatibility badges.
- **Check:** editing an option shows an inline modified marker. The value-source indicator shows whether the value is the preset default, an inherited value, or a user override. Compatibility badges appear when a preset is incompatible with the active printer.
- **Maps to:** SETTINGS-04, SETTINGS-07.
- **PASS / FAIL / N-A:** ____

---

## Section D — Material Settings vs `shotScreen/材料参数设置页.png`

Open the Material settings dialog. Compare each region below to
`材料参数设置页.png`. The 8 regions mirror Section C (shared dialog shell).

### D.1 — Dialog shell / window chrome (SETMAT-SHELL)
- **Compare to:** `材料参数设置页.png` overall frame — the dialog title bar and window chrome (dark theme).
- **Check:** the material settings open as an independent window. The title bar shows the material-scope title.
- **Maps to:** SETTINGS-01, INV-04.
- **PASS / FAIL / N-A:** ____

### D.2 — Preset selector bar (SETMAT-PRESETBAR)
- **Compare to:** `材料参数设置页.png` top — the material preset combo, dirty/save indicator, Save / Save As / Reset.
- **Check:** the preset combo shows the current material (filament) preset. Dirty indicator and Save / Save As / Reset behave as in C.2.
- **Maps to:** SETTINGS-04, SETTINGS-05.
- **PASS / FAIL / N-A:** ____

### D.3 — Top category tabs (SETMAT-TABS)
- **Compare to:** `材料参数设置页.png` top — the row of material-scope tabs (Filament / Temperature / Cooling / Retraction / Advanced / G-code).
- **Check:** the tab row lists the material-scope categories. Switching tabs changes the visible option groups.
- **Maps to:** SETTINGS-02.
- **PASS / FAIL / N-A:** ____

### D.4 — Left option-group navigation (SETMAT-GROUPNAV)
- **Compare to:** `材料参数设置页.png` left column — the option-group list per visible tab.
- **Check:** the left column lists option groups for the selected material tab. Selecting a group scrolls the option area.
- **Maps to:** SETTINGS-02.
- **PASS / FAIL / N-A:** ____

### D.5 — Main option editing area (SETMAT-OPTIONS)
- **Compare to:** `材料参数设置页.png` center — the scrollable area of typed controls (temperature °C, retraction mm, flow %).
- **Check:** options render with the correct typed control and unit. Editing marks dirty and shows value-source.
- **Maps to:** SETTINGS-03, SETTINGS-04.
- **PASS / FAIL / N-A:** ____

### D.6 — Search + advanced toggle (SETMAT-SEARCH)
- **Compare to:** `材料参数设置页.png` above the option area — search box and Advanced toggle.
- **Check:** search filters options; Advanced toggle shows/hides advanced-scope options.
- **Maps to:** SETTINGS-06, PREPSB-04.
- **PASS / FAIL / N-A:** ____

### D.7 — Footer action bar (SETMAT-FOOTER)
- **Compare to:** `材料参数设置页.png` bottom — Save / Discard / Cancel buttons.
- **Check:** footer actions present; Unsaved Changes guard fires on dirty close.
- **Maps to:** SETTINGS-05.
- **PASS / FAIL / N-A:** ____

### D.8 — Dirty / compat / warning indicators (SETMAT-DIRTY)
- **Compare to:** `材料参数设置页.png` inline — modified-option markers, inheritance/source indicators, compatibility badges.
- **Check:** editing shows modified marker + value-source indicator; compatibility badges appear when incompatible.
- **Maps to:** SETTINGS-04, SETTINGS-07.
- **PASS / FAIL / N-A:** ____

---

## Section E — Full Local Workflow (import -> configure -> prepare -> slice -> preview -> export)

Walk the full v3.6 local workflow end-to-end. Each item exercises a
cross-region transition.

### E.1 — Import a model
- **Action:** File -> Import (or drag-drop) a small STL onto the Prepare page.
- **Expected:** the model appears in the viewport (PREP-VIEWPORT) and a row appears in the object list (PREP-OBJLIST). The Slice button becomes enabled.
- **Maps to:** PREPWF-01, SHELL-03.
- **PASS / FAIL / N-A:** ____

### E.2 — Select a printer preset
- **Action:** in the left sidebar (PREP-SIDEBAR), pick a different printer preset from the combo.
- **Expected:** the preset name updates; any incompatible state is shown via compatibility badges (SETPRINT-DIRTY).
- **Maps to:** PREPSB-01, PREPSB-03.
- **PASS / FAIL / N-A:** ____

### E.3 — Edit a process setting
- **Action:** open the Process settings dialog, edit one option (e.g. layer height), do NOT save.
- **Expected:** the option shows a modified marker (SETPRINT-DIRTY). The Prepare sidebar's preset indicator shows dirty state.
- **Maps to:** SETTINGS-03, SETTINGS-04, PREPSB-05.
- **PASS / FAIL / N-A:** ____

### E.4 — Observe slice-result-stale indicator
- **Action:** with the unsaved settings edit from E.3, return to Prepare.
- **Expected:** the slice-result-stale indicator fires (the previous slice result is marked stale because settings changed). The Slice button prompts a re-slice.
- **Maps to:** SETTINGS-07, PREPWF-06.
- **PASS / FAIL / N-A:** ____

### E.5 — Slice
- **Action:** click Slice.
- **Expected:** the slice progress bar advances (PREP-SLICESTATUS); on completion the 预览 tab becomes enabled and the Slice output path is populated.
- **Maps to:** PREPWF, GCODE-01.
- **PASS / FAIL / N-A:** ____

### E.6 — Switch to Preview
- **Action:** click the 预览 tab.
- **Expected:** the Preview page renders the real G-code (PREV-VIEWPORT), the layer/move sliders populate, and the statistics panel shows non-zero values.
- **Maps to:** PREVLAY-01, GCODE-01.
- **PASS / FAIL / N-A:** ____

### E.7 — Move the layer + move sliders
- **Action:** drag the vertical layer slider (PREV-VSLIDER) and the horizontal move slider (PREV-MSLIDER).
- **Expected:** the Preview stays visible at every position; the current-layer and current-move indicators update; the tool tooltip updates.
- **Maps to:** PREVLAY-02, GCODE-05.
- **PASS / FAIL / N-A:** ____

### E.8 — Export G-code
- **Action:** from Preview (or Prepare), export the G-code to a temp path.
- **Expected:** the export succeeds (SHELL-03 canExportGCode gate opens); the exported file is non-empty; the Preview stays intact after the export.
- **Maps to:** SHELL-03, GCODE-05.
- **PASS / FAIL / N-A:** ____

### E.9 — Switch color mode
- **Action:** in Preview, switch the color mode via the view-mode combo (PREV-TOP).
- **Expected:** the legend (PREV-LEGEND) updates to the new mode's role rows; the viewport recolors accordingly.
- **Maps to:** GCODE-02, GCODE-03.
- **PASS / FAIL / N-A:** ____

---

## Sign-Off

Fill in at the end of the run.

- Visual parity against the 4 screenshots: **PASS / FAIL**
- Behavior parity against upstream OrcaSlicer: **PASS / FAIL**
- Outstanding gaps (list each with region + requirement + follow-up owner):

  - _______________________________________________________________
  - _______________________________________________________________
  - _______________________________________________________________

When this checklist is fully run and the sign-off is recorded, the v3.6
milestone's VERIFY-04 is closed. Phase 58 verification status moves from
`human_needed` to `complete`.
