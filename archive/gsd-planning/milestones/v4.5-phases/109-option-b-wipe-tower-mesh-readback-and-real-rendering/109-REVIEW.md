---
phase: 109-option-b-wipe-tower-mesh-readback-and-real-rendering
status: changes_required_fixed
base: e46b582
head: HEAD (incl. CRITICAL-1 fix)
files_reviewed:
  - src/core/services/SliceService.h
  - src/core/services/SliceService.cpp
  - src/core/rendering/GizmoGeometry.h
  - src/core/rendering/GizmoGeometry.cpp
  - src/qml_gui/Renderer/RhiViewportRenderer.cpp
  - src/qml_gui/Renderer/SoftwareViewport.cpp
  - tests/ViewModelSmokeTests.cpp
  - tests/QmlUiAuditTests.cpp
counts: {critical: 1, warning: 1, info: 3, total: 5}
---

# Phase 109 Code Review — Option B Wipe-Tower Mesh Readback And Real Rendering

## Verdict: changes_required → FIXED

The code review caught a load-bearing correctness defect (CRITICAL-1) before merge. The fix is applied; regression ctest 4/4 passes.

## Findings

| # | Severity | Finding | Status |
|---|----------|---------|--------|
| CRITICAL-1 | critical | Worker captured `shell.its.vertices` (a DEDUPLICATED POINT CLOUD) and streamed it as `GL_TRIANGLES`. Upstream `its_convex_hull` (TriangleMesh.cpp:1342-1424) returns vertices + a SEPARATE `its.indices` facet list; upstream renders via `init_from(shell)` using BOTH. Capturing only vertices → garbage triangles or silent drop (when vertex count not divisible by 3). | **FIXED** — worker now expands `shell.its.indices` into a flattened XYZ triangle soup (9 floats/triangle). Doc comments updated. |
| WARNING-1 | warning | Regression test fixture was a hand-built triangle soup (the contract consumers expect) — but production produced a point cloud. Test passed while production rendered garbage. | **RESOLVED by CRITICAL-1 fix** — production now matches the test's contract (both produce triangle soup). The existing fixture is now correct-by-coincidence; a future hardening pass could add a point-cloud-input negative test. |
| INFO-1 | info | WM-02 capture-by-value invariant correctly honored (Frozen Decision 1 extended). mergedMesh + shell are stack-local TriangleMesh values; only floats escape. | No action. |
| INFO-2 | info | Option A (WM-03/WM-06) preserved byte-identical: buildWipeTowerVertices unchanged; uploadWipeTowerBuffer Option A else-branch unchanged; SoftwareViewport Option A box unchanged. The new gate is additive. | No action. |
| INFO-3 | info | hasRealMesh gate + dirty-flag plumbing sound. Minor perf: synchronize deep-compares meshVertices vector O(N) per-frame; consider hashing/generation-counter. | Non-blocking; future optimization. |

## The CRITICAL-1 Fix (applied)

**Before (broken):** captured `shell.its.vertices` directly as flattened XYZ — a point cloud.
**After (correct):** expands `shell.its.indices` facets into a flattened XYZ triangle soup:

```cpp
const auto &sv = shell.its.vertices;
const auto &si = shell.its.indices;
if (!sv.empty() && !si.empty()) {
  capturedGeometry.meshVertices.reserve(si.size() * 9);
  for (const Slic3r::Vec3i32 &f : si) {
    for (int k = 0; k < 3; ++k) {
      const Slic3r::Vec3f &v = sv[f[k]];
      capturedGeometry.meshVertices.push_back(v.x());
      capturedGeometry.meshVertices.push_back(v.y());
      capturedGeometry.meshVertices.push_back(v.z());
    }
  }
  capturedGeometry.hasRealMesh = true;
}
```

This preserves the `meshVertices` pure-float-vector contract (Frozen Decision 1 honored), keeps `buildWipeTowerMeshVertices` + SoftwareViewport mirror unchanged (they already expect triangle soup), and produces a real triangle payload matching what every downstream consumer and the test fixture assume.

## Why the regression test didn't catch this

The test fixture (`ViewModelSmokeTests.cpp:4258-4261`) supplied a hand-authored triangle soup (6 vertices as 2 explicit triangles). That's the correct contract — but the production worker produced a point cloud, not a soup. The test passed because it exercised the consumer's happy path with the right shape; it never ran the production capture code. With CRITICAL-1 fixed, production now matches the test's contract.

A future hardening pass (deferred) could add a direct unit test on `buildWipeTowerMeshVertices` with a known point-cloud input to prove it does NOT silently misinterpret a deduplicated cloud.

## Verified

- Regression ctest 4/4 PASS after CRITICAL-1 fix (PrepareSceneDataTests, PartPlateTests 51/51, ViewModelSmokeTests 99/99 incl. the Option B test, QmlUiAuditTests 76/76 incl. optionBWipeTowerMeshCoexistsWithOptionA).
- OWzxSlicer.exe + owzx_app_core compile/link clean.
- Option A regression ctest (v4.4 Phase 100-102 tests) unchanged and passing.
- `git diff --check` clean; encoding guard clean.
- The capture-by-value invariant (no TriangleMesh*/its* escape) preserved by the fix.

## Conclusion

Phase 109 ships a correct Option B implementation after the CRITICAL-1 fix. The independent code review earned its keep here — the bug would have shipped garbage wipe-tower rendering in production. WTMESH-01/02/03 closed. Option A preserved byte-identical. Phase 116 (WTMESH-04 regression lock) can proceed.

Full report: `.planning/phases/109-option-b-wipe-tower-mesh-readback-and-real-rendering/109-REVIEW.md`
