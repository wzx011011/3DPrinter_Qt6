# Retrospective

A living document. Each milestone adds a section; cross-milestone trends are updated at the bottom.

## Milestone: v3.0 — PartPlate Core

**Shipped:** 2026-06-26
**Phases:** 7 (16-20 mainline + 21-22 review-driven) | **Plans:** 7 | **Requirements:** 14/14 satisfied
**Git range:** `v2.9..v3.0` (21 commits, 49 files, +5520/-502)
**Audit status:** `tech_debt` — review-clean (Phase 21+22)

### What Was Built
Real PartPlate/PartPlateList domain model replacing the mock parallel-vector shell; 3MF multi-plate round-trip (write+load); per-plate config fully honored during slicing; clone/reorder/printable lifecycle ops with QML UI; two review cycles fixing 4 P0/P1 findings.

### What Worked
- **Scout-before-committing on Phase 19 (D-14).** Before planning the per-plate slice refactor, a scout revealed Qt6's stack-local `Slic3r::Print` already isolated plates per-slice — turning an assumed-large refactor into a documentation task. This single scout saved a multi-phase rewrite. Lesson: for any "must refactor architecture" plan, verify the assumption first.
- **Two explicit review cycles (Phase 21 code, Phase 22 UI).** The milestone wasn't just "passes its audit" — it got genuine critical review that found 2 functional bugs (clonePlate mock-mode wrong plate; config silent-drop) the green tests missed. Both bugs were in error paths the happy-path tests didn't exercise.
- **Tests-first for the big-bang migration (Phase 16 Wave 1).** Landing the model + 5 unit tests before the destructive Wave 2 migration caught zero design flaws but gave confidence the model was right before touching ProjectServiceMock. Worth the extra plan.
- **Round-trip test QSKIP with honest reason (Phase 18).** Rather than fake-pass or fake-fail, the round-trip test documents exactly why it can't run (store_bbs_3mf needs geometry). This is more useful than a misleading PASS and more honest than hiding the limitation.
- **Source-truth alignment consistency.** Every gray-area decision (instance-level membership, native DynamicPrintConfig, big-bang, deep clone) chose the most upstream-faithful option. This made integration between phases clean (Phase 18 3MF + Phase 19 slice both consume PartPlate::config() naturally).

### What Was Inefficient
- **Canonical full-verify is slow (8+ min).** Phase 18 alone had 8 build-fix cycles × ~8 min = ~55 min just waiting for builds. This dominated the milestone wall-clock. The root causes: libslic3r compiled from source every time; ninja sometimes missed test recompilation (moc/autogen tracking), forcing full rebuilds to be safe. A faster feedback loop (incremental ninja + targeted test runs mid-phase, full canonical only at phase end) would have cut this dramatically.
- **ninja incremental-miss on tests.** Multiple times (Phase 18, 21) I edited test source but the test binary wasn't recompiled, so "passing" was stale. The moc/autogen dependency on Q_OBJECT test classes doesn't track added test methods reliably. This eroded trust in incremental builds and pushed toward slow full canonicals.
- **Bug-1 (clonePlate mock-mode) survived Phase 17's review-light verification.** The Phase 17 test asserted "dst has objects" but not "current didn't gain objects" — a negative assertion. Happy-path tests systematically miss error/cross-contamination paths. Phase 21 review caught it; the lesson is to write negative assertions (what should NOT change) alongside positive ones.
- **Big-bang migration (Phase 16-02) touched ~50 sites, not ~30 as planned.** The plan underestimated because two async load lambdas and the JSON round-trip each had their own plate-vector references. A pre-migration grep of all vector references would have given an accurate count. The big-bang itself was still the right call (no bridge debt), just under-scoped in the plan.

### Patterns Established
- **`src/core/model/` pure-domain-object layer** with ProjectServiceMock as Qt adapter.
- **File-local free functions for libslic3r-type-returning helpers** (buildPlateDataList) to avoid header pollution — a reusable technique for any libslic3r API that returns non-forward-declarable types.
- **pendingPlate* receiver staging members** to carry per-plate state across async load lambdas (load read-lambda → rebuild-lambda scope boundary).
- **QmlUiAuditTests actively guards feature wiring** (not just generic honest-UI rules) — extended in Phase 22 to assert clonePlate/movePlate/setPlatePrintable are wired. Pattern for future UI features: add a wiring assertion, not just a "no placeholder" check.
- **Two review cycles (code + UI) before milestone close** as a quality gate — not just the process audit.

### Key Lessons
1. **Scout before refactoring.** Phase 19's D-14 finding (stack-local Print already isolates) turned a big refactor into docs. Always verify "the architecture must change" assumptions with a read.
2. **Happy-path tests systematically miss error paths.** Both review bugs (Phase 21) were in failure modes the green tests didn't exercise. Write negative assertions + review error paths explicitly.
3. **Build speed is a productivity multiplier.** 8-min canonical × many cycles made Phase 18 take 4× longer than it should. Investing in fast feedback (incremental trust, prebuilt libslic3r) pays back across every future milestone.
4. **Reviews find what audits and tests miss.** The v3.0 audit passed (14/14 requirements, 6/6 integration) AND 44 tests passed AND both bugs were live in the code. Audit = "did you build what you said"; review = "is what you built actually correct." Both needed.
5. **QSKIP with an honest reason beats fake-pass.** The Phase 18 round-trip QSKIP made the PLATE-09 limitation explicit and diagnosable rather than hidden behind a green check.

### Cost Observations
- Phases: 7 (5 mainline + 2 review), Plans: 7.
- Build-fix cycles: ~13 total (Phase 18 had 8 — the libslic3r header-pollution + cross-lambda-scoping chain).
- Two Explore-agent delegations (gap analysis + milestone audit integration check); rest inline.
- Code review + UI review added 2 phases but caught 4 real issues — high value-per-effort.

---

## Milestone: v2.9 — Implementation Realignment and Stabilization

**Shipped:** 2026-06-25
**Phases:** 6 | **Plans:** 6 | **Requirements:** 28/28 satisfied
**Git range:** `a34d666..4a1b009` (20 commits, 82 files, +4984/-517)
**Audit status:** `tech_debt` (no blockers)

### What Was Built

A stabilization/truth-reset milestone, not new features:
- Planning realignment: `.planning` entry files now agree with git history through v2.8 and mark v2.9 complete.
- Source hygiene: removed targeted `\r\n` literal-escape artifacts and mojibake; removed `SliceService.cpp.backup`; promoted `AppSettingsService`/`SoftwareViewport` to tracked implementation.
- Calibration closure: PA/Flow Rate/Temp Tower wired to stable topbar ids with deterministic ViewModel regression coverage; unimplemented modes explicit Pending/Blocked.
- Hybrid integration: deterministic protocol-level test coverage for SSDP/MQTT/FTP/camera; SoftwareViewport default + OpenGL-behind-`OWZX_OPENGL` enforced by QmlUiAuditTests.
- Visible placeholder triage: disabled/no-op controls hidden or honestly classified; ModelMall copy corrected to local-preview/unavailable.
- Verification gate: canonical verify green; explicit smoke runs (32 + 7 passed).

### What Worked

- **Three-source cross-reference audit** (VERIFICATION.md + SUMMARY frontmatter + REQUIREMENTS traceability) caught zero drift — every requirement had aligned evidence across all three sources. This is the strongest evidence-quality signal seen so far.
- **Deterministic test coverage replacing transport confidence.** Phase 13's pattern of "fixture the protocol, document the live-hardware gap separately" turned Hybrid claims into testable contracts without pretending the hardware path was verified.
- **Stable-id routing for calibration** (Phase 12) instead of list indexes — trivial change, eliminated a whole class of menu-reordering regressions and is now enforced by tests.
- **QmlUiAuditTests as an honesty gate** (Phases 13-14) — static assertions on QML (viewport default, no marketplace copy, no empty handlers) caught UI-honesty regressions at build time rather than at manual review.
- **Phase numbering continued from prior milestone** (10-15, not 1-6) kept git history and planning coherent.

### What Was Inefficient

- **Hygiene scans were scoped to touched files only.** Phase 11 did not run a broad repo-wide mojibake/escape scan, so untouched files may still contain damage. A one-shot repo-wide sweep would have been cheaper than discovering this in a later milestone.
- **`.Codex` vs `.codex` path casing** slipped through because Windows is case-insensitive. It should have been caught at Phase 10 rule-file creation (the lowercase git path was visible). Normalize before any case-sensitive CI.
- **The "32 passed" evidence-count question** cost an audit cycle. The integration checker statically counted 30 slots and flagged a discrepancy; reconciling required reading the actual `build/ViewModelSmokeTests.phase15.txt` (`Totals: 32 passed`). Lesson: trust the runtime test output over static slot counting when they disagree.
- **`ffmpeg/freetype.dll` transient post-build copy failure** (Phase 13) — environment flakiness that passed on rerun. Worth adding a retry or a clearer diagnostic to the canonical script.

### Patterns Established

- **Real/Hybrid/Mock/Blocked/Placeholder** classification vocabulary is now canonical for requirements, audits, and handoffs.
- **Per-phase VERIFICATION.md with a frontmatter `requirements:` map** is the authoritative per-phase evidence artifact; the milestone audit aggregates them.
- **Manual-verification-required frontmatter** (`manual_verification_required: mqtt_live_publish: true`, etc.) explicitly separates protocol-level automated coverage from live-hardware verification — no more "Hybrid" meaning "we hope it works."
- **QmlUiAuditTests enforce UI honesty invariants** (viewport mode, no placeholder copy, calibration routing) at build time.

### Key Lessons

1. **Truth-reset milestones are worth doing before large modules.** Starting v3.0 (PartPlate/AssembleView) from a baseline where planning, code, and evidence agree is materially safer than the alternative.
2. **When a `*Mock` service contains real paths, the class name lies.** The classification vocabulary fixes the communication problem; consider whether a rename pass is ever worth it (probably not — too many call sites).
3. **Gate user-visible behavior behind static audits, not just runtime tests.** QmlUiAuditTests caught things runtime tests couldn't.
4. **Document hardware-dependent verification explicitly.** "Hybrid" without a `manual_verification_required` note is just debt with a confident name.

### Cost Observations

- Phases: 6, Plans: 6 — small, focused milestone.
- Heavy use of read-only Explore agent for the cross-phase integration check (single delegation, comprehensive result).
- No code-review or debug cycles triggered; execution was clean (mode: yolo, auto_advance).

---

## Cross-Milestone Trends

| Milestone | Phases | Plans | Reqs satisfied | Audit status | Notes |
|---|---|---|---|---|---|
| v2.9 | 6 | 6 | 28/28 | tech_debt | First milestone with full 3-source audit; zero requirement drift |
| v3.0 | 7 (5+2 review) | 7 | 14/14 | tech_debt (review-clean) | First milestone with explicit code+UI review cycles; scout-before-refactor (D-14) avoided a big rewrite; build speed is the dominant inefficiency |

*Last updated: 2026-06-26 after v3.0 milestone completion.*
