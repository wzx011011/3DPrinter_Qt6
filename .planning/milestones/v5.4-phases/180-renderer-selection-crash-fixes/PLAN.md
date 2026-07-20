# Phase 180: Renderer/Selection Crash Fixes

**Status:** Planned
**Workstream:** CL
**Requirement:** CRASH-01
**Dependencies:** none (Wave A parallel)

## Goal

Port two upstream bb3 fixes to the Qt6 renderer/selection destructor chain. Both are confirmed applicable (Qt6 has equivalent code paths).

## Scope

### A1 — GLCanvas destruction use-after-free (upstream `5a53d2eb88`)

- **Upstream fix:** `src/slic3r/GUI/GLCanvas3D.cpp` + `Selection.cpp` added an `is_closing()` guard so Selection::clear and warning notification teardown do not call back into a half-destroyed Plater during canvas destruction.
- **Qt6 equivalent:** `src/qml_gui/Renderer/RhiViewport.{cpp,h}` destructor chain + `src/core/rendering/` Scene/Selection teardown. The Qt6 RHI viewport owns scene data that viewmodels observe — ensure destructor ordering does not leave dangling observers.
- **Approach:** Investigate Qt6 destructor ordering; add an equivalent "closing" guard so viewmodel signal slots do not fire into a half-destroyed RhiViewport. Pattern likely: set a `m_isClosing` flag at the start of `~RhiViewport`, check it in slots that touch scene data.

### A2 — Prime tower rotation crash via wipe-tower synthetic id (upstream `d24e7f75ef`)

- **Upstream fix:** `src/slic3r/GUI/Selection.cpp` — rotate/scale/mirror on the wipe tower crashed because the wipe-tower synthetic id (≥1000) was used as a ModelObject index. The fix skips synthetic ids in the transform loop.
- **Qt6 equivalent:** `src/core/viewmodels/EditorViewModel.cpp` + `src/core/rendering/ObjectPicking.{cpp,h}`. The Qt6 transform code path (rotate/scale/mirror on selection) must skip synthetic wipe-tower ids the same way.
- **Approach:** Audit EditorViewModel transform methods (`applyRotate`/`applyScale`/`applyMirror` and helpers) for any index-based access that would mishandle id ≥1000. Add an equivalent guard. Map to upstream `Selection::rotate` skip pattern.

## Out of Scope

- A3/A5/A6/A10 — those go to Phase 182 (research only).
- Any change to libslic3r (already at bb3).
- Any new product behavior.

## Verification

- QmlUiAuditTests: add CRASH-01 anchor in Phase 187 (REGRESS-08).
- Manual test: open project with prime tower, select it, rotate/scale/mirror — no crash.
- Manual test: close OWzxSlicer while a slice is in progress — no use-after-free crash on exit.
- Canonical build exits 0, 0 errors.

## Risk Notes

- The Qt6 RhiViewport destructor chain is not a 1:1 port of upstream GLCanvas3D — the `is_closing()` guard pattern translates, but the exact "what calls back into what" must be re-derived from Qt6 code. Estimated 200-400 lines including investigation.
- A2's Qt6 equivalent depends on how EditorViewModel models the wipe tower (as a ModelObject with synthetic id, or as a separate concept). If Qt6 doesn't use synthetic ids at all, A2 may collapse to "not applicable" — surface this finding early.
