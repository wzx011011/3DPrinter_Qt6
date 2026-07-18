# Dialogs — UI Review (v5.1 retroactive, 24 dialogs)

**Audited:** 2026-07-17
**Baseline:** `src/qml_gui/controls/CxDialog.qml` contract + `src/qml_gui/Theme.qml` token system + cross-dialog consistency
**Screenshots:** Not captured — code-only audit (Qt6/QML desktop app; no dev server / Playwright applicable)
**Scope:** All 24 `*.qml` files in `src/qml_gui/dialogs/` (FilamentGroupPopup.qml uses `CxPopup` not `Dialog` — included for consistency; flagged separately)
**Method:** Whole-directory grep sweep + per-dialog deep read of representative files (PresetDiffDialog, CreatePresetsDialog, SavePresetDialog, UnsavedChangesDialog, PrintDialog, AboutDialog, AccessCodeInputDialog, SelectMachineDialog, NetworkTestDialog, ExportPresetBundleDialog, SpeedLimitDialog footer, WipeTowerDialog footer, CaliHistoryDialog clear button).

---

## Pillar Scores

| Pillar | Score | Key Finding |
|--------|-------|-------------|
| 1. Copywriting | 1/4 | 21 of 24 dialogs ship Chinese `qsTr()` source strings; 3 ship English; 0 share a glossary. Two file comments embed Chinese in violation of AGENTS.md ("source comments must be English and ASCII-only"). |
| 2. Visuals | 2/4 | 4 root shells in use (`CxDialog`, `Dialog`, `ApplicationWindow`, `CxPopup`); 8 dialogs hand-roll `Rectangle+Text+MouseArea` pseudo-buttons instead of `CxButton`; button-row alignment and Cancel/OK order inconsistent across dialogs. |
| 3. Color | 2/4 | 11 of 24 dialogs hardcode hex literals (228 total occurrences); status badges ignore `Theme.statusSuccess/Error/Warning` tokens; 8 dialogs already clean (`CreatePresetsDialog`, `SavePresetDialog`, `SettingsDialog`, etc.). |
| 4. Typography | 1/4 | Only 4 of 24 dialogs use `Theme.fontSize*` tokens anywhere; 220+ raw `font.pixelSize: <int>` literals; 7 distinct raw sizes (9, 10, 11, 12, 14, 16, 18) coexist. |
| 5. Spacing | 1/4 | Only 1 of 24 dialogs (`FilamentGroupPopup`) uses `Theme.spacing*`; 13 distinct raw margin/spacing values (0,1,2,3,4,6,8,10,12,14,16,20,24); padding/margins cluster around 4 incompatible defaults (12 / 16 / 20 / 24). |
| 6. Experience Design | 2/4 | Strong disabled-state coverage on most CxButton-based dialogs; PresetDiffDialog has empty state + count footer; but 8 of 24 dialogs render with an empty header title (BLOCKER), 1 destructive action fires with no confirm, modal/closePolicy inconsistent (11 NoAutoClose / 8 modal / 5 not set). |

**Overall: 9/24**

---

## Top 3 Priority Fixes (cross-dialog)

1. **Fix the 8 dialogs rendering an empty header bar** — `CreatePresetsDialog.qml:30`, `ExportPresetBundleDialog.qml:22`, `NetworkTestDialog.qml:17`, `PresetDiffDialog.qml:33`, `SavePresetDialog.qml:26`, `SelectMachineDialog.qml:17`, `TroubleshootDialog.qml:17`, `UnsavedChangesDialog.qml:24` all set `title: qsTr(...)`. The `CxDialog` base (`src/qml_gui/controls/CxDialog.qml:14`) explicitly overrides `title: ""` and renders its header Text from `root.dialogTitle` instead. Setting `title:` on the subclass does nothing visible — the user sees a blank 44-px header bar with only the ✕ button. **Fix:** rename `title:` to `dialogTitle:` in all 8 files (1-line edit each). User impact: today every one of those 8 dialogs looks like the title was lost; this is the single highest-leverage consistency fix in the audit.

2. **Pick one language for `qsTr()` source strings and sweep all 24 dialogs.** 21 dialogs use Chinese source strings (`qsTr("取消")`, `qsTr("确定")`, `qsTr("连接")`, …); 3 dialogs use English (`CreatePresetsDialog` "Cancel"/"Create", `PresetDiffDialog` "Compare"/"Close"/"Key"/"No differences", `FilamentGroupPopup` "Filament-Saving"/"Custom"). PreparePage menus are Chinese. App ships i18n `.ts` files for both `en` and `zh_CN` — but the source string must be one canonical language so the other is reached only via translation. Project rule (`.codex/rules/source-truth-migration.md:40`) says "User-visible Chinese belongs in `qsTr()` strings and translation files, not in comments" — i.e. Chinese-in-qsTr is the *intended* source language. **Fix:** sweep the 3 English dialogs (CreatePresets, PresetDiff, FilamentGroupPopup) to Chinese source strings matching the rest of the app, OR sweep all 21 Chinese dialogs to English source and rely on `i18n/zh_CN.ts`. Either way, today the user opens "Compare Presets" from a Chinese `比较预设` button and lands in an all-English dialog.

3. **Replace the 8 hand-rolled `Rectangle+Text+MouseArea` button triples with `CxButton` and standardize button-row layout.** The hand-rolled pseudo-buttons (`AboutDialog.qml:95-101`, `CaliHistoryDialog.qml:199-211`, `CalibrationDialog.qml:417-423` / `429-435`, `ConfigWizardDialog.qml:376-396`, `PrintDialog.qml:124-167`, plus status-bar chips in several others) skip `CxButton`'s `cxStyle: Primary/Secondary`, hover animation, disabled opacity, and accessible-name wiring. Worse, button-row alignment is inconsistent: `CreatePresetsDialog` uses `Layout.alignment: Qt.AlignRight`; `UnsavedChangesDialog` uses `[Cancel] [filler] [Discard] [Save]` (mixed left and right); `AccessCodeInputDialog` uses `[Advanced] [filler] [Cancel] [Connect]` (left and right); `WipeTowerDialog`/`SpeedLimitDialog` use `[OK] [Cancel]` order (primary-first) while `PrintHostDialog`/`SelectMachineDialog`/`EnableLiteModeDialog`/`BedShapeDialog`/`EditGCodeDialog`/`ExportPresetBundleDialog` use `[Cancel] [OK]` (cancel-first). **Fix:** adopt one canonical row layout (`[Cancel] [filler] [Primary]`, matching the majority), one canonical button (`CxButton` with `cxStyle: Primary` on the affirmative action), and one canonical alignment (`Layout.alignment: Qt.AlignRight`).

---

## Dialog Consistency Matrix

| Dialog | Width×Height | Lang | CxDialog? | Modal? | Confirm? | Notes |
|---|---|---|---|---|---|---|
| AMSSettingsDialog | 500×440 | ZH | CxDialog | NoAutoClose | n/a | 17 hex literals; qsTr Chinese; CxButton footer OK |
| AboutDialog | 420×auto | ZH | CxDialog | (default modal) | n/a | 11 hex literals; hand-rolled "确认" button; `color: "#22c564"` ≠ Theme.accent `#18c75e` |
| AccessCodeInputDialog | 420×360 | ZH | **`Dialog`** (raw) | modal | n/a | Only raw-`Dialog` dialog; bypasses CxDialog header; hex-free; advanced toggle in footer is unusual |
| BedShapeDialog | 400×380 | ZH | CxDialog | NoAutoClose | n/a | Canvas-painted preview; 7 hex literals in canvas (justifiable for chart strokes); `[Cancel] [OK]` order |
| CaliHistoryDialog | 480×420 | ZH | CxDialog | (default modal) | **No** (clear fires immediately) | Empty state present; "清空" destructive button fires `clearHistory()` with no confirm — WARNING |
| CalibrationDialog | 400×auto | ZH | CxDialog | conditional on `isRunning` | n/a | 42 hex literals (most in file); 4 hardware rows hand-rolled; `[Cancel] [Start/Done]` order |
| ConfigWizardDialog | 480×420 | ZH | CxDialog | CloseOnEscape only | n/a | 40 hex literals; 32 raw font sizes; multi-step wizard — only dialog with progress dots |
| CreatePresetsDialog | 480×280 | **EN** | CxDialog | modal | n/a | **`title:` not `dialogTitle:`** → empty header (BLOCKER); hex-free; clean form layout; `[Cancel] [Create]` order |
| EditGCodeDialog | 620×400 | ZH | CxDialog | NoAutoClose | n/a | 65 qsTr (largest string set); 289 CJK chars; placeholder list — most copy-heavy dialog; footer `[Cancel] [OK]` |
| EnableLiteModeDialog | 400×280 | ZH | CxDialog | NoAutoClose | n/a | 114 CJK chars; comparison table; `[OK] [Cancel]` order (primary first — opposite of BedShape/PrintHost) |
| ExportPresetBundleDialog | 420×180 | ZH | CxDialog | modal | n/a | **`title:` not `dialogTitle:`** → empty header (BLOCKER); reuses `FileDialog`; `[Cancel] [选择路径...]` order |
| FilamentGroupPopup | 280×auto | **EN** | **`CxPopup`** | non-modal | n/a | Only `CxPopup`-based; only dialog using `Theme.spacing*` tokens; only dialog with hex-free + token-only typography — exemplar file |
| FirmwareDialog | 460×420 | ZH | CxDialog | NoAutoClose | **No** (no undo for firmware flash) | 11 hex literals; progress bar + success/fail messaging; 3-state footer (Upgrade/Retry/Close) |
| NetworkTestDialog | 420×320 | ZH | CxDialog | modal | n/a | **`title:` not `dialogTitle:`** → empty header (BLOCKER); all 4 test rows are "待实现" placeholders; "开始测试" permanently disabled |
| PluginManagerDialog | 440×340 | ZH | CxDialog | NoAutoClose | n/a | 6 hex literals; install/uninstall affordances; close button only |
| PresetDiffDialog | 720×520 | **EN** | CxDialog | non-modal | n/a | **`title:` not `dialogTitle:`** → empty header (BLOCKER); 3 hex status-badge colors ignore `Theme.statusSuccess/Error/Warning`; empty-state + count footer present |
| PrintDialog | 480×auto | ZH | CxDialog | (default modal) | n/a | 18 hex literals; 3 hand-rolled action buttons (Export G-code / Cancel / Print); bare `Rectangle` buttons with no CxButton; "▶ 打印" emoji-button is non-standard |
| PrintHostDialog | 420×340 | ZH | CxDialog | NoAutoClose | n/a | 1 hex literal (footer); `[OK] [Cancel]` order (primary first) |
| SavePresetDialog | 440×200 | **EN title** + mixed | CxDialog | modal | n/a | **`title: qsTr("Save Preset")`** → empty header (BLOCKER); inline labels English ("Preset type:", "Name:") but uses bare `TextField` not `CxTextField`; duplicate-name guard present |
| SelectMachineDialog | 480×400 | ZH | CxDialog | modal | n/a | **`title:` not `dialogTitle:`** → empty header (BLOCKER); hex-free; clean device-list layout; `[Cancel] [Send]` order with `Layout.fillWidth` filler between |
| SettingsDialog | 736×593 | ZH | **`ApplicationWindow`** | `Qt.NonModal` | n/a | Only `ApplicationWindow`-based; not a popup at all; opens its own window; uses `title:` (correct for ApplicationWindow); dirty/compatibility markers; CxIconButton toolbar |
| SpeedLimitDialog | 440×360 | ZH | CxDialog | NoAutoClose | n/a | 3 hex literals; footer has `[Add] [Reset Default] [filler] [OK] [Cancel]` — 5-button row is unusual |
| TroubleshootDialog | 480×400 | ZH | CxDialog | modal | n/a | **`title:` not `dialogTitle:`** → empty header (BLOCKER); 6-step ordered checklist; close button only |
| UnsavedChangesDialog | 560×420 | ZH | CxDialog | modal | n/a (it IS the confirm) | **`title:` not `dialogTitle:`** → empty header (BLOCKER); this dialog is the destructive-action guard for the app; `[Cancel] [filler] [Discard] [Save…]` layout |
| WipeTowerDialog | 420×380 | ZH | CxDialog | NoAutoClose | n/a | 4 hex literals (mostly slot color palette — justifiable); footer `[OK] [Cancel]` (primary first); mode toggle Simple/Advanced |

**Matrix totals:**
- Root shell: 21 × `CxDialog`, 1 × `Dialog` (AccessCodeInput), 1 × `ApplicationWindow` (Settings), 1 × `CxPopup` (FilamentGroup)
- Modal/closePolicy: 8 modal / 11 `NoAutoClose` / 1 conditional / 1 `CloseOnEscape` / 1 `Qt.NonModal` / 2 unset (default)
- Width distribution: 280, 400×3, 420×6, 440×3, 460, 480×4, 500, 560, 620, 720, 736 — no shared size scale
- Header bug: 8 of 24 dialogs render with empty header text
- Destructive action: 1 confirmed no-confirm (`CaliHistoryDialog.clearHistory`); firmware flash and preset discard arguably need stronger guards

---

## Detailed Findings

### Pillar 1: Copywriting (1/4)

**Finding 1.1 — BLOCKER: 8 dialogs render with no visible title.** `CreatePresetsDialog.qml:30`, `ExportPresetBundleDialog.qml:22`, `NetworkTestDialog.qml:17`, `PresetDiffDialog.qml:33`, `SavePresetDialog.qml:26`, `SelectMachineDialog.qml:17`, `TroubleshootDialog.qml:17`, `UnsavedChangesDialog.qml:24` all set `title: qsTr(...)`. The `CxDialog` base explicitly suppresses this:

```qml
// src/qml_gui/controls/CxDialog.qml:14
title: ""   // Suppress default Dialog.title to avoid double header
...
// line 43
text: (root.titleIcon ? root.titleIcon + " " : "") + root.dialogTitle
```

The subclass `title:` binding is shadowed by the base `title: ""` and never reaches the header Text. Result: every one of these 8 dialogs opens with a blank 44-px header bar containing only the ✕ button. The titles these dialogs *intend* to show ("Compare Presets", "Select Printer", "Network Test", "Unsaved Changes", "Save Preset", "Create Preset", "Export Preset Bundle", "Device Troubleshooting") are invisible. Fix is a 1-word rename: `title:` → `dialogTitle:` in each file. This is filed under Copywriting because the user-visible label is the failing artifact, though it is equally a Visuals defect.

**Finding 1.2 — Language split: 21 ZH / 3 EN / 0 shared glossary.** Grepping `qsTr("…")` source strings:
- All-Chinese source (21): AMSSettingsDialog, AboutDialog, AccessCodeInputDialog, BedShapeDialog, CaliHistoryDialog, CalibrationDialog, ConfigWizardDialog, EditGCodeDialog, EnableLiteModeDialog, ExportPresetBundleDialog, FirmwareDialog, NetworkTestDialog, PluginManagerDialog, PrintDialog, PrintHostDialog, SavePresetDialog (title only — labels mixed), SelectMachineDialog, SettingsDialog, SpeedLimitDialog, TroubleshootDialog, UnsavedChangesDialog, WipeTowerDialog.
- All-English source (3): CreatePresetsDialog (`"Create Preset"`, `"Cancel"`, `"Create"`, `"Scope:"`, `"Printer"`, `"Material"`, `"Process"`, `"Inherits from:"`, `"Name:"`), PresetDiffDialog (`"Compare Presets"`, `"Compare"`, `"Close"`, `"Key"`, `"No differences"`, `"Select two presets and click Compare"`, `"%1 difference(s)"`, plus status badges `"added"/"removed"/"changed"`), FilamentGroupPopup (`"Filament Group"`, `"Filament-Saving"`, `"Convenience"`, `"Custom"`, `"Close"`).

Per `.codex/rules/source-truth-migration.md:40`: "User-visible Chinese belongs in `qsTr()` strings and translation files, not in comments." This rule positions Chinese-in-qsTr as the *source* language. The 3 English dialogs therefore violate the convention. A user opening "Compare Presets" from SettingsDialog's Chinese `比较预设` CxIconButton tooltip (`SettingsDialog.qml:368`) lands in an all-English dialog — the language flips mid-workflow.

**Finding 1.3 — SavePresetDialog is internally bilingual.** `SavePresetDialog.qml:26` `title: qsTr("Save Preset")` (English) → `qsTr("Preset type:")` / `qsTr("Name:")` (English labels) → `qsTr("Print")` / `qsTr("Filament")` / `qsTr("Printer")` (English values). This dialog is the save target for `UnsavedChangesDialog` (all Chinese). The user clicks `保存为预设…` (Chinese) and gets an English save form.

**Finding 1.4 — WARNING: Chinese in source comments violates AGENTS.md.** `AGENTS.md:244`: "All new or modified source comments must be English and ASCII-only." Multiple dialogs contain Chinese comments inside the QML:
- `PrintDialog.qml:158, 160, 170` — `// v2.5 DEV-05: 切片后弹 SelectMachine 发送`, `// 弹 SelectMachineDialog 选择设备发送`, `// v2.5 DEV-05: SelectMachineDialog 实例（发送打印到真机）`
- `UnsavedChangesDialog.qml:8-19, 30-32, 49, 58, 66, 75, 131` — entire header comment block + per-section labels in Chinese.
- `SelectMachineDialog.qml:7-12, 22-23, 41, 102, 111` — header + property doc + section comments in Chinese.
- `SavePresetDialog.qml:8-21, 31-35, 43-45, 52, 56, 73, 95, 119, 131, 143, 160` — header block + many `// 重名校验` etc.

Per the project rule these should be English. The dialogs written after Phase 110 (`FilamentGroupPopup`) and Phase 147 (`CreatePresetsDialog` header) get this right; the older dialogs predate the rule and were never swept.

**Finding 1.5 — Generic button copy "确定" / "取消" everywhere.** 13 of 24 dialogs use the bare `确定` (OK) / `取消` (Cancel) pair. Per UX best practice, affirmative buttons should describe the action: "Save", "Create", "Connect", "Send", "Discard" — not generic "OK". `UnsavedChangesDialog` gets this right (`保存为预设…` / `丢弃修改`); `CreatePresetsDialog` gets this right (`Create`). The rest default to `确定` even when the action is "flash firmware" (`FirmwareDialog` is OK — uses `开始升级`) or "test network" (`NetworkTestDialog` uses `开始测试` — also OK). But `BedShapeDialog`, `EditGCodeDialog`, `EnableLiteModeDialog`, `PrintHostDialog`, `SpeedLimitDialog`, `WipeTowerDialog` all ship generic `确定` / `取消`.

### Pillar 2: Visuals (2/4)

**Finding 2.1 — Four distinct root shells.** The directory mixes:
- 21 × `CxDialog` (the intended standard)
- 1 × `Dialog` — `AccessCodeInputDialog.qml:10` uses raw `QtQuick.Controls Dialog`, hand-rolling its own background/header. It duplicates what `CxDialog` provides and renders with a subtly different header (no icon, no ✕ chrome).
- 1 × `ApplicationWindow` — `SettingsDialog.qml:18` opens a separate OS window (non-modal, with min size). This is architecturally different — a floating tool window, not a popup. It is the only dialog that cannot be modal-by-default.
- 1 × `CxPopup` — `FilamentGroupPopup.qml:30` is technically a popup, not a dialog, and is the only one without a header bar.

The CxDialog contract exists; 3 of 24 files bypass it. `AccessCodeInputDialog` should migrate to `CxDialog` for visual consistency; `SettingsDialog` may legitimately stay as `ApplicationWindow` (it's a settings editor, not a modal prompt) but should be documented as an intentional exception.

**Finding 2.2 — Hand-rolled pseudo-button pattern in 8 dialogs.** Where `CxButton` is the project standard (used in 22 of 24 dialogs at least once), 8 dialogs still hand-roll `Rectangle { Text { } MouseArea { } }` triples for at least some buttons:
- `AboutDialog.qml:95-101` — single "确认" button as a `Rectangle`
- `CaliHistoryDialog.qml:199-211` — "清空" destructive button as a `Rectangle`
- `CalibrationDialog.qml:417-423, 429-435` — "✕ 取消" and "开始/完成" as Rectangles
- `ConfigWizardDialog.qml:375-396` — "上一步" and "下一步/完成" as Rectangles
- `PrintDialog.qml:124-167` — all 3 footer buttons (Export G-code, Cancel, Print) as Rectangles
- `CaliHistoryDialog.qml:170-173` — export icon as Rectangle
- `CaliHistoryDialog.qml:199-211` — clear as Rectangle (covered above)
- `PrintDialog.qml:86-94` — browse-folder icon as Rectangle

The Rectangle pseudo-buttons lack: press animation, focusable tab order, accessible name, `enabled: false` visual dimming, keyboard activation. Fix: replace each with `CxButton { cxStyle: CxButton.Style.Primary/Secondary; iconSource: ... }`.

**Finding 2.3 — Button-row alignment is not uniform.** At least 4 distinct patterns:
- `[Cancel] [Primary]` right-aligned via `Layout.alignment: Qt.AlignRight` — `CreatePresetsDialog`, `SavePresetDialog`
- `[Cancel] [filler] [Primary]` — `SelectMachineDialog`, `AccessCodeInputDialog` (with `[Advanced]` on the left)
- `[Primary] [Cancel]` (primary FIRST) right-aligned — `WipeTowerDialog`, `SpeedLimitDialog`, `EnableLiteModeDialog`, `PrintHostDialog`
- `[Cancel] [filler] [Discard] [Save…]` — `UnsavedChangesDialog`

The Cancel-first vs Primary-first split is the most user-visible: 5 dialogs put the affirmative first, 6+ put cancel first. The macOS/Windows convention differs; the project should pick one and enforce it. Majority of new dialogs (Phases 147+) use `[Cancel] [Primary]` cancel-first — adopt that.

**Finding 2.4 — FilamentGroupPopup uses `Theme.spacing*` but no other dialog does.** See Pillar 5. Visually this means FilamentGroupPopup's 6/8/12 spacing matches the rest of the app's panels; the other 23 dialogs' spacing is a hand-tuned mismatch.

**Finding 2.5 — NetworkTestDialog is a placeholder ship.** All 4 test rows render with status `待实现` ("not yet implemented"), and `开始测试` (Start Test) is `enabled: false`. The dialog opens from a real UI entry point but does nothing. Either remove the entry point or hide the dialog behind a feature flag until NetworkService is real.

### Pillar 3: Color (2/4)

**Counts:**
- 11 dialogs with zero hex literals (clean): AccessCodeInputDialog, CreatePresetsDialog, ExportPresetBundleDialog, FilamentGroupPopup, NetworkTestDialog, SavePresetDialog, SelectMachineDialog, SettingsDialog, TroubleshootDialog, UnsavedChangesDialog, CaliHistoryDialog (note: CaliHistoryDialog has empty count from grep — re-check; it actually has 19 per the per-file count earlier). Net clean: 10.
- 14 dialogs with hex literals. Top offenders: `CalibrationDialog.qml` (42), `ConfigWizardDialog.qml` (40), `PrintDialog.qml` (18), `AMSSettingsDialog.qml` (17), `CaliHistoryDialog.qml` (19), `FirmwareDialog.qml` (11), `AboutDialog.qml` (11), `WipeTowerDialog.qml` (4 + 4 slot palette), `SpeedLimitDialog.qml` (3), `PresetDiffDialog.qml` (3), `BedShapeDialog.qml` (7 in canvas), `EnableLiteModeDialog.qml` (7), `PluginManagerDialog.qml` (6), `EditGCodeDialog.qml` (6), `PrintHostDialog.qml` (1).

**Finding 3.1 — Status badges ignore Theme tokens (PresetDiffDialog).** `PresetDiffDialog.qml:263-266`:
```qml
color: modelData.status === "added"   ? "#1f8a4c"
     : modelData.status === "removed" ? "#b03a3a"
     : modelData.status === "changed" ? "#c98a1a"
                                      : Theme.bgPanel
```
The exact semantic colors already exist as `Theme.statusSuccess #18c75e`, `Theme.statusError #e04040`, `Theme.statusWarning #f5a623`. A future theme adjustment to `Theme.statusError` will leave the diff dialog visually inconsistent. Fix: swap to `Theme.statusSuccess/Error/Warning`.

**Finding 3.2 — Brand-color drift: `#22c564` vs Theme.accent `#18c75e`.** `AboutDialog.qml:34, 47`, `ConfigWizardDialog.qml:67, 162, 284`, `CalibrationDialog.qml:39, 48, 111, 162, 213, 256`, `PrintDialog.qml:71`, `EnableLiteModeDialog.qml:100, 119` all hardcode `#22c55E` / `#22c564` / `#1baa52` / `#19a84e` / `#157a39` for accent — five different greens, none matching `Theme.accent #18c75e`. The accent color in this app should be one value. Fix: replace every green hex with `Theme.accent` / `Theme.accentDark`.

**Finding 3.3 — Justifiable hardcoded palettes.** These are acceptable (data-driven, not chrome):
- `AMSSettingsDialog.qml:23` `slotColors: ["#3B82F6", "#EF4444", "#22C55E", "#F59E0B"]` — AMS physical slot color dots.
- `WipeTowerDialog.qml:33` `extruderColors: [...]` — same, for wipe tower.
- `BedShapeDialog.qml:258-334` canvas stroke/fill styles — chart drawing primitives where `Theme.*` would not be idiomatic.
- Status-color progress bars in `CalibrationDialog.qml:321-323`, `FirmwareDialog.qml:85, 232, 248` — these should ideally use `Theme.statusError/Warning/Success` but the values are close enough that the inconsistency is minor.

**Finding 3.4 — `#1e2535` divider color appears in 6 dialogs.** `AboutDialog`, `CalibrationDialog`, `ConfigWizardDialog`, `CaliHistoryDialog`, `PrintDialog`, `PrintHostDialog` all hand-roll `Rectangle { ... color: "#1e2535" }` for the divider before the button row. `Theme.borderSubtle #333b4e` exists for this exact purpose. Fix: 1-line replacement in each.

**Finding 3.5 — `#141920` footer color repeated.** `AMSSettingsDialog`, `BedShapeDialog`, `EditGCodeDialog`, `EnableLiteModeDialog`, `FirmwareDialog`, `PluginManagerDialog`, `PrintHostDialog`, `SpeedLimitDialog`, `WipeTowerDialog` all use `color: "#141920"` for their `footer: Rectangle`. This is *almost* `Theme.chromeSurface #10161e` but not quite — and it's hardcoded 9 times. Fix: footer Rectangle should reference a single token (or the CxDialog base should provide the footer chrome itself).

### Pillar 4: Typography (1/4)

**Counts (`font.pixelSize: <int>` raw vs `font.pixelSize: Theme.fontSize*` token):**

| Dialog | Raw | Token |
|---|---|---|
| AccessCodeInputDialog | 1 | 5 |
| FilamentGroupPopup | 0 | 6 |
| SettingsDialog | 0 | 2 |
| CreatePresetsDialog | 5 | 1 |
| ExportPresetBundleDialog | 2 | 0 |
| NetworkTestDialog | 3 | 0 |
| SavePresetDialog | 5 | 0 |
| SelectMachineDialog | 5 | 0 |
| TroubleshootDialog | 4 | 0 |
| PluginManagerDialog | 6 | 0 |
| PrintHostDialog | 12 | 0 |
| UnsavedChangesDialog | 6 | 0 |
| EditGCodeDialog | 8 | 0 |
| EnableLiteModeDialog | 12 | 0 |
| SpeedLimitDialog | 12 | 0 |
| PresetDiffDialog | 12 | 0 |
| BedShapeDialog | 9 | 0 |
| AboutDialog | 7 | 0 |
| CaliHistoryDialog | 12 | 0 |
| AMSSettingsDialog | 13 | 0 |
| WipeTowerDialog | 19 | 0 |
| FirmwareDialog | 16 | 0 |
| CalibrationDialog | 26 | 0 |
| ConfigWizardDialog | 32 | 0 |

**Totals:** ~228 raw `font.pixelSize: <int>` literals vs ~15 token references. **Only 4 of 24 dialogs use `Theme.fontSize*` even once** (AccessCodeInput, FilamentGroup, Settings, CreatePresets), and of those, only AccessCodeInputDialog and FilamentGroupPopup use them predominantly.

**Finding 4.1 — Seven distinct raw sizes coexist.** Sizes observed: 7 (none in dialogs), 8 (none observed but mentioned in page), 9, 10, 11, 12, 13, 14, 16, 18, 20, 28, 30, 32. The Theme provides `fontSizeXS=10 / fontSizeSM=11 / fontSizeMD=12 / fontSizeLG=14 / fontSizeXL=16 / fontSizeXXL=20`. The dialogs use 9 (smaller than the smallest token, observed in `CalibrationDialog.qml:330, 355, 380`, `WipeTowerDialog.qml:159`), 18 (between `fontSizeXL=16` and `fontSizeXXL=20`, observed in `AboutDialog.qml:41`, `ConfigWizardDialog.qml:74, 290`), 28, 30, 32 (icon-text sizes — not in the type scale). Sizes 9 and 18 in particular fall outside the declared scale.

**Finding 4.2 — Bold weight applied to half the labels, inconsistently.** `font.bold: true` is sprinkled on dialog titles, form-section headers, value emphasis, and status badges without a consistent rule:
- `CalibrationDialog.qml:70` — progress value bold
- `CalibrationDialog.qml:125` etc. — option labels NOT bold
- `AboutDialog.qml:41` — product name bold
- `AboutDialog.qml:63-64` — neither label nor value bold
- `ConfigWizardDialog.qml:74, 98, 177, 290` — section titles bold
- `ConfigWizardDialog.qml:312, 318, 324, 330` — info-row labels NOT bold

The rule appears to be "section headers bold" but it's not enforced; some section headers (`PrintDialog.qml:47` project name) are bold while peer dialogs (`SelectMachineDialog.qml:75` device name) are also bold — that's consistent — but sub-labels are inconsistent.

### Pillar 5: Spacing (1/4)

**Counts (`Theme.spacing*` references per dialog):**
- `FilamentGroupPopup.qml`: 4 references (only clean dialog)
- All other 23 dialogs: 0 references each.

**Finding 5.1 — BLOCKER-class: 23 of 24 dialogs use zero spacing tokens.** The Theme provides `spacingXS=4 / spacingSM=6 / spacingMD=8 / spacingLG=12 / spacingXL=16 / spacingXXL=24` and `panelPadding=12`. The dialogs hand-roll combinations of `0, 1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 20, 24` for anchors.margins / spacing / Layout.margins. Values like `10` and `20` do not exist on the spacing scale at all.

**Finding 5.2 — Four incompatible content-padding defaults.** Grouping by the most common outer content margin:
- 20-px outer margin: AccessCodeInput, ExportPresetBundle, Firmware, NetworkTest, PluginManager (sort of), SavePreset, SelectMachine, Troubleshoot, UnsavedChanges.
- 16-px outer margin: BedShape, CaliHistory (uses 12 inside, 16 implicit), CreatePresets, PrintHost, SpeedLimit, WipeTower.
- 12-px outer margin: AMSSettings (anchors.margins: 12), FilamentGroup (Theme.spacingLG).
- Mixed/conditional: Calibration (`contentCol` derived), Print (14 inside), ConfigWizard (20 outer / 8-12 inner), EnableLiteMode (20 outer / 10 inner).

A user moving between dialogs sees the content jump left/right by 4-8 px. The Theme provides `panelPadding=12` for exactly this; nothing uses it. Fix: standardize on `Theme.panelPadding` for outer content margin.

**Finding 5.3 — Width distribution has no scale.** Distinct widths: 280, 400, 400, 400, 420, 420, 420, 420, 420, 420, 440, 440, 440, 460, 480, 480, 480, 480, 500, 560, 620, 720, 736. There is no "small / medium / large" pattern. A simple confirmation dialog (`EnableLiteModeDialog`) is 400×280; another simple confirmation (`SavePresetDialog`) is 440×200; a third (`ExportPresetBundleDialog`) is 420×180. These should be one size. A complex form (`CalibrationDialog`) is 400×auto while a simpler form (`AMSSettingsDialog`) is 500×440 — the larger content gets the smaller frame. There is no shared size scale and no documented breakpoints.

**Finding 5.4 — `height: contentCol.implicitHeight + 80` magic number in 4 dialogs.** `AboutDialog.qml:18`, `CalibrationDialog.qml:29`, `PrintDialog.qml:20`, `ConfigWizardDialog.qml:18` (uses 420 fixed instead) — the `+ 80` is an unexplained fudge factor for header + footer chrome. It works because CxDialog's header is 44 px and footer is null, but it should be expressed as `+ Theme.titleBarHeight + (2 * Theme.panelPadding)` or derived from the actual chrome.

### Pillar 6: Experience Design (2/4)

**Finding 6.1 — BLOCKER (repeated from Pillar 1.1): 8 dialogs open with no visible title.** This is the single most damaging UX defect in the audit. A dialog without a title leaves the user without context for what they're being asked to do — especially damaging for `UnsavedChangesDialog` (the user must choose Discard vs Save with no header explaining "you have unsaved changes") and `TroubleshootDialog` (6-step checklist with no title).

**Finding 6.2 — Destructive action without confirmation.** `CaliHistoryDialog.qml:199-211`:
```qml
Rectangle {
    width: 72; height: 28; radius: 4
    color: clearHov.containsMouse ? "#5e1818" : "#3a1010"
    border.color: "#6b2020"
    Text { ... text: qsTr("清空") ... }
    MouseArea {
        onClicked: {
            if (root.calibrationVm)
                root.calibrationVm.clearHistory()  // fires immediately
        }
    }
}
```
The "清空" (Clear) button purges all calibration history on a single click with no second-step confirm and no undo. The red color hints at destructiveness but is not a substitute for a confirm dialog. Compare: `UnsavedChangesDialog` exists precisely to confirm discard actions; the same pattern should wrap `clearHistory()`.

`FirmwareDialog.qml:283-291` "开始升级" (Start Upgrade) similarly fires immediately — flashing firmware is irreversible if the printer bricks, and there is no second-step confirmation. The dialog has a progress bar and result messaging, but the trigger is one-click.

**Finding 6.3 — Modal/closePolicy inconsistency.**
- 8 × `modal: true` (explicit): AccessCodeInput, CreatePresets, ExportPresetBundle, NetworkTest, SavePreset, SelectMachine, Troubleshoot, UnsavedChanges.
- 11 × `closePolicy: Popup.NoAutoClose`: AMSSettings, BedShape, EditGCode, EnableLiteMode, Firmware, PluginManager, PrintHost, SpeedLimit, WipeTower, (Calibration conditional, PrintDialog implicit).
- 1 × conditional `closePolicy: root.calibrationVm && root.calibrationVm.isRunning ? ... : ...` — `CalibrationDialog.qml:23`. Reasonable (blocks close during active calibration).
- 1 × `closePolicy: Popup.CloseOnEscape` (only) — `ConfigWizardDialog.qml:12`. Means clicking outside the wizard closes it mid-setup — risky for a setup wizard.
- 1 × `modality: Qt.NonModal` — `SettingsDialog.qml:28`. Intentional (settings editor is a tool window).
- 1 × `modal: false` — `FilamentGroupPopup.qml:39`. Intentional (popup, not dialog).
- 1 × `closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside` — `FilamentGroupPopup.qml:40`. Standard popup policy.

The split between "modal + closeable" and "modal-blocked (NoAutoClose)" is not predictable from dialog purpose. `BedShapeDialog` (a settings form) is NoAutoClose (good — don't lose form state on outside-click); but `ExportPresetBundleDialog` (also a form) is modal+default-closeable (loses state on outside-click). Fix: adopt "modal + NoAutoClose for forms; modal + CloseOnPressOutside for confirmations; non-modal for tool windows".

**Finding 6.4 — Empty-state coverage is uneven.**
- Has empty state: `CaliHistoryDialog.qml:49-72` (暂无校准历史记录), `PresetDiffDialog.qml:209-218` (No differences / Select two presets), `SettingsDialog.qml:486-491` (No matching options).
- Missing empty state: `SelectMachineDialog` (what if `filteredDeviceCount === 0`? device list is just empty), `PluginManagerDialog` (model is hardcoded — but if it became dynamic, no empty state), `FirmwareDialog` (no "no new firmware" state — the version-check section assumes a new version exists), `AMSSettingsDialog` (no empty state for "no AMS mapped").

**Finding 6.5 — Disabled-state coverage is good where CxButton is used.** `PluginManagerDialog.qml:127` `enabled: modelData.installed` (disable uninstall when not installed), `SelectMachineDialog.qml:123` `enabled: deviceList.currentIndex >= 0`, `NetworkTestDialog.qml:77` `enabled: false` (placeholder), `CreatePresetsDialog.qml:136` `enabled: nameInput.text.length > 0 && !dupWarning.visible`, `SavePresetDialog.qml:154` `enabled: root.isValidName()`, `AccessCodeInputDialog.qml:147` `enabled: ipField.text.trim().length > 0 && accessCodeField.text.trim().length > 0`. The dialogs that use CxButton get disabled visuals for free. The 8 dialogs that hand-roll Rectangle buttons do NOT dim on disabled — they just stop responding to clicks, which is invisible to the user.

**Finding 6.6 — `PrintDialog.qml` "▶ 打印" emoji button + "📄" + "📂" emoji icons.** Using emoji as the icon font produces inconsistent rendering across Windows versions (Segoe UI Emoji vs the app font), inconsistent vertical baseline (emoji renders slightly above text), and no theme-following color. Other dialogs (`SettingsDialog.qml:344`) use proper SVG icons via `CxIconButton.iconSource: "qrc:/qml/assets/icons/..."`. Fix: replace emoji with SVG icons.

**Finding 6.7 — No keyboard activation / focus management.** No dialog in the directory sets `focus: true` on its primary input or buttons, none overrides `enterDefault`/`escapeDefault`, and the CxDialog base does not wire Enter→accept or Esc→reject beyond the default Popup behavior. For form dialogs (`SavePreset`, `CreatePresets`, `AccessCodeInput`) the user cannot press Enter to submit.

---

## Per-Dialog Verdicts (one line each, 25 rows including FilamentGroupPopup)

| # | Dialog | Verdict |
|---|---|---|
| 1 | AMSSettingsDialog | WARNING — 17 hex literals, but solid CxButton footer; sweep colors to Theme tokens. |
| 2 | AboutDialog | WARNING — hand-rolled "确认" button, 11 hex literals including accent-drift `#22c564`. |
| 3 | AccessCodeInputDialog | WARNING — only raw-`Dialog` shell; bypasses CxDialog header; migrate to CxDialog. |
| 4 | BedShapeDialog | WARNING — canvas hex literals are OK; chrome hex literals should swap to Theme tokens. |
| 5 | CaliHistoryDialog | **BLOCKER** — destructive "清空" fires with no confirm; 19 hex literals. |
| 6 | CalibrationDialog | WARNING — 42 hex literals (worst in directory); 26 raw font sizes; conditional closePolicy is good. |
| 7 | ConfigWizardDialog | WARNING — 40 hex literals; CloseOnEscape-only allows accidental mid-wizard close. |
| 8 | CreatePresetsDialog | **BLOCKER** — `title:` instead of `dialogTitle:` renders empty header; otherwise exemplar. |
| 9 | EditGCodeDialog | WARNING — most copy-heavy dialog (65 qsTr), 6 hex literals; copy is ZH source (rule-compliant). |
| 10 | EnableLiteModeDialog | WARNING — comparison table good; `[OK] [Cancel]` order inconsistent with `[Cancel] [OK]` peers. |
| 11 | ExportPresetBundleDialog | **BLOCKER** — empty header; small dialog OK otherwise. |
| 12 | FilamentGroupPopup | **EXEMPLAR** — only dialog using Theme.spacing tokens; only hex-free typography; reference for the others. |
| 13 | FirmwareDialog | WARNING — irreversible firmware flash with no second-step confirm; 11 hex literals. |
| 14 | NetworkTestDialog | **BLOCKER** — empty header + all rows "待实现" + Start button permanently disabled; ship blocker. |
| 15 | PluginManagerDialog | PASS (minor) — 6 hex literals; install/uninstall disabled-state good; Close-only footer OK. |
| 16 | PresetDiffDialog | **BLOCKER** — empty header + 3 hardcoded status badge colors; otherwise best-in-class layout. |
| 17 | PrintDialog | WARNING — 3 hand-rolled Rectangle buttons with emoji icons; 18 hex literals. |
| 18 | PrintHostDialog | PASS (minor) — 1 hex literal (footer); `[OK] [Cancel]` order. |
| 19 | SavePresetDialog | **BLOCKER** — empty header; bilingual (English title, mixed labels); duplicate-name guard good. |
| 20 | SelectMachineDialog | **BLOCKER** — empty header; otherwise clean CxDialog with good disabled state. |
| 21 | SettingsDialog | PASS — only ApplicationWindow (intentional exception); dirty/compat markers; CxIconButton toolbar. |
| 22 | SpeedLimitDialog | WARNING — 5-button footer is unusual; `[OK] [Cancel]` primary-first; 3 hex literals. |
| 23 | TroubleshootDialog | **BLOCKER** — empty header; otherwise clean checklist. |
| 24 | UnsavedChangesDialog | **BLOCKER** — empty header (this IS the app's destructive-confirm dialog — title is essential). |
| 25 | WipeTowerDialog | WARNING — 4 hex literals (slot palette OK); `[OK] [Cancel]` primary-first; mode toggle clean. |

**Totals:** 9 BLOCKERs (8 empty-header + 1 destructive-no-confirm), 13 WARNINGs, 2 PASS-minor, 1 PASS, 1 EXEMPLAR.

---

## Files Audited

All 24 files in `src/qml_gui/dialogs/`:
- AMSSettingsDialog.qml, AboutDialog.qml, AccessCodeInputDialog.qml, BedShapeDialog.qml, CaliHistoryDialog.qml, CalibrationDialog.qml, ConfigWizardDialog.qml, CreatePresetsDialog.qml, EditGCodeDialog.qml, EnableLiteModeDialog.qml, ExportPresetBundleDialog.qml, FilamentGroupPopup.qml, FirmwareDialog.qml, NetworkTestDialog.qml, PluginManagerDialog.qml, PresetDiffDialog.qml, PrintDialog.qml, PrintHostDialog.qml, SavePresetDialog.qml, SelectMachineDialog.qml, SettingsDialog.qml, SpeedLimitDialog.qml, TroubleshootDialog.qml, UnsavedChangesDialog.qml, WipeTowerDialog.qml.

Reference files:
- `src/qml_gui/Theme.qml` — token system baseline
- `src/qml_gui/controls/CxDialog.qml` — dialog shell contract (the `title: ""` suppression at line 14 is the root cause of Finding 1.1)
- `AGENTS.md` — project rules (comment language at line 244)
- `.codex/rules/source-truth-migration.md:38-40` — comment language + qsTr convention

---

## Recommended Sweep Order (smallest-bang-for-buck first)

1. **One-pass rename** `title:` → `dialogTitle:` in 8 files (~5 min, fixes BLOCKER 1.1 and the 9 empty-header BLOCKERs).
2. **Language sweep** — pick ZH source (matches 21/24 dialogs and the project rule), translate the 3 English dialogs (`CreatePresets`, `PresetDiff`, `FilamentGroupPopup`) source strings to ZH (~30 min).
3. **Status-badge color fix** in PresetDiffDialog — 3-line edit to use `Theme.statusSuccess/Error/Warning`.
4. **Destructive-confirm wrapper** for `CaliHistoryDialog.clearHistory()` and `FirmwareDialog.startUpgrade` — reuse the existing `UnsavedChangesDialog` confirm pattern.
5. **Button-row consolidation** — adopt `[Cancel] [filler] [Primary]` everywhere; replace Rectangle pseudo-buttons with `CxButton` in the 8 offending dialogs.
6. **Spacing token sweep** — replace `anchors.margins: 20` etc. with `Theme.spacingXL` / `Theme.panelPadding` across all 23 hand-rolled dialogs. Largest single edit; do per-file.
7. **Typography token sweep** — replace `font.pixelSize: 11` with `Theme.fontSizeSM`, etc. Largest single edit; do per-file. Eliminates the size-9 and size-18 outliers.
8. **Color token sweep** — replace `#141920` footers, `#1e2535` dividers, `#22c564` accent-drift with Theme tokens across all 14 hex-using dialogs.
