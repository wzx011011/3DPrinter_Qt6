---
phase: 105-d3d12-debug-layer-wiring
status: clean
base: e3fe6a7
head: HEAD
files_reviewed:
  - src/qml_gui/Renderer/RhiBackendSelector.cpp
  - src/qml_gui/main_qml.cpp
  - tests/QmlUiAuditTests.cpp
counts: {critical: 0, warning: 0, info: 2, total: 2}
---

# Phase 105 Code Review — D3D12 Debug Layer Wiring

## Verdict: APPROVE (clean)

All four review priorities pass: DL-04 default-build safety (Pitfall 5), env-flag parsing, qputenv timing, test determinism.

## Verification of Review Priorities

1. **DL-04 default-build safety (Pitfall 5) — PASS (critical):** `RhiProbeOwner::d3d12Params` default-constructed (`enableDebugLayer` defaults false); the only mutation is inside `if (d3d12DebugLayerRequested())`; the probe `QRhi` is a throwaway local destroyed at `probeBackend` return; `qputenv("QSG_RHI_DEBUG", "1")` is itself env-gated. No leak path when OWZX_D3D12_DEBUG is unset. Default-launch liveness corroborates.

2. **Env-flag parsing — PASS:** `d3d12DebugLayerRequested()` is fail-safe across the full input space (unset → false; empty/whitespace → false; "0"/"false"/"off"/"no" → false; "1"/"true"/"on"/"yes" any-case → true). Mirrors `normalizeRequestedBackend` truthy handling.

3. **qputenv timing — PASS:** `qputenv("QSG_RHI_DEBUG", "1")` at main_qml.cpp:285-286 precedes `QGuiApplication app` at :312 by ~26 lines. Qt Quick reads QSG_RHI_DEBUG during scene-graph RHI init, so timing is correct.

4. **Test determinism — PASS:** `d3d12DebugLayerWiredBehindEnvFlag` uses QFile + QT_TESTCASE_SOURCEDIR + QString::contains + position-ordered assertions (enableDebugLayer before QRhi::create; qputenv before QGuiApplication). The DL-04 gate assertion locks `if (d3d12DebugLayerRequested())` literally — a future regression to bare `enableDebugLayer = true;` would fail loudly.

## Findings (2 LOW informational)

| # | Severity | Finding |
|---|----------|---------|
| 1 | info | The probe-path `enableDebugLayer` annotates only the throwaway probe QRhi (validation at device/adapter creation). The live crash fires in `RhiViewportRenderer.cpp:282-298` beginPass, which is annotated by the live-path `QSG_RHI_DEBUG` forwarding. Both paths are needed; both are correctly documented in source comments. Phase 106 readers should not mistake probe-path enablement as sufficient on its own — the live-path forwarding is load-bearing. |
| 2 | info | The dual-path design (probe + live) is the correct engineering choice given the crash fires in the live render path, not at probe QRhi::create. Accurately reflected in DL-03 option (a). |

## Conclusion

Clean infrastructure phase. D3D12-01 closed. No blockers, no highs. The 2 info items are documentation notes for Phase 106 readers, not defects. Regression ctest 4/4 PASS. Default-launch liveness confirmed (no debug-layer leak). Ready for Phase 106 (crash root-cause investigation using the new OWZX_D3D12_DEBUG tooling).
