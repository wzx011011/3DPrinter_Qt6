# Phase 218 Plan: ConfigWizard Multi-Vendor UI

**Requirement:** WIZ-04, WIZ-05
**Goal:** Make the vendor user-selectable in the wizard.
**Status:** Complete · **Commit:** `0073308`

## Steps

- [x] activeVendor is now a writable property (was readonly fixed to
      vendorList[0]); default restores from selectedVendor().
- [x] onActiveVendorChanged calls presetSvc.loadVendor(activeVendor).
- [x] Vendor picker combo (CxComboBox) at the top of the Printer page; lists
      availableVendorNames(); onActivated sets activeVendor.
- [x] Wizard completion persists selectedVendor + selectedPrinterModel.
- [x] Header comment updated (no longer "single-vendor by design").

## Verification

- owzx_app_core builds clean.
