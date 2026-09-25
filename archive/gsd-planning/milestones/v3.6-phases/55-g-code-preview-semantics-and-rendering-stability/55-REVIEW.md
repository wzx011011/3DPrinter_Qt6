---
phase: 55
status: resolved
depth: standard
reviewed_files:
  - src/core/viewmodels/PreviewViewModel.h
  - src/core/viewmodels/PreviewViewModel.cpp
  - src/qml_gui/Renderer/RhiViewportRenderer.h
  - src/qml_gui/Renderer/RhiViewportRenderer.cpp
  - src/qml_gui/Renderer/RhiViewport.h
  - src/qml_gui/Renderer/RhiViewport.cpp
  - src/qml_gui/components/VisibilityFilter.qml
  - src/qml_gui/pages/PreviewPage.qml
  - tests/QmlUiAuditTests.cpp (role-filter tests, lines 1270-1302)
  - tests/ViewModelSmokeTests.cpp (no-repack test, lines 2955-2986)
critical: 1
warning: 1
info: 2
---

# Phase 55 Code Review — G-code Preview Semantics and Rendering Stability

## Summary

Phase 55 closes GCODE-01..05: it extends the GCV1 wire format with a canonical
libvgcode `int role`, replaces the coarse 5-category parser with a 20-role
canonical parser, adds render-side per-role visibility filtering, and ships a
VisibilityFilter.qml UI + GLViewport.roleVisibility binding.

The **critical correctness checks from the brief were all verified CORRECT
against upstream source truth**:

- **Enum divergence (the headline risk):** `kRoleMap[]` (PreviewViewModel.cpp:112-132)
  maps every `;TYPE:` display string **directly** to the canonical libvgcode
  `EGCodeExtrusionRole` index — never via the libslic3r integer. Spot-checked
  against `third_party/OrcaSlicer/src/libvgcode/include/Types.hpp:131-157`
  (`EGCodeExtrusionRole` order) and `third_party/OrcaSlicer/src/libslic3r/
  ExtrusionEntity.cpp:583-608` (`role_to_string`):
  `"Ironing"→7` ✓, `"Bottom surface"→15` ✓, `"Bridge"→8` ✓,
  `"Gap infill"→9` ✓, `"Skirt"→10` ✓, `"Support"→11` ✓,
  `"Support interface"→12` ✓, `"Prime tower"→13` ✓, `"Custom"→14` ✓,
  `"Internal Bridge"→16` ✓, `"Brim"→17` ✓, `"Support transition"→18` ✓.
- **`kRoleColors[20][3]`** (PreviewViewModel.cpp:137-158) matches upstream
  `DEFAULT_EXTRUSION_ROLES_COLORS` (`ViewerImpl.cpp:283-305`) byte-for-byte.
- **No-repack invariant:** `toggleRoleVisibility` (PreviewViewModel.cpp:727-736)
  flips `m_roleVisibility[i]` and emits `stateChanged()` only — no call to
  `recolorAndPackSegments()`, no mutation of `gcodePreviewData_`. Verified.
  `setShowTravelMoves` (PreviewViewModel.cpp:682-689) correctly KEEPS its repack
  (only the default flipped to `false`, PreviewViewModel.h:278).
- **Role index bounds:** `roleForTypeImpl` returns 0 (None) for unrecognized
  strings (PreviewViewModel.cpp:187-196); `roleColor` clamps to `[0,20)`
  (PreviewViewModel.cpp:712-718); `isRoleVisible` bounds-checks
  (PreviewViewModel.cpp:722); the renderer's role-skip bounds-checks
  (RhiViewportRenderer.cpp:716). All correct.
- **Wire-format lockstep:** `PackedSegment` (PreviewViewModel.cpp:46-67) and
  `GcvPackedSegment` (RhiViewportRenderer.cpp:584-597 **and** RhiViewport.cpp:20-29)
  carry the identical 16-float + 4-int layout. `static_assert(sizeof
  (GcvPackedSegment) == 76)` is present in **both** renderer TUs.

**However**, one finding below reveals that the role-visibility feature is
**non-functional at runtime** despite the correct per-component logic and the
green test suite. This is precisely the "subtle cross-file bug tests might not
catch" class the brief flagged, and it is the single most important outcome of
this review.

The test suite is green because every test validates a component in isolation:
`ViewModelSmokeTests` checks the no-repack invariant at the ViewModel level
only; `QmlUiAuditTests` greps the renderer source for the literal string
`"m_roleVisibility"`; no test binds the producer to the consumer end-to-end.

---

## Findings

### 1. Critical — Role-visibility feature is a dead path: binding feeds a 18-row QVariantMap list into a consumer that requires 20 dense bools

**File:line:**
- Producer — `src/core/viewmodels/PreviewViewModel.cpp:738-768` (`roleVisibilities()`)
- Binding — `src/qml_gui/pages/PreviewPage.qml:290`
- Consumer — `src/qml_gui/Renderer/RhiViewportRenderer.cpp:94-103` (`synchronize`) and `:716-718` (`computePreviewDrawRange`)

**Description:**

There is a data-shape contract mismatch between the property the ViewModel
exposes, the QML binding, and the property the renderer consumes. The three
stages disagree on what `roleVisibilities` / `roleVisibility` *is*, and the net
effect is that **clicking a checkbox in VisibilityFilter never hides any
segments** — the renderer's per-role mask is empty on every frame.

Trace of the data flow:

1. **Producer** `PreviewViewModel::roleVisibilities()` emits a `QVariantList`
   of **18 `QVariantMap` rows** (canonical roles 1..19 excluding None=0 and
   Custom=14, per the `kExcludedRoles[]` filter at line 745). Each element is a
   map `{roleIndex, label, color, visible}` — i.e. **18 elements, each a
   non-bool QVariantMap**.

2. **Binding** `PreviewPage.qml:290` binds this list verbatim into the
   viewport:
   ```qml
   roleVisibility: root.previewVm ? root.previewVm.roleVisibilities : []
   ```

3. **Consumer** `RhiViewportRenderer::synchronize` (RhiViewportRenderer.cpp:94-103)
   expects a **dense 20-element bool list** and gates on size:
   ```cpp
   if (viewport->m_roleVisibility.size() >= 20)        // 18 >= 20  →  FALSE
   {
     m_roleVisibility.resize(20);
     for (int i = 0; i < 20; ++i)
       m_roleVisibility[i] = viewport->m_roleVisibility.at(i).toBool();
   }
   else
   {
     m_roleVisibility.clear();                          // ← runs every frame
   }
   ```

Two compounding failures:

- **(a) Size gate fails.** `viewport->m_roleVisibility.size()` is 18, so the
  `>= 20` gate is `false` → the `else` branch runs → `m_roleVisibility.clear()`
  → the mask is **empty** for every frame after the first binding.
- **(b) Type mismatch (would compound even if size matched).**
  `viewport->m_roleVisibility.at(i).toBool()` converts a `QVariantMap` to bool.
  `QVariant(QVariantMap).toBool()` is an invalid conversion that returns
  `false`, so all 20 slots would read `false` → every segment masked out →
  blank preview.

**Net runtime effect:** in `computePreviewDrawRange` (RhiViewportRenderer.cpp:716):
```cpp
if (span.role >= 0 && span.role < m_roleVisibility.size()   // size() == 0
    && !m_roleVisibility[span.role])
  continue;
```
With `m_roleVisibility.size() == 0`, the guard `span.role < 0` is never true, so
**no span is ever skipped**. Every extrusion role always renders, regardless of
checkbox state. The VisibilityFilter UI visibly toggles, `toggleRoleVisibility`
correctly flips `m_roleVisibility[i]`, `stateChanged()` fires, the QML binding
re-pushes the row list, but the renderer discards it and the preview never
changes.

**Why the green test suite missed this:**
- `ViewModelSmokeTests::roleVisibilityToggleDoesNotRepackGcodePreviewData`
  (ViewModelSmokeTests.cpp:2962) asserts only that `gcodePreviewData_` is
  unchanged by a toggle — which it is (the no-repack invariant is correctly
  honored at the ViewModel level). It never checks the renderer filters.
- `QmlUiAuditTests::rhiViewportRendererComputePreviewDrawRangeAppliesRoleFilter`
  (QmlUiAuditTests.cpp:1273) greps the renderer source for the literal string
  `"m_roleVisibility"` (line 1285). The string is present, so the test passes,
  but the test does not exercise the producer→binding→consumer data path.
- No test instantiates `PreviewViewModel`, toggles a role, runs the renderer
  synchronize, and asserts the draw range changes. There is no end-to-end
  integration test of the filter.

This is the highest-value finding of the review: the enum-divergence fix and
no-repack invariant are both correct, but the feature ships visibly broken
because two `Q_PROPERTY`s that the brief treats as the same concept (`roleVisibilities`
on the ViewModel vs `roleVisibility` on the Viewport) carry **different shapes**
and are wired together by a single binding line.

**Suggested fix (advisory — do not apply per review scope):**

Expose a second, renderer-shaped property from the ViewModel that emits a dense
20-element bool list, and bind THAT to the viewport. Keep `roleVisibilities`
(the 18 QVariantMap rows) for the UI Repeater. For example, add:

```cpp
// PreviewViewModel.h
Q_PROPERTY(QVariantList roleVisibilityMask READ roleVisibilityMask NOTIFY stateChanged)

// PreviewViewModel.cpp
QVariantList PreviewViewModel::roleVisibilityMask() const {
  QVariantList mask;
  for (int i = 0; i < 20; ++i) mask.append(m_roleVisibility[i]);
  return mask;
}
```

and change the binding in PreviewPage.qml:290 to:
```qml
roleVisibility: root.previewVm ? root.previewVm.roleVisibilityMask : []
```

(Alternatively, teach the renderer's `synchronize` to interpret the QVariantMap
rows by their `roleIndex` key into a 20-bool array. The dedicated-mask approach
is cleaner and keeps the UI rows and the render mask decoupled.)

Either way, add an integration-level test that drives the full path and asserts
that toggling a role changes the renderer's filtered span count — the current
tests guard the pieces but not the contract between them.

---

### 2. Warning — No producer-side `static_assert(sizeof(PackedSegment) == 76)`; wire-format lockstep is enforced only on the consumer side

**File:line:** `src/core/viewmodels/PreviewViewModel.cpp:46-67` (the `PackedSegment` definition has no size assertion).

**Description:**

The renderer has `static_assert(sizeof(GcvPackedSegment) == 76)` in **two** TUs
(RhiViewportRenderer.cpp:594 and RhiViewport.cpp:28), but the producer's
`PackedSegment` in `PreviewViewModel.cpp` has **no** equivalent guard. The
research doc "Pitfall 1" and the Phase 55 brief both specify that the two
structs be "lockstepped via static_assert". The lockstep is currently maintained
only by careful manual synchronization of two separate anonymous-namespace
structs.

The research example (55-RESEARCH.md:640) shows a cross-file assert
(`sizeof(PackedSegment) == sizeof(GcvPackedSegment)`), which cannot compile
across anonymous-namespace TUs. But a producer-side
`static_assert(sizeof(PackedSegment) == 76, ...)` *can* and would catch a
future one-sided field addition as a build error in the producer TU.

The structs are currently identical and the consumer asserts guard both
renderer copies, so this is a robustness gap rather than a live bug — but it is
exactly the "Pitfall 1" scenario the phase set out to defend against, and the
defense is half-built.

**Suggested fix:** Add `static_assert(sizeof(PackedSegment) == 76,
"PackedSegment must be 76 bytes (16 floats + 4 ints) to match GcvPackedSegment");`
immediately after the `PackedSegment` definition in PreviewViewModel.cpp.

---

### 3. Info — Stale comment says segments are "80 bytes each" (now 76)

**File:line:** `src/qml_gui/Renderer/RhiViewportRenderer.cpp:578`

**Description:**

The Phase 26 block comment reads:
```
// count * PackedSegment (80 bytes each). Each segment → 2 Line vertices ...
```
After Phase 55 added the `int role` field, the struct is 76 bytes (16 floats =
64 + 4 ints = 16). The comment predates the change and was not updated. Harmless
in practice — the actual parse uses `sizeof(GcvPackedSegment)` (RhiViewportRenderer.cpp:613)
and the size is guarded by the static_assert — but the comment is now
misleading to anyone reasoning about the wire format.

**Suggested fix:** Update "80 bytes each" → "76 bytes each" in the comment.

---

### 4. Info — `logOnceIfNeeded` uses function-local statics shared across all PreviewViewModel instances

**File:line:** `src/core/viewmodels/PreviewViewModel.cpp:227-241` (`logOnceIfNeeded`)

**Description:**

`logOnceIfNeeded` guards its one-time log line with `static bool logged[4]`
and `static const int modes[4]` (PreviewViewModel.cpp:229-230). Function-local
statics have process lifetime and are shared across all instances of
`PreviewViewModel`. In the production app there is one ViewModel, so the
behavior is as intended. But in test scenarios that construct multiple
ViewModels (or run the fixture-driven path repeatedly), the "log once" guard
becomes "log once for the entire test process," which can mask whether a given
instance ever hit the deferred-data branch.

This is minor and arguably intentional (the message is informational), but
worth noting since the brief asks for edge cases tests might not catch, and
the test that would catch a never-logging regression is itself gated by this
shared state.

**Suggested fix:** No change needed for production. If test observability
matters, convert the guard to a per-instance member `bool m_loggedDeferred[4]`
initialized in the constructor.
