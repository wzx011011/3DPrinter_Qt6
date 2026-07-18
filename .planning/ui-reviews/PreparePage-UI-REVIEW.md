# PreparePage — UI Review (v5.1 retroactive 6-pillar audit)

**Audited:** 2026-07-17
**Baseline:** `.planning/milestones/v3.9-phases/74-prepare-source-truth-gap-audit/74-UI-SPEC.md` (Prepare page design contract) + abstract 6-pillar standards
**Screenshots:** Not captured — code-only audit (Qt6/QML desktop app; no dev server / Playwright applicable)
**Primary target:** `src/qml_gui/pages/PreparePage.qml` (3965 lines)
**In scope this round:** v5.1 Phase 154 (Compare Presets), Phase 156 (session-capture scheduler), Phase 158 (Emboss style extension); plus whole-file 6-pillar sweep.

---

## Pillar Scores

| Pillar | Score | Key Finding |
|--------|-------|-------------|
| 1. Copywriting | 2/4 | Language is bilingual-by-accident: dialogs are 100% English (PresetDiffDialog, CreatePresetsDialog) or 100% Chinese (UnsavedChangesDialog) with no shared glossary; Phase 158 tooltips leak `E mboss.hpp` / `ProjectCurve` developer jargon into user copy. |
| 2. Visuals | 2/4 | Phase 158 boldness control uses raw QtQuick `Slider` while every peer gizmo panel (Simplify, Support paint, etc.) uses `CxSlider`; gizmo action rows are hand-rolled `Rectangle+Text+MouseArea` pseudo-buttons that bypass `CxButton`/`CxIconButton` disabled & press animations. |
| 3. Color | 2/4 | 46 hardcoded hex literals; PresetDiffDialog status badges (#1f8a4c/#b03a3a/#c98a1a) ignore existing `Theme.statusSuccess/Error/Warning`; 2 references to non-existent `Theme.borderActive` token (silent `undefined`). |
| 4. Typography | 2/4 | 114 of 179 `font.pixelSize` declarations are hardcoded literals (92× size 10, 16× 9, 3× 8, 1× 7, 1× 13, 1× 14) — only 36% use `Theme.fontSize*` tokens. |
| 5. Spacing | 1/4 | Zero `Theme.spacing*` token usage anywhere in the file; 7+ distinct raw spacing values plus arbitrary `bottomMargin: 52`. Sidebar locked at 392px (min=max=default) directly contradicts UI-SPEC's "narrow compact" requirement (Phase 74 Critical gap still unresolved). |
| 6. Experience Design | 2/4 | Strong disabled-state coverage (51 `enabled:` bindings) and Emboss async feedback wiring (postNotification/postError), but destructive actions (delete-object / delete-plate / clear-plate / clearAllPaintData) fire instantly with no confirmation; async Emboss has no progress/spinner affordance. |

**Overall: 11/24**

---

## Top 3 Priority Fixes

1. **Phase 158 Emboss `Slider` → `CxSlider` and remove dev-jargon tooltips** — `src/qml_gui/pages/PreparePage.qml:3323-3351`. The boldness control is the only raw `QtQuick.Controls.Slider` in the entire 3965-line file (every other gizmo panel — Simplify line 2932/2952, Support paint line 2073 — uses `CxSlider` with the project's accent handle, press scale animation, and disabled opacity). The two new `CxCheckBox` tooltips at lines 3343 and 3350 leak implementation detail (`"上游 Emboss.hpp 尚无 ProjectCurve 实现；意图已持久化，几何变形延后"`) — this reads as a developer note, not user copy. Replace with user-appropriate copy (e.g. `qsTr("贴附到模型表面（实验性，几何效果将在后续版本生效）")`) or remove the tooltip entirely until the feature produces visible output.

2. **Pick one language for the dialogs and apply it uniformly.** PresetDiffDialog.qml (Phase 154) is 100% English (`"Compare Presets"`, `"Printer"`, `"Material"`, `"Process"`, `"Scope:"`, `"Compare"`, `"Close"`, `"No differences"`, `"Select two presets and click Compare"`, `"%1 difference(s)"`, status badges `"added"/"removed"/"changed"`). CreatePresetsDialog.qml (Phase 147) is 100% English. UnsavedChangesDialog.qml is 100% Chinese. PreparePage object-info bar mixes English into a Chinese surface (`"Sliced"/"Stale"/"Ready"/"OK"/"%1 non-manifold edges"/"%1 errors repaired"` at lines 3742-3744, 3906, 3924, 3933). SettingsDialog empty-states at lines 490-491 are English inside an otherwise-Chinese dialog. Pick Chinese (the rest of the app) and run a single sweep.

3. **Add confirmation guards for destructive actions** — `deleteSelection` (Delete key line 149, menu lines 279/480), `removeAllOnPlate` (line 604 "清空平板"), `deletePlate` (line 760), `clearAllPaintData` (line 2131) all execute immediately with no `Dialog`/confirm step. A user reaching for Delete to deselect (or right-clicking a plate intending "settings") can destroy work irreversibly. Pattern already exists in the file (`plateRenameDialog`, `plateSettingsDialogComp`) — add a destructive-confirm dialog component and route the four triggers through it.

---

## Detailed Findings

### Pillar 1: Copywriting (2/4)

**Finding 1.1 — Dialog language is split down the middle.** Phase 154's new `PresetDiffDialog.qml` is entirely English (`qsTr("Compare Presets")`, `qsTr("Scope:")`, `qsTr("Printer")`, `qsTr("Material")`, `qsTr("Process")`, `qsTr("Compare")`, `qsTr("Close")`, `qsTr("Key")`, `qsTr("Select two presets and click Compare")`, `qsTr("No differences")`, `qsTr("%1 difference(s)")`, status `"added"/"removed"/"changed"`). The peer `CreatePresetsDialog.qml` is also entirely English. The peer `UnsavedChangesDialog.qml` is entirely Chinese. `SettingsDialog.qml` is Chinese except for two empty-state strings (`"No matching options"`, `"No options"` at lines 490-491). The runtime application language is Chinese (every PreparePage menu label is `qsTr("删除选中")` etc.). A user opening "Compare Presets" from the Chinese tooltip-labeled `比较预设` button lands in an all-English dialog.

**Finding 1.2 — Developer jargon in user-facing tooltips (Phase 158).** `src/qml_gui/pages/PreparePage.qml:3342-3344` and `3349-3351`:
```qml
ToolTip.text: qsTr("上游 Emboss.hpp 尚无 ProjectCurve 实现；意图已持久化，几何变形延后")
```
References to `E mboss.hpp`, `ProjectCurve`, "上游" (upstream), "持久化" (persisted), and "几何变形延后" (geometry deformation deferred) are implementation-detail text that no end-user of a 3D slicer should see. The CxCheckBox labels themselves ("贴附表面", "曲线投影") are user-appropriate; only the tooltip copy leaks internals.

**Finding 1.3 — English status strings inside a Chinese surface.** `PreparePage.qml:3742-3744` plate-bar status:
```qml
text: sliceResultStatus === 1 ? qsTr("Sliced")
    : sliceResultStatus === 2 ? qsTr("Stale")
    : qsTr("Ready")
```
And the manifold info bar at lines 3906, 3924, 3933: text `"OK"`, `qsTr(" | %1 non-manifold edges")`, `qsTr(" | %1 errors repaired")` — all English while every surrounding label (`"对对象"`, `"平板"`) is Chinese.

**Finding 1.4 — Placeholder copy leaks pre-release notes.** `PreparePage.qml:3372` `qsTr("(异步执行后状态显示于此)")` ("status will display here after async execution") ships as the visible placeholder. Per UI-SPEC Required States, the panel should show a real idle/empty state, not author a meta-description of where status will go.

**Finding 1.5 — Mixed-script menu items.** `PreparePage.qml:415` `qsTr("编辑工艺设置")` (Chinese) is paired with `qsTr("Layer height")`-style raw keys flagged in Phase 74 GAP-MATRIX SIDE-02 as Critical (sidebar option-row labels still partly raw English). Out of PreparePage's direct scope but reachable through `backend.forwardSettingsRequest`.

### Pillar 2: Visuals (2/4)

**Finding 2.1 — Phase 158 Emboss boldness slider uses raw `Slider`, breaks panel consistency.** `PreparePage.qml:3323-3328`:
```qml
Slider {
    Layout.preferredWidth: 80
    from: 0.0; to: 2.0; stepSize: 0.1
    value: root.editorVm ? root.editorVm.embossBoldness : 0.0
    onMoved: if (root.editorVm) root.editorVm.embossBoldness = value
}
```
This is the **only** raw `Slider` in the file (grep `\bSlider\b` returns 1 match). The Support-paint panel (line 2073), Simplify panel (lines 2932 and 2952), and Measure panel all use `CxSlider` — which applies `Theme.accent` handle fill, 14px handle, `ColorAnimation` press feedback, and 0.45 disabled opacity. The raw `Slider` renders with QtQuick.Controls' default native look (no accent fill, no animation), producing a visible style mismatch inside the same 6px-spaced column as the styled CxSpinBox/CxComboBox peers above and below it. **Fix:** swap to `CxSlider` with `from: 0.0; to: 2.0; stepSize: 0.1`.

**Finding 2.2 — Inline pseudo-buttons bypass `CxButton`.** Twelve-plus "执行" / "异步执行" / "执行运算" / "执行切割" / "执行检测" / "执行钻孔" / "执行简化" / "添加文字" / "导入 SVG" / "+" add-plate buttons are hand-rolled `Rectangle { Text { } MouseArea { } }` triples (lines 3260-3262, 3358-3361, 3363-3366, 3416-3418, 3451-3453, 3482-3484, 3514-3516, 3546-3548, 3839-3854). They have no hover state, no press scale, no disabled visual (the SVG panel's disabled "执行" button at line 3541 `enabled: false` on the CxTextField is fine, but if the action button were ever disabled it would not visually dim). `CxButton` exists for exactly this purpose and is used properly in the dialogs.

**Finding 2.3 — Six distinct inline button heights.** Across the gizmo panels and plate bar: `height: 20` (1×), `22` (5×), `24` (18×), `26` (3×), `28` (11×), `30` (1×). `Theme.controlHeightSM=28`, `controlHeightMD=34`, `controlHeightLG=40` exist as tokens but are not honored. The 24px-height buttons in particular cluster in the gizmo panels while 28px buttons cluster in dialog rows — same visual role, different sizes.

**Finding 2.4 — Focal-point risk: gizmo panel stack overlaps top-center.** Eight different gizmo panels (Transform, Support paint, Measure, Cut, AdvancedCut, Emboss, MeshBoolean, FaceDetector, Text, SVG, SLA, Hollow, Simplify) all anchor `top: parent.top; topMargin: gizmoPanelTopOffset (74)` and `horizontalCenter: parent.horizontalCenter`. Visibility is gated by `gizmoMode` so only one shows at a time — that part is correct. But because each panel computes its own width from `content.implicitWidth + 24`, switching gizmos produces a visible width jump. UI-SPEC Visual Rules requires "stable dimensions so hover/disabled states do not shift the viewport layout"; the gizmo swap does shift the floating panel.

**Finding 2.5 — Icon-only "+" add-plate button has no tooltip.** `PreparePage.qml:3839-3854`: a 26×26 `Rectangle { Text { text: "+" } MouseArea { } }` with no `ToolTip`. Per UI-SPEC Visual Rules and standard a11y practice, icon-only controls must carry a tooltip.

### Pillar 3: Color (2/4)

**Counts (PreparePage.qml):**
- 31 distinct `Theme.*` color tokens referenced (top: `Theme.textPrimary` 62×, `Theme.textMuted` 48×, `Theme.accent` 24×, `Theme.bgFloating` 18×, `Theme.borderSubtle` 28×, `Theme.bgElevated` 11×).
- **46 hardcoded hex literals** across 42 lines.

**Finding 3.1 — Justifiable hardcoded colors.** The following are arguably acceptable (data-driven palettes, not chrome):
- `extruderColors: ["#FF4444", "#44AA44", "#4444FF", "#FF8800"]` (line 838) — material color swatches, upstream-defined.
- The 16-color filament palette at lines 3174-3178 — used for AMS slot color dots.
- Axis accents `"#e066a0" / "#4ec9b0" / "#569cd6"` (lines 1967/1976/1985, 2184-2188, repeated 2402/2407, 2432/2437, 2464/2469, 2497/2502, 2698/2703) — VS-Code-style X/Y/Z axis coloring reused as the selected-state color for gizmo tool toggles. This is consistent within the file but lives outside the token system, so it cannot be theme-adjusted.

**Finding 3.2 — Phase 154 PresetDiffDialog status badges ignore Theme tokens.** `PresetDiffDialog.qml:263-266`:
```qml
color: modelData.status === "added"   ? "#1f8a4c"
     : modelData.status === "removed" ? "#b03a3a"
     : modelData.status === "changed" ? "#c98a1a"
                                      : Theme.bgPanel
```
The exact semantic colors these badges represent already exist in `Theme.qml`: `statusSuccess #18c75e`, `statusError #e04040`, `statusWarning #f5a623`. Hardcoding close-but-different hexes means a future theme adjustment to `Theme.statusError` will leave the diff dialog visually inconsistent.

**Finding 3.3 — Non-existent `Theme.borderActive` token, referenced twice.** `PreparePage.qml:1077` and `1329`:
```qml
border.color: pillDropArea.containsDrag ? Theme.accent : (pillDragMA.containsMouse ? Theme.borderActive : "transparent")
```
`grep Theme.qml` shows no `borderActive` property — only `borderDefault`, `borderSubtle`, `borderStrong`, `borderFocus`, `borderInput`. At runtime `Theme.borderActive` evaluates to `undefined`, which QML renders as a default (typically black or no stroke depending on Qt version). This is a silent defect; the file emits a QML warning at startup.

**Finding 3.4 — Hardcoded semantic colors in the manifold info bar.** `PreparePage.qml:3899-3900`, `3925`, `3934`: `"#c0392b"` (red), `"#e67e22"` (orange), `"#27ae60"` (green) — flat-color semantic indicators that bypass `Theme.statusError` / `Theme.statusWarning` / `Theme.statusSuccess`. These look like leftover flat-ui palette colors from an earlier design iteration.

**Finding 3.5 — Drag overlay color uses inline ARGB.** `PreparePage.qml:1870` `color: "#4a0b1018"` — 50% transparent dark bed color used as the file-import drag highlight. `Theme.overlayDim` (`#80000000`) is the token equivalent.

**Finding 3.6 — Color misuse on rename dialog buttons.** `PreparePage.qml:575` `color: "#fff"` (raw white) instead of `Theme.textOnAccent`. Single occurrence but trivially fixable.

### Pillar 4: Typography (2/4)

**Counts (PreparePage.qml):**
- **179 `font.pixelSize` declarations total.**
- Tokenized: 65 (36%) — `Theme.fontSizeSM` 39×, `fontSizeMD` 20×, `fontSizeLG` 3×, `fontSizeXS` 2×, `fontSizeXL` 1×.
- Hardcoded literals: **114 (64%)** — `10` 92×, `9` 16×, `8` 3×, `7` 1×, `13` 1×, `14` 1×.
- `font.bold: true` 34× — concentrated in panel titles and tab labels (consistent usage).
- `font.family: "Consolas, monospace"` 14× for numeric/value readouts (consistent).
- `font.italic` 1× (Emboss async placeholder line 3375).

**Finding 4.1 — `font.pixelSize: 10` is the de-facto body size, not the tokenized size.** `Theme.fontSizeMD = 12` and `Theme.fontSizeSM = 11` exist precisely to be the body/secondary sizes; instead 92 sites use raw `10`. This means a future bump to `Theme.fontSizeSM` will not propagate to the gizmo panel body text, the plate-bar labels, or the support-paint readouts — they will stay at 10px and silently diverge.

**Finding 4.2 — Out-of-scale sizes 7, 8, 9, 13, 14.** Sizes 7 and 8 appear only in plate-bar secondary text (`font.pixelSize: 7` line 3908, `8` lines 3736/3748) and the manifold icon. Size 9 is the gizmo-panel hint/caption size. Sizes 13 and 14 each appear once (line 783 and 3852) — the only two literal sizes that happen to match a Theme token (`fontSizeLG=14`, with 13 being an off-by-one). The Theme scale has 6 sizes (10/11/12/14/16/20); the file invents two extras (7, 8, 9, 13) outside that scale.

**Finding 4.3 — `font.family: "Consolas, monospace"` not centralized.** 14 sites hardcode the same family string. If the monospace choice ever changes (e.g. for CJK monospace fallback), 14 edits are required. `Theme.qml` exposes no `monoFontFamily` token.

### Pillar 5: Spacing (1/4) — lowest-scoring pillar

**Counts (PreparePage.qml):**
- **Zero** uses of any `Theme.spacing*` token (`spacingXS/SM/MD/LG/XL/XXL` defined as 4/6/8/12/16/24). Verified by `grep -oE '(spacing|...margins): Theme\.[a-zA-Z]+'` returning 0 lines.
- Raw spacing values in use: `0` (2×), `3` (2×), `4` (17×), `6` (35×), `8` (14×), `10` (11×), `12` (6×), `14` (1×), `16` (1×), plus `bottomMargin: 52` (1×), `leftMargin/rightMargin: 14` (4×), `margins: 1` (1×), `margins: 3` (1×).

**Finding 5.1 — Phase 158 Emboss `RowLayout { spacing: 12 }` (line 3332) breaks the panel's own rhythm.** The Emboss panel `ColumnLayout { spacing: 6 }` (line 3282) and every peer gizmo panel uses `spacing: 6` for its row groups. The new italic/use-surface/curve-projection checkbox row at line 3330 uses `spacing: 12` with no documented reason — the wider gap visually detaches the new toggles from the rest of the Emboss controls. UI-SPEC: "left sidebar must be dense but readable; option rows should not become oversized dashboard cards" — same principle applies to floating gizmo panels.

**Finding 5.2 — Seven distinct radius values; most-used (4, 6) are not on the Theme scale.** `radius: 4` (35×), `6` (24×), `8` (4×), `9` (2×), `7` (2×), `5` (2×), `3` (2×). Theme tokens are `radiusSM=3`, `radiusMD=5`, `radiusLG=8`, `radiusXL=12`. The de-facto radii (4 for inline buttons, 6 for gizmo panels) bypass the system. UI-SPEC explicitly says "Keep card radius at 8px or less" — value is met but the *consistency* rule is broken.

**Finding 5.3 — Sidebar locked at 392px (min=max=default).** `PreparePage.qml:29-31`:
```qml
property int sidebarWidth: 392
property int sidebarMinWidth: 392
property int sidebarMaxWidth: 392
```
Phase 74 GAP-MATRIX PREP-SIDEBAR (Severity: Critical) requires a "narrow compact" sidebar and notes the current 390px width is "visibly wider than target." The comment at `PreparePage.qml:1645` says `// ═══ Left sidebar (280px) ═══` — a stale comment that contradicts the actual value, suggesting the 280px target was once intended but never applied. With `min == max == 392`, the user cannot resize it narrower either. **This Critical gap from v3.9 is still open in v5.1.**

**Finding 5.4 — Arbitrary `bottomMargin: 52`.** `PreparePage.qml:3875` (object info bar) and the derived `prepareBottomViewControlsBottomMargin` math (line 25-26) hardcode 52 and 44 and 8 to position floating UI relative to the viewport insets. No `Theme.*` token; values are tuned for one resolution and will not scale with the proposed future `Theme.panelPadding`/density work.

**Finding 5.5 — Inline `Layout.preferredWidth` magic numbers.** CxSpinBox widths of 80 (8×), CxComboBox widths of 100/120/140/160/200, action-rectangle widths of 70/80/90/100/120, all hardcoded. UI-SPEC does not enumerate a width scale, but the absence of any width token means density tuning requires a sweep.

### Pillar 6: Experience Design (2/4)

**Finding 6.1 — Strong disabled-state coverage.** 51 `enabled:` bindings throughout the file (object menu items gated on `canDuplicateSelectedObjects` / `canDeleteSelection` / `canTransformSelection` / `availableGizmoMask & (1 << mode)`, plate menu items gated on `contextPlateIndex >= 0 && plateObjectCount > 0`, etc.). This is the correct pattern and matches UI-SPEC's "Disabled/blocked controls must be honestly disabled" rule. The support paint cursor-type and tool toggles (lines 2046-2065, 2095-2113) do not have disabled states, but they are only visible when the gizmo is active, so this is acceptable.

**Finding 6.2 — Destructive actions fire with no confirmation (BLOCKER).** Four destructive entry points have zero confirmation:
- `deleteSelection` — keyboard Delete/Backspace (line 149) AND right-click menu (lines 279, 480).
- `removeAllOnPlate` — `清空平板` (line 604).
- `deletePlate` — `删除平板` (line 760).
- `clearAllPaintData` — `清除全部` (line 2131).

UI-SPEC Required States includes "blocked/disabled tool with a visible reason" — these go further: they succeed silently with no undo prompt (the project does have `UndoRedoManager` so recovery is technically possible, but the user is not told this). Standard slicer UX (OrcaSlicer upstream `GUI_ObjectList::del_object`) shows a yes/no dialog.

**Finding 6.3 — Async Emboss feedback is wired but invisible-during-run.** Phase 158's async path (`embossSelectedAsync` line 3365) does have completion handlers at lines 3382-3387 (`onEmbossVolumeAdded` posts a notification; `onEmbossVolumeFailed` posts an error). But during the async run there is no BusyIndicator, spinner, or "running…" state — only the static italic placeholder text at line 3370-3377 `(异步执行后状态显示于此)`. A long text can take seconds; the user has no visual signal that anything is happening.

**Finding 6.4 — Session-capture scheduler UX impact (Phase 156).** `PreparePage.qml:3593` `function onStateChanged() { sessionThumbScheduler.restart() }` + `Timer { interval: 250; repeat: false }` (lines 3616-3621). The 250ms debounce coalesces bursts, and the timer is `repeat: false`, so this is correct. However, `captureMissingPlateThumbnails()` (line 3623) loops over every plate and calls `requestThumbnailCapture` for any plate missing a thumbnail — on a 36-plate session (the max) this enqueues 36 GL FBO captures in one frame. The renderer serializes them, but the user may see a brief stall. No visible UX impact under normal conditions but worth profiling.

**Finding 6.5 — Plate-bar thumbnail source fallback is correct.** `PreparePage.qml:3708-3714` prefers the live `lastThumbnailData` for the current plate and falls back to `plateThumbnailBase64(index)` for non-current plates — this matches the Phase 156 write path (`setPlateThumbnailFromBase64` at line 3607). The Phase 158 changes do not touch this path; consistency holds.

**Finding 6.6 — Empty project / no-plate state handled.** Plate bar visibility `root.editorVm.plateCount > 0` (line 3648); viewport warning overlay (lines 1895-1928) covers viewport-warning states; rename/plate-settings dialogs are wired. The SVG-import file-path field at line 3541 is `enabled: false` (placeholder-only) with no upstream path to populate it from the panel — that is a known Phase-deferred control and acceptable.

**Finding 6.7 — Error/toast feedback infrastructure exists** (`backend.postError`, `backend.postNotification` used at lines 665, 675, 684, 693, 3383, 3386) — pattern is consistent and routed through `BackendContext` per the architecture contract.

---

## v5.1 Phase-Specific Verdicts

- **Phase 154 (Compare Presets)** — WARNING. Dialog works but ships in English inside a Chinese app; status badge colors bypass Theme tokens. `SettingsDialog.qml:363-374` Compare button wiring is clean; `requestComparePresets` signal round-trip is correct.
- **Phase 156 (Session-capture scheduler)** — PASS. Timer + debounce + per-plate write path is correctly wired; no visible UX regression; thumbnail fallback chain remains consistent.
- **Phase 158 (Emboss style extension)** — WARNING. New boldness `Slider` should be `CxSlider`; new checkbox row uses `spacing: 12` instead of the panel's `6`; tooltip copy leaks `E mboss.hpp`/`ProjectCurve` implementation detail. Functionality (italic + use-surface + curve-projection persistence) is wired correctly to the ViewModel properties.

---

## Files Audited

- `src/qml_gui/pages/PreparePage.qml` (3965 lines — primary)
- `src/qml_gui/dialogs/PresetDiffDialog.qml` (291 lines — Phase 154)
- `src/qml_gui/dialogs/SettingsDialog.qml` (543 lines — Phase 154/158 changes)
- `src/qml_gui/dialogs/CreatePresetsDialog.qml` (156 lines — visual-consistency comparison)
- `src/qml_gui/dialogs/UnsavedChangesDialog.qml` (152 lines — visual-consistency comparison)
- `src/qml_gui/Theme.qml` (110 lines — token reference)
- `src/qml_gui/controls/CxSpinBox.qml`, `CxSlider.qml`, `CxCheckBox.qml`, `CxIconButton.qml`, `CxButton.qml` (control-library reference)
- `.planning/milestones/v3.9-phases/74-prepare-source-truth-gap-audit/74-UI-SPEC.md` (design baseline)
- `.planning/milestones/v3.9-phases/74-prepare-source-truth-gap-audit/74-GAP-MATRIX.md` (gap baseline)
- `AGENTS.md` (project rules — Theme singleton, qsTr, ASCII-only comments)

**Note on method:** This is a code-only audit (Qt6/QML desktop application; no dev server / Playwright in scope). All findings are derived from source analysis using `command grep` (the shell's `grep` function is intercepted by ZCode's internal search; `command grep` bypasses it). The application is launchable via `build/OWzxSlicer.exe` for optional screenshot evidence in a follow-up round.
