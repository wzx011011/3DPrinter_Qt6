---
phase: 57-deprecated-ui-removal-and-architecture-cleanup
plan: 02
subsystem: testing, docs
tags: [cleanup, regression-test, encoding-guard, source-truth, qml-audit]
requires:
  - phase: 57-deprecated-ui-removal-and-architecture-cleanup
    provides: "Wave 1 deletions (7 QML files + 3 routes + dead machinery) that this plan's regression test locks in"
provides:
  - "2 rewritten source-truth docs (源码真值功能矩阵.md, 源码真值基线.md) with CrealityPrint paths rewritten to OrcaSlicer"
  - "2 new QmlUiAudit regression tests (deletedSettingsPathsStayAbsent, deletedRoutesStayAbsent) locking Wave 1 deletions as a permanent ctest invariant"
  - "Documented broader-UI sweep proving no other dead UI lingers beyond the locked cleanup list"
  - "Clean encoding-guard pass on all phase-touched files (qml.qrc BOM stripped)"
affects:
  - "Future phases that touch BackendContext/ConfigViewModel: the deletedRoutesStayAbsent test will fail CI if any of the 6 dead tokens (canLeaveSettingsPage, requestConfigPageExitIfNeeded, executeDeferredConfigExit, clearDeferredConfigExit, requestLeaveSettingsPage, leave-settings-page) is reintroduced"
  - "Future phases that touch qml.qrc: the deletedSettingsPathsStayAbsent test will fail CI if any of the 7 deleted QML paths is re-added"
tech-stack:
  added: []
  patterns:
    - "Compiled regression test as permanent deletion invariant (extends QmlUiAuditTests; readFile via QT_TESTCASE_SOURCEDIR repo-root resolution)"
    - "Source-truth doc path-rewrite discipline (preserve historical project-name mentions as standalone notes; rewrite path-prefix occurrences)"

key-files:
  created:
    - .planning/phases/57-deprecated-ui-removal-and-architecture-cleanup/57-02-SUMMARY.md
    - .planning/phases/57-deprecated-ui-removal-and-architecture-cleanup/deferred-items.md
  modified:
    - docs/源码真值功能矩阵.md
    - docs/源码真值基线.md
    - tests/QmlUiAuditTests.cpp
    - src/qml_gui/qml.qrc
key-decisions:
  - "Treated the docs/ directory .gitignore as authoritative for the 2 source-truth docs: force-added them alongside the 2 already-tracked docs (CrealityPrint_Qt_GUI重写架构.md, v3.6-ui-inventory.md), so the rewrite is captured in version control."
  - "Stripped the pre-existing UTF-8 BOM from qml.qrc even though it predated Phase 57 — CLEAN-04 covers qml.qrc as a Wave 1 file (57-01 removed 7 entries from it), and the global encoding guard mandates BOM removal."
  - "Did NOT remediate the pre-existing mojibake (GBK-as-UTF-8) in BackendContext.h / ConfigViewModel.cpp Chinese comments — confirmed it predates Phase 57 via git show 264413c:<path>, it's outside CLEAN-04's phase-touched-files scope, and remediation would require rewriting dozens of comment lines across many files. Logged in deferred-items.md for a future encoding-normalization phase."
patterns-established:
  - "Deletion becomes invariant: when a future phase removes user-visible surface (QML files, named routes), follow up with a QmlUiAudit source-grep test asserting the deleted paths/tokens stay absent, so a regression fails CI deterministically."
requirements-completed: [CLEAN-02, CLEAN-03, CLEAN-04]

duration: ~55min
completed: 2026-07-03
---

# Phase 57 Plan 02: Deprecated UI Removal — Doc Rewrite + Regression Audit + Encoding Cleanup Summary

Rewrote the 2 locked source-truth docs (CrealityPrint paths → OrcaSlicer), added 2 compiled QmlUiAudit regression tests that lock the Wave 1 deletions as a permanent ctest invariant (38 passed, was 36), performed the broader-UI sweep proving no other dead UI lingers, and closed CLEAN-04 with a clean encoding-guard pass on all phase-touched files.

## Performance

- **Duration:** ~55 min
- **Tasks:** 4 (all complete)
- **Files modified:** 4 (2 docs, 1 test file, 1 qml.qrc BOM strip)
- **Files created:** 2 (SUMMARY.md, deferred-items.md)

## Accomplishments
- 2 source-truth docs rewritten: `源码真值功能矩阵.md` (21 path occurrences + title + bare repo path) and `源码真值基线.md` (title + repo path + version.inc macro). Both have ZERO `third_party/CrealityPrint/` occurrences.
- 2 new QmlUiAudit regression tests compile and pass on the post-57-01 tree: `deletedSettingsPathsStayAbsent` (asserts qml.qrc cleanliness + on-disk absence for 7 deleted QML files) and `deletedRoutesStayAbsent` (asserts BackendContext.{h,cpp} + ConfigViewModel.{h,cpp} do not contain any of 6 dead tokens). QmlUiAuditTests now reports 38 passed (was 36).
- Broader-UI sweep documented: Pattern 1 (`old*.qml` files) returns 0 hits; Pattern 2 (`legacy|deprecated|unused|placeholder-only|disconnected`) triaged into 5 out-of-scope categories (Q_UNUSED C++ markers, Disconnected MQTT/Camera state, live tier-normalization aliases, ConfigOptionModel live fallback, FilamentSlot honest-UI stub locked by Phase 52 audit test); Pattern 3 (stale Phase-50-56 TODOs) found only the FilamentSlot honest-UI stub which is locked by an existing audit test.
- Encoding guard closed: stripped pre-existing UTF-8 BOM from qml.qrc (only the 3-byte prefix removed, content byte-identical). Canonical `auto_verify_with_vcvars.ps1 -ExitOnBuildFailure` exits 0; all 5 scene suites green (PrepareScene, PartPlate, ViewModelSmokeTests, QmlUiAuditTests, PreviewParser).

## Task Commits

1. **Task 1: Rewrite 2 locked doc paths (CrealityPrint → OrcaSlicer)** - `af5131e` (docs)
2. **Task 2: Add QmlUiAudit regression tests locking Wave 1 deletions** - `ceeae34` (test) — tdd task; the RED state was the absence of these tests, GREEN achieved because 57-01 already deleted the locked items.
3. **Task 3: Broader dead-UI sweep** - no commit (SUMMARY-only; no newly-found dead UI to delete).
4. **Task 4: Encoding guard + canonical build green** - `c12c1f2` (chore: strip pre-existing BOM from qml.qrc).

**Plan metadata:** pending (final metadata commit follows this SUMMARY).

## Files Created/Modified
- `docs/源码真值功能矩阵.md` - 21 `third_party/CrealityPrint/` paths + title + bare repo path rewritten to OrcaSlicer; the source-of-truth inventory for upstream module → Qt6 anchor mapping.
- `docs/源码真值基线.md` - Title, repo path, and `version.inc` macro renamed (CREALITYPRINT_VERSION → ORCASLICER_VERSION); historical project-name note preserved as a standalone mention in the update line.
- `tests/QmlUiAuditTests.cpp` - Added `<QFileInfo>` include; declared 2 new private slots (`deletedSettingsPathsStayAbsent`, `deletedRoutesStayAbsent`); implemented both bodies reusing the `readSource()` helper. Net +82 lines.
- `src/qml_gui/qml.qrc` - Pre-existing UTF-8 BOM stripped (1 line changed, content byte-identical otherwise).

## Decisions Made
- **Force-add the gitignored docs**: `docs/` is gitignored (`.gitignore:61`), but the 2 source-truth docs were explicitly listed as the plan's locked cleanup-clause targets. Force-added them so the rewrite is captured in version control, matching the pattern of the 2 already-tracked docs.
- **Reuse `readSource()` helper**: instead of inline `QFile + QTextStream` (the plan's pseudocode), reused the existing `readSource()` helper that resolves via `QT_TESTCASE_SOURCEDIR` (the repo root). This matches the established Phase 55/56 audit-test pattern and avoids adding a `<QTextStream>` include dependency.
- **Scope the encoding-guard remediation**: BOM strip = in-scope (qml.qrc is a CLEAN-04 phase-touched file); mojibake remediation = out-of-scope (predates Phase 57, in untouched comment lines, would require rewriting dozens of lines). Logged to deferred-items.md.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Stripped pre-existing UTF-8 BOM from qml.qrc**
- **Found during:** Task 4 (encoding-guard pass)
- **Issue:** `encoding_guard.py` flagged `src/qml_gui/qml.qrc` as having a UTF-8 BOM. Verified via `git show 264413c:src/qml_gui/qml.qrc | head -c 3 | xxd` (BOM `efbbbf`) that it predated Phase 57, but qml.qrc is a Wave 1 file (57-01 removed 7 `<file>` entries), so it falls under CLEAN-04's encoding-cleanup scope.
- **Fix:** Ran `encoding_guard.py --fix src/qml_gui/qml.qrc`. Only the 3-byte prefix was removed; content byte-identical otherwise (`git diff --stat` shows 1 line, just the BOM).
- **Files modified:** src/qml_gui/qml.qrc
- **Verification:** `encoding_guard.py src/qml_gui/qml.qrc` → `encoding_guard ok`; canonical build green.
- **Committed in:** c12c1f2

**2. [Rule 3 - Blocking] docs/ is gitignored — force-added the 2 rewritten docs**
- **Found during:** Task 1 commit
- **Issue:** `git add docs/源码真值*.md` was rejected because `.gitignore:61` ignores `docs/`. The plan's frontmatter lists these 2 files as `files_modified`, so the deliverable must be in version control.
- **Fix:** Used `git add -f` to force-track the 2 docs, matching the pattern of the 2 already-tracked docs (`CrealityPrint_Qt_GUI重写架构.md`, `v3.6-ui-inventory.md`).
- **Files modified:** (index only — docs now tracked)
- **Verification:** `git ls-files docs/` returns 4 entries (was 2).
- **Committed in:** af5131e

---

**Total deviations:** 2 auto-fixed (1 bug, 1 blocking)
**Impact on plan:** Both auto-fixes necessary for the locked cleanup clause and CLEAN-04. No scope creep.

## Phase 57+ Deferred UI Findings

Task 3 broader-UI sweep. No newly-found dead UI to delete (the locked list of 7 was complete). Out-of-scope findings logged here and in `deferred-items.md`:

| Finding | Location | Classification | Reason |
|---|---|---|---|
| `old*.qml` Pattern-1 grep | src/qml_gui/ | clean (0 hits) | No old-prefixed QML files exist anywhere |
| `Q_UNUSED(...)` markers | BackendContext.cpp, RhiViewport.cpp, ProjectServiceMock.cpp, etc. (~40 hits) | out-of-scope | C++ compiler noise, not dead UI (plan-explicit) |
| `Disconnected` state enum | CameraServiceMock.h, MonitorViewModel.h, MqttClient.h | out-of-scope | Live runtime state, not dead UI |
| `legacy category-derived fallback` comment | ConfigOptionModel.cpp:481 | out-of-scope | Live fallback logic (plan-explicit) |
| `Accept both new tier strings ... and legacy aliases` | ConfigViewModel.cpp:118, 1175 (normalizedTier) | out-of-scope | Live tier-normalization for backward-compat (`machine`/`process` aliases accepted) |
| `Legacy: return print category presets` | PresetServiceMock.cpp:711 (presetNames) | out-of-scope | Live API called by ConfigViewModel::presetNames |
| `stub is retained for any legacy caller` | BackendContext.cpp:425-432 (openSettings) | deferred | Misleading comment — method is LIVE (5 QML callers); comment rewording is a C++ cleanup, not CLEAN-02 UI scope |
| `TODO(Phase 56): wire colorPickerLoader` | FilamentSlot.qml:57 | out-of-scope | Live honest-UI stub, locked by existing QmlUiAudit test `leftSidebarPresetControlsAreWiredAndHonest` (asserts the TODO is present) |
| Pre-existing mojibake (GBK-as-UTF-8) | BackendContext.h:37,73,183,345,499; ConfigViewModel.cpp:490,1105,1410 | deferred | Predates Phase 57 (verified via git show 264413c); outside CLEAN-04 scope; remediation = its own dedicated phase |

## Known Stubs

None. This plan does not introduce any stubbed data paths. The FilamentSlot.qml honest-UI stub documented above is a pre-existing Phase 52 contract, not introduced here.

## Threat Flags

None. The phase touches no network, auth, storage, IPC, or external-input surface (per the plan's threat model, T-57-05/06 mitigated by the doc-rewrite discipline and the compiled regression test; T-57-07/SC accepted — no PII/secrets, no package installs).

## Verification

- Task 1: `! grep -q "third_party/CrealityPrint/" docs/源码真值*.md` prints `PASS_DOC_REWRITE` (0 occurrences in both docs).
- Task 2: `cd build && ./QmlUiAuditTests.exe -o r.txt,txt` → `Totals: 38 passed, 0 failed, 0 skipped, 0 blacklisted, 24ms`. Both new tests visible in output (`PASS   : QmlUiAuditTests::deletedSettingsPathsStayAbsent()`, `PASS   : QmlUiAuditTests::deletedRoutesStayAbsent()`).
- Task 3: `grep -rln --include="*.qml" -E "old[A-Z].*(Page|View|Panel|Dialog|Sidebar|Bar)\.qml" src/qml_gui/` returns 0 lines.
- Task 4: `encoding_guard.py` on the 13 phase-touched files → `encoding_guard ok` (after BOM strip). Canonical `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1 -ExitOnBuildFailure` exits 0 with all 5 scene suites green.

## Self-Check: PASSED

- `git log --oneline -5` shows all 3 task commits: `c12c1f2` (Task 4 chore), `ceeae34` (Task 2 test), `af5131e` (Task 1 docs).
- `git ls-files docs/` shows 4 entries (was 2) — the 2 source-truth docs are now tracked.
- `head -c 3 src/qml_gui/qml.qrc | xxd` returns `3c52 43` (`<RC`) — BOM stripped.
- `grep -c "third_party/CrealityPrint" docs/源码真值*.md` returns 0 for both files.
- `QmlUiAuditTests.exe` reports 38/0/0 (was 36/0/0) — both new regression tests compiled and green.

## Phase 57 Closeout

With 57-02 complete, Phase 57 closes with all 4 CLEAN requirements delivered:
- **CLEAN-01**: 4 off-design Settings files + 3 legacy sidebar panels + 3 named routes + dead deferred-config-exit machinery removed (Wave 1, 57-01). Locked as a permanent invariant by the 57-02 regression test.
- **CLEAN-02**: locked cleanup complete + broader-UI sweep documented with 0 hits for `old*.qml` and all `legacy|deprecated|unused` hits triaged into out-of-scope categories (this plan).
- **CLEAN-03**: no business logic leaked into QML in the changed areas (the deleted files carried no behavior; the replacement SettingsDialog surface was already audited in Phase 56-03).
- **CLEAN-04**: encoding guard clean on all phase-touched files (qml.qrc BOM stripped; pre-existing mojibake deferred with rationale).

---

*Phase: 57-deprecated-ui-removal-and-architecture-cleanup*
*Plan: 02*
*Completed: 2026-07-03*
