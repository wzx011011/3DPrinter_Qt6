# Phase 85 UI-SPEC: Settings Shell And Tab Layout Restoration

**Status:** Approved for planning
**Scope:** printer, material, and process parameter settings dialog shell only

## Visual Contract

The settings dialog must read like the target screenshots: a compact dark
operational window, not a nested settings page.

- Window: independent non-modal `ApplicationWindow`, 736x593, native window
  title and native close affordance.
- Surface: dark gray chrome/header over dark base content, local to
  `SettingsDialog.qml`.
- Density: compact 44px top preset/action row and compact horizontal tab strip.
- Body: single main option pane with right vertical scrollbar; no visible left
  group navigation sidebar.
- No marketing spacing, cards-inside-cards, hero treatment, or broad theme
  overhaul.

## Copy Contract

All visible shell strings must be clean UTF-8 and `qsTr()` wrapped where
appropriate.

Titles:

- Printer: `打印机设置`
- Material: `材料设置`
- Process: `工艺设置`

Printer tabs:

- `基础信息`
- `打印机G-code`
- `材料`
- `挤出机`
- `移动能力`
- `注释`

Material tabs:

- `耗材丝`
- `冷却`
- `参数覆盖`
- `高级`
- `材料`
- `依赖`
- `注释`

Process tabs:

- `质量`
- `强度`
- `速度`
- `支撑`
- `底板`
- `冷却`
- `回抽`
- `其他`

## Interaction Contract

- Preset combo remains the dominant left-side control and keeps existing
  per-tier preset selection behavior.
- Save and Save As remain operable but become compact icon actions with
  tooltips, not text buttons.
- Search is a compact action by default and reveals a small field when active
  or when text is present.
- Advanced/simple mode remains operable as a compact toggle-like control.
- Dirty and compatibility status remain visible without expanding the top row.
- Escape and native close use the existing dirty-close guard.
- No duplicate in-row close button is visible.

## Component Contract

- Reuse `CxComboBox`, `CxIconButton`, `CxTextField`, `CxSwitch`, and existing
  icon assets.
- Do not introduce new custom SVGs for save/search/mode when existing assets
  can carry the affordance.
- Keep backend semantics in `ConfigViewModel` and existing dialogs.
- Keep QML responsibilities limited to presentation and event wiring.

## Test Contract

Source audits must lock the restored shell:

- clean title/tab labels are present;
- mojibake strings in the Phase 85 shell are absent;
- `GroupNavSidebar` is not instantiated inside `SettingsDialog.qml`;
- group filtering state is not a visible shell dependency;
- top-row text `Save`/`Save As...` buttons are absent;
- compact icon actions and existing save/search/advanced hooks are present;
- `main.qml` dispatch still opens printer, material, and process dialogs.

## Checker Result

Inline UI checker review: approved.

- Brand/domain fit: pass, dense operational settings UI.
- Screenshot alignment: pass for shell/top row/tabs; row internals explicitly
  deferred to Phase 86.
- Component fit: pass, uses existing `Cx*` controls/assets.
- Interaction coverage: pass, all modified controls retain commands/tooltips.
- Responsiveness: pass, fixed-size desktop dialog target is screenshot-defined.
- Scope discipline: pass, no LAN/device/cloud/network or global theme work.
