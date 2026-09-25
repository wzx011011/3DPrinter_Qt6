# Phase 106: D3D12 Crash Root-Cause And Backend Readiness - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-generated (discuss skipped)

<domain>
Close D3D12-02 (root cause identified or time-boxed) + D3D12-03 (no default promotion; Vulkan SDK-blocked doc). Phase 105 wired the OWZX_D3D12_DEBUG tooling; Phase 106 uses it to triage the startup `0xc0000005` access violation that fires in the live QQuickRhiItem render path (`RhiViewportRenderer.cpp:282-298` beginPass).

This is an INVESTIGATION phase. "Done" = root cause documented (preferred) OR time-boxed with a documented hypothesis + recommended next steps (acceptable per D3D12-03). NOT a guaranteed code fix.

Success criteria (from ROADMAP):
1. The D3D12 crash root cause is identified OR time-boxed with documented hypothesis + recommended next steps.
2. D3D12-03 documented: default-backend promotion stays out of scope; Vulkan SDK-blocked.

</domain>

<decisions>
### Carry-Forward
- Phase 105: `OWZX_D3D12_DEBUG=1` + `qputenv("QSG_RHI_DEBUG", "1")` enables the D3D12 debug layer + Qt RHI debug output. Use this to repro + triage.
- Phase 105 finding: crash fires in `RhiViewportRenderer.cpp:282-298` beginPass (live render path), NOT at probe `QRhi::create`. The BUG-V31-1 comment at `:283-285` is a leading hypothesis, NOT confirmed — the cited fix is already in place at `:286-296`, so a NEW crash needs NEW isolation.
- Research PITFALLS.md pitfall 5: the BUG-V31-1 comment is a hypothesis; don't assume it's the current root cause.

### Scope
1. **Repro:** Launch `OWzxSlicer.exe` with `OWZX_RHI_RENDERER=d3d12 OWZX_D3D12_DEBUG=1` and capture the debug-layer output + crash dump.
2. **Triage:** Use the debug output to isolate where the access violation fires (which D3D12 API call, which resource). Hypothesize root cause.
3. **Document:** Write a root-cause report (preferred) OR a time-boxed hypothesis + recommended next steps. Cite the debug output.
4. **D3D12-03:** Confirm in PROJECT.md / STATE.md that default-backend promotion stays out of scope until root cause is resolved; Vulkan is SDK-blocked (Qt disables `vulkan`).
5. **No default-backend promotion** regardless of investigation outcome (D3D12-03).

</decisions>

<deferred>
- D3D12 default promotion — future milestone after root cause resolved.
- Vulkan backend — SDK-blocked, evaluation-only.
</deferred>
