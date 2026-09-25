# Phase 58: End-to-End Visual and Functional Verification - Context

**Gathered:** 2026-07-03
**Status:** Ready for planning
**Mode:** Verification phase — test creation + UAT checklist + failure classification

<domain>
## Phase Boundary

Phase 58 proves the restored v3.6 workflow works visually and functionally from
import through G-code export, and produces the verification artifacts that close
the milestone. The automated coverage already partially exists (Phase 55 Preview
tests, Phase 56 settings tests, Phase 57 cleanup regression tests, E2E workflow
tests). Phase 58 fills the gaps, encodes the Phase 50 inventory checks, and
produces the manual UAT checklist that the user runs against the 4 screenshots.

**In scope (VERIFY-01..05):**
1. **VERIFY-01 (inventory + registration audit):** encode the Phase 50 §2
   deterministic checks as `tests/InventoryAuditTests.cpp` (region counts per
   screenshot 6–12 / total 30–40; 9-column schema; status/verification enums;
   region-ID format; coverage anchors INV-02/03/04; cleanup format; no-blank-
   upstream). Plus assert QML route/resource registration contains only live
   components (no deleted paths from Phase 57).
2. **VERIFY-02 (workflow transitions):** confirm automated coverage of
   import → configure → prepare → slice → preview → export, including slice
   invalidation after settings changes. Most exists (E2EWorkflowTests +
   Phase 56 SETTINGS-07 tests); add any missing transition assertion.
3. **VERIFY-03 (Preview stability):** confirm deterministic harnesses cover
   Preview layer/move/camera interactions so the disappearing-preview bug cannot
   regress. Exists from Phase 55 (GCODE-05 tests) — verify still green, add a
   gap test only if a transition is uncovered.
4. **VERIFY-04 (manual UAT checklist):** produce `58-UAT.md` — a user-runnable
   checklist validating visual parity against the 4 screenshots
   (`shotScreen/准备页.png`, `预览页.png`, `打印机参数设置页.png`, `材料参数设置页.png`)
   and behavior parity against the mapped upstream source. This is the
   human-verification gate; it does NOT block the phase goal (status will be
   `human_needed`, presented to the user).
5. **VERIFY-05 (canonical verification):** run `scripts/auto_verify_with_vcvars.ps1`
   + `ctest`; any failure (e.g., pre-existing CliTests missing-fixture failures)
   classified with file/command/cause/follow-up-owner.

**Out of scope:**
- Re-implementing Phase 56/57 work.
- Device/cloud/AssembleView (Future milestones).
- Fixing the pre-existing CliTests missing-fixture failures (separate fixture
  milestone — STATE.md carry-forward).

</domain>

<decisions>
## Implementation Decisions

### Locked
- The Phase 50 inventory checks (§2, 9 deterministic assertions) are encoded
  verbatim as InventoryAuditTests — they are the VERIFY-01 regression guard.
- The 4 screenshots are the visual truth for VERIFY-04.
- CliTests failures are PRE-EXISTING (missing `hotend.stl`/`Block20XY.stl`
  fixtures — STATE.md carry-forward) — classified, NOT fixed in this phase.

### Claude's Discretion
- Whether InventoryAuditTests is a new test target or folded into QmlUiAuditTests
  (prefer a new `InventoryAuditTests` target per Phase 50 §2 which names it
  explicitly for VERIFY-01).
- Exact UAT checklist structure (per-screenshot sections vs per-workflow).
- Whether to add gap tests for VERIFY-02/03 or rely on the existing coverage
  (audit first; add only real gaps).

</decisions>

<code_context>
## Existing Code Insights

### Reusable assets
- `.planning/phases/50-screenshot-and-source-truth-inventory/50-INVENTORY.md` §2
  — the 9 deterministic checks to encode (presence, region counts, schema,
  enums, region-ID format, coverage anchors, cleanup format, file existence,
  no-blank-upstream). §2 also documents the GNU grep `-cE` quirk + the
  `awk -F'|' 'NF==11'` row-count approach to replicate in C++.
- `tests/QmlUiAuditTests.cpp::readSource()` helper — same pattern
  (QT_TESTCASE_SOURCEDIR + QFile) for InventoryAuditTests.
- Existing green coverage: ViewModelSmokeTests 84/0, QmlUiAuditTests 38/0,
  PreviewParserTests, E2EWorkflowTests (incl. SETTINGS-07 tests), PrepareScene,
  PartPlate.

### Integration points
- `tests/CMakeLists.txt` — add the InventoryAuditTests target (CTest registration).
- The InventoryAuditTests read `docs/v3.6-ui-inventory.md` (canonical live doc)
  + `.planning/phases/50-.../50-INVENTORY.md` (frozen snapshot) — both must pass
  the §2 checks.

### Established patterns
- Build: ONLY `scripts/auto_verify_with_vcvars.ps1`; ONLY `build/`.
- Qt Test output via `-o file,txt` (stdout is buffered/invisible on redirect).
- Encoding: UTF-8 no BOM, ASCII-only comments.

</code_context>

<specifics>
## Specific Ideas

- VERIFY-01 is the highest-value new artifact — the InventoryAuditTests lock the
  Phase 50 inventory contract so future drift is caught at ctest time. Mirror
  the §2 checks exactly (the §2 prose gives literal commands + numeric results).
- VERIFY-04 UAT checklist should be runnable by a non-developer: "open the app,
  do X, compare to screenshot Y, check Z." Each item maps to a region in the
  inventory + a requirement.
- VERIFY-05: classify CliTests (`testLoadHotend`/`testSliceBlock20XY` — missing
  fixtures) + the auto_verify smoke-step flakiness (Qt version-mismatch warning,
  environmental — see Phase 56 VALIDATION note) as known/non-blocking.
- This phase will likely end with status `human_needed` (VERIFY-04 manual UAT)
  — that is correct and expected; the milestone hands off to the user for
  visual sign-off.

</specifics>

<deferred>
## Deferred Ideas

- Fixing CliTests missing fixtures (`hotend.stl`, `Block20XY.stl`) — future
  fixture milestone (STATE.md carry-forward).
- Full cross-platform (macOS/Linux) verification — Windows-only this milestone.
- Performance/benchmark regression suite — future.

</deferred>
