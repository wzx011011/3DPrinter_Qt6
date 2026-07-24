# Phase 191 Plan: Local App Launch Liveness Gate

**Requirement:** RUN-01
**Goal:** Keep the canonical script's app launch check explicit and repeatable.

## Files

- Modify: `scripts/auto_verify_with_vcvars.ps1`

## Steps

- [x] Confirm the script deploys Qt, MSVC, OCCT, and runtime DLLs before launch.
- [x] Start `OWzxSlicer.exe` from `build/`.
- [x] Wait at least 5 seconds.
- [x] If the process exits early, print `[Launch] OWzxSlicer exited early` and
  return non-zero.
- [x] If the process stays alive, print `APP_RUNNING_PID=<pid>`.
- [x] Stop the launched process so automation does not leave a background app.
- [x] Verify by running:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Expected: when all earlier gates pass, the script prints `APP_RUNNING_PID=...`
and then terminates the process cleanly.
