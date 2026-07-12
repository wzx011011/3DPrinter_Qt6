---
phase: 101
phase_name: Wipe-Tower Real Rendering Upgrade
milestone: v4.4
base_commit: 36851f0
head_commit: f089ba6
status: clean
files_reviewed:
  - src/core/rendering/GizmoGeometry.cpp
  - src/qml_gui/Renderer/SoftwareViewport.cpp
  - tests/ViewModelSmokeTests.cpp
counts:
  critical: 0
  warning: 0
  info: 4
  total: 4
ctest: 4/4 passed (PrepareSceneDataTests, PartPlateTests, ViewModelSmokeTests 97 slots incl. new test, QmlUiAuditTests)
build: OWzxSlicer.exe compiled/linked clean
rhi_pipeline: unchanged (RhiViewportRenderer correct end-to-end per Phase 100)
---

# Phase 101 Code Review — Wipe-Tower Real Rendering Upgrade

Review scope: the git diff `36851f0..HEAD` across the 3 listed source files
(+324/-0 lines, 4 commits). Focus is on the CHANGES, not pre-existing code.

## Findings Summary

| # | Severity | File | Location | Summary |
|---|----------|------|----------|---------|
| 1 | info | GizmoGeometry.cpp | comment block L449-485 | Stale self-citation: comment references `GizmoGeometry.cpp:462-463` / `:462` for `kGroundY`/`kColor`, but the 36-line comment insertion shifted those to L498-499. |
| 2 | info (fixed) | SoftwareViewport.cpp | L450-451, L458 | Same stale `GizmoGeometry.cpp:462-463` / `:462` citation carried into the SoftwareViewport comment block. **Fixed post-review: switched to symbol-only references (`GizmoGeometry kGroundY`) to avoid future drift.** |
| 3 | info | tests/ViewModelSmokeTests.cpp | L4075-4098 (assertions) | Substring `qml.contains(...)` assertions are whitespace-sensitive; a QML reformatter could break the test without changing behavior. |
| 4 | info | tests/ViewModelSmokeTests.cpp | L4063 comment | Comment cites PreparePage.qml `~:1648` for the GLViewport instance; actual is L1649 (`GLViewport {`) / L1650 (`id: viewport3d`). Covered by `~` qualifier. |

No critical or warning findings. All four are documentation/robustness nits
that do not affect correctness, runtime behavior, or test determinism.

---

## Detail: GizmoGeometry.cpp (comment-only, body byte-for-byte unchanged)

### Verification of "comment-only" claim
Confirmed. `git show 36851f0:src/core/rendering/GizmoGeometry.cpp` (base
commit) shows the `buildWipeTowerVertices` function body identical to HEAD.
The diff is +36 lines, all inside the `// ===` header comment block above the
function (L449-485 at HEAD). The function signature, the `kGroundY`/
`kColor` constants, the `hw`/`hd`/`xMin`/`xMax`/`y0`/`y1`/`zMin`/`zMax`
derivation, the 36-vertex `positions` table, and the
`appendColoredVert` loop are unchanged.

### Verification of upstream anchors (not invented line numbers)
All anchors in the comment were checked against
`third_party/OrcaSlicer/src/slic3r/GUI/3DScene.cpp` and
`third_party/OrcaSlicer/src/libslic3r/Print.hpp`:

| Cited anchor | Claim | Verified |
|---|---|---|
| `3DScene.cpp:840` | `load_wipe_tower_preview` definition | ✓ L840 |
| `3DScene.cpp:855` | `make_cube(width, depth, height)` | ✓ L855 |
| `3DScene.cpp:851-873` | per-extruder colored slices | ✓ |
| `3DScene.cpp:882` | `v.is_wipe_tower = true` | ✓ L882 |
| `3DScene.cpp:887` | `load_real_wipe_tower_preview` definition | ✓ L887 |
| `3DScene.cpp:914` | `mesh.convex_hull_3d()` | ✓ L914 |
| `Print.hpp:745-746` | `real_wipe_tower_mesh` + `real_brim_mesh` | ✓ |
| `Print.hpp:766` | `std::optional<WipeTowerMeshData> wipe_tower_mesh_data` | ✓ |
| `Print.hpp:776` | `clear()` resets `wipe_tower_mesh_data = std::nullopt` | ✓ |

---

## Detail: SoftwareViewport.cpp (QPainter wipe-tower box, +52 lines)

### Geometry match vs. RHI box (buildWipeTowerVertices)

| Quantity | buildWipeTowerVertices (RHI) | SoftwareViewport QPainter | Match |
|---|---|---|---|
| `kGroundY` | `-0.04f` | `constexpr float kGroundY = -0.04f;` | ✓ |
| color | `{0.35f, 0.60f, 0.85f, 0.50f}` | `QColor::fromRgbF(0.35f, 0.60f, 0.85f, 0.50f)` | ✓ |
| `hw`/`hd` | `width*0.5f` / `depth*0.5f` | `m_wipeTowerWidth*0.5f` / `m_wipeTowerDepth*0.5f` | ✓ |
| x extent | `x-hw .. x+hw` | `m_wipeTowerX-hw .. m_wipeTowerX+hw` | ✓ |
| z extent | `z-hd .. z+hd` | `m_wipeTowerZ-hd .. m_wipeTowerZ+hd` | ✓ |
| y extent | `kGroundY .. kGroundY+height` | `kGroundY .. kGroundY+m_wipeTowerHeight` | ✓ |
| center convention | x/z = center (Phase 100 W1) | x/z = center (documented) | ✓ |

The box geometry is a faithful mirror of the RHI path.

### m_showWipeTower gate correctness (WTREAD-02, no single-material leak)
The gate is:
```cpp
if (m_showWipeTower && m_wipeTowerWidth > 0.f &&
    m_wipeTowerDepth > 0.f && m_wipeTowerHeight > 0.f)
```
- `SoftwareViewport.h:238` declares `bool m_showWipeTower = false;` — the
  default is false, so a null-editorVm / pre-slice / single-material state
  does not render a placeholder box.
- The `> 0.f` guards are defensive: even with zeroed dims, no degenerate box
  is drawn. Stricter than the RHI path (which relies on
  `buildWipeTowerVertices`'s internal early-return).
- No leak path identified.

### Transform reuse (no invented transform)
The wipe-tower faces use the SAME world→screen transform as the existing
bed-grid and model-mesh paint (`project()` + `rotatePoint(p - m_center)`).
Faces append to the same depth-sorted `QVector<Face>` vector and are sorted
together. No new matrix, scale, or projection introduced.

### Face windings
The 5 visible faces (top + 4 sides; bottom omitted as coplanar with the bed
grid) are emitted as quads. SoftwareViewport painter does NOT use back-face
culling (renders all appended faces after depth-sort), so winding order
doesn't affect visibility. Depth-sort is the only visibility mechanism,
applied uniformly. Correct face set: top + 4 sides = 5 faces, bottom
correctly omitted. The wipe-tower faces intentionally do NOT apply the
model-mesh lambertian shading (the RHI box uses flat `kColor`) — correct
mirror.

---

## Detail: tests/ViewModelSmokeTests.cpp (new test, +68 lines)

### Source-audit fallback robustness
NOT a weak grep. Full file read via `QFile` + `QT_TESTCASE_SOURCEDIR`
(=`CMAKE_SOURCE_DIR` at CMakeLists.txt:400):
- **Deterministic**: resolves to source-tree PreparePage.qml regardless of
  working dir or build dir. No QML engine, no QRhi context.
- **Build-dir independent**: `CMAKE_SOURCE_DIR` is repo root.
- **Failure-mode explicit**: `QFileInfo::exists` + `qmlFile.open` both have
  `QVERIFY2` with descriptive messages.

### WTREAD-02 null-editorVm default lock
The 7th assertion locks that `showWipeTower` defaults to `false` when
`editorVm` is null. Combined with the `m_showWipeTower = false` defaults in
SoftwareViewport.h:238 and RhiViewport.h:304, this is a meaningful (not
vacuous) contract lock — if a future refactor changed `: false` to `: true`,
this test would fail.

### Determinism / no QML-engine-state reliance
The test reads a file from disk and performs pure string assertions. There
is no `QQmlEngine`, no signal spy, no event loop. Fully deterministic,
cannot flake.

---

## Cross-Cutting Verification

### Regression coverage
The new test brings ViewModelSmokeTests to 97 slots. Combined with the
Phase 100 readback test, the end-to-end dim-reach contract (SliceService →
EditorViewModel → PreparePage.qml → renderer) is locked at both ends. The
RHI render path (RhiViewportRenderer::uploadWipeTowerBuffer) was unchanged
by Phase 101 and remains correct end-to-end per Phase 100.

## Conclusion

**status: clean.** The Phase 101 changes are correct, faithful to the
upstream anchors, conventionally adherent, and regression-tested. The 4
info-level findings are documentation/robustness nits. Finding 2 (stale
SoftwareViewport line citations) was fixed post-review by switching to
symbol-only references. Findings 1, 3, 4 remain as documented minor nits
(non-blocking). No fixes required for v4.4 ship.

Full report: `.planning/phases/101-wipe-tower-real-rendering-upgrade/101-REVIEW.md`
