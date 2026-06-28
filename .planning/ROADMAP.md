# Roadmap: OWzx Slicer

## Milestones

- Complete: **v2.9** - Phases 10-15 (shipped 2026-06-25)
- Complete: **v3.0 PartPlate Core** - Phases 16-22 (shipped 2026-06-26)
- Complete: **v3.1 QRhi Rendering** - Phases 23-28 (shipped 2026-06-28)
- Complete with tech debt: **v3.2 Multi-Plate Data Polish** - Phases 29-32 (audited 2026-06-28)
- Active: **v3.3 Slice Preview Main Flow MVP** - Phases 33-36

## Active Milestone: v3.3 Slice Preview Main Flow MVP

**Goal:** Make the main user workflow work quickly and reliably: load a model, slice it, enter Preview, and see a non-empty D3D11 QRhi-rendered G-code preview with basic layer and move controls.

**Success criteria:**
- A user can load the committed STL fixture or an equivalent model, slice from the normal Prepare workflow, and land in Preview.
- PreviewViewModel exposes the sliced output path and non-empty preview payload with layer and move counts.
- D3D11 QRhi Preview renders the toolpath without falling back to `SoftwareViewport` as the normal path.
- Layer/move controls update visible ranges without crashes or hangs.
- Parser and workflow regressions are covered by focused tests plus the canonical build command.

## Phases

### Phase 33: Slice-to-Preview Navigation Gate - Complete

**Goal:** Wire the real slice completion path to Preview so the first visible main flow is usable.

**Requirements:** `FLOW-01`, `FLOW-02`, `FLOW-03`, `TEST-01` initial coverage.

**Deliverables:**
- Slice completion updates Preview state before or as the UI enters Preview.
- Completion notification/action remains coherent with automatic Preview navigation.
- Regression coverage proves the slice-to-preview transition cannot silently regress.

**Success criteria:**
- Slicing the committed fixture produces an output path and non-empty PreviewViewModel data.
- The app switches to Preview, or Preview is the immediate visible completion action.
- Focused tests pass before canonical verification.

**Evidence:**
- Commit: `5a4d37f feat: switch to preview after slicing completes`.
- Regression: `E2EWorkflowTests::test_backend_switches_to_preview_after_slice` covers BackendContext, EditorViewModel, PreviewViewModel, output path, layer count, move count, GCV1 payload, and Preview tab/viewMode switch.
- Verification: canonical build command exited 0 on 2026-06-28.

### Phase 34: G-code Preview Parser MVP

**Goal:** Make Preview data parsing robust enough for common slicer output.

**Requirements:** `GCODE-01`, `GCODE-02`, `GCODE-03`, `TEST-02`.

**Deliverables:**
- Parser support for G0/G1 travel/extrude, M82/M83, G92 E reset, layer Z, and tool changes.
- Stable preview payload metadata for layer count, move count, range controls, line type, and tool/extruder classification.
- Parser fixture tests for edge cases.

**Success criteria:**
- Parser fixture tests prove absolute/relative extrusion, reset, travel/extrude split, Z layers, and tool changes.
- Real sliced fixture produces non-empty preview payload and sane layer/move statistics.

### Phase 35: D3D11 Preview Rendering Interaction

**Goal:** Ensure the default high-performance Preview renderer is visibly useful and interactive.

**Requirements:** `RENDER-01`, `RENDER-02`, `RENDER-03`, `TEST-03`.

**Deliverables:**
- D3D11 QRhi Preview renders nonblank G-code segments by default on Windows.
- Layer slider, move slider, travel visibility, and color mode MVP update the renderer range.
- Smoke/audit coverage prevents accidental normal-path fallback to `SoftwareViewport`.

**Success criteria:**
- Manual launch log reports D3D11 QRhi selection.
- Preview canvas is nonblank after slicing.
- Layer/move/travel interactions remain responsive on the committed fixture and a larger sample.

### Phase 36: Verification and Handoff

**Goal:** Close v3.3 with deterministic evidence and a clear next backlog.

**Requirements:** `TEST-01`, `TEST-02`, `TEST-03` final coverage and all v3.3 requirements.

**Deliverables:**
- Canonical verification using `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
- Manual UAT notes for the load -> slice -> Preview workflow.
- Code review findings fixed or explicitly deferred.
- Updated planning state, requirements traceability, and next milestone backlog.

**Success criteria:**
- v3.3 requirements are complete or explicitly deferred with evidence.
- No active process or build artifact confusion remains after verification.
- User can test the main flow in the launched app.

## Deferred Backlog

- `THUMB-03` real GL/QRhi-capture thumbnails and 3MF pixel round-trip.
- `FIXTURE-02` full PLATE-09 save/reload assertions after writer integration fix.
- AssembleView.
- Auto filament-map recommendation.
- Wipe-tower geometry/rendering.
- D3D12 crash root cause.
- Missing CLI fixtures.

## Next Step

Start Phase 34:

```text
$gsd-plan-phase 34
$gsd-execute-phase 34 --interactive
```

---

*Last updated: 2026-06-28 for v3.3 milestone definition.*
