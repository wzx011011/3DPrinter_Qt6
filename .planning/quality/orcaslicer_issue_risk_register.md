# OrcaSlicer Issue Risk Register

Purpose: convert upstream OrcaSlicer incidents into OWzx Qt6/QML design
constraints and verification targets. This is not a feature backlog. It is an
avoidance register for local layers we reimplemented around the inherited
libslic3r engine.

Last updated: 2026-06-27

## Scope Split

- Inherited engine risk: bugs inside libslic3r slicing algorithms should be
  tracked as inherited behavior unless a task explicitly authorizes engine
  changes.
- Local implementation risk: Qt/QML shell, viewmodels, OpenGL/software
  viewport, G-code preview parsing/rendering, task scheduling, cancellation,
  status propagation, multi-plate aggregation, networking, and calibration
  orchestration must use upstream issues as design constraints.

## Register

| Upstream issue | Risk theme | Impacted OWzx modules | Design constraint | Verification route | Priority |
|---|---|---|---|---|---|
| [#14338](https://github.com/OrcaSlicer/OrcaSlicer/issues/14338) | G-code preview can diverge after post-processing | `PreviewViewModel`, `GCodeRenderer`, export flow | Preview must rebuild from final exported G-code when post-processing/export transformations are possible. | Automated parser test with a post-processed final G-code fixture; manual visual check until fixture exists. | P0 |
| [#10199](https://github.com/OrcaSlicer/OrcaSlicer/issues/10199) | Temperature preview metadata can show `0` while G-code is correct | `PreviewViewModel`, preview legend/tool tooltip, `GCodeRenderer` color modes | Preview metadata must be parsed from final G-code commands and must distinguish unknown from zero. | Automated G-code parser tests for temperature, fan, speed, width, and tool metadata. | P0 |
| [#14419](https://github.com/OrcaSlicer/OrcaSlicer/issues/14419) | All Plates Stats incorrect with shared instances across plates | `SliceService`, `EditorViewModel`, PartPlate aggregation | Stats and slice results must use plate identity plus object instance identity, not only object name/shared model. | Automated multi-plate stats test with shared object instances across plates. | P0 |
| [#14336](https://github.com/OrcaSlicer/OrcaSlicer/issues/14336) | Slice all plates can fail or skip expected work | `EditorViewModel::m_sliceAllQueue`, `SliceService::startSlicePlate` | Slice-all scheduling must define behavior for locked, non-printable, failed, and cancelled plates. | Automated queue/state test for all plate categories and partial failure. | P0 |
| [#10281](https://github.com/OrcaSlicer/OrcaSlicer/issues/10281) | Slim tree supports can freeze slicing at toolpath optimization | `SliceService`, slice progress UI, cancellation path | Inherited engine hangs must not freeze the Qt UI; cancellation and terminal state recovery are required. | Automated cancellation state test; manual long-running support-slice check with large model fixture. | P1 |
| [#11698](https://github.com/OrcaSlicer/OrcaSlicer/issues/11698) | Complex/fuzzy models can trigger memory allocation failures during slice/preview | `PreviewViewModel`, `GCodeRenderer`, viewport data upload | Preview must cap, chunk, stream, or fall back to lite mode for large G-code/segment data. | Large synthetic G-code parser/render smoke; manual GPU memory matrix. | P1 |
| [#6433](https://github.com/OrcaSlicer/OrcaSlicer/issues/6433), [#8145](https://github.com/OrcaSlicer/OrcaSlicer/issues/8145), [#10090](https://github.com/OrcaSlicer/OrcaSlicer/issues/10090) | OpenGL/driver/display-server failures can blank or crash 3D view | `main_qml.cpp`, `GLViewport`, `SoftwareViewport` | Keep software/no-OpenGL fallback; app startup and navigation cannot depend on successful OpenGL FBO rendering. | Startup smoke in default software mode; manual `OWZX_OPENGL=1` GPU/driver matrix. | P1 |
| [#3686](https://github.com/OrcaSlicer/OrcaSlicer/issues/3686), [#7023](https://github.com/OrcaSlicer/OrcaSlicer/issues/7023) | Variable/adaptive layer-height UI can crash when renderer data is incomplete | layer-height viewmodel/UI, preview layer range, viewport interaction | Layer-height tools must validate empty/missing layer data and fail visibly without crashing. | Automated empty-layer-data test; manual adaptive-layer workflow once migrated. | P1 |
| [#9144](https://github.com/OrcaSlicer/OrcaSlicer/issues/9144), [#4885](https://github.com/OrcaSlicer/OrcaSlicer/issues/4885) | Pressure Advance calibration can crash for specific profiles | `CalibrationServiceMock`, `CalibrationViewModel`, `SliceService` calibration injection | Calibration slices must validate profile compatibility and reset calibration config after success/failure/cancel. | Automated calibration config reset/error test; manual PA Tower with non-default machine profiles. | P1 |
| [#12919](https://github.com/OrcaSlicer/OrcaSlicer/issues/12919), [#10756](https://github.com/OrcaSlicer/OrcaSlicer/issues/10756) | Device/network tabs can freeze or crash when connecting to printer UIs | `MonitorViewModel`, `DeviceServiceMock`, camera/network services | Device page network work must be asynchronous, cancellable, and isolated from core prepare/preview UI. | Automated service timeout/cancel test; manual live device matrix. | P2 |
| [#7428](https://github.com/OrcaSlicer/OrcaSlicer/issues/7428) | Feature interactions can change G-code density and preview expectations | config merge, `SliceService`, preview statistics | Config merge must preserve upstream precedence and expose when feature combinations alter output volume/performance. | Config merge regression tests plus slice fixture comparison when available. | P2 |

## Promotion Rules

- P0/P1 entries must become design constraints before related migration work is
  implemented.
- Deterministic P0 entries should become automated tests before the relevant
  workflow is marked Real.
- Environment-dependent entries may remain manual, but the fallback design must
  be implemented and documented.
- If a new upstream issue maps to an already listed risk, update the existing
  row instead of creating a duplicate theme.
