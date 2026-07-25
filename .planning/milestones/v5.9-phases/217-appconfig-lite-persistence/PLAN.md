# Phase 217 Plan: AppConfig-lite Persistence

**Requirement:** WIZ-03
**Goal:** Persist the wizard's vendor + printer model choice.
**Status:** Complete · **Commit:** `0073308`

## Steps

- [x] selectedVendor()/setSelectedVendor() — QSettings wizard/selectedVendor.
- [x] selectedPrinterModel()/setSelectedPrinterModel() — QSettings
      wizard/selectedPrinterModel.

## Verification

- owzx_app_core builds clean.
