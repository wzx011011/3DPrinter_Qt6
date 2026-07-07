# Phase 82: Preview G-code Roles Color Modes And Rendering - Context

**Gathered:** 2026-07-07
**Status:** Ready for planning
**Mode:** Autonomous smart-discuss defaults

<domain>
## Phase Boundary

Phase 82 owns the Preview right-panel role/color semantics: role colors,
role visibility controls, 17 upstream view modes, honest availability for
data-unavailable modes, legend semantics, and payload preservation while
changing Preview controls. It must not rewrite the parser wire format, QRhi
backend policy, or slicing engine.

</domain>

<decisions>
## Implementation Decisions

### View Mode Availability
- Preserve the existing 17-mode upstream order exposed by
  `PreviewViewModel::viewModes()`.
- Keep the current rendering fallback for data-unavailable modes
  (`Actual Speed`, `Jerk`, `Actual Flow`, `Pressure Advance`) but expose an
  explicit ViewModel status so the UI does not imply full upstream data exists.
- Show the status as a compact header pill, not a large instructional banner.

### Role Colors And Visibility
- Keep role colors sourced from upstream `DEFAULT_EXTRUSION_ROLES_COLORS`.
- Keep role visibility as a renderer-side mask. Toggling a role must not repack
  or clear `gcodePreviewData`.
- Keep `roleVisibilities` for UI rows and `roleVisibilityMask` for the renderer
  as separate shapes.

### Payload Preservation
- Mode changes may recolor/repack the GCV1 payload by design, but every mode
  must keep a valid non-empty `GCV1` payload after G-code is loaded.
- Layer, move, and role visibility interactions must not make the payload
  disappear.

</decisions>

<code_context>
## Existing Code Insights

- `PreviewViewModel::viewModes()` already exposes the 17 upstream modes.
- `PreviewViewModel::recolorAndPackSegments()` already maps line type,
  extruder/tool, gradient, summary, and data-unavailable modes.
- `PreviewViewModel::roleForType`, `roleColor`, `roleVisibilities`, and
  `roleVisibilityMask` already use canonical libvgcode role indices.
- `PreviewPage.qml` binds `GLViewport.gcodeViewMode` and `roleVisibility` to
  ViewModel state.
- `VisibilityFilter.qml` calls `toggleRoleVisibility`.
- `Legend.qml` already handles discrete, gradient, and extruder legend types.

</code_context>

<deferred>
## Deferred Ideas

- Runtime screenshot evidence and final visual comparison remain Phase 83.
- D3D12/Vulkan backend investigation remains future backend work.

</deferred>
