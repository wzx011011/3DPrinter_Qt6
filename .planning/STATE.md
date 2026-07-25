---
gsd_state_version: 1.0
milestone: v5.9
milestone_name: ConfigWizard Multi-Vendor Selection
status: completed
last_updated: "2026-07-25T20:30:00.000Z"
last_activity: 2026-07-25
progress:
  total_phases: 4
  completed_phases: 4
  total_plans: 4
  completed_plans: 4
  percent: 100
---

# Project State

**Latest completed milestone:** v5.9 ConfigWizard Multi-Vendor Selection,
closed on 2026-07-25.

**Completion evidence:** The ConfigWizard is now multi-vendor. Users pick from
46 upstream vendor JSONs (Anker/Anycubic/BBL/Creality/Dremel/Elegoo/...); the
chosen vendor's presets load on demand (loadSingleVendor/loadVendor); the
selection persists across launches (AppConfig-lite QSettings). PresetUpdater
(network) stays deferred. All verification gates PASS:
v56CrossWorkstreamRegressionLocked (incl. new WIZ-03 checks),
mmuSegmentationPaintFeedsSlice, v50PresetIniAndCreateDialogWired,
multiPlateFullStateRoundTrip. See `.planning/milestones/v5.9-MILESTONE-AUDIT.md`.

**Next step:** select the next milestone from the deferred backlog (SLA slice
path now that Hollow editing exists; full 3-way material compatibility
filtering; or H2C/A2L if the product decision + bb3 submodule land).

## Latest Milestone: v5.9 ConfigWizard Multi-Vendor Selection

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
