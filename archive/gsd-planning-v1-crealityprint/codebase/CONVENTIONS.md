# Coding Conventions

**Analysis Date:** 2026-05-31

## Language & Standard

- **Language:** C++17 (`set(CMAKE_CXX_STANDARD 17)`) for all `src/core` and `src/qml_gui` C++ code
- **QML:** Qt 6.10 declarative UI (`import QtQuick`, `import QtQuick.Controls`)
- **C Standard:** C11 (used for `nanosvg_impl.cpp` and delay-load hooks)

## Naming Patterns

### Files

**C++ headers:**
- PascalCase matching the primary class: `EditorViewModel.h`, `ProjectServiceMock.h`, `GLViewport.h`, `BackendContext.h`
- One class per file (header + implementation pair)

**C++ implementations:**
- Exact match with header: `EditorViewModel.cpp`, `BackendContext.cpp`

**QML files:**
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

**Formatting:**
- No automated formatter configured (no `.clang-format`, `.editorconfig`, or `.prettierrc` detected)
- **Indentation:** 2 spaces (both C++ and QML — observed consistently across all files)
- **Brace style:** K&R / Allman hybrid — opening brace on next line for functions/classes, same line for control flow in QML
- **Line length:** No strict limit, but typically 80-120 characters

**Key observed patterns:**

```cpp
// Header guard (always)
#pragma once

// Class declaration style — final on QObject subclasses
class EditorViewModel final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString statusText READ statusText NOTIFY stateChanged)

public:
  explicit EditorViewModel(ProjectServiceMock *projectService, SliceService *sliceService, QObject *parent = nullptr);

  // ...

signals:
  void stateChanged();

private:
  ProjectServiceMock *projectService_ = nullptr;
  SliceService *sliceService_ = nullptr;
};
```

**QML component style:**

```qml
import QtQuick
import QtQuick.Controls

Button {
    id: root

    property int cxStyle: CxButton.Style.Primary
    property bool compact: false

    enum Style { Primary, Secondary, Danger, Ghost }

    implicitHeight: compact ? 24 : 30
    // ...
}
```

## Import Organization

**C++ headers — order:**
1. Project headers with `#include "core/..."` or `#include "qml_gui/..."`
2. Qt headers with `#include <Q...>`
3. Standard library headers with `#include <...>`

```cpp
// From EditorViewModel.cpp:
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
```

**QML imports — order:**
1. Qt modules (`import QtQuick`, `import QtQuick.Controls`, etc.)
2. Parent directory (`import ".."`)
3. Local subdirectories (`import "../controls"`, `import "../panels"`, etc.)
4. Registered C++ types (`import CrealityGL 1.0`)

## Qt Meta-Object Patterns

**Q_PROPERTY:**
- All viewmodel/service state exposed via `Q_PROPERTY` with `READ` + `NOTIFY`
- Writable properties add `WRITE`
- Computed/derived properties use `CONSTANT`
- Signal name convention: single `stateChanged()` per class covers most properties; specific signals for independent state (`paintDataChanged`, `bedShapeChanged`)

**Q_INVOKABLE:**
- All methods callable from QML are marked `Q_INVOKABLE`
- Actions (verbs): `loadFile()`, `requestSlice()`, `duplicateSelectedObjects()`
- Queries returning collections: `objectNames()`, `plateNames()`, `filterOptionIndices()`

**Q_ENUM:**
- Used for enum exposure to QML: `Q_ENUM(GizmoMode)`, `Q_ENUM(CanvasType)`

## Error Handling

**Service layer (`ServiceTypes.h`):**
- Structured error via `ServiceError` with severity levels (0=Info, 1=Warning, 2=Error, 3=Fatal)
- Generic `ServiceResult<T>` return type with `ok`, `value`, `error` fields
- `ServiceResult<void>` specialization for void operations
- Factory methods: `ServiceResult::success(value)`, `ServiceResult::failure(error)`

**Logging:**
- `qWarning()` for errors and unexpected states
- `qInfo()` for informational messages
- `qDebug()` included in some files but used sparingly
- Tagged format: `[GL]`, `[Backend]`, `[Latency]`, `[ProjectService]` prefix in log messages
- Example: `qWarning("[GL] uploadMesh ignored: invalid object count %d", int(objCount));`

**QML-side error display:**
- Errors posted via `BackendContext::postError(message, severity)` and `postNotification()`
- Notification queue system with severity-based auto-dismiss timing
- Toast vs banner vs modal based on severity level

## Comments & Documentation

**Language:** Chinese (Simplified) comments throughout — this is a Chinese-team project migrating CrealityPrint

**Alignment annotations:**
- Every feature comment includes upstream reference in parentheses:
  - `/// 对齐上游 Plater::show_object_info` (aligns with upstream Plater::show_object_info)
  - `/// 对齐上游 GLGizmoFlatten` (aligns with upstream GLGizmoFlatten)
  - `/// 对齐上游 TriangleMeshStats::open_edges` (aligns with upstream TriangleMeshStats::open_edges)

**Section dividers:**
```cpp
// ── TransformCommand ────────────────────────────────────────────────────────
// ── Object manipulation (对齐上游 ObjectManipulation) ──────────────────────
```

**Doxygen/JSDoc-style:**
- Used on classes and important methods: `/// Description here`
- Constructor documentation with usage examples (see `JobBase.h`)
- Struct field comments where not self-evident

**TODO markers:**
- Format: `// TODO: description` or `// TODO: 中文描述`
- Used sparingly for deferred implementation: ~10 total across codebase
- Example: `// TODO: Implement actual clear logic when mesh selection is available`

## Class Design Patterns

**ViewModel pattern:**
- All ViewModels are `final` classes inheriting `QObject`
- Constructor injection for dependencies (services, other viewmodels)
- Single `stateChanged()` signal for bulk UI refresh
- No business logic in QML — viewmodels handle all computation

**Service pattern:**
- Services are `final` QObject classes
- Expose data via Q_PROPERTY and Q_INVOKABLE
- Mock suffix indicates the layer (even when real libslic3r calls are present)
- `#ifdef HAS_LIBSLIC3R` guards for conditional real API usage

**Singleton:**
- `JobManager::instance()` — Meyer's singleton
- `BackendContext` created once in `main_qml.cpp`, injected to QML as context property

**Signal emission:**
- Explicit `emit stateChanged();` (not omitted `emit` keyword)
- Emitted after every state mutation in setters

## Undo/Redo Pattern

- `UndoRedoManager` wraps `QUndoStack`, owned by `BackendContext`
- Commands inherit `QUndoCommand`
- Two-phase construction: capture old state in constructor, call `setNewTransform()`/`setResult()`/`setVolumeCountBefore()` before `push()`
- Macro support: `beginMacro()` / `endMacro()` for grouping

## Module Design

**No barrel files:** Each header is included directly by path

**Forward declarations:** Used extensively in headers to minimize compile dependencies:
```cpp
class ProjectServiceMock;
class SliceService;
class UndoRedoManager;
```

**Cross-cutting ownership:**
- `BackendContext` owns all ViewModels and Services
- ViewModels receive raw pointers to services (non-owning)
- `UndoRedoManager` is injected into `EditorViewModel` via setter

## QML Conventions

**Property bindings:**
- Direct binding to backend properties: `text: backend.currentPage`
- Required properties pattern: `required property var editorVm`

**Component definitions:**
- Inline `component` declarations for page-local reusable items:
```qml
component ToolStripDivider: Rectangle { ... }
component TitleBarDivider: Rectangle { ... }
```

**Theme usage:**
- Singleton `Theme` object (registered via `qmldir`)
- Color references: `Theme.bgBase`, `Theme.accent`, `Theme.textPrimary`
- Spacing/sizing: `Theme.spacingMD`, `Theme.fontSizeLG`, `Theme.controlHeightMD`

**Localization:**
- `qsTr()` for all user-visible strings
- Translation files: `i18n/zh_CN.ts`, `i18n/en.ts`, `i18n/ja.ts`, `i18n/ko.ts`, `i18n/de.ts`, `i18n/fr.ts`

**Conditional compilation:**
- `#ifdef HAS_LIBSLIC3R` for real backend integration
- `#ifdef Q_OS_WIN` for Windows-specific code

---

*Convention analysis: 2026-05-31*
