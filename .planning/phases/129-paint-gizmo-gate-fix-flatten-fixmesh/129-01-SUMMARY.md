# Phase 129 Summary: Paint-Gizmo Gate Fix + Flatten + FixMesh

**Phase:** 129 (WS1, POLISH-01/02/03)
**Status:** Complete
**Date:** 2026-07-15

## What Shipped
- POLISH-01: flipped `kViewportTrianglePickingAvailable` to true (Support/Seam/MMU paint gizmos now report correct availability).
- POLISH-02: flattenSelected calls real `orientObject` (Slic3r::orientation::orient) instead of mock 6-face.
- POLISH-03: fixMesh + reloadFromDisk call real `Slic3r::its_merge_vertices` + `Slic3r::its_remove_degenerate_faces` (was no-op copy); reloadFromDisk also re-reads from disk via ReadSTLFile.

## Verification
Canonical build (j6) exit 0; all 5 ctest groups PASS; APP_RUNNING_PID=35512.
