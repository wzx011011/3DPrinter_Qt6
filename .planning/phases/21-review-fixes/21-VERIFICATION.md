---
phase: 21
slug: review-fixes
status: passed
verified: 2026-06-26
requirements: [PLATE-03, PLATE-10]
plans: [21-01]
---

# Phase 21 Verification — Review-Driven Bug Fixes

**Status: passed.** Both review bugs fixed + regression-guarded; merge-direction assumption confirmed.

## Must-haves vs. Codebase

| Must-have | Evidence | Result |
|---|---|---|
| clonePlate mock-mode targets dst (not current) | `ProjectServiceMock.cpp` clonePlate: savedCurrent/setCurrentPlateIndex(dst)/restore + accurate comment | ✓ PASS |
| setPlateScopedOptionValue warns on unknown key | `qWarning("[PartPlate] unknown config key...")` before return false | ✓ PASS |
| clone test asserts current didn't gain objects | `projectServiceClonePlateDeepCopiesObjects`: sourceCountBefore/current assertions | ✓ PASS |
| merge-direction test added (plate wins) | `sliceServiceConfigMergeDirectionPlateWins`: asserts 0.4 after base.apply(plate) | ✓ PASS (0.4 = plate wins) |
| canonical verify exits 0; no regression | exit 0; 44 passed, 0 failed, 1 skipped | ✓ PASS |

## Requirements Affected

| REQ | Before Phase 21 | After Phase 21 |
|---|---|---|
| PLATE-03 (clone deep copy) | Complete (mock-mode target bug undetected) | Complete (fixed + regression guard) |
| PLATE-10 (per-plate config merge) | Complete (silent-drop + untested direction) | Complete (warns + merge direction test-confirmed) |

## Verification Commands Run
- `auto_verify_with_vcvars.ps1` → **exit 0**
- `ViewModelSmokeTests.exe` → **44 passed, 0 failed, 1 skipped** (43 prior + 1 new merge-direction)
- `QmlUiAuditTests.exe` → **7 passed, 0 failed**

## Conclusion
Phase 21 complete. The two P0 review bugs are fixed and regression-guarded, and the D-15 merge-direction assumption is now test-verified (not just comment-asserted). v3.0 is review-clean for the P0 items; remaining review notes (DESIGN-2/3, TEST-1 fixture) are non-blocking v3.1 candidates.
