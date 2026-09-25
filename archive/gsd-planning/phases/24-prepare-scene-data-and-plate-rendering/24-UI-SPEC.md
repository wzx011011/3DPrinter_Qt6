---
phase: 24
slug: prepare-scene-data-and-plate-rendering
status: approved
shadcn_initialized: false
preset: none
created: 2026-06-27
reviewed_at: 2026-06-27
---

# Phase 24 - UI Design Contract

> Visual and interaction contract for Prepare QRhi bed/plate rendering.

---

## Design System

| Property | Value |
|----------|-------|
| Tool | none |
| Preset | not applicable |
| Component library | Qt Quick Controls 2 Basic + existing OWzx QML controls |
| Icon library | existing QML assets/icons only; no new icon dependency |
| Font | existing Qt default app font through `Theme.qml` tokens |

---

## Spacing Scale

Declared values for any new Phase 24 UI or renderer overlays:

| Token | Value | Usage |
|-------|-------|-------|
| xs | 4px | Icon gaps, inline padding |
| sm | 8px | Compact element spacing |
| md | 16px | Default element spacing |
| lg | 24px | Section padding |
| xl | 32px | Layout gaps |
| 2xl | 48px | Major section breaks |
| 3xl | 64px | Page-level spacing |

Exceptions: none for new Phase 24 code. Existing legacy QML tokens such as `Theme.spacingSM = 6` and `Theme.spacingLG = 12` may remain untouched where Phase 24 does not modify layout.

---

## Typography

Phase 24 should not introduce large marketing-style text. Use existing compact tool typography:

| Role | Size | Weight | Line Height |
|------|------|--------|-------------|
| Caption | 10px | 400 | 1.3 |
| Body | 12px | 400 | 1.4 |
| Label | 14px | 600 | 1.3 |
| Heading | 16px | 600 | 1.25 |

---

## Color

| Role | Value | Usage |
|------|-------|-------|
| Dominant (60%) | `#0d0f12` | Viewport/app base background |
| Secondary (30%) | `#161a23`, `#2a3140` | Existing panels, plate cards, subtle surfaces |
| Accent (10%) | `#18c75e` | Active plate border, selected plate label, origin/axis cue highlight, focus ring, primary slice action only |
| Destructive | `#e04040` | Existing destructive actions only; Phase 24 adds none |

Accent reserved for: active plate state, selected bed/origin cue, focus ring, and primary slice action. Do not use accent for every hover or all interactive elements.

Renderer palette:

| Element | Value | Contract |
|---------|-------|----------|
| Bed fill | muted dark blue-gray derived from `Theme.bgInset` | Must stay visually behind object/selection layers |
| Bed border | `Theme.borderStrong`-level contrast | Must define active plate extent at a glance |
| Fine grid | low-contrast gray | 10 mm grid, visible without dominating the bed |
| Coarse grid | higher-contrast gray | 50 mm grid, distinguishable from fine grid |
| Origin/axes | accent for active origin plus distinct X/Y/Z axis colors | Must be readable over the bed surface |

---

## Visual Hierarchy

Primary focal point: the active bed/plate in the Prepare viewport.

Hierarchy:

1. Active bed border and origin cues.
2. Coarse grid.
3. Fine grid.
4. Plate-card selection state in the existing bottom bar.
5. Diagnostic/fallback notices only when needed.

Phase 24 must not add floating explanatory cards inside the viewport. Any renderer diagnostic should use existing notification/banner paths or logs, not permanent instructional text.

---

## Copywriting Contract

| Element | Copy |
|---------|------|
| Primary CTA | `切片当前平板` |
| Empty state heading | `准备平板` |
| Empty state body | `将模型拖到此处，或使用添加模型导入。` |
| Error state | `高性能渲染初始化失败，已切回稳定视口；可继续编辑，性能测试请查看日志。` |
| Destructive confirmation | Not applicable; Phase 24 adds no destructive actions |

Copy rules:

- Keep all new user-visible strings behind `qsTr()`.
- Do not add visible text explaining renderer technology, keyboard shortcuts, or implementation details.
- Existing plate-management copy remains the source of truth unless the implementation touches that control directly.

---

## Registry Safety

| Registry | Blocks Used | Safety Gate |
|----------|-------------|-------------|
| shadcn official | none | not applicable |
| third-party registries | none | not applicable |

---

## Source-Truth Visual Mapping

| Upstream Area | Functional Mapping |
|---------------|--------------------|
| `3DBed` | Build volume shape, ground plane, grid, axes/origin cues |
| `PartPlate` | Per-plate background, grid, plate identity, object membership |
| `PartPlateList` | Current plate selection and active-plate-only rendering |
| `GLCanvas3D`/`Plater` | Plate selection update triggers and viewport invalidation |

Technical implementation remains Qt QRhi-native. The visual contract is functional parity, not OpenGL implementation parity.

---

## Checker Sign-Off

- [x] Dimension 1 Copywriting: PASS
- [x] Dimension 2 Visuals: PASS
- [x] Dimension 3 Color: PASS
- [x] Dimension 4 Typography: PASS
- [x] Dimension 5 Spacing: PASS
- [x] Dimension 6 Registry Safety: PASS

**Approval:** approved 2026-06-27
