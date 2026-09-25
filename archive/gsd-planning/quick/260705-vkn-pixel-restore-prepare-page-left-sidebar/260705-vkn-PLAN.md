---
quick_id: 260705-vkn
slug: pixel-restore-prepare-page-left-sidebar
status: complete
date: 2026-07-05
---

# Quick Task 260705-vkn: Pixel restore Prepare page left sidebar

## Goal

Restore the default Prepare page left sidebar against `shotScreen/准备页.png` with pixel-oriented acceptance rather than broad visual similarity.

## Acceptance

- Prepare left sidebar expanded width is close to the screenshot boundary (`392px`, tolerance `+/- 6px`).
- Sidebar content fills the expanded width instead of ending near `160px`.
- Expanded sidebar does not show the extra `设置` title strip or top green collapse button.
- Default left sidebar is scoped to printer, filament, process preset, search, and inline parameter rows. Object list and slice progress remain available as files/features, but are not mounted in the default screenshot sidebar.
- Sidebar palette moves toward screenshot gray panel values instead of the current very dark blue-black surface.
- Add source audit coverage that fails on the current implementation and locks the restored structure.
- Run the focused audit test, encoding guard, canonical verification script, then launch the app and capture/compare the left sidebar crop.

## Plan

1. Add a failing QML source audit for the pixel restoration contract.
2. Restore `PreparePage.qml` sidebar width defaults to screenshot width.
3. Remove the expanded `DockableSidebar` title strip and make `LeftSidebar` start at the top of the panel.
4. Rewrite the default `LeftSidebar` composition to the screenshot-density structure while preserving preset/settings/filter bindings.
5. Update Phase 78 cleanup assertions so they no longer require object list and slice progress to be mounted in the default left sidebar.
6. Verify with focused Qt test, canonical build script, app launch screenshot, and a crop metric comparison.

## Result

- Code commit: `eab371a`
- Restored the default Prepare left sidebar to the screenshot width contract across `BackendContext`, `Plater`, and `PreparePage`.
- Removed the expanded `DockableSidebar` title strip that pushed content down.
- Rebuilt `LeftSidebar` as the screenshot-oriented printer, filament, process, search, and inline parameter column.
- Added audit coverage so persisted sidebar width cannot silently override the pixel contract again.
- Final 2560x1400 runtime screenshot measured target boundary `400px` vs current `398px` (`-2px` delta).

## Evidence

- Runtime screenshot: `build/pixel-audit/prepare-left-restored-runtime-final.png`
- Target/current comparison: `build/pixel-audit/prepare-left-restored-side-by-side-final.png`
- Verification: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` passed.
