---
phase: 70-rotate-scale-gizmos
status: passed
verified_at: 2026-07-04T21:08:00+08:00
verifier: autonomous (gsd-autonomous --auto/--all)
build_command: "powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1"
requirements: [GROT-01, GROT-02, GSCA-01, GSCA-02]
---

# Phase 70 Verification

**Result:** PASSED.

## RED Check

Command:

```powershell
$vcvars='C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
cmd /c "`"$vcvars`" >nul & set"
cmake --build build --target ViewModelSmokeTests QmlUiAuditTests
```

Outcome:

- First plain-shell build attempt failed before code compilation because MSVC
  standard-library include paths were not loaded (`type_traits` missing).
- Re-running through `vcvars64.bat` reached the intended RED failure:
  `ViewModelSmokeTests.cpp` failed to compile because `EditorViewModel` had no
  `beginGizmoRotateDrag`, `applyGizmoRotateDelta`, `endGizmoRotateDrag`,
  `beginGizmoScaleDrag`, `applyGizmoScaleFactor`, or `endGizmoScaleDrag`
  methods.

## Focused Checks

Command:

```powershell
$vcvars='C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
cmd /c "`"$vcvars`" >nul & set"
$env:CMAKE_PREFIX_PATH='E:\Qt6.10'
$env:Qt6_DIR='E:\Qt6.10'
$env:PATH='E:\Qt6.10\bin;' + $env:PATH
cmake --build build --target ViewModelSmokeTests QmlUiAuditTests
$env:QT_FORCE_STDERR_LOGGING='1'
Push-Location build
.\ViewModelSmokeTests.exe gizmoRotateDragCoalescesIntoSingleUndoCommand -v1
.\ViewModelSmokeTests.exe gizmoScaleDragCoalescesIntoSingleUndoCommand -v1
.\QmlUiAuditTests.exe rhiRotateScaleGizmoBridgeStaysCppOwned -v1
Pop-Location
```

Outcome:

- Targeted build passed.
- `ViewModelSmokeTests::gizmoRotateDragCoalescesIntoSingleUndoCommand` passed.
- `ViewModelSmokeTests::gizmoScaleDragCoalescesIntoSingleUndoCommand` passed.
- `QmlUiAuditTests::rhiRotateScaleGizmoBridgeStaysCppOwned` passed.

## Canonical Verification

Command:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Outcome:

- CMake configure passed in `build/`.
- `OWzxSlicer.exe` built successfully.
- Test targets built successfully, including `ViewModelSmokeTests` and
  `QmlUiAuditTests`.
- Scripted checks passed:
  - Prepare scene data tests
  - PartPlate geometry and arrangement tests
  - ViewModel smoke tests
  - QML UI audit tests
  - PreviewParser tests
  - app launch smoke (`APP_RUNNING_PID=19240`)
  - E2E pipeline tests

The build still emits pre-existing third-party/upstream warnings, including
codepage and numeric-conversion warnings from Eigen, Boost, CGAL/libslic3r,
Qt private-module warnings, and `QThreadPool::start` return-value warnings.
No Phase 70-owned compile error remains.

## Hygiene

Commands:

```powershell
git diff --check
python $env:USERPROFILE\.coding-encoding-guard\encoding_guard.py .planning\phases\70-rotate-scale-gizmos\70-CONTEXT.md .planning\phases\70-rotate-scale-gizmos\70-01-PLAN.md src\qml_gui\Renderer\RhiViewportRenderer.h src\qml_gui\Renderer\RhiViewportRenderer.cpp src\qml_gui\Renderer\RhiViewport.h src\qml_gui\Renderer\RhiViewport.cpp src\core\viewmodels\EditorViewModel.h src\core\viewmodels\EditorViewModel.cpp src\qml_gui\pages\PreparePage.qml tests\ViewModelSmokeTests.cpp tests\QmlUiAuditTests.cpp
```

Outcome:

- `git diff --check` passed.
- Encoding guard passed for touched Phase 70 files.

## Review

Inline review found and fixed:

- QML initially chose the drag-end viewmodel method from the current toolbar
  mode. It now records `activeGizmoDragMode` at drag begin and uses that
  stable mode at drag end.
- ViewModel rotate/scale apply methods now reject non-finite radian/factor
  inputs before mutating object state.
