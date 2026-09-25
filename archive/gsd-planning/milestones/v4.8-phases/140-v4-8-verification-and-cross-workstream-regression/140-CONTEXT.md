# Phase 140: v4.8 Verification And Cross-Workstream Regression - Context

**Gathered:** 2026-07-16
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss).

<domain>
## Phase Boundary

The v4.8 cross-workstream regression gate (REGRESS-03): canonical build passes clean (exit 0); regression ctest passes (all groups); Prepare/Preview/Assembly/paint/calibration/MeshBoolean/Drill behaviors regression-free.

This is a VERIFICATION-ONLY phase. It adds a `v48CrossWorkstreamRegressionLocked` source-audit slot to `tests/QmlUiAuditTests.cpp` (mirroring the v4.6/v4.7 pattern) that asserts the v4.8 workstream anchors are present in the source, then runs the canonical build + all ctest groups + app launch liveness as the final milestone gate. No product-code changes.

**v4.8 workstream anchors to lock:**
- **WS1 (CGAL)**: `kCgalMeshBooleanAvailable = true` (Phase 137); MeshBoolean real call (`MeshBoolean::minus`/`self_union`); Drill real call (`MeshBoolean::minus` in drillObject).
- **WS2 (Assembly ASM-01)**: ProjectServiceMock assemble-transform accessors (`setAssembleOffset` etc.); EditorViewModel assembly-canvas routing (`m_activeCanvasType == 2` branch in apply-slots); RhiViewport `assembleOffsets` Q_PROPERTY.
- **WS3 (i18n)**: en.ts has 0 unfinished (filled).
- **v4.6/v4.7 regression**: paint bridge, calibration modes, paint-gizmo gate, flatten/fixMesh, KBShortcutsDialog still hold (the v4.7 lock already covers these; the v4.8 slot re-asserts the v4.7 anchors are still present so the v4.8 work didn't regress them).

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All choices at Claude's discretion — discuss skipped.

### Confirmed approach
- Add ONE new private slot `v48CrossWorkstreamRegressionLocked()` to `tests/QmlUiAuditTests.cpp`, declared near the existing `v47CrossWorkstreamRegressionLocked` declaration and implemented near its body. Each assertion is a `source.contains(...)` grep-style check on a source file.
- Reuse the existing `readSource()` helper used by v4.6/v4.7 slots.
- AUTOMOC reconfigure required (canonical verify does this).
- Verification = canonical build (`scripts/auto_verify_with_vcvars.ps1`) exit 0 + all ctest groups PASS + app launch liveness (`APP_RUNNING_PID`).

### Assertion set (source-audit grep checks)
- WS1: `evm.contains("kCgalMeshBooleanAvailable = true")`; `projSvc.contains("MeshBoolean::minus")` (boolean + drill).
- WS2: `projSvc.contains("setAssembleOffset")`; `evm.contains("m_activeCanvasType == 2")` (routing); `rhiViewport.contains("assembleOffsets")`.
- WS3: `enTs` — assert the `type="unfinished"` count is low/zero (read file, count occurrences, assert <= some small threshold).
- v4.7 regression: re-assert the v4.7 anchors (paint gate true, orientObject, its_merge_vertices, calibration modes 7/9) still present.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `tests/QmlUiAuditTests.cpp:5937` — `v47CrossWorkstreamRegressionLocked` (exact template to mirror).
- `tests/QmlUiAuditTests.cpp:462` — declaration site.
- `readSource()` helper in QmlUiAuditTests.

### Established Patterns
- Each milestone ships a `vNNCrossWorkstreamRegressionLocked` slot that grep-asserts the milestone's source anchors.
- Frontmatter `phase/status/requirements/plans` in the VERIFICATION.md.

### Integration Points
- QmlUiAuditTests is in the UI ctest group; the canonical verify runs it.

</code_context>

<specifics>
## Specific Ideas

- The slot is the regression lock — it prevents the v4.8 work from being silently reverted.
- Manual UI smoke (assembly Move gizmo drag, MeshBoolean subtract, Drill) is the human-gated complement; document in VERIFICATION as recommended manual checks.

</specifics>

<deferred>
## Deferred Ideas

- Automated UI/E2E tests for the assembly transform interaction (beyond the 3MF round-trip already in Plan 138-04) — future work.

</deferred>
