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
// QML 务实实现（详见 03-PLAN.md Wave 2 "务实方案"）：
//   - Plater 作为 wrapper 容器，内部常驻 PreparePage + PreviewPage 两个实例
//   - viewMode 驱动哪个 Page visible（View3D→PreparePage, Preview→PreviewPage）
//   - 两 Page 实例都不销毁 → 满足 ARCH-07 状态保留
//   - 单一 Plater 容器被 Prepare/Preview tab 共享 → 满足 ARCH-05
//   - viewMode 三选一切内部视图 → 满足 ARCH-06
//
// 对上游契约的偏离：上游是 view3D/preview 两独立 canvas 共享 model；
//   本方案是两 Page（各自 canvas）常驻。功能等价（状态保留 + 视图切换），
//   实现路径不同。未来若要真正共享单 canvas，需把 GLViewport 提到 Plater 级（Phase 6+）。
// ─────────────────────────────────────────────────────────────────────────────

Item {
    id: root

    // ViewModel 注入（main.qml 从 backend 透传）
    required property var editorVm
    required property var previewVm
    property var configVm

    /// Plater 当前视图模式（绑定 backend.currentViewMode）
    /// vmView3D=0 / vmPreview=1 / vmAssembleView=2
    required property int viewMode

    /// 对外 alias —— 供 main.qml BBLTopbar / 菜单接线
    /// （替代 Phase 02 时期直接 id=preparePage 的引用方式）
    property alias preparePageRef: preparePage
    property alias previewPageRef: previewPage

    /// 左侧面板可见性透传（PreparePage 原有属性，供 BBLTopbar 折叠按钮接线）
    property bool leftPanelVisible: true

    /// 处理流程分类透传（PreparePage 原有属性）
    property string processCategory: ""

    // Phase 4: sidebar dockable 三态透传 (backend → Plater → PreparePage)
    property bool sidebarCollapsed: false
    property int sidebarWidth: 390
    property int sidebarMinWidth: 360
    property int sidebarMaxWidth: 480
    property int sidebarDockArea: 0
    property var sidebarToggleRequested: null
    property var sidebarWidthChanged: null

    focus: true

    // vm 常量（与 BackendContext::ViewMode 数值对齐：View3D=0/Preview=1/AssembleView=2）
    readonly property int vmView3D: 0
    readonly property int vmPreview: 1
    readonly property int vmAssembleView: 2

    // ═══ View3D slot（Prepare 视图，常驻不销毁）═══════════════════════════════
    // 对齐上游 Plater::view3D —— 对象编辑/3D 场景 canvas
    PreparePage {
        id: preparePage
        anchors.fill: parent
        // 常驻实例：visible 切换，不销毁（ARCH-07 状态保留）
        visible: root.viewMode === root.vmView3D
        editorVm: root.editorVm
        configVm: root.configVm
        leftPanelVisible: root.leftPanelVisible
        processCategory: root.processCategory
        // Phase 4: sidebar 三态透传
        sidebarCollapsed: root.sidebarCollapsed
        sidebarWidth: root.sidebarWidth
        sidebarMinWidth: root.sidebarMinWidth
        sidebarMaxWidth: root.sidebarMaxWidth
        sidebarDockArea: root.sidebarDockArea
        sidebarToggleRequested: root.sidebarToggleRequested
        sidebarWidthChanged: root.sidebarWidthChanged
    }

    // ═══ Preview slot（G-code 预览，常驻不销毁）═══════════════════════════════
    // 对齐上游 Plater::preview —— 切片结果/G-code 回放 canvas
    PreviewPage {
        id: previewPage
        anchors.fill: parent
        // 常驻实例：visible 切换，不销毁（ARCH-07 状态保留）
        visible: root.viewMode === root.vmPreview
        previewVm: root.previewVm
        editorVm: root.editorVm
        configVm: root.configVm
        processCategory: root.processCategory
    }

    // ═══ AssembleView slot（v2.0 占位）═══════════════════════════════════════
    // 上游 AssembleView 在 v2.0 为 Out of Scope，仅保留枚举入口
    Item {
        id: assembleSlot
        anchors.fill: parent
        visible: root.viewMode === root.vmAssembleView

        Text {
            anchors.centerIn: parent
            text: qsTr("装配视图暂不可用")
            color: Theme.textSecondary
            font.pixelSize: 14
        }
    }
}
