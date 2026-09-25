# Phase 190 Plan: Canonical Script Result Classification

**Requirement:** VERIFY-01
**Goal:** Make the canonical verification script's selected environment and
failure stage obvious from the log.

## Files

- Modify: `scripts/auto_verify_with_vcvars.ps1`

## Steps

- [x] Add or preserve log lines before configure for:
  - repo root
  - build dir
  - Qt root
  - upstream root
  - deps prefix
  - MSVC compiler path
  - Ninja path
- [x] Wrap configure/build/test/deploy/launch sections with clear stage labels
  such as `[Configure]`, `[Build]`, `[Test]`, `[Deploy]`, and `[Launch]`.
- [x] When a required step exits non-zero, print the failing stage before
  returning the same exit code.
- [x] Keep the script as the only canonical build entry point.
- [x] Verify by running:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Expected: the log includes the selected paths and, if it fails, the failing
stage can be identified without reading the whole log.
