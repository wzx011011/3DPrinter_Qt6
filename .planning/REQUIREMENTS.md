# Requirements: OWzx Slicer — v4.6 Core Feature Completion Sweep

**Defined:** 2026-07-14
**Core Value:** OrcaSlicer upstream behavior is the product source of truth; Qt6 code inherits that behavior and must not invent new product behavior without an upstream mapping.
**Milestone:** v4.6 — Core Feature Completion Sweep (Mega-Milestone). Lift the four highest-value main-flow gaps from "skeleton-level" to "end-to-end usable" in one cycle.

## Scope Basis

A code-truth gap audit (run during `/gsd:new-milestone`) confirmed each workstream targets an existing-but-incomplete surface:

| WS | Target | Current State (evidence) |
|---|---|---|
| 1 | Preview TickCode / IMSlider closed loop | `LayerSlider.qml` is functionally complete (tick render + right-click add/edit/delete menu) but **orphaned — never instantiated**; PreviewPage renders `PreviewLayerRail.qml` which shows **no ticks**. `custom_gcode_per_print_z` has **zero references** in the tree; tick CRUD mutates memory only and is **silently lost on re-slice**. Read-side parse of sliced G-code exists. |
| 2 | Gizmo triangle-paint engine | Support/Seam/MMU have `GizmoMode` enum values + buttons + Q_PROPERTY panels, but **no pick / no render / no execute**. `setTriangleSupportState` takes a triangle index that **no QML/C++ code ever computes**. Zero `renderSupport`/`renderSeam`/`renderMmu` functions. `SupportPaintTypes.h` data layer exists; TriangleSelector pipeline absent. |
| 3 | Calibration mode completion | Real libslic3r slice path exists for 3/9 modes (PA/FlowRate/TempTower via `SliceService.cpp:567-579`). MaxVolumetricSpeed has UI but `mode=0` (no slice). 4 more software-sliceable modes absent. Range params hardcoded; K-value writeback is mock. |
| 4 | Tech-debt convergence | i18n non-en translation ~0%; multiple shipped phases missing Nyquist VALIDATION.md; legacy dead-code pages (DeviceListPage force-empty bug, AuxiliaryPage semantic mismatch, ConfigPage redundancy) persist. |

## v4.6 Requirements

Requirements for this milestone. Each maps to exactly one roadmap phase (filled by roadmapper).

### WS1 — Preview TickCode / IMSlider Closed Loop

- [ ] **TICK-01**: User sees layer tick marks (pause / custom G-code / color change / filament change) rendered on the Preview layer slider when a sliced G-code contains them, replacing the tick-less `PreviewLayerRail` rendering.
- [ ] **TICK-02**: User can add a tick (pause, custom G-code, color change, or filament change) via right-click on the slider and the tick is written back to libslic3r `Model::set_custom_gcode_per_print_z` (closing the zero-reference gap).
- [ ] **TICK-03**: Editing a tick (add / remove / change color / change extruder) triggers a re-slice, and the resulting G-code actually contains the user's pause / color-change / custom-G-code markers at the chosen layer.
- [ ] **TICK-04**: Tick type coverage matches upstream — user can insert and round-trip all 5 types: PausePrint, CustomGcode, ToolChange (filament change), ColorChange, Template.
- [ ] **TICK-05**: User can drag an existing tick to a new layer to relocate it (upstream IMSlider supports tick drag; current CRUD has no `moveTick`).

### WS2 — Gizmo Triangle-Paint Engine

- [ ] **PAINT-01**: A `TriangleSelector`-style triangle-pick + adaptive-subdivide + paint-state pipeline is ported to C++ (reusing v4.5 per-volume ITS + SceneRaycaster), so mouse input can be resolved to a specific triangle index on the selected object's mesh.
- [ ] **PAINT-02**: The QRhi/D3D11 renderer draws a colored-facet overlay for painted triangles (support enforcer/blocker red/blue, seam, MMU per-extruder colors); the Software fallback path mirrors the overlay with QPainter.
- [ ] **PAINT-03**: User can paint with a brush — mouse-move maps to triangle indices within a brush-radius sphere/circle, brush size is adjustable, and paint/erase/smart-fill (bucket) modes work on the selected object.
- [ ] **PAINT-04**: GLGizmoSupportPainter (support enforcer + blocker) and GLGizmoSeamPainter (seam painting) work end-to-end: the painted facets feed into the slice as support/blocker/painting modifiers and affect the generated support / seam placement.
- [ ] **PAINT-05**: GLGizmoMmuSegmentation works end-to-end: user paints mesh regions with per-extruder colors and the segmentation feeds the multi-material slice (painting regions assigned to extruders).

### WS3 — Calibration Mode Completion

- [ ] **CALIB-01**: The 4 software-sliceable libslic3r Calibration modes that Qt6 does not yet dispatch (MaxVolumetricSpeed, VolumetricRate, RetractionTune, FlowRateProxy) are wired into the existing `SliceService::setCalibParams` → `print.set_calib_params` path and produce real calibration G-code (raising coverage from 3/9 to 7/9 software modes).
- [ ] **CALIB-02**: User can set the calibration range (start / end / step) in the CalibrationDialog UI, replacing the hardcoded service ranges.
- [ ] **CALIB-03**: Calibration result (K-value / flow rate / etc.) is read back from the real slice output and written into the active preset where libslic3r provides it, replacing the mock K-value writeback.

### WS4 — Tech-Debt Convergence

- [ ] **I18N-01**: Non-en translation coverage is filled for zh_CN / ja / ko / de / fr (currently ~0% translated) for the strings touched by v4.6 workstreams and the main-flow UI surfaces.
- [ ] **PROC-01**: Nyquist VALIDATION.md files are produced for v4.6 phases and the gap is closed for previously-shipped phases that are missing them (process debt carry-forward).
- [ ] **CLEAN-01**: Legacy dead-code pages are repaired or removed: DeviceListPage force-empty bug fixed, AuxiliaryPage semantic mismatch (UI analysis vs service file-copy) reconciled, redundant ConfigPage removed, with old files/routes/registrations/resources/imports/tests cleaned up in the same milestone.

### Cross-Cutting — Verification & Regression

- [ ] **REGRESS-01**: Canonical build (`scripts/auto_verify_with_vcvars.ps1`) passes clean, regression ctest passes, and Prepare/Preview settings/assembling behaviors are regression-free across all v4.6 workstreams.

## Future Requirements

Deferred to a later milestone. Tracked but not in the v4.6 roadmap.

### Gizmo (after WS2 TriangleSelector ships)

- **HOLLOW-01**: GLGizmoHollow works end-to-end (hollow-paint + OpenVDB-based hollowing). Depends on PAINT-01 and OpenVDB availability (currently unavailable).
- **FACEDET-01**: GLGizmoFaceDetector detects flat faces (currently `detectFlatFaces` is a mock `return false`). Depends on PAINT-01.
- **SLASUP-01**: GLGizmoSlaSupports (SLA support generation). Currently only an enum value with no logic.
- **MEASURE-06**: Assembly-mode transformation actions (deferred from v4.5).

### Calibration (hardware-dependent — out of scope under printer-hardware removal rule)

- **CALIB-HW-01**: ManualLeveling / BedLeveling / Vibration modes. Require live printer hardware.

### Backend

- **D3D12-04**: D3D12 default-backend promotion. Deferred from v4.5; needs a confirmed root cause + clean repro on the original machine.
- **PLATE-09-FULL**: Full save/reload state assertions.

## Out of Scope

Explicitly excluded from v4.6. Documented to prevent scope creep.

| Feature | Reason |
|---------|--------|
| Hollow / FaceDetector / SlaSupports gizmos | Share the WS2 TriangleSelector engine; unlock as a follow-up after WS2 ships the stable paint foundation. |
| Hardware-dependent Calibration modes (ManualLeveling / BedLeveling / Vibration) | Require live printer hardware; fall under the existing printer-hardware-workflows removal rule. |
| D3D12 default-backend promotion | Deferred from v4.5; needs confirmed root cause + clean repro. |
| Vulkan production backend | Qt SDK disables the `vulkan` public feature; SDK-blocked. |
| MEASURE-06 Assembly-mode transformation actions | Deferred from v4.5; needs the stable feature-picking foundation that v4.5/v4.6 provide. |
| LAN device discovery, device send/upload, cloud print, Monitor task lifecycle, ModelMall/Home WebView/cloud workflows, live camera/network streams, printer-connected hardware workflows | Removed from forward scope by user direction on 2026-07-07 unless explicitly reopened. |
| Changing libslic3r slicing algorithms | Outside GUI migration scope; high risk. |
| Alternate build directories or non-canonical build scripts | Project rule: only `build/` and `scripts/auto_verify_with_vcvars.ps1`. |

## Traceability

Filled during roadmap creation. Each requirement maps to exactly one phase.

| Requirement | Phase | Status |
|-------------|-------|--------|
| TICK-01 | Phase 117 | Satisfied |
| TICK-02 | Phase 118 | Satisfied |
| TICK-03 | Phase 118 | Satisfied |
| TICK-04 | Phase 119 | Satisfied |
| TICK-05 | Phase 119 | Satisfied |
| PAINT-01 | Phase 120 | Satisfied |
| PAINT-02 | Phase 121 | Satisfied |
| PAINT-03 | Phase 121 | Satisfied |
| PAINT-04 | Phase 122 | Satisfied |
| PAINT-05 | Phase 123 | Satisfied |
| CALIB-01 | Phase 124 | Satisfied |
| CALIB-02 | Phase 125 | Satisfied |
| CALIB-03 | Phase 125 | Satisfied |
| I18N-01 | Phase 127 | Satisfied |
| PROC-01 | Phase 127 | Satisfied |
| CLEAN-01 | Phase 126 | Satisfied |
| REGRESS-01 | Phase 128 | Satisfied |

**Coverage:**
- v4.6 requirements: 17 total
- Mapped to phases: 17
- Satisfied: 17/17 ✓
- Unmapped: 0

---
*Requirements defined: 2026-07-14*
*Last updated: 2026-07-14 after v4.6 requirements gathering*
