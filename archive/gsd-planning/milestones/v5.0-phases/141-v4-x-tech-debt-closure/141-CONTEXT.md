# Phase 141: v4.x Tech-Debt Closure - Context

**Gathered:** 2026-07-17
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Close the four small code-only items carried out of v4.8 (CGAL-02 true intersection, orphaned `meshBooleanSelected` menu stub, `drillObject` C4715, ASM rotate/scale live-visual compose) and seed the v5.0 regression lock that later workstreams will append to.

Requirements: DEBT-01, DEBT-02, DEBT-03, DEBT-04, DEBT-05

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — discuss phase was skipped per user setting. Use ROADMAP phase goal, success criteria, and codebase conventions to guide decisions.

Source-truth anchors (from v5.0 exploration):
- DEBT-01: `ProjectServiceMock.cpp:3152-3156` swap `MeshBoolean::minus` → `MeshBoolean::intersect` (upstream API exists at `MeshBoolean.hpp:54,58,62` + `_cgal_intersection` at `.cpp:268-272`); ALSO fix the bug where tool object is deleted after intersection (`model_->objects.erase` at `:3176` — wrong for intersection; only delete tool for difference op).
- DEBT-02: `PreparePage.qml:401-406` CxMenuItem "网格布尔运算" that calls `EditorViewModel::meshBooleanSelected()` stub at `EditorViewModel.cpp:4538-4543`. Either remove the menu item OR repoint to working `booleanExecute` path. Recommend removal (the working boolean path is via boolean dialog at `PreparePage.qml:3170` CxComboBox + `:3176` `booleanExecute()`).
- DEBT-03: `ProjectServiceMock.cpp:3242-3362` `drillObject` — add `return true;` after `srcVol->set_new_unique_id();` at line 3346 (end of success path inside try). Currently the try block has no return; falls off end → C4715.
- DEBT-04: `RhiViewportRenderer.cpp:1615-1743` `buildModelVertices` — the assemble-offset block at lines 1718-1740 is translate-only (`v.x += off.x()`). Compose the full transform: read `ModelInstance::m_assemble_transformation` rotation + scale, build a QMatrix4x4 or apply rotate+scale to the vertex before adding the translate offset. The offset is currently plumbed as `QVector3D` via `m_assembleOffsetBySource` (`RhiViewportRenderer.cpp:67-86`, populated from `RhiViewport::m_assembleOffsets` at `RhiViewport.h:51,215,434`). Either widen the plumbing to forward rotation+scale, or read the assemble transform directly from ProjectServiceMock at render-sync time.
- DEBT-05: New `v50TechDebtRegressionLocked` source-audit slot in `tests/` (likely `QmlUiAuditTests.cpp`) asserting DEBT-01..04 anchors AND re-asserting v4.8/v4.7/v4.6 anchors (mirror the pattern from `v48CrossWorkstreamRegressionLocked`).

</decisions>

<code_context>
## Existing Code Insights

Codebase context will be gathered during plan-phase research. Key files:
- `src/core/services/ProjectServiceMock.cpp` (meshBoolean op==2 + drillObject)
- `src/core/viewmodels/EditorViewModel.cpp` (meshBooleanSelected stub)
- `src/qml_gui/pages/PreparePage.qml` (orphaned menu)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp` (buildModelVertices assemble offset)
- `tests/QmlUiAuditTests.cpp` (regression slot pattern)
- `third_party/OrcaSlicer/src/libslic3r/MeshBoolean.cpp` (`intersect` API reference)

Build verification: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` in `build/` only.

</code_context>

<specifics>
## Specific Ideas

No specific requirements — discuss phase skipped. Refer to ROADMAP phase description and success criteria.

</specifics>

<deferred>
## Deferred Ideas

None — discuss phase skipped.

</deferred>
