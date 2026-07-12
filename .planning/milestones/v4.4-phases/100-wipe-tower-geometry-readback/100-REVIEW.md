---
status: findings
files_reviewed:
  - src/core/services/SliceService.h
  - src/core/services/SliceService.cpp
  - src/core/viewmodels/EditorViewModel.h
  - src/core/viewmodels/EditorViewModel.cpp
  - src/qml_gui/pages/PreparePage.qml
  - src/qml_gui/Renderer/SoftwareViewport.h
  - tests/ViewModelSmokeTests.cpp
base_commit: 168fbc7
head_commit: 30cad85
counts:
  critical: 0
  warning: 1
  info: 5
  total: 6
---

# Phase 100 Code Review — Wipe-Tower Geometry Readback

Reviewed the `168fbc7..HEAD` diff (7 commits, 454 insertions). Focus areas
per the review brief: thread safety, lifetime/TOCTOU, signal/slot gating
(WTREAD-02), Q_PROPERTY correctness, gate enforcement, test quality, and
the c45db36 fixup. All frozen-decision and gating invariants verified sound;
one geometric-semantics WARNING and five INFO notes follow.

## Findings

| ID | Severity | File | Line | Summary |
|----|----------|------|------|---------|
| W1 | warning  | src/core/services/SliceService.cpp | 641-642 | Captured x/z is the tower CORNER (bbx.min + rib_offset), but the consumer `GizmoGeometry::buildWipeTowerVertices` expects a CENTER (uses `x - hw`/`x + hw`) — renders offset by (+w/2, +d/2). |
| I1 | info     | src/core/services/SliceService.h | 53-60 | `brimWidth`, `ribOffsetX`, `ribOffsetY` fields are captured but never read by EditorViewModel or the renderer. |
| I2 | info     | src/core/viewmodels/EditorViewModel.cpp | 5107-5108 | `onWipeTowerGeometryReady` always emits `wipeTowerGeometryChanged()` even when no member value actually changed (e.g. consecutive identical readbacks). |
| I3 | info     | src/qml_gui/Renderer/SoftwareViewport.h | 236-240 | SoftwareViewport wipe-tower dim defaults (0.f) are inconsistent with the RhiViewport/EditorViewModel baseline (10/10/50/100/25); benign only while show=false. |
| I4 | info     | tests/ViewModelSmokeTests.cpp | 3963-3965 | The `qRegisterMetaType<WipeTowerGeometry>` static initializer is test-only (needed for the by-name `Q_ARG` invocation). Production does NOT register the metatype — this is correct because the cross-thread hop uses a functor capture, not Q_ARG marshalling. Confirming soundness, not flagging a gap. |
| I5 | info     | tests/ViewModelSmokeTests.cpp | 3987-4024 | Smoke test asserts raw value passthrough (200→200) only; it does not cover geometric-position correctness (corner-vs-center), so it would not catch W1. |

## Detail

### W1 — Corner-vs-center coordinate mismatch (warning)

**Location**: `src/core/services/SliceService.cpp:641-642` (capture) flowing to
`src/qml_gui/Renderer/RhiViewportRenderer.cpp:1075-1079` (consume) via
`src/core/viewmodels/EditorViewModel.cpp` and `src/qml_gui/pages/PreparePage.qml:1670-1675`.

**Evidence**:

The capture computes the upstream corner origin `pt0`:
```cpp
capturedGeometry.x = float(bbx.min.x() + ribOffset.x());   // SliceService.cpp:641
capturedGeometry.z = float(bbx.min.y() + ribOffset.y());   // SliceService.cpp:642
```
This matches upstream `Print.cpp:2871-2873`, where `pt0 = bbx.min + rib_offset`
is unambiguously the bottom-left CORNER — the upstream rectangle is then built
as `{pt0, pt0+(w,0), pt0+(w,d), pt0+(0,d)}` (`Print.cpp:2874-2877`).

The Qt consumer treats the same value as a CENTER:
```cpp
// src/core/rendering/GizmoGeometry.cpp:465-469
const float hw = width * 0.5f;
const float hd = depth * 0.5f;
const float xMin = x - hw;
const float xMax = x + hw;
const float zMin = z - hd;
```
`buildWipeTowerVertices(x, z, width, depth, height)` is invoked directly with
`m_wipeTowerX`/`m_wipeTowerZ` at `RhiViewportRenderer.cpp:1075-1079`, and the
pipeline performs a pure passthrough with no corner→center conversion anywhere.

**Impact**: the rendered wipe-tower box is translated by `(+width/2, +depth/2)`
in the bed plane relative to its true sliced position. For a typical
`prime_tower_width` of ~60 mm this is a ~30 mm visual offset. The tower still
renders with correct width/depth/height — only its position is wrong.

**Severity rationale**: WARNING rather than CRITICAL. No crash, no memory
issue, no violation of Frozen Decision 1 or WTREAD-02. But this phase's
declared deliverable is "real rendering" of the read-back geometry, and
position correctness is part of that. The inline comment at
`SliceService.cpp:635-638` claims the X/Z derivation "matching buildWipeTowerVertices
at GizmoGeometry.cpp:449" — the Y→Z plane mapping matches, but the
corner-vs-center convention does not, so the comment is misleading.

**Suggested fix (either side, not both)**:
- Capture side: `capturedGeometry.x = float(bbx.min.x() + ribOffset.x()) + capturedGeometry.width * 0.5f;` (and analogously for z with `depth`); or
- Consumer side: convert corner→center in `RhiViewportRenderer` before calling `buildWipeTowerVertices`.

The test (I5) would not catch this; a follow-up should add a geometric-position
assertion or a visual UAT step.

### I1 — Captured-but-unused struct fields (info)

`WipeTowerGeometry::brimWidth`, `ribOffsetX`, `ribOffsetY` are populated in the
worker (`SliceService.cpp:640`, `643-644`) but `EditorViewModel::onWipeTowerGeometryReady`
only reads `width/depth/height/x/z` (`EditorViewModel.cpp:5093-5097`). The
fields mirror upstream `WipeTowerData` for future-proofing; currently dead
weight on the cross-thread struct. Harmless (POD), but if they remain unused
they should either be dropped (minimize the frozen cross-thread surface) or
the brim/rib rendering should land in a follow-up. No action required for
Phase 100 acceptance.

### I2 — Unconditional NOTIFY emit (info)

`onWipeTowerGeometryReady` always emits `wipeTowerGeometryChanged()` even when
the readback is identical to the current state (e.g. two consecutive
single-material slices both produce `valid=false` with unchanged dims, or two
identical valid readbacks). This triggers six QML binding re-evaluations and a
`m_wipeTowerDirty` rebuild cycle in `RhiViewportRenderer` for no-op updates.
Cheap in practice (small struct, infrequent event — once per slice). A
short-circuit `if (changed) emit;` would be a minor efficiency improvement,
not a correctness fix.

### I3 — SoftwareViewport default inconsistency (info)

After the WTREAD-02 alignment, `SoftwareViewport.h` has `m_showWipeTower = false`
but keeps `m_wipeTowerWidth/Depth/Height/X/Z = 0.f`, whereas the aligned
baselines (`RhiViewport.h:304-309`, `EditorViewModel.h:1039-1045`,
PreparePage.qml fallbacks) use `10/10/50/100/25`. SoftwareViewport is not
instantiated in any page today (only registered as the alternate GLViewport
QML type at `main_qml.cpp:305`), so this is invisible while `show=false`. If
SoftwareViewport is ever activated, the 0.f defaults would briefly disagree
with the rest of the stack until the first readback — aligning them to
`10/10/50/100/25` would be tidier defense-in-depth. No action required.

### I4 — Metatype registration scope (info, confirming soundness)

`qRegisterMetaType<WipeTowerGeometry>` appears ONLY in
`tests/ViewModelSmokeTests.cpp:3963-3965` (a file-scope static initializer
with `Q_UNUSED`). Production code never registers the metatype, and this is
CORRECT: the cross-thread hop uses the functor overload
`QMetaObject::invokeMethod(receiver, lambda, Qt::QueuedConnection)` at
`SliceService.cpp:716`, which wraps the lambda in a `QFunctorSlotObject` —
the captured `WipeTowerGeometry` travels inside the C++ lambda object and is
never marshalled through `Q_ARG`/the meta-type system. The subsequent
`emit receiver->wipeTowerGeometryReady(capturedGeometry)` runs on the GUI
thread (both `sliceService_` and `EditorViewModel` live on the GUI thread), so
the AutoConnection resolves to DirectConnection and the slot receives a plain
`const WipeTowerGeometry &` to a GUI-thread-local copy. No metatype needed.

### I5 — Test does not cover geometric position (info)

`wipeTowerGeometryReadbackAppliesValidAndInvalidGate` is a wiring smoke test:
it asserts raw passthrough (`validGeo.x = 200.f` → `editor.wipeTowerX() == 200.f`)
and the WTREAD-02 gate behavior (`valid=false` → `show=false` + dims persist).
It does NOT validate that the captured `x/z` is geometrically correct
(corner vs center) relative to what the renderer expects. This is why W1
escapes the test. The test's scope (wiring, not geometry) is reasonable for a
smoke test; a geometric-position assertion or a render-output golden would be
a separate follow-up tied to the W1 fix.

## Verified Sound (no action)

- **Frozen Decision 1 (capture-by-value, no Print* escape)**: POD struct, worker-local construction, by-value lambda capture, no `Print*`/`WipeTowerData*` escapes the worker. The `const Slic3r::WipeTowerData &wtData` reference is consumed synchronously inside the try-block to copy scalars out only.
- **No TOCTOU**: capture reads the stack-local `print`, not `activePrint_.load()`; `activePrint_.store(nullptr)` publishes state to other threads but does not touch the worker's local reference.
- **WTREAD-02 gate (success-branch-only emit)**: the single `wipeTowerGeometryReady` emitter sits after `sliceFinished` and after the cancel/error early returns; cancel and error branches cannot reach it.
- **WTREAD-02 gate (data-driven)**: `has_wipe_tower()=false` leaves `capturedGeometry.valid=false`; `onWipeTowerGeometryReady` then forces `m_showWipeTower=false` and leaves the dim members untouched (dims persist, NOT reset to placeholders) — matches the design intent and the test's invalid-path assertions.
- **Q_PROPERTY correctness**: 6 READ-only + shared NOTIFY, no WRITE — correct one-way libslic3r→GUI flow.
- **Defaults alignment**: EditorViewModel (`false`, `10/10/50/100/25`) == RhiViewport.h:304-309 == PreparePage.qml fallbacks.
- **c45db36 fixup**: `private slots:` block correctly placed at line 822, immediately before `signals:` (831) and after all public members; no public surface accidentally re-classified. The C2248 breakage is resolved.
- **Test quality**: QSignalSpy + by-name invokeMethod exercises the connect wiring end-to-end; valid + invalid path coverage; invalid path correctly asserts dims persist rather than reset — matching WTREAD-02 intent.

## Conclusion

Phase 100 satisfies WTREAD-01 (readback wired end-to-end) and WTREAD-02 (has_wipe_tower gate enforced, no placeholder leak). All frozen-decision and gating invariants verified sound. The one WARNING (W1, corner-vs-center coordinate mismatch) is a real geometric-position bug that should be fixed before Phase 101/102 since it directly affects "real rendering" correctness. The five INFO items are minor and non-blocking.

Full report: `.planning/phases/100-wipe-tower-geometry-readback/100-REVIEW.md`
