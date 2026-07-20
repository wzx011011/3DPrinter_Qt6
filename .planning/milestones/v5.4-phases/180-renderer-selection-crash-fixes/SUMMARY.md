# Phase 180: Renderer/Selection Crash Fixes — RESEARCH SUMMARY

**Status:** Researched — both items NOT APPLICABLE, no code changes
**Workstream:** CL
**Requirement:** CRASH-01
**Researched:** 2026-07-20

## Outcome

**Both A1 and A2 are NOT APPLICABLE to Qt6 architecture. Zero code changes.**

This is not a "we couldn't find the bug" — it's a "the bug cannot exist in Qt6 because the architectural preconditions are absent" verdict, backed by code evidence.

## A1 — GLCanvas destruction use-after-free (upstream `5a53d2eb88`)

**Verdict: NOT APPLICABLE.**

### Why Qt6 is immune

The upstream crash root cause is wxWidgets destructor ordering: `Plater::p` (pImpl) is freed BEFORE the child `GLCanvas3D` is destroyed, so `GLCanvas3D::~GLCanvas3D() → reset_volumes() → Selection::clear()` dereferences an already-freed pImpl.

Qt6 architecture has none of these elements:
- **No Plater, no pImpl**: RhiViewport directly inherits QQuickRhiItem. No "owner dies before owned" trap.
- **No Selection::clear()**: Qt6 has no upstream Selection class at all.
- **No explicit RhiViewport destructor**: RhiViewport's destruction is compiler-synthesized; members are non-QObject helpers (CameraController, PrepareSceneData) that emit no signals.
- **QPointer guard already in place**: RhiViewportRenderer (render thread) holds `QPointer<RhiViewport>` (`RhiViewportRenderer.h:379`) and null-checks before every cross-thread callback (`:534`, `:826`). Item death auto-no-ops.
- **Qt::QueuedConnection**: cross-thread `invokeMethod` is safely discarded if target is invalid.

### Evidence files

- `src/qml_gui/Renderer/RhiViewport.{h,cpp}` — no `~RhiViewport` declared; no pImpl
- `src/qml_gui/Renderer/RhiViewportRenderer.{h,cpp}:16-19, 534, 820-849` — QPointer + null-check pattern
- `src/qml_gui/Renderer/PrepareSceneData.h` — pure data class, no Q_OBJECT/emit

## A2 — Prime tower rotation crash via wipe-tower synthetic id (upstream `d24e7f75ef`)

**Verdict: NOT APPLICABLE.**

### Why Qt6 is immune

The upstream crash requires two preconditions, both absent in Qt6:
1. `GLVolume::object_idx()` returns a synthetic id (≥1000 for wipe tower) — Qt6 has no `object_idx()` concept.
2. `Selection::m_list` stores `GLVolume*` including wipe tower volumes — Qt6 wipe tower is NEVER in any selection set.

Qt6 wipe tower architecture:
- Wipe tower is a set of independent Q_PROPERTY on EditorViewModel (`EditorViewModel.h:830-846, 921-932, 1369-1387`: `showWipeTower`, `wipeTowerWidth/Depth/Height/X/Z`, `wipeTowerMeshVertices`, etc.)
- Populated single-direction from SliceService post-slice (`EditorViewModel.cpp:3336-3342, 6402-6432`)
- Rendered via independent GPU buffer (`RhiViewportRenderer.cpp:441 renderWipeTower, 1184 uploadWipeTowerBuffer, 558/590/602 buffers`)
- **NOT pickable**: picking scene (`RhiViewport.cpp:975-994 updatePickingScene`) only loads model objects, never wipe tower. So "select wipe tower" cannot even happen in Qt6 UI.

Selection model:
- `m_selectedSourceIndices` (QSet<int>) stores only real ModelObject array indices (`EditorViewModel.h:1241-1248`)
- `selectSourceObject()` (`:4246-4286`) double-guards: `sourceIndex < m_objects.size()` AND `currentPlateObjectIndices().contains(sourceIndex)` — wipe tower is not in currentPlateObjectIndices, so it can never enter the selection set.

Transform path:
- All ProjectServiceMock transform methods (`mirrorObject:3426`, `orientObject:3025`, `setObjectPosition/Rotation/Scale:5373/5397/5421`) have `size_t(index) >= model_->objects.size()` early-return guards. Even a forged out-of-range index returns false rather than segfaulting.

### Evidence files

- `src/core/viewmodels/EditorViewModel.{h,cpp}:4246-4286` — selectSourceObject double guard
- `src/core/services/ProjectServiceMock.cpp:3426, 3025, 5373-5421` — transform boundary guards
- `src/qml_gui/Renderer/RhiViewport.cpp:975-994` — picking scene excludes wipe tower
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:441, 558, 1184` — wipe tower independent render path

## Code changes: 0 lines

No code modifications. Both fixes are documented as "Qt6 architecture is immune" — these are upstream wxWidgets/OpenGL-specific issues with no Qt6 equivalent.

## Recommendation for Phase 187 (REGRESS-08)

For the CRASH-01 anchor in `v54RegressionLocked`, instead of asserting a code symbol that doesn't exist, assert the architectural immunities themselves:
- A1 anchor: verify `QPointer<RhiViewport>` exists in `RhiViewportRenderer.h` (the mechanism that makes A1 N/A).
- A2 anchor: verify `m_selectedSourceIndices` exists in `EditorViewModel.h` + wipe tower is NOT in picking scene (the mechanisms that make A2 N/A).

This locks the architectural preconditions that prevent these bugs, so any future refactor that removes the immunity will fail the gate.
