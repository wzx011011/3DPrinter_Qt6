# Codebase Concerns

**Analysis Date:** 2026-05-31

## Tech Debt

### 1. God Object: ProjectServiceMock (4,563 lines)

- **Issue:** `ProjectServiceMock` is simultaneously a model loader, project serializer, object CRUD manager, volume manager, plate manager, scoped config store, mesh data provider, plate thumbnail renderer, and a pass-through for 15+ libslic3r real APIs. It uses `#ifdef HAS_LIBSLIC3R` conditional compilation in ~80 locations to dual-path between Mock arrays and real libslic3r calls.
- **Files:** `src/core/services/ProjectServiceMock.cpp`, `src/core/services/ProjectServiceMock.h`
- **Impact:** Every new upstream API integration must be added to this file, increasing merge conflicts, review burden, and regression risk. The dual-mode pattern (Mock arrays + real model) requires manual synchronization of parallel data structures (e.g., `objectNames_` list vs `model_->objects[]`), which has already caused bugs (see Known Bugs below).
- **Fix approach:** Decompose into focused services: `ObjectStore` (CRUD + transforms), `PlateManager` (plate lifecycle), `ProjectSerializer` (load/save JSON + future 3MF), `MeshDataProvider` (mesh data extraction + coordinate transforms), `ScopedConfigStore` (per-object/per-volume/per-plate overrides). Extract the real libslic3r integration behind interfaces so the Mock can be swapped cleanly.

### 2. God Object: EditorViewModel (3,500 lines, 749-line header)

- **Issue:** `EditorViewModel` exposes ~200 Q_PROPERTY and ~120 Q_INVOKABLE methods. It manages selection state, gizmo parameters for 14+ gizmos (Move, Rotate, Scale, Measure, Flatten, Cut, AdvancedCut, SupportPaint, SeamPaint, Hollow, Simplify, MMU, Drill, Emboss, MeshBoolean, FaceDetector, Text, SVG), slice bridge, plate management, arrange settings, bed shape settings, object manipulation, viewport warnings, and undo/redo routing.
- **Files:** `src/core/viewmodels/EditorViewModel.cpp`, `src/core/viewmodels/EditorViewModel.h`
- **Impact:** Adding a new gizmo requires modifying both files with new properties, getters, setters, state variables, and Q_PROPERTY declarations. QML binding surface is enormous, increasing QML runtime property resolution overhead.
- **Fix approach:** Extract per-gizmo state and operations into dedicated GizmoViewModel classes (e.g., `CutGizmoViewModel`, `SupportPaintViewModel`, `HollowGizmoViewModel`). EditorViewModel would own a map of gizmo ViewModels and forward property access. This also aligns with the upstream `GLGizmosManager` pattern where each gizmo is a separate object.

### 3. Dual-Mode Conditional Compilation (`#ifdef HAS_LIBSLIC3R`)

- **Issue:** 80+ `#ifdef HAS_LIBSLIC3R` blocks across the codebase create two parallel code paths that must be maintained in lockstep. When `HAS_LIBSLIC3R` is ON, real libslic3r calls are used; when OFF, Mock arrays and QTimer-based simulations are used. Both paths must produce compatible outputs.
- **Files:** `src/core/services/ProjectServiceMock.cpp` (~60 blocks), `src/core/viewmodels/EditorViewModel.cpp` (~11 blocks), `src/core/services/SliceService.cpp` (~8 blocks), `src/qml_gui/Models/ConfigOptionModel.cpp` (~2 blocks), `src/qml_gui/Renderer/GCodeRenderer.cpp` (~1 block)
- **Impact:** Risk of real-mode regressions going undetected because CI/smoke tests may only run in Mock mode. The "release readiness" doc notes the build is currently tested in Mock mode. Some code paths have `HAS_LIBSLIC3R` enabled but contain `// TODO: upstream PartPlate config access` stubs that silently return fallback values.
- **Fix approach:** Define an interface/abstraction layer (`IProjectService`, `ISliceEngine`) that has both Mock and Real implementations selected at runtime rather than compile time. This eliminates the dual-path maintenance burden and enables testing real-mode paths without requiring the full libslic3r build.

### 4. Mock Services Are Permanent Infrastructure

- **Issue:** 7 service classes with "Mock" in their name (`ProjectServiceMock`, `DeviceServiceMock`, `CameraServiceMock`, `CloudServiceMock`, `NetworkServiceMock`, `PresetServiceMock`, `CalibrationServiceMock`) are not test doubles -- they are the production implementation for modules whose upstream APIs have not been integrated. They are constructed directly in `BackendContext.cpp` and wired into ViewModels as the real services.
- **Files:** `src/core/services/*Mock.{cpp,h}`, `src/qml_gui/BackendContext.cpp`
- **Impact:** The "Mock" naming creates cognitive overhead -- these are not temporary; they are semi-permanent production code with substantial feature surface (e.g., `DeviceServiceMock` at 707 lines with HMS data, print simulation, temperature fluctuation). Renaming later will be a large refactor. The Mock services are not behind interfaces, making it impossible to swap them for real implementations without modifying all callers.
- **Fix approach:** Rename to production names (e.g., `DeviceService`, `CloudService`) once their API contracts stabilize. Extract interfaces (`IDeviceService`, `ICloudService`) so real implementations can be dropped in without modifying ViewModels.

### 5. Plate Architecture Is Fundamentally Blocked (P0.5.5)

- **Issue:** The upstream `PartPlateList` is tightly coupled to wxWidgets/OpenGL (`GLCanvas3D`, `GLTexture`, `3DScene`, `GLModel`). The Qt6 migration cannot import it directly. The current Mock plate management provides functional add/delete/rename/lock but cannot replicate the upstream plate model where plates own their own `Model` instance, independent slice results, and bed-type-specific configurations.
- **Files:** `src/core/services/ProjectServiceMock.cpp` (plate methods), `src/core/viewmodels/EditorViewModel.cpp` (plate bridge)
- **Impact:** This blocks: (1) per-plate independent slicing (each plate must clone the model), (2) real plate-scoped config overrides (currently stubbed with `// TODO: upstream PartPlate config access` at line 1210 and 1229), (3) multi-plate 3MF save/load fidelity.
- **Fix approach:** Design a Qt-native `PlateManager` that owns per-plate `Slic3r::Model` clones (shallow copy via `Model::duplicate()`). This is an architectural prerequisite before any plate-level feature can move past `[-]`.

## Known Bugs

### 1. `exportSelectedAsStl()` Is a No-Op

- **Symptoms:** When user exports selected object as STL, the function emits `stateChanged()` twice but performs no actual file I/O. No error is shown to user.
- **Files:** `src/core/viewmodels/EditorViewModel.cpp:2976-2991`
- **Trigger:** User selects an object and invokes "Export STL"
- **Workaround:** None. STL export is completely non-functional.

### 2. `saveProject()` Returns Failure in `HAS_LIBSLIC3R` Mode

- **Symptoms:** When built with `HAS_LIBSLIC3R=ON`, `ProjectServiceMock::saveProject()` sets `lastError_` to "3MF export not implemented" and returns false at line 4031-4035. The Mock JSON path is never reached in real mode.
- **Files:** `src/core/services/ProjectServiceMock.cpp:4030-4035`
- **Trigger:** Any save operation when built with real libslic3r
- **Workaround:** None in HAS_LIBSLIC3R mode. The JSON save works only in Mock mode.

### 3. Plate-Scoped Config Read/Write Are Stubbed

- **Symptoms:** `plateScopedOptionValue()` always returns the fallback value; `setPlateScopedOptionValue()` always returns false when `HAS_LIBSLIC3R` is defined.
- **Files:** `src/core/services/ProjectServiceMock.cpp:1210,1229`
- **Trigger:** Any per-plate configuration access in real mode (e.g., plate-specific temperature overrides)
- **Workaround:** Mock mode provides working per-plate config via `m_mockPlateOverrides`.

### 4. Duplicate `stateChanged()` Emission in `exportSelectedAsStl()`

- **Symptoms:** `stateChanged()` is emitted twice in succession (lines 2986 and 2988) in the export function, causing unnecessary QML re-evaluation.
- **Files:** `src/core/viewmodels/EditorViewModel.cpp:2986-2988`
- **Trigger:** Calling exportSelectedAsStl
- **Workaround:** Cosmetic issue, no functional impact.

## Security Considerations

### 1. Hardcoded Serial Number in QML

- **Risk:** `FirmwareDialog.qml:22` contains `property string serialNumber: "CP04A00XXXXXXXX"` as a placeholder. If this ships in production, users may believe it reflects their actual device serial number.
- **Files:** `src/qml_gui/dialogs/FirmwareDialog.qml:22`
- **Current mitigation:** None. The property is static in QML.
- **Recommendations:** Remove the hardcoded value and replace with a dynamically populated value from `DeviceServiceMock` or backend. Add a code review gate preventing hardcoded serial numbers.

### 2. File Path Handling Without Sanitization

- **Risk:** Several `Q_INVOKABLE` methods accept file paths from QML and pass them directly to file operations (`loadFile`, `saveProject`, `addVolumeFromFile`, `importSVG`). QML input fields are not validated server-side before use.
- **Files:** `src/core/services/ProjectServiceMock.cpp` (loadFile, saveProject, addVolumeFromFile, addSvgVolume), `src/core/viewmodels/EditorViewModel.cpp` (loadFile, importSVG, exportSelectedAsStl)
- **Current mitigation:** Qt file APIs handle most path edge cases. The app is a desktop application, not a network service.
- **Recommendations:** Add path validation (existence check, extension whitelist) before passing to libslic3r I/O functions. Verify 3MF/STL file signatures before parsing to guard against malformed inputs.

### 3. QML-Based Login Form Without Secure Input

- **Risk:** The cloud login dialog (`HomePage.qml`) contains a QML `TextField` for password input. While functional, QML TextField does not guarantee secure memory handling of the password string, and the password is passed as a plain QString to `CloudServiceMock::login()`.
- **Files:** `src/qml_gui/pages/HomePage.qml`, `src/core/services/CloudServiceMock.cpp`
- **Current mitigation:** Not a real security concern yet since `CloudServiceMock` is Mock (hardcoded credentials accept any input).
- **Recommendations:** When integrating real bambu_networking, ensure password is never logged, never stored in plaintext QProperties, and is cleared from memory after use. Use `echoMode: TextInput.Password` (already present) and clear the field on dialog close.

## Performance Bottlenecks

### 1. Full Mesh Data Rebuild on Any State Change

- **Issue:** `EditorViewModel::refreshMeshCacheAndFitHint()` extracts all vertex data from `model_->objects[]` and builds the TLV mesh byte array every time any state changes. With multiple complex models, this involves iterating all objects, instances, volumes, and their triangles, then applying coordinate transforms.
- **Files:** `src/core/viewmodels/EditorViewModel.cpp` (refreshMeshCacheAndFitHint), `src/core/services/ProjectServiceMock.cpp` (meshData)
- **Cause:** Single `stateChanged()` signal drives all UI updates, including mesh data. No dirty flag or incremental update mechanism.
- **Improvement path:** Track mesh dirty state separately. Only rebuild mesh data when objects/transforms actually change (not when selection changes, gizmo mode changes, etc.). Consider GPU-side mesh buffers that persist across frames.

### 2. CPU-Side Vertex Copy for Ray Picking

- **Issue:** `GLViewportRenderer` maintains a `cpuVerts` CPU-side copy of all vertex data for ray-triangle intersection (Moller-Trumbore picking). This doubles memory usage for mesh data.
- **Files:** `src/qml_gui/Renderer/GLViewportRenderer.cpp` (MeshBatch, pickObject)
- **Cause:** No GPU readback path implemented; picking must run on CPU.
- **Improvement path:** Implement GPU-based picking via OpenGL occlusion queries or transform-feedback-based bounding box tests. At minimum, only copy bounding box data for early-reject tests and load full vertex data lazily.

### 3. ProjectServiceMock.saveProject Serializes Entire Project State to JSON

- **Issue:** JSON project save serializes all object names, transforms, volumes, plates, layer ranges, scoped overrides, and plate settings into a single JSON document using Qt's `QJsonObject`/`QJsonArray`. For large projects (many objects, complex overrides), this can be slow.
- **Files:** `src/core/services/ProjectServiceMock.cpp:4036-4250`
- **Cause:** No incremental or delta save.
- **Improvement path:** This is Mock-only and will be replaced by 3MF export. No optimization needed for Mock mode.

### 4. Large QML Files Impact Load Time

- **Issue:** `PreparePage.qml` (3,736 lines), `MonitorPage.qml` (2,222 lines), `PrintSettings.qml` (1,723 lines) are very large QML files. QML engine parses and compiles these at startup.
- **Files:** `src/qml_gui/pages/PreparePage.qml`, `src/qml_gui/pages/MonitorPage.qml`, `src/qml_gui/panels/PrintSettings.qml`
- **Cause:** No component decomposition. Each page is a single file with all inline components.
- **Improvement path:** Extract reusable sub-components (e.g., ObjectList is already extracted at 1,165 lines; SliceProgress at 620 lines). Apply the same pattern to plate cards, toolbar sections, and gizmo control panels.

## Fragile Areas

### 1. Parallel Data Structure Synchronization (ProjectServiceMock)

- **Files:** `src/core/services/ProjectServiceMock.cpp`, `src/core/services/ProjectServiceMock.h`
- **Why fragile:** The service maintains parallel data structures: `objectNames_` (QStringList), `objectPositions_`/`objectRotations_`/`objectScales_` (QList<QVector3D>), `objectPrintableStates_` (QList<bool>), `objectVisibleStates_` (QList<bool>), `plateObjectIndices_` (QList<QList<int>>), `m_mockVolumes` (QHash), `m_mockObjectOverrides` (QHash), and `model_` (Slic3r::Model*). Every CRUD operation (add, delete, duplicate, split, move, reorder) must update all of these atomically. A missed update causes index misalignment or data loss.
- **Safe modification:** Always modify through the existing public API methods (addObject, deleteObject, duplicateObject, splitObject, moveObject, renameObject) which update all structures together. Never directly manipulate the parallel arrays.
- **Test coverage:** Smoke tests cover basic import/delete. Missing: split-then-undo, duplicate-then-modify-then-undo, multi-plate reorder, bulk delete.

### 2. Undo/Redo Command Registration Completeness

- **Files:** `src/core/services/UndoCommands.cpp`, `src/core/viewmodels/EditorViewModel.cpp`
- **Why fragile:** Not all object mutations are registered as undo commands. The undo system covers: Transform, DeleteObjects, AddObject, Selection, Rename, MoveObject, Clone, VolumeDelete, Boolean, Drill, Cut, Simplify. But operations like `setObjectPrintable`, `setVolumeExtruderId`, plate reassignment (`moveSelectedObjectToPlate`), and volume type changes are not undoable.
- **Safe modification:** Check `EditorViewModel` for `m_undoManager->push(...)` calls before assuming any operation is undoable. New mutations should wrap state changes in a new QUndoCommand subclass.
- **Test coverage:** Smoke tests do not verify undo/redo behavior.

### 3. Gizmo Mode State Machine in GLViewport

- **Files:** `src/qml_gui/Renderer/GLViewport.cpp`, `src/qml_gui/Renderer/GLViewportRenderer.cpp`
- **Why fragile:** Gizmo mode is an integer enum in `GLViewport` with 18+ values (0=Move, 1=Rotate, 2=Scale, 3=Measure, 4=Flatten, 5=Cut, 6=SupportPaint, 7=SeamPaint, 8=Hollow, 9=Simplify, 10=MmuSegmentation, 12=Emboss, 13=MeshBoolean, 15=FaceDetector, 16=Text, 17=SVG, 18=SlaSupports). No state transition validation exists. Setting mode to an unsupported value or switching modes while an operation is in-progress may leave the renderer in an inconsistent state.
- **Safe modification:** Always switch gizmo modes through `EditorViewModel` (which manages the active gizmo) rather than directly setting `GLViewport::gizmoMode`.
- **Test coverage:** Smoke tests do not exercise gizmo mode transitions.

### 4. QML Component Instantiation at Startup

- **Files:** `src/qml_gui/main.qml`, `src/qml_gui/pages/*.qml`
- **Why fragile:** All pages are instantiated at startup via a `StackLayout` with all pages as children. Even pages the user may never visit (Calibration, MultiMachine, ModelMall) are fully constructed. Adding a new page increases startup time and memory.
- **Safe modification:** New pages should use `Loader` with lazy loading so they are only instantiated when navigated to.
- **Test coverage:** Smoke test verifies startup time but does not benchmark memory usage.

## Scaling Limits

### 1. Object Count Limits

- **Current capacity:** The parallel array approach in ProjectServiceMock implies O(n) linear scan for most operations. `meshData()` rebuilds all meshes on every state change. No explicit object count limit is enforced, but performance degrades with large assemblies.
- **Limit:** Approximately 50-100 objects before UI becomes noticeably sluggish (untested; estimate based on full mesh rebuild per state change).
- **Scaling path:** Implement dirty-flag mesh caching, batch rendering (instanced rendering for identical objects), and level-of-detail (LOD) meshes.

### 2. Undo Stack Depth

- **Current capacity:** Default 50, configurable up to 200 via SettingsViewModel. Each command captures full state snapshots (mesh data, selection state, plate assignments).
- **Limit:** Memory grows linearly with undo depth. Mesh snapshot commands (`BooleanCommand`, `DrillCommand`) serialize full mesh data, so deep undo stacks with large models can consume significant RAM.
- **Scaling path:** Implement incremental mesh diffs for undo (only store changed vertices), or cap undo depth adaptively based on available memory.

### 3. GCode Parsing (PreviewViewModel)

- **Current capacity:** Parses G-code files sequentially into layer/segment structures. File is fully loaded into memory.
- **Limit:** Large G-code files (100MB+) may cause memory pressure. No streaming parser implemented.
- **Scaling path:** Implement streaming G-code parser that reads line-by-line and only retains current layer data for rendering.

## Dependencies at Risk

### 1. OpenVDB (Link Failure)

- **Risk:** OpenVDB is required for Hollow Gizmo's real mesh hollowing and drain hole generation. It currently fails to link.
- **Impact:** Hollow gizmo cannot perform real hollowing operations. `sla::hollow_mesh_and_drill` is unavailable. The drill gizmo uses a simplified `MeshBoolean::cgal::minus` instead of the full hollow-and-drill path.
- **Migration plan:** Either fix the OpenVDB build (dependency chain: OpenVDB -> Boost.Iostreams -> Blosc -> zlib), or implement a simplified hollowing algorithm using CGAL boolean operations.

### 2. FFmpeg (Not Found)

- **Risk:** FFmpeg is required for RTSP camera stream decoding and media playback. It is not available in the build environment.
- **Impact:** All camera/video features are Mock-only. `CameraServiceMock` simulates 3 virtual cameras but cannot decode real streams.
- **Migration plan:** Integrate FFmpeg via vcpkg or pre-built binaries. Alternative: use GStreamer or Qt Multimedia with RTSP plugin.

### 3. bambu_networking (Closed-Source Plugin)

- **Risk:** The upstream device communication, MQTT, cloud login, and cloud sync all depend on `bambu_networking.dll`, a closed-source library from BambuLab. It cannot be built from source.
- **Impact:** Monitor page, cloud services, device management, and multi-machine features cannot move beyond Mock without either (a) obtaining the library, (b) reverse-engineering the protocol, or (c) building a replacement protocol stack.
- **Migration plan:** Document in `docs/依赖与协议边界审计.md` as "包装适配" strategy. May require building a Qt-native MQTT/HTTP client that speaks the Bambu protocol.

### 4. TriangleSelector (Not Migrated)

- **Risk:** Per-triangle painting operations (support paint, seam paint, MMU segmentation) require `TriangleSelector` / `TriangleSelectorPatch` from upstream, which manages per-triangle paint state storage and GPU-side visualization.
- **Impact:** Support painting, seam painting, and MMU segmentation gizmos have UI and controls but cannot actually paint triangles. The `setTriangleSupportState()` method exists in EditorViewModel but is a stub.
- **Migration plan:** Port `TriangleSelector` to Qt6, using a CPU-side per-triangle data structure backed by a flat array indexed by triangle ID. GPU visualization can use a separate VBO with per-triangle color attributes.

## Missing Critical Features

### 1. Real 3MF Project Save/Export

- **Problem:** `saveProject()` in HAS_LIBSLIC3R mode is a stub returning "3MF export not implemented". Users cannot save their work in the industry-standard format.
- **Blocks:** Project persistence, file round-trip with other slicers, cloud backup.
- **Files:** `src/core/services/ProjectServiceMock.cpp:4030-4035`

### 2. Real Tab/Config Data Loading from Upstream

- **Problem:** ConfigOptionModel loads real option definitions from upstream `print_config_def` (about 110 keys), but the real `Tab`/`PresetBundle` data structure (category hierarchy, page grouping, dependency rules, visibility conditions) is not loaded. The 3-tier preset hierarchy uses hardcoded values in PresetServiceMock.
- **Blocks:** Accurate preset inheritance, config dependency resolution, full Settings page fidelity.
- **Files:** `src/qml_gui/Models/ConfigOptionModel.cpp`, `src/core/services/PresetServiceMock.cpp`

### 3. GL Shell Rendering (P2.8.2)

- **Problem:** Shell rendering (outer wall visualization in preview) requires `GLVolumeCollection` integration from upstream, which depends on libslic3r's internal GL model structures. Currently stubbed.
- **Blocks:** Complete G-code preview visualization.
- **Files:** `src/qml_gui/Renderer/GCodeRenderer.cpp`

### 4. Real Device Connection Protocol

- **Problem:** All device communication (MQTT, SSDP, HTTP) is Mock. No real protocol implementation exists.
- **Blocks:** Monitor page, multi-machine, HMS real-time alerts, cloud sync.
- **Files:** `src/core/services/DeviceServiceMock.{cpp,h}`, `src/core/services/NetworkServiceMock.{cpp,h}`, `src/core/services/CameraServiceMock.{cpp,h}`

## Test Coverage Gaps

### 1. No Undo/Redo Testing

- **What's not tested:** Undo/redo for any operation (transform, delete, clone, cut, boolean, drill, simplify). The undo framework (`UndoRedoManager`, 8+ `QUndoCommand` subclasses) has zero test coverage.
- **Files:** `src/core/services/UndoRedoManager.cpp`, `src/core/services/UndoCommands.cpp`
- **Risk:** Command bugs (incorrect state restoration, missing property updates, memory leaks in undo data) will go undetected until manual testing.
- **Priority:** High -- undo/redo is a core user-facing feature and bugs here can cause data loss.

### 2. No Real-Mode Integration Testing

- **What's not tested:** Any code path behind `#ifdef HAS_LIBSLIC3R`. Smoke tests run in Mock mode. Real libslic3r API calls (3MF import, slice, arrange, orient, cut, boolean, drill, emboss, SVG import) are not tested automatically.
- **Files:** All files containing `#ifdef HAS_LIBSLIC3R` blocks
- **Risk:** Real-mode regressions can break the working import/slice/gizmo features without detection. The 3MF import smoke test (`topbar_import_3mf_generates_mesh`) does run with real libslic3r but only tests a single asset file.
- **Priority:** High -- the build enables HAS_LIBSLIC3R by default, but testing does not exercise it.

### 3. No QML Rendering/Visual Testing

- **What's not tested:** Visual output of the 3D viewport, gizmo rendering, G-code preview colors, bed grid, wipe tower, markers, and any OpenGL rendering.
- **Files:** `src/qml_gui/Renderer/GLViewportRenderer.cpp`, `src/qml_gui/Renderer/GCodeRenderer.cpp`
- **Risk:** Shader compilation errors, rendering artifacts, and visual regressions go undetected. The CMake option `ENABLE_VISUAL_REGRESSION=ON` exists but no visual regression tests are implemented.
- **Priority:** Medium -- currently caught by manual inspection, but automated visual testing would prevent shader regressions.

### 4. No Error Path Testing

- **What's not tested:** Error handling paths: loading invalid files, slicing with no objects, deleting last object, undo past stack limit, saving to read-only path, network timeouts, clipboard operations with empty selection.
- **Files:** All service and viewmodel files
- **Risk:** Error handling code paths (null checks, fallback values, error signals) may be dead code or produce incorrect error messages.
- **Priority:** Medium -- errors in edge cases can cause crashes or confusing user experience.

### 5. No Performance Testing

- **What's not tested:** Performance with large models, many objects, large G-code files, many plates, deep undo stacks.
- **Files:** N/A (no performance test infrastructure)
- **Risk:** Performance regressions from new features (e.g., adding another gizmo with mesh rebuild) go undetected.
- **Priority:** Low -- currently not a release blocker but will matter as real model complexity increases.

### 6. Minimal Smoke Test Coverage (11 Tests, Single Test File)

- **What's not tested:** The test file `tests/ViewModelSmokeTests.cpp` contains only 11 test functions covering basic import, slice, monitor refresh, preset switching, navigation, 3MF import, delete, and multi-plate. No tests exist for: gizmos, config scope overrides, plate management, calibration, model mall, multi-machine, home page, notifications, search, or any QML component.
- **Files:** `tests/ViewModelSmokeTests.cpp`
- **Risk:** The 66/66 smoke test metric from the build script checks binary existence and QML resource completeness, not functional correctness. The actual QtTest suite is only 11 tests.
- **Priority:** High -- the gap between "66 smoke test checks" and "11 unit tests" is misleading.

---

*Concerns audit: 2026-05-31*
