import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OWzxGL 1.0
import ".."
import "../controls"
import "../components" as Components

// ─────────────────────────────────────────────────────────────────────────────
// Plater.qml — Phase 3 单一共享实例（对齐上游 Plater.cpp 的 wxPanel 单实例设计）
//
// 上游契约（third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp）：
//   - Plater 是单一 wxPanel，被 Prepare tab (tp3DEditor) 和 Preview tab (tpPreview) 共享
//   - 内部持有两个独立 GLCanvas3D（view3D + preview + assemble_view）
//   - 通过 viewMode 切换可见性（非销毁重建），保留全部状态（ARCH-07）
//
// QML 实现要点：
//   - 单实例放在 StackLayout 的 tp3DEditor slot（main.qml 负责）
//   - view3DSlot / previewSlot / assembleSlot 三个子 Item，由 backend.currentViewMode 驱动 visible
//   - Wave 1：仅占位（验证 viewMode 联动）；Wave 2：迁入 PreparePage / PreviewPage 内容
// ─────────────────────────────────────────────────────────────────────────────

Item {
    id: root

    // Wave 1 骨架属性（Wave 2 迁内容时复用）
    required property var editorVm
    required property var previewVm
    property var configVm

    /// Plater 当前视图模式（绑定 backend.currentViewMode）
    /// vmView3D=0 / vmPreview=1 / vmAssembleView=2
    required property int viewMode

    /// 对外接口 alias（Wave 2 迁内容后激活，Wave 1 留空占位）
    // property alias viewport3dRef: viewport3d

    focus: true

    // ── View3D slot（Prepare 视图）─────────────────────────────────────────────
    // Wave 1：占位；Wave 2：迁入 PreparePage.qml 的 GLViewport + 工具栏 + 对话框接线
    Item {
        id: view3DSlot
        anchors.fill: parent
        visible: root.viewMode === root.vmView3D  // backend.vmView3D 经 Plater 透传

        // Wave 1 占位标签（Wave 2 删除）
        Text {
            anchors.centerIn: parent
            text: qsTr("Plater — View3D slot (Wave 1 placeholder)")
            color: "#9aa7b5"
            font.pixelSize: 14
        }
    }

    // ── Preview slot（G-code 预览）─────────────────────────────────────────────
    // Wave 1：占位；Wave 2：迁入 PreviewPage.qml 的 GLViewport + LayerSlider + 键盘快捷键
    Item {
        id: previewSlot
        anchors.fill: parent
        visible: root.viewMode === root.vmPreview

        Text {
            anchors.centerIn: parent
            text: qsTr("Plater — Preview slot (Wave 1 placeholder)")
            color: "#9aa7b5"
            font.pixelSize: 14
        }
    }

    // ── AssembleView slot（v2.0 占位）──────────────────────────────────────────
    // 上游 AssembleView 在 v2.0 为 Out of Scope，仅保留枚举入口
    Item {
        id: assembleSlot
        anchors.fill: parent
        visible: root.viewMode === root.vmAssembleView

        Text {
            anchors.centerIn: parent
            text: qsTr("AssembleView — reserved (v2.0 placeholder)")
            color: "#9aa7b5"
            font.pixelSize: 14
        }
    }

    // ── vm 常量透传（供 visible 绑定直接引用，无需经 backend）──────────────────
    // 与 BackendContext::ViewMode 数值对齐：View3D=0/Preview=1/AssembleView=2
    readonly property int vmView3D: 0
    readonly property int vmPreview: 1
    readonly property int vmAssembleView: 2
}
