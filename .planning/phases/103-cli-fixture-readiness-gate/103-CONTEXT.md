# Phase 103: CLI Fixture Readiness Gate - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped via workflow.skip_discuss)

<domain>
## Phase Boundary

Gate the existing argv plumbing (`--load-model`, `--open-page`, `--open-dialog`, `--skip-first-run` at `main_qml.cpp:170-257`) on `QQmlApplicationEngine::objectCreated` + `QQuickWindow::frameSwapped` (not the current `singleShot(0)` trick) so screenshots are deterministic — closing the long-standing Windows-capture-API runtime-evidence blocker (FIXTURE-02).

Success criteria (from ROADMAP):
1. The argv fixture execution waits for `QQmlApplicationEngine::objectCreated` AND `QQuickWindow::frameSwapped` (not just `singleShot(0)`) before applying open-page/open-dialog/load-model requests.
2. A regression test (source-audit or integration) confirms the gate fires in the correct order.
3. The Windows-capture-API runtime-evidence blocker is documented as closed (the gate is the canonical workaround bar per STATE.md).

Requirement: **FIXTURE-02** only. FIXTURE-01 (multi-material fixture model), FIXTURE-03 (recipes), FIXTURE-04 (anti-feature doc) are Phase 104.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — discuss phase was skipped per user setting. Use the ROADMAP phase goal, research SUMMARY.md, and codebase conventions.

### Carry-Forward Inputs (frozen in prior milestones + research)
- **The argv plumbing ALREADY EXISTS** (research STACK.md + ARCHITECTURE.md confirmed). `main_qml.cpp:170-205` `parseStartupOpenRequest` + `:207-257` `applyStartupOpenRequests` + `:349` (the call) is the full fixture-to-BackendContext transport. WS3 is NOT new flags — it's the readiness gate.
- **The current `singleShot(0)` trick is at `main_qml.cpp:213`** (research PITFALLS.md pitfall 4) — insufficient for deterministic screenshots because the QML scene graph may not have rendered a frame yet.
- **The gate fix:** wait for `QQmlApplicationEngine::objectCreated` (engine loaded main.qml) AND `QQuickWindow::frameSwapped` (first frame rendered) before applying open-page/open-dialog/load-model. The combination guarantees both the QML object tree exists AND a frame has been painted.
- **Anti-feature (FIXTURE-04, Phase 104):** these flags are OWzx-only test-evidence plumbing, NOT a user-facing deep-link product feature. The comment in main_qml.cpp already says this; preserve it.
- **v4.5 cross-workstream dep:** WS4 D3D12 crash-repro depends on this gate (deterministic startup state). Phase 103 must land before Phase 106 can repro cleanly.

### Known Phase 103 Scope
1. **Replace the `singleShot(0)` gate** in `applyStartupOpenRequests` with a `QQmlApplicationEngine::objectCreated` + `QQuickWindow::frameSwapped` two-stage wait.
2. **Add a regression test** — source-audit (grep proving the gate uses objectCreated + frameSwapped, not singleShot) is the deterministic-bar pattern (mirrors Phase 102 `wipeTowerReadbackAndRenderAnchorsPresent`).
3. **Canonical verifier + ctest** — production code compiles/links clean; regression ctest 4/4 passes.

</decisions>

<code_context>
## Existing Code Insights

Codebase context will be gathered during plan-phase research. Key anchors:
- `src/qml_gui/main_qml.cpp:170-205` (parseStartupOpenRequest — reads QCommandLineParser values)
- `src/qml_gui/main_qml.cpp:207-257` (applyStartupOpenRequests — the singleShot(0) gate + the apply logic)
- `src/qml_gui/main_qml.cpp:349` (the call site after QQmlApplicationEngine::load)
- `src/qml_gui/main_qml.cpp:170-203` (QCommandLineParser setup — the 4 options already wired)
- `tests/QmlUiAuditTests.cpp` (the source-audit test home — pattern reference from Phase 102)

</code_context>

<specifics>
## Specific Ideas

No specific requirements — discuss phase skipped. Refer to ROADMAP phase description, research SUMMARY.md, and codebase conventions.

</specifics>

<deferred>
## Deferred Ideas

- **FIXTURE-01 (multi-material fixture model), FIXTURE-03 (recipes), FIXTURE-04 (anti-feature doc):** Phase 104. This phase is the readiness GATE only.
- **WS4 D3D12 crash-repro:** Phase 106 depends on this gate landing first.
- LAN/device/cloud/network/Monitor/ModelMall/camera/printer-hardware scope (removed).

</deferred>
