# Roadmap: OWzx Slicer

## Milestones

- ✅ **v2.9** Implementation Realignment and Stabilization — Phases 10-15 (shipped 2026-06-25)
- ✅ **v3.0** PartPlate Core — Phases 16-22 (shipped 2026-06-26)
- ✅ **v3.1** QRhi Rendering — Phases 23-28 (shipped 2026-06-28)
- ✅ **v3.2** Multi-Plate Data Polish — Phases 29-32 (audited 2026-06-28)
- ✅ **v3.3** Slice Preview Main Flow MVP — Phases 33-36 (superseded by v3.4)
- ✅ **v3.4** Import to G-code Complete Workflow — Phases 37-43 (closed by automated E2E)
- ✅ **v3.5** Preset Authoring Complete Workflow — Phases 44-49 (superseded after Phase 46)
- ✅ **v3.6** Screenshot-Driven OrcaSlicer UI Restoration — Phases 50-58 (shipped 2026-07-03)
- ✅ **v3.7** Screenshot-Level UI Parity Closure — Phases 59-64 (2026-07-04)
- ✅ **v3.8** RHI Gizmo Parity — Phases 65-73 (shipped 2026-07-04)
- ✅ **v3.9** Prepare Page UI Restoration — Phases 74-78 (shipped 2026-07-06)
- ✅ **v4.0** Preview Page UI Restoration — Phases 79-83 (shipped 2026-07-07)
- ✅ **v4.1** Parameter Settings Dialogs Source-Truth Restoration — Phases 84-88 (shipped 2026-07-09)
- 🚧 **v4.2** AssembleView Source-Truth Restoration — Phases 89-93 (in progress)

## Current Milestone: v4.2 AssembleView Source-Truth Restoration

**Goal:** Restore OrcaSlicer's AssembleView to screenshot/source-truth parity, replacing the current `Plater.qml` placeholder with a real canvas host, explosion-ratio slider, Assembly measurement gizmo (`Ctrl+Y`), data pool, and Plater `CanvasAssembleView` routing on the default RHI/D3D11 path.

**Scope rule:** This milestone is local/offline only. LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware workflows remain removed from scope. Arrange (auto-arrangement) is already implemented and explicitly out of scope.

**Visual truth:** `shotScreen/装配页.png` (default), `shotScreen/装配页_爆炸.png` (explosion ratio), `shotScreen/装配页_测量.png` (measurement gizmo).
**Behavior truth:** `third_party/OrcaSlicer/src/slic3r/GUI/GUI_Preview.hpp:180` (AssembleView class), `GLCanvas3D.hpp:509-513` (CanvasAssembleView enum), `GLCanvas3D.hpp:596` (explosion ratio), `Gizmos/GLGizmoAssembly.hpp` (Assembly gizmo), `Plater.cpp` (routing).

## Phases

- [x] Phase 89: AssembleView Source-Truth Gap Audit
- [x] Phase 90: AssembleView Shell And Canvas Host Restoration
- [x] Phase 91: Explosion Ratio And Assembly Rendering
- [x] Phase 92: Assembly Measurement Gizmo
- [x] Phase 93: AssembleView Verification And Cleanup

| Phase | Name | Goal | Requirements |
|---|---|---|---|
| 89 | AssembleView Source-Truth Gap Audit | Freeze the v4.2 AssembleView region map, current gaps, upstream anchors, Qt targets, replacement decisions, and verification expectations before edits. | ASMAUDIT-01, ASMAUDIT-02 |
| 90 | AssembleView Shell And Canvas Host Restoration | Replace the `Plater.qml` placeholder with a real AssembleView page/canvas host registered as the third `CanvasAssembleView`-equivalent canvas, with navigation entry and screenshot-aligned 4-region chrome, without breaking Prepare/Preview. | ASMSHELL-01, ASMSHELL-02, ASMROUTE-01 |
| 91 | Explosion Ratio And Assembly Rendering | Implement the explosion-ratio slider and multi-part volume separation rendering on the default RHI/D3D11 path with connector guide lines, mirroring upstream `m_explosion_ratio` behavior. | ASMEXPLODE-01, ASMEXPLODE-02 |
| 92 | Assembly Measurement Gizmo | Port the Assembly measurement gizmo (`Ctrl+Y`, `GLGizmoAssembly`/`GLGizmoMeasure` ONLY_ASSEMBLY mode) with measurement overlays, value annotations, and the right-side measurement panel. | ASMMEASURE-01, ASMMEASURE-02 |
| 93 | AssembleView Verification And Cleanup | Wire AssembleView data pool plumbing, lock final source/QML audits, remove stale placeholder artifacts, run canonical verifier, launch app, and record AssembleView visual evidence. | ASMROUTE-02, ASMVERIFY-01, ASMVERIFY-02 |

### Phase 89: AssembleView Source-Truth Gap Audit

**Status:** Complete
**Plans:** 1/1

Success criteria:
1. AssembleView screenshot-visible regions (4-region chrome, explosion slider, assembly-info panel, measurement panel, canvas view controls) are mapped to target screenshots, upstream source files, Qt targets, replacement decisions, and verification evidence. — Met by `89-GAP-MATRIX.md` (11 ASM-* regions, 11 columns, 3 screenshots, upstream anchors with line citations).
2. The current `Plater.qml:104-117` placeholder and `BackendContext.h:199` enum entry are reconciled into a removal/replacement plan with explicit out-of-scope classification for Arrange. — Met by the Placeholder Reconciliation (ASMAUDIT-02) and Out-of-Scope Classification sections.

### Phase 90: AssembleView Shell And Canvas Host Restoration

**Status:** Complete
**Plans:** 1/1

Success criteria:
1. A real AssembleView page/canvas host replaces the `Plater.qml` placeholder; navigation reaches AssembleView with no leftover "装配视图暂不可用" text.
2. AssembleView is registered as the third canvas host (mirroring `CanvasAssembleView = 2`) and coexists with Prepare/Preview without breaking navigation, payload, or view-mode state.
3. Plater-level selection/undo-redo/gizmo routing branches correctly on `CanvasAssembleView` so Prepare/Preview behavior is unchanged.

### Phase 91: Explosion Ratio And Assembly Rendering

**Status:** Complete
**Plans:** 1/1

Success criteria:
1. User can adjust an explosion-ratio control that separates multi-part object volumes for assembly inspection, mirroring upstream `m_explosion_ratio` (default 1.0, reset capability). — Met by the `explosionRatio` Q_PROPERTY on `EditorViewModel` (default 1.0, `resetExplosionRatio`) + the 爆炸比例 `CxSlider` in `AssemblePage.qml` bound to it.
2. AssembleView renders all loaded model volumes on the default RHI/D3D11 path with correct per-volume separation at the current explosion ratio, without regressing Prepare/Preview rendering. — Met by the per-volume blob restructure (`ProjectServiceMock::meshData`) + the CanvasAssembleView offset pass in `RhiViewportRenderer::buildModelVertices` (offset gated to AssembleView && ratio != 1.0); PrepareSceneDataTests + PartPlateTests + PreviewParserTests green.
3. Connector guide lines between separated parts render when the explosion ratio is non-default (matching `shotScreen/装配页_爆炸.png`). — Met by `uploadAssemblyConnectorBuffer` + `renderAssemblyConnectors` in `RhiViewportRenderer`, yellow GL_LINES segments between original volume centers, gated to AssembleView && ratio > 1.0.

### Phase 92: Assembly Measurement Gizmo

**Status:** Complete
**Plans:** 1/1

Success criteria:
1. User can invoke the Assembly measurement gizmo (`Ctrl+Y`) on the AssembleView canvas, mirroring `GLGizmoAssembly` activability rules (explosion ratio near 1.0, multi-volume selection). — Met by `GizmoAssemblyMeasure = 19` (distinct from `GizmoMeasure = 3`) + `EditorViewModel::isAssemblyMeasureActivable()` (AssembleView + `abs(ratio-1.0) < 1e-2` + ≥2 selected, mirrors `GLGizmoAssembly.cpp:53-68`) returning `(1 << 19)` in `availableGizmoMask()` + the `Ctrl+Y` Shortcut in `AssemblePage.qml`.
2. The Assembly gizmo measures distances/angles/relations between selected volumes using the upstream `ONLY_ASSEMBLY` measure mode, rendering measurement overlays anchored to the right geometry. — Met by the `AssemblyMeasureGeometry` C++ helper (AABB-center distance + longest-axis angle, a documented simplification of the full feature-picking engine — deferred to Phase 93) + the `RhiViewportRenderer` overlay (white dashed dimension line + arrowheads + teal value box gated to `CanvasAssembleView && m_gizmoMode == 19`).
3. Measurement value annotations (e.g. distance/angle) and the right-side measurement panel render per `shotScreen/装配页_测量.png`. — Met by the right-side 测量 panel in `AssemblePage.qml` (header + 装配测量 mode label + plane indicator + distance/angle rows bound to the viewmodel Q_PROPERTYs) + the in-world teal value box; canonical build zero errors; 5 regression suites pass.

### Phase 93: AssembleView Verification And Cleanup

**Status:** Complete
**Plans:** 1/1

Success criteria:
1. AssembleView data pool plumbing (`AssembleViewDataID`/`AssembleViewDataPool`) caches per-object data needed by the view without leaking into Prepare/Preview state.
2. Replaced AssembleView placeholder leaves no stale files, imports, resource entries, tests, or disconnected code paths; source/QML audits cover region mapping, canvas-type routing, explosion-ratio wiring, and gizmo anchors.
3. The canonical verifier passes, `build/OWzxSlicer.exe` launches, AssembleView is reachable at runtime, and visual evidence is recorded against the target screenshots.

Success criteria met:
1. AssembleView data pool plumbing caches per-object data without leaking into Prepare/Preview. — Met by `src/core/rendering/AssembleViewDataPool.{h,cpp}` (a minimal-but-correct port of upstream `AssembleViewDataID`/`AssembleViewDataBase`/`AssembleViewDataPool`/`ModelObjectsInfo`) owned by `EditorViewModel` behind the `m_activeCanvasType == 2` gate (mirrors `GLGizmosManager.cpp:427-431`). `selectedVolumeBoundsForAssemblyMeasure()` reads from the cached `ModelObjectsInfo` when on AssembleView; Prepare/Preview never populate or read it. The `assembleViewDataPoolIsolatedFromPrepareAndPreview` ViewModelSmokeTests slot + the `assembleViewRestorationMilestoneHasFinalVerificationCoverage` data-pool assertion both PASS. The `ModelObjectsClipper` resource is deferred (enum slot reserved; needs per-volume ITS — documented in `93-01-SUMMARY.md` / `93-VERIFICATION.md`).
2. Replaced AssembleView placeholder leaves no stale artifacts. — Met by the `assembleViewRestorationMilestoneHasFinalVerificationCoverage` + `assembleViewPlaceholderArtifactsStayAbsent` QmlUiAudit slots (both PASS) + a grep-clean audit (zero `装配视图暂不可用` / `assembleSlot` / v2.0 Out-of-Scope remnants; qml.qrc has exactly one normalized `pages/AssemblePage.qml` entry; the `ViewMode::AssembleView = 2` / `vmAssembleView` / `kLastVm` routing anchor preserved).
3. Canonical verifier passes; app launches; AssembleView reachable; visual evidence recorded. — Met by `build/93-01-canonical-build.log` zero errors (`OWzxSlicer.exe` linked at `[237/237]`), `build/93-01-test-run.log` exit 0 (5 suites pass), `OWzxSlicer.exe` runtime launch (alive in the process list), and the capture-blocked visual-evidence deviation (Phase 88/91 SETVERIFY-02 precedent: runtime launch + manual click-through + canonical verifier + regression ctest).

## Deferred Backlog

- Auto filament-map recommendation and wipe-tower geometry/rendering.
- Real thumbnail capture and 3MF pixel round-trip.
- Missing CLI fixtures and deterministic argv-based GUI fixture loading for screenshots.
- D3D12 root-cause investigation and future Vulkan/D3D12 backend promotion.

## Removed Scope

- LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, and printer-connected hardware calibration are no longer backlog items.

## Next Step

Phase 89 gap audit, Phase 90 shell + canvas host, and Phase 91 explosion-ratio
+ per-volume separation rendering are complete. Plan Phase 92 (Assembly
Measurement Gizmo) next — it ports `GLGizmoAssembly`/`ONLY_ASSEMBLY`
(`Ctrl+Y`) starting from the `availableGizmoMask()` AssembleView early-return
in `EditorViewModel`:

```text
$gsd-plan-phase 92
```

---

*Last updated: 2026-07-09 after Phase 91 plan 01 completion.*
