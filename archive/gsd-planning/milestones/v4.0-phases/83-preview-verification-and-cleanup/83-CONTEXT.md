# Phase 83 Context: Preview Verification And Cleanup

**Milestone:** v4.0 Preview Page UI Restoration
**Requirements:** `PVCLEAN-01`, `PVVERIFY-01`, `PVVERIFY-02`
**Mode:** Final cleanup, audit, canonical verification, runtime evidence

## Objective

Phase 83 closes the Preview restoration milestone by proving that the restored
Preview UI is the active path, stale replaced paths are absent, and the app can
build, launch, and show the restored Preview surface against the target
screenshot.

## Source Truth

- Visual target: `shotScreen/预览页.png`
- Behavior truth:
  - `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.*`
  - `third_party/OrcaSlicer/src/slic3r/GUI/GCodeViewer.*`
  - `third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.*`
  - `third_party/OrcaSlicer/src/libslic3r/GCode/*`

## Scope

- Add final source/QML audit coverage for restored Preview resources, bindings,
  actionable controls, and stale path absence.
- Keep `SoftwareViewport` as the guarded QRhi-unavailable fallback, but prevent
  direct `PreviewPage.qml` references to it.
- Run the canonical verifier only through
  `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- Launch `build/OWzxSlicer.exe` and record Preview runtime visual evidence.

## Out Of Scope

- D3D12/Vulkan backend promotion or root-cause debugging.
- New Preview behavior not mapped to upstream source truth.
- Reopening Prepare page visual work.
