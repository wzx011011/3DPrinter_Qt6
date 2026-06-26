# Upstream Issue Avoidance Rules

This project inherits OrcaSlicer behavior as source truth, but the Qt6/QML
shell, viewmodels, renderers, preview parser, and task orchestration are local
implementations. Upstream OrcaSlicer issues are therefore design input for the
local layers, not permission to invent different product behavior.

## Risk Register

- Record relevant upstream issues in
  `.planning/quality/orcaslicer_issue_risk_register.md`.
- A risk entry must identify the impacted Qt6 module, the required design
  constraint, and the verification route.
- Treat the register as a planning input for future migrations. Do not close a
  local requirement only because the upstream issue is closed.

## Design Constraints

- G-code preview data must come from the final exported G-code whenever the
  workflow can run post-processing, custom G-code insertion, or export-time
  transformations. Intermediate slicer data is acceptable only when documented
  as an incomplete or diagnostic path.
- OpenGL-backed viewports must keep a software or no-OpenGL fallback path.
  Driver-specific blank-window or crash failures must not block basic app
  startup and navigation.
- Slice, calibration, model loading, preview parsing, upload, and other
  long-running tasks must expose cancellation and must always transition to a
  terminal UI state after success, failure, or cancellation.
- Multi-plate slice results and statistics must be modeled by plate identity
  and object instance identity, not by object display name or shared model
  pointer alone.
- Preview rendering must tolerate missing, malformed, partial, very large, or
  post-processed G-code without crashing. Large preview data needs an explicit
  cap, streaming, chunking, or lite-mode fallback before it is considered Real.
- Configuration merge code must preserve upstream precedence: global presets,
  filament/process/printer presets, plate overrides, calibration parameters, and
  export/post-processing changes must not silently overwrite each other.
- Environment-specific rendering problems belong in a manual environment
  matrix when they cannot be automated locally; keep the architectural fallback
  even if the current Windows development machine does not reproduce them.

## Test Selection

Prefer automated tests for deterministic regressions:

- final G-code preview parsing, including post-processed files;
- temperature, fan, speed, extrusion role, and tool-change preview metadata;
- slice-all queue state across locked, non-printable, failed, and cancelled
  plates;
- multi-plate statistics involving shared objects and instances;
- cancellation terminal states for slicing, calibration, load, and preview
  rebuilds;
- malformed preview buffers and empty preview data.

Use manual validation matrices for driver, display server, remote desktop, GPU,
printer firmware, and live device protocol issues.

Do not modify libslic3r slicing algorithms to avoid a UI-layer issue unless an
explicit migration task authorizes an upstream-aligned engine change.
