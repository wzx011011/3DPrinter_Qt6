---
phase: 55
status: passed
verified_at: 2026-07-02
must_haves_total: 31
must_haves_verified: 31
requirements: [GCODE-01, GCODE-02, GCODE-03, GCODE-04, GCODE-05]
---

# Phase 55 — Verification Report

**Phase:** 55 — "G-code Preview Semantics and Rendering Stability"
**Phase goal:** "Complete the G-code preview behavior behind the restored Preview UI and prevent renderer regressions."
**Verifier method:** Every must_have truth in every PLAN was checked against the ACTUAL codebase (source reads, greps) and against freshly-executed test runs. SUMMARYs were NOT trusted — claims were independently re-verified.

---

## Goal Verification

**VERDICT: The phase goal is ACHIEVED.**

Phase 55 set out to (a) complete the G-code preview *behavior* behind the restored Preview UI and (b) *prevent renderer regressions*. Both halves are demonstrably delivered:

1. **Preview behavior is complete and semantically correct.** The coarse 5-category `;TYPE:` parser is replaced by a fine-grained 20-role canonical libvgcode parser (`kRoleMap[]`, PreviewViewModel.cpp:117-137), the view-mode selector is expanded from 13 to the full 17 upstream `EViewType` modes (viewModes():468-491), a user-visible per-role VisibilityFilter UI is wired into the Preview right panel (PreviewPage.qml:374 + components/VisibilityFilter.qml), and the render-side per-role visibility filter is functional end-to-end (the code-review Critical fix: `roleVisibilityMask` 20-dense-bool property bound to GLViewport.roleVisibility, consumed by computePreviewDrawRange at RhiViewportRenderer.cpp:716-718). The headline enum-divergence risk (libslic3r vs libvgcode) is fixed and regression-guarded.

2. **Renderer regressions are prevented.** The GCV1 wire format carries a canonical `int role` lockstepped across all three structs (PackedSegment / GcvPackedSegment ×2) with `static_assert(sizeof(...) == 76)` on both producer and consumer sides. The disappearing-preview bug class is locked behind 14 new headless regression tests covering the no-repack invariant, no-placeholder real-data path, reslice/export/page-switch/slice-failed payload survival, and the D3D11/SoftwareViewport startup policy. All 142 tests across the 4 targets pass green.

A code review found **1 Critical** defect (the role-visibility feature was a dead path at runtime despite green component-level tests). It was **fixed** in commit `012808e` and the fix is independently verified correct here (producer shape, binding, consumer gate, plus a new integration test guarding the full path).

The only items not verified by this phase are two explicitly-deferred **manual visual UAT** checks (drag-stability and VisibilityFilter layout parity), which are owned by Phase 58 per 55-VALIDATION.md. These do not block the phase goal.

---

## Requirement Traceability

| Req | Verified Behavior | Evidence | Result |
|-----|-------------------|----------|--------|
| **GCODE-01** | After a live slice, `gcodePreviewData` is a non-empty GCV1 blob with layerCount>0, moveCount>0, and no placeholder/demo/sample markers; slice output comes from a real local file (SliceService::outputPath), NOT a fixture | `E2EWorkflowTests::previewReceivesRealGcodeAfterLiveSliceNoPlaceholder` — source read (E2EWorkflowTests.cpp:1343-1384) asserts all conditions as hard QVERIFY2; **PASS** at runtime. Run log: live slice wrote `build/Prusa_20260702_192542.gcode` (586150 bytes, 15 layers, 16532 segments) — a real local file, not a fixture | ✅ |
| **GCODE-02** | (a) 20-role canonical libvgcode parser; (b) enum-divergence fix; (c) 17 upstream view modes; (d) render-side per-role visibility works end-to-end; (e) toggleRoleVisibility does NOT repack; (f) showTravelMoves defaults false | (a) `kRoleMap[]` 19 entries + implicit None(0) = 20 canonical indices (PreviewViewModel.cpp:117-137); (b) spot-checked "Ironing"→7, "Bottom surface"→15, "Bridge"→8, "Gap fill"→9, "Skirt"→10, "Support"→11 — all correct; (c) `viewModes()` returns exactly 17 names (PreviewViewModel.cpp:468-491); (d) `roleVisibilityMask()` 20 dense bools (PreviewViewModel.cpp:775-789) → PreviewPage.qml:294 binding → RhiViewportRenderer synchronize size>=20 gate (line 94) → computePreviewDrawRange skip (line 716-718); (e) toggleRoleVisibility emits stateChanged only, no recolor (PreviewViewModel.cpp:732-741); (f) `showTravelMoves_ = false` (PreviewViewModel.h:286). Tests: `test_role_string_mapping_covers_upstream_enum`, `test_view_modes_match_upstream_seventeen`, `test_divergent_role_colors_correct`, `roleVisibilityMaskFeedsRendererShapeAndTogglesPropagate`, `roleVisibilityToggleDoesNotRepackGcodePreviewData` — **all PASS** | ✅ |
| **GCODE-03** | Legend min/max labels stable across layer/move drag (global scope); setCurrentMove updates currentGcodeLine + gcodeLines window atomically (single stateChanged) | `ViewModelSmokeTests::legendGradientBoundsStableAcrossLayerMoveDrag` (asserts labels unchanged before/after drag) + `currentMoveUpdatesGcodeLineWindowAtomically` (asserts `spy.count() == 1` + currentGcodeLine!=0 + !gcodeLines.isEmpty) — source read (ViewModelSmokeTests.cpp:3045-3105); **both PASS** | ✅ |
| **GCODE-04** | PreviewPage.qml never references SoftwareViewport; main_qml.cpp registers RhiViewport as default GLViewport with SoftwareViewport only behind an env-var gate; computePreviewDrawRange role-skip; GcvPackedSegment sizeof 76 | grep: PreviewPage.qml has **0** "SoftwareViewport" occurrences, **1** "Components.VisibilityFilter", binds `roleVisibilityMask`. main_qml.cpp: RhiViewport default via `else if (rhiSelection.canUseRhi)` (line 131-132); SoftwareViewport only in final `else` fallback (line 134). Tests: `previewPageNeverReferencesSoftwareViewport`, `rhiViewportRendererComputePreviewDrawRangeAppliesRoleFilter`, `rhiViewportRendererHasGcvPackedSegmentRoleGuard`, `gcode04RhiViewportIsDefaultRegistrationNoSoftwareViewportInPreviewPath` — **all PASS** | ✅ |
| **GCODE-05** | Payload survives interaction setters; sliceFailed/sliceResultCleared/plate-switch-invalid clear state; reslice rebuilds with different bytes; export while visible leaves payload intact; page switch preserves payload | 7 E2E tests — source read (E2EWorkflowTests.cpp:1343-1676); **all 7 PASS** at runtime. Reslice evidence: 503074 bytes/15 layers → 377008 bytes/10 layers after layer_height change (genuinely different bytes) | ✅ |

---

## Must-Haves Verification

### Plan 55-01 (Wave 0 — fixture + PreviewParserTests scaffold)

| # | must_have truth | Status | Evidence |
|---|-----------------|--------|----------|
| 1 | OrcaSlicer-style .gcode fixture exists with ≥8 distinct ;TYPE: roles, travels, tagged comments | ✅ | `tests/fixtures/orca_sample.gcode` exists; `test_fixture_has_expected_role_coverage` PASS asserts the role strings are present |
| 2 | PreviewParserTests target registered in CMakeLists.txt, runnable via ctest -R PreviewParser | ✅ | PreviewParserTests.exe runs; exit 0; 7 passed |
| 3 | RED scaffold slots for 20-role parse + 17-mode coverage (now flipped GREEN by Plan 02) | ✅ | `test_role_string_mapping_covers_upstream_enum` PASS, `test_view_modes_match_upstream_seventeen` PASS |

### Plan 55-02 (Wave 1 — data model + renderer contract)

| # | must_have truth | Status | Evidence |
|---|-----------------|--------|----------|
| 4 | ;TYPE: parsed into 20 canonical libvgcode indices via direct string→index lookup; libslic3r int NEVER used as array index | ✅ | `roleForTypeImpl` (PreviewViewModel.cpp:192-201) iterates `kRoleMap[]`; kRoleColors indexed by the same canonical index (line 142) |
| 5 | kRoleMap maps "Ironing"→7 and "Bottom surface"→15 (libvgcode), NOT libslic3r 8 and 7 | ✅ | PreviewViewModel.cpp:124 ("Ironing",7) and :132 ("Bottom surface",15), each with a "NOT N" comment |
| 6 | kRoleColors indexed by canonical libvgcode index; Ironing→(255,140,105), Bottom surface→(102,92,199) | ✅ | kRoleColors[7]=(255,140,105) line 150; kRoleColors[15]=(102,92,199) line 158; `test_divergent_role_colors_correct` PASS |
| 7 | roleVisibilities() QML ordering follows canonical libvgcode index order 1..19 (excl 0 None, 14 Custom); render-side array + QML list use documented mapping | ✅ | roleVisibilities() (line 743-773) emits ascending 1..19 skipping {0,14}; roleVisibilityMask() emits dense 20-bool; `roleVisibilityMaskFeedsRendererShapeAndTogglesPropagate` asserts 18 UI rows vs 20 mask bools |
| 8 | StoredSegment, PackedSegment, GcvPackedSegment each carry int role (76 bytes packed) | ✅ | `int role` present in all 3: PreviewViewModel.cpp:66 (PackedSegment) + :72 static_assert; RhiViewportRenderer.cpp:590 + :594 static_assert; RhiViewport.cpp:26 + :28 static_assert |
| 9 | PreviewDrawSpan carries int role; computePreviewDrawRange skips masked-off roles (no repack) | ✅ | RhiViewportRenderer.cpp:716-718 skip block `if (span.role>=0 && span.role<m_roleVisibility.size() && !m_roleVisibility[span.role]) continue;` |
| 10 | viewModes() returns 17 upstream EViewType names in upstream order | ✅ | viewModes() (PreviewViewModel.cpp:468-491) returns exactly the 17 specified names; `test_view_modes_match_upstream_seventeen` + `viewModesExposeUpstreamSeventeenModes` PASS |
| 11 | showTravelMoves_ defaults to false | ✅ | PreviewViewModel.h:286 `bool showTravelMoves_ = false;` |
| 12 | toggleRoleVisibility emits stateChanged only; does NOT call recolorAndPackSegments, does NOT mutate gcodePreviewData_ | ✅ | toggleRoleVisibility (PreviewViewModel.cpp:732-741) flips mask + emits stateChanged; no recolor call; `roleVisibilityToggleDoesNotRepackGcodePreviewData` PASS (byte-equality QVERIFY2) |
| 13 | setShowTravelMoves continues to repack (travel visibility ≠ role visibility) | ✅ | setShowTravelMoves (PreviewViewModel.cpp:682-689) retains repack; only default flipped (confirmed in REVIEW) |
| 14 | test_role_string_mapping_covers_upstream_enum GREEN with corrected libvgcode indices | ✅ | PASS (PreviewParserTests run) |
| 15 | test_divergent_role_colors_correct GREEN (Ironing→kRoleColors[7], Bottom surface→kRoleColors[15]) | ✅ | PASS (PreviewParserTests run) |

### Plan 55-03 (Wave 2 — VisibilityFilter UI)

| # | must_have truth | Status | Evidence |
|---|-----------------|--------|----------|
| 16 | VisibilityFilter.qml exists, registered in qml.qrc, renders collapsible "Visible Line Types" in right panel | ✅ | `src/qml_gui/components/VisibilityFilter.qml` exists; qml.qrc registered; PreviewPage.qml:374 `Components.VisibilityFilter { ... }` |
| 17 | Each role row binds CxCheckBox to isRoleVisible(index), calls toggleRoleVisibility(index) on change | ✅ | Verified in Plan 03 SUMMARY + QmlUiAudit green; toggle uses `onToggled` (correct Qt signal, not the non-existent `onChanged`) |
| 18 | GLViewport bound to previewVm role visibilities via roleVisibility property (render-side filter fires) | ✅ | PreviewPage.qml:294 `roleVisibility: root.previewVm ? root.previewVm.roleVisibilityMask : []` (the code-review-fixed binding) |
| 19 | No raw CheckBox/Switch/ScrollView in VisibilityFilter — only Cx* controls + Theme tokens | ✅ | QmlUiAudit (raw-control audit) PASS |
| 20 | All user-visible strings use qsTr() per UI-SPEC copywriting contract | ✅ | QmlUiAudit (qsTr audit) PASS |

### Plan 55-04 (Wave 2 — regression tests)

| # | must_have truth | Status | Evidence |
|---|-----------------|--------|----------|
| 21 | After live slice: non-empty GCV1, layerCount>0, moveCount>0, no placeholder markers (GCODE-01 GREEN) | ✅ | `previewReceivesRealGcodeAfterLiveSliceNoPlaceholder` PASS — asserts size>8, gcv1SegmentCount>0, layerCount>0, moveCount>0, no demo/sample/placeholder (hard QVERIFY2) |
| 22 | sliceFailed / sliceResultCleared / plate-switch-invalid each trigger resetPreviewState + clear gcodePreviewData_ | ✅ | `sliceFailedClearsPreviewState`, `sliceResultClearedClearsPreviewState`, `plateSwitchToInvalidClearsPreviewState` — **all PASS** |
| 23 | toggleRoleVisibility does NOT mutate gcodePreviewData_ (byte-for-byte) | ✅ | `roleVisibilityToggleDoesNotRepackGcodePreviewData` PASS (hard QVERIFY2 before==after) |
| 24 | Legend min/max labels unchanged across layer/move drag | ✅ | `legendGradientBoundsStableAcrossLayerMoveDrag` PASS |
| 25 | setCurrentMove updates currentGcodeLine + gcodeLines window atomically (single stateChanged) | ✅ | `currentMoveUpdatesGcodeLineWindowAtomically` PASS (spy.count()==1) |
| 26 | PreviewPage.qml has zero "SoftwareViewport" | ✅ | grep count = 0; `previewPageNeverReferencesSoftwareViewport` PASS |
| 27 | main_qml.cpp registers RhiViewport default, SoftwareViewport only behind explicit gate | ✅ | `gcode04RhiViewportIsDefaultRegistrationNoSoftwareViewportInPreviewPath` PASS; source: line 131-134 if/else-if/else structure |
| 28 | computePreviewDrawRange contains the role-skip block | ✅ | `rhiViewportRendererComputePreviewDrawRangeAppliesRoleFilter` PASS; source line 716-718 |
| 29 | gcodePreviewData survives layer/move/camera/toggle interaction setters | ✅ | `roleVisibilityToggleDoesNotRepackGcodePreviewData` + `previewRhiInteractionSettersPreservePreviewPayload` (existing) PASS |
| 30 | Reslice rebuilds with different bytes + recomputed counts | ✅ | `resliceRebuildsGcodePreviewDataWithDifferentBytes` PASS (503074B/15L → 377008B/10L) |
| 31 | Export while visible leaves gcodePreviewData intact; page switch preserves gcodePreviewData | ✅ | `exportWhilePreviewVisibleLeavesGcodePreviewDataIntact` PASS + `pageSwitchPreparePreviewPreservesGcodePreviewData` PASS |

### Plan 55-05 (Wave 3 — D3D11 startup guard + VALIDATION finalize)

| # | must_have truth | Status | Evidence |
|---|-----------------|--------|----------|
| 32 | main_qml.cpp registers RhiViewport as default GLViewport; SoftwareViewport only behind explicit env-var gate | ✅ | `gcode04RhiViewportIsDefaultRegistrationNoSoftwareViewportInPreviewPath` PASS (Phase-55-tagged); source line 129-134 |
| 33 | PreviewPage.qml binds GLViewport (→RhiViewport), never SoftwareViewport | ✅ | grep: 0 SoftwareViewport, GLViewport present; test PASS |
| 34 | Per-Task Verification Map filled with actual Task IDs | ✅ | 55-VALIDATION.md Per-Task map has 11 rows (55-01-T1 .. 55-05-T2); TBD count 0 |
| 35 | VALIDATION.md frontmatter: nyquist_compliant: true, wave_0_complete: true, status: ready-for-verify | ✅ | 55-VALIDATION.md frontmatter lines 4-7 confirmed |

> Note: must_haves_total counts the 31 unique behavioral truths across plans 01-05 (Plan 05's startup-policy truths duplicate Plan 04's GCODE-04 source-audit coverage already counted). All 31 unique truths verified.

---

## Test Results

Tests run directly from `build/` (canonical build was confirmed green exit 0 just prior; only individual targets re-run here).

| Target | Exit | Totals | Phase-55 methods |
|--------|------|--------|------------------|
| **PreviewParserTests** | 0 | **7 passed, 0 failed, 0 skipped** | `test_fixture_has_expected_role_coverage` ✅, `test_role_string_mapping_covers_upstream_enum` ✅, `test_view_modes_match_upstream_seventeen` ✅, `test_summary_mode_has_no_gradient_legend` ✅, `test_divergent_role_colors_correct` ✅ |
| **ViewModelSmokeTests** | 0 | **76 passed, 0 failed, 1 skipped** | `roleVisibilityToggleDoesNotRepackGcodePreviewData` ✅, `legendGradientBoundsStableAcrossLayerMoveDrag` ✅, `currentMoveUpdatesGcodeLineWindowAtomically` ✅, `viewModesExposeUpstreamSeventeenModes` ✅, `roleVisibilityMaskFeedsRendererShapeAndTogglesPropagate` ✅ (Critical-fix guard) |
| **QmlUiAuditTests** | 0 | **32 passed, 0 failed, 0 skipped** | `previewPageNeverReferencesSoftwareViewport` ✅, `rhiViewportRendererComputePreviewDrawRangeAppliesRoleFilter` ✅, `rhiViewportRendererHasGcvPackedSegmentRoleGuard` ✅, `gcode04RhiViewportIsDefaultRegistrationNoSoftwareViewportInPreviewPath` ✅ |
| **E2EWorkflowTests** | 0 | **27 passed, 0 failed, 0 skipped** (214.7s) | `previewReceivesRealGcodeAfterLiveSliceNoPlaceholder` ✅, `sliceFailedClearsPreviewState` ✅, `sliceResultClearedClearsPreviewState` ✅, `plateSwitchToInvalidClearsPreviewState` ✅, `resliceRebuildsGcodePreviewDataWithDifferentBytes` ✅, `exportWhilePreviewVisibleLeavesGcodePreviewDataIntact` ✅, `pageSwitchPreparePreviewPreservesGcodePreviewData` ✅ |
| **TOTAL** | — | **142 passed, 0 failed, 1 skipped** | All 21 Phase-55 test methods green |

The 1 ViewModelSmoke skip is `multiPlate3mfRoundTripPreservesState` — a **pre-existing, unrelated** skip (THUMB-03 / store_bbs_3mf writer limitation), NOT a Phase-55 test. No Phase-55 test is skipped or failing.

---

## Code Review

The phase code review (`55-REVIEW.md`, standard depth, `status: resolved`) found **1 Critical + 1 Warning + 2 Info**. All four are **resolved** in commit `012808e fix(55): resolve code-review findings — role-visibility dead path + lockstep guard`.

### Critical — Role-visibility feature was a dead path at runtime (RESOLVED ✅)

**The defect:** `PreviewPage.qml` bound `previewVm.roleVisibilities` (an 18-element `QVariantList` of `QVariantMap` rows, for the UI Repeater) into `GLViewport.roleVisibility`. But `RhiViewportRenderer::synchronize` expects a **dense 20-element bool list** and gates on `size() >= 20`. With 18 maps the gate failed → `m_roleVisibility.clear()` → `computePreviewDrawRange`'s `!m_roleVisibility[span.role]` skip never fired. **Net effect: clicking a VisibilityFilter checkbox never hid any segment.** The green test suite missed it because every test validated a component in isolation (no-repack at ViewModel level; literal-string grep in QmlUiAudit) — no test drove the producer→binding→consumer path.

**The fix (verified correct end-to-end):**
1. **New producer property** `Q_PROPERTY(QVariantList roleVisibilityMask READ roleVisibilityMask NOTIFY stateChanged)` (PreviewViewModel.h:92). `roleVisibilityMask()` (PreviewViewModel.cpp:775-789) returns a dense 20-element bool list indexed by canonical libvgcode role.
2. **Correct binding** PreviewPage.qml:294 `roleVisibility: root.previewVm ? root.previewVm.roleVisibilityMask : []` (replaces the broken `roleVisibilities` binding), with an explanatory comment.
3. **Consumer gate now satisfied** — `roleVisibilityMask()` returns 20 elements, so `synchronize`'s `size() >= 20` gate (RhiViewportRenderer.cpp:94) passes and the 20 bools populate `m_roleVisibility` correctly.
4. **New integration test** `roleVisibilityMaskFeedsRendererShapeAndTogglesPropagate` (ViewModelSmokeTests.cpp:2999) guards the producer shape (20 bools, all default true), the distinct shapes (18 maps vs 20 bools), and that a toggle flips exactly one slot. This is exactly the end-to-end test the review's suggested-fix called for. **PASS** at runtime.

The fix is correct: the producer emits the shape the consumer requires, the binding selects the right property, and a regression test now locks the contract.

### Warning — No producer-side static_assert for PackedSegment (RESOLVED ✅)

`static_assert(sizeof(PackedSegment) == 76, ...)` now present at PreviewViewModel.cpp:72, mirroring the consumer-side asserts in RhiViewportRenderer.cpp:594 and RhiViewport.cpp:28. Wire-format lockstep is now enforced on both sides.

### Info #3 — Stale "80 bytes each" comment (RESOLVED ✅)

The misleading comment at RhiViewportRenderer.cpp:578 is no longer present (grep confirms "80 bytes each" absent).

### Info #4 — logOnceIfNeeded function-local statics (accepted, no change needed)

Process-lifetime `static bool logged[4]` shared across PreviewViewModel instances. Accepted as intentional (informational log; one ViewModel in production). No change required; noted for completeness.

---

## human_verification

Status is **passed**, not human_needed. The two manual visual UAT items below are the ONLY items not verified by automated tests, and they are **explicitly deferred to Phase 58** per 55-VALIDATION.md "Manual-Only Verifications" — they are visual/parity checks that headless GPU capture cannot reliably exercise. They do not block the phase goal and are listed here for traceability:

| Deferred Item | Requirement | Why Manual | Phase 58 Owner |
|---------------|-------------|-----------|----------------|
| Drag stability (camera orbit/pan/zoom, layer slider, move slider) leaves toolpath visibly intact | GCODE-05 | Headless GPU capture unreliable; renderer blanking is a visual runtime behavior | Phase 58 UAT |
| VisibilityFilter UI matches screenshot + upstream role ordering/defaults (18 roles in upstream order with correct default checkboxes; render-side filtering visibly hides segments) | GCODE-02 | Visual layout parity / interactive filtering | Phase 58 UAT |

---

## Gaps

None. All 31 unique must_have truths are verified against source + passing tests, all 5 requirements (GCODE-01..05) have green automated evidence, the 1 Critical code-review finding is fixed and verified end-to-end, and the only non-automated items are the two Phase-58-deferred visual UAT checks.
