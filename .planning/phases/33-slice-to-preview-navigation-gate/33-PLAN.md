# Phase 33 Plan: Slice-to-Preview Navigation Gate

## Objective

Wire the real slice completion path to Preview so the load -> slice -> Preview workflow has a visible completion state.

## Scope

- Update the slice-finished application path so Preview state is populated and the current tab moves to Preview.
- Preserve the existing completion notification behavior.
- Add regression coverage for output path, non-empty preview data, and selected Preview tab.

## Verification

- Focused E2E regression for slice-to-preview navigation.
- Canonical repository verification with `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`.
