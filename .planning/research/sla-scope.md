# SLA Scope Investigation — v5.1 Planning Input

**Investigated:** 2026-07-17 (read-only, post-v5.0 ship)
**Purpose:** determine whether v5.1 should be the SLA milestone or a smaller v5.0-cleanup milestone.

## Two key findings that change planning

### 1. The v5.0 audit framing was wrong — SLA is already compiled and linked
The v5.0 audit / Phase 143 SUMMARY said SLAPrint needs "wiring from scratch (~35 files in libslic3r/SLA + SLAPrint.cpp 1261 lines + SLAPrintSteps)". Investigation of build_p142c.utf8.log shows **all of those files already compile and link** into `libslic3r_from_source`:
- `[156/340] SLAPrint.cpp.obj` + `[164/340] SLAPrintSteps.cpp.obj`
- `[254-270/340] SLA/Clustering, ConcaveHull, IndexedMesh, Pad, RasterToPolygons, RasterBase, SpatIndex, Hollowing, SupportTreeBuildsteps, SupportTreeBuilder, SupportPointGenerator, SupportTreeMesher`
- `[172/340] Zipper.cpp.obj`, `[175/340] miniz_extension.cpp.obj`, `[221/340] Format/SL1.cpp.obj`, `[225/340] Format/ZipperArchiveImport.cpp.obj`

The CMake globs (`BuildLibslic3rFromSource.cmake:185-188, 264-268`) pick up `*.cpp` + `SLA/*.cpp` automatically. The `.sl1` zip-archive writer is reachable from Qt today. **No new dependencies, no new link surface.**

### 2. The real blocker is 4 phases of Qt orchestration, not 35 files
What's actually missing:
- `SliceService.cpp` is FFF-hard-coded (line 519: `Slic3r::Print print;`). No `printer_technology` switch.
- `PresetServiceMock.h:18` — only 3 categories (Print/Filament/Printer); no SLA category, no `printer_technology` config option read.
- Export dialog hard-codes `.gcode` extension.
- `case 18` (SLA supports gizmo) returns false.
- Zero hollow/drain-hole 3MF persistence.

The minimal "close VDB-06" path is 4 phases (wire-up + schema-min + output dialog + verify), NOT 8-10.

## Scope clusters

| Cluster | Size | Phase count |
|---|---|---|
| Wire-up (PrinterTechnology switch in SliceService + SL1Archive output + forward hollow Q_PROPERTYs) | small | 1 |
| Schema minimal (1 SLA printer + 1 material + 1 print preset synthesized; printer_technology read) | medium | 2 |
| Output dialog (.sl1 extension + exportArchiveToPath API) | small-medium | 1 |
| UI SLA-aware (case 18 → SLA gate; hide FFF-only options) | medium | 1-2 |
| SlaSupports gizmo port (1237 lines + IMGUI + RHI point rendering) | large | 3-4 |
| 3MF hollow persistence | small-medium | 1 |
| FaceDetector gizmo | medium | 1 |

**Minimal VDB-06 close path: 4 phases.**
**Full SLA milestone (with SlaSupports gizmo): 8-10 phases.**

## Phase 143 scaffolding reusability
**Fully reusable as-is.** Every Phase 143 artifact (7 Hollow Q_PROPERTYs + button + panel + case 8 reachability) maps 1:1 to upstream `SLAPrintObjectConfig` keys. Only the downstream consumer (SLAPrint) needs connecting.

## VDB-06 acceptance correction
The original VDB-06 says "produces a non-solid G-code". This is **factually wrong for SLA** (SLAPrint emits `.sl1`, not G-code). VDB-06 must be reworded: "produces a `.sl1` archive whose layer PNGs show a visible cavity when hollowEnabled=true."

## Alternative smaller v5.1 (high-ROI)
Close the 4 other v5.0 deferred items (each is 1 phase, primitives already shipped):
- **PSET-05** QML diff-view consumer for the existing comparePresets primitive
- **EMB-06** editable-text 3MF metadata via TextConfigurationSerialization
- **PLATE-05** runtime thumbnail capture scheduler
- **PLATE-06** live multi-plate round-trip ctest
- Plus Emboss style/SVG follow-ups (variable fonts, system-font enumeration)

Each is a 1-phase completion of an already-shipped primitive. Lower risk, faster cycle, no scope surprise. SLA then becomes v5.2 with proper scope (4-phase VDB-06 close + SlaSupports gizmo).

## Recommendation

**Defer SLA to v5.2.** Do v5.1 as a small "v5.0 deferred items closure" milestone. Rationale:
1. v5.0 shipped 4 documented partials that each need ~1 phase to close (low-risk completions of primitives that already exist).
2. SLA is now known to be 4 phases (not 8-10 as the v5.0 audit suggested) — but it's still bigger than a cleanup milestone, and benefits from its own focused scope.
3. v5.1 cleanup → v5.2 SLA is a cleaner cadence than v5.1 SLA (which would leave the 4 partials dangling further).
