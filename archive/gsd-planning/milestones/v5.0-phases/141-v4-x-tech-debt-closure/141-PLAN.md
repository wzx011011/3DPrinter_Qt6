# Phase 141: v4.x Tech-Debt Closure — Plan

**Phase:** 141
**Workstream:** WS1 (Tech-Debt)
**Requirements:** DEBT-01, DEBT-02, DEBT-03, DEBT-04, DEBT-05
**Mode:** Auto (skip_discuss=true) — executed inline given small, well-scoped fixes

## Goal

Close the four code-only tech-debt items carried out of v4.8 and seed the v5.0 regression lock.

## Plans

### 141-01: CGAL intersection + drillObject return + menu removal (3 small fixes)

**DEBT-01** — `src/core/services/ProjectServiceMock.cpp` meshBoolean op==2 branch:
- Swap `Slic3r::MeshBoolean::minus(srcMesh, toolMesh)` → `Slic3r::MeshBoolean::cgal::intersect(srcMesh, toolMesh)`
  (upstream API at `MeshBoolean.hpp:54` is inside `namespace cgal` — the original exploration note missed the nested namespace; corrected after first build failure).
- Fix tool-deletion bug: gate `model_->objects.erase(...)` on `operation != 2` (intersection keeps the tool; only union/difference delete it).

**DEBT-02** — Remove orphaned "网格布尔运算" CxMenuItem:
- `src/qml_gui/pages/PreparePage.qml`: delete the CxMenuItem block.
- `src/core/viewmodels/EditorViewModel.cpp`: delete `meshBooleanSelected()` stub body.
- `src/core/viewmodels/EditorViewModel.h`: delete the Q_INVOKABLE declaration.
- Leave an explanatory comment block in PreparePage.qml so the deletion is grep-traceable.

**DEBT-03** — `ProjectServiceMock::drillObject` C4715:
- Add `return true;` after `srcVol->set_new_unique_id();` at the end of the `try{}` success path (was falling off the end → MSVC C4715).

### 141-02: ASM rotate/scale live-visual compose (DEBT-04)

The translate-only assemble-offset rendering was the largest item. Four-part plumbing extension, all parallel to the existing translate-only path:

1. `EditorViewModel.h/cpp`: add `Q_PROPERTY assembleRotations` + `assembleScales` + getters, reading `projectService_->assembleRotation/assembleScale` (already exist from v4.8 Phase 138).
2. `RhiViewport.h/cpp`: add parallel `Q_PROPERTY` + `m_assembleRotations` / `m_assembleScales` members + setters (same invalidation pattern as `setAssembleOffsets`: bump `m_sceneGeneration` + `m_modelGeneration` + `update()`).
3. `RhiViewportRenderer.h/cpp`: add `m_assembleRotations` / `m_assembleScales` mirror + `QHash<int, QMatrix4x4> m_assembleTransformBySource`. In `synchronize()` build the full compose matrix `translate * rotateZ * rotateY * rotateX * scale` (matches the gizmo Euler XYZ convention). In `buildModelVertices` prefer the full matrix when present, fall back to legacy offset map otherwise (preserves the regression-slot anchor on `m_assembleOffsetBySource`).
4. `AssemblePage.qml`: bind `assembleRotations` + `assembleScales` to the viewport (parallel to the existing `assembleOffsets` binding).

### 141-03: Regression lock (DEBT-05)

`tests/QmlUiAuditTests.cpp`:
- Declare `void v50TechDebtRegressionLocked();` in the `private slots:` block.
- Append test body asserting all DEBT-01..04 anchors AND re-asserting the v4.8 anchors (kCgalMeshBooleanAvailable, MeshBoolean::minus still wired for difference/drill, setAssembleOffset still present).

## Verification

Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exit 0 + 5/5 ctest groups PASS (including the new `v50TechDebtRegressionLocked` slot).

## Key discovery during execution

The v5.0 exploration note said `Slic3r::MeshBoolean::intersect` exists. It does — but inside `namespace cgal` (`MeshBoolean.hpp:54`). The first build failed with `error C2039: "intersect": 不是 "Slic3r::MeshBoolean" 的成员`; fixed by qualifying as `Slic3r::MeshBoolean::cgal::intersect`. REQUIREMENTS.md VDB-01 says the Qt6 fork renamed the OpenVDB gating target similarly — worth remembering for Phase 142.
