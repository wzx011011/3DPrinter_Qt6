<!-- refreshed: 2026-05-31 -->
# Architecture

**Analysis Date:** 2026-05-31

## System Overview

```text
┌─────────────────────────────────────────────────────────────────────┐
│                    QML View Layer (Presentation)                     │
│              `src/qml_gui/main.qml` (ApplicationWindow)             │
├──────────┬──────────┬──────────┬──────────┬──────────┬──────────────┤
│ Prepare  │ Preview  │ Monitor  │ Settings │ Project  │ Calibration  │
│ Page     │ Page     │ Page     │ Page     │ Page     │ ... Pages    │
│ `pages/` │ `pages/` │ `pages/` │ `pages/` │ `pages/` │ `pages/`     │
└────┬─────┴────┬─────┴────┬─────┴────┬─────┴────┬─────┴──────────────┘
     │          │          │          │          │
     ▼          ▼          ▼          ▼          ▼
┌─────────────────────────────────────────────────────────────────────┐
│                   ViewModel Layer (State & Orchestration)            │
│         `src/core/viewmodels/EditorViewModel.*` (750+ properties)   │
│         `src/core/viewmodels/PreviewViewModel.*`                   │
│         `src/core/viewmodels/ConfigViewModel.*`                    │
│         `src/core/viewmodels/MonitorViewModel.*`                   │
│         `src/core/viewmodels/HomeViewModel.*`                      │
│         `src/core/viewmodels/SettingsViewModel.*`                  │
│         `src/core/viewmodels/ProjectViewModel.*`                  │
│         `src/core/viewmodels/CalibrationViewModel.*`               │
│         `src/core/viewmodels/ModelMallViewModel.*`                │
│         `src/core/viewmodels/MultiMachineViewModel.*`            │
└──────────────────────────┬──────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    Service Layer (Business Logic)                    │
│         `src/core/services/ProjectServiceMock.*` (model/project I/O)│
│         `src/core/services/SliceService.*` (slice engine bridge)   │
│         `src/core/services/UndoRedoManager.*` (QUndoStack wrapper) │
│         `src/core/services/UndoCommands.*` (per-operation commands)  │
│         `src/core/services/JobManager.*` / `JobBase.*` (async jobs) │
│         `src/core/services/PresetServiceMock.*` (presets, not real) │
│         `src/core/services/DeviceServiceMock.*` (devices, not real)│
│         `src/core/services/NetworkServiceMock.*` (networking stub) │
│         `src/core/services/CalibrationServiceMock.*`               │
│         `src/core/services/CameraServiceMock.*` / `CloudServiceMock.*`│
└──────────────────────────┬──────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    Rendering Bridge (OpenGL)                         │
│  `src/qml_gui/Renderer/GLViewport.*` (QQuickFramebufferObject)      │
│  `src/qml_gui/Renderer/GLViewportRenderer.*` (actual GL draws)    │
│  `src/qml_gui/Renderer/GCodeRenderer.*` (G-code preview)            │
│  `src/qml_gui/Renderer/CameraController.*` (orbit/pan/zoom)        │
│  `src/core/rendering/GLShaderUtil.*` (shader compile helper)        │
│  `src/core/rendering/SupportPaintTypes.h` (paint data structures)   │
└──────────────────────────┬──────────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    libslic3r (Slicing Engine — upstream)              │
│         `third_party/CrealityPrint/src/libslic3r/` (built from src) │
│         Model, Print, TriangleMesh, cut_mesh, arrange_objects, ...  │
└─────────────────────────────────────────────────────────────────────┘
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

**Overall:** MVVM (Model-View-ViewModel) with a Service layer and manual dependency injection.

**Key Characteristics:**
- `BackendContext` is the **composition root** — it constructs all services and viewmodels, wires dependencies, and exposes them as Q_PROPERTY objects to QML via a single context property named `backend`.
- QML pages receive viewmodel references as properties (e.g., `editorVm: backend.editorViewModel`) and bind to their Q_PROPERTY signals.
- Services are injected into viewmodels via constructor pointers. Viewmodels never own services.
- The upstream `third_party/CrealityPrint/src/slic3r/GUI` is treated as **source truth** — every Qt6 behavior must map to an upstream equivalent.
- Many services carry a `Mock` suffix but contain real libslic3r code behind `#ifdef HAS_LIBSLIC3R` guards. The name is a legacy artifact from early prototyping.

## Layers

**QML View Layer:**
- Purpose: Presentation, layout, interaction wiring, visual composition
- Location: `src/qml_gui/pages/`, `src/qml_gui/panels/`, `src/qml_gui/components/`, `src/qml_gui/controls/`, `src/qml_gui/dialogs/`
- Contains: QML files (.qml), SVG/PNG assets, theme definitions
- Depends on: ViewModels (via `backend.editorViewModel`, etc.), Theme singleton, custom controls
- Used by: Qt Quick runtime (loaded from qrc)

**ViewModel Layer:**
- Purpose: State management, UI-facing API, orchestration of service calls
- Location: `src/core/viewmodels/`
- Contains: QObject subclasses with Q_PROPERTY/Q_INVOKABLE API
- Depends on: Services (injected), UndoRedoManager
- Used by: QML pages, BackendContext

**Service Layer:**
- Purpose: Business logic, data persistence, libslic3r bridge, async operations
- Location: `src/core/services/`
- Contains: QObject subclasses — real services (SliceService) and mock/partial services (ProjectServiceMock, PresetServiceMock, etc.)
- Depends on: libslic3r (conditionally via HAS_LIBSLIC3R), Qt Concurrent
- Used by: ViewModels, BackendContext

**Rendering Bridge Layer:**
- Purpose: OpenGL rendering, 3D viewport, camera control, G-code visualization
- Location: `src/qml_gui/Renderer/` + `src/core/rendering/`
- Contains: QQuickFramebufferObject subclass (GLViewport), Renderer subclass (GLViewportRenderer), CameraController, shader utilities
- Depends on: Qt OpenGL, QQuickFramebufferObject API, mesh data from EditorViewModel
- Used by: QML pages (PreparePage, PreviewPage)

**Upstream Source Truth:**
- Purpose: Functional reference for all behavior — the wxWidgets/ImGui code that defines what the Qt6 migration must replicate
- Location: `third_party/CrealityPrint/src/slic3r/GUI/` (GUI behavior), `third_party/CrealityPrint/src/libslic3r/` (slicing engine)
- Contains: 400+ upstream source files (wxWidgets GUI, OpenGL 3D, ImGui overlays)
- Depends on: wxWidgets 3.1, ImGui, native OpenGL
- Used by: Migration reference only — not compiled as part of Qt6 GUI target

## Data Flow

### Primary Request Path: Load Model

1. User triggers File > Import Model in QML menu (`src/qml_gui/main.qml:89`)
2. QML calls `backend.topbarImportModel(filePath)` on BackendContext (`src/qml_gui/BackendContext.h:203`)
3. BackendContext delegates to `ProjectServiceMock::loadFile()` which calls `libslic3r` Model::load() (`src/core/services/ProjectServiceMock.cpp`)
4. ProjectServiceMock emits `loadFinished` signal; EditorViewModel calls `refreshAfterLoad()` (`src/core/viewmodels/EditorViewModel.cpp`)
5. EditorViewModel rebuilds object entries, updates meshData property (`src/core/viewmodels/EditorViewModel.h:42`)
6. QML PreparePage observes `editorVm.meshData` change and GLViewport renders the mesh (`src/qml_gui/Renderer/GLViewport.h:22`)

### Primary Request Path: Slice

1. QML PreparePage calls `editorVm.requestSlice()` (`src/core/viewmodels/EditorViewModel.h:587`)
2. EditorViewModel delegates to `SliceService::startSlice()` (`src/core/services/SliceService.h:46`)
3. SliceService creates `Slic3r::Print`, runs `Print::process()` on background thread, emits progress signals
4. EditorViewModel receives progress, updates sliceStatusLabel Q_PROPERTY
5. QML SliceProgress panel shows progress bar; on completion, EditorViewModel emits `previewRequested`

### Primary Request Path: Undo/Redo

1. QML keyboard shortcut calls `preparePage.undoFromTopbar()` → `EditorViewModel::undo()` (`src/core/viewmodels/EditorViewModel.h:603`)
2. EditorViewModel delegates to `UndoRedoManager::undo()` → `QUndoStack::undo()` (`src/core/services/UndoRedoManager.h:38`)
3. UndoCommand (e.g., AddVolumeCommand) calls `undo()` which restores mesh snapshot in ProjectServiceMock
4. UndoCommand calls `EditorViewModel::rebuildAndNotify()` to refresh UI state
5. EditorViewModel emits `stateChanged`, QML updates object list and mesh data

**State Management:**
- All mutable UI state lives in ViewModels as Q_PROPERTY members with explicit NOTIFY signals.
- Domain data (models, plates, volumes) lives in ProjectServiceMock.
- BackendContext holds notification queue, theme settings, and page navigation state.
- No global singletons except the `backend` context property.

## Key Abstractions

**NotificationLevel / NotificationType (enums in BackendContext):**
- Purpose: Typed notification system aligned with upstream NotificationManager
- Examples: `src/qml_gui/BackendContext.h:32-63`
- Pattern: Queue-based with priority, auto-dismiss timeout, history log, progress support

**ServiceError / ServiceResult<T> (in ServiceTypes.h):**
- Purpose: Typed result wrapper for service operations — error code, severity, user-visible message
- Examples: `src/core/viewmodels/ServiceTypes.h:11-39`
- Pattern: `ServiceResult<T>::success(value)` / `ServiceResult<T>::failure(error)` factory methods

**SupportPaintTypes namespace (in SupportPaintTypes.h):**
- Purpose: Enum and struct definitions for support/seam/hollow painting — aligned with upstream TriangleSelector
- Examples: `src/core/rendering/SupportPaintTypes.h:15-161`
- Pattern: Plain data structures in a `Crality3D` namespace, used by EditorViewModel and future rendering code

**MockVolumeType enum:**
- Purpose: Volume type classification aligned with upstream ModelVolumeType
- Examples: `src/core/services/ProjectServiceMock.h:20-28`
- Pattern: Enum with ModelPart, NegativeVolume, ParameterModifier, SupportBlocker, SupportEnforcer, TextEmboss, SvgEmboss

## Entry Points

**Application Entry (QML path — primary):**
- Location: `src/qml_gui/main_qml.cpp`
- Triggers: Process launch
- Responsibilities: QGuiApplication setup, OpenGL backend selection, GLViewport type registration, BackendContext construction, QQmlApplicationEngine loading of `main.qml`, crash handler installation

**Application Entry (Widgets path — legacy):**
- Location: `src/main.cpp` (not active when CREALITY_QML_GUI=ON)
- Triggers: Process launch (only when CREALITY_QML_GUI=OFF)
- Responsibilities: Qt Widgets application with PreparePage/PreviewPage/MonitorPage/ParameterPage

**QML Entry:**
- Location: `src/qml_gui/main.qml`
- Triggers: QQmlApplicationEngine::load()
- Responsibilities: ApplicationWindow shell, title bar with menu system, StackLayout page navigation (11 pages), keyboard shortcuts, file dialogs, notification center

**CMake Build Entry:**
- Location: `CMakeLists.txt` (project root)
- Triggers: cmake configure
- Responsibilities: C++17 standard, automoc/autorcc/autouic, libslic3r from source, Qt6 QML/Quick/OpenGL/Concurrent, DLL deployment

## Architectural Constraints

- **Threading:** Qt single-threaded GUI with QThread/QConcurrent/Qt Concurrent for background slice operations. SliceService uses `std::atomic_bool` for cancel flags. UndoCommands execute synchronously on the GUI thread.
- **Global state:** BackendContext is the sole composition root, created on the stack in `main()` and exposed as `backend` context property. No other module-level singletons. ProjectServiceMock holds the `Slic3r::Model*` pointer (conditional on HAS_LIBSLIC3R).
- **Circular imports:** ViewModels depend on Services (via injected pointers). BackendContext depends on both. No circular C++ includes — all forward-declared in BackendContext.h, concrete includes in BackendContext.cpp.
- **Conditional compilation:** `HAS_LIBSLIC3R` guard controls all libslic3r API usage across services and viewmodels. `CREALITY_QML_GUI` cmake option toggles between QML and Widgets build paths.
- **QML property binding:** QML pages never hold direct references to service objects. All access goes through ViewModel properties exposed via BackendContext. This is enforced by the project rules in `.claude/rules/qml-boundaries.md`.
- **Delay-loaded OCCT:** OpenCASCADE DLLs are delay-loaded to avoid CRT mismatch deadlocks. `DelayLoadHook.cpp` intercepts and suppresses actual loading. See `src/core/DelayLoadHook.cpp`.

## Anti-Patterns

### QML pages directly accessing service internals

**What happens:** A QML page reaches through a ViewModel to call a service method or read service state directly.
**Why it's wrong:** Breaks the MVVM layering. QML should only interact with ViewModel Q_PROPERTY/Q_INVOKABLE API.
**Do this instead:** Add the needed API to the ViewModel, which delegates to the service internally. See `src/core/viewmodels/EditorViewModel.h` which wraps all ProjectServiceMock and SliceService calls.

### Adding new product behavior without upstream mapping

**What happens:** A developer adds a new feature or UI flow that has no corresponding implementation in `third_party/CrealityPrint/src/slic3r/GUI/`.
**Why it's wrong:** This is a source-truth migration project. Unmapped behavior creates divergence and makes upstream tracking impossible.
**Do this instead:** Before implementing, find the corresponding upstream module in `third_party/CrealityPrint/src/slic3r/GUI/`, verify the task exists in `docs/源码对照迁移任务追踪.md`, and align the Qt6 behavior with the upstream wxWidgets implementation. See `.claude/rules/source-truth-migration.md`.

### Modifying libslic3r source code

**What happens:** A developer modifies files under `third_party/CrealityPrint/src/libslic3r/` to add features or fix issues.
**Why it's wrong:** libslic3r is upstream source truth — modifications create merge conflicts and make it impossible to track the baseline commit.
**Do this instead:** Adapt the behavior in the Qt6 service/adapter layer. If a bug exists in libslic3r, report it upstream or patch it in a clearly marked local adapter.

## Error Handling

**Strategy:** Multi-level severity system (Info, Warning, Error, Fatal) defined in `ServiceTypes.h`. QML has dedicated UI for each level (Toast, Banner, Modal).

**Patterns:**
- `BackendContext::postError(message, severity)` — routed to ErrorToast (severity 0) or ErrorBanner (severity 1+)
- `BackendContext::postNotification()` with typed notification types (SlicingProgress, ExportFinished, etc.)
- `ServiceResult<T>` — returned by service methods; callers check `.ok` before using `.value`
- `ProjectServiceMock::lastError` property — holds last error string for inspection by ViewModel
- Silent failure with `return false` pattern in many Q_INVOKABLE methods — QML callers should check return values

## Cross-Cutting Concerns

**Logging:** Startup diagnostics via `appendStartupLog()` to `build/startup_diagnostics.log`. QML warnings captured by `QQmlEngine::warnings` signal. Crash dumps via `CrashHandlerWin` in `src/core/debug/CrashHandlerWin.cpp`. Delay-load diagnostics in `delayload_diag.log`.

**Validation:** Viewport warnings (ObjectOutside, ObjectClashed) checked in `EditorViewModel::checkViewportWarnings()`. Config validation deferred to future preset service real implementation.

**Authentication:** Not applicable at this stage. Device/cloud services are mock implementations. Upstream uses bambu_networking DLL for cloud authentication (out of scope for current migration phase).

**Internationalization:** Qt Linguist workflow with 6 language files in `i18n/` (zh_CN, en, ja, ko, de, fr). QML uses `qsTr()` throughout. BackendContext manages `QTranslator` switching and triggers `QQmlApplicationEngine::retranslate` on language change.

---

*Architecture analysis: 2026-05-31*
