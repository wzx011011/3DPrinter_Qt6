---
phase: 55-g-code-preview-semantics-and-rendering-stability
plan: 02
subsystem: ui
tags: [qml, qt6, qml-gui, rhi, gcode-preview, libvgcode, orcaslicer, extrusion-role]

# Dependency graph
requires:
  - phase: 55-g-code-preview-semantics-and-rendering-stability (plan 01)
    provides: PreviewParserTests target scaffold + OrcaSlicer-style G-code fixture + RED test slots
provides:
  - GCV1 wire format extended with canonical int role (PackedSegment/GcvPackedSegment = 76 bytes, lockstepped via static_assert)
  - Render-side per-role visibility filter (no repack; toggleRoleVisibility emits stateChanged only)
  - 20-role canonical libvgcode parser replacing the coarse 5-category styleFor()
  - 17 upstream EViewType view modes (Summary..Tool) aligned with libvgcode update_by_mode order
  - Q_INVOKABLE roleForType/roleColor/isRoleVisible/toggleRoleVisibility + roleVisibilities Q_PROPERTY
  - showTravelMoves_ default flipped to false (upstream GCodeViewer alignment)
  - Divergent-role-color regression guard (Ironing=7 / Bottom surface=15)
affects:
  - 55-g-code-preview-semantics-and-rendering-stability (plan 03: UI binding to role toggles)
  - 55-g-code-preview-semantics-and-rendering-stability (plan 04: audit-test extensions for GCODE-01/03/04/05)

# Tech tracking
tech-stack:
  added: []  # no new libraries; uses existing Qt6 + libvgcode headers already vendored
  patterns:
    - "Canonical libvgcode index is the single source of truth for extrusion role (never the libslic3r integer — enums diverge past index 6)"
    - "Render-side filtering over repacking: toggleRoleVisibility flips a mask + emits stateChanged; renderer skips masked spans in computePreviewDrawRange (Phase 41 interaction-stability invariant)"
    - "GCV1 wire-format lockstep guarded by static_assert(sizeof(GcvPackedSegment) == 76) on both viewmodel and renderer sides"

key-files:
  created: []
  modified:
    - src/core/viewmodels/PreviewViewModel.h
    - src/core/viewmodels/PreviewViewModel.cpp
    - src/qml_gui/Renderer/RhiViewportRenderer.h
    - src/qml_gui/Renderer/RhiViewportRenderer.cpp
    - src/qml_gui/Renderer/RhiViewport.h
    - src/qml_gui/Renderer/RhiViewport.cpp
    - tests/PreviewParserTests.cpp
    - tests/E2EWorkflowTests.cpp

key-decisions:
  - "Use the libvgcode EGCodeExtrusionRole index (0..19) as the canonical role value everywhere; map display strings DIRECTLY to this index via kRoleMap so the libslic3r ExtrusionRole integer is never used as an index (Pitfall 6 enum-divergence fix)"
  - "Render-side per-role visibility filters in computePreviewDrawRange instead of repacking — preserves the Phase 41 interaction-stability invariant (no GCV1 mutation during interaction)"
  - "setShowTravelMoves keeps its repack behavior; only the DEFAULT was flipped to false (separate concern from per-role toggle)"
  - "Flip the 3 RED PreviewParserTests slots to GREEN with QSKIP removal (not QEXPECT_FAIL — a single QEXPECT_FAIL only covers one assertion)"
  - "Align viewModes() to the full 17 upstream EViewType modes rather than the prior 13-mode subset; E2E test mode-index assertions renumbered accordingly"

patterns-established:
  - "kRoleMap: display-string -> canonical libvgcode index lookup table (single source of truth for role resolution)"
  - "kRoleColors[20][3]: upstream DEFAULT_EXTRUSION_ROLES_COLORS indexed by canonical libvgcode index"
  - "EViewType VT_* enum mirrors upstream libvgcode update_by_mode ordering (VT_Summary=0 .. VT_Tool=16)"
  - "Role visibility exposed to QML as QVariantList roleVisibilities (ascending canonical index, None(0)/Custom(14) hidden from UI rows but kept in the array for safe indexing)"

requirements-completed: [GCODE-01, GCODE-02]

# Metrics
duration: 55min
completed: 2026-07-02
---

# Plan 02: G-code Preview Data Model + Renderer Contract Summary

**20-role canonical libvgcode extrusion-role parser, 17 upstream EViewType view modes, render-side per-role visibility, and the GCV1 wire-format `int role` extension — closing GCODE-01/02 at the data+renderer contract level**

## Performance

- **Duration:** ~55 min
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments
- Extended the GCV1 packed-segment wire format with a canonical `int role` field (72 -> 76 bytes), lockstepped across PreviewViewModel (PackedSegment) and the renderer (GcvPackedSegment in RhiViewport + RhiViewportRenderer) and guarded by `static_assert(sizeof(GcvPackedSegment) == 76)`
- Added render-side per-role visibility (Q_PROPERTY roleVisibility + Q_INVOKABLE toggleRoleVisibility) that filters in computePreviewDrawRange WITHOUT repacking the GCV1 payload — preserving the Phase 41 interaction-stability invariant
- Replaced the coarse 5-category styleFor() with a fine-grained 20-role parser keyed on the canonical libvgcode EGCodeExtrusionRole index (the Pitfall 6 enum-divergence fix: libslic3r and libvgcode roles diverge past index 6)
- Aligned viewModes() to the 17 upstream EViewType modes (Summary..Tool) and renumbered the E2E test mode-index assertions
- Flipped 3 RED PreviewParserTests slots to GREEN + added a divergent-role-color regression guard, and flipped the showTravelMoves_ default to false

## Task Commits

Each task was committed atomically:

1. **Task 1: extend GCV1 wire format with int role + render-side role visibility + showTravelMoves default false** - `ee68dec` (feat)
2. **Task 2: replace styleFor with 20-role canonical libvgcode parser, 17 upstream view modes, divergent-role-color guard** - `e390da6` (feat)

## Files Created/Modified
- `src/core/viewmodels/PreviewViewModel.h` - StoredSegment.int role; Q_PROPERTY roleVisibilities; Q_INVOKABLE roleForType/roleColor/isRoleVisible/toggleRoleVisibility; m_roleVisibility[20]; showTravelMoves_ default=false
- `src/core/viewmodels/PreviewViewModel.cpp` - PackedSegment.int role; kRoleMap/kRoleColors/kRoleLabels tables; roleForTypeImpl; EViewType VT_* enum; parser uses canonical role index; recolorAndPackSegments+buildLegendItems rewritten with VT_* labels; viewModes() returns 17 modes; toggleRoleVisibility emits stateChanged only (no repack)
- `src/qml_gui/Renderer/RhiViewportRenderer.h` - PreviewDrawSpan.int role; QVector<bool> m_roleVisibility
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp` - GcvPackedSegment.int role + static_assert 76; span append carries role; synchronize() reads roleVisibility; computePreviewDrawRange skips masked roles
- `src/qml_gui/Renderer/RhiViewport.h` - Q_PROPERTY roleVisibility + getter/setter/signal; m_roleVisibility member
- `src/qml_gui/Renderer/RhiViewport.cpp` - GcvPackedSegment.int role + static_assert 76; ctor inits 20 trues; setRoleVisibility stores + update() only (no mutation)
- `tests/PreviewParserTests.cpp` - flipped 3 RED slots to GREEN (role mapping / 17 modes / Summary no-legend) + new test_divergent_role_colors_correct (Ironing=7->(255,140,105), Bottom surface=15->(102,92,199))
- `tests/E2EWorkflowTests.cpp` - moveCount()->extrudeMoveCount() (default false); setShowTravelMoves(true) before 2-move count; 4 viewModeIndex assertions renumbered to 17-mode indices (Fan Speed 13, Temperature 14, Acceleration 5, Tool 16 / Filament 2)

## Decisions Made
- **Canonical role = libvgcode index, never libslic3r int.** kRoleMap maps display strings directly to the libvgcode index; the libslic3r ExtrusionRole integer is never used to index kRoleColors. This is the Pitfall 6 fix: the two enums diverge past index 6 (e.g. libslic3r Bottom surface=7 vs libvgcode=15; libslic3r Ironing=8 vs libvgcode=7).
- **Render-side filtering, not repack.** toggleRoleVisibility flips m_roleVisibility[role] and emits stateChanged() only; it does NOT call recolorAndPackSegments() or mutate gcodePreviewData_. The renderer's computePreviewDrawRange skips spans whose role is masked off. This preserves the Phase 41 interaction-stability invariant (interactions must not trigger GCV1 repacking).
- **showTravelMoves keeps its repack; only the default flips.** setShowTravelMoves retains its existing recpack behavior; the change is strictly the default value true->false (upstream GCodeViewer alignment). This is a separate concern from per-role visibility.
- **QSKIP removal, not QEXPECT_FAIL, to flip RED->GREEN.** A single QEXPECT_FAIL only covers one assertion; the slots make multiple assertions, so QSKIP was the correct mechanism (matching Plan 55-01's convention).
- **17-mode viewModes() renumber.** viewModes() now returns the full upstream EViewType set in update_by_mode order; the E2E test mode-index assertions were renumbered (Fan Speed 5->13, Temperature 6->14, Acceleration 12->5, Tool 3->16 / Filament 2).

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Breaking test fix] E2EWorkflowTests moveCount() assertion after showTravelMoves default flip**
- **Found during:** Task 1 (build verification)
- **Issue:** Flipping showTravelMoves_ default true->false excludes travel segments from the GCV1 payload, so the test asserting `gcv1SegmentCount == preview.moveCount()` failed (count dropped to the extrude-only subset).
- **Fix:** Changed the assertion to `preview.extrudeMoveCount()`, and added `preview.setShowTravelMoves(true)` before the subsequent 2-move count assertion to restore the travel-inclusive payload for that check.
- **Files modified:** tests/E2EWorkflowTests.cpp
- **Verification:** E2EWorkflowTests passes (exit 0)
- **Committed in:** ee68dec (part of Task 1 commit)

**2. [Rule 1 - Breaking test fix] E2EWorkflowTests viewModeIndex assertions after 17-mode renumber**
- **Found during:** Task 2 (build verification)
- **Issue:** viewModes() expanded from 13 to 17 upstream modes, shifting indices (Fan Speed 5->13, Temperature 6->14, Acceleration 12->5, Tool 3->16, Filament->2). The E2E assertions used the old indices.
- **Fix:** Renumbered all 4 viewModeIndex assertions to the new EViewType indices.
- **Files modified:** tests/E2EWorkflowTests.cpp
- **Verification:** E2EWorkflowTests passes (exit 0)
- **Committed in:** e390da6 (part of Task 2 commit)

**3. [Rule 4 - Build infra] Focused incremental build helper for build-timeout mitigation (created and removed)**
- **Found during:** Tasks 1 & 2 (canonical build verification)
- **Issue:** The canonical build script (scripts/auto_verify_with_vcvars.ps1) does a full ~6-8min libslic3r rebuild on every run, exceeding the 10-min execution cap before the ViewModelSmokeTests MOC step (and thus the ctest/PreviewParser phase) completes.
- **Fix:** Used a transient inline PowerShell invocation (no permanent file; created and deleted) that reuses vcvars64.bat + incremental ninja on just the test targets, then ran the test exes directly via ctest. The canonical build script and CMakeLists.txt were NOT modified (orchestrator-owned).
- **Files modified:** none committed (transient helper only)
- **Verification:** All acceptance-gate test exes return exit 0; two full canonical runs show zero compile/link errors.
- **Committed in:** n/a (no committed artifact)

---

**Total deviations:** 3 auto-fixed (2 breaking-test fixes [Rule 1], 1 build-infra mitigation [Rule 4])
**Impact on plan:** All auto-fixes necessary for correctness and build verification. No scope creep. The canonical build script and CMakeLists.txt were left untouched as required.

## Issues Encountered
- **PreviewParserTests meta-call type mismatch (Task 1, resolved in Task 2):** The Task 1 RED test invoked roleForType via QMetaObject::invokeMethod with Q_RETURN_ARG(QVariant,...) but roleForType returns int, causing a conversion failure. This was EXPECTED RED mid-plan; Task 2 resolved it by calling preview.roleForType() directly and asserting all 20 canonical indices.
- **Canonical build deterministic timeout:** Each canonical run rebuilds all 237 libslic3r objects (~6-8min) before reaching the test-compile phase, hitting the 10-min cap at the ViewModelSmokeTests MOC step. Verified the code itself compiles/links with zero errors across two full runs (OWzxSlicer.exe + libslic3r + E2EWorkflowTests all link clean), and obtained the definitive green signal by running the freshly-built E2EWorkflowTests.exe (15:08, against the 15:06 libslic3r) + the 5 acceptance-gate test exes directly — all exit 0.
- **CliTests (pre-existing, NOT in acceptance gate):** 2 CliTests failures were confirmed pre-existing/environmental (invoke the owzx-cli binary, check stdout for "object") with zero overlap against any file modified in this plan.

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- The data+renderer contract for GCODE-01/02 is complete at the viewmodel+renderer level. Plan 03 binds the QML UI to the new roleForType/roleColor/isRoleVisible/toggleRoleVisibility + roleVisibilities API and the 17-mode selector.
- The live no-placeholder RED test for GCODE-01 (real slice output) lands in Plan 04, alongside the audit-test extensions for GCODE-03/04/05.

## Self-Check: PASSED

- [x] `git diff e390da6 HEAD` shows clean working tree (committed state == disk state)
- [x] Every test exe is newer than its source file (no stale-against-source exes)
- [x] E2EWorkflowTests.exe (built 15:08 against the 15:06 libslic3r lib) returns exit 0
- [x] PreviewParserTests, ViewModelSmokeTests, QmlUiAuditTests, PrepareSceneDataTests, PartPlateTests all return exit 0
- [x] Two full canonical runs show zero `error C` / link errors (OWzxSlicer.exe + libslic3r + completed test targets link clean)
- [x] Acceptance greps confirmed on disk: `showTravelMoves_ = false`; `static_assert(sizeof(GcvPackedSegment) == 76)` in both renderer files; `toggleRoleVisibility` emits stateChanged only (no recolor); render-side role filter `!m_roleVisibility[span.role]` at RhiViewportRenderer.cpp:716-717; `[PreviewParser] PreviewParser tests passed` sentinel present in the canonical build script
- [x] toggleRoleVisibility does NOT repack (Phase 41 interaction-stability invariant preserved)
- [x] setShowTravelMoves keeps its repack behavior (only the default flipped)
- [x] Canonical build script (scripts/auto_verify_with_vcvars.ps1) and CMakeLists.txt were NOT modified

**NOTE on canonical-build sentinel:** The canonical build script did not reach its `[PreviewParser] PreviewParser tests passed` print in these runs because the full libslic3r rebuild (~6-8min) consumed the available time budget before the test-compile phase finished. The green signal was instead established by (a) two clean full builds (zero compile/link errors) and (b) direct execution of the freshly-built test exes (all exit 0). This is an environmental timeout, not a code failure.

---
*Phase: 55-g-code-preview-semantics-and-rendering-stability*
*Plan: 02*
*Completed: 2026-07-02*
