---
phase: 72-precise-object-picking
status: passed
verified_at: 2026-07-05T00:12:47+08:00
verifier: autonomous (gsd-autonomous --auto/--all)
build_command: "powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1"
requirements: [GPICK-01]
---

# Phase 72 Verification

**Result:** PASSED.

## RED Check

Command:

```powershell
$vcvars='C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
cmd /c "`"$vcvars`" >nul && cmake --build build --target ObjectPickingTests QmlUiAuditTests"
```

Outcome:

- Build failed before implementation because the newly registered
  `ObjectPickingTests` target referenced missing `ObjectPicking.cpp/.h` files.
- This was the intended RED failure for Phase 72.

## Focused Checks

Commands:

```powershell
$vcvars='C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
cmd /c "`"$vcvars`" >nul && cmake --build build --target ObjectPickingTests QmlUiAuditTests OWzxSlicer"
build\ObjectPickingTests.exe -o build\objectpicking.txt,txt
build\QmlUiAuditTests.exe rhiViewportSelectionPickingBridgeStaysCppOwned -o build\qmluiselect.txt,txt
build\QmlUiAuditTests.exe -o build\qmluiaudit.txt,txt
```

Outcome:

- Targeted build passed.
- `ObjectPickingTests` passed: 6 passed, 0 failed.
- Targeted RHI picking audit passed: 3 passed, 0 failed.
- Full `QmlUiAuditTests` passed: 45 passed, 0 failed.
- `OWzxSlicer.exe` linked successfully.

## Hygiene

Commands:

```powershell
git diff --check
python $env:USERPROFILE\.coding-encoding-guard\encoding_guard.py CMakeLists.txt src\core\rendering\ObjectPicking.h src\core\rendering\ObjectPicking.cpp src\core\rendering\GizmoMath.h src\qml_gui\Renderer\RhiViewport.h src\qml_gui\Renderer\RhiViewport.cpp tests\ObjectPickingTests.cpp tests\QmlUiAuditTests.cpp .planning\phases\72-precise-object-picking\72-CONTEXT.md .planning\phases\72-precise-object-picking\72-01-PLAN.md
```

Outcome:

- `git diff --check` passed.
- Encoding guard passed on phase-touched files.

## Canonical Verification

Command:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```

Outcome:

- CMake configure passed in `build/`.
- Full application build passed.
- Scripted checks passed:
  - Prepare scene data tests
  - PartPlate geometry and arrangement tests
  - ViewModel smoke tests
  - QML UI audit tests
  - PreviewParser tests
  - app launch smoke (`APP_RUNNING_PID=19108`)
  - E2E pipeline tests

## Residual Warnings

- Existing third-party/upstream warnings remain: CGAL data-dir warning, Qt
  private module warning, MSVC codepage warnings from Eigen/Boost/CGAL headers,
  numeric conversion warnings in upstream libslic3r, and `QThreadPool` returned
  `QFuture` warnings in existing services.
- These warnings predate Phase 72 and did not block the canonical verifier.

## Requirement Coverage

- `GPICK-01`: covered by `ObjectPicking` ray-AABB + Moller-Trumbore tests, the
  `RhiViewport::pickSourceObjectAt` integration through `GizmoMath::computeRay`,
  the projected-AABB false-positive regression test, and the QML ownership
  audit that rejects `projectBoundsToScreenRect` picking.
