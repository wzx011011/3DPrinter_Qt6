import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OWzxGL 1.0
import ".."
import "../controls"
import "../components"

// ─────────────────────────────────────────────────────────────────────────────
// GLToolbars.qml — Phase G4 3D 视口工具栏 overlay
//
// 对齐上游 OrcaSlicer 在 GL canvas 上绘制的工具栏（ImGui），用 QML overlay 等价实现。
// 上游参考: third_party/OrcaSlicer/src/slic3r/GUI/GLCanvas3D.cpp (GLToolbar 渲染)
//
// 三个 overlay:
//   1. MainToolbar (顶部水平): +Add/+Plate/Orient/Arrange | More/Split/Layers
//   2. Gizmos 竖条 (左侧垂直): Move/Rotate/Scale/Flatten/Cut/Support/Seam/...
//   3. ViewToolbar (右侧垂直): Top/Front/Right/Back/ISO/Reset
//
// 接线: editorVm 的 Q_INVOKABLE action + viewport3d.gizmoMode + viewport3d.requestViewPreset
// 挂载点: Plater.qml 的 viewportArea（anchors 到视口边缘）
// ─────────────────────────────────────────────────────────────────────────────

Item {
    id: root

    // ViewModel + viewport 注入（由 Plater 提供）
    required property var editorVm
    required property var viewport3d  // GLViewport 实例（用于 gizmoMode/requestViewPreset）

    // ═══════════════════════════════════════════════════════════════════════
    // 1. MainToolbar — 顶部水平工具栏（对齐上游 GLCanvas3D MainToolbar）
    // 按钮: +Add | +Plate | Orient | Arrange || More | Split | Layers
    // ═══════════════════════════════════════════════════════════════════════
    Rectangle {
        id: mainToolbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 14
        anchors.leftMargin: 14
        width: mainToolbarRow.implicitWidth + 24
        height: 46
        radius: 12
        color: Theme.bgFloating
        border.width: 1
        border.color: Theme.borderSubtle

        Row {
            id: mainToolbarRow
            anchors.centerIn: parent
            spacing: 6

            // 第一组: 导入/平板/朝向/排列
            // +Add (导入模型，对齐上游 "Add" — 弹出文件对话框由 PreparePage 处理)
            GlyphButton {
                glyph: "＋"; tip: qsTr("导入模型 (Add)")
                enabled: root.editorVm
                onClicked: addModelRequested()
            }
            // +Plate (添加平板，对齐上游 "Add plate")
            GlyphButton {
                glyph: "⊞"; tip: qsTr("添加平板 (Add Plate)")
                enabled: root.editorVm && root.editorVm.plateCount < 10
                onClicked: if (root.editorVm) root.editorVm.addPlate()
            }
            // Orient (自动朝向，对齐上游 "Orient")
            GlyphButton {
                glyph: "⊙"; tip: qsTr("自动朝向 (Orient)")
                enabled: root.editorVm && root.editorVm.selectedObjectCount > 0
                onClicked: if (root.editorVm) root.editorVm.autoOrientSelected()
            }
            // Arrange (排布，对齐上游 "Arrange")
            GlyphButton {
                glyph: "⇲"; tip: qsTr("自动排布 (Arrange)")
                enabled: root.editorVm
                onClicked: if (root.editorVm) root.editorVm.arrangeAllObjects()
            }

            // 分隔线 (Row 子项不能用 anchors，靠 Row 默认垂直居中)
            Rectangle { width: 1; height: 22; color: Theme.borderSubtle }

            // 第二组: 更多/拆分/层编辑
            // More (复制选中，对齐上游 "More" — 实际是复制/克隆)
            GlyphButton {
                glyph: "⧉"; tip: qsTr("复制选中 (Duplicate)")
                enabled: root.editorVm && root.editorVm.selectedObjectCount > 0
                onClicked: if (root.editorVm) root.editorVm.duplicateSelectedObjects()
            }
            // Split (拆分对象，对齐上游 "Split")
            GlyphButton {
                glyph: "⫶"; tip: qsTr("拆分对象 (Split)")
                enabled: root.editorVm && root.editorVm.selectedObjectCount === 1
                onClicked: if (root.editorVm) root.editorVm.splitSelectedObject()
            }
            // Layers (层编辑占位 — 对齐上游 "LayersEditing"，v2.0 占位)
            GlyphButton {
                glyph: "≡"; tip: qsTr("层编辑 (Layers)")
                enabled: false  // TODO: 层编辑功能待实现
                onClicked: {}
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // 2. Gizmos 竖条 — 左侧垂直工具栏（对齐上游 GLGizmosManager）
    // 按钮: Move/Rotate/Scale/Flatten/Cut/Support/Seam/Simplify/Measure/Emboss
    // ═══════════════════════════════════════════════════════════════════════
    Rectangle {
        id: gizmosBar
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 14
        width: 42
        height: gizmosCol.implicitHeight + 16
        radius: 12
        color: Theme.bgFloating
        border.width: 1
        border.color: Theme.borderSubtle

        Column {
            id: gizmosCol
            anchors.centerIn: parent
            spacing: 2

            // Move (对齐上游 GLGizmoMove, 快捷键 W)
            GizmoButton {
                glyph: "✥"; tip: qsTr("移动 (Move) — W")
                active: root.viewport3d && root.viewport3d.gizmoMode === GLViewport.GizmoMove
                enabled: root.editorVm && root.editorVm.selectedObjectCount > 0
                onClicked: if (root.viewport3d) root.viewport3d.gizmoMode = GLViewport.GizmoMove
            }
            // Rotate (对齐上游 GLGizmoRotate, 快捷键 E)
            GizmoButton {
                glyph: "⟳"; tip: qsTr("旋转 (Rotate) — E")
                active: root.viewport3d && root.viewport3d.gizmoMode === GLViewport.GizmoRotate
                enabled: root.editorVm && root.editorVm.selectedObjectCount > 0
                onClicked: if (root.viewport3d) root.viewport3d.gizmoMode = GLViewport.GizmoRotate
            }
            // Scale (对齐上游 GLGizmoScale, 快捷键 R)
            GizmoButton {
                glyph: "⤢"; tip: qsTr("缩放 (Scale) — R")
                active: root.viewport3d && root.viewport3d.gizmoMode === GLViewport.GizmoScale
                enabled: root.editorVm && root.editorVm.selectedObjectCount > 0
                onClicked: if (root.viewport3d) root.viewport3d.gizmoMode = GLViewport.GizmoScale
            }

            // 分隔
            Rectangle { width: 26; height: 1; color: Theme.borderSubtle }

            // Flatten (对齐上游 GLGizmoFlatten, 快捷键 G)
            GizmoButton {
                glyph: "⏚"; tip: qsTr("平放 (Flatten) — G")
                active: root.viewport3d && root.viewport3d.gizmoMode === GLViewport.GizmoFlatten
                enabled: root.editorVm && root.editorVm.selectedObjectCount > 0
                onClicked: if (root.viewport3d) root.viewport3d.gizmoMode = GLViewport.GizmoFlatten
            }
            // Cut (对齐上游 GLGizmoCut, 快捷键 Ctrl+Shift+X)
            GizmoButton {
                glyph: "✂"; tip: qsTr("切割 (Cut)")
                active: root.viewport3d && root.viewport3d.gizmoMode === GLViewport.GizmoCut
                enabled: root.editorVm && root.editorVm.selectedObjectCount > 0
                onClicked: if (root.viewport3d) root.viewport3d.gizmoMode = GLViewport.GizmoCut
            }
            // Advanced Cut (对齐上游 GLGizmoAdvancedCut)
            GizmoButton {
                glyph: "⌗"; tip: qsTr("高级切割 (Advanced Cut)")
                active: root.viewport3d && root.viewport3d.gizmoMode === GLViewport.GizmoAdvancedCut
                enabled: root.editorVm && root.editorVm.selectedObjectCount > 0
                onClicked: if (root.viewport3d) root.viewport3d.gizmoMode = GLViewport.GizmoAdvancedCut
            }

            // 分隔
            Rectangle { width: 26; height: 1; color: Theme.borderSubtle }

            // Support Paint (对齐上游 GLGizmoFdmSupports, 快捷键 P)
            GizmoButton {
                glyph: "⚑"; tip: qsTr("支撑绘制 (Support) — P")
                active: root.viewport3d && root.viewport3d.gizmoMode === GLViewport.GizmoSupportPaint
                enabled: root.editorVm && root.editorVm.selectedObjectCount > 0
                onClicked: if (root.viewport3d) root.viewport3d.gizmoMode = GLViewport.GizmoSupportPaint
            }
            // Seam Paint (对齐上游 GLGizmoSeam)
            GizmoButton {
                glyph: "◉"; tip: qsTr("缝线绘制 (Seam)")
                active: root.viewport3d && root.viewport3d.gizmoMode === GLViewport.GizmoSeamPaint
                enabled: root.editorVm && root.editorVm.selectedObjectCount > 0
                onClicked: if (root.viewport3d) root.viewport3d.gizmoMode = GLViewport.GizmoSeamPaint
            }
            // Simplify (对齐上游 GLGizmoSimplify)
            GizmoButton {
                glyph: "▽"; tip: qsTr("简化 (Simplify)")
                active: root.viewport3d && root.viewport3d.gizmoMode === GLViewport.GizmoSimplify
                enabled: root.editorVm && root.editorVm.selectedObjectCount > 0
                onClicked: if (root.viewport3d) root.viewport3d.gizmoMode = GLViewport.GizmoSimplify
            }
            // Measure (对齐上游 GLGizmoMeasure, 快捷键 Ctrl+U)
            GizmoButton {
                glyph: "📐"; tip: qsTr("测量 (Measure) — Ctrl+U")
                active: root.viewport3d && root.viewport3d.gizmoMode === GLViewport.GizmoMeasure
                enabled: root.editorVm
                onClicked: if (root.viewport3d) root.viewport3d.gizmoMode = GLViewport.GizmoMeasure
            }
            // Mesh Boolean (对齐上游 GLGizmoMeshBoolean)
            GizmoButton {
                glyph: "⊕"; tip: qsTr("布尔运算 (Mesh Boolean)")
                active: root.viewport3d && root.viewport3d.gizmoMode === GLViewport.GizmoMeshBoolean
                enabled: root.editorVm && root.editorVm.selectedObjectCount > 0
                onClicked: if (root.viewport3d) root.viewport3d.gizmoMode = GLViewport.GizmoMeshBoolean
            }
            // Emboss (对齐上游 GLGizmoEmboss)
            GizmoButton {
                glyph: "T"; tip: qsTr("浮雕文字 (Emboss)")
                active: root.viewport3d && root.viewport3d.gizmoMode === GLViewport.GizmoEmboss
                enabled: root.editorVm
                onClicked: if (root.viewport3d) root.viewport3d.gizmoMode = GLViewport.GizmoEmboss
            }
            // SVG (对齐上游 GLGizmoSVG)
            GizmoButton {
                glyph: "◈"; tip: qsTr("SVG 导入 (Svg)")
                active: root.viewport3d && root.viewport3d.gizmoMode === GLViewport.GizmoSVG
                enabled: root.editorVm
                onClicked: if (root.viewport3d) root.viewport3d.gizmoMode = GLViewport.GizmoSVG
            }
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // 3. ViewToolbar — 右侧垂直视角预设（对齐上游 GLCanvas3D ViewToolbar）
    // 按钮: Top/Front/Right/Back/ISO/Reset(Fit)
    // ═══════════════════════════════════════════════════════════════════════
    Rectangle {
        id: viewToolbar
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 14
        anchors.rightMargin: 14
        width: 42
        height: viewCol.implicitHeight + 16
        radius: 12
        color: Theme.bgFloating
        border.width: 1
        border.color: Theme.borderSubtle

        Column {
            id: viewCol
            anchors.centerIn: parent
            spacing: 2

            // Top (对齐上游视角预设 0)
            GizmoButton {
                glyph: "⊤"; tip: qsTr("俯视图 (Top)")
                onClicked: if (root.viewport3d) root.viewport3d.requestViewPreset(0)
            }
            // Front (对齐上游视角预设 1)
            GizmoButton {
                glyph: "⊓"; tip: qsTr("前视图 (Front)")
                onClicked: if (root.viewport3d) root.viewport3d.requestViewPreset(1)
            }
            // Right (对齐上游视角预设 2)
            GizmoButton {
                glyph: "⊐"; tip: qsTr("右视图 (Right)")
                onClicked: if (root.viewport3d) root.viewport3d.requestViewPreset(2)
            }
            // ISO (对齐上游视角预设 3 — 等轴视图)
            GizmoButton {
                glyph: "⟀"; tip: qsTr("等轴视图 (ISO)")
                onClicked: if (root.viewport3d) root.viewport3d.requestViewPreset(3)
            }
            // Fit/Reset (适应视图)
            GizmoButton {
                glyph: "⊡"; tip: qsTr("适应视图 (Fit)")
                onClicked: fitViewRequested()
            }
        }
    }

    // ── 对外信号/回调（由 Plater 接线）──
    // +Add 按钮需要弹文件对话框（PreparePage 持有 FileDialog）
    signal addModelRequested()
    signal fitViewRequested()

    // ── 内联按钮组件（MainToolbar 用的水平按钮）──
    component GlyphButton: Item {
        id: gb
        property string glyph: ""
        property string tip: ""
        property bool enabled: true
        signal clicked()
        width: 34
        height: 34
        opacity: gb.enabled ? 1.0 : 0.35
        Rectangle {
            anchors.fill: parent
            radius: 7
            color: gb.enabled && ma.containsMouse ? Theme.bgHover : "transparent"
        }
        Text {
            anchors.centerIn: parent
            text: gb.glyph
            color: gb.enabled ? Theme.textPrimary : Theme.textDisabled
            font.pixelSize: 16
            font.bold: true
        }
        ToolTip.visible: ma.containsMouse && gb.tip !== ""
        ToolTip.text: gb.tip
        ToolTip.delay: 400
        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: gb.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: if (gb.enabled) gb.clicked()
        }
    }

    // ── 内联按钮组件（Gizmos/View 用的垂直按钮，含 active 高亮态）──
    component GizmoButton: Item {
        id: gz
        property string glyph: ""
        property string tip: ""
        property bool active: false
        property bool enabled: true
        signal clicked()
        width: 34
        height: 32
        opacity: gz.enabled ? 1.0 : 0.35
        Rectangle {
            anchors.fill: parent
            radius: 7
            color: gz.active ? Theme.accent : (gz.enabled && gma.containsMouse ? Theme.bgHover : "transparent")
            border.width: gz.active ? 1 : 0
            border.color: gz.active ? Theme.accent : "transparent"
        }
        Text {
            anchors.centerIn: parent
            text: gz.glyph
            color: gz.active ? Theme.textOnAccent : (gz.enabled ? Theme.textPrimary : Theme.textDisabled)
            font.pixelSize: 15
            font.bold: true
        }
        ToolTip.visible: gma.containsMouse && gz.tip !== ""
        ToolTip.text: gz.tip
        ToolTip.delay: 400
        MouseArea {
            id: gma
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: gz.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: if (gz.enabled) gz.clicked()
        }
    }
}
