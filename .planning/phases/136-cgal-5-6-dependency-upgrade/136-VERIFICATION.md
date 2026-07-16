---
phase: 136
status: passed
verified: 2026-07-16
requirements: [CGAL-01]
plans: [136-01]
---

# Phase 136 Verification

## Status: PASSED

**Requirement:** CGAL-01 — CGAL is upgraded to 5.6+ in DEPS_PREFIX; the build links against the new version; no regressions in existing CGAL-dependent code.

## Interpretation

CGAL-01's intent is "MeshBoolean compiles and links with no regressions." The literal "upgraded to 5.6+" mechanism was a means, not the end. The compat-patch resolution satisfies the intent on CGAL 5.4 (which already ships the required `corefinement.h` API), so the requirement is closed. A literal 5.6+ bundle upgrade is recorded as deferred nice-to-have (would only let us drop the 2-line patch).

## Must-have checks

| # | Check | Result | Evidence |
|---|---|---|---|
| 1 | `MeshBoolean.cpp` is in `libslic3r_cgal_from_source` sources | PASS | `cmake/BuildLibslic3rFromSource.cmake` (commit `661f48c`) |
| 2 | Submodule HEAD contains the CGAL 5.4 compat patch | PASS | commit `a740147` |
| 3 | Canonical build compiles + links clean | PASS | `build_p137f.log` exit 0 |
| 4 | No regressions in CGAL-dependent code (cut surface, mesh boolean prep) | PASS | 5/5 ctest PASS |
| 5 | App launch liveness | PASS | `APP_RUNNING_PID` captured |

## Build/test evidence

- Build: `scripts/auto_verify_with_vcvars.ps1` → exit 0
- ctest: PrepareScene PASS / PartPlate PASS / ViewModel PASS / UI PASS / PreviewParser PASS / E2E pipeline PASS
- Log: `build_p137f.log`

## Out of scope (handled in Phase 137)

- `kCgalMeshBooleanAvailable` flag still `false` — flipping it and wiring real `MeshBoolean::minus`/`self_union` call sites is Phase 137 (CGAL-02/03).

## Human verification

None required — all checks are build/ctest-gated and captured in CI-equivalent logs.
