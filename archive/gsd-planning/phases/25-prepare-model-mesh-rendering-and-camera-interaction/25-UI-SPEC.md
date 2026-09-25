---
phase: 25
slug: prepare-model-mesh-rendering-and-camera-interaction
status: approved
shadcn_initialized: false
preset: none
created: 2026-06-27
reviewed_at: 2026-06-27
---

# Phase 25 - UI Design Contract

> Visual and interaction contract for QRhi Prepare model mesh rendering, camera interaction, and selection/hover feedback.

---

## Design System

| Property | Value |
|----------|-------|
| Tool | none |
| Preset | not applicable |
| Component library | Qt Quick Controls 2 Basic + existing OWzx QML controls |
| Icon library | existing QML assets/icons only; no new icon dependency |
| Font | existing Qt default app font through `Theme.qml` tokens |

Phase 25 does not introduce a new page, toolbar, panel, modal, or control family. It extends the existing Prepare viewport behavior inside the QRhi gated renderer.

---

## Spacing Scale

Declared values for any new Phase 25 overlay or fallback notice:

| Token | Value | Usage |
|-------|-------|-------|
| xs | 4px | Icon gaps, inline padding |
| sm | 8px | Compact element spacing |
| md | 16px | Default element spacing |
| lg | 24px | Section padding |
| xl | 32px | Layout gaps |
| 2xl | 48px | Major section breaks |
| 3xl | 64px | Page-level spacing |

Exceptions: no new visible layout should be added for normal QRhi mesh rendering. Existing `PreparePage.qml`, `GLToolbars`, object list, and context menus keep their current spacing.

---

## Typography

Phase 25 should not add instructional viewport text, technology labels, benchmark readouts, or hero-scale typography. If a fallback/error message must be surfaced through existing notification UI, use the app's compact tool typography:

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
| Dominant (60%) | `#0d0f12` / existing viewport base | Prepare viewport background and unoccupied canvas |
| Secondary (30%) | `#161a23`, `#2a3140` | Existing panels, plate cards, bed surface and low-contrast grid |
| Accent (10%) | `#18c75e` | Active plate/context cue, selected-object outline or tint, focus/primary action only |
| Destructive | `#e04040` | Existing destructive actions only; Phase 25 adds none |

Accent reserved for: active/selected object feedback, active plate context, focus ring, and existing primary slice action. Do not apply accent to every mesh, hover, or all interactive states.

Renderer palette:

| Element | Value | Contract |
|---------|-------|----------|
| Model base colors | deterministic muted multi-color set, reusing current SoftwareViewport object color family where possible | Distinguish objects without overwhelming selection/hover states |
| Selected object | accent tint or outline with clear contrast over model material | Must remain visible on all base model colors |
| Hovered object | subtle light outline or tint distinct from selected state | Must not look selected when only hovered |
| Wire/outline overlay | light neutral or accent depending on selected state | Must be thin enough not to obscure model shape |
| Bed/grid | Phase 24 UI-SPEC palette | Must visually sit behind model geometry |

---

## Interaction Contract

| Interaction | Behavior |
|-------------|----------|
| Left drag on empty viewport | Orbit camera, matching existing GL/Software Prepare behavior |
| Middle drag | Pan camera target |
| Mouse wheel | Zoom camera distance |
| Fit view toolbar action | Fit camera to active model bounds when present, otherwise to bed/plate context |
| Right click | Preserve existing Prepare context menu routing; do not steal it for camera or picking |
| Click model | Select the hit object through `EditorViewModel` selection state |
| Move over model | Show lightweight hover feedback without changing selection |

Interaction feedback must update without full mesh buffer reupload. Camera movement changes only camera/uniform state; selection and hover changes only lightweight per-object state or overlay drawing.

---

## Visual Hierarchy

Primary focal point: active plate model geometry.

Hierarchy:

1. Selected model object.
2. Hovered model object.
3. Non-selected model meshes.
4. Active bed border/origin cues.
5. Coarse grid.
6. Fine grid and base viewport background.

Phase 25 must not add floating explanatory cards inside the viewport. Renderer diagnostics should use logs or existing notification/banner paths.

---

## Copywriting Contract

| Element | Copy |
|---------|------|
| Primary CTA | Existing Prepare slice action remains unchanged |
| Empty state heading | Existing Prepare empty state remains unchanged |
| Empty state body | Existing Prepare empty state remains unchanged |
| Error state | `高性能渲染初始化失败，已切回稳定视口；可继续编辑，性能诊断请查看日志。` |
| Destructive confirmation | Not applicable; Phase 25 adds no destructive action |

Copy rules:

- Keep all new user-visible strings behind `qsTr()`.
- Do not add visible text explaining QRhi, D3D12, GPU buffers, shader state, keyboard shortcuts, or implementation details.
- Do not duplicate existing toolbar labels or context-menu copy.

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
| `GLCanvas3D` | Prepare viewport camera, picking, hover, selection update flow |
| `Camera` / `CameraUtils` | Orbit/pan/zoom/fit view behavior and camera presets |
| `Selection` | Selected/hovered object state is user-visible and synchronized with editor selection |
| `PartPlate` / `PartPlateList` | Current plate object membership and active-plate-only rendering |
| `3DBed` | Bed/grid/axis context remains behind model geometry |

Technical implementation remains Qt QRhi-native. The visual contract is functional parity, not upstream OpenGL implementation parity.

---

## Checker Sign-Off

- [x] Dimension 1 Copywriting: PASS
- [x] Dimension 2 Visuals: PASS
- [x] Dimension 3 Color: PASS
- [x] Dimension 4 Typography: PASS
- [x] Dimension 5 Spacing: PASS
- [x] Dimension 6 Registry Safety: PASS

**Approval:** approved 2026-06-27
