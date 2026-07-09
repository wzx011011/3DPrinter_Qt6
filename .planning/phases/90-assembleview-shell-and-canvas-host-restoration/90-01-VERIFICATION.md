---
phase: 90-assembleview-shell-and-canvas-host-restoration
plan: 01
verified: 2026-07-09
status: passed
requirements: [ASMSHELL-01, ASMSHELL-02, ASMROUTE-01]
canonical_build_run: true
canonical_build_exit_code: 0
canonical_build_command: "powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1"
---

# Phase 90 Plan 01 Verification Report

## Result

Status: passed.

Phase 90 plan 01 replaced the `Plater.qml` AssembleView placeholder with a real
canvas host + 4-region page shell + navigation toggle + `CanvasAssembleView`
routing, on the default QRhi/D3D11 path. The canonical build produced
`build/OWzxSlicer.exe`, all six test suites passed, and the new
`QmlUiAuditTests::assembleViewShellReplacesPlaceholderAndRegistersCanvasHost`
slot ran green. Prepare/Preview behavior is unchanged (regression-free).

## Requirement Coverage

| Requirement | Status | Evidence |
|---|---|---|
| ASMSHELL-01 | passed | `Plater.qml:106-113` now instantiates `AssemblePage` (visible when `viewMode === vmAssembleView`). The old `assembleSlot` Item, the "装配视图暂不可用" Text, and the "Out of Scope、仅保留枚举入口" comment are gone (`rg -c` returns 0). `AssemblePage.qml` is registered in `qml.qrc:11` and delivers the 4-region chrome (top bar + LeftSidebar + GLViewport + bottom 装配体信息 panel). |
| ASMSHELL-02 | passed | `AssemblePage.qml` hosts a `GLViewport { canvasType: GLViewport.CanvasAssembleView }` bound to `editorVm.meshData`/bed/plate/object, so AssembleView is the third registered canvas host coexisting with Prepare/Preview. Navigation payload and view-mode state are shared (same `editorVm`/`configVm`/`processCategory` as PreparePage; no scene duplication). `BBLTopbar.qml:628-655` adds the 装配视图 toggle calling `backend.requestChangeViewMode(backend.vmAssembleView)`. |
| ASMROUTE-01 | passed | `BackendContext.cpp:389` injects `activeCanvasType` into the shared `EditorViewModel` on every view-mode change (mirrors Plater.cpp:7322/11744/11823). `EditorViewModel::availableGizmoMask()` early-returns 0 when `m_activeCanvasType == 2` (Plater.cpp:11601,11635), and `selectSourceObject()` documents the shared single-stack selection routing. `RhiViewportRenderer.cpp:188,241` widened the basic-mesh guards so AssembleView uploads/draws without regressing the strict `== CanvasPreview` Preview guards (lines 82, 218/224, 271/281). |

## Canonical Build

Command run (the ONLY valid build command per AGENTS.md):

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Result: exit code 0.

Build log milestones (verify run 4, 2026-07-09):

- `[vcvars] Patched Windows Kits 10.0.26100.0 paths into INCLUDE/LIB` (vcvars SDK detection failed; script patched it).
- `-- Configuring done`, `-- Generating done`.
- `[234/237] Linking CXX static library libslic3r_from_source.lib`
- `[235/237] Automatic MOC and UIC for target OWzxSlicer`
- `[236/236] Linking CXX executable OWzxSlicer.exe`
- Test targets relinked fresh:
  - `[3/3] Linking CXX executable E2EWorkflowTests.exe`
  - `[3/3] Linking CXX executable ViewModelSmokeTests.exe`
  - `[4/4] Linking CXX executable PrepareSceneDataTests.exe`
  - `[3/3] Linking CXX executable QmlUiAuditTests.exe`
  - `[3/3] Linking CXX executable PreviewParserTests.exe`
- (owzx-cli + CliTests targets also built.)

No compilation errors from the Phase 90 source changes appeared in any build
log. The MOC step for `OWzxSlicer` succeeded, confirming the new
`Q_PROPERTY`/`Q_INVOKABLE`/member additions in `EditorViewModel.h` and
`BackendContext.h` are meta-object valid.

## Test Results

All six test suites passed inside the canonical verify run:

| Suite | Marker in verify log | Result |
|---|---|---|
| PrepareSceneDataTests | `[PrepareScene] Prepare scene data tests passed` | passed |
| PartPlateTests | `[PartPlate] PartPlate tests passed` | passed |
| ViewModelSmokeTests | `[ViewModel] ViewModel smoke tests passed` | passed |
| QmlUiAuditTests | `[UI] QML UI audit tests passed` | passed |
| PreviewParserTests | `[PreviewParser] PreviewParser tests passed` | passed |
| E2EWorkflowTests | `[E2E] All pipeline tests passed` | passed |

The new `QmlUiAuditTests::assembleViewShellReplacesPlaceholderAndRegistersCanvasHost`
slot compiled and ran as part of `[UI] QML UI audit tests passed`. Test binary
evidence: `build/QmlUiAuditTests.exe` grew from 516096 bytes (00:49 stale) to
526336 bytes (17:03 fresh link), and the suite passed, proving the new slot
executed green against the updated source.

Prepare/Preview regression-free: PrepareSceneDataTests, PartPlateTests, and
PreviewParserTests all passed, confirming Prepare/Preview canvas routing,
geometry, and parser behavior are unchanged by the widened AssembleView render
guards and the new `m_activeCanvasType` routing.

## Source-Audit Checklist

Each item below was verified with `rg` against the shipped source. This mirrors
the assertions encoded in the new test slot.

| # | Check | Result |
|---|---|---|
| A | Placeholder removed from `Plater.qml` (`装配视图暂不可用`, `id: assembleSlot`, `Out of Scope`) | 0 occurrences (removed) |
| B | `AssemblePage` host present in `Plater.qml` with `canvasType: GLViewport.CanvasAssembleView` and `visible: viewMode === vmAssembleView` | `Plater.qml:106,109` present |
| C | `CanvasAssembleView = 2` in `RhiViewport::CanvasType` enum | `RhiViewport.h:67` present (comment cites `GLCanvas3D.hpp:509-513`) |
| D | Render branch widened for AssembleView (basic-mesh upload/draw use `!= CanvasPreview`) | `RhiViewportRenderer.cpp:188,241` |
| E | `pages/AssemblePage.qml` registered in `qml.qrc` | `qml.qrc:11` present |
| F | Navigation toggle in `BBLTopbar.qml` calls `requestChangeViewMode(backend.vmAssembleView)` with 装配视图 label | `BBLTopbar.qml:628,639,644,654` |
| G | `BackendContext` injects `activeCanvasType` on view-mode change and exposes `currentCanvasType()` | `BackendContext.cpp:389`, `BackendContext.h:238` |
| H | `EditorViewModel` has `setActiveCanvasType`/`activeCanvasType`/`m_activeCanvasType = 0` and `availableGizmoMask()` AssembleView early-return | `EditorViewModel.h:726,727,778`, `EditorViewModel.cpp:1735,2003` |
| I | New test slot `assembleViewShellReplacesPlaceholderAndRegistersCanvasHost` declared + defined | `tests/QmlUiAuditTests.cpp:114,2964` |

## Preservation Checklist

The plan required these routing anchors to be preserved unchanged. Verified:

- `vmAssembleView: 2` Q_PROPERTY in `Plater.qml` (used by `AssemblePage.visible`) — preserved.
- `ViewMode::AssembleView = 2` enum entry in `BackendContext.h` — preserved.
- `vmAssembleView()` accessor / `kLastVm` boundary in `BackendContext.cpp` (`setCurrentViewMode` and `requestChangeViewMode`) — preserved on both code paths.
- Preview strict render guards (`== CanvasPreview` at `RhiViewportRenderer.cpp:82,218,224,271,281`) — unchanged.
- Shared `EditorViewModel`/`ProjectServiceMock` model and shared single `UndoRedoManager` stack — unchanged; AssembleView reuses them (no scene/stack duplication).

## Out-of-Scope Guard

No LAN/device/cloud/network, Monitor, ModelMall/Home WebView, camera stream,
D3D12/Vulkan, libslic3r slicing algorithm, Arrange, or `assembleObjects` scope
was reintroduced. Explosion ratio, the Assembly measurement gizmo, and the
AssembleView data pool remain explicitly routed to Phase 91, Phase 92, and
Phase 93 respectively.

## Build Artifact Evidence

```
build/OWzxSlicer.exe        16:44 (fresh, 33693184 bytes)
build/QmlUiAuditTests.exe   17:03 (fresh, 526336 bytes, +10160 vs stale)
build/ViewModelSmokeTests.exe 17:03 (fresh)
build/PrepareSceneDataTests.exe 17:03 (fresh)
build/E2EWorkflowTests.exe  16:46 (fresh)
```

(Phase 90 did not produce a standalone runtime screenshot; per the plan, that
visual evidence belongs to Phase 93. The Phase 90 acceptance bar is the
canonical build + ctest regression + the new test slot passing, all confirmed
above.)

## Conclusion

Phase 90 plan 01 is verified. The AssembleView placeholder is gone, the real
canvas host + page shell + navigation toggle are wired, the `CanvasAssembleView`
routing branches land in BackendContext/EditorViewModel/RhiViewportRenderer,
the new QmlUiAuditTests slot passes, Prepare/Preview is regression-free, and
the canonical verifier exited 0.
