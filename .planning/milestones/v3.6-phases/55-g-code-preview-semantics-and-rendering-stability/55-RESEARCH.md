# Phase 55: G-code Preview Semantics and Rendering Stability - Research

**Researched:** 2026-07-02
**Domain:** OrcaSlicer G-code preview rendering (libslic3r GCodeProcessor + libvgcode Viewer + Qt6/QML RhiViewport)
**Confidence:** HIGH

## Summary

This phase completes the G-code preview behavior behind the Phase 54-restored Preview UI. The core technical challenge is closing three semantic gaps between our current implementation and upstream OrcaSlicer: (1) our parser maps G-code `;TYPE:` comments to only 5 coarse feature categories, but upstream has 20 fine-grained `EGCodeExtrusionRole` values; (2) our `StoredSegment` and `GcvPackedSegment` lack a role/extrusion-role field needed for per-role visibility filtering; (3) our 13 view modes are incomplete against upstream's 17 `EViewType` values.

The upstream uses `libvgcode` (a standalone C++ library inside OrcaSlicer's source tree) with a clean separation: `Settings` struct holds visibility arrays, `ViewerImpl::update_enabled_entities()` builds an `enabled_segments` vector per frame by checking each `PathVertex` against the visibility masks, and the renderer draws only enabled segments. This is purely draw-time filtering -- no vertex buffer repack. Our renderer already has a draw-span pattern (`PreviewDrawSpan`) in `RhiViewportRenderer` that can be extended with a role tag for identical render-side filtering.

**Primary recommendation:** Add an `int role` field to both `StoredSegment` and `PackedSegment`/`GcvPackedSegment`, extend the `PreviewDrawSpan` struct with an `int role` field, and add a `QVector<bool>` role-visibility mask to `RhiViewport` properties. The renderer's `computePreviewDrawRange()` skips spans whose role is not visible. No repack needed on toggle -- only `update()`.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- Add the **full upstream-aligned per-role visibility set** mapped to `StoredSegment` roles: Travel, Wipe, Seam, Perimeter, External Perimeter, Infill, Solid Infill, Top Solid Infill, Support, Bridge (and others present in upstream `GCodeViewer` move-type visibility list). Audit the upstream `EMoveType` / role list first, then expose what maps to parsed data.
- House the filter UI in a collapsible **"Visible" group inside the right legend/statistics panel**, matching upstream `GCodeViewer`'s right-side visibility list and the restored Phase 54 right panel. (Not left panel; not a top-bar popover.)
- Filtering is **render-side**: a visibility toggle flip updates draw filtering over the already-uploaded segment buffer and calls `update()` only. It must **not** repack `gcodePreviewData`. Repacking happens only on color-mode change, payload change, or resource rebuild. This preserves the Phase 41 interaction-stability guarantee.
- Default visibility matches upstream: travel and wipe hidden after first view; extrusion roles visible. (Not all-on; not all-off-except-extrusion.)
- The 13 existing view modes are the candidate complete set. **Audit against upstream `GCodeViewer::EViewType`** and close any gap (e.g. custom-gcode / extra modes) rather than redesigning the list.
- Legend scope under slider filtering is **global** (full slice): min/max/colors reflect the whole toolpath; the per-move current value follows the cursor via the tool-position tooltip. Legend must **not** recompute on every slider drag.
- Gradient legend min/max computed per `StoredSegment` field across all segments, **recomputed only on recolor (mode change)**, not on slider/toggle interaction.
- Keep the existing `stealthMode` property; expose a two-segment Normal/Stealth estimate control near the view-mode combo.
- Prove no-placeholder via a **RED test on the live local slice path**: run a deterministic slice over a fixture, assert `gcodePreviewData` non-empty, `layerCount > 0`, `moveCount > 0`, and that no segment carries placeholder/demo/sample marker bytes.
- D3D11 no-regression: source-audit that `PreviewPage.qml` never references `SoftwareViewport`, that `main_qml.cpp` registers `RhiViewport` as `GLViewport` on the default path.
- Drag/page-switch/reslice/export regression: deterministic source/audit tests extending Phase 41's `QmlUiAuditTests`, centered on the **GCV1 payload-survives-interaction invariant**.

### Claude's Discretion

- Exact upstream `EMoveType` -> `StoredSegment` role mapping when a 1:1 mapping is ambiguous.
- Internal helper naming for render-side visibility filtering (e.g. a role-mask bitfield on `PackedSegment` / `GcvPackedSegment`).
- How the right-panel "Visible" group is laid out (checkbox list density, scroll behavior) as long as it matches the screenshot and upstream semantics.

### Deferred Ideas (OUT OF SCOPE)

- D3D12 crash root-cause and Vulkan default promotion remain separate backend work.
- Pixel-perfect visual regression capture for G-code preview remains Phase 58 manual UAT.
- Full upstream Preview rendering parity beyond the v3.6 local workflow.
- Device/cloud Preview and send workflows remain outside v3.6.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| GCODE-01 | Real G-code segment/layer metadata from slicing/export, no placeholder dependency | No-placeholder data path traced (Section 4); existing `rebuildFromGCode()` uses real parser on live slice output; RED test design in Validation Architecture |
| GCODE-02 | Color modes + per-role line-type filters (travel/perimeter/infill/support/skirt/brim/wipe tower) | Upstream `EGCodeExtrusionRole` (20 values) and `EOptionType` (9 values) identified with source file:line refs (Section 2); render-side filtering architecture mapped (Section 3); `StoredSegment` role gap identified |
| GCODE-03 | Layer/move filtering coherence: draw range + legend + G-code text/current-line | Legend coherence confirmed global-scope with source (Section 5); existing `computePreviewDrawRange()` is correct for layer/move; G-code text sync path traced |
| GCODE-04 | D3D11 QRhi path, no SoftwareViewport regression | `main_qml.cpp` registration audited; `PreviewPage.qml` confirmed SoftwareViewport-free; `QmlUiAuditTests.cpp` guards extendable |
| GCODE-05 | Regression tests: camera drag, layer drag, move drag, page switch, reslice, export | Existing `E2EWorkflowTests.cpp` payload-survival pattern documented; new invariants enumerated (Section 7) |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Per-role extrusion classification | API / Backend (PreviewViewModel parser) | -- | G-code `;TYPE:` comment parsing is data classification logic, not rendering. Upstream `ExtrusionEntity::role_to_string()` defines the source-of-truth mapping. |
| Per-role visibility mask state | API / Backend (PreviewViewModel) | -- | Visibility state is UI state owned by the ViewModel, exposed as Q_PROPERTY. Upstream `Settings::extrusion_roles_visibility` is a parallel concept. |
| Render-side draw filtering | Renderer (RhiViewportRenderer) | QML (RhiViewport property binding) | The renderer's `computePreviewDrawRange()` already filters by layer/move; adding role filtering follows the same draw-time skip pattern. No repack needed. |
| View-mode recolor | API / Backend (PreviewViewModel) | -- | CPU-prebaked colors in `recolorAndPackSegments()` -- this already works for 13 modes, extending to 17 is the same pattern. |
| Legend gradient min/max | API / Backend (PreviewViewModel) | -- | `buildLegendItems()` computes min/max across all visible segments at recolor time. Must remain global-scope, not filtered by slider. |
| G-code text / current-line | API / Backend (PreviewViewModel) | -- | `rebuildGcodeLineWindow()` and `currentGcodeLine` are ViewModel responsibilities. |
| VisibilityFilter UI | Browser / Client (QML) | -- | Pure presentation: checkbox list bound to ViewModel toggles. Matches upstream ImGui legend pattern. |

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Qt 6.10 (QRhi) | 6.10 | RhiViewport renderer backend (D3D11 default) | Project-locked Qt version; D3D11 is the known-stable Windows QRhi backend |
| Qt Test (QtTest) | 6.10 | Regression tests (QmlUiAuditTests, E2EWorkflowTests) | Project-locked test framework; QSignalSpy + source-audit pattern established |

### Supporting
| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| CxCheckBox | project-local | Per-role visibility checkboxes in VisibilityFilter | Reuse existing control -- matches StatsPanel toggle pattern |
| CxScrollView | project-local | Right panel scroll container | Already wraps StatsPanel + Legend |
| CollapsibleSection | project-local | Section header with collapse/expand toggle | Pattern from left sidebar "盘与层" and right panel "分析" sections |
| Theme singleton | project-local | All spacing, color, and font tokens | Mandatory per CLAUDE.md design system |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Render-side role filtering (skip in computePreviewDrawRange) | Repack gcodePreviewData on toggle | Repack is O(n) per toggle, breaks Phase 41 interaction-stability guarantee, and is explicitly forbidden by CONTEXT.md |
| Adding `int role` to GcvPackedSegment | Bitfield packing of role into unused float bits | Bitfield requires bit-level manipulation on every filter check; `int role` is 4 bytes, adds clarity, and sizeof(PackedSegment) is not a bottleneck (GPU upload is the cost, not struct size) |

**Installation:**
No external packages needed. All dependencies are project-local (Cx* controls, Theme) or framework-locked (Qt 6.10).

**Version verification:** No npm/PyPI packages to verify. All libraries are either project-local or Qt framework.

## Package Legitimacy Audit

> No external packages are installed in this phase. All UI components are project-local Cx* controls and Theme singleton. No registry verification needed.

**Packages removed due to slopcheck [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```
G-code File (.gcode)
    |
    v
[SliceService] --- sliceFinished / resultChanged / sliceFailed / sliceResultCleared --->|
    |                                                                             |
    v                                                                             |
[PreviewViewModel::rebuildFromGCode()]                                          |
    |  Parses ;TYPE: comments into EGCodeExtrusionRole (NEW: fine-grained)       |
    |  Produces: StoredSegment{ x,y,z, baseR/baseG/baseB, feedrate, ..., role }  |
    |                                                                            |
    v                                                                            |
[PreviewViewModel::recolorAndPackSegments()]                                     |
    |  Mode 0 (FeatureType): uses role colors from DEFAULT_EXTRUSION_ROLES_COLORS |
    |  Modes 1..16: gradient / tool / extruder coloring                           |
    |  Packs: PackedSegment{ x,y,z, r,g,b, ..., layer, move, role (NEW) }        |
    |  NOTE: role field currently MISSING from PackedSegment -- must be added      |
    |                                                                            |
    v                                                                            |
[gcodePreviewData] QByteArray (GCV1 magic + count + packed segments)             |
    |                                                                            |
    +--- Q_PROPERTY binding ---> [RhiViewport::previewData] -----> update()      |
    |                                                                            |
    v                                                                            |
[RhiViewportRenderer::parsePreviewSegments()]                                    |
    |  Unpacks GCV1 -> m_previewVertices + m_previewDrawSpans                     |
    |  Each PreviewDrawSpan{ layer, move, vertexOffset, vertexCount, role (NEW) } |
    |                                                                            |
    v                                                                            |
[RhiViewportRenderer::computePreviewDrawRange()]                                  |
    |  For each span: if span.layer in [layerMin, layerMax]                       |
    |                 AND span.move < moveEnd                                     |
    |                 AND m_roleVisibility[span.role] (NEW)                       |
    |                 then include in draw range                                 |
    |                                                                            |
    v                                                                            |
[QRhi GPU draw call]                                                             |
    |                                                                            |
    +--- [VisibilityFilter.qml] <--- toggleRoleVisibility(i) --+                |
         Bound to previewVm.isRoleVisible(i)                    |                |
         Each checkbox -> PreviewViewModel::toggleRoleVisibility(i)             |
         ViewModel emits stateChanged() -> RhiViewport.update()                 |
         Renderer reads new m_roleVisibility without repacking                   |
                                                                             sliceFinished ---> rebuild payload
                                                                             sliceFailed  ---> resetPreviewState()
```

### Recommended Project Structure
```
src/
  core/viewmodels/
    PreviewViewModel.h          # Add: role field, roleVisibilities property, toggleRoleVisibility()
    PreviewViewModel.cpp         # Add: fine-grained ;TYPE: parser, role-to-color mapping, role on PackedSegment
  qml_gui/
    Renderer/
      RhiViewport.h             # Add: roleVisibility property (QVector<bool> or QBitArray)
      RhiViewport.cpp           # Add: setRoleVisibility(), pass to renderer sync
      RhiViewportRenderer.h     # Add: role field to PreviewDrawSpan, m_roleVisibility array
      RhiViewportRenderer.cpp   # Extend: computePreviewDrawRange() role skip, parse role from GcvPackedSegment
    components/
      VisibilityFilter.qml      # NEW: Collapsible per-role checkbox group
    pages/
      PreviewPage.qml           # Insert VisibilityFilter between StatsPanel and Legend
  tests/
    QmlUiAuditTests.cpp         # Extend: SoftwareViewport guard, payload-survival invariants
    E2EWorkflowTests.cpp        # Extend: no-placeholder RED test after live slice
    ViewModelSmokeTests.cpp     # Extend: role visibility toggle does not repack gcodePreviewData
```

### Pattern 1: Render-Side Role Filtering (upstream `ViewerImpl::update_enabled_entities`)
**What:** Instead of repacking the vertex buffer when visibility toggles, the renderer checks each draw span's role against a visibility mask at draw-range computation time. This is O(visible_spans) per frame, not O(all_segments) repack.
**When to use:** Any per-entity visibility toggle that does not affect color or geometry (only presence/absence).
**Example:**
```cpp
// RhiViewportRenderer::computePreviewDrawRange() -- extended with role check
for (const auto &span : m_previewDrawSpans) {
    if (span.layer < layerLow || span.layer > layerHigh)
        continue;
    if (span.move >= m_moveEnd)
        continue;
    // NEW: render-side role filtering (no repack)
    if (span.role >= 0 && span.role < m_roleVisibility.size()
        && !m_roleVisibility[span.role])
        continue;

    if (!foundStart) {
        startOffset = span.vertexOffset;
        foundStart = true;
    }
    endOffset = span.vertexOffset + span.vertexCount;
}
```
Source: Pattern mirrors upstream `ViewerImpl::update_enabled_entities()` at `src/libvgcode/src/ViewerImpl.cpp:1156-1184`.

### Pattern 2: Upstream `;TYPE:` Comment Parsing
**What:** OrcaSlicer generates G-code with `;TYPE:Inner wall`, `;TYPE:Outer wall`, `;TYPE:Bridge`, etc. comments (via `ExtrusionEntity::role_to_string()`). The parser must match these exact English strings to map to fine-grained roles.
**When to use:** When parsing OrcaSlicer-generated G-code. Also handles `;FEATURE:Type Height H Width W` format and Cura-compatible `;TYPE:wall-outer` format.

**CRITICAL ENUM INDEX MISMATCH (diverges at index 7):** The parser-side enum (`libslic3r ExtrusionRole` in `ExtrusionEntity.hpp:20-43`, used by `string_to_role`/`role_to_string` that produce and consume `;TYPE:` comments) and the color/visibility enum (`libvgcode EGCodeExtrusionRole` in `Types.hpp:131-157`, used by `DEFAULT_EXTRUSION_ROLES_COLORS` + `extrusion_roles_visibility`) have **different orderings past index 6**. The shared display strings (e.g. "Bottom surface", "Ironing") are identical across both enums, but the integer indices are NOT.

| Index | libslic3r ExtrusionRole (parser) | libvgcode EGCodeExtrusionRole (color/visibility) |
|-------|----------------------------------|--------------------------------------------------|
| 0 | erNone | None |
| 1 | erPerimeter | Perimeter |
| 2 | erExternalPerimeter | ExternalPerimeter |
| 3 | erOverhangPerimeter | OverhangPerimeter |
| 4 | erInternalInfill | InternalInfill |
| 5 | erSolidInfill | SolidInfill |
| 6 | erTopSolidInfill | TopSolidInfill |
| **7** | **erBottomSurface** | **Ironing** |
| **8** | **erIroning** | **BridgeInfill** |
| **9** | **erBridgeInfill** | **GapFill** |
| **10** | **erInternalBridgeInfill** | **Skirt** |
| **11** | **erGapFill** | **SupportMaterial** |
| **12** | **erSkirt** | **SupportMaterialInterface** |
| **13** | **erBrim** | **WipeTower** |
| **14** | **erSupportMaterial** | **Custom** |
| **15** | **erSupportMaterialInterface** | **BottomSurface** |
| **16** | **erSupportTransition** | **InternalBridgeInfill** |
| **17** | **erWipeTower** | **Brim** |
| **18** | **erCustom** | **SupportTransition** |
| 19 | erMixed | Mixed |

**Migration strategy (chosen for Plan 02):** Use the **libvgcode `EGCodeExtrusionRole` index as the canonical role index throughout the Qt6 codebase** (StoredSegment.role, PackedSegment.role, GcvPackedSegment.role, PreviewDrawSpan.role, `m_roleVisibility[20]`, render-side skip, `roleVisibilities()` QML ordering, FeatureType color lookup). The `;TYPE:` parser maps the upstream display string DIRECTLY to the libvgcode index via `kRoleMap[]` (do NOT index `kRoleColors` by the libslic3r ExtrusionRole integer). Rationale: (a) the color palette and visibility array are both indexed by `EGCodeExtrusionRole`; (b) the strings match 1:1 across both enums so the parser just needs a string-keyed lookup; (c) a single canonical index everywhere eliminates the dual-indexing bug class.

**Example:**
```cpp
// Maps the upstream display string DIRECTLY to the libvgcode EGCodeExtrusionRole
// index (the canonical role index throughout the Qt6 codebase).
// Source strings: third_party/OrcaSlicer/src/libslic3r/ExtrusionEntity.cpp:583-608 (role_to_string)
// Target indices: third_party/OrcaSlicer/src/libvgcode/include/Types.hpp:131-157 (EGCodeExtrusionRole)
static const struct { const char *name; int role; } kRoleMap[] = {
    {"Inner wall",              1},  // Perimeter
    {"Outer wall",              2},  // ExternalPerimeter
    {"Overhang wall",           3},  // OverhangPerimeter
    {"Sparse infill",           4},  // InternalInfill
    {"Internal solid infill",   5},  // SolidInfill
    {"Top surface",             6},  // TopSolidInfill
    {"Ironing",                 7},  // Ironing             (NOT 8 — that is the libslic3r index)
    {"Bridge",                  8},  // BridgeInfill        (NOT 9)
    {"Gap infill",              9},  // GapFill             (NOT 11)
    {"Skirt",                  10},  // Skirt               (NOT 12)
    {"Support",                11},  // SupportMaterial     (NOT 14)
    {"Support interface",      12},  // SupportMaterialInterface (NOT 15)
    {"Prime tower",            13},  // WipeTower           (NOT 17)
    {"Custom",                 14},  // Custom              (NOT 18)
    {"Bottom surface",         15},  // BottomSurface       (NOT 7)
    {"Internal Bridge",        16},  // InternalBridgeInfill(NOT 10)
    {"Brim",                   17},  // Brim                (NOT 13)
    {"Support transition",     18},  // SupportTransition   (NOT 16)
    {"Multiple",               19},  // Mixed               (identical in both enums)
};
// Travel moves / unrecognized -> 0 (None).
```
Source: parser strings `third_party/OrcaSlicer/src/libslic3r/ExtrusionEntity.cpp:583-639`; canonical indices `third_party/OrcaSlicer/src/libvgcode/include/Types.hpp:131-157` [VERIFIED against both upstream files].

### Anti-Patterns to Avoid
- **Repacking gcodePreviewData on role toggle:** CONTEXT.md explicitly forbids this. Only `update()` should be called on toggle. Repack only on mode change, payload change, or resource rebuild.
- **Adding role filtering in QML:** QML boundary rules prohibit business logic in QML. All filtering logic lives in the renderer (C++).
- **Coarse ;TYPE: parsing:** Current `styleFor()` maps to 5 categories (WALL, INFILL, SUPPORT, TRAVEL, OTHER). This loses the fine-grained role information needed for the 20-value visibility filter. Must be replaced with the full 20-value mapping.
- **Mixing libslic3r and libvgcode role indices:** The two enums diverge past index 6 (see the cross-reference table in Pattern 2). Pick ONE index space (the Qt6 codebase uses libvgcode EGCodeExtrusionRole as canonical) and use it everywhere: parser output, PackedSegment.role, GcvPackedSegment.role, m_roleVisibility[], roleVisibilities() ordering, color lookup.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Per-role extrusion colors | Hardcoded color constants | Upstream `DEFAULT_EXTRUSION_ROLES_COLORS` from `ViewerImpl.cpp:283-305` | Upstream already defines the canonical 20-role color palette. Reproducing it risks drift. |
| View-mode string names | Ad-hoc English strings | Upstream `get_view_type_string()` from `GCodeViewer.cpp:66-103` | Upstream is source-of-truth for display names. |
| ;TYPE: string-to-role mapping | Custom regex matching | Upstream `ExtrusionEntity::string_to_role()` from `ExtrusionEntity.cpp:611-639` | Must parse the exact same strings upstream generates. |
| Gradient color palette | Custom color array | Upstream `Range_Colors` or existing `kRangeColors` from `PreviewViewModel.cpp:72` | Already matches upstream palette. |

**Key insight:** The upstream `libvgcode` library has all the infrastructure we need (role enum, visibility defaults, color palette, draw-time filtering). We should replicate its data model, not invent our own.

## Upstream Enum Reference (Definitive)

### EGCodeExtrusionRole (20 values) -- CANONICAL role index for the Qt6 codebase
Source: `third_party/OrcaSlicer/src/libvgcode/include/Types.hpp:131-157` [VERIFIED: upstream source]

> **CROSS-REFERENCE — DO NOT MIX INDEX SPACES:** The libslic3r `enum ExtrusionRole` in
> `third_party/OrcaSlicer/src/libslic3r/ExtrusionEntity.hpp:20-43` (used by
> `string_to_role`/`role_to_string`, which produce and consume the `;TYPE:` comment
> strings) has a **DIFFERENT ordering** past index 6. The display strings (e.g.
> "Bottom surface", "Ironing") are identical across both enums, but the integer
> indices are NOT. The Qt6 codebase uses the **libvgcode index** as canonical everywhere
> (parser output, PackedSegment.role, GcvPackedSegment.role, m_roleVisibility[],
> roleVisibilities() ordering, color lookup). The parser maps the upstream display
> string DIRECTLY to the libvgcode index via `kRoleMap[]` (see Pattern 2) — it must
> NOT translate via the libslic3r integer.
>
> Divergence summary (libslic3r index → libvgcode index for the same display string):
> BottomSurface 7→15, Ironing 8→7, BridgeInfill 9→8, InternalBridgeInfill 10→16,
> GapFill 11→9, Skirt 12→10, Brim 13→17, SupportMaterial 14→11,
> SupportMaterialInterface 15→12, SupportTransition 16→18, WipeTower 17→13,
> Custom 18→14. Indices 0..6 and 19 are identical in both enums.

The table below is indexed by the **libvgcode** `EGCodeExtrusionRole` (the canonical Qt6 index). All colors come from `DEFAULT_EXTRUSION_ROLES_COLORS` at `third_party/OrcaSlicer/src/libvgcode/src/ViewerImpl.cpp:283-305`, which is also indexed by `EGCodeExtrusionRole`.

| Canonical (libvgcode) Index | EGCodeExtrusionRole | Display Name (upstream) | Default Visible | Default Color (R,G,B) | libslic3r ExtrusionRole (for cross-ref) |
|-----------------------------|---------------------|-------------------------|-----------------|-----------------------|------------------------------------------|
| 0 | None | Unknown | true | (230,179,179) | erNone |
| 1 | Perimeter | Inner wall | true | (255,230,77) | erPerimeter |
| 2 | ExternalPerimeter | Outer wall | true | (255,125,56) | erExternalPerimeter |
| 3 | OverhangPerimeter | Overhang wall | true | (31,31,255) | erOverhangPerimeter |
| 4 | InternalInfill | Sparse infill | true | (176,48,41) | erInternalInfill |
| 5 | SolidInfill | Internal solid infill | true | (150,84,204) | erSolidInfill |
| 6 | TopSolidInfill | Top surface | true | (240,64,64) | erTopSolidInfill |
| 7 | Ironing | Ironing | true | (255,140,105) | erIroning (libslic3r idx 8) |
| 8 | BridgeInfill | Bridge | true | (77,128,186) | erBridgeInfill (libslic3r idx 9) |
| 9 | GapFill | Gap infill | true | (255,255,255) | erGapFill (libslic3r idx 11) |
| 10 | Skirt | Skirt | true | (0,135,110) | erSkirt (libslic3r idx 12) |
| 11 | SupportMaterial | Support | true | (0,255,0) | erSupportMaterial (libslic3r idx 14) |
| 12 | SupportMaterialInterface | Support interface | true | (0,128,0) | erSupportMaterialInterface (libslic3r idx 15) |
| 13 | WipeTower | Prime tower | true | (179,227,171) | erWipeTower (libslic3r idx 17) |
| 14 | Custom | Custom | true | (94,209,148) | erCustom (libslic3r idx 18) |
| 15 | BottomSurface | Bottom surface | true | (102,92,199) | erBottomSurface (libslic3r idx 7) |
| 16 | InternalBridgeInfill | Internal bridge | true | (77,128,186) | erInternalBridgeInfill (libslic3r idx 10) |
| 17 | Brim | Brim | true | (0,59,110) | erBrim (libslic3r idx 13) |
| 18 | SupportTransition | Support transition | true | (0,64,0) | erSupportTransition (libslic3r idx 16) |
| 19 | Mixed | Mixed | true | (128,128,128) | erMixed |

**UI-SPEC note:** The UI-SPEC lists 18 roles (excluding None and Custom). This is correct for the VisibilityFilter UI -- upstream `GCodeViewer::render_legend()` also shows only roles present in the G-code, not None/Custom. The QML `roleVisibilities()` list returns rows in canonical (libvgcode) index order; the UI row order therefore matches the table above (excluding index 0 None and index 14 Custom).

**Default visibility:** All extrusion roles default to `true` [VERIFIED: `third_party/OrcaSlicer/src/libvgcode/src/Settings.hpp:49-71`].

### EOptionType (move-type visibility, 9 values)
Source: `third_party/OrcaSlicer/src/libvgcode/include/Types.hpp:164-182` [VERIFIED: upstream source]

| Index | EOptionType | Default Visible | Default Color |
|-------|------------|-----------------|---------------|
| 0 | Travels | **false** | (56,72,155) |
| 1 | Wipes | **false** | (255,255,0) |
| 2 | Retractions | false | (205,34,214) |
| 3 | Unretractions | false | (73,173,207) |
| 4 | Seams | true | (230,230,230) |
| 5 | ToolChanges | false | (193,190,99) |
| 6 | ColorChanges | false | (218,148,139) |
| 7 | PausePrints | false | (82,240,131) |
| 8 | CustomGCodes | false | (226,210,67) |

**Note:** Upstream options_items (visible in UI) lists only: Travel, Retract, Unretract, Wipe, Seam (from `GCodeViewer.cpp:1105-1113`). These are the ones that get toggle checkboxes. ToolChanges, ColorChanges, PausePrints, CustomGCodes are not shown in the normal UI. CONTEXT.md says "travel and wipe hidden after first view" which matches Travel=false and Wipe=false defaults. Our current `showTravelMoves` defaults to `true` -- Plan 02 changes this to `false` to match upstream (Pitfall 3).

### EViewType (17 values -- upstream complete list)
Source: `third_party/OrcaSlicer/src/libvgcode/include/Types.hpp:80-103` [VERIFIED: upstream source]

| Index | EViewType | Upstream Display Name | Current? | Notes |
|-------|-----------|----------------------|----------|-------|
| 0 | Summary | Summary | **MISSING** | Non-color mode, just statistics display. Upstream puts it first. |
| 1 | FeatureType | Line Type | Yes (index 0) | Feature/role coloring |
| 2 | ColorPrint | Filament | **MISMATCH** | Current index 9 is "Flow" -- should be "Filament" (ColorPrint) |
| 3 | Speed | Speed | Yes (index 4) | |
| 4 | ActualSpeed | Actual Speed | **MISSING** | Separate from Speed |
| 5 | Height | Layer Height | Yes (index 1) | |
| 6 | Width | Line Width | Yes (index 2) | |
| 7 | VolumetricFlowRate | Flow | **MISMATCH** | Current index 9 "Flow" maps to VolumetricFlowRate but named wrong. Upstream calls it "Flow". |
| 8 | ActualVolumetricFlowRate | Actual Flow | **MISSING** | Separate from VolumetricFlowRate |
| 9 | LayerTimeLinear | Layer Time | Yes (index 10) | |
| 10 | LayerTimeLogarithmic | Layer Time (log) | Yes (index 11) | |
| 11 | FanSpeed | Fan Speed | Yes (index 5) | |
| 12 | Temperature | Temperature | Yes (index 6) | |
| 13 | PressureAdvance | Pressure Advance | **MISSING** | ORCA-specific. StoredSegment has no pressure_advance field. |
| 14 | Acceleration | Acceleration | Yes (index 12) | |
| 15 | Jerk | Jerk | **MISSING** | StoredSegment has no jerk field. |
| 16 | Tool | Tool | Yes (index 3) | Upstream puts Tool last ("for first layer inspection") |

**Corrected view-mode list** (upstream order from `update_by_mode()`):
```
0: Summary          (NEW -- statistics only, no gradient legend)
1: Line Type        (was 0)
2: Filament         (was "Flow" at 9 -- renamed to match upstream "ColorPrint" -> "Filament")
3: Speed            (was 4)
4: Actual Speed     (NEW)
5: Acceleration     (was 12)
6: Jerk             (NEW -- requires jerk field on StoredSegment)
7: Layer Height     (was 1)
8: Line Width       (was 2)
9: Flow             (was index 7 "Filament" -- rename to upstream "VolumetricFlowRate" -> "Flow")
10: Actual Flow      (NEW)
11: Layer Time       (was 10)
12: Layer Time (log) (was 11)
13: Fan Speed        (was 5)
14: Temperature      (was 6)
15: Pressure Advance (NEW -- requires pressure_advance field on StoredSegment)
16: Tool             (was 3 -- upstream puts it last)
```

**Impact on existing code:** The mode index change is a BREAKING change for the `recolorAndPackSegments()` switch statement and the legend builder. All mode-index-based branching must be updated. StoredSegment needs new fields for Jerk, PressureAdvance, and ActualSpeed.

**UI-SPEC correction:** The UI-SPEC listed 13 modes and flagged index 9 "Flow" as mismatch. Research confirms: "Flow" at index 9 should be "Filament" (ColorPrint). The full list has 17 modes, not 13. Summary is a non-color mode that may not need a gradient legend. Jerk and PressureAdvance require new StoredSegment fields from upstream `;JERK:` and `;PA:` tagged comments or GCodeProcessor computed fields.

## Render-Side Filtering Integration Points

### Current GCV1 Wire Format (needs extension)
**File:** `src/core/viewmodels/PreviewViewModel.cpp:46-66` (PackedSegment in anonymous namespace)
**File:** `src/qml_gui/Renderer/RhiViewportRenderer.cpp:571-577` (GcvPackedSegment in anonymous namespace)

Current PackedSegment layout (no role field):
```cpp
struct PackedSegment {
    float x1, y1, z1, x2, y2, z2;  // 24 bytes
    float r, g, b;                    // 12 bytes
    float feedrate, fan_speed, temperature, width, layer_time, acceleration; // 24 bytes
    int extruder_id, layer, move;    // 12 bytes
};  // Total: 72 bytes
```

**Required change:** Add `int role;` at the end (grows to 76 bytes). Both PackedSegment (PreviewViewModel.cpp) and GcvPackedSegment (RhiViewportRenderer.cpp) must be updated in lockstep. The role value is the **canonical libvgcode EGCodeExtrusionRole index** (0..19) — see the cross-reference table in "Upstream Enum Reference".

### Current PreviewDrawSpan (needs extension)
**File:** `src/qml_gui/Renderer/RhiViewportRenderer.h:105-110`

```cpp
struct PreviewDrawSpan {
    int layer;
    int move;
    quint32 vertexOffset;
    quint32 vertexCount;
};
```

**Required change:** Add `int role;` field (canonical libvgcode index).

### Filter Check Location
**File:** `src/qml_gui/Renderer/RhiViewportRenderer.cpp:648-713` (`computePreviewDrawRange`)

The role check goes at line ~695 (after the move check, before the startOffset/endOffset logic):
```cpp
if (span.role >= 0 && span.role < m_roleVisibility.size()
    && !m_roleVisibility[span.role])
    continue;
```

### RhiViewport Property (new)
**File:** `src/qml_gui/Renderer/RhiViewport.h`

Add: `Q_PROPERTY(QVector<bool> roleVisibility READ roleVisibility WRITE setRoleVisibility NOTIFY roleVisibilityChanged)`

The renderer sync code at `RhiViewportRenderer.cpp:60-91` already reads `m_previewData`, `m_layerMin`, `m_layerMax`, `m_moveEnd`, `m_showTravelMoves`, `m_gcodeViewMode` from the viewport. Add `m_roleVisibility` to this sync block.

## No-Placeholder Data Path Trace

### Signal -> Rebuild Chain
```
SliceService::sliceFinished
    -> PreviewViewModel::syncPreviewWithActiveResult()
       -> SliceService::outputPath()  // real .gcode file path
       -> rebuildFromGCode(realPath)  // real G-code parser
          -> resetPreviewState()
          -> Parse G-code line-by-line: ;TYPE:, ;HEIGHT:, ;WIDTH:, M/G commands
          -> Produces StoredSegment[] with real feedrate, fan, temp, accel, etc.
          -> recolorAndPackSegments()
             -> GCV1 binary blob
          -> emit stateChanged()
       -> QML binding updates: previewData, layerCount, moveCount, legend

SliceService::resultChanged
    -> syncPreviewWithActiveResult()  // same path

SliceService::sliceFailed
    -> resetPreviewState()
       -> gcodePreviewData_.clear()
       -> segments_.clear()
       -> emit stateChanged()

SliceService::sliceResultCleared
    -> resetPreviewState()

Plate switch to invalid
    -> PreviewViewModel sees empty outputPath
       -> resetPreviewState()
```

### Current Gap: Placeholder Access
The current code has **no** placeholder/sample/demo code path. The `rebuildFromGCode()` function only processes real G-code files. The GCODE-01 RED test should verify this by asserting:
1. After a live slice, `gcodePreviewData` starts with "GCV1" and has segment count > 0
2. `layerCount > 0` and `moveCount > 0`
3. At least one segment has `isTravel == false` and `feedrate > 0`
4. The payload is NOT from a hardcoded fixture file (check that the filePath is the SliceService output path)

### Current Gap: Fine-Grained Role Parsing
The current `styleFor()` function (`PreviewViewModel.cpp:114-126`) maps `;TYPE:` comments to only 5 categories. This must be replaced with the full 20-value `EGCodeExtrusionRole` mapping using the exact strings from `ExtrusionEntity::role_to_string()`, translated DIRECTLY to the canonical libvgcode index (see Pattern 2).

## Legend Coherence

**Confirmed:** Upstream legend is global-scope [VERIFIED: `third_party/OrcaSlicer/src/libvgcode/src/ViewerImpl.cpp` -- `update_enabled_entities()` operates on the full view range, not the filtered range]. Legend min/max are computed per `StoredSegment` field across ALL segments, not filtered-by-slider segments.

**Our implementation:** `buildLegendItems(int mode, float minV, float maxV)` at `PreviewViewModel.cpp:1360-` already computes min/max across all visible segments (before layer/move filtering). The `computePreviewDrawRange()` only affects what the renderer draws. Legend data is recomputed only in `recolorAndPackSegments()`, which is called on mode change and travel toggle -- NOT on slider drag. This is correct.

**Legend type mapping:**
- Mode 0 (FeatureType): discrete per-role legend items [VERIFIED]
- Modes 3, 7, 8 (Tool/ColorPrint/FilamentId): discrete per-extruder legend items [VERIFIED]
- Modes 1,2,4,5,6,9,10,11,12 (gradient modes): gradient legend with min/max [VERIFIED]
- Summary: no legend (statistics only) [VERIFIED: upstream Summary mode has no legend rendering]

## G-code Text / Current-Line Sync

**Path:** `rebuildGcodeLineWindow()` at `PreviewViewModel.cpp` -- builds `m_gcodeSourceLines` from G-code source, creates `gcodeLines` QVariantList for QML display.

**Current-line mapping:** `currentGcodeLine` is set via `updateToolPositionData()` when `currentMove` changes. The `SourceGcodeLine` struct maps `lineNumber` and `moveIndex` bidirectionally.

**Atomicity:** `setCurrentMove(int move)` calls `updateToolPositionData()` which updates `currentGcodeLine_`, `gcodeLines_`, and all tool position fields, then emits `stateChanged()`. This is atomic (single-threaded GUI).

**Missing:** The current-line highlight in the G-code text window should scroll to show the current line. This is a QML-side concern (ListView positioning) that Phase 54 may already handle.

## D3D11 / SoftwareViewport Status

**RhiViewport as default:** `main_qml.cpp` registers `RhiViewport` as `GLViewport` when QRhi initializes successfully (the normal path). SoftwareViewport is only registered as a fallback when `OWZX_OPENGL` environment variable is set or QRhi fails [VERIFIED: `QmlUiAuditTests.cpp:145-152`].

**PreviewPage.qml:** Confirmed to never reference `SoftwareViewport` [VERIFIED: grep of `PreviewPage.qml` shows only `GLViewport` (which resolves to RhiViewport)].

**Existing guards in QmlUiAuditTests.cpp:**
- `test_rhiDefaultRegistration()` -- asserts RhiViewport registered as GLViewport
- `test_previewPageNeverReferencesSoftwareViewport()` -- asserts no SoftwareViewport string in PreviewPage.qml (NOTE: this test may not exist yet -- needs verification. The existing tests check main_qml.cpp registration and PreparePage.qml bindings).

## Common Pitfalls

### Pitfall 1: GCV1 Wire Format Drift
**What goes wrong:** PackedSegment and GcvPackedSegment get out of sync (one has role field, the other does not). The renderer reads garbage for the role field.
**Why it happens:** Two separate anonymous namespace struct definitions in different files.
**How to avoid:** Both structs must be updated in the same commit. Add a compile-time `static_assert(sizeof(PackedSegment) == sizeof(GcvPackedSegment))` guard.
**Warning signs:** Preview renders but some segments are invisible or all visible regardless of toggle state.

### Pitfall 2: View-Mode Index Shift Breaks Existing Tests
**What goes wrong:** Changing the view-mode list from 13 to 17 entries shifts all index-based switch cases in `recolorAndPackSegments()`.
**Why it happens:** The mode-to-field mapping is hardcoded as `case 1: v = s.height; break;` etc.
**How to avoid:** Use an enum-based mapping instead of raw indices. At minimum, create a lookup table.
**Warning signs:** Switching to "Speed" mode shows layer-height coloring instead.

### Pitfall 3: Role Visibility Defaults Mismatch
**What goes wrong:** Our `showTravelMoves_` defaults to `true` but upstream defaults to `false` for travel visibility.
**Why it happens:** Historical artifact from when the feature was first implemented.
**How to avoid:** Set `showTravelMoves_ = false` in the constructor to match upstream.
**Warning signs:** Travel moves appear in the preview immediately after first slice.

### Pitfall 4: StoredSegment Missing New Fields
**What goes wrong:** Adding new view modes (Jerk, PressureAdvance, ActualSpeed) requires new fields on StoredSegment, but the G-code parser does not populate them.
**Why it happens:** OrcaSlicer G-code uses `;JERK:` and computed pressure advance values that our parser does not extract.
**How to avoid:** Parse `;JERK:` tagged comments and compute pressure advance from `;PA:` or feedrate/acceleration ratios. If upstream G-code does not contain the data, those modes fall back to a default value.
**Warning signs:** Jerk/PressureAdvance mode shows all segments in the same gradient color.

### Pitfall 5: Repacking on Travel Toggle
**What goes wrong:** The current `setShowTravelMoves()` calls `recolorAndPackSegments()` which rebuilds the entire GCV1 blob. This is correct for travel visibility (it changes which segments are included in the payload). But for role visibility, CONTEXT.md explicitly forbids repacking.
**Why it happens:** The existing travel toggle IS a repack operation (it removes travel segments from the buffer). Role visibility is different -- it keeps all segments but skips drawing some.
**How to avoid:** Travel/wipe visibility continues to repack. Role visibility uses the new render-side skip. These are two different mechanisms.

### Pitfall 6: libslic3r vs libvgcode Role Index Mismatch
**What goes wrong:** The parser maps `;TYPE:` strings to a libslic3r `ExtrusionRole` integer and that integer is then used to index `DEFAULT_EXTRUSION_ROLES_COLORS` / `m_roleVisibility[]` — but both arrays are indexed by the **libvgcode** `EGCodeExtrusionRole`. The two enums diverge past index 6 (see the cross-reference table in "Upstream Enum Reference" / Pattern 2). 10 of the 20 roles get the wrong color and the wrong visibility-slot, silently.
**Why it happens:** The same display strings exist in both enums, so a developer assumes the integers agree.
**How to avoid:** Use the libvgcode `EGCodeExtrusionRole` index as canonical everywhere in the Qt6 codebase. The `kRoleMap[]` parser table maps the display string DIRECTLY to the libvgcode index — it must not translate via the libslic3r integer. Add an acceptance test that asserts the string→color mapping for at least the divergent roles (e.g. "Bottom surface" → (102,92,199); "Ironing" → (255,140,105)).
**Warning signs:** Ironing renders in the purple reserved for Bottom surface; Bottom surface renders in the orange reserved for Ironing.

## Code Examples

### Fine-Grained ;TYPE: Parser (replaces current styleFor)
```cpp
// Source strings: third_party/OrcaSlicer/src/libslic3r/ExtrusionEntity.cpp:583-608 (role_to_string)
// Canonical target index: third_party/OrcaSlicer/src/libvgcode/include/Types.hpp:131-157
//   (EGCodeExtrusionRole -- DO NOT use the libslic3r ExtrusionRole integer here)
struct RoleEntry {
    const char *name;  // must match ExtrusionEntity::role_to_string() output exactly
    int role;          // CANONICAL libvgcode EGCodeExtrusionRole index
};
static const RoleEntry kRoleMap[] = {
    {"Inner wall",              1},  // Perimeter
    {"Outer wall",              2},  // ExternalPerimeter
    {"Overhang wall",           3},  // OverhangPerimeter
    {"Sparse infill",           4},  // InternalInfill
    {"Internal solid infill",   5},  // SolidInfill
    {"Top surface",             6},  // TopSolidInfill
    {"Ironing",                 7},  // Ironing             (libslic3r idx 8)
    {"Bridge",                  8},  // BridgeInfill        (libslic3r idx 9)
    {"Gap infill",              9},  // GapFill             (libslic3r idx 11)
    {"Skirt",                  10},  // Skirt               (libslic3r idx 12)
    {"Support",                11},  // SupportMaterial     (libslic3r idx 14)
    {"Support interface",      12},  // SupportMaterialInterface (libslic3r idx 15)
    {"Prime tower",            13},  // WipeTower           (libslic3r idx 17)
    {"Custom",                 14},  // Custom              (libslic3r idx 18)
    {"Bottom surface",         15},  // BottomSurface       (libslic3r idx 7)
    {"Internal Bridge",        16},  // InternalBridgeInfill(libslic3r idx 10)
    {"Brim",                   17},  // Brim                (libslic3r idx 13)
    {"Support transition",     18},  // SupportTransition   (libslic3r idx 16)
    {"Multiple",               19},  // Mixed
};

// Role default colors from upstream DEFAULT_EXTRUSION_ROLES_COLORS
// Source: third_party/OrcaSlicer/src/libvgcode/src/ViewerImpl.cpp:283-305
// Indexed by CANONICAL libvgcode EGCodeExtrusionRole (matches kRoleMap output).
static const float kRoleColors[][3] = {
    {230/255.f, 179/255.f, 179/255.f}, // 0  None
    {255/255.f, 230/255.f,  77/255.f}, // 1  Perimeter
    {255/255.f, 125/255.f,  56/255.f}, // 2  ExternalPerimeter
    { 31/255.f,  31/255.f, 255/255.f}, // 3  OverhangPerimeter
    {176/255.f,  48/255.f,  41/255.f}, // 4  InternalInfill
    {150/255.f,  84/255.f, 204/255.f}, // 5  SolidInfill
    {240/255.f,  64/255.f,  64/255.f}, // 6  TopSolidInfill
    {255/255.f, 140/255.f, 105/255.f}, // 7  Ironing
    { 77/255.f, 128/255.f, 186/255.f}, // 8  BridgeInfill
    {255/255.f, 255/255.f, 255/255.f}, // 9  GapFill
    {  0/255.f, 135/255.f, 110/255.f}, // 10 Skirt
    {  0/255.f, 255/255.f,   0/255.f}, // 11 SupportMaterial
    {  0/255.f, 128/255.f,   0/255.f}, // 12 SupportMaterialInterface
    {179/255.f, 227/255.f, 171/255.f}, // 13 WipeTower
    { 94/255.f, 209/255.f, 148/255.f}, // 14 Custom
    {102/255.f,  92/255.f, 199/255.f}, // 15 BottomSurface
    { 77/255.f, 128/255.f, 186/255.f}, // 16 InternalBridgeInfill
    {  0/255.f,  59/255.f, 110/255.f}, // 17 Brim
    {  0/255.f,  64/255.f,   0/255.f}, // 18 SupportTransition
    {128/255.f, 128/255.f, 128/255.f}, // 19 Mixed
};
```

### Extended PackedSegment with Role
```cpp
// PreviewViewModel.cpp (anonymous namespace)
struct PackedSegment
{
    float x1, y1, z1, x2, y2, z2;
    float r, g, b;
    float feedrate, fan_speed, temperature, width, layer_time, acceleration;
    int extruder_id, layer, move;
    int role;  // CANONICAL libvgcode EGCodeExtrusionRole index (0..19)
};

// RhiViewportRenderer.cpp (anonymous namespace)
struct GcvPackedSegment
{
    float x1, y1, z1, x2, y2, z2;
    float r, g, b;
    float feedrate, fan_speed, temperature, width, layer_time, acceleration;
    int extruder_id, layer, move;
    int role;  // must match PackedSegment layout exactly (canonical libvgcode index)
};
// Compile-time guard:
static_assert(sizeof(PackedSegment) == sizeof(GcvPackedSegment),
    "PackedSegment and GcvPackedSegment must have identical layout");
```

### Role Visibility Toggle (PreviewViewModel)
```cpp
// PreviewViewModel.h -- new properties
Q_PROPERTY(QVariantList roleVisibilities READ roleVisibilities NOTIFY stateChanged)
Q_INVOKABLE bool isRoleVisible(int roleIndex) const;
Q_INVOKABLE void toggleRoleVisibility(int roleIndex);

// PreviewViewModel.h -- new member
std::array<bool, 20> m_roleVisibility; // indexed by CANONICAL libvgcode EGCodeExtrusionRole

// Constructor: all extrusion roles visible by default (matching upstream)
PreviewViewModel::PreviewViewModel(SliceService *sliceService, QObject *parent)
    : QObject(parent), sliceService_(sliceService)
{
    m_roleVisibility.fill(true); // all extrusion roles visible
    m_roleVisibility[0] = true;  // None -- not shown in UI but kept for indexing
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| OrcaSlicer GCodeViewer (wxWidgets + ImGui) | libvgcode Viewer (standalone C++ lib) | OrcaSlicer v7.0.1 era | OrcaSlicer refactored G-code rendering into a reusable `libvgcode` library with QRhi-like abstraction |
| Coarse 5-category ;TYPE: mapping | 20-value EGCodeExtrusionRole fine-grained mapping | Phase 55 (this phase) | PreviewViewModel parser must be upgraded |
| 13 view modes | 17 view modes (upstream complete) | Phase 55 (this phase) | ViewMode index shift; new modes need StoredSegment fields |
| Travel toggle via repack | Travel toggle still repacks; role toggle is render-side skip | Phase 55 (this phase) | Two separate mechanisms for two different visibility types |

**Deprecated/outdated:**
- Current `styleFor()` 5-category mapping must be replaced entirely.
- Current view-mode index numbering (0-12) will change.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | OrcaSlicer-generated G-code always uses the English strings from `ExtrusionEntity::role_to_string()` in `;TYPE:` comments | Upstream Enum Reference | If localized strings appear, our parser fails to match. LOW risk -- OrcaSlicer G-code output is always English. |
| A2 | OrcaSlicer generates `;JERK:` tagged comments with jerk values in G-code output | View-mode list | If not present, Jerk and PressureAdvance modes will show default coloring. MEDIUM risk -- needs verification against a real OrcaSlicer-generated .gcode file. |
| A3 | The `;FEATURE:Type Height H Width W` format is also present in OrcaSlicer G-code (as a secondary metadata source) | Parser extension | If only `;TYPE:` is present, our parser only needs the ;TYPE: path. LOW risk. |
| A4 | Summary mode (index 0 upstream) requires no gradient legend and only shows statistics | View-mode list | If Summary has hidden legend behavior, our implementation may be incomplete. LOW risk -- upstream Summary mode code path is simple. |
| A5 | `showTravelMoves_` should default to `false` to match upstream | Common Pitfalls | If users expect travel visible by default, changing this breaks user expectations. LOW risk -- matches upstream behavior. |

## Open Questions (RESOLVED)

> All three questions below are formally resolved as of 2026-07-02. The resolutions
> drive Plan 02 (Task 1 + Task 2) and Plan 04. Each item marks the accepted resolution.

1. **Summary mode behavior** — (RESOLVED)
   - **Resolution:** Summary mode renders no gradient legend. Plan 02 sets `m_legendType = 0` (discrete) and clears `legendItems_` when `viewModeIndex_ == 0` (Summary). Statistics panel stays visible. Plan 04 adds the assertion `legendItems_.isEmpty() && legendType() == 0` for Summary mode.
   - Rationale: upstream `EViewType::Summary` renders statistics only, with no color-by-field visualization and no gradient legend. This matches the CONTEXT-locked "Legend scope under slider filtering is global" + "Summary is non-color mode" interpretation.

2. **Jerk and PressureAdvance data availability** — (RESOLVED, accepting Assumption A2)
   - **Resolution:** Assumption A2 is accepted with documented risk. The Phase 55 G-code fixture (committed in Plan 01) deliberately omits `;JERK:` / `;PA:` tags. Jerk (mode 6), Pressure Advance (mode 15), Actual Flow (mode 10), and Actual Speed (mode 4) view modes return a uniform gradient and emit a single one-time log line `[Preview] Jerk/PA/ActualFlow/ActualSpeed data unavailable in fixture-driven path`. No new StoredSegment fields are added in Phase 55.
   - Rationale: the v3.6 local-workflow scope (CONTEXT.md) does not require these data fields; the uniform-gradient fallback is a non-crashing, non-misleading behavior that defers cleanly to a future phase when the upstream tags are confirmed and parsed.

3. **ActualSpeed vs Speed data source** — (RESOLVED, deferred with accepted risk)
   - **Resolution:** ActualSpeed (mode 4) is deferred alongside Jerk/PA/ActualFlow. The data is technically derivable (`distance / (layer_time * 60)` per segment) but the fixture lacks the required layer-time-per-segment precision to validate it. Plan 02 treats ActualSpeed identically to Jerk/PA: uniform gradient + one-time log. A future phase will reintroduce ActualSpeed with per-segment time data once the GCodeProcessor time-tracking path is migrated.
   - Rationale: deferring avoids shipping an unvalidated per-segment time computation that could mislead users. Matches Assumption A2's accepted risk and the v3.6 local-workflow scope.

## Environment Availability

> Step 2.6: SKIPPED (no external dependencies identified -- all work is within the existing project codebase using Qt 6.10 which is already installed and configured).

## Validation Architecture

### Test Framework
| Property | Value |
|----------|-------|
| Framework | Qt Test (QtTest) + CTest |
| Config file | `CMakeLists.txt` (root, `include(CTest)` + `add_test`) |
| Quick run command | `ctest --output-on-failure -R QmlUiAudit` |
| Full suite command | `ctest --output-on-failure` |

### Phase Requirements -> Test Map
| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| GCODE-01 | After live slice, gcodePreviewData is non-empty GCV1 with real segments | unit/integration | `ctest --output-on-failure -R E2EWorkflow` | Yes -- extend `E2EWorkflowTests.cpp` |
| GCODE-01 | No placeholder/demo/sample path reachable from normal workflow | source-audit | grep-based: no "demo"/"sample"/"placeholder" strings in PreviewViewModel | Yes -- extend `QmlUiAuditTests.cpp` |
| GCODE-01 | sliceFailed/sliceResultCleared triggers resetPreviewState | unit | `ctest --output-on-failure -R E2EWorkflow` | Yes -- extend existing test |
| GCODE-02 | 20 EGCodeExtrusionRole values parsed from ;TYPE: comments | unit | `ctest --output-on-failure -R PreviewParser` | No -- Wave 0 gap |
| GCODE-02 | Role visibility toggle does not repack gcodePreviewData | unit | `ctest --output-on-failure -R ViewModelSmoke` | Yes -- extend `ViewModelSmokeTests.cpp` |
| GCODE-02 | Render-side filtering respects role visibility mask | source-audit | grep: `computePreviewDrawRange` contains role check | Yes -- extend `QmlUiAuditTests.cpp` |
| GCODE-03 | Legend min/max does not change on slider drag | unit | `ctest --output-on-failure -R ViewModelSmoke` | Yes -- extend `ViewModelSmokeTests.cpp` |
| GCODE-03 | G-code text window updates atomically with currentMove | unit | `ctest --output-on-failure -R ViewModelSmoke` | Yes -- extend `ViewModelSmokeTests.cpp` |
| GCODE-04 | PreviewPage.qml never references SoftwareViewport | source-audit | `ctest --output-on-failure -R QmlUiAudit` | Yes -- extend `QmlUiAuditTests.cpp` |
| GCODE-04 | main_qml.cpp registers RhiViewport as default GLViewport | source-audit | `ctest --output-on-failure -R QmlUiAudit` | Yes -- existing test |
| GCODE-05 | gcodePreviewData survives layer/move/camera/toggle interactions | unit | `ctest --output-on-failure -R E2EWorkflow` | Yes -- existing test |
| GCODE-05 | Reslice clears and rebuilds gcodePreviewData | unit | `ctest --output-on-failure -R E2EWorkflow` | Yes -- extend `E2EWorkflowTests.cpp` |
| GCODE-05 | Export leaves gcodePreviewData intact | unit | `ctest --output-on-failure -R E2EWorkflow` | Yes -- extend `E2EWorkflowTests.cpp` |
| GCODE-05 | Page switch (Prepare<->Preview) preserves gcodePreviewData | unit | `ctest --output-on-failure -R E2EWorkflow` | Yes -- extend `E2EWorkflowTests.cpp` |

### Sampling Rate
- **Per task commit:** `ctest --output-on-failure -R QmlUiAudit` (source-audit tests, <5s)
- **Per wave merge:** `ctest --output-on-failure` (full suite including E2E)
- **Phase gate:** Full suite green before `/gsd:verify-work`

### Wave 0 Gaps
- [ ] `tests/PreviewParserTests.cpp` -- NEW: unit tests for ;TYPE: string-to-role mapping (covers GCODE-02 parser correctness)
- [ ] `tests/ViewModelSmokeTests.cpp` -- extend: add role visibility toggle does-not-repack test, legend coherence test
- [ ] `tests/QmlUiAuditTests.cpp` -- extend: PreviewPage SoftwareViewport guard, computePreviewDrawRange role check assertion, PackedSegment/GcvPackedSegment sizeof guard
- [ ] `tests/E2EWorkflowTests.cpp` -- extend: reslice invalidation test, export-stability test, page-switch preservation test
- [ ] G-code fixture: commit a small realistic OrcaSlicer-generated .gcode file with multiple extrusion roles, travel moves, and tagged comments (;HEIGHT:, ;WIDTH:, ;JERK:)

## Security Domain

> No security enforcement needed for this phase. G-code preview rendering is a read-only visualization path with no network, authentication, or cryptography concerns. No user input is persisted or transmitted. All data is local G-code file parsing.

## Sources

### Primary (HIGH confidence)
- `third_party/OrcaSlicer/src/libvgcode/include/Types.hpp` - EViewType (17 values), EMoveType (12 values), EGCodeExtrusionRole (20 values), EOptionType (9 values), Settings visibility defaults
- `third_party/OrcaSlicer/src/libvgcode/src/Settings.hpp:14-72` - Default visibility arrays for extrusion_roles_visibility and options_visibility
- `third_party/OrcaSlicer/src/libvgcode/src/ViewerImpl.cpp:283-305` - DEFAULT_EXTRUSION_ROLES_COLORS and DEFAULT_OPTIONS_COLORS palettes
- `third_party/OrcaSlicer/src/libvgcode/src/ViewerImpl.cpp:1156-1184` - update_enabled_entities() render-side filtering logic
- `third_party/OrcaSlicer/src/libslic3r/ExtrusionEntity.hpp:20-43` - ExtrusionRole enum (libslic3r version -- DIFFERENT ordering from libvgcode past index 6)
- `third_party/OrcaSlicer/src/libslic3r/ExtrusionEntity.cpp:583-639` - role_to_string() and string_to_role() exact string mappings
- `third_party/OrcaSlicer/src/slic3r/GUI/GCodeViewer.cpp:66-103` - get_view_type_string() display name mapping
- `third_party/OrcaSlicer/src/slic3r/GUI/GCodeViewer.cpp:1070-1114` - update_by_mode() view_type_items ordering
- `third_party/OrcaSlicer/src/slic3r/GUI/GCodeViewer.cpp:277-323` - to_string(EMoveType) and to_string(EGCodeExtrusionRole)

### Secondary (MEDIUM confidence)
- `src/core/viewmodels/PreviewViewModel.h` - Current StoredSegment struct, viewmodel properties
- `src/core/viewmodels/PreviewViewModel.cpp:114-126` - Current styleFor() 5-category mapping (to be replaced)
- `src/core/viewmodels/PreviewViewModel.cpp:46-66` - Current PackedSegment struct (to be extended)
- `src/core/viewmodels/PreviewViewModel.cpp:1233-1358` - recolorAndPackSegments() implementation
- `src/qml_gui/Renderer/RhiViewportRenderer.h:105-110` - PreviewDrawSpan struct (to be extended)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:571-577` - GcvPackedSegment struct (to be extended)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:648-713` - computePreviewDrawRange() (to be extended)
- `tests/QmlUiAuditTests.cpp` - Existing source-audit tests
- `tests/E2EWorkflowTests.cpp:490-518` - Existing payload-survival tests

### Tertiary (LOW confidence)
- [ASSUMED] OrcaSlicer G-code includes `;JERK:` tagged comments (needs verification against real output)
- [ASSUMED] ActualSpeed can be computed from distance/time per segment without upstream GCodeProcessorResult

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All components are project-local or Qt framework, no external dependencies
- Architecture: HIGH - Upstream source code directly inspected and cross-referenced
- Pitfalls: HIGH - All patterns derived from actual code analysis with file:line citations
- View-mode list: HIGH - Directly extracted from upstream `update_by_mode()` and `get_view_type_string()`
- Extrusion role defaults: HIGH - Directly from upstream `Settings.hpp`
- Role enum cross-reference: HIGH - Both `ExtrusionEntity.hpp` (libslic3r) and `Types.hpp` (libvgcode) directly inspected 2026-07-02; divergence past index 6 confirmed against `role_to_string` and `DEFAULT_EXTRUSION_ROLES_COLORS`

**Research date:** 2026-07-02
**Valid until:** 30 days (upstream source is locked to v7.0.1, unlikely to change)
