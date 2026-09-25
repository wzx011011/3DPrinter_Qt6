---
phase: 14
slug: visible-placeholder-triage
status: approved
shadcn_initialized: false
preset: none
created: 2026-06-25
reviewed_at: 2026-06-25
---

# Phase 14 - UI Design Contract

> Visual and interaction contract for remaining UI repair work in Phase 14: Visible Placeholder Triage.

---

## Purpose

Phase 14 closes the remaining visible UI design debt identified by the global UI audit after the quick fix `260625-0cz-ui`.

The goal is not to implement large source-truth modules such as full AssembleView, ModelMall WebView, or every calibration mode. The goal is to make every visible UI surface honest, classified, visually consistent, and non-inert: wired when behavior exists, hidden when unavailable, or clearly documented as blocked/future in planning without leaking internal phase/version copy into runtime UI.

---

## Design System

| Property | Value |
|----------|-------|
| Tool | none |
| Preset | not applicable |
| Component library | Qt Quick Controls 2 Basic style plus project `Cx*` QML controls |
| Icon library | Existing SVG assets under `src/qml_gui/assets/icons`; prefer existing icons and `CxIconButton` |
| Font | Qt platform default; use `Theme.fontSize*` tokens only unless explicitly listed below |
| Styling source | `src/qml_gui/Theme.qml` is the visual token source |
| Runtime language | Chinese for user-visible product UI in this phase; technical English only when it is a file format, protocol, or upstream-defined proper noun |

No shadcn or web registry applies because this is a native Qt/QML desktop application.

---

## Remaining UI Repair Scope

| Area | Classification | Required UI Treatment |
|------|----------------|-----------------------|
| Export project | Real/Hybrid | Visible action must open a save/export dialog and call existing backend/viewmodel behavior. No TODO handler. |
| Export model | Real/Hybrid | Visible action must open a format-aware model export dialog and call `EditorViewModel::exportModel`. |
| Preferences | Real/Hybrid | Visible action must open the existing settings/preferences workflow through `BackendContext::openSettings()`. |
| Implemented calibration modes | Hybrid | Topbar/menu entries must route to `CalibrationViewModel` selection and calibration page. Only modes with current backend support may be visible as actionable entries. |
| Unimplemented calibration modes | Pending/Blocked | Do not show as enabled actions. If surfaced in planning, list with upstream reference and block reason. Runtime UI must not contain phase/version labels. |
| Account / login | Placeholder/Blocked | Hide topbar icon until backed by source-truth account workflow. Home-page local login mock may remain only if visibly scoped as local device/profile setup, not cloud account completion. |
| Model store / ModelMall | Blocked/Placeholder | Do not expose topbar shortcut as complete. ModelMall page may remain accessible only as a blocked/preview surface with clear unavailable WebView copy and no fake marketplace promises. |
| Publish | Blocked/Placeholder | Hide topbar publish shortcut unless ModelMall publish flow is implemented or explicitly blocked with user-safe copy. |
| Filament group popup | Placeholder | Hide topbar icon until a real popup and viewmodel behavior exist. |
| Layer editing | Placeholder/Partial | Disabled layer-editing controls must either open existing layer tools or be hidden. If kept visible, copy must state "暂不可用" and planning must classify it. |
| AssembleView | Placeholder | Runtime copy must be neutral Chinese such as "装配视图暂不可用"; no phase numbers, "reserved", or implementation notes. |
| Left sidebar search / parameter subtabs | Partial | Search must either open the existing search dialog or be disabled/hidden. Parameter subtabs must not imply loaded parameter pages when only a placeholder list exists. |
| Documentation / update checks | Blocked/Future | Hidden or disabled without empty handlers. If visible in menus, labels remain non-actionable disabled items and planning records why. |

---

## Spacing Scale

Declared values must follow the standard GSD scale. Phase 14 may map these values onto existing `Theme.qml` tokens only when the resulting runtime value matches the standard scale.

| Token | Value | Usage |
|-------|-------|-------|
| `Theme.spacingXS` | 4px | Icon gaps, compact inline spacing |
| `Theme.spacingMD` | 8px | Default dense tool and menu spacing |
| `Theme.spacingXL` | 16px | Section-level gaps |
| `Theme.spacingXXL` | 24px | Major page-level padding |
| `Theme.spacingPage` | 32px | Page-side padding if added to `Theme.qml` during implementation |
| `Theme.spacingSection` | 48px | Major grouped content separation if added to `Theme.qml` during implementation |
| `Theme.spacingViewport` | 64px | Large viewport-safe empty-state spacing if added to `Theme.qml` during implementation |

Exceptions:
- Existing `Theme.spacingSM = 6px` and `Theme.spacingLG = 12px` may remain in untouched legacy code, but Phase 14 must not introduce new uses of 6px or 12px.
- Minimum click targets for icon buttons must stay at or above 32px.
- New raw margins, spacing, and radii are not allowed in Phase 14 unless they are replacing an existing one-off value with a token-equivalent follow-up.

---

## Typography

Phase 14 must use the existing `Theme.qml` type scale and must not introduce new runtime font sizes.

| Role | Size | Weight | Line Height |
|------|------|--------|-------------|
| Micro label | `Theme.fontSizeXS` / 10px | 400 | 1.3 |
| Dense body / menu item | `Theme.fontSizeMD` / 12px | 400 | 1.4 |
| Section title | `Theme.fontSizeLG` / 14px | 600 | 1.3 |
| Page heading / dialog title | `Theme.fontSizeXL` / 16px | 600 | 1.25 |

Rules:
- Runtime operational text below 10px is not allowed.
- Raw `font.pixelSize: 7`, `8`, or `9` must not be added; existing instances touched by Phase 14 must be raised to `Theme.fontSizeXS` or removed.
- Icon glyphs used as decorative empty-state artwork must not define page hierarchy. Prefer SVG icons or existing icon assets.
- No more than two weights in new code: regular `400` and semibold/bold `600`.

---

## Color

Use the existing OWzx dark operational palette. Phase 14 must reduce palette drift rather than introduce new colors.

| Role | Value | Usage |
|------|-------|-------|
| Dominant (60%) | `Theme.bgBase`, `Theme.chromeSurface`, `Theme.bgPanel` | Window background, topbar, side panels |
| Secondary (30%) | `Theme.bgSurface`, `Theme.bgElevated`, `Theme.bgInset`, `Theme.chromeHover` | Cards, input wells, hover states |
| Accent (10%) | `Theme.accent`, `Theme.accentLight`, `Theme.borderFocus` | Active navigation item, primary action, focused input, selected calibration item |
| Destructive | `Theme.statusError`, `Theme.chromeDangerHover`, `Theme.bgErrorSubtle` | Delete, remove, failure state, destructive confirmation only |
| Warning / blocked | `Theme.statusWarning`, `Theme.bgWarningSubtle` | Blocked dependency, unavailable runtime workflow |
| Informational | `Theme.statusInfo` | Progress, neutral information banners |

Accent reserved for:
- Active top-level navigation.
- Primary CTA in dialogs or menus.
- Focus ring for editable controls.
- Selected calibration or settings item.
- Progress success state.

Accent is not reserved for all hover states, all links, decorative icons, or disabled placeholder surfaces.

Hardcoded hex colors are not allowed in Phase 14 files unless the value is data-derived content such as a filament swatch, preview legend, or imported model color.

---

## Copywriting Contract

Runtime copy must be product-facing Chinese and must not expose implementation state. Forbidden runtime terms include: `placeholder`, `reserved`, `TODO`, `Phase`, `v2.x`, `占位 Tab`, and "实现" when describing an unavailable feature.

| Element | Copy |
|---------|------|
| Primary CTA | `导出模型`, `导出项目`, `打开设置`, `开始校准` |
| Empty state heading | `暂无可用内容` |
| Empty state body | `此功能尚未接入可用工作流。请从已启用的工具继续操作。` |
| Blocked state heading | `功能暂不可用` |
| Blocked state body | `当前依赖尚未接入或验证，已记录为后续迁移范围。` |
| Error state | `操作未完成。请检查当前项目状态后重试。` |
| Destructive confirmation | `删除对象：此操作会从当前项目移除选中对象，可通过撤销恢复。` |
| Hidden unavailable action | No runtime copy; classify in planning only. |

Action labels must be specific verb + noun:
- Use `导出模型`, not `导出`.
- Use `打开设置`, not `设置` when it is a command.
- Use `开始校准`, not `开始`.

Disabled visible actions must have one of these treatments:
- Hidden if no user action can resolve it.
- Disabled with tooltip/body copy `功能暂不可用` if the surface must remain visible for source-truth parity.
- Replaced by a planning-only placeholder registry entry if it is not useful in runtime UI.

---

## Layout And Interaction Contract

### Topbar

- Topbar must remain dense and operational, not a marketing surface.
- Visible topbar controls must be actionable or hidden.
- Account, model store, publish, and filament group controls remain hidden until their backend/viewmodel behavior exists.
- Page tabs must only list reachable, meaningful pages. `tpPlaceholder2` must not be exposed as a tab.
- `tpPlaceholder1` may be labeled `辅助` only while it loads `AuxiliaryPage`.

### Menus

- No `onTriggered: {}` or empty `onClicked: {}` in top-level menus.
- Disabled menu items may exist only for conventional actions such as update checks when planning classifies them as blocked/future.
- Calibration menu entries may only be enabled for modes that map to existing `CalibrationViewModel` indexes.
- Documentation/help items must be hidden or disabled until real help content exists.

### Sidebar And Settings

- Sidebar controls must use Chinese copy and `Theme` typography.
- Search controls must either perform visible search/open `SearchDialog` or be hidden/disabled.
- Parameter subtabs must not imply real parameter pages unless the selected tab changes visible parameter content.
- Placeholder parameter content must use `功能暂不可用` wording or be moved to planning only.

### Placeholder Surfaces

- Placeholder surfaces must not be counted as complete workflows.
- If a placeholder remains visible, it must satisfy all three:
  1. It has neutral Chinese copy.
  2. It has no phase/version/internal implementation wording.
  3. It is listed in the Phase 14 placeholder registry section below.

---

## Placeholder Registry

| Surface | Runtime Treatment | Planning Classification | Next Real Milestone |
|---------|-------------------|-------------------------|---------------------|
| Account / cloud login topbar | Hidden | Blocked/Placeholder | Cloud/multi-machine milestone |
| Model store topbar | Hidden | Blocked/Placeholder | Web/ModelMall milestone |
| Publish topbar | Hidden | Blocked/Placeholder | Web/ModelMall milestone |
| Filament group popup | Hidden | Placeholder | Multi-filament workflow milestone |
| `tpPlaceholder2` debug page | Hidden | Placeholder | Debug tooling only if source-truth requires it |
| AssembleView | Neutral unavailable copy if reached | Placeholder | PartPlate / AssembleView milestone |
| ModelMall WebView | Blocked/preview copy; no fake web claim | Blocked/Placeholder | Web/Cloud milestone |
| GL toolbar layer editing | Hidden or disabled with `功能暂不可用` | Placeholder | Layer editing parity task |
| Left sidebar parameter subtabs | Partial/unavailable copy unless real content exists | Partial/Placeholder | Preset/settings completion milestone |
| Documentation / update check | Disabled or hidden | Future/Blocked | Help/update service milestone |

---

## Verification Contract

Phase 14 implementation must extend or preserve `QmlUiAuditTests` so the following remain guarded:

- No visible top-level placeholder tab copy.
- No `label: qsTr("占位")` in `BBLTopbar.qml`.
- No runtime `reserved (v2.0 placeholder)` copy in `Plater.qml`.
- No top-level TODO export/preferences handlers.
- No top-level empty `onTriggered: {}` in `BBLTopbar.qml` or `main.qml`.
- No forbidden topbar hardcoded chrome colors: `#181818`, `#2a2a2a`, `#4CD582`, `#c0c0c0`, `#303030`.
- No listed mixed-language sidebar labels from the global UI audit.
- No `font.pixelSize` below 10 in touched sidebar controls.

Additional Phase 14 checks to add during implementation:

- `BBLTopbar.qml` visible action entries are all in one of: wired, hidden, disabled-with-copy.
- Remaining placeholder registry surfaces are documented in `.planning` and do not leak phase/version copy into runtime QML.
- Topbar calibration menu indexes match the actual `CalibrationViewModel` item order or use named backend methods if added later.

Full verification command:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

No alternate build directory or build script is allowed.

---

## Registry Safety

| Registry | Blocks Used | Safety Gate |
|----------|-------------|-------------|
| shadcn official | none | not applicable |
| third-party | none | not applicable |

---

## Checker Sign-Off

- [x] Dimension 1 Copywriting: PASS
- [x] Dimension 2 Visuals: PASS
- [x] Dimension 3 Color: PASS
- [x] Dimension 4 Typography: PASS
- [x] Dimension 5 Spacing: PASS
- [x] Dimension 6 Registry Safety: PASS

**Approval:** approved 2026-06-25
