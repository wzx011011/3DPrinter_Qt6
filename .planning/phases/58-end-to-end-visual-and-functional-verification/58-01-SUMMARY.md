---
phase: 58-end-to-end-visual-and-functional-verification
plan: 01
subsystem: testing
tags: [inventory-audit, regression-guard, qt-test, ctest, verify-01]

requires:
  - phase: 50-screenshot-and-source-truth-inventory
    provides: "the 9 section 2 deterministic inventory checks (lines 170-286) + the frozen 50-INVENTORY.md snapshot"
provides:
  - "tests/InventoryAuditTests.cpp — Qt Test target encoding the 9 Phase 50 section 2 checks as a permanent ctest-level regression guard (VERIFY-01)"
  - "CMakeLists.txt registration mirroring the QmlUiAuditTests block (qt_add_executable + Qt6::Test + QT_TESTCASE_SOURCEDIR + add_test + ENVIRONMENT)"
affects: [v3.6-milestone-closeout, future-inventory-edits, future-screenshot-milestones]

tech-stack:
  added: []
  patterns:
    - "Doc-content regression guard: Qt Test reads repo-local Markdown via QT_TESTCASE_SOURCEDIR + QFile, same pattern as QmlUiAuditTests::readSource"
    - "NF==11 row counter: split each line on '|', filter parts.size()==11, skip schema headers (cell0==\"region_id\") and Markdown separator rows (cell0 all dashes) — C++ port of `awk -F'|' 'NF==11'`"
    - "Line-based anchor counter: count lines whose trimmed form startsWith the marker, ignoring prose mentions of the marker text in backticks"

key-files:
  created:
    - tests/InventoryAuditTests.cpp
  modified:
    - CMakeLists.txt

key-decisions:
  - "New target InventoryAuditTests rather than folding into QmlUiAuditTests — Phase 50 section 2 names it explicitly for VERIFY-01 and the concern boundary (doc-content audit vs QML-source audit) is cleaner separated."
  - "Check 8 asserts only the 6-tag cleanup vocabulary (format), NOT file-existence. The Phase 50 section 2 file-existence half (4 Settings files existed at Phase 50 close) was inverted by Phase 57 (those files must STAY ABSENT). Re-asserting existence here would invert the Phase 57 regression. The post-removal invariant is permanently guarded by QmlUiAuditTests::deletedSettingsPathsStayAbsent / deletedRoutesStayAbsent; this target only records a cross-reference slot."
  - "Row counter skips both the 9-column schema header row (cell0==\"region_id\") AND the Markdown separator row (cell0 all dashes / colons) — both split into 9 cells / 11 fields and would otherwise inflate the count."

patterns-established:
  - "Doc-content audit harness: single-file QtTest with Q_OBJECT + #include \"X.moc\" + QTEST_MAIN, readSource via QT_TESTCASE_SOURCEDIR, doc-labeled QVERIFY2 messages (\"canonical: ...\" / \"snapshot: ...\") so a future drift points directly at the offending doc."
  - "Cross-reference slot pattern: a passing test slot whose body is a static `true` assertion with a message pointing at the real guard in another target — so `ctest -R Inventory` readers find the cross-reference without duplicating assertions."

requirements-completed: [VERIFY-01]

duration: 25min
completed: 2026-07-03
---

# Phase 58 Plan 01: InventoryAuditTests Summary

**Encoded the 9 Phase 50 section 2 deterministic inventory checks as a 12-slot Qt Test target that runs in every canonical verify pass, locking region counts / schema / status+verification enums / region-ID format / INV-02-03-04 coverage anchors / cleanup format / no-blank-upstream against both the canonical doc and the frozen snapshot.**

## Performance

- **Duration:** ~25 min
- **Started:** 2026-07-03T05:04Z
- **Completed:** 2026-07-03T05:29Z
- **Tasks:** 2 (combined into a single commit because Task 2's CMakeLists registration is what makes Task 1's binary discoverable to ctest)
- **Files modified:** 2 (1 created, 1 modified)

## Accomplishments

- New Qt Test target `tests/InventoryAuditTests.cpp` (12 slots) encodes the 9 Phase 50 section 2 checks + a Phase 57 cleanup cross-reference slot, all GREEN on both `docs/v3.6-ui-inventory.md` and `.planning/.../50-INVENTORY.md`.
- Registered in `CMakeLists.txt` next to `QmlUiAuditTests` (same `qt_add_executable` + `Qt6::Test` + `QT_TESTCASE_SOURCEDIR` + `add_test` + `ENVIRONMENT PATH` block).
- `ctest --test-dir build -R InventoryAuditTests` reports **12 passed, 0 failed** in ~10ms.
- A future drift in region count, schema, status/verification enum, region-ID format, INV-02/03/04 coverage anchors, or cleanup format now fails the canonical verify with a doc-labeled message pointing at the offending file.

## Task Commits

Each task was committed atomically. Tasks 1+2 share a commit because the CMake registration is what makes the test target buildable.

1. **Task 1+2: Create InventoryAuditTests.cpp + register in CMakeLists.txt** - `592c4ef` (feat)

**Plan metadata:** pending (will be added by the final docs commit).

## Files Created/Modified

- `tests/InventoryAuditTests.cpp` (created, 585 lines) — single-file QtTest encoding the 9 section 2 checks against both docs, plus the Phase 57 cross-reference slot.
- `CMakeLists.txt` (modified) — new InventoryAuditTests target block immediately after QmlUiAuditTests, with the Phase 58 / VERIFY-01 comment header.

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Header + separator rows inflated region count**
- **Found during:** Task 1 first test run.
- **Issue:** The `regionRows()` helper initially counted every line with 9 cells / 11 fields under `split('|')`. But the 9-column schema header row (`| region_id | region_name | ...`) AND the Markdown table separator row (`|---|---|...`) both also have 9 cells / 11 fields, so the canonical doc reported 38 region rows instead of 34 (4 region tables x 2 extra rows each).
- **Fix:** Extended `regionRows()` to skip rows whose cell 0 equals `region_id` (the schema header) AND rows whose cell 0 is all dashes / colons (the Markdown separator).
- **Files modified:** `tests/InventoryAuditTests.cpp`.
- **Commit:** `592c4ef`.

**2. [Rule 1 - Bug] Coverage anchor count matched prose mentions of the marker**
- **Found during:** Task 1 first test run.
- **Issue:** The snapshot's section 2 verification prose records the check results verbatim, including the inline text `` `<!-- INV-02 coverage ... -->` ×1 `` in backticks. A naive `QString::count("<!-- INV-02 coverage")` therefore returned 2 for the snapshot (1 real anchor + 1 prose mention) instead of 1. The canonical doc, which does not embed the section 2 prose, correctly returned 1.
- **Fix:** Replaced the `QString::count` calls with a line-based lambda `countAnchors()` that counts only lines whose trimmed form `startsWith` the marker — only real anchor comments begin a line with `<!-- INV-XX coverage`.
- **Files modified:** `tests/InventoryAuditTests.cpp`.
- **Commit:** `592c4ef`.

**3. [Rule 1 - Bug] Cleanup-tag format scan matched prose "tags:" mentions**
- **Found during:** Task 1 second test run.
- **Issue:** The section 7 cleanup-format lock scan matched any line whose first lowercase-colon token looked like a cleanup tag. The snapshot's section 2 prose contains the sentence "uses the six tags — `file:`×4, ...", which the scanner flagged as an unknown `tags:` tag.
- **Fix:** Restricted the format scan to lines inside a fenced code block (delimited by ```` ``` ```` on its own line) — the actual section 7 cleanup checklist lives in a ```` ```text ```` fence, so prose mentions elsewhere no longer trip the lock.
- **Files modified:** `tests/InventoryAuditTests.cpp`.
- **Commit:** `592c4ef`.

**4. [Rule 1 - Bug] QVERIFY2 inside const QString-returning helper**
- **Found during:** Task 1 first build.
- **Issue:** The `loadInventory()` helper was declared `const` returning `QString` but called `QVERIFY2` internally. The QVERIFY2 macro expands to `return;` (void) on failure, which conflicts with the QString return type — MSVC error C2561.
- **Fix:** Removed the QVERIFY2 from `loadInventory()` (it now just returns the readSource result); each caller QVERIFY2s the returned QString is non-empty with a doc-labeled message. Same net behavior, no macro-in-helper conflict.
- **Files modified:** `tests/InventoryAuditTests.cpp`.
- **Commit:** `592c4ef`.

### Auth Gates

None — no authentication surface in this plan.

## Known Stubs

None. The test reads real repo-local Markdown files via `QT_TESTCASE_SOURCEDIR` and asserts against their actual content. No mock data, no placeholder returns, no unwired props.

## Threat Flags

None. No new runtime surface, no network/auth surface, no schema changes at trust boundaries. The test reads two repo-local Markdown files; failure messages may include file paths and row contents (repo-local, non-sensitive). The threat register in the plan (T-58-01-SC accept, T-58-01-I accept, T-58-02-D accept) accurately characterizes the surface.

## Self-Check: PASSED

- `tests/InventoryAuditTests.cpp` exists — FOUND.
- `CMakeLists.txt` InventoryAuditTests target block — FOUND (verified via `ctest -N -R Inventory` lists exactly 1 test).
- Commit `592c4ef` exists in git log — FOUND.
- `InventoryAuditTests.exe` builds with 0 errors and 0 new warnings — VERIFIED.
- `ctest --test-dir build -R InventoryAuditTests` reports 12 passed, 0 failed — VERIFIED.
