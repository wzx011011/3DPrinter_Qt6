---
phase: 43
artifact: uat
status: pending_user
created_at: 2026-06-29T10:37:40+08:00
commit: ac84277
requirements: [VERIFY-05]
---

# Phase 43 Manual UAT

Automated verification passed. Manual UAT remains pending user confirmation in the running GUI.

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

- [ ] Import a real STL fixture or local STL from the normal UI.
- [ ] Import/open a real 3MF if available.
- [ ] Verify Prepare shows objects, active plate, mesh, and enabled slice action.
- [ ] Slice the current plate and wait for completion.
- [ ] Enter Preview.
- [ ] Drag the layer height/range controls.
- [ ] Drag move controls if visible.
- [ ] Rotate/zoom/pan the camera with mouse.
- [ ] Verify the model/G-code toolpath does not disappear after layer, move, or camera interaction.
- [ ] Export current plate `.gcode`; verify the file exists and is non-empty.
- [ ] Export all printable valid plates; verify generated files exist and are non-empty.

## User Result

Pending.

Do not mark `VERIFY-05` satisfied until this checklist is confirmed.
