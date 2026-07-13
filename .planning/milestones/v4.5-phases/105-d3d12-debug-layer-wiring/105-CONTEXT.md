# Phase 105: D3D12 Debug Layer Wiring - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
Close D3D12-01: enable the D3D12 debug layer behind an env flag in `RhiBackendSelector.cpp` (before `QRhi::create`) so the startup `0xc0000005` access violation (reproducible via `OWZX_RHI_RENDERER=d3d12`) can be triaged with debug output in Phase 106.

This is a small infrastructure change — it does NOT investigate the crash (that's Phase 106). It only wires the debug layer so Phase 106 has the tooling.
</domain>

<decisions>
### Carry-Forward
- Research ARCHITECTURE.md: `QRhiD3D12InitParams` is at `RhiBackendSelector.cpp:22` (a member of `RhiProbeOwner`); the debug layer field is ungated.
- Research STACK.md: the debug layer is enabled via `QRhiD3D12InitParams::enableDebugLayer` (Qt 6.x). No PIX integration; no `WinPixEventRuntime` link.
- D3D12-03: default promotion stays out of scope.

### Scope
1. Set `d3d12Params.enableDebugLayer = true` when a new env flag (e.g. `OWZX_D3D12_DEBUG=1`) is set, BEFORE `QRhi::create`. Apply in BOTH the `probeBackend` path AND the live QQuickRhiItem path (the live path is where the crash happens; probeBackend at :63-87 only creates the QRhi, doesn't render).
2. Add a startup-log line when the debug layer is enabled so the user knows the env flag took effect.
3. Source-audit regression test (QmlUiAuditTests slot) confirming the env-flag gate exists.
</decisions>

<code_context>
- `src/qml_gui/Renderer/RhiBackendSelector.cpp:22` (`QRhiD3D12InitParams d3d12Params;` — the field)
- `src/qml_gui/Renderer/RhiBackendSelector.cpp:80` (`QRhi::create(candidate.implementation, params)` — the probe create site)
- `src/qml_gui/main_qml.cpp:268` (`QQuickWindow::setGraphicsApi(rhiSelection.selectedGraphicsApi)` — the live path selection)
- The live QQuickRhiItem path uses Qt's QSG RHI backend selection (via `QSG_RENDERER_INTERFACE` / `setGraphicsApi`) — the QRhiD3D12InitParams for the LIVE render path comes from QtQuick's private RHI setup, NOT directly from RhiBackendSelector. **Important: the debug layer on the live path is enabled via `QSG_RHI_DEBUG` env or by setting it before QGuiApplication.** Phase 105 must wire BOTH paths or document why one is sufficient.
</code_context>

<deferred>
- D3D12-02 (root cause investigation) — Phase 106.
- D3D12-03 (no default promotion) — doc only.
</deferred>
