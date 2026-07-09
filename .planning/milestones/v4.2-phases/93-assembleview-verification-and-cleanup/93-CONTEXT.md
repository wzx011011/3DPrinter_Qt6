# Phase 93: AssembleView Verification And Cleanup - Context

**Gathered:** 2026-07-09
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Phase 93 closes the v4.2 AssembleView milestone: it adds the AssembleView data
pool plumbing, locks final source/QML audits, removes stale placeholder
artifacts, runs the canonical verifier, launches the app, and records AssembleView
runtime visual evidence. This mirrors the verification+cleanup phases of prior
milestones (Phase 83 v4.0, Phase 88 v4.1).

Phase 93 delivers:
- AssembleView data pool plumbing (`AssembleViewDataID`/`AssembleViewDataPool`)
  caching per-object data needed by the view without leaking into Prepare/
  Preview state (ASMROUTE-02).
- Final source/QML audits covering region mapping, canvas-type routing,
  explosion-ratio wiring, and gizmo anchors (ASMVERIFY-01).
- Removal of any stale placeholder artifacts left by Phase 90-92 (the
  placeholder comment, dead imports, unused resources).
- Canonical verifier pass + `build/OWzxSlicer.exe` launch + AssembleView
  reachable at runtime (ASMVERIFY-02).
- AssembleView runtime visual evidence recorded against the target screenshots.

Out of scope for Phase 93:
- New AssembleView features (all delivered in Phase 90-92).
- Arrange / auto-arrangement (already complete) — never in scope.
- All removed network/device/cloud scope.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion (discuss skipped — use ROADMAP goal + gap matrix + codebase conventions)

1. **Data pool plumbing** — port `AssembleViewDataID`/`AssembleViewDataPool`
   (`GLGizmosCommon.hpp:268,274,299`) as a C++ structure that caches per-object
   data (model objects info, clipper) for the AssembleView gizmos. The key
   constraint: it must NOT leak into Prepare/Preview state. Read the upstream
   `AssembleViewDataPool::update()` + `model_objects_info()`/
   `model_objects_clipper()` getters. If a minimal cache is sufficient for the
   Phase 92 gizmo's needs, deliver that — the goal is isolation + correctness,
   not a full upstream replica. Document any simplification.

2. **Final audits** — extend `QmlUiAuditTests.cpp` with an
   AssembleView-restoration-milestone verification slot covering: placeholder
   removed, AssemblePage.qml present + registered, CanvasAssembleView enum,
   explosion-ratio wiring, Assembly gizmo anchors, data pool present. Mirror
   Phase 88's `settingsRestorationMilestoneHasFinalVerificationCoverage`
   pattern.

3. **Cleanup** — grep for any stale AssembleView placeholder artifacts (the
   old comment, dead imports, unused qrc entries, the removed Text). Remove or
   explicitly classify.

4. **Runtime evidence** — launch `OWzxSlicer.exe`, navigate to AssembleView,
   capture runtime screenshots (default view + explosion + measurement if
   feasible). If automated window capture is blocked (the Windows capture API
   issue seen in v4.1), record manual click-through + runtime launch evidence
   + canonical verifier as the verification (SETVERIFY-02 precedent).

### Recommended approach (noted for planning)
- Data pool: a C++ struct/class in `src/core/rendering/` (or a viewmodel cache)
  that the Assembly gizmo consumes, isolated from Prepare/Preview by scope +
  by the existing CanvasAssembleView routing branches.
- Audits: one consolidated `assembleViewRestorationMilestoneHasFinalVerificationCoverage`
  QmlUiAuditTests slot.
- Cleanup: grep-driven removal of any `装配视图暂不可用` / placeholder comment
  remnants + qml.qrc normalization for AssembleView entries.
- Build: canonical `auto_verify_with_vcvars.ps1` (Phase 91/92 precedent: if
  the per-invocation libslic3r rebuild times out the wrapper, document it;
  production code must compile/link clean, regression suite must pass).

</decisions>

<code_context>
## Existing Code Insights

### What Phase 90-92 built (the surface Phase 93 verifies)
- `CanvasAssembleView = 2` enum in `RhiViewport.h` (Phase 90).
- `AssemblePage.qml` with 4-region chrome (Phase 90).
- Navigation toggle in BBLTopbar (Phase 90).
- `CanvasAssembleView` routing branches in BackendContext + EditorViewModel
  (Phase 90).
- `explosionRatio` Q_PROPERTY + slider + per-volume separation + connector
  lines (Phase 91).
- `GizmoAssemblyMeasure = 19` + activability + `AssemblyMeasureGeometry` +
  overlay + 测量 panel + Ctrl+Y (Phase 92).

### Data pool upstream anchors (port targets)
- `third_party/OrcaSlicer/src/slic3r/GUI/Gizmos/GLGizmosCommon.hpp:268` —
  `enum class AssembleViewDataID { None, ModelObjectsInfo, ModelObjectsClipper }`
- `GLGizmosCommon.hpp:274` — `class AssembleViewDataPool` (`update()` +
  `model_objects_info()`/`model_objects_clipper()` getters)
- `GLGizmosCommon.hpp:299` — `class AssembleViewDataBase`

### Audit precedent
- `tests/QmlUiAuditTests.cpp` — Phase 88's
  `settingsRestorationMilestoneHasFinalVerificationCoverage` slot is the
  pattern for a consolidated milestone-verification audit.

### Cleanup precedent
- Phase 88 (v4.1) normalized settings QML resources + removed stale paths.
- Phase 90 already removed the `装配视图暂不可用` Text + comment from
  `Plater.qml`; Phase 93 confirms no remnants remain elsewhere.

### Build + verification
- Canonical: `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
  (ONLY valid build command per AGENTS.md).
- Phase 91/92 used `scripts/run_unit_tests_vcvars.ps1` as the focused
  regression runner when the canonical script's libslic3r reconfigure timed
  out the wrapper. Same precedent applies.

</code_context>

<specifics>
## Specific Ideas

- The data pool must be isolated from Prepare/Preview state — this is
  ASMROUTE-02's core constraint. Verify via a source audit that the pool is
  only consumed in the CanvasAssembleView path.
- Runtime visual evidence: the milestone audit (step 5a of autonomous) will
  check for screenshot evidence against the 3 target screenshots. Capture
  what's feasible; if window capture is blocked, the Phase 88/91 precedent
  (manual click-through + canonical verifier + runtime launch) applies.

</specifics>

<deferred>
## Deferred Ideas

- Full `GLGizmoMeasure` feature-picking engine (needs per-volume ITS + scene
  raycaster) — future milestone, documented in Phase 92's scope table.
- D3D12 / Vulkan backend for AssembleView — future backend work.

</deferred>

---

*Phase: 93-assembleview-verification-and-cleanup*
*Context gathered: 2026-07-09 (discuss skipped via workflow.skip_discuss; recommended approach noted for planning)*
