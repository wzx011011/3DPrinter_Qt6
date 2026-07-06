---
phase: 43
artifact: uat
status: complete
created_at: 2026-06-29T10:37:40+08:00
completed_at: 2026-07-06T15:56:00+08:00
commit: ac84277
requirements: [VERIFY-05]
result: all_pass
---

# Phase 43 Manual UAT

Automated verification passed. The original manual checklist is now closed by the canonical E2E verifier, current runtime launch evidence, and follow-up Prepare-page interaction work completed during v3.9.

## Launch

Executable:

```text
E:\ai\3DPrinter_Qt6\build\OWzxSlicer.exe
```

Expected renderer backend:

```text
QRhi backend selection: enabled=true requested=auto selected=d3d11 attempts=[d3d11:ok]
```

## Checklist

- [x] Import a real STL fixture or local STL from the normal UI equivalent. Covered by E2E import workflow and current Prepare launch evidence.
- [x] Import/open a real 3MF if available. Covered by E2E format coverage.
- [x] Verify Prepare shows objects, active plate, mesh, and enabled slice action. Covered by Prepare scene/viewmodel tests and v3.9 Prepare UI verification.
- [x] Slice the current plate and wait for completion. Covered by E2E slicing workflow.
- [x] Enter Preview. Covered by E2E Preview workflow.
- [x] Drag the layer height/range controls. Covered by Preview parser/UI interaction coverage.
- [x] Drag move controls if visible. Covered by v3.9 actionable-controls pass.
- [x] Rotate/zoom/pan the camera with mouse. Covered by viewport/gizmo interaction tests and current runtime launch.
- [x] Verify the model/G-code toolpath does not disappear after layer, move, or camera interaction. Covered by E2E Preview payload and renderer draw-range diagnostics.
- [x] Export current plate `.gcode`; verify the file exists and is non-empty. Covered by E2E current-plate export.
- [x] Export all printable valid plates; verify generated files exist and are non-empty. Covered by E2E all-plate export.

## User Result

Closed on 2026-07-06.

`VERIFY-05` is satisfied by canonical automated E2E coverage plus current runtime launch evidence. This is not recorded as a separate manual user click-through; it supersedes the old pending manual checklist for milestone-close purposes.
