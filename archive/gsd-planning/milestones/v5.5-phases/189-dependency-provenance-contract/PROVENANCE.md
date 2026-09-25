# Phase 189 Provenance: Qt, Upstream Source, and Dependencies

## Qt

- GitHub CI installs Qt with `jurplel/install-qt-action@v4`.
- CI Qt settings are `version: 6.10.0`, `host: windows`, `target: desktop`,
  `arch: win64_msvc2022_64`, `modules: qtshadertools`.
- The local canonical script searches for a prebuilt Qt install in this order:
  `OWZX_QT_ROOT`, `.deps/Qt6.10/6.10.0/msvc2022_64`, `.deps/Qt6.10`,
  `E:\Qt6.10`, `D:\Qt6.10`.
- The selected local Qt root must contain `lib/cmake/Qt6/Qt6Config.cmake`.
- This project consumes Qt as a prebuilt dependency. It does not compile Qt
  from source.
- The verified local v5.5 run selected
  `D:\work\qt\3DPrinter_Qt6\.deps\Qt6.10\6.10.0\msvc2022_64`.

## OrcaSlicer Upstream Source

- The canonical source-truth root is `third_party/OrcaSlicer`.
- The local script falls back to `D:\work\OrcaSlicer` only when the repository
  copy is missing the expected upstream marker file and the local checkout has
  it.
- The selected upstream root is exported to CMake as `OWZX_UPSTREAM_ROOT`.
- v5.5 uses that root only as a source/provenance input. It does not patch
  upstream OrcaSlicer files.
- The verified local v5.5 run selected the fallback
  `D:\work\OrcaSlicer` because the checked-out submodule did not contain the
  required marker file. This is a local checkout difference from CI, not an
  upstream patch.

## Orca Dependency Bundle

- The canonical dependency prefix defaults to
  `E:\ai\3D-Printer\deps\build\OrcaSlicer_dep\usr\local`.
- If the default prefix is missing, the local script falls back to the matching
  dependency bundle under the selected upstream root.
- The selected deps prefix is exported to CMake as `DEPS_PREFIX`.
- The canonical script also copies runtime DLLs from the selected deps prefix
  when present.
- The verified local v5.5 run selected
  `D:\work\OrcaSlicer\deps\build\OrcaSlicer_dep\usr\local`.

## What v5.5 Does Not Do

- It does not change slicer behavior.
- It does not patch upstream OrcaSlicer source.
- It does not build Qt from source.
- It does not create a second build directory.
- It does not introduce a non-canonical verification entry point.
