---
phase: 29
slug: multi-plate-arrangement-grid
status: draft
nyquist_compliant: false
wave_0_complete: false
created: 2026-06-28
---

# Phase 29 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | QtTest (Qt6::Test) — matches existing `tests/PrepareSceneDataTests.cpp`, `tests/ViewModelSmokeTests.cpp` |
| **Config file** | `CMakeLists.txt` (test target registration, pattern at `CMakeLists.txt:372-382`) |
| **Quick run command** | `ctest --test-dir build -R PartPlateTests --output-on-failure` |
| **Full suite command** | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` (the ONLY canonical build+test command per AGENTS.md) |
| **Estimated runtime** | ~60-90s for PartPlateTests alone; ~10-15min for full canonical verify |

---

## Sampling Rate

- **After every task commit:** Run `ctest --test-dir build -R PartPlateTests --output-on-failure` (geometry unit tests are fast; integration tests through arrangeObjects are ~5-10s).
- **After every plan wave:** Run the full canonical `auto_verify_with_vcvars.ps1` (cmake configure + ninja build + ctest all targets).
- **Before `/gsd:verify-work`:** Full suite must be green.
- **Max feedback latency:** ~90s (unit tests) / ~15min (full canonical verify).

---

## Per-Task Verification Map

> Plan-task IDs are placeholders — the planner will assign final IDs. The requirement mapping is authoritative.

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 29-01-* | 01 (geometry) | 1 | ARRANGE-01 | — | N/A (pure geometry, no untrusted input) | unit | `ctest -R PartPlateTests --output-on-failure` | ❌ W0 (tests/PartPlateTests.cpp is NEW) | ⬜ pending |
| 29-01-colum | 01 | 1 | ARRANGE-01 | — | N/A | unit (data-driven) | `ctest -R PartPlateTests -R computeColumCount --output-on-failure` | ❌ W0 | ⬜ pending |
| 29-01-origin | 01 | 1 | ARRANGE-01 | — | N/A | unit | `ctest -R PartPlateTests -R computeOrigin --output-on-failure` | ❌ W0 | ⬜ pending |
| 29-01-index | 01 | 1 | ARRANGE-01 | — | N/A | unit | `ctest -R PartPlateTests -R computePlateIndex --output-on-failure` | ❌ W0 | ⬜ pending |
| 29-02-* | 02 (distribution) | 2 | ARRANGE-02 | — | N/A (no untrusted input; arrangeObjects takes a QString bed string parsed deterministically) | integration | `ctest -R PartPlateTests -R arrangeDistributes --output-on-failure` | ❌ W0 | ⬜ pending |
| 29-03-* | 03 (locked) | 2 | ARRANGE-03 | — | N/A | integration | `ctest -R PartPlateTests -R lockedExclusion --output-on-failure` | ❌ W0 | ⬜ pending |
| 29-04-build | all | 3 | ARRANGE-01,02,03 | — | N/A | build | `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1` | ✅ (script exists) | ⬜ pending |
| 29-05-qmlaudit | all | 3 | (no UI change, regression guard) | — | N/A | integration | `ctest -R QmlUiAuditTests --output-on-failure` | ✅ | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

- [ ] `tests/PartPlateTests.cpp` — NEW file. QtTest class with private slots for: `computeColumCount_data()`/`computeColumCount()` (data-driven, 1..36 parity table), `computeOriginGridMath()`, `computePlateIndexRoundTrip()`, `updatePlateOriginsWritesToPlates()`, `arrangeDistributesAcrossPlates()` (ARRANGE-02), `lockedPlateExclusion()` (ARRANGE-03), `allLockedReturnsFalse()` (D-29-12 edge case).
- [ ] `CMakeLists.txt` — register `PartPlateTests` target following the `PrepareSceneDataTests` pattern (`CMakeLists.txt:372-382`): `qt_add_executable`, `target_include_directories(... PRIVATE src)`, `target_link_libraries(... PRIVATE Qt6::Test)` + the PartPlate/PartPlateList object sources (or `owzx_app_core` if that's the aggregation target), `add_test`, `set_tests_properties(... ENVIRONMENT "PATH=...Qt6.10/bin")`.
- [ ] Verify the canonical build script builds the new test target (`scripts/auto_verify_with_vcvars.ps1` has an `Invoke-NinjaTarget` helper — confirm `PartPlateTests` is built or relies on `ninja` default target).

*Existing infrastructure (QtTest, ctest integration) covers the framework need. Only the test file + CMake registration is new.*

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Multi-plate visual rendering after arrange | ARRANGE-02 (visual confirmation) | Rendering is out of Phase 29 scope (RESEARCH §8 item 10: GLViewport does NOT yet read `origin()`). Data-layer correctness is unit/integration tested; visual confirmation deferred to a future GL phase. | N/A in Phase 29 — data layer verified by integration test asserting plate count + membership distribution. |

*All Phase 29 in-scope behaviors (ARRANGE-01/02/03) have automated verification. The only manual item is visual rendering, which is explicitly out of scope.*

---

## Validation Sign-Off

- [ ] All tasks have `<automated>` verify or Wave 0 dependencies
- [ ] Sampling continuity: no 3 consecutive tasks without automated verify
- [ ] Wave 0 covers all MISSING references (tests/PartPlateTests.cpp + CMake registration)
- [ ] No watch-mode flags
- [ ] Feedback latency < 90s (unit) / < 15min (canonical verify)
- [ ] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
