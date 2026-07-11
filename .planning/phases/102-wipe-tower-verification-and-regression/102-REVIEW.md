---
phase: 102
title: Wipe-Tower Verification And Regression
milestone: v4.4
status: clean
files_reviewed:
  - tests/QmlUiAuditTests.cpp
base_commit: ca763bf
head_commit: ee9d703
critical: 0
warning: 0
info: 3
total: 3
ctest: 4/4 passed (PrepareSceneDataTests, PartPlateTests, ViewModelSmokeTests, QmlUiAuditTests incl. new slot)
build: OWzxSlicer.exe compiled/linked clean
launch: OWzxSlicer.exe PID 34240, 5-second no-crash liveness confirmed
---

# Phase 102 Code Review

Scope: the change introduced by `88a4504` (test(102-01): consolidate 8 WT-* source-audit anchors into QmlUiAuditTests) against base `ca763bf`. Only one source file changed: `tests/QmlUiAuditTests.cpp` (declaration at :153-160, implementation at :3506-3622, 117 new lines).

## Findings Table

| ID | Severity | Location | Summary |
|----|----------|----------|---------|
| 102-R1 | info | QmlUiAuditTests.cpp:3523,3530 | `editorVmHeader` (EditorViewModel.h) is loaded and non-empty-checked but no anchor assertion uses it — dead load. |
| 102-R2 | info | QmlUiAuditTests.cpp:3613-3621 | WT-SOFTWARE-VIEWPORT comment claims the lock covers all 5 dim members (`m_wipeTowerWidth/Depth/Height/X/Z`) but only 2 anchors are asserted (`m_showWipeTower`, `m_wipeTowerWidth`). |
| 102-R3 | info | QmlUiAuditTests.cpp:3536-3552 | WT-VIEWPORT-DEFAULTS uses whitespace-sensitive partial-substring anchors for 6 QML bindings — brittle to reformatting, but explicitly documented as intentional and matches Phase 101 precedent. |

Status: **clean** for merge purpose (no critical/warning). All 3 findings are info-level observations a future tightening pass could pick up; none block the regression lock from doing its job in v4.4.

## Verification Performed

### Test correctness (QFile + QT_TESTCASE_SOURCEDIR)
The new slot reuses the file-local `readSource()` helper which resolves files via `QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR)).filePath(relativePath)`. Deterministic, build-dir-independent — the same pattern used by every other slot in this file (Phase 95/96 siblings). All 7 relative paths resolve correctly. Each load is guarded by `QVERIFY2(!...isEmpty(), ...)` so a missing file fails loudly rather than silently passing `.contains()` checks against an empty string.

### Anchor accuracy (8 WT-* regions verified against source)
All 29 QVERIFY2 assertions cross-checked against the live source tree. Every asserted substring present byte-for-byte:

- **WT-VIEWPORT-DEFAULTS** (6 anchors) — PreparePage.qml:1670-1675 has all 6 bindings.
- **WT-PRINT-DATA** (2 anchors) — SliceService.cpp:647 (`print.wipe_tower_data()`), :642 (`print.has_wipe_tower()`).
- **WT-READBACK-POINT** (3 anchors) — SliceService.cpp:590 (`print.process()`), :664/668/673 (`activePrint_.store(nullptr`), :648 (`capturedGeometry.valid = true`). Ordering claim correct.
- **WT-HAS-WIPE-GATE** (3 anchors) — SliceService.cpp:642, EditorViewModel.cpp:5103 (`m_showWipeTower = true`), :5108 (`m_showWipeTower = false`).
- **WT-PLACEHOLDER-BOX** (1 anchor) — GizmoGeometry.cpp:488.
- **WT-RENDERER-BUFFER** (1 anchor) — RhiViewportRenderer.cpp:1075 inside `uploadWipeTowerBuffer`.
- **WT-RENDER-UPGRADE** (3 anchors) — GizmoGeometry.cpp:451 (`Option A`), :458 (`load_wipe_tower_preview`), :473/474/480/484/485 (`Option B`).
- **WT-SOFTWARE-VIEWPORT** (2 anchors) — SoftwareViewport.cpp:209/211/456 (`m_showWipeTower`), :217/219/456/461 (`m_wipeTowerWidth`).

### Region naming (gap-matrix coverage)
The 8 region tokens in the QVERIFY2 messages are an exact 8-for-8 match against `99-GAP-MATRIX.md`. Every failure path is attributable to a specific gap-matrix row — the stated goal of the slot.

### Determinism
Confirmed: the slot body contains no `QQmlEngine`, `QSignalSpy`, `QEventLoop`, `exec()`, `processEvents`. Pure file-I/O + `QString::contains`. The 4/4 ctest pass is consistent.

## Detail Sections

### 102-R1 (info): `editorVmHeader` is loaded but never asserted
At :3523 the slot loads `EditorViewModel.h` into `editorVmHeader`, and at :3530 it checks non-empty — but no subsequent `editorVmHeader.contains(...)` call appears. The `.cpp` sibling (`editorVmSource`) IS asserted (:3581, :3583). A refactor that deleted the 6 Q_PROPERTYs from the header would break QML binding at runtime but pass this lock. The non-empty check at :3530 is a loadability sentinel only — marginal value. Optional tightening: either drop the load + check as dead, or add a `Q_PROPERTY.*wipeTowerWidth` anchor to lock the header contract. Non-blocking.

### 102-R2 (info): WT-SOFTWARE-VIEWPORT comment/anchor coverage mismatch
Comment at :3614-3617 claims the lock verifies the software path consumes `m_showWipeTower` plus "the dim members (`m_wipeTowerWidth/Depth/Height/X/Z`)". Actual source (SoftwareViewport.cpp:456-468) does consume all five at paint. But only 2 anchors are asserted (:3618-3621): `m_showWipeTower` and `m_wipeTowerWidth`. Impact: low (`m_wipeTowerWidth` is representative; file small enough that any refactor touching one dim likely touches them all). Either trim the comment to match (state "representative anchor") or add the 4 missing anchors to match the comment. Non-blocking.

### 102-R3 (info): Whitespace-sensitivity of WT-VIEWPORT-DEFAULTS anchors
Each of the 6 QML-binding anchors is a partial substring ending mid-expression. Intentionally whitespace-sensitive: a reformat that changed the binding layout would trip the lock. The comment explicitly acknowledges this. Matches Phase 101 ViewModelSmokeTests precedent. Acceptable for a regression lock whose purpose is to resist silent refactors. No action needed.

## Recommendation

**Status: clean (mergeable).** No critical or warning issues. The 3 info-level observations are optional tightening candidates for a future audit cycle. The 4/4 regression ctest pass + clean OWzxSlicer.exe compile/link + 5-second launch liveness corroborate the source-level verification.

Full report: `.planning/phases/102-wipe-tower-verification-and-regression/102-REVIEW.md`
