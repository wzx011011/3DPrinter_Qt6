# Phase 36: Verification and Handoff - Context

**Gathered:** 2026-06-28
**Status:** Ready for planning
**Source:** v3.3 ROADMAP plus Phase 33-35 summaries and verifications

<domain>
## Phase Boundary

Phase 36 closes v3.3. It verifies the load -> slice -> Preview MVP, reviews the Phase 33-35 code path, launches the app for manual user testing, and records handoff/backlog state. It does not introduce new preview features unless a blocking regression is found during verification or review.
</domain>

<decisions>
## Implementation Decisions

### Verification Scope
- D-36-01: Run the canonical verification command as the final automated gate.
- D-36-02: Treat Phase 33-35 verification files as requirement evidence, then re-check traceability in `.planning/REQUIREMENTS.md`.
- D-36-03: Confirm no stale `OWzxSlicer` process remains before and after verification unless intentionally launched for user UAT.

### Review Scope
- D-36-04: Review the changed v3.3 code paths that affect main flow: slice completion navigation, `PreviewViewModel` parser/payload, and `RhiViewportRenderer` Preview range drawing.
- D-36-05: Fix blocking review findings in Phase 36 if they affect the MVP flow; defer non-blocking polish to the next backlog.

### Handoff Scope
- D-36-06: Launch `build/OWzxSlicer.exe` for user testing after verification passes.
- D-36-07: Record UAT instructions and known deferred items so the next milestone can start without re-discovering v3.3 state.

### the agent's Discretion
- Exact format of the UAT handoff document.
- Whether non-blocking review notes live in `36-SUMMARY.md` or a separate review note, provided they are traceable.
</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `.planning/phases/33-slice-to-preview-navigation-gate/33-SUMMARY.md` and `33-PLAN.md`.
- `.planning/phases/34-g-code-preview-parser-mvp/34-SUMMARY.md` and `34-VERIFICATION.md`.
- `.planning/phases/35-d3d11-preview-rendering-interaction/35-SUMMARY.md` and `35-VERIFICATION.md`.
- Canonical verifier: `scripts/auto_verify_with_vcvars.ps1`.

### Established Patterns
- v3.3 evidence is stored per phase as `SUMMARY.md` and `VERIFICATION.md`.
- Final handoff phases should prefer documentation, review, and launch evidence over speculative implementation.
- All builds use only `build/` and the canonical PowerShell verifier.

### Integration Points
- `BackendContext` and `SliceService` for slice-completion navigation.
- `PreviewViewModel` for G-code parser and packed `GCV1` payload.
- `PreviewPage.qml`, `RhiViewport`, and `RhiViewportRenderer` for Preview rendering.
</code_context>

<specifics>
## Specific Ideas

- Use `git diff`/`git show` against recent v3.3 commits to inspect behavior, not just summaries.
- Inspect `startup_diagnostics.log` for latest QRhi D3D11 selection.
- Launch the app only after automated verification and process cleanup.
</specifics>

<deferred>
## Deferred Ideas

- Real thumbnail capture and 3MF pixel round-trip.
- Full upstream Preview parity.
- D3D12 crash root cause and Vulkan promotion.
- Missing CLI fixtures.
- Wipe tower, AssembleView, and auto filament-map recommendation.
</deferred>

---

*Phase: 36-verification-and-handoff*
*Context gathered: 2026-06-28 via autonomous lifecycle defaults*
