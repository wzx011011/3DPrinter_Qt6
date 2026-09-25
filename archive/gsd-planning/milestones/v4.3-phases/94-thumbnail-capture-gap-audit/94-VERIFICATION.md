---
phase: 94-thumbnail-capture-gap-audit
verified: 2026-07-09
status: passed
requirements: [THUMBAUDIT-01, THUMBAUDIT-02]
canonical_build_run: false
canonical_build_reason: "Documentation/source audit only; no production QML/C++ changed."
---

# Phase 94 Verification Report

## Result

Status: passed.

Phase 94 produced the canonical v4.3 thumbnail capture + 3MF writer gap
matrix. It did not modify product source files, QML resources, CMake files,
tests, or runtime assets. Full canonical build and application launch are
intentionally routed to Phase 98 after implementation phases 95-97 change the
QRhi capture infrastructure, the 3MF write side, and the round-trip test.

## Requirement Coverage

| Requirement | Status | Evidence |
|---|---|---|
| THUMBAUDIT-01 | passed | `94-GAP-MATRIX.md` maps all 10 thumbnail-capture regions (THUMB-MOCK-GENERATOR through THUMB-CLEANUP) to OrcaSlicer upstream source files (with line anchors: `bbs_3mf.cpp:6133,6137,5879,1640`, `PartPlate::store_to_3mf_structure`), current Qt mock paths (`ProjectServiceMock.cpp:4242,4362,4372,5089-5093,5127-5135,5455-5466,5654-5660`; `RhiViewport.cpp:476-488`; `RhiViewport.h:229`; `PartPlate.h:122-124,255`), the QRhi readback approach (Frozen Decisions Option A), the MSAA resolve strategy (single-sample offscreen RT), and verification expectations (Phase 97 round-trip + Phase 98 canonical verifier + visual evidence). |
| THUMBAUDIT-02 | passed | `94-GAP-MATRIX.md` Frozen Decisions section locks three designs before implementation: (1) QRhi readback approach = Option A (offscreen `QRhiTexture` render-target at thumbnail size + `QRhiResourceUpdateBatch::readBackTexture()`), with the rationale that it mirrors upstream's dedicated thumbnail framebuffer and yields a thumbnail-sized single-sample texture; (2) MSAA resolve approach = render the thumbnail to a single-sample (sample count 1) offscreen RT so no resolve step is needed at readback (citing `RhiViewport.cpp:47 setSampleCount(4)`); (3) render-thread capture request queue + QImage callback = mirror the existing `m_fitRequestCount`/`m_viewPreset` synchronize() pattern (`RhiViewport.h:314-315`, `RhiViewport.cpp:415,435,442`, `RhiViewportRenderer.cpp:35`), with the readback inside `render()` and the QImage delivered back via a queued signal/connection. |

## Matrix Coverage

The matrix includes all required region IDs (10 of 10):

- THUMB-MOCK-GENERATOR
- THUMB-REQUEST-STUB
- THUMB-WRITE-PLATE
- THUMB-WRITE-STOREPARAMS
- THUMB-READ-SIDE
- THUMB-RHI-READBACK
- THUMB-MSAA-RESOLVE
- THUMB-RT-QUEUE
- THUMB-PLATE-CACHE
- THUMB-CLEANUP

The matrix table header lists all 10 columns in order: Region, Current Mock
Path, Upstream Anchor, Qt Integration Point, Decision, Gap, Severity, Owner
Phase, Requirement, Verification.

The matrix records exact Qt target paths for the mock generator
(`ProjectServiceMock.cpp:4242,4362,4372`), the request stub
(`RhiViewport.cpp:476-488`, `RhiViewport.h:229`), the write-side gaps
(`ProjectServiceMock.cpp:5089-5093,5127-5135`), the read side
(`ProjectServiceMock.cpp:5455-5466,5654-5660`), and the destination cache
(`PartPlate.h:122-124,255`). It also records OrcaSlicer upstream source
anchors with line citations for `bbs_3mf.cpp:6133` (writer reads
`StoreParams::thumbnail_data`), `bbs_3mf.cpp:6137`
(`_add_thumbnail_file_to_archive` invocation) + `:5879` (declaration),
`bbs_3mf.cpp:1640` (`_extract_from_archive`), and `PartPlate::store_to_3mf_structure`.

## Upstream Anchor Citations

| Region | Upstream Anchor | Verified |
|---|---|---|
| THUMB-MOCK-GENERATOR | `PartPlate::store_to_3mf_structure` (buildPlateDataList source-truth anchor) | yes |
| THUMB-REQUEST-STUB | Upstream dedicated thumbnail framebuffer (maps to Option A); Qt entry `RhiViewport.h:229` | yes |
| THUMB-WRITE-PLATE | `bbs_3mf.cpp:6137` (`_add_thumbnail_file_to_archive` invocation) + `:5879` (declaration) | yes |
| THUMB-WRITE-STOREPARAMS | `bbs_3mf.cpp:6133` (writer reads `StoreParams::thumbnail_data`) | yes |
| THUMB-READ-SIDE | `bbs_3mf.cpp:1640` (`_extract_from_archive` populates `plate->plate_thumbnail.pixels`) | yes |
| THUMB-RHI-READBACK | Upstream dedicated thumbnail framebuffer (Option A offscreen RT) | yes |
| THUMB-MSAA-RESOLVE | Upstream resolves before producing the thumbnail image; cites `RhiViewport.cpp:47 setSampleCount(4)` | yes |
| THUMB-RT-QUEUE | Upstream dispatches capture to the render context; cites the `m_fitRequestCount`/`m_viewPreset` synchronize() template | yes |
| THUMB-PLATE-CACHE | `PartPlate::thumbnail_data` destination (upstream `PartPlate.hpp` cache) | yes |
| THUMB-CLEANUP | No-dead-paths rule (No Deprecated UI Rule from PROJECT.md) | yes |

Each anchor was read from the current Qt6 source tree (`src/core/services/
ProjectServiceMock.cpp`, `src/qml_gui/Renderer/RhiViewport.cpp`,
`RhiViewport.h`, `RhiViewportRenderer.cpp`, `src/core/model/PartPlate.h`) and
confirmed against the line content cited in `94-CONTEXT.md`. The OrcaSlicer
upstream anchors (`bbs_3mf.cpp:6133,6137,5879,1640`) were carried forward from
the pre-planning exploration and the v3.2/v4.3 requirements.

## Frozen Decisions

The `Frozen Decisions (THUMBAUDIT-02)` section is present and locks all three
designs:

1. **QRhi readback (THUMB-RHI-READBACK)** — Option A (offscreen QRhiTexture
   render-target at thumbnail size + `readBackTexture`) locked over Option B
   (live `renderTarget()` readback + downscale), citing upstream's dedicated
   thumbnail framebuffer. Owner Phase 95; Requirement THUMBCAP-01.
2. **MSAA resolve (THUMB-MSAA-RESOLVE)** — render the thumbnail to a
   single-sample (sample count 1) offscreen RT so no resolve step is needed,
   citing `RhiViewport.cpp:47 setSampleCount(4)` as the reason the on-screen RT
   is multisampled. Owner Phase 95; Requirement THUMBCAP-02.
3. **Render-thread capture queue (THUMB-RT-QUEUE)** — mirror the existing
   `m_fitRequestCount`/`m_viewPreset` synchronize() pattern
   (`RhiViewport.h:314-315`, `RhiViewport.cpp:415,435,442`,
   `RhiViewportRenderer.cpp:35`), with readback inside `render()` and QImage
   delivered back via a queued signal/connection. Owner Phase 95; Requirement
   THUMBCAP-03.

## Out-of-Scope Classification

The `Out-of-Scope Classification` section names auto filament-map + wipe-tower,
CLI fixtures, D3D12/Vulkan, GLGizmoMeasure engine, and the removed
LAN/device/cloud/network/Monitor/ModelMall/camera scope. No removed scope was
reintroduced.

## Requirement Routing

The `v4.3 Requirement Routing` table lists all 12 v4.3 requirements
(THUMBAUDIT-01, THUMBAUDIT-02, THUMBCAP-01, THUMBCAP-02, THUMBCAP-03,
THUMBWRITE-01, THUMBWRITE-02, THUMBWRITE-03, THUMBRT-01, THUMBRT-02,
THUMBVERIFY-01, THUMBVERIFY-02) with their owner phase and matrix region(s).

## Commands Run

```bash
rg -n "THUMB-MOCK-GENERATOR|THUMB-REQUEST-STUB|THUMB-WRITE-PLATE|THUMB-WRITE-STOREPARAMS|THUMB-READ-SIDE|THUMB-RHI-READBACK|THUMB-MSAA-RESOLVE|THUMB-RT-QUEUE|THUMB-PLATE-CACHE|THUMB-CLEANUP" .planning/phases/94-thumbnail-capture-gap-audit/94-GAP-MATRIX.md
```

Result: passed; all 10 region IDs are present.

```bash
rg -n "bbs_3mf.cpp:6133|bbs_3mf.cpp:6137|bbs_3mf.cpp:5879|bbs_3mf.cpp:1640|ProjectServiceMock.cpp:4242|ProjectServiceMock.cpp:476|ProjectServiceMock.cpp:5089|ProjectServiceMock.cpp:5127|ProjectServiceMock.cpp:5455|RhiViewport.cpp:476|RhiViewport.h:229|PartPlate.h:122|PartPlate.h:255|store_to_3mf_structure|_add_thumbnail_file_to_archive|_extract_from_archive" .planning/phases/94-thumbnail-capture-gap-audit/94-GAP-MATRIX.md
```

Result: passed; all upstream + Qt anchors are present.

```bash
rg -n "Frozen Decisions|THUMB-RHI-READBACK|THUMB-MSAA-RESOLVE|THUMB-RT-QUEUE|readBackTexture|setSampleCount|renderTarget|synchronize|m_fitRequestCount|m_viewPreset|Option A|Option B|THUMBAUDIT-01|THUMBAUDIT-02|THUMBCAP|THUMBWRITE|THUMBRT|THUMBVERIFY" .planning/phases/94-thumbnail-capture-gap-audit/94-GAP-MATRIX.md
```

Result: passed; frozen decisions, requirement routing, out-of-scope
classification, and requirement coverage are present.

```bash
git diff --check
```

Result: passed (exit 0).

```bash
py -3 %USERPROFILE%/.coding-encoding-guard/encoding_guard.py .planning/phases/94-thumbnail-capture-gap-audit/94-GAP-MATRIX.md .planning/phases/94-thumbnail-capture-gap-audit/94-VERIFICATION.md .planning/phases/94-thumbnail-capture-gap-audit/94-01-SUMMARY.md
```

Result: passed for all three Phase 94 artifacts.

## Build Decision

The canonical build command was not run for Phase 94:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Reason: Phase 94 changed planning artifacts only. Per the plan, full canonical
build, app launch, and runtime thumbnail capture visual evidence belong to
Phase 98 after QRhi capture infrastructure (Phase 95), 3MF write integration
(Phase 96), and round-trip testing (Phase 97) land.

## Conclusion

Phase 94 is complete. Phases 95-98 can now implement against the frozen
thumbnail capture matrix without rediscovering the current mock paths, the
deferred write-side gaps, the upstream writer anchors, the QRhi readback
decision, the MSAA resolve strategy, the render-thread capture queue design,
the destination PartPlate cache, the cleanup candidates, or the verification
routes.
