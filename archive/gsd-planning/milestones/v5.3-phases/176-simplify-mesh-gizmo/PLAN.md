# Phase 176: Simplify Mesh Gizmo

**Status:** Executed
**Workstream:** FEAT
**Requirement:** FEAT-03

## Result

Replace the v3.x stub at EditorViewModel::simplifyMeshSelected with a delegation
to the real simplifySelected implementation.

## Key finding (audit was wrong)

The v5.3 parity audit flagged Simplify as "explicit stub — not yet implemented".
Direct code investigation revealed the real simplify pipeline already existed:
- EditorViewModel::simplifySelected() at line 1355 was already a real
  implementation calling ProjectServiceMock::simplifyObject() with a
  SimplifyCommand (undo integration).
- ProjectServiceMock::simplifyObject() already calls
  Slic3r::its_quadric_edge_collapse(...) (the real libslic3r simplify math).

The stub was a DUPLICATE method simplifyMeshSelected() at line 4681 that was
a no-op logging "not yet implemented". PreparePage.qml:447 + ObjectList.qml:928
were calling the stub instead of the real method.

## Fix

simplifyMeshSelected now delegates to simplifySelected (the real path). No
new libslic3r code needed — the simplify math was already wired; only the
VM method call chain was broken.

## Verification
- QmlUiAuditTests 134/134 PASS
- OWzxSlicer link OK
