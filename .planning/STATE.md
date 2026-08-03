---
gsd_state_version: 1.0
milestone: v5.11
milestone_name: Process Settings Source-Truth Convergence
status: planning
last_updated: "2026-08-03T07:04:00.805Z"
last_activity: 2026-08-03
progress:
  total_phases: 0
  completed_phases: 0
  total_plans: 0
  completed_plans: 0
  percent: 0
---

# Project State

**Latest completed milestone:** v5.10 Polish & Source-Truth Consolidation,
closed on 2026-07-26.

**SLA permanently removed from scope (user decision 2026-07-26):** the SLA
slice path (SLAPrint, PNG layers, SLA presets) is permanently out of scope.
Hollow drain-hole editing remains (it edits/persists holes and is useful for
FFF models too), but SLA slicing will never be implemented. Do not propose SLA
as a milestone candidate.

**Completion evidence:** Hollow gizmo now refreshes markers on project/plate/
selection change (H1 fix) and renders full cylinders (DrainHole-style). The
filament_colour data channel is established (PresetServiceMock → ProjectServiceMock),
and MMU colour sources are unified (extrudersColors now reads configured
filament colours, matching the swatch UI). ConfigWizard filters materials by
selected printer model. All verification gates PASS:
v56CrossWorkstreamRegressionLocked, mmuSegmentationPaintFeedsSlice,
v50UnsavedChangesAndFilterWired, multiPlateFullStateRoundTrip.

**Next step:** select the next milestone from the deferred backlog. SLA is
permanently removed (see above). Remaining candidates: Hollow per-point
selection, full 3-way material compatibility filtering, H2C/A2L (needs product
decision + bb3 submodule).

## Latest Milestone: v5.10 Polish & Source-Truth Consolidation

**Result:** Consolidated review findings + backlog candidates. Hollow H1 refresh
fixed (markers now rebuild on project/plate/selection change + gizmo enter).
Hollow cylinder rendering (inline cylinder along hole normal, replacing the
16-seg disc). hollowSelectedHoleCount renamed to hollowHoleCount (semantics
clarified). filament_colour data channel (PresetServiceMock.activeFilamentColours
→ ProjectServiceMock.setActiveFilamentColours, synced by EditorViewModel).
MMU colour source unified (extrudersColors reads configured colours). ConfigWizard
3-way material filter (materialsForVendorAndPrinter). SLA permanently removed.

| Phase | Name | Status | Requirement |
|---|---|---|---|
| 220 | Hollow Refresh + Semantics | Complete | HOLLOW-REFRESH |
| 221 | Hollow Cylinder Rendering | Complete | HOLLOW-CYL |
| 222 | filament_colour Data Channel | Complete | FIL-COLOUR |
| 223 | MMU Colour Source Unification | Complete | MMU-UNIFY |
| 224 | ConfigWizard Material Filter | Complete | WIZ-04 |
| 225 | Regression + SLA Removal + Tests | Complete | VER, SLA-REMOVE |

## Previous Milestone: v5.9 ConfigWizard Multi-Vendor Selection

**Result:** The wizard was single-vendor by design (Phase 200 fixed
activeVendor to vendorList[0]). v5.9 makes the vendor user-selectable with
on-demand loading + AppConfig-lite persistence. Backend loadSingleVendor
extracted vendor-agnostically; availableVendorNames scans 46 vendor JSONs;
loadVendor loads on demand; selectedVendor/selectedPrinterModel persist.

| Phase | Name | Status | Requirement |
|---|---|---|---|
| 216 | Multi-Vendor Preset Loading (Backend) | Complete | WIZ-01,02 |
| 217 | AppConfig-lite Persistence | Complete | WIZ-03 |
| 218 | ConfigWizard Multi-Vendor UI | Complete | WIZ-04,05 |
| 219 | Regression + Test Hardening | Complete | WIZ-06, VER-01..04 |

## Project Reference

See:

- `.planning/PROJECT.md`
- `.planning/ROADMAP.md`
- `.planning/REQUIREMENTS.md`
- `.planning/milestones/v5.9-ROADMAP.md`
- `.planning/milestones/v5.9-REQUIREMENTS.md`

**Core value:** OrcaSlicer upstream behavior is the product source of truth.

## Current Position

Phase: Not started (defining requirements)
Plan: —
Status: Defining requirements
Last activity: 2026-08-03 — Milestone v5.11 started
