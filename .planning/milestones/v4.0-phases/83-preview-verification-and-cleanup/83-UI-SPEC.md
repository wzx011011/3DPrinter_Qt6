# Phase 83 UI Contract: Final Preview Verification

This phase does not redesign Preview. It locks the UI surfaces restored by
Phases 80-82.

## Required Surfaces

| Surface | Contract |
|---|---|
| Preview page | Uses the registered `GLViewport` type, never direct `SoftwareViewport`. |
| Renderer bindings | Binds G-code payload, layer range, move end, travel, role visibility, bed, marker, view mode, and marker coordinates to `PreviewViewModel`. |
| Left sidebar | Remains the shared restored sidebar constrained by the Preview layout. |
| Layer rail | Uses `PreviewLayerRail` with a vertical range control and callable ViewModel layer actions. |
| Move bar | Uses `MoveSlider` with playback, step, and drag actions backed by `PreviewViewModel`. |
| Right analysis | Uses `StatsPanel`, `VisibilityFilter`, and `Legend` backed by ViewModel state. |
| Availability | Data-unavailable color modes surface an honest compact status. |
| Empty state | Appears only when no Preview payload is ready. |

## Cleanup Rules

- Replaced simple layer slider paths must not remain in `PreviewPage.qml`.
- Restored Preview QML files must not expose placeholder markers or TODO copy.
- `SoftwareViewport` may remain registered only as a QRhi-unavailable fallback.
- QML resources must include each active restored Preview component.

## Evidence

Phase 83 must record:

- Automated audit coverage.
- Canonical verifier result.
- Runtime app launch result.
- A captured Preview screenshot stored under this phase directory.
