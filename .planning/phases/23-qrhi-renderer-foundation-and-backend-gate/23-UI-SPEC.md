---
phase: 23
slug: qrhi-renderer-foundation-and-backend-gate
status: approved
shadcn_initialized: false
preset: none
created: 2026-06-27
reviewed_at: 2026-06-27
---

# Phase 23 - UI Design Contract

> Visual and interaction contract for the gated QRhi viewport foundation.

---

## Design System

| Property | Value |
|----------|-------|
| Tool | none |
| Preset | not applicable |
| Component library | Qt Quick Controls 2 Basic plus existing OWzx QML controls |
| Icon library | existing QML icon/text controls; no new icon set for this phase |
| Font | existing application font through Qt/QML Theme tokens |

---

## Spacing Scale

Declared values for this phase:

| Token | Value | Usage |
|-------|-------|-------|
| xs | 4px | Inline gaps, focus ring offsets |
| sm | 8px | Compact diagnostic rows and viewport overlays |
| md | 16px | Default margins around non-intrusive status content |
| lg | 24px | Separation from existing toolbars and side panels |
| xl | 32px | Major panel gaps when diagnostics expand |
| 2xl | 48px | Reserved for future full diagnostic panels, not used in Phase 23 |
| 3xl | 64px | Reserved for future full diagnostic panels, not used in Phase 23 |

Exceptions: existing QML Theme has historical 6px and 12px tokens; Phase 23 must not introduce new non-4px spacing values.

---

## Typography

| Role | Size | Weight | Line Height |
|------|------|--------|-------------|
| Label | 12px | 400 | 1.4 |
| Body | 14px | 400 | 1.5 |
| Heading | 16px | 600 | 1.25 |
| Diagnostic value | 20px | 600 | 1.2 |

---

## Color

| Role | Value | Usage |
|------|-------|-------|
| Dominant (60%) | `Theme.bgBase` / `#0d0f12` | Existing Prepare/Preview viewport surroundings and app background |
| Secondary (30%) | `Theme.bgSurface` / `#131720` | Existing panels, diagnostic rows, fallback status surfaces |
| Accent (10%) | `Theme.accent` / `#18c75e` | Active QRhi enabled indicator, selected backend chip, benchmark success state, focus ring |
| Destructive | `Theme.statusError` / `#e04040` | QRhi backend initialization failure state only |

Accent reserved for: active QRhi enabled indicator, selected backend chip, benchmark success state, focus ring. It must not be applied to every interactive element.

---

## Copywriting Contract

| Element | Copy |
|---------|------|
| Primary CTA | Run Render Benchmark |
| Empty state heading | QRhi renderer is disabled |
| Empty state body | Enable `OWZX_RHI_RENDERER=1` to test the QRhi viewport path. Default rendering remains unchanged. |
| Error state | QRhi backend failed to initialize. OWzx is using the stable viewport path. Check `startup_diagnostics.log` for backend details. |
| Destructive confirmation | None. Phase 23 introduces no destructive user action. |

---

## Viewport Host Contract

| Area | Contract |
|------|----------|
| Prepare host | The existing Prepare viewport area remains the visual anchor. QRhi host must occupy the same bounds as the current `GLViewport`/`SoftwareViewport` item and must not shift toolbars, context menus, or sidebars. |
| Preview host | The existing Preview viewport frame remains unchanged. QRhi host must fit inside the current viewport content area and preserve marker/legend overlays for later phases. |
| Fallback state | If QRhi cannot initialize, the user sees the stable viewport path. Diagnostics are logged and may surface as a non-blocking notification, not a modal startup failure. |
| Backend indicator | If a visible indicator is added, keep it compact and diagnostic-only: backend name, enabled/disabled state, and fallback reason. |
| QML boundary | QML may choose the registered item and bind properties. QML must not parse scene data, select QRhi backends, or implement rendering logic. |

---

## Registry Safety

| Registry | Blocks Used | Safety Gate |
|----------|-------------|-------------|
| none | none | No third-party UI registry is used in this Qt/QML phase. |

---

## Checker Sign-Off

- [x] Dimension 1 Copywriting: PASS
- [x] Dimension 2 Visuals: PASS
- [x] Dimension 3 Color: PASS
- [x] Dimension 4 Typography: PASS
- [x] Dimension 5 Spacing: PASS
- [x] Dimension 6 Registry Safety: PASS

**Approval:** approved 2026-06-27
