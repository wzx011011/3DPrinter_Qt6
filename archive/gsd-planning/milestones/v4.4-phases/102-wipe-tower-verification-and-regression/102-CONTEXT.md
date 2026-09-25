# Phase 102: Wipe-Tower Verification And Regression - Context

**Gathered:** 2026-07-11
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Final verification + regression for milestone v4.4 (Wipe-Tower Geometry
Readback And Real Rendering). Confirms the placeholder 10/10/50/100/25
defaults are no longer steady-state when a real slice exists, and that the
canonical verifier + OWzxSlicer.exe launch + Prepare/Preview/AssembleView
regression are all clean.

Success criteria (from ROADMAP):
1. **WTVERIFY-01**: Placeholder defaults (10/10/50/100/25) are no longer
   steady-state rendered values when a real slice exists; source/QML audits
   cover the readback wiring (Phase 100) + rendering-upgrade anchors
   (Phase 101).
2. **WTVERIFY-02**: The canonical verifier passes, `build/OWzxSlicer.exe`
   launches, Prepare/Preview/AssembleView rendering is regression-free, and
   the wipe-tower is visible at runtime when a multi-material slice produces
   one.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — discuss phase was skipped per user setting.

### Carry-Forward Inputs (frozen in prior phases)
- Phase 99 (WTAUDIT-01/02): the gap matrix maps 8 WT-* regions to upstream anchors + frozen decisions; WTVERIFY-01 source audits span WT-VIEWPORT-DEFAULTS, WT-PRINT-DATA, WT-READBACK-POINT, WT-HAS-WIPE-GATE, WT-PLACEHOLDER-BOX.
- Phase 100 (WTREAD-01/02): SliceService WipeTowerGeometry POD + wipeTowerGeometryReady signal; EditorViewModel 6 Q_PROPERTYs; PreparePage.qml:1648 GLViewport binds all 6; SoftwareViewport default show=false; W1 corner→center fix (commit b12d0e5).
- Phase 101 (WTRENDER-01/02): Option A baseline locked in GizmoGeometry.cpp comment; SoftwareViewport QPainter box closes the rendering gap; wipeTowerRealDimsReachRendererPipeline regression test; RHI render pipeline unchanged (was already correct end-to-end).

### Known Phase 102 Scope
1. **WTVERIFY-01 (source/QML audits):** Run the 8 WT-* region source audits
   from the Phase 99 gap matrix and confirm each is satisfied. The audits
   are:
   - WT-VIEWPORT-DEFAULTS: PreparePage.qml:1648 GLViewport binds wipeTower Q_PROPERTYs (Phase 100 + Phase 101 test lock).
   - WT-PRINT-DATA: `Print::wipe_tower_data()` / `has_wipe_tower()` read inside SliceService (Phase 100).
   - WT-READBACK-POINT: readback between print.process() and activePrint_.store(nullptr) (Phase 100).
   - WT-HAS-WIPE-GATE: showWipeTower set from has_wipe_tower(); no geometry when false (Phase 100 + W1).
   - WT-PLACEHOLDER-BOX: buildWipeTowerVertices fed real dims (Phase 101 test).
   - WT-RENDERER-BUFFER: uploadWipeTowerBuffer + renderWipeTower unchanged structure (Phase 101 verified).
   - WT-RENDER-UPGRADE: Option A baseline (Phase 101 comment); Option B documented as deferred.
   - WT-SOFTWARE-VIEWPORT: SoftwareViewport receives same real dims + gate (Phase 101 QPainter box).
   A source-audit test (e.g. in QmlUiAuditTests or ViewModelSmokeTests) that
   greps for the readback + rendering anchors consolidates these into a
   regression-lock.

2. **WTVERIFY-02 (canonical verifier + launch + regression ctest):**
   - Run `scripts/auto_verify_with_vcvars.ps1`; document the build timeout
     pattern if the libslic3r reconfigure times out the wrapper (production
     must still compile/link clean).
   - Confirm `OWzxSlicer.exe` launches (process liveness — 5-second no-crash
     probe per the STATE.md deferred-items bar; Windows capture API blocked).
   - Regression ctest: PartPlateTests, PrepareSceneDataTests,
     ViewModelSmokeTests (97+ slots), QmlUiAuditTests — all PASS.
   - Runtime wipe-tower visibility: per STATE.md, runtime VISUAL evidence is
     blocked; reachability is verified via process-liveness + canonical
     verifier + regression ctest, not a screenshot. Document this bar in
     the SUMMARY.

3. **Source-audit test consolidation:** Add a QmlUiAuditTests or
   ViewModelSmokeTests slot that asserts the WT-* source anchors are present
   (grep-style regression lock). This is the Phase 102 WTVERIFY-01
   deliverable — it prevents future regression where someone removes the
   readback wiring or the PreparePage binding.

4. **Optional scratch cleanup:** Scratch build logs (build_wt_verify.log,
   build_phase*.log if any) accumulated across v4.4 phases — remove if
   unreferenced (mirror Phase 98 cleanup).

</decisions>

<code_context>
## Existing Code Insights

Codebase context will be gathered during plan-phase research. Key anchors:
- `src/core/services/SliceService.cpp:628-656` (Phase 100 readback capture, incl. W1 corner→center at :655-656)
- `src/core/services/SliceService.h` (WipeTowerGeometry POD + wipeTowerGeometryReady signal)
- `src/core/viewmodels/EditorViewModel.h:650-655` (6 Q_PROPERTYs) + `.cpp` (onWipeTowerGeometryReady slot)
- `src/qml_gui/pages/PreparePage.qml:1670-1675` (GLViewport bindings)
- `src/core/rendering/GizmoGeometry.cpp:449-485` (Phase 101 Option A baseline comment)
- `src/qml_gui/Renderer/RhiViewportRenderer.cpp:1064-1095` (uploadWipeTowerBuffer — unchanged)
- `src/qml_gui/Renderer/SoftwareViewport.cpp:447-498` (Phase 101 QPainter box)
- `tests/ViewModelSmokeTests.cpp` (Phase 100 readback test + Phase 101 render-pipeline test)
- `tests/QmlUiAuditTests.cpp` (the source-audit test home — pattern reference)

</code_context>

<specifics>
## Specific Ideas

No specific requirements — discuss phase skipped. Refer to ROADMAP phase description and success criteria.

</specifics>

<deferred>
## Deferred Ideas

Per ROADMAP "Deferred Backlog" + "Removed Scope" sections — do NOT promote:
- Auto filament-map recommendation + wipe-tower (now partially shipped via v4.4 — the geometry readback + rendering is done; the auto filament-map recommendation remains future).
- CLI fixtures + deterministic argv GUI fixture loading (FIXTURE-02).
- D3D12 root cause + Vulkan/D3D12 backend.
- Full GLGizmoMeasure engine + AssembleViewDataPool clipper.
- LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware workflows (removed scope).
- Option B real wipe-tower mesh via wipe_tower_mesh_data (LOCKED future upgrade per Frozen Decision 2).

</deferred>
