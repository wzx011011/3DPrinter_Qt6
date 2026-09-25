# Phase 239 Summary — Slicing Engine Semantics

**Completed:** 2026-08-16
**Requirements:** ENGN-01..03 — all done.
**Verification:** canonical `auto_verify_with_vcvars.ps1` exit 0 (agent run +
independent re-run before commit; all 8 suites, app launch, E2E 29/29).

## Changes (11 files)

1. **ENGN-01 auto-reslice on preview switch**: `switchToPreview()` rewritten
   from the upstream do_reslice port (Plater.cpp:6165-6234):
   - VALID plate result → ENGN-02 reuse path (below) + pending switch.
   - STALE result + sliceable → arms `m_pendingPreviewAfterSlice`, calls
     `requestSlice()` (auto-reslice); `sliceFinished` completes the switch
     (`previewRequested`), `sliceFailed` clears it. A cancelled reslice
     never switches (E2E-asserted).
   - MISSING/unsliceable → honest hint (kept: existing test pins the
     尚未切片 hint for missing results; the plan's auto-reslice scope is
     stale results).
2. **ENGN-02 G-code reuse path**: valid preview entries load through
   `SliceService::loadGCodeFromPrevious(plateOutputPath)` (off-thread parse;
   upstream BackgroundSlicingProcess.cpp:199-221 semantics) instead of
   reslicing; new `previewReusedPreviousGcode` property + ResultSource::
   PreviousGcode. Fixed in passing: the reuse path now preserves the parsed
   estimated time and prior per-plate labels/layer counts (previously wiped
   to empty). Reuse is cleared by the full invalidation set
   (invalidateSliceResultsForPlate, invalidateAllSliceResults, requestSlice,
   loadFile, clearWorkspace, refreshAfterLoad).
3. **ENGN-03 non-blocking export + validation warnings**:
   - `exportSourceToPath`: unsafe-target validation stays synchronous, the
     1 MiB chunked copy runs in a QtConcurrent worker with byte-based
     progress + chunk-boundary cancel (`cancelExport`); completion via
     queued exportFinished/exportFailed. `exportAllPlateGCodeToDirectory`
     snapshots plates on the GUI thread and copies sequentially in one
     worker. CliRunner waits via QEventLoop on the new signals.
   - Validation warnings: the slice worker calls `print.validate(&warning)`
     (upstream Print.cpp:1063 / warning fill :930-934), aborts only on
     non-empty return, and emits the warning before sliceFinished;
     BackendContext routes `validateWarning` → `postValidateWarning`
     (first caller of that dead method) — warnings land as NotiWarning
     notifications, never NotiError.

## Tests

- ViewModelSmokeTests +4 slots (145 green): stale→auto-reslice→switch;
  reuse flagged + invalidation clears + next entry reslices; export worker
  (4 MiB synthetic, Exporting state, byte progress, committed file,
  Completed); validate warning routed as notification without error state.
- QmlUiAuditTests +engineSemanticsSourceAudit (146 green): function-body
  scoped assertions (pending flag arming, requestSlice call, reuse caller,
  QtConcurrent::run + copyGcodeChunked + cancelExport, print.validate(&w),
  BackendContext validateWarning→postValidateWarning routing).
- E2EWorkflowTests: async export waits + cancelled-reslice-never-switches.

## Honest gates / upstream deltas

- Auto-reslice covers STALE results; MISSING keeps the honest hint (test-
  pinned; plan scope was stale).
- Reuse re-parses the plate G-code per entry — OWzx has no persistent
  in-memory gcode_result like upstream's persistent Print.
- Validate warnings emit on the slice-success path only (upstream also
  validates on config change; no persistent Print exists to validate
  against here).
- Pre-existing out-of-gate CLI failures noted (not this phase, not fixed):
  standalone CliTests testLoadHotend (case assertion drift) and
  testSliceBlock20XY (missing fixture dir); the verify script builds but
  does not run CliTests.
