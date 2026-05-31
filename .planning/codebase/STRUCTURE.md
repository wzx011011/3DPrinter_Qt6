# Codebase Structure

**Analysis Date:** 2026-05-31

## Directory Layout

```
3DPrinter_Qt6/
├── CMakeLists.txt                    # Unified build entry (QML/Widgets toggle via CREALITY_QML_GUI)
├── vcpkg.json                        # vcpkg dependency manifest (boost, tbb, cgal, etc.)
├── CLAUDE.md                         # Claude Code instructions (project-level)
├── docs/                             # Architecture docs, migration tracking
├── i18n/                             # Qt Linguist translation files (6 languages)
├── cmake/                            # CMake modules for libslic3r build
├── scripts/                          # Build, verify, diagnostic PowerShell scripts
├── tests/                            # Smoke / regression tests
├── build/                            # Build output (generated, not committed)
├── third_party/
│   └── CrealityPrint/               # Upstream source truth (submodule)
│       └── src/
│           ├── slic3r/GUI/           # Upstream GUI modules (400+ files, wxWidgets)
│           └── libslic3r/            # Slicing engine (built from source)
├── src/                              # Qt6 migration target
│   ├── qml_gui/                      # QML view + interaction layer
│   │   ├── main_qml.cpp              # QML app entry + type registration
│   │   ├── BackendContext.h/.cpp      # Composition root (services + viewmodels)
│   │   ├── main.qml                  # Main window, title bar, page navigation
│   │   ├── Theme.qml                 # Singleton theme palette
│   │   ├── qml.qrc                   # Qt resource file
│   │   ├── qmldir                    # QML module directory
│   │   ├── pages/                    # Top-level pages (13 QML files)
│   │   ├── panels/                   # Sub-page panels (4 QML files)
│   │   ├── components/               # Reusable components (15 QML files)
│   │   ├── controls/                 # Custom control library (8 QML files)
│   │   ├── dialogs/                  # Modal/non-modal dialogs (14 QML files)
│   │   ├── Renderer/                 # OpenGL 3D rendering bridge (8 C++ files)
│   │   ├── Models/                   # QML data models (4 C++ files)
│   │   ├── data/                     # Static JSON data (hints)
│   │   └── assets/                   # Icons, reference images
│   ├── core/                         # Qt business logic layer
│   │   ├── services/                 # Services: project, slice, undo, jobs, mocks (22 files)
│   │   ├── viewmodels/               # ViewModels: editor, preview, monitor, etc. (24 files)
│   │   ├── rendering/                # Rendering utilities (3 files)
│   │   ├── debug/                    # Crash handler, early diagnostics (3 files)
│   │   └── DelayLoadHook.cpp         # OCCT delay-load suppression
│   ├── main.cpp                      # Widgets entry (legacy, only when CREALITY_QML_GUI=OFF)
│   ├── MainWindow.cpp/.h             # Widgets main window (legacy)
│   ├── pages/                        # Widgets pages (legacy, 8 files)
│   ├── theme/                        # Widgets theme (legacy, 2 files)
│   └── nanosvg_impl.cpp              # NanoSVG implementation
└── .claude/
    └── rules/                        # Project rules loaded by Claude Code
        ├── source-truth-migration.md # Migration workflow rules
        ├── build-rules.md            # Build system rules
        ├── debugging.md              # Debugging rules
        ├── qml-boundaries.md         # QML layer boundary rules
        └── core-architecture.md      # Core architecture rules
```

## Directory Purposes

**`src/qml_gui/` — QML View and Interaction Layer:**
- Purpose: All QML-based UI — pages, panels, dialogs, controls, and the OpenGL rendering bridge
- Contains: QML files (.qml), C++ QML-exposed types (QQuickFramebufferObject, QQmlListProperty), resource definitions
- Key files: `main.qml` (main window shell), `BackendContext.h/.cpp` (composition root), `Theme.qml` (singleton palette)

**`src/core/services/` — Service Layer:**
- Purpose: Business logic, data persistence, libslic3r API bridge, async job management
- Contains: Real services (SliceService, UndoRedoManager, JobManager) and mock/partial services (ProjectServiceMock, PresetServiceMock, DeviceServiceMock, etc.)
- Key files: `ProjectServiceMock.h` (model/project data store), `SliceService.h` (slice engine), `UndoRedoManager.h` (undo/redo framework), `UndoCommands.h` (per-operation undo commands), `JobBase.h`/`JobManager.h` (async job system)

**`src/core/viewmodels/` — ViewModel Layer:**
- Purpose: UI state management and service orchestration for each page
- Contains: QObject subclasses with Q_PROPERTY/Q_INVOKABLE API, one per page/feature area
- Key files: `EditorViewModel.h` (Prepare page — largest, 750+ lines), `PreviewViewModel.h`, `MonitorViewModel.h`, `ConfigViewModel.h`, `ServiceTypes.h` (shared error types)

**`src/core/rendering/` — Rendering Utilities:**
- Purpose: Shared rendering data structures and shader helpers
- Contains: GLShaderUtil (shader compile helper), SupportPaintTypes (paint enums/structs aligned with upstream)
- Key files: `GLShaderUtil.h`, `SupportPaintTypes.h`

**`src/core/debug/` — Diagnostics:**
- Purpose: Windows crash handling and early-startup diagnostics
- Contains: CrashHandlerWin (minidump writer), EarlyCrashDiag
- Key files: `CrashHandlerWin.h`

**`src/qml_gui/Renderer/` — OpenGL 3D Rendering Bridge:**
- Purpose: QQuickFramebufferObject-based 3D viewport and G-code preview renderer
- Contains: GLViewport (QML-registered type), GLViewportRenderer (actual GL draws), GCodeRenderer, CameraController
- Key files: `GLViewport.h`, `GLViewportRenderer.h`, `GCodeRenderer.h`, `CameraController.h`

**`src/qml_gui/Models/` — QML Data Models:**
- Purpose: Qt AbstractListModel subclasses for QML ListView/ComboBox bindings
- Contains: ConfigOptionModel, PresetListModel
- Key files: `ConfigOptionModel.h`, `PresetListModel.h`

**`third_party/CrealityPrint/` — Upstream Source Truth:**
- Purpose: Functional reference — the wxWidgets/ImGui CrealityPrint code that defines all behavior the Qt6 migration must replicate
- Contains: 400+ GUI files in `src/slic3r/GUI/`, full slicing engine in `src/libslic3r/`, dependencies (eigen, expat, miniz, etc.)
- Note: Git submodule; changes here represent upstream baseline shifts

**`docs/` — Project Documentation:**
- Purpose: Architecture design, migration task tracking, dependency audit
- Key files: `CrealityPrint_Qt_GUI重写架构.md` (canonical architecture doc), `源码对照迁移任务追踪.md` (migration task tracker), `项目结构.md` (structure reference)

**`scripts/` — Build and Diagnostic Scripts:**
- Purpose: PowerShell automation for building, testing, diagnostics
- Key files: `auto_verify_with_vcvars.ps1` (authoritative build script), `smoke_test.ps1` (runtime verification), `capture_qml_warnings.ps1` (QML warning capture)

**`tests/` — Test Suite:**
- Purpose: Smoke tests and regression tests
- Key files: `ViewModelSmokeTests.cpp`

**`i18n/` — Internationalization:**
- Purpose: Qt Linguist translation files
- Contains: `zh_CN.ts`, `en.ts`, `ja.ts`, `ko.ts`, `de.ts`, `fr.ts`

## Key File Locations

**Entry Points:**
- `src/qml_gui/main_qml.cpp`: QML application entry (primary path, CREALITY_QML_GUI=ON)
- `src/main.cpp`: Widgets application entry (legacy path, CREALITY_QML_GUI=OFF)
- `src/qml_gui/main.qml`: QML root window definition
- `CMakeLists.txt`: Build system entry point

**Configuration:**
- `CMakeLists.txt`: Build configuration (Qt6 components, libslic3r options, DLL deployment)
- `vcpkg.json`: Dependency manifest for vcpkg
- `src/qml_gui/qtquickcontrols2.conf`: Qt Quick Controls 2 style configuration
- `src/qml_gui/qml.qrc`: Qt resource compiler input (all QML files and assets)
- `src/qml_gui/qmldir`: QML module registration (singleton Theme)

**Composition Root:**
- `src/qml_gui/BackendContext.h/.cpp`: Central wiring of all services and viewmodels

**Core Logic:**
- `src/core/services/ProjectServiceMock.cpp`: Model loading, plate management, mesh operations
- `src/core/services/SliceService.cpp`: Slice engine integration with libslic3r
- `src/core/services/UndoRedoManager.cpp`: Undo/redo framework
- `src/core/services/UndoCommands.cpp`: Individual undo command implementations
- `src/core/viewmodels/EditorViewModel.cpp`: Prepare page state management
- `src/core/viewmodels/PreviewViewModel.cpp`: Preview page state management

**Rendering:**
- `src/qml_gui/Renderer/GLViewport.cpp`: QML-exposed 3D viewport
- `src/qml_gui/Renderer/GLViewportRenderer.cpp`: OpenGL rendering implementation
- `src/qml_gui/Renderer/GCodeRenderer.cpp`: G-code path visualization
- `src/qml_gui/Renderer/CameraController.cpp`: Camera orbit/pan/zoom

**Theme:**
- `src/qml_gui/Theme.qml`: Singleton theme palette (dark theme colors, fonts, spacing)

**Testing:**
- `tests/ViewModelSmokeTests.cpp`: ViewModel smoke tests

## Naming Conventions

**Files:**
- C++ headers: PascalCase with extension `.h` (e.g., `EditorViewModel.h`, `SliceService.h`)
- C++ implementations: PascalCase with extension `.cpp` (e.g., `EditorViewModel.cpp`)
- QML files: PascalCase with extension `.qml` (e.g., `PreparePage.qml`, `CxButton.qml`)
- QML controls: `Cx` prefix (e.g., `CxButton.qml`, `CxComboBox.qml`, `CxIconButton.qml`)
- Mock services: `*Mock` suffix on both class name and filename (e.g., `ProjectServiceMock.h`, `DeviceServiceMock.cpp`)
- Real services: descriptive name with `Service` suffix (e.g., `SliceService.h`, `UndoRedoManager.h`)
- ViewModels: `*ViewModel` suffix (e.g., `EditorViewModel.h`, `ConfigViewModel.h`)
- QML pages: `*Page.qml` suffix (e.g., `PreparePage.qml`, `MonitorPage.qml`)
- QML dialogs: `*Dialog.qml` suffix (e.g., `PrintDialog.qml`, `BedShapeDialog.qml`)
- QML components: PascalCase descriptive names (e.g., `LayerSlider.qml`, `NotificationCenter.qml`)
- Scripts: `snake_case.ps1` (e.g., `auto_verify_with_vcvars.ps1`, `smoke_test.ps1`)

**Directories:**
- Lowercase names (e.g., `services/`, `viewmodels/`, `rendering/`, `panels/`, `controls/`)
- Legacy Widgets code: `src/pages/` (lowercase)
- QML GUI code: `src/qml_gui/` with mixed-case subdirectories matching file naming

**Classes:**
- C++ classes: PascalCase (e.g., `BackendContext`, `EditorViewModel`, `SliceService`)
- C++ enums: PascalCase for enum type, PascalCase for values (e.g., `MockVolumeType::ModelPart`, `GizmoMode::GizmoMove`)
- C++ namespaces: PascalCase (e.g., `Crality3D`, `GLShaderUtil`)
- QML component IDs: camelCase (e.g., `preparePage`, `backend`, `fileMenu`)

**Q_INVOKABLE methods:**
- camelCase (e.g., `loadFile()`, `requestSlice()`, `deleteSelectedObjects()`, `setCutAxis()`)

**Q_PROPERTY names:**
- camelCase (e.g., `meshData`, `hasSelection`, `currentPage`, `sliceStatusLabel`)

## Where to Add New Code

**New ViewModel (for a new page/feature):**
- Header: `src/core/viewmodels/NewFeatureViewModel.h`
- Implementation: `src/core/viewmodels/NewFeatureViewModel.cpp`
- Wire in: `src/qml_gui/BackendContext.cpp` (constructor — create and expose as Q_PROPERTY)
- Register: `src/qml_gui/BackendContext.h` (forward declare, add member pointer, add Q_PROPERTY)

**New Service (real or mock):**
- Header: `src/core/services/NewService.h` (use `Mock` suffix if not yet real)
- Implementation: `src/core/services/NewService.cpp`
- Wire in: `src/qml_gui/BackendContext.cpp` (constructor — create and inject into dependent ViewModels)
- Add to CMakeLists.txt sources if not already included via glob

**New QML Page:**
- File: `src/qml_gui/pages/NewFeaturePage.qml`
- Register in QRC: `src/qml_gui/qml.qrc` (add `<file>pages/NewFeaturePage.qml</file>`)
- Add to StackLayout: `src/qml_gui/main.qml` (add Loader or direct component with page index)
- Add tab entry: `src/qml_gui/main.qml` `buildWorkflowTabs()` function

**New QML Dialog:**
- File: `src/qml_gui/dialogs/NewDialog.qml`
- Register in QRC: `src/qml_gui/qml.qrc`
- Instantiate in `src/qml_gui/main.qml` (with signal-based show/hide pattern)
- Add trigger signal in BackendContext: `showNewDialogRequested()` signal + `showNewDialog()` Q_INVOKABLE

**New QML Control (reusable):**
- File: `src/qml_gui/controls/CxNewControl.qml` (use `Cx` prefix)
- Register in QRC: `src/qml_gui/qml.qrc`
- Import in pages via `"controls"` directory

**New QML Component (page-specific):**
- File: `src/qml_gui/components/NewComponent.qml`
- Register in QRC: `src/qml_gui/qml.qrc`
- Import in pages via `"components"` directory

**New Rendering Feature (gizmo, shader, etc.):**
- Shader/utility code: `src/core/rendering/` (new header if shared, or inline in Renderer)
- Gizmo mode enum: Add to `GLViewport::GizmoMode` enum in `src/qml_gui/Renderer/GLViewport.h`
- Gizmo property: Add Q_PROPERTY to `EditorViewModel.h`
- Rendering implementation: Add to `GLViewportRenderer.cpp` (geometry build + render methods)

**New Undo Command:**
- Header: `src/core/services/UndoCommands.h` (add new QUndoCommand subclass)
- Implementation: `src/core/services/UndoCommands.cpp`
- Integration: Push via `UndoRedoManager::push()` from the ViewModel method that performs the operation

**New Test:**
- File: `tests/NewFeatureTests.cpp`
- Register: CMakeLists.txt (add to CTest via `add_test()`)
- Pattern: Follow `tests/ViewModelSmokeTests.cpp`

**New Translation:**
- Update existing: `i18n/zh_CN.ts`, `i18n/en.ts`, etc. (via Qt Linguist `lupdate` + `lrelease`)
- All user-visible strings in QML must use `qsTr()`

## Special Directories

**`build/`:**
- Purpose: CMake build output (binaries, intermediate files, diagnostics)
- Generated: Yes (by cmake configure)
- Committed: No (gitignored)
- Contains: `startup_diagnostics.log`, `crash_dumps/`, `delayload_diag.log`

**`third_party/CrealityPrint/`:**
- Purpose: Upstream source truth (git submodule)
- Generated: No
- Committed: Via submodule reference
- Contains: Full CrealityPrint source tree including libslic3r

**`src/qml_gui/assets/`:**
- Purpose: SVG icons and reference screenshot images for visual regression comparison
- Generated: No
- Committed: Yes
- Contains: `icons/` (20+ SVG files), `prepare_ref.png`, `preview_ref.png`, `monitor_ref.png`

**`src/qml_gui/data/`:**
- Purpose: Static JSON data files loaded at runtime
- Generated: No
- Committed: Yes
- Contains: `hints.json` (hint database for notification system)

**`src/pages/` (legacy):**
- Purpose: Qt Widgets pages (PreparePage, PreviewPage, MonitorPage, ParameterPage)
- Generated: No
- Committed: Yes
- Status: Retained for CREALITY_QML_GUI=OFF build path; not actively developed

---

*Structure analysis: 2026-05-31*
