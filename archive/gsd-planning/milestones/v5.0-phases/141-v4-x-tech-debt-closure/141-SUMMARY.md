# Phase 141: v4.x Tech-Debt Closure — Summary

**Phase:** 141 (v5.0 / WS1)
**Status:** Verified ✓
**Requirements:** DEBT-01, DEBT-02, DEBT-03, DEBT-04, DEBT-05

## What shipped

All 5 DEBT-* requirements closed in one phase. The fixes are small, code-only, and well-localized.

### DEBT-01 — CGAL-02 true intersection
`src/core/services/ProjectServiceMock.cpp` meshBoolean op==2 branch:
- Swapped `Slic3r::MeshBoolean::minus(srcMesh, toolMesh)` → `Slic3r::MeshBoolean::cgal::intersect(srcMesh, toolMesh)`.
  - **Key discovery:** the upstream `intersect` API lives in `namespace cgal` (i.e. `Slic3r::MeshBoolean::cgal::intersect`, `MeshBoolean.hpp:54`). The original v5.0 exploration note missed the nested namespace. First build failed with C2039; fixed by qualifying with `cgal::`.
- Fixed the tool-deletion bug: `model_->objects.erase(...)` is now gated on `operation != 2` (intersection keeps the tool; only union/difference delete it). Previously the tool was deleted for all op branches, which is semantically wrong for intersection.

### DEBT-02 — Orphaned menu + stub removed
- `src/qml_gui/pages/PreparePage.qml`: removed the orphaned "网格布尔运算" CxMenuItem. Left a documentation comment so the deletion is grep-traceable.
- `src/core/viewmodels/EditorViewModel.cpp`: removed `meshBooleanSelected()` stub body.
- `src/core/viewmodels/EditorViewModel.h`: removed the Q_INVOKABLE declaration.
- The working boolean path (`booleanExecute` → `ProjectServiceMock::meshBoolean`) is unchanged.

### DEBT-03 — drillObject C4715 fix
`src/core/services/ProjectServiceMock.cpp`: added `return true;` at the end of the `try{}` success path in `drillObject` (after `srcVol->set_new_unique_id();`). Previously fell off the end → MSVC C4715.

### DEBT-04 — ASM rotate/scale live-visual compose
The largest item. Four-part plumbing extension, all parallel to the existing translate-only path:
1. `EditorViewModel.h/cpp`: added `Q_PROPERTY assembleRotations` + `assembleScales` + getters, reading `projectService_->assembleRotation/assembleScale` (which already exist from v4.8 Phase 138).
2. `RhiViewport.h/cpp`: added parallel `Q_PROPERTY` + members + setters (same invalidation pattern as `setAssembleOffsets`).
3. `RhiViewportRenderer.h/cpp`: added `m_assembleRotations` / `m_assembleScales` mirror + `QHash<int, QMatrix4x4> m_assembleTransformBySource`. In `synchronize()` builds the full compose matrix `translate * rotateZ * rotateY * rotateX * scale` (matches gizmo Euler XYZ convention). In `buildModelVertices` prefers the full matrix when present, falls back to the legacy offset map otherwise (preserves the v4.8 anchor on `m_assembleOffsetBySource`).
4. `AssemblePage.qml`: binds `assembleRotations` + `assembleScales` to the viewport.

### DEBT-05 — Regression lock
`tests/QmlUiAuditTests.cpp`:
- New `v50TechDebtRegressionLocked()` source-audit slot (declared + body).
- Asserts all DEBT-01..04 anchors AND re-asserts v4.8 anchors (kCgalMeshBooleanAvailable, MeshBoolean::minus still wired for difference/drill, setAssembleOffset still present).
- Anchor choices learned: assert on the callable residue (`meshBooleanSelected()` QML invocation + `Q_INVOKABLE bool meshBooleanSelected` declaration), not on documentation-comment text. The original anchor `!contains("网格布尔运算")` failed because the deletion-record comment mentioned the menu label.

## Verification

- **Build**: OWzxSlicer.exe links clean at 259/259 (verified across two full canonical runs; each killed by the harness 10-min limit at the test-compilation phase, but the link itself was green — no FAILED, no LNK errors). Direct incremental `ninja QmlUiAuditTests` rebuild from `build.ninja` succeeds in 4 steps.
- **Tests** (all green):
  - PrepareSceneDataTests: 12/12 PASS
  - PartPlateTests: 55/55 PASS
  - ViewModelSmokeTests: 102/102 PASS
  - QmlUiAuditTests: 100/100 PASS (incl. new `v50TechDebtRegressionLocked` slot + all v4.6/v4.7/v4.8 anchors)

## Lessons

1. **Anchor on callables, not comments.** Source-audit slots that assert `!contains("X")` will false-positive on documentation comments that mention X. Anchor on the QML invocation `foo()` or the C++ declaration `Q_INVOKABLE bool foo` instead.
2. **Namespace-qualified API lookups need namespace verification.** The v5.0 exploration correctly identified `intersect` exists at `MeshBoolean.hpp:54` but missed the wrapping `namespace cgal {`. Build early to catch these.
3. **Background task budget.** The canonical verify script does a CMake reconfigure + full libslic3r rebuild (~15 min), exceeding the 10-min harness limit. For verification of small targeted changes, `ninja <target>` from the existing `build.ninja` is much faster and gives the same answer for the specific target.
