# Phase 82 Summary: Preview G-code Roles Color Modes And Rendering

**Status:** Implementation complete pending final commit
**Requirements:** `PVRENDER-01`, `PVRENDER-02`, `PVRENDER-03`

## Completed

- Preserved the upstream 17-mode Preview view-mode order and role-color path.
- Added ViewModel availability APIs:
  - `currentViewModeAvailable`,
  - `currentViewModeStatus`,
  - `viewModeAvailable(index)`,
  - `viewModeStatusText(index)`.
- Marked data-unavailable modes honestly:
  - `Actual Speed`,
  - `Jerk`,
  - `Actual Flow`,
  - `Pressure Advance`.
- Added a compact Preview header status pill for unavailable modes.
- Preserved the existing source-truth rendering path:
  - `gcodeViewMode` stays bound to `PreviewViewModel::viewModeIndex`,
  - role filtering stays bound to the dense `roleVisibilityMask`,
  - UI role rows continue to use `roleVisibilities`,
  - role toggles remain renderer-side and do not repack payload data.
- Added tests for:
  - view-mode availability and status,
  - GCV1 payload survival across all view modes,
  - QML role/color/legend/render binding coverage.

## Not Changed

- No GCV1 payload layout or parser wire format was changed.
- No RHI backend policy was changed.
- No slicing engine behavior was changed.

## Follow-Up

- Phase 83 owns final cleanup, stale path audit, canonical verifier, app launch,
  and Preview screenshot evidence against the target image.
