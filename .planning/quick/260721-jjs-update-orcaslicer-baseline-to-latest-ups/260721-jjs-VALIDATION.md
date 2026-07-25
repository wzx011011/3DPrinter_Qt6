---
quick_id: 260721-jjs
status: pending
---

# Validation Strategy

## Wave 0

- `git -C third_party/OrcaSlicer merge-base --is-ancestor 8b93cc5df3347c657ce6ac9a58f6923a21c2959b HEAD`
- `git -C third_party/OrcaSlicer diff --check 8b93cc5df3347c657ce6ac9a58f6923a21c2959b...HEAD`
- `git -C third_party/OrcaSlicer diff --stat 8b93cc5df3347c657ce6ac9a58f6923a21c2959b...HEAD`
- Assert the exact two-file allowlist from `git -C third_party/OrcaSlicer diff --name-only 8b93cc5df3347c657ce6ac9a58f6923a21c2959b...HEAD`.
- `git -C third_party/OrcaSlicer diff 8b93cc5df3347c657ce6ac9a58f6923a21c2959b...HEAD -- src/libslic3r/Config.hpp src/libslic3r/MeshBoolean.cpp`

## Focused Tests

- Add `configEnumNullKeysMapGuards()` to `tests/ViewModelSmokeTests.cpp` and run `build\ViewModelSmokeTests.exe configEnumNullKeysMapGuards` after the canonical build.
- Run the repository's existing ViewModel smoke test through the canonical script; do not claim it passed if only built.

## Final Gate

- Root: `python %USERPROFILE%\.coding-encoding-guard\encoding_guard.py`
- Submodule: `Push-Location third_party/OrcaSlicer; python $env:USERPROFILE\.coding-encoding-guard\encoding_guard.py src/libslic3r/Config.hpp src/libslic3r/MeshBoolean.cpp; Pop-Location`
- `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`
- Verify zero QML warnings and successful app smoke/E2E stages reported by the script.
- `git -C third_party/OrcaSlicer ls-remote origin refs/heads/owzx-upstream-2026-07-21` must return the exact child SHA.
- After creating the local parent gitlink/docs commit, clone that exact local parent commit into a temporary directory, run `git submodule update --init third_party/OrcaSlicer`, and require exit code 0 before publishing the parent commit.
- `rg -n "v7\.0\.1|0d4ac73a6f3224a2bf753d7b9e67d7d515bc8557" AGENTS.md docs/源码真值基线.md docs/源码对照迁移任务追踪.md docs/v3.6-ui-inventory.md docs/架构梳理_快速认知.md docs/架构梳理.html` must produce no active stale-lock claim.
