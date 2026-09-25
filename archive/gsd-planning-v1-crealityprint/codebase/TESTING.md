# Testing Patterns

**Analysis Date:** 2026-05-31

## Test Framework

**Runner:**
- Qt Test (`QtTest`) — the standard Qt unit testing framework
- Header: `<QtTest>`
- Macro: `QTEST_MAIN(TestClassName)`
- Config: No separate test config file; tests are discovered via CMake `include(CTest)`

**Assertion Library:**
- Qt Test built-in: `QVERIFY()`, `QVERIFY2()`, `QCOMPARE()`, `QTRY_VERIFY_WITH_TIMEOUT()`

**Build Integration:**
- `CMakeLists.txt` includes `include(CTest)` at line 47
- The test file `tests/ViewModelSmokeTests.cpp` is NOT currently wired into CMake via `add_test()`
- Tests are built and run manually or as part of the smoke test script

## Test Files

**Single test file:**
- `tests/ViewModelSmokeTests.cpp` (260 lines) — the only unit/integration test file in the repository

**No test directories exist** under `src/` (no co-located tests)

## Test Structure

**Suite organization:**

```cpp
// tests/ViewModelSmokeTests.cpp

#include <QSignalSpy>
#include <QDir>
#include <QFileInfo>
#include <QtTest>

#include "qml_gui/BackendContext.h"
#include "core/services/DeviceServiceMock.h"
// ... more includes

class ViewModelSmokeTests final : public QObject
{
  Q_OBJECT

private slots:
  void editor_import_model_updates_state();
  void preview_receives_slice_progress();
  void slice_reuse_previous_gcode_file();
  void monitor_refresh_updates_network_and_device();
  void config_default_and_switch_preset();
  void topbar_new_save_and_navigation_behaviors();
  void topbar_open_import_with_temp_model_file();
  void topbar_import_3mf_generates_mesh();
  void topbar_delete_selected_object_updates_model_and_mesh();
  void multipplate_switch_and_delete_smoke();
};
```

**Test method naming:**
- snake_case: `editor_import_model_updates_state()`
- Pattern: `<component>_<action>_<expected_result>()`

## Test Patterns

**Setup pattern (inline):**
- No `init()` / `initTestCase()` / `cleanup()` methods used
- Each test creates its own objects inline:
```cpp
void ViewModelSmokeTests::editor_import_model_updates_state()
{
  ProjectServiceMock project;
  SliceService slice(&project);
  EditorViewModel editor(&project, &slice);

  QCOMPARE(editor.modelCount(), 0);
  // ...
}
```

**Teardown pattern:**
- None — objects are stack-allocated and destruct automatically

**Assertion pattern:**
- `QCOMPARE(expected, actual)` for equality checks
- `QVERIFY(condition)` for boolean checks
- `QVERIFY2(condition, message)` with descriptive message for file existence checks
- `QTRY_VERIFY_WITH_TIMEOUT(condition, ms)` for async operations:
```cpp
QTRY_VERIFY_WITH_TIMEOUT(spy.count() > 0, 1500);
QTRY_VERIFY_WITH_TIMEOUT(editor->modelCount() > 0, 10000);
```

**Signal verification:**
- `QSignalSpy` used extensively to verify signals are emitted:
```cpp
QSignalSpy spy(&editor, &EditorViewModel::stateChanged);
QVERIFY(editor.loadFile(stlPath));
QVERIFY(spy.count() >= 1);
```

**Skip pattern:**
- `QSKIP("reason")` for conditional test skipping:
```cpp
if (editor->plateCount() < 2)
  QSKIP("test asset is not multi-plate in current environment");
```

## Mocking

**Framework:** Manual mocking (no mocking library such as Google Mock)

**Approach:**
- Services with `Mock` suffix serve dual purpose: they are both the test doubles AND the production service layer
- `ProjectServiceMock`, `DeviceServiceMock`, `NetworkServiceMock`, `PresetServiceMock` etc. contain real mock data generation and are used directly in both tests and production
- `BackendContext` wires all services and viewmodels together for integration testing

**What is mocked:**
- Device data: `DeviceServiceMock::buildMockDevices()` generates realistic device records
- Network: `NetworkServiceMock` returns canned latency/online values
- Camera: `CameraServiceMock` provides simulated stream data
- Presets: `PresetServiceMock` serves preset list with hardcoded values
- Cloud: `CloudServiceMock` returns mock cloud data

**What is NOT mocked in tests:**
- ViewModel logic — tested as real implementations
- File I/O — tests use real files from `third_party/CrealityPrint/resources/`
- SliceService — tested with real G-code parsing

## Test Data

**Location:** Tests use real upstream resource files as fixtures:
```cpp
const QString stlPath = QDir::cleanPath(
  QStringLiteral(QT_TESTCASE_SOURCEDIR) +
  QStringLiteral("/third_party/CrealityPrint/resources/profiles/hotend.stl")
);
const QString model3mf = QDir::cleanPath(
  QStringLiteral(QT_TESTCASE_SOURCEDIR) +
  QStringLiteral("/third_party/CrealityPrint/resources/calib/arc/arc.3mf")
);
const QString gcodePath = QDir::cleanPath(
  QStringLiteral(QT_TESTCASE_SOURCEDIR) +
  QStringLiteral("/third_party/CrealityPrint/resources/printers/ams_load.gcode")
);
```

**File existence checks:**
- Always verify fixture files exist before use:
```cpp
QVERIFY2(QFileInfo::exists(stlPath), qPrintable(stlPath));
```

## Smoke Tests (PowerShell)

**Script:** `scripts/smoke_test.ps1`
**Purpose:** Post-build validation — not unit tests, but deployment/readiness checks

**Phases:**
1. **Binary exists** — checks `FramelessDialogDemo.exe` exists and > 100KB
2. **Qt DLLs present** — verifies Qt6Core.dll, Qt6Qml.dll, etc. in build directory
3. **App startup** — launches app, verifies it stays alive for N seconds (no crash)
4. **QML warnings** — parses `startup_diagnostics.log` for QML warnings and critical errors
5. **Build artifacts** — checks `.obj` files for all expected compilation units (26 files)
6. **QML resource integrity** — verifies all 29 QML source files exist
7. **Standalone startup** — verifies DLL dependencies are met without vcvars environment

**Expected test count:** 66 checks (26 obj + 29 QML + misc)

**Run command:**
```powershell
powershell -ExecutionPolicy Bypass -File scripts/smoke_test.ps1
```

## QML Warning Capture

**Script:** `scripts/capture_qml_warnings.ps1`
- Captures QML binding warnings and connection warnings at runtime

## Coverage

**Target:** None enforced
- CMake option `ENABLE_COVERAGE` exists (`option(ENABLE_COVERAGE "Enable compiler coverage instrumentation" OFF)`) but is off by default
- No coverage reports generated in CI or locally

## Test Types Present

**Unit tests:**
- Minimal — `ViewModelSmokeTests` is closer to integration tests
- No isolated unit tests for individual service methods

**Integration tests:**
- `ViewModelSmokeTests` — tests full ViewModel + Service wiring
- Tests cross-cutting flows: import model, slice, monitor, config switching
- Uses `BackendContext` for top-level workflow tests

**E2E tests:**
- Not used (no GUI automation framework)

**Visual regression tests:**
- CMake option `ENABLE_VISUAL_REGRESSION` exists but no test implementation detected

## Test Execution

**Qt Test (manual):**
```bash
# Build and run tests (not currently wired to CMake)
cd build
./ViewModelSmokeTests
```

**Smoke test (post-build):**
```powershell
powershell -ExecutionPolicy Bypass -File scripts/smoke_test.ps1
```

**Full verification:**
```powershell
powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1
```
This runs cmake configure + ninja build + smoke test in sequence.

## Test Coverage Gaps

**Untested areas:**
- No unit tests for `ProjectServiceMock` — the largest and most complex service (4563 lines)
- No unit tests for `EditorViewModel` (3500 lines) — tested only through smoke tests
- No unit tests for `UndoCommands` (627 lines) — undo/redo is tested only as integration
- No unit tests for `GLViewportRenderer` (2165 lines) — OpenGL rendering untested
- No unit tests for `ConfigViewModel` (1196 lines)
- No unit tests for any QML component or page
- No unit tests for `SliceService` G-code parsing
- No unit tests for `JobManager` / `JobBase` background job system

**Risk:** High — core business logic (model loading, slicing, undo/redo) has no isolated unit test coverage. Bugs could go undetected until smoke test or manual testing.

**Priority:** High — add unit tests for `UndoCommands`, `ProjectServiceMock` mesh operations, and `SliceService` parsing

---

*Testing analysis: 2026-05-31*
