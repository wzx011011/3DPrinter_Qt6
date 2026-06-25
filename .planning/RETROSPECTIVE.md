# Retrospective

A living document. Each milestone adds a section; cross-milestone trends are updated at the bottom.

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

*Last updated: 2026-06-25 after v2.9 milestone completion.*
