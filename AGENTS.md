# 3DPrinter_Qt6 Codex Instructions

This repository is a source-truth migration project from OrcaSlicer to Qt6/QML (brand: OWzx).

See @.codex/rules/source-truth-migration.md for the canonical project migration rules.
See @.codex/rules/build-rules.md for the canonical build rules.

Screenshot-driven UI milestones use screenshots as visual/layout truth and OrcaSlicer source as behavior truth. If an existing page is materially off-design, replace it and remove the old files, routes, registrations, tests, imports, and resources instead of keeping deprecated UI code.

## Project Skills

- Use `/migrating-source-truth` to continue the next recorded migration task under the project rules. Append `all` for continuous batch mode.
- Use `/analyzing-source-truth-gap <task-or-feature>` to perform a read-only upstream-to-Qt gap analysis before implementation.

## Build

**唯一构建命令：** `powershell -ExecutionPolicy Bypass -File scripts/auto_verify_with_vcvars.ps1`

**唯一构建目录：** `build/`

不得创建其他构建目录，不得使用其他构建脚本。详见 `.codex/rules/build-rules.md`。

<!-- GSD:project-start source:PROJECT.md -->
## Project

**OWzx Slicer — OrcaSlicer Qt6/QML Migration**

将 OrcaSlicer 开源 3D 打印切片软件从 C++/wxWidgets 迁移为 C++/Qt6/QML 架构（品牌名 OWzx）。保留底层切片引擎（libslic3r）不变，重写整个 GUI 层，对齐上游全部用户可见行为和工作流。

**Core Value:** 上游 OrcaSlicer 源码为功能真值——Qt6 代码必须完整继承上游行为，不得自由设计新的产品行为。

### Constraints

- **Tech Stack**: C++17 / Qt 6.10 / QML / CMake / Ninja / MSVC — Windows 10 构建环境
- **Upstream Lock**: 官方 `upstream/main` 锁定为 `8b93cc5df3347c657ce6ac9a58f6923a21c2959b`；OWzx 本地兼容子提交为 `4cb3b9ce792f15c38fabfb4bb9700895d32b1166`，不得自由设计新行为
- **Build Command**: 唯一构建命令 `scripts/auto_verify_with_vcvars.ps1`，唯一构建目录 `build/`
- **Architecture**: 业务逻辑在 core/，QML 仅做呈现，不得在 QML 内联脚本中承载业务逻辑
- **Dependency**: CGAL 可用（libslic3r_cgal），OpenVDB 不可用，FFmpeg 不可用
- **Platform**: 当前仅 Windows，上游同时支持 macOS/Linux
<!-- GSD:project-end -->

<!-- GSD:stack-start source:codebase/STACK.md -->
## Technology Stack

## Languages
- C++17 - All native backend code, viewmodels, services, rendering, and the main Qt application. Used in `src/core/`, `src/qml_gui/`, `third_party/OrcaSlicer/src/libslic3r/`.
- QML/JavaScript - All UI layer. Used in `src/qml_gui/pages/`, `src/qml_gui/components/`, `src/qml_gui/controls/`, `src/qml_gui/dialogs/`, `src/qml_gui/panels/`.
- C - Embedded dependencies compiled from source: miniz, qoi, semver, glu-libtess, mcut. Located in `third_party/OrcaSlicer/deps_src/`.
- Objective-C++ - macOS-specific sources in upstream (e.g., `MacUtils.mm`, `ModelIO.mm`). Not used in the Qt6 migration on Windows.
- PowerShell - Build scripts (`scripts/auto_verify_with_vcvars.ps1`, `scripts/smoke_test.ps1`, etc.).
- CMake - Build system configuration in `CMakeLists.txt`, `cmake/`.
## Runtime
- Target platform: Windows 10/11 (win64), MSVC toolchain (Visual Studio 2022)
- CRT: Dynamic CRT (`/MD`) for the Qt6 executable; upstream libslic3r static libs use static CRT (`/MT`)
- MSVC runtime: `MultiThreaded$<$<CONFIG:Debug>:Debug>DLL` (CMP0091 policy)
- Build output: `build/OWzxSlicer.exe`
- vcpkg for external dependency resolution (`vcpkg.json`)
- Lockfile: `vcpkg.json` baseline `6f29f12e82a8293156836ad81cc9bf5af41fe836`
- Upstream pre-built dependencies consumed from `E:/ai/3D-Printer/deps/build/OrcaSlicer_dep/usr/local` (via `DEPS_PREFIX`)
- CMake 3.16+ minimum, CMake 3.21+ for Qt 6.10
- Generator: Ninja
- Build type: Release
- CMake options: `BUILD_LIBSLIC3R=ON`, `LIBSLIC3R_FROM_SOURCE=ON`, `OWZX_QML_GUI=ON`
- Precompiled headers enabled for libslic3r (`pchheader.hpp`)
## Frameworks
- Qt 6.10 - QML-first GUI architecture. Components used: Qt6::Qml, Qt6::Quick, Qt6::QuickControls2, Qt6::OpenGL, Qt6::Concurrent, Qt6::LinguistTools.
- Qt Quick Controls 2 "Basic" style (`src/qml_gui/qtquickcontrols2.conf`).
- QSGRendererInterface::OpenGL forced as graphics API (required for `QQuickFramebufferObject`).
- OpenGL (legacy GL) via Qt OpenGL module and custom `QQuickFramebufferObject` subclass (`src/qml_gui/Renderer/GLViewport`).
- Custom shader utilities in `src/core/rendering/GLShaderUtil`.
- wxWidgets 3.x - The original OrcaSlicer GUI framework.
- GLEW - OpenGL extension loading in upstream.
- ImGui - Immediate mode GUI used in upstream for certain overlays.
- Qt Test (`QtTest`) - Smoke tests in `tests/ViewModelSmokeTests.cpp`, using `QSignalSpy` for signal verification.
- CTest integration enabled in root `CMakeLists.txt` (`include(CTest)`).
- vcpkg for dependency management
- CMake with Ninja generator
- PowerShell for build automation scripts
## Key Dependencies
- Boost 1.84+ (filesystem, algorithm, spirit, log, property_tree, beast, thread, locale, regex, chrono, atomic, date-time, iostreams, program-options, nowide, assign, bimap, multi-index, geometry, endian, foreach, format, functional, lexical-cast, multiprecision, polygon, process, signals2, stacktrace, uuid) - Core infrastructure for libslic3r and slicer algorithms
- TBB (Threading Building Blocks) - Parallel computation for slicing and mesh processing
- OpenSSL - HTTPS/TLS for cloud communication (NetworkAgent), certificate handling
- CGAL - Computational geometry: mesh boolean ops, triangulation, cut surface (linked as separate `libslic3r_cgal` target)
- cereal - C++11 serialization (project files, configuration persistence)
- NLopt 1.4+ - Nonlinear optimization (used in slicing algorithms)
- zlib - Compression (STL processing, model I/O)
- libpng - PNG image handling
- libjpeg-turbo - JPEG image handling
- Expat - XML parsing
- admesh - STL mesh repair and processing
- clipper (v1) + clipper2 - Polygon clipping/offsetting
- glu-libtess - Tessellation
- mcut - Mesh cutting
- miniz - zlib replacement
- qoi - QOI image format
- semver - Semantic version parsing
- libnest2d - 2D bin packing (pre-built from upstream build)
- qhull / qhullcpp - Convex hull computation
- assimp - 3D model import (STL, OBJ, STEP, etc.)
- libigl - Geometry processing library
- cr_tpms - Closed-source TPMS infill library (delay-loaded DLL; DLL deleted at deploy time, effectively unavailable)
- Eigen - Linear algebra
- nlohmann/json - JSON parsing
- nanosvg - SVG parsing
- spline - Spline math
- stb_dxt - DXT texture compression
- ankerl/unordered_dense - Fast hash maps
- OpenVDB - Link failure; required for hollow/support paint gizmos (HMS/VDB-dependent)
- FFmpeg - Not found; required for RTSP camera stream decoding (blocked)
- MetaRTC/WebRTC - Required for WebRTC camera streams (blocked)
- Paho MQTT C++ - Used in upstream for device communication (not yet migrated)
## Configuration
- `CMAKE_PREFIX_PATH` - Must include Qt 6.10 path and pre-built deps path (`DEPS_PREFIX`)
- `Qt6_DIR` - Points to Qt 6.10 installation (`E:/Qt6.10` on local dev, installed via `jurplel/install-qt-action@v4` in CI)
- `DEPS_PREFIX` - Pre-built dependencies from upstream OrcaSlicer build (default: `E:/ai/3D-Printer/deps/build/OrcaSlicer_dep/usr/local`)
- `QML_DEBUG_LOG` - Set to enable QML debug logging to `startup_diagnostics.log`
- `QT_LOGGING_RULES` - Hardcoded to `qt.qml.binding=true;qt.qml.connections=true` in `src/qml_gui/main_qml.cpp`
- Root config: `CMakeLists.txt`
- libslic3r build: `cmake/BuildLibslic3rFromSource.cmake` (compiles from upstream source)
- Embedded deps build: `cmake/BuildDepsFromSource.cmake` (compiles miniz, qoi, mcut, etc. from source)
- Pre-built libslic3r import: `cmake/BuildLibslic3r.cmake` (alternative, not used by default)
- QML resource file: `src/qml_gui/qml.qrc`
- QML type registration: `src/qml_gui/qmldir`
- Qt Quick Controls 2 config: `src/qml_gui/qtquickcontrols2.conf`
- Generated headers: `libslic3r_version.h`, `buildinfo.h` (configured by CMake)
- `OWZX_QML_GUI=1` - QML GUI mode
- `HAS_LIBSLIC3R=1` - libslic3r available (when `BUILD_LIBSLIC3R=ON`)
- `BOOST_ALL_NO_LIB`, `BOOST_USE_WINAPI_VERSION=0x602`, `BOOST_SYSTEM_USE_UTF8` - Static Boost linking on MSVC
- `USE_TBB`, `TBB_USE_CAPTURED_EXCEPTION=0` - TBB configuration
- `NOMINMAX`, `UNICODE`, `_UNICODE` - Windows Unicode setup
- `/NODEFAULTLIB:LIBCMT`, `/NODEFAULTLIB:libcpmt` - Exclude static CRT to avoid conflict
- OCCT (TK*.dll) libraries: **linked statically (load-time import), NOT delay-loaded**. Delay-load was abandoned because it caused `TDataStd_GenericEmpty::Paste` recursive crashes (`__pfnDliNotifyHook2` was never linked into the exe). TK*.dll are deployed normally. See `cmake/BuildLibslic3rFromSource.cmake:599-679`.
- `/DELAYLOAD:cr_tpms_library.dll` - TPMS library delay-loaded (closed-source; DLL deleted at deploy time, effectively unavailable)
- `delayimp.lib` - MSVC delay-load helper (only for cr_tpms now)
- Qt Linguist translations: `i18n/zh_CN.ts`, `i18n/en.ts`, `i18n/ja.ts`, `i18n/ko.ts`, `i18n/de.ts`, `i18n/fr.ts`
- QTranslator used for runtime language switching
- `engine->retranslate()` called on language change
## Platform Requirements
- Windows 10/11 (64-bit)
- Visual Studio 2022 with MSVC toolchain (vcvars64.bat)
- Qt 6.10 (Qt6::Qml, Qt6::Quick, Qt6::QuickControls2, Qt6::OpenGL, Qt6::Concurrent, Qt6::LinguistTools)
- CMake 3.16+ (3.21+ for Qt 6.10)
- Ninja build system
- vcpkg for dependency management
- Pre-built upstream dependencies from OrcaSlicer build
- OpenGL-capable GPU and drivers
- Windows 10/11 desktop application (WIN32_EXECUTABLE)
- Distributed as standalone `.zip` with `windeployqt`-collected DLLs
- OCCT (TK*.dll) deployed normally in dev build; only excluded from release zip (load-time import, 38 DLLs present in `build/`); OCCT is available for STEP/SVG/text-shape import. Mesh boolean / cut surface use CGAL (not OCCT) and are excluded due to CGAL version (need 5.6+, have 5.4).
- Qt deployment: `windeployqt --release --qmldir src` in CI
<!-- GSD:stack-end -->

<!-- GSD:conventions-start source:CONVENTIONS.md -->
## Conventions

## Language & Standard
- **Language:** C++17 (`set(CMAKE_CXX_STANDARD 17)`) for all `src/core` and `src/qml_gui` C++ code
- **QML:** Qt 6.10 declarative UI (`import QtQuick`, `import QtQuick.Controls`)
- **C Standard:** C11 (used for `nanosvg_impl.cpp`; `DelayLoadHook.cpp` exists on disk but is **dead code** in the FromSource build — not compiled/linked, see `cmake/BuildLibslic3rFromSource.cmake:612-619`)
## Naming Patterns
### Files
- PascalCase matching the primary class: `EditorViewModel.h`, `ProjectServiceMock.h`, `GLViewport.h`, `BackendContext.h`
- One class per file (header + implementation pair)
- Exact match with header: `EditorViewModel.cpp`, `BackendContext.cpp`
- PascalCase with descriptive suffix: `PreparePage.qml`, `ConfigPage.qml`, `MonitorPage.qml`
- Custom controls prefixed with `Cx`: `CxButton.qml`, `CxComboBox.qml`, `CxSlider.qml`, `CxSpinBox.qml`, `CxTextField.qml`, `CxCheckBox.qml`, `CxIconButton.qml`, `CxPillAction.qml`
- Dialogs suffixed with `Dialog`: `PrintDialog.qml`, `CalibrationDialog.qml`, `BedShapeDialog.qml`
- Panels suffixed with panel purpose: `Sidebar.qml`, `ObjectList.qml`, `PrintSettings.qml`, `SliceProgress.qml`
### Classes
- **ViewModels:** PascalCase, suffix `ViewModel`: `EditorViewModel`, `PreviewViewModel`, `MonitorViewModel`, `ConfigViewModel`, `HomeViewModel`, `SettingsViewModel`, `ProjectViewModel`, `CalibrationViewModel`, `ModelMallViewModel`, `MultiMachineViewModel`
- **Services:** PascalCase, suffix `ServiceMock` (even when containing real libslic3r integration): `ProjectServiceMock`, `DeviceServiceMock`, `NetworkServiceMock`, `PresetServiceMock`, `SliceService`, `CameraServiceMock`, `CloudServiceMock`, `CalibrationServiceMock`
- **Undo commands:** PascalCase, suffix `Command`: `TransformCommand`, `DeleteObjectsCommand`, `AddObjectCommand`, `SelectionCommand`, `RenameCommand`, `CutCommand`, `BooleanCommand`, `DrillCommand`, `SimplifyCommand`, `AddVolumeCommand`
- **Renderer classes:** PascalCase, descriptive: `GLViewport`, `GLViewportRenderer`, `GCodeRenderer`, `CameraController`
- **QML models:** PascalCase, suffix `Model`: `ConfigOptionModel`, `PresetListModel`
- **Utility classes:** PascalCase with namespace: `GLShaderUtil` (in `namespace GLShaderUtil`)
### Functions
- camelCase: `loadFile()`, `setCurrentPlateIndex()`, `objectCount()`, `selectedObjectIndex()`
- Getter accessors match property name exactly: `progress()`, `slicing()`, `modelCount()`
- Setter accessors prefixed with `set`: `setCutAxis()`, `setCutPosition()`, `setDrillRadius()`
- QML-invokable actions use verb phrases: `requestSlice()`, `cancelSlice()`, `switchToPreview()`, `duplicateSelectedObjects()`
### Variables
- **Private member variables:** snake_case with trailing underscore: `projectService_`, `sliceService_`, `statusText_`, `m_cachedMeshData`, `m_undoManager`, `m_fitHint`
- **Two styles coexist** (historical): older code uses `name_` suffix, newer code uses `m_name` prefix. Both are acceptable in this codebase.
- **Local variables:** camelCase: `stlPath`, `beforeState`, `gcodePath`
- **Constants:** `constexpr` for compile-time, `k` prefix or `UPPER_CASE`: `kBboxBytes`, `CanvasView3D`, `GizmoMove`
- **Enums:** PascalCase for type, PascalCase for values: `enum GizmoMode { GizmoMove = 0, GizmoRotate = 1, ... }`
### Types & Structs
- PascalCase: `MockDevice`, `MockAmsSlot`, `MockHmsItem`, `MockVolumeEntry`, `MockLayerRange`, `ServiceError`, `ServiceResult`
- Enum classes: PascalCase type, PascalCase values: `enum class SupportPaintState : int8_t { None = 0, Enforcer = 1, Blocker = 2 }`
- Template types: PascalCase: `ServiceResult<void>`, `ServiceResult<T>`
### Signals
- camelCase: `stateChanged`, `progressChanged`, `sliceFinished`, `projectChanged`, `devicesChanged`
- Past-tense for completed events: `loadFinished`, `sliceFinished`
- For request-pattern signals: `showConfigWizardRequested`, `showBedShapeDialogRequested`
## Code Style
- No automated formatter configured (no `.clang-format`, `.editorconfig`, or `.prettierrc` detected)
- **Indentation:** 2 spaces (both C++ and QML — observed consistently across all files)
- **Brace style:** K&R / Allman hybrid — opening brace on next line for functions/classes, same line for control flow in QML
- **Line length:** No strict limit, but typically 80-120 characters
#pragma once
## Import Organization
#include "EditorViewModel.h"
#include "core/services/ProjectServiceMock.h"
#include "core/services/SliceService.h"
#include "core/services/UndoRedoManager.h"
#include "core/services/UndoCommands.h"
#include <QFileInfo>
#include <QUrl>
#include <QVector4D>
#include <QSettings>
#include <algorithm>
#include <QSet>
#include <QTimer>
#include <cstring>
#include <cmath>
#include <QDebug>
## Qt Meta-Object Patterns
- All viewmodel/service state exposed via `Q_PROPERTY` with `READ` + `NOTIFY`
- Writable properties add `WRITE`
- Computed/derived properties use `CONSTANT`
- Signal name convention: single `stateChanged()` per class covers most properties; specific signals for independent state (`paintDataChanged`, `bedShapeChanged`)
- All methods callable from QML are marked `Q_INVOKABLE`
- Actions (verbs): `loadFile()`, `requestSlice()`, `duplicateSelectedObjects()`
- Queries returning collections: `objectNames()`, `plateNames()`, `filterOptionIndices()`
- Used for enum exposure to QML: `Q_ENUM(GizmoMode)`, `Q_ENUM(CanvasType)`
## Error Handling
- Structured error via `ServiceError` with severity levels (0=Info, 1=Warning, 2=Error, 3=Fatal)
- Generic `ServiceResult<T>` return type with `ok`, `value`, `error` fields
- `ServiceResult<void>` specialization for void operations
- Factory methods: `ServiceResult::success(value)`, `ServiceResult::failure(error)`
- `qWarning()` for errors and unexpected states
- `qInfo()` for informational messages
- `qDebug()` included in some files but used sparingly
- Tagged format: `[GL]`, `[Backend]`, `[Latency]`, `[ProjectService]` prefix in log messages
- Example: `qWarning("[GL] uploadMesh ignored: invalid object count %d", int(objCount));`
- Errors posted via `BackendContext::postError(message, severity)` and `postNotification()`
- Notification queue system with severity-based auto-dismiss timing
- Toast vs banner vs modal based on severity level
## Comments & Documentation
- Every feature comment includes upstream reference in parentheses:
- Used on classes and important methods: `/// Description here`
- Constructor documentation with usage examples (see `JobBase.h`)
- Struct field comments where not self-evident
- All new or modified source comments must be English and ASCII-only.
- Format: `// TODO: description`
- Used sparingly for deferred implementation: ~10 total across codebase
- Example: `// TODO: Implement actual clear logic when mesh selection is available`
## Class Design Patterns
- All ViewModels are `final` classes inheriting `QObject`
- Constructor injection for dependencies (services, other viewmodels)
- Single `stateChanged()` signal for bulk UI refresh
- No business logic in QML — viewmodels handle all computation
- Services are `final` QObject classes
- Expose data via Q_PROPERTY and Q_INVOKABLE
- Mock suffix indicates the layer (even when real libslic3r calls are present)
- `#ifdef HAS_LIBSLIC3R` guards for conditional real API usage
- `JobManager::instance()` — Meyer's singleton
- `BackendContext` created once in `main_qml.cpp`, injected to QML as context property
- Explicit `emit stateChanged();` (not omitted `emit` keyword)
- Emitted after every state mutation in setters
## Undo/Redo Pattern
- `UndoRedoManager` wraps `QUndoStack`, owned by `BackendContext`
- Commands inherit `QUndoCommand`
- Two-phase construction: capture old state in constructor, call `setNewTransform()`/`setResult()`/`setVolumeCountBefore()` before `push()`
- Macro support: `beginMacro()` / `endMacro()` for grouping
## Module Design
- `BackendContext` owns all ViewModels and Services
- ViewModels receive raw pointers to services (non-owning)
- `UndoRedoManager` is injected into `EditorViewModel` via setter
## QML Conventions
- Direct binding to backend properties: `text: backend.currentPage`
- Required properties pattern: `required property var editorVm`
- Inline `component` declarations for page-local reusable items:
- Singleton `Theme` object (registered via `qmldir`)
- Color references: `Theme.bgBase`, `Theme.accent`, `Theme.textPrimary`
- Spacing/sizing: `Theme.spacingMD`, `Theme.fontSizeLG`, `Theme.controlHeightMD`
- `qsTr()` for all user-visible strings
- Translation files: `i18n/zh_CN.ts`, `i18n/en.ts`, `i18n/ja.ts`, `i18n/ko.ts`, `i18n/de.ts`, `i18n/fr.ts`
- `#ifdef HAS_LIBSLIC3R` for real backend integration
- `#ifdef Q_OS_WIN` for Windows-specific code
<!-- GSD:conventions-end -->

<!-- GSD:architecture-start source:ARCHITECTURE.md -->
## Architecture

## System Overview
```text
```
## Component Responsibilities
| Component | Responsibility | File |
|-----------|----------------|------|
| BackendContext | Composition root: owns all services, viewmodels; exposes to QML via context property; notification system; theme/language/scale | `src/qml_gui/BackendContext.h` |
| EditorViewModel | Prepare page state: object list, selection, transforms, gizmo settings (15+ gizmos), plate management, slice progress bridge, undo/redo proxy | `src/core/viewmodels/EditorViewModel.h` |
| ProjectServiceMock | Model/project data store: load 3MF/STL/OBJ, plate management, object CRUD, mesh snapshot for undo, scoped config overrides | `src/core/services/ProjectServiceMock.h` |
| SliceService | Slice engine bridge: starts/cancels slices via libslic3r Print, G-code export, progress reporting | `src/core/services/SliceService.h` |
| UndoRedoManager | QUndoStack wrapper: push/clear/undo/redo, macro support, shared with EditorViewModel | `src/core/services/UndoRedoManager.h` |
| GLViewport | QML-registered OpenGL viewport: QQuickFramebufferObject, mesh data property binding, mouse/gizmo interaction, camera control | `src/qml_gui/Renderer/GLViewport.h` |
| GLViewportRenderer | Actual GL rendering: grid, mesh, gizmos (move/rotate/scale), bed, wipe tower, picking, ray casting | `src/qml_gui/Renderer/GLViewportRenderer.h` |
| GCodeRenderer | G-code preview rendering (layer travel visualization) | `src/qml_gui/Renderer/GCodeRenderer.h` |
| PreviewViewModel | Preview page state: layer range, playback control, legend, statistics, color modes | `src/core/viewmodels/PreviewViewModel.h` |
| MonitorViewModel | Device page state: device list, connection status, camera feeds (mock) | `src/core/viewmodels/MonitorViewModel.h` |
| ConfigViewModel | Settings/preset state: config options, preset selection (mock-backed) | `src/core/viewmodels/ConfigViewModel.h` |
## Pattern Overview
- `BackendContext` is the **composition root** — it constructs all services and viewmodels, wires dependencies, and exposes them as Q_PROPERTY objects to QML via a single context property named `backend`.
- QML pages receive viewmodel references as properties (e.g., `editorVm: backend.editorViewModel`) and bind to their Q_PROPERTY signals.
- Services are injected into viewmodels via constructor pointers. Viewmodels never own services.
- The upstream `third_party/OrcaSlicer/deps_src/slic3r/GUI` is treated as **source truth** — every Qt6 behavior must map to an upstream equivalent.
- Many services carry a `Mock` suffix but contain real libslic3r code behind `#ifdef HAS_LIBSLIC3R` guards. The name is a legacy artifact from early prototyping.
## Layers
- Purpose: Presentation, layout, interaction wiring, visual composition
- Location: `src/qml_gui/pages/`, `src/qml_gui/panels/`, `src/qml_gui/components/`, `src/qml_gui/controls/`, `src/qml_gui/dialogs/`
- Contains: QML files (.qml), SVG/PNG assets, theme definitions
- Depends on: ViewModels (via `backend.editorViewModel`, etc.), Theme singleton, custom controls
- Used by: Qt Quick runtime (loaded from qrc)
- Purpose: State management, UI-facing API, orchestration of service calls
- Location: `src/core/viewmodels/`
- Contains: QObject subclasses with Q_PROPERTY/Q_INVOKABLE API
- Depends on: Services (injected), UndoRedoManager
- Used by: QML pages, BackendContext
- Purpose: Business logic, data persistence, libslic3r bridge, async operations
- Location: `src/core/services/`
- Contains: QObject subclasses — real services (SliceService) and mock/partial services (ProjectServiceMock, PresetServiceMock, etc.)
- Depends on: libslic3r (conditionally via HAS_LIBSLIC3R), Qt Concurrent
- Used by: ViewModels, BackendContext
- Purpose: OpenGL rendering, 3D viewport, camera control, G-code visualization
- Location: `src/qml_gui/Renderer/` + `src/core/rendering/`
- Contains: QQuickFramebufferObject subclass (GLViewport), Renderer subclass (GLViewportRenderer), CameraController, shader utilities
- Depends on: Qt OpenGL, QQuickFramebufferObject API, mesh data from EditorViewModel
- Used by: QML pages (PreparePage, PreviewPage)
- Purpose: Functional reference for all behavior — the wxWidgets/ImGui code that defines what the Qt6 migration must replicate
- Location: `third_party/OrcaSlicer/src/slic3r/GUI/` (GUI behavior), `third_party/OrcaSlicer/src/libslic3r/` (slicing engine)
- Contains: 400+ upstream source files (wxWidgets GUI, OpenGL 3D, ImGui overlays)
- Depends on: wxWidgets 3.1, ImGui, native OpenGL
- Used by: Migration reference only — not compiled as part of Qt6 GUI target
## Data Flow
### Primary Request Path: Load Model
### Primary Request Path: Slice
### Primary Request Path: Undo/Redo
- All mutable UI state lives in ViewModels as Q_PROPERTY members with explicit NOTIFY signals.
- Domain data (models, plates, volumes) lives in ProjectServiceMock.
- BackendContext holds notification queue, theme settings, and page navigation state.
- No global singletons except the `backend` context property.
## Key Abstractions
- Purpose: Typed notification system aligned with upstream NotificationManager
- Examples: `src/qml_gui/BackendContext.h:32-63`
- Pattern: Queue-based with priority, auto-dismiss timeout, history log, progress support
- Purpose: Typed result wrapper for service operations — error code, severity, user-visible message
- Examples: `src/core/viewmodels/ServiceTypes.h:11-39`
- Pattern: `ServiceResult<T>::success(value)` / `ServiceResult<T>::failure(error)` factory methods
- Purpose: Enum and struct definitions for support/seam/hollow painting — aligned with upstream TriangleSelector
- Examples: `src/core/rendering/SupportPaintTypes.h:15-161`
- Pattern: Plain data structures in a `Crality3D` namespace, used by EditorViewModel and future rendering code
- Purpose: Volume type classification aligned with upstream ModelVolumeType
- Examples: `src/core/services/ProjectServiceMock.h:20-28`
- Pattern: Enum with ModelPart, NegativeVolume, ParameterModifier, SupportBlocker, SupportEnforcer, TextEmboss, SvgEmboss
## Entry Points
- Location: `src/qml_gui/main_qml.cpp`
- Triggers: Process launch
- Responsibilities: QGuiApplication setup, OpenGL backend selection, GLViewport type registration, BackendContext construction, QQmlApplicationEngine loading of `main.qml`, crash handler installation
- Location: `src/main.cpp` (not active when OWZX_QML_GUI=ON)
- Triggers: Process launch (only when OWZX_QML_GUI=OFF)
- Responsibilities: Qt Widgets application with PreparePage/PreviewPage/MonitorPage/ParameterPage
- Location: `src/qml_gui/main.qml`
- Triggers: QQmlApplicationEngine::load()
- Responsibilities: ApplicationWindow shell, title bar with menu system, StackLayout page navigation (11 pages), keyboard shortcuts, file dialogs, notification center
- Location: `CMakeLists.txt` (project root)
- Triggers: cmake configure
- Responsibilities: C++17 standard, automoc/autorcc/autouic, libslic3r from source, Qt6 QML/Quick/OpenGL/Concurrent, DLL deployment
## Architectural Constraints
- **Threading:** Qt single-threaded GUI with QThread/QConcurrent/Qt Concurrent for background slice operations. SliceService uses `std::atomic_bool` for cancel flags. UndoCommands execute synchronously on the GUI thread.
- **Global state:** BackendContext is the sole composition root, created on the stack in `main()` and exposed as `backend` context property. No other module-level singletons. ProjectServiceMock holds the `Slic3r::Model*` pointer (conditional on HAS_LIBSLIC3R).
- **Circular imports:** ViewModels depend on Services (via injected pointers). BackendContext depends on both. No circular C++ includes — all forward-declared in BackendContext.h, concrete includes in BackendContext.cpp.
- **Conditional compilation:** `HAS_LIBSLIC3R` guard controls all libslic3r API usage across services and viewmodels. `OWZX_QML_GUI` cmake option toggles between QML and Widgets build paths.
- **QML property binding:** QML pages never hold direct references to service objects. All access goes through ViewModel properties exposed via BackendContext. This is enforced by the project rules in `.codex/rules/qml-boundaries.md`.
- **OCCT linkage:** OpenCASCADE (OCCT) libraries are linked statically (load-time import) in the FromSource build, NOT delay-loaded. `src/core/DelayLoadHook.cpp` exists on disk but is dead code (not compiled/linked; `__pfnDliNotifyHook2` was never linked into the exe). OCCT is available at runtime for STEP/SVG/text-shape import; mesh boolean / cut surface are excluded due to CGAL version (not OCCT). See `cmake/BuildLibslic3rFromSource.cmake:599-679`.
## Anti-Patterns
### QML pages directly accessing service internals
### Adding new product behavior without upstream mapping
### Modifying libslic3r source code
## Error Handling
- `BackendContext::postError(message, severity)` — routed to ErrorToast (severity 0) or ErrorBanner (severity 1+)
- `BackendContext::postNotification()` with typed notification types (SlicingProgress, ExportFinished, etc.)
- `ServiceResult<T>` — returned by service methods; callers check `.ok` before using `.value`
- `ProjectServiceMock::lastError` property — holds last error string for inspection by ViewModel
- Silent failure with `return false` pattern in many Q_INVOKABLE methods — QML callers should check return values
## Cross-Cutting Concerns
<!-- GSD:architecture-end -->

<!-- GSD:skills-start source:skills/ -->
## Project Skills

| Skill | Description | Path |
|-------|-------------|------|
| analyzing-source-truth-gap | "Analyzing an upstream-to-Qt migration gap without making code changes. Triggers when the user asks for a read-only comparison between OrcaSlicer upstream behavior and current Qt6 implementation." | `.Codex/skills/analyzing-source-truth-gap/SKILL.md` |
| migrating-source-truth | "Migrating source-truth tasks from OrcaSlicer upstream to Qt6/QML. Triggers when the user asks to continue migration, pick up the next task, or run migration in batch mode." | `.Codex/skills/migrating-source-truth/SKILL.md` |
<!-- GSD:skills-end -->

<!-- GSD:workflow-start source:GSD defaults -->
## GSD Workflow Enforcement

Before using Edit, Write, or other file-changing tools, start work through a GSD command so planning artifacts and execution context stay in sync.

Use these entry points:
- `/gsd-quick` for small fixes, doc updates, and ad-hoc tasks
- `/gsd-debug` for investigation and bug fixing
- `/gsd-execute-phase` for planned phase work

Do not make direct repo edits outside a GSD workflow unless the user explicitly asks to bypass it.
<!-- GSD:workflow-end -->

<!-- GSD:profile-start -->
## Developer Profile

> Profile not yet configured. Run `/gsd-profile-user` to generate your developer profile.
> This section is managed by `generate-Codex-profile` -- do not edit manually.
<!-- GSD:profile-end -->
