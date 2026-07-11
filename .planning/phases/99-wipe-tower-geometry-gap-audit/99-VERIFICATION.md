---
phase: 99-wipe-tower-geometry-gap-audit
verified: 2026-07-11
status: passed
requirements: [WTAUDIT-01, WTAUDIT-02]
canonical_build_run: false
canonical_build_reason: "Documentation/source audit only; no production QML/C++ changed."
---

# Phase 99 Verification Report

## Result

Status: passed.

Phase 99 produced the canonical v4.4 wipe-tower geometry readback + rendering
gap matrix. It did not modify product source files, QML resources, CMake files,
tests, or runtime assets. Full canonical build and application launch are
intentionally routed to Phase 102 after implementation phases 100-101 land the
readback wiring and the rendering-upgrade baseline.

## Requirement Coverage

| Requirement | Status | Evidence |
|---|---|---|
| WTAUDIT-01 | passed | `99-GAP-MATRIX.md` maps all 8 wipe-tower regions (WT-PLACEHOLDER-BOX through WT-SOFTWARE-VIEWPORT) to OrcaSlicer upstream source files (with line anchors: `Print.hpp:740-786,988-989,1078-1080`, `3DScene.cpp:840-885,887-925`), current Qt placeholder paths (`GizmoGeometry.h:74`/`.cpp:449-499`, `RhiViewport.h:54-59,181-192,304-309`, `RhiViewportRenderer.h:57,75,129,143,152,177,216-222`/`.cpp:1064-1095,1894-1908`, `SoftwareViewport.h:35-40,126-137,231-236`/`.cpp:207-253`, `PreparePage.qml:1648`), the post-slice readback design (Frozen Decision 1, citing `SliceService.cpp:508,584,625/629/634,763` + `EditorViewModel.h:860`), the rendering-upgrade decision (Frozen Decision 2, Option A baseline + Option B future), and verification expectations (Phase 102 source audits + canonical verifier + runtime visual evidence). |
| WTAUDIT-02 | passed | `99-GAP-MATRIX.md` Frozen Decisions section locks three designs before implementation: (1) post-slice readback integration point = read `Print::wipe_tower_data()` inside the SliceService worker after `print.process()` succeeds (between `:584` and the `:625` invalidation), capture dims into worker-local storage, deliver to the GUI thread alongside `sliceFinished` (`:763`), push into `RhiViewport` Q_PROPERTYs via `EditorViewModel` (`EditorViewModel.h:860`) — because the `Print` is only valid mid-slice between `activePrint_.store(&print)` (`:508`) and `activePrint_.store(nullptr)` (`:625`/`:629`/`:634`); (2) rendering-upgrade approach = Option A (dimensioned box from real `bbx`/`depth`/`height`/`position`/`width`, fed to the existing `buildWipeTowerVertices`, mirroring `3DScene.cpp:840-885 load_wipe_tower_preview`) LOCKED as the v4.4 baseline, with Option B (real mesh from optional `wipe_tower_mesh_data` via `convex_hull_3d`, mirroring `3DScene.cpp:887-925 load_real_wipe_tower_preview`) documented as a future upgrade; (3) `has_wipe_tower()` gating (`Print.hpp:988`) = when false, no geometry is pushed and `showWipeTower` stays false (`RhiViewport.h:54`/`:304`, `RhiViewportRenderer.h:216`), so no placeholder box leaks on single-material slices — the gate becomes data-driven from the readback (SoftwareViewport's differing default `show=true` at `SoftwareViewport.h:231` is aligned to the same gate). |

## Matrix Coverage

The matrix includes all required region IDs (8 of 8):

- WT-PLACEHOLDER-BOX
- WT-VIEWPORT-DEFAULTS
- WT-RENDERER-BUFFER
- WT-PRINT-DATA
- WT-READBACK-POINT
- WT-RENDER-UPGRADE
- WT-HAS-WIPE-GATE
- WT-SOFTWARE-VIEWPORT

The matrix table header lists all 10 columns in order: Region, Placeholder
Path, Upstream Anchor, Qt Integration Point, Decision, Gap, Severity, Owner
Phase, Requirement, Verification.

The matrix records exact Qt target paths for the placeholder box
(`GizmoGeometry.h:74`/`.cpp:449-499`), the viewport defaults
(`RhiViewport.h:54-59,181-192,304-309`; `PreparePage.qml:1648` unbound), the
renderer buffer path (`RhiViewportRenderer.h:57,75,129,143,152,177,216-222`;
`.cpp:1064-1095,1894-1908`), the software viewport
(`SoftwareViewport.h:35-40,126-137,231-236`/`.cpp:207-253`), the readback
integration point (`SliceService.cpp:508,584,625/629/634,763`;
`EditorViewModel.h:860`), and the has-wipe gate
(`RhiViewport.h:54,304`; `RhiViewportRenderer.h:216`). It also records
OrcaSlicer upstream source anchors with line citations for
`Print.hpp:740-786` (WipeTowerData struct: tool_changes, depth, brim_width,
height, bbx "including brim", rib_offset, optional wipe_tower_mesh_data),
`Print.hpp:988-989` (has_wipe_tower, wipe_tower_data), `Print.hpp:1078-1080`
(get_wipe_tower_depth/bbx/rib_offset), `3DScene.cpp:840-885`
(load_wipe_tower_preview via make_cube), and `3DScene.cpp:887-925`
(load_real_wipe_tower_preview via convex_hull_3d).

Severity classification: 4 Critical (WT-VIEWPORT-DEFAULTS, WT-PRINT-DATA,
WT-READBACK-POINT, WT-HAS-WIPE-GATE), 3 High (WT-PLACEHOLDER-BOX,
WT-RENDERER-BUFFER, WT-RENDER-UPGRADE), 1 Medium (WT-SOFTWARE-VIEWPORT).

## Upstream Anchor Citations

| Region | Upstream Anchor | Verified |
|---|---|---|
| WT-PLACEHOLDER-BOX | `3DScene.cpp:840-885 load_wipe_tower_preview` (`make_cube(width, depth, height)` at `:855`, `v.is_wipe_tower=true` at `:882`) | yes |
| WT-VIEWPORT-DEFAULTS | N/A — Qt-only Q_PROPERTY layer (no single upstream line); references `3DScene.cpp:840 load_wipe_tower_preview` GLVolumeCollection path as context | yes |
| WT-RENDERER-BUFFER | `3DScene.cpp:840-885` (box upload via `make_cube` + `init_from`) + `3DScene.cpp:887-925` (real mesh upload via `init_from(mesh.convex_hull_3d())`) | yes |
| WT-PRINT-DATA | `Print.hpp:740-786` (WipeTowerData struct), `:988-989` (has_wipe_tower/wipe_tower_data), `:1078-1080` (get_wipe_tower_depth/bbx/rib_offset) | yes |
| WT-READBACK-POINT | Upstream `Print::process()` populates `wipe_tower_data`; upstream GUI reads it after slice to drive `3DScene.cpp:840 load_wipe_tower_preview` | yes |
| WT-RENDER-UPGRADE | Option A `3DScene.cpp:840-885` (make_cube box); Option B `3DScene.cpp:887-925` (real mesh via `wipe_tower_mesh_data`/`convex_hull_3d`) | yes |
| WT-HAS-WIPE-GATE | `Print.hpp:988 has_wipe_tower()` is the gate | yes |
| WT-SOFTWARE-VIEWPORT | Same as WT-VIEWPORT-DEFAULTS (SoftwareViewport is a parallel non-RHI renderer; upstream is GL-only with no equivalent) | yes |

Each anchor was read from the current Qt6 source tree (`src/core/rendering/
GizmoGeometry.{h,cpp}`, `src/qml_gui/Renderer/RhiViewport.h`,
`RhiViewportRenderer.{h,cpp}`, `SoftwareViewport.{h,cpp}`,
`src/qml_gui/pages/PreparePage.qml`, `src/core/services/SliceService.cpp`,
`src/core/viewmodels/EditorViewModel.h`, `src/qml_gui/main_qml.cpp`) and
confirmed against the line content. The OrcaSlicer upstream anchors
(`Print.hpp:740-786,988-989,1078-1080`, `3DScene.cpp:840-885,887-925`) were
read directly from `third_party/OrcaSlicer/` and match the pre-planning
exploration in `99-CONTEXT.md`.

## Frozen Decisions

The `Frozen Decisions (WTAUDIT-02)` section is present and locks all three
designs:

1. **Post-slice readback integration point (WT-READBACK-POINT)** — read
   `Print::wipe_tower_data()` inside the SliceService worker after
   `print.process()` succeeds (between `:584` and the `:625` invalidation),
   capture dims into worker-local storage (no `Print*` escapes the worker),
   deliver to the GUI thread alongside `sliceFinished` (`:763`), push into
   `RhiViewport` Q_PROPERTYs via `EditorViewModel` (`EditorViewModel.h:860`).
   The `Print` is only valid mid-slice between `activePrint_.store(&print)`
   (`:508`) and `activePrint_.store(nullptr)` (`:625`/`:629`/`:634`). Owner
   Phase 100; Requirement WTREAD-01.
2. **Rendering-upgrade approach (WT-RENDER-UPGRADE)** — Option A (dimensioned
   box from real `bbx`/`depth`/`height`/`position`/`width`, fed to the existing
   `buildWipeTowerVertices`, mirroring `3DScene.cpp:840-885
   load_wipe_tower_preview`'s `make_cube`) LOCKED as the v4.4 baseline; Option B
   (real mesh from optional `wipe_tower_mesh_data` via `convex_hull_3d`,
   mirroring `3DScene.cpp:887-925 load_real_wipe_tower_preview`) documented as
   a future upgrade. Owner Phase 101; Requirement WTRENDER-02.
3. **has_wipe_tower() gating (WT-HAS-WIPE-GATE)** — `Print.hpp:988
   has_wipe_tower()` is the gate; when false, no geometry is pushed and
   `showWipeTower` stays false (`RhiViewport.h:54`/`:304`,
   `RhiViewportRenderer.h:216`), so no placeholder box leaks on single-material
   slices. The gate becomes data-driven from the readback; SoftwareViewport's
   differing default `show=true` (`SoftwareViewport.h:231`) is aligned to the
   same gate. Owner Phase 100; Requirement WTREAD-02.

## Out-of-Scope Classification

The `Out-of-Scope Classification` section names auto filament-map, per-plate
wipe-tower architecture refactor, D3D12/Vulkan, GLGizmoMeasure engine, CLI
fixtures, and the removed LAN/device/cloud/network/Monitor/ModelMall/camera
scope. No removed scope was reintroduced.

## Requirement Routing

The `v4.4 Requirement Routing` table lists all 8 v4.4 requirements
(WTAUDIT-01, WTAUDIT-02, WTREAD-01, WTREAD-02, WTRENDER-01, WTRENDER-02,
WTVERIFY-01, WTVERIFY-02) with their owner phase and matrix region(s).

## Commands Run

```bash
rg -n "WT-PLACEHOLDER-BOX|WT-VIEWPORT-DEFAULTS|WT-RENDERER-BUFFER|WT-PRINT-DATA|WT-READBACK-POINT|WT-RENDER-UPGRADE|WT-HAS-WIPE-GATE|WT-SOFTWARE-VIEWPORT" .planning/phases/99-wipe-tower-geometry-gap-audit/99-GAP-MATRIX.md
```

Result: passed; all 8 region IDs are present (22 matching lines across rows,
routing, and coverage).

```bash
rg -n "Print.hpp:740|Print.hpp:988|Print.hpp:1078|3DScene.cpp:840|3DScene.cpp:887|GizmoGeometry.h:74|GizmoGeometry.cpp:449|RhiViewport.h:54|RhiViewport.h:304|RhiViewportRenderer.h:57|RhiViewportRenderer.h:216|RhiViewportRenderer.cpp:1064|RhiViewportRenderer.cpp:1894|SoftwareViewport.h:35|SoftwareViewport.cpp:207|SliceService.cpp:508|SliceService.cpp:584|SliceService.cpp:763|EditorViewModel.h:860|PreparePage.qml:1648|has_wipe_tower|wipe_tower_data|wipe_tower_mesh_data|convex_hull_3d|make_cube|load_wipe_tower_preview|load_real_wipe_tower_preview" .planning/phases/99-wipe-tower-geometry-gap-audit/99-GAP-MATRIX.md
```

Result: passed; all upstream + Qt anchors are present (26 matching lines).

```bash
rg -n "Frozen Decisions|WT-READBACK-POINT|WT-RENDER-UPGRADE|WT-HAS-WIPE-GATE|SliceService.cpp:508|SliceService.cpp:584|SliceService.cpp:763|EditorViewModel.h:860|has_wipe_tower|wipe_tower_mesh_data|convex_hull_3d|make_cube|load_wipe_tower_preview|load_real_wipe_tower_preview|Option A|Option B|WTAUDIT-01|WTAUDIT-02|WTREAD|WTRENDER|WTVERIFY" .planning/phases/99-wipe-tower-geometry-gap-audit/99-GAP-MATRIX.md
```

Result: passed; frozen decisions, requirement routing, out-of-scope
classification, and requirement coverage are present (60 matching lines).

```bash
git diff --check
```

Result: passed (exit 0).

```bash
py -3 %USERPROFILE%/.coding-encoding-guard/encoding_guard.py .planning/phases/99-wipe-tower-geometry-gap-audit/99-GAP-MATRIX.md .planning/phases/99-wipe-tower-geometry-gap-audit/99-VERIFICATION.md .planning/phases/99-wipe-tower-geometry-gap-audit/99-01-SUMMARY.md
```

Result: passed for all three Phase 99 artifacts.

## Build Decision

The canonical build command was not run for Phase 99:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Reason: Phase 99 changed planning artifacts only. Per the plan, full canonical
build, app launch, and runtime wipe-tower visibility visual evidence belong to
Phase 102 after the geometry readback (Phase 100) and the rendering-upgrade
baseline (Phase 101) land.

## Conclusion

Phase 99 is complete. Phases 100-102 can now implement against the frozen
wipe-tower geometry matrix without rediscovering the current placeholder box
path, the hardcoded viewport defaults, the upstream `Print::wipe_tower_data()`
anchors, the post-slice readback integration point, the rendering-upgrade
decision, the `has_wipe_tower()` gate, or the verification routes.
