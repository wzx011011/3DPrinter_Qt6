---
phase: 226-source-mapped-process-hierarchy
reviewed: 2026-08-13
status: fixed
original_review: 226-REVIEW.md
---

# Phase 226 Review Fix

The two findings from the deep review are closed in commit `d80b129`.

## CR-01: Process schema boundary

`ConfigOptionModel::auditProcessManifest()` now compares the ordered Process
manifest with the upstream `Slic3r::Preset::print_options()` collection and
checks every manifest key against `print_config_def`. The ten upstream preset
fields that are intentionally not rendered by `TabPrint::build()` are kept in
an explicit non-UI allowlist. `loadFromUpstreamSchema()` fails closed and logs
the mapping errors instead of silently loading a narrower projection.

## WR-01: no-libs hierarchy behavior

The Process page and group manifest is now compiled independently of
`HAS_LIBSLIC3R`. The fallback constructor assigns page/group metadata to every
matching built-in option, so Process hierarchy APIs continue to return the
source order when the reduced fallback dataset is used. The fallback is
intentionally limited to the existing built-in options; it is not reported as
a complete upstream schema.

## Verification

- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
  completed successfully.
- `ViewModelSmokeTests.exe sourceMappedProcessHierarchyMatchesTabPrint` passed.
- `QmlUiAuditTests.exe processSettingsConsumesSourceMappedHierarchy` passed.
- Full build summary: PrepareSceneData 14 passed; QML UI audit 140 passed;
  E2E workflow 29 passed; no failures or skips.
- `git diff --check` and the UTF-8 encoding guard passed for the fixed source.
