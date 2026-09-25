---
quick_id: 260706-uix
slug: prepare-actionable-controls
status: complete
date: 2026-07-06
---

# Quick Task 260706-uix Summary

## Completed

- Converted restored Prepare top action placeholders into real `EditorViewModel` actions.
- Added backend-gated topbar behavior for single-plate slicing and G-code export readiness.
- Disabled high-risk gizmo modes that are not currently backed by complete runtime capability:
  - Mesh Boolean and Drill require unavailable CGAL MeshBoolean support.
  - Support Paint and Seam Paint require viewport triangle picking before they can be honest.
- Added QML audit coverage and ViewModel smoke coverage for the new gates.

## Verification

- Focused QML audit: passed.
- QML lint: passed with existing warnings.
- Encoding guard: passed.
- `git diff --check`: passed.
- Canonical verifier: passed with `scripts/auto_verify_with_vcvars.ps1`.
- Runtime launch: `build/OWzxSlicer.exe` started and responded as `OWzx Slicer`.

## Notes

- Computer Use screenshot capture failed on the OWzx window with `SetIsBorderRequired failed: 不支持此接口 (0x80004002)`. The app process itself was responsive.
- `.zcode/` was pre-existing/unrelated and was not touched.
