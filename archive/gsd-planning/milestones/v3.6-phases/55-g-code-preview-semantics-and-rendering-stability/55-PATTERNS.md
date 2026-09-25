# Phase 55: G-code Preview Semantics and Rendering Stability - Pattern Map

**Mapped:** 2026-07-02
**Files analyzed:** 10
**Analogs found:** 10 / 10

## File Classification

| New/Modified File | Role | Data Flow | Closest Analog | Match Quality |
|-------------------|------|-----------|----------------|---------------|
| `src/qml_gui/components/VisibilityFilter.qml` (NEW) | component | request-response (QML binding) | `src/qml_gui/components/StatsPanel.qml` + `Legend.qml` | exact |
| `src/core/viewmodels/PreviewViewModel.h` (MODIFY) | viewmodel | CRUD (state management) | `src/core/viewmodels/PreviewViewModel.h` (self) | self-modify |
| `src/core/viewmodels/PreviewViewModel.cpp` (MODIFY) | viewmodel | CRUD (state management) | `src/core/viewmodels/PreviewViewModel.cpp` (self) | self-modify |
| `src/qml_gui/Renderer/RhiViewport.h` (MODIFY) | renderer | request-response (property binding) | `src/qml_gui/Renderer/RhiViewport.h` (self) | self-modify |
| `src/qml_gui/Renderer/RhiViewport.cpp` (MODIFY) | renderer | request-response (property binding) | `src/qml_gui/Renderer/RhiViewport.cpp` (self) | self-modify |
| `src/qml_gui/Renderer/RhiViewportRenderer.h` (MODIFY) | renderer | batch (draw-range filtering) | `src/qml_gui/Renderer/RhiViewportRenderer.h` (self) | self-modify |
| `src/qml_gui/Renderer/RhiViewportRenderer.cpp` (MODIFY) | renderer | batch (draw-range filtering) | `src/qml_gui/Renderer/RhiViewportRenderer.cpp` (self) | self-modify |
| `src/qml_gui/pages/PreviewPage.qml` (MODIFY) | page | request-response (QML binding) | `src/qml_gui/pages/PreviewPage.qml` (self) | self-modify |
| `tests/PreviewParserTests.cpp` (NEW) | test | CRUD (unit test) | `tests/QmlUiAuditTests.cpp` + `tests/E2EWorkflowTests.cpp` | role-match |
| `tests/fixtures/*.gcode` (NEW) | test fixture | file-I/O | `third_party/OrcaSlicer/tests/data/` | reference |

## Pattern Assignments

### `src/qml_gui/components/VisibilityFilter.qml` (NEW component, request-response)

**Analog:** `src/qml_gui/components/StatsPanel.qml` (same right-panel consumer component) + `src/qml_gui/components/Legend.qml` (Repeater + colored rectangle pattern)

**Imports pattern** (StatsPanel.qml lines 1-5):
```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"
```

**Component shell pattern** (StatsPanel.qml lines 7-9):
```qml
Item {
    id: root
    required property var previewVm

    implicitHeight: statsLayout.implicitHeight
```

**Section header pattern** (CollapsibleSection.qml lines 20-33 -- the card-based section wrapper):
```qml
Rectangle {
    id: card
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    height: titleBar.height + (root.expanded ? (contentContainer.implicitHeight + 8) : 0)
    color: Theme.bgElevated
    radius: 8
    border.width: 1
    border.color: Theme.borderSubtle

    Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
}
```

**Colored-rectangle + label Repeater pattern** (Legend.qml lines 85-106):
```qml
Repeater {
    model: root.previewVm ? root.previewVm.legendItems : []
    delegate: RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Rectangle {
            width: 10
            height: 10
            radius: 2
            color: modelData.color
        }
        Label {
            Layout.fillWidth: true
            text: modelData.label
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            elide: Text.ElideRight
        }
    }
}
```

**CxCheckBox binding pattern** (StatsPanel.qml lines 52-56):
```qml
ToggleRow {
    label: qsTr("显示空驶")
    checked: root.previewVm ? root.previewVm.showTravelMoves : false
    onChanged: if (root.previewVm) root.previewVm.setShowTravelMoves(checked)
}
```

**Row height and spacing convention** (StatsPanel.qml lines 78-84 -- card inner ColumnLayout spacing: 6-9px margins):
```qml
ColumnLayout {
    id: statValues
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.margins: 9
    spacing: 6
```

**qrc registration pattern** (qml.qrc has `components/StatsPanel.qml` at line 61, `components/Legend.qml` at line 60). New `components/VisibilityFilter.qml` must be added to the same `<qresource>` block.

---

### `src/core/viewmodels/PreviewViewModel.h` (MODIFY viewmodel, CRUD)

**Analog:** Self-modify. Existing `PreviewViewModel.h` lines 1-301 provide the full pattern.

**Q_PROPERTY declaration pattern** (lines 59-63 -- existing visibility toggle):
```cpp
Q_PROPERTY(bool showTravelMoves READ showTravelMoves WRITE setShowTravelMoves NOTIFY stateChanged)
Q_PROPERTY(bool showBed READ showBed WRITE setShowBed NOTIFY stateChanged)
Q_PROPERTY(bool showMarker READ showMarker WRITE setShowMarker NOTIFY stateChanged)
```

New property to add (follows same pattern):
```cpp
Q_PROPERTY(QVariantList roleVisibilities READ roleVisibilities NOTIFY stateChanged)
```

**Q_INVOKABLE declaration pattern** (lines 149-155):
```cpp
Q_INVOKABLE void setShowTravelMoves(bool enabled);
Q_INVOKABLE void setShowBed(bool enabled);
Q_INVOKABLE void setShowMarker(bool enabled);
```

New methods to add:
```cpp
Q_INVOKABLE bool isRoleVisible(int roleIndex) const;
Q_INVOKABLE void toggleRoleVisibility(int roleIndex);
```

**Member variable pattern** (lines 251-253 -- snake_case with trailing underscore):
```cpp
bool showTravelMoves_ = true;
bool showBed_ = true;
bool showMarker_ = true;
```

New member to add:
```cpp
std::array<bool, 20> m_roleVisibility;  // indexed by EGCodeExtrusionRole
```

**StoredSegment struct extension** (lines 258-273):
```cpp
struct StoredSegment
{
    float x1, y1, z1, x2, y2, z2;
    float baseR, baseG, baseB;
    float feedrate;
    float fan_speed;
    float temperature;
    float width;
    float height;
    float layer_time;
    float acceleration;
    float volumetric_rate;
    int extruder_id;
    int layer;
    int move;
    bool isTravel;
    // NEW: int role;  -- EGCodeExtrusionRole index (0=None, 1=Perimeter, ..., 19=Mixed)
};
```

---

### `src/core/viewmodels/PreviewViewModel.cpp` (MODIFY viewmodel, CRUD)

**Analog:** Self-modify. Key sections for modification.

**PackedSegment struct** (lines 46-66):
```cpp
struct PackedSegment
{
    float x1;
    float y1;
    float z1;
    float x2;
    float y2;
    float z2;
    float r;
    float g;
    float b;
    float feedrate;
    float fan_speed;
    float temperature;
    float width;
    float layer_time;
    float acceleration;
    int extruder_id;
    int layer;
    int move;
    // NEW: int role;  -- must match GcvPackedSegment exactly
};
```

**styleFor function to replace** (lines 114-126 -- current 5-category mapping):
```cpp
FeatureStyle styleFor(const QString &type)
{
    const QString t = type.toUpper();
    if (t.contains("WALL") || t.contains("PERIMETER"))
      return {QStringLiteral("外壁"), QStringLiteral("#FF8C3A"), 1.0f, 0.55f, 0.23f};
    if (t.contains("INFILL") || t.contains("FILL"))
      return {QStringLiteral("填充"), QStringLiteral("#38A4FF"), 0.22f, 0.64f, 1.0f};
    if (t.contains("SUPPORT"))
      return {QStringLiteral("支撑"), QStringLiteral("#8C63FF"), 0.55f, 0.39f, 1.0f};
    if (t.contains("TRAVEL"))
      return {QStringLiteral("空驶"), QStringLiteral("#6E7785"), 0.43f, 0.47f, 0.52f};
    return {QStringLiteral("其他"), QStringLiteral("#53D890"), 0.33f, 0.84f, 0.56f};
}
```

**Segment construction pattern** (lines 989-1013 -- where `role` field must be populated):
```cpp
const FeatureStyle style = styleFor(extruding ? currentType : QStringLiteral("TRAVEL"));

StoredSegment seg;
seg.x1 = x;
seg.y1 = y;
seg.z1 = z;
seg.x2 = nx;
seg.y2 = ny;
seg.z2 = nz;
seg.baseR = style.r;
seg.baseG = style.g;
seg.baseB = style.b;
// ... existing fields ...
seg.layer = layer;
seg.move = moveIndex;
seg.isTravel = !extruding;
// NEW: seg.role = fineGrainedRoleFromType(extruding ? currentType : "TRAVEL");
segments_.push_back(seg);
```

**viewModes() function** (lines 345-362 -- to be extended from 13 to 17 modes):
```cpp
QStringList PreviewViewModel::viewModes() const
{
    return {
        QStringLiteral("Line Type"),
        QStringLiteral("Layer Height"),
        // ... 11 more ...
        QStringLiteral("Acceleration")};
}
```

**Visibility setter pattern** (lines 542-560 -- setShowTravelMoves uses recolorAndPackSegments):
```cpp
void PreviewViewModel::setShowTravelMoves(bool enabled)
{
    if (showTravelMoves_ == enabled)
      return;
    showTravelMoves_ = enabled;
    recolorAndPackSegments();
    emit stateChanged();
}
```

IMPORTANT: The new `toggleRoleVisibility()` must NOT call `recolorAndPackSegments()`. It must only emit `stateChanged()`. This is the render-side filtering requirement from CONTEXT.md.

---

### `src/qml_gui/Renderer/RhiViewport.h` (MODIFY renderer, request-response)

**Analog:** Self-modify. Existing Q_PROPERTY pattern (lines 23-27):
```cpp
Q_PROPERTY(int layerMin READ layerMin WRITE setLayerMin)
Q_PROPERTY(int layerMax READ layerMax WRITE setLayerMax)
Q_PROPERTY(int moveEnd READ moveEnd WRITE setMoveEnd)
Q_PROPERTY(bool showTravelMoves READ showTravelMoves WRITE setShowTravelMoves)
```

New property to add (follows existing pattern):
```cpp
Q_PROPERTY(QVariantList roleVisibility READ roleVisibility WRITE setRoleVisibility NOTIFY roleVisibilityChanged)
```

New signal to add (line 193-198):
```cpp
signals:
    // ... existing ...
    void roleVisibilityChanged();
```

New member (line 248-249):
```cpp
QVariantList m_roleVisibility;
```

---

### `src/qml_gui/Renderer/RhiViewport.cpp` (MODIFY renderer, request-response)

**Analog:** Self-modify. Interaction setter pattern (lines 72-102):
```cpp
void RhiViewport::setLayerMin(int value)
{
    if (m_layerMin == value)
        return;
    m_layerMin = value;
    update();
}

void RhiViewport::setMoveEnd(int value)
{
    if (m_moveEnd == value)
        return;
    m_moveEnd = value;
    update();
}
```

New setter to add (same pattern, just stores + update()):
```cpp
void RhiViewport::setRoleVisibility(const QVariantList &value)
{
    if (m_roleVisibility == value)
        return;
    m_roleVisibility = value;
    update();
}
```

---

### `src/qml_gui/Renderer/RhiViewportRenderer.h` (MODIFY renderer, batch)

**Analog:** Self-modify.

**PreviewDrawSpan struct** (lines 105-110):
```cpp
struct PreviewDrawSpan {
    int layer;
    int move;
    quint32 vertexOffset;
    quint32 vertexCount;
    // NEW: int role;  -- EGCodeExtrusionRole index for render-side filtering
};
```

**New member** (lines 98-103):
```cpp
QVector<bool> m_roleVisibility;  // NEW: per-role visibility mask from viewport
```

---

### `src/qml_gui/Renderer/RhiViewportRenderer.cpp` (MODIFY renderer, batch)

**Analog:** Self-modify.

**GcvPackedSegment struct** (lines 571-577):
```cpp
struct GcvPackedSegment
{
    float x1, y1, z1, x2, y2, z2;
    float r, g, b;
    float feedrate, fan_speed, temperature, width, layer_time, acceleration;
    int extruder_id, layer, move;
    // NEW: int role;  -- must match PackedSegment layout exactly
};
```

**Span creation pattern** (line 625):
```cpp
m_previewDrawSpans.append({seg[i].layer, seg[i].move, vertexOffset, 2});
```
Must become:
```cpp
m_previewDrawSpans.append({seg[i].layer, seg[i].move, vertexOffset, 2, seg[i].role});
```

**Synchronize block pattern** (lines 76-91):
```cpp
if (m_previewData != viewport->m_previewData) {
    m_previewData = viewport->m_previewData;
    resetPreviewGpuState(false);
    parsePreviewSegments();
}
m_layerMin = viewport->m_layerMin;
m_layerMax = viewport->m_layerMax;
m_moveEnd = viewport->m_moveEnd;
m_showTravelMoves = viewport->m_showTravelMoves;
m_gcodeViewMode = viewport->m_gcodeViewMode;
// NEW: m_roleVisibility = ... (convert QVariantList to QVector<bool>)
```

**computePreviewDrawRange filter pattern** (lines 648-713 -- role check insertion point):
```cpp
void RhiViewportRenderer::computePreviewDrawRange(quint32 &firstVertex, quint32 &vertexCount) const
{
    // ... existing setup ...
    for (const auto &span : m_previewDrawSpans) {
        if (span.layer < layerLow || span.layer > layerHigh)
            continue;
        if (span.move >= m_moveEnd)
            continue;
        // NEW: render-side role filtering (no repack)
        if (span.role >= 0 && span.role < m_roleVisibility.size()
            && !m_roleVisibility[span.role])
            continue;

        if (!foundStart) {
            startOffset = span.vertexOffset;
            foundStart = true;
        }
        endOffset = span.vertexOffset + span.vertexCount;
    }
    // ... existing tail ...
}
```

---

### `src/qml_gui/pages/PreviewPage.qml` (MODIFY page, request-response)

**Analog:** Self-modify. Insertion point (lines 362-372):
```qml
                                Components.StatsPanel {
                                    Layout.fillWidth: true
                                    previewVm: root.previewVm
                                }

                                Components.Legend {
                                    Layout.fillWidth: true
                                    previewVm: root.previewVm
                                }
```

Insert `Components.VisibilityFilter` between StatsPanel and Legend:
```qml
                                Components.StatsPanel {
                                    Layout.fillWidth: true
                                    previewVm: root.previewVm
                                }

                                Components.VisibilityFilter {
                                    Layout.fillWidth: true
                                    previewVm: root.previewVm
                                }

                                Components.Legend {
                                    Layout.fillWidth: true
                                    previewVm: root.previewVm
                                }
```

**Component import** (line 7):
```qml
import "../components" as Components
```
Already present -- no change needed.

---

### `tests/PreviewParserTests.cpp` (NEW test, unit)

**Analog:** `tests/QmlUiAuditTests.cpp` (source-audit test structure) + `tests/E2EWorkflowTests.cpp` (parser regression structure)

**QmlUiAuditTests class structure** (lines 8-48):
```cpp
class QmlUiAuditTests final : public QObject
{
    Q_OBJECT

private slots:
    void topLevelUiHasNoVisiblePlaceholdersOrNoopActions();
    // ... many test methods ...

private:
    QString readSource(const QString &relativePath) const;
};

QString QmlUiAuditTests::readSource(const QString &relativePath) const
{
    QFile file(QDir(QStringLiteral(QT_TESTCASE_SOURCEDIR)).filePath(relativePath));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll());
}
```

**E2EWorkflowTests helper pattern** (lines 18-52):
```cpp
namespace
{
    static const QString kStlPath = QDir::cleanPath(
        QStringLiteral(QT_TESTCASE_SOURCEDIR) +
        QStringLiteral("/third_party/OrcaSlicer/tests/data/test_3mf/Prusa.stl"));

    int gcv1SegmentCount(const QByteArray &payload)
    {
        if (!payload.startsWith("GCV1") || payload.size() < 8)
            return -1;
        int count = 0;
        std::memcpy(&count, payload.constData() + 4, sizeof(count));
        return count;
    }
}
```

**CMake registration pattern** (CMakeLists.txt lines 388-396 -- QmlUiAuditTests registration):
```cmake
qt_add_executable(QmlUiAuditTests tests/QmlUiAuditTests.cpp)
target_link_libraries(QmlUiAuditTests PRIVATE Qt6::Test)
target_compile_definitions(QmlUiAuditTests PRIVATE
    QT_TESTCASE_SOURCEDIR="${CMAKE_SOURCE_DIR}"
)
add_test(NAME QmlUiAuditTests COMMAND QmlUiAuditTests)
set_tests_properties(QmlUiAuditTests PROPERTIES
    ENVIRONMENT "PATH=${_qt6_bin_dir};$ENV{PATH}"
)
```

For PreviewParserTests (needs libslic3r for real parser testing), use the ViewModelSmokeTests/E2EWorkflowTests link pattern (CMakeLists.txt lines 359-363):
```cmake
qt_add_executable(PreviewParserTests tests/PreviewParserTests.cpp)
target_link_libraries(PreviewParserTests PRIVATE owzx_app_core
    Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::OpenGL Qt6::Concurrent Qt6::Network Qt6::Test
)
target_link_options(PreviewParserTests PRIVATE /FORCE:MULTIPLE)
add_test(NAME PreviewParserTests COMMAND PreviewParserTests)
set_tests_properties(PreviewParserTests PROPERTIES
    ENVIRONMENT "PATH=${_qt6_bin_dir};$ENV{PATH}"
)
```

**E2EWorkflowTests test method pattern** (lines 560-580 -- preview parser test):
```cpp
void E2EWorkflowTests::test_preview_parser_handles_extrusion_modes_and_travel_filter()
{
    // ... construct services + viewmodel ...
    // ... load fixture gcode ...
    // ... assert segment counts, role distribution ...
}
```

---

## Shared Patterns

### Interaction Setter Pattern (no-repack)
**Source:** `src/qml_gui/Renderer/RhiViewport.cpp` lines 72-102
**Apply to:** All new RhiViewport interaction setters (setRoleVisibility)
```cpp
void RhiViewport::setLayerMin(int value)
{
    if (m_layerMin == value)
        return;
    m_layerMin = value;
    update();
}
```
CRITICAL: The new `setRoleVisibility` must NOT mutate `m_previewData`. It only calls `update()`.

### Render-Side Skip Pattern (no repack)
**Source:** `src/qml_gui/Renderer/RhiViewportRenderer.cpp` lines 692-696
**Apply to:** `computePreviewDrawRange()` role filtering
```cpp
if (span.layer < layerLow || span.layer > layerHigh)
    continue;
if (span.move >= m_moveEnd)
    continue;
```
The role check follows the same `continue` pattern:
```cpp
if (span.role >= 0 && span.role < m_roleVisibility.size()
    && !m_roleVisibility[span.role])
    continue;
```

### PackedSegment / GcvPackedSegment Lock-Step
**Source:** `src/core/viewmodels/PreviewViewModel.cpp` lines 46-66 AND `src/qml_gui/Renderer/RhiViewportRenderer.cpp` lines 571-577
**Apply to:** Both structs must be updated with `int role;` in the same commit. Add `static_assert` guard:
```cpp
// In RhiViewportRenderer.cpp, after GcvPackedSegment:
static_assert(sizeof(GcvPackedSegment) == 76, "GcvPackedSegment must be 76 bytes after adding role");
```

### ViewModel Property + Signal Pattern
**Source:** `src/core/viewmodels/PreviewViewModel.h` lines 19-81
**Apply to:** All new PreviewViewModel Q_PROPERTY additions
```cpp
Q_PROPERTY(QVariantList roleVisibilities READ roleVisibilities NOTIFY stateChanged)
```
Single `stateChanged()` signal covers all property changes (established pattern in this codebase).

### QML Component Registration
**Source:** `src/qml_gui/qml.qrc` lines 60-66
**Apply to:** New `components/VisibilityFilter.qml` must be added to `<qresource prefix="/">` block alongside existing `StatsPanel.qml` and `Legend.qml`.

### Test CMake Registration
**Source:** `CMakeLists.txt` lines 359-396
**Apply to:** New `PreviewParserTests` target follows the same `qt_add_executable` + `target_link_libraries` + `add_test` pattern. Source-audit tests (QmlUiAuditTests-style) link only `Qt6::Test`. Integration tests (ViewModelSmokeTests/E2EWorkflowTests-style) link `owzx_app_core` + full Qt modules.

### AUTOMOC Note for Single-File Tests
**Source:** `CMakeLists.txt` lines 370-376
**Apply to:** If `PreviewParserTests` is a single `.cpp` with `Q_OBJECT`, the AUTOMOC caveat applies: "single-file tests with cpp-internal Q_OBJECT have weak moc dependency tracking." Delete `build/PreviewParserTests_autogen/timestamp` after adding new slots in incremental builds.

## No Analog Found

Files with no close match in the codebase (planner should use RESEARCH.md patterns instead):

| File | Role | Data Flow | Reason |
|------|------|-----------|--------|
| `tests/fixtures/*.gcode` | test fixture | file-I/O | No test fixture G-code files exist in `tests/` yet. Use upstream OrcaSlicer G-code output format from RESEARCH.md Section 2 as reference. The fixture must contain `;TYPE:` comments matching `ExtrusionEntity::role_to_string()` output. |

Note: This is the only gap. The upstream G-code fixture format is well-documented in RESEARCH.md (upstream `EGCodeExtrusionRole` reference, Section 2). The planner should generate a minimal realistic .gcode file with at least one segment per major extrusion role (Inner wall, Outer wall, Sparse infill, Top surface, Support, Skirt, Bridge, etc.) plus travel moves and tagged comments (`;HEIGHT:`, `;WIDTH:`, `;FEEDRATE:`).

## Metadata

**Analog search scope:** `src/qml_gui/components/`, `src/qml_gui/Renderer/`, `src/core/viewmodels/`, `tests/`, `src/qml_gui/pages/`, `CMakeLists.txt`
**Files scanned:** 12 (CollapsibleSection.qml, StatsPanel.qml, Legend.qml, CxCheckBox.qml, PreviewViewModel.h, PreviewViewModel.cpp, RhiViewport.h, RhiViewport.cpp, RhiViewportRenderer.h, RhiViewportRenderer.cpp, PreviewPage.qml, CMakeLists.txt, QmlUiAuditTests.cpp, ViewModelSmokeTests.cpp, E2EWorkflowTests.cpp, qml.qrc)
**Pattern extraction date:** 2026-07-02
