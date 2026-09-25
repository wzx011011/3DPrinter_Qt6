# Phase 216 Plan: Multi-Vendor Preset Loading (Backend)

**Requirement:** WIZ-01, WIZ-02
**Goal:** Make loadVendorPresets() vendor-agnostic and expose on-demand load.
**Status:** Complete · **Commit:** `0073308`

## Steps

- [x] Extracted Creality-specific parse into loadSingleVendor(profilesDir,
      vendorFileName) — vendor-agnostic.
- [x] loadVendorPresets() delegates to loadSingleVendor("Creality").
- [x] availableVendorNames() scans profiles/*.json.
- [x] loadVendor(name) on-demand load (idempotent via m_loadedVendors).
- [x] resolveProfilesDir() shared helper.

## Verification

- owzx_app_core builds clean.
