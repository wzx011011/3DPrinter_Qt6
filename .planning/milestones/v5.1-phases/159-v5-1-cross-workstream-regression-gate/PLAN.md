# Phase 159: v5.1 Cross-Workstream Regression Gate

**Status:** Ready to execute
**Workstream:** Cross-cutting (REGRESS-05)
**Requirement:** REGRESS-05

## Goal

Consolidated v5.1 regression gate. A single `v51RegressionLocked` slot
re-asserts every v5.1 anchor from Phases 154-158 (one per workstream) AND
re-asserts the v5.0/v4.8/v4.7/v4.6 milestone anchors — so v5.1 work did not
regress them. Mirrors the v50RegressionLocked pattern (which is itself
re-asserted here).

## Plan

### Wave 1 — Regression gate

1. `tests/QmlUiAuditTests.cpp`: add `v51RegressionLocked()` private slot.
   One anchor per v5.1 workstream (spot-check, since the per-phase detail slots
   already run individually):
   - **CLOS-01** (Phase 154): `comparePresetsDetailed` proxy exists in ConfigViewModel
   - **CLOS-02** (Phase 155): `attachEmbossMetadata` writes `text_configuration`
   - **CLOS-03** (Phase 156): `setPlateThumbnailFromBase64` write path exists
   - **CLOS-04** (Phase 157): `multiPlateFullStateRoundTrip` live ctest exists
   - **EMBO-F** (Phase 158): `font_prop.boldness = m_embossBoldness` (no longer hardcoded)
   - **v5.0 re-assertion**: `find_package(OpenVDB)`, `MeshBoolean::cgal::intersect`,
     `text2shapes`, `pendingPlateThumbnails_` (spot-checks across all 5 v5.0 WSs)
   - **v4.8 re-assertion**: `kCgalMeshBooleanAvailable = true`, `MeshBoolean::minus`
   - **v4.7 re-assertion**: paint-gate flag, `orientObject`, `its_merge_vertices`
   - **v4.6 re-assertion**: `calibMode = 7` and `calibMode = 9` (tower modes)

## Verification

- Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0
- 5/5 ctest groups PASS (PrepareScene / PartPlate / ViewModel / UI / PreviewParser)
- The new `v51RegressionLocked` slot passes
- All v5.0/v4.x regression slots still pass
