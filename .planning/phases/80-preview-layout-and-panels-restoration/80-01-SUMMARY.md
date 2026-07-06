# Phase 80 Summary: Preview Layout And Panels Restoration

**Status:** Implementation complete pending final commit
**Requirements:** `PVLAYOUT-01`, `PVLAYOUT-02`, `PVLAYOUT-03`

## Completed

- Restored `PreviewPage.qml` to fixed Preview target dimensions:
  - left sidebar `392 px`,
  - right analysis panel `300 px`,
  - far-right layer rail and bottom move bar retained with stable dimensions.
- Reworked the right analysis stack:
  - `StatsPanel`,
  - `VisibilityFilter`,
  - `Legend`,
  - bounded `G-code` source-line panel.
- Removed visible mojibake from touched Preview QML and replaced it with
  readable localized strings.
- Preserved the existing real Preview backend path:
  - `GLViewport.CanvasPreview`,
  - `previewVm.gcodePreviewData`,
  - `roleVisibilityMask`,
  - `gcodeLines`,
  - `PreviewViewModel` toggle and move/layer APIs.
- Added a Phase 80 source audit to `QmlUiAuditTests` that locks layout anchors,
  right-panel bindings, and mojibake absence.

## Not Changed

- No C++ backend behavior was changed.
- No QRhi/D3D backend policy was changed.
- No parser, renderer wire format, or role color semantics were changed.

## Follow-Up

- Phase 81 owns the deeper layer rail and move/playback control restoration.
- Phase 82 owns role color and view-mode semantic polish.
- Phase 83 owns full canonical verifier, app launch, and screenshot evidence.
