# Build Rules

This project has one canonical full verification path.

## Canonical Command

Run full verification with:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

## Canonical Build Directory

Use only:

```text
build/
```

## Rules

- Do not create alternate build directories.
- Do not use alternate full-build scripts.
- Do not bypass `scripts/auto_verify_with_vcvars.ps1` when claiming full build verification.
- Do not clean or delete `build/` unless the user explicitly asks for that operation.
- Documentation-only phases may use lightweight checks instead of a full rebuild, but must say that no full rebuild was run.

## Expected Environment

- Windows 10/11.
- Visual Studio 2022 MSVC toolchain.
- CMake and Ninja.
- Qt 6.10.
- Prebuilt OrcaSlicer dependency prefix as configured by the project CMake files.
- Git on PATH at configure time: the from-source build applies
  `patches/orcaslicer/*.patch` onto the pristine `third_party/OrcaSlicer`
  submodule checkout (see `cmake/ApplyOrcaSlicerPatches.cmake`).

## OrcaSlicer Submodule

- The submodule stays a pristine upstream checkout; the gitlink pins the
  upstream baseline commit and carries no OWzx edits.
- All OWzx compatibility changes live in `patches/orcaslicer/*.patch` and are
  applied to the submodule working tree automatically at configure time.
  Configure is idempotent (fingerprint stamp `.owzx-patches-applied` plus a
  dirty-tree check); an out-of-sync submodule fails configure loudly instead
  of building an unknown tree.
- After changing the patch series, regenerate the baseline state with:
  reset the submodule to the pinned commit, drop untracked files created by
  the old series, and re-run configure.

## Verification Reporting

When reporting verification, state:

- The exact command run.
- Whether configure/build/app smoke/E2E tests passed.
- Whether `ViewModelSmokeTests.exe` was built only or also executed.
- Any environment-dependent checks that were skipped or require manual validation.
