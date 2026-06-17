import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"
import "../components"

// ─────────────────────────────────────────────────────────────────────────────
// DockableSidebar.qml — Phase 4 Sidebar Dockable 容器
//
// 上游契约（third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp）：
//   - Sidebar 是固定左侧 wxPanel + collapse 显隐（collapse_sidebar, Plater.cpp:4452）
//   - EVT_GLCANVAS_COLLAPSE_SIDEBAR 触发 toggle（Plater.cpp:5117/5184）
//   - 持久化 app_config collapsed_sidebar（Plater.cpp:5399）
//
// QML 实现（Phase 4 增强，详见 04-CONTEXT.md）：
//   - 包装 LeftSidebar（contentItem）作为可滚动内容区
//   - 折叠按钮在标题栏右侧（对齐上游 collapse_toolbar；Phase 6 迁到 GLCanvas overlay）
//   - 右/左边缘拖拽 handle 调宽度（增强，上游固定宽度）
//   - dockArea 由 BackendContext 管理，PreparePage 决定左右位置（本组件不关心顺序）
//   - 折叠时宽度收为 0，viewportArea 独占（对齐上游 Sidebar.Show/Hide）
//
// 本组件只管"一个 sidebar 的外观 + 折叠 + 拖宽"；
// dock Left/Right 的位置由 PreparePage 的 RowLayout 排序决定。
// ─────────────────────────────────────────────────────────────────────────────

Item {
    id: root

    // ViewModel 注入（透传给内部 LeftSidebar）
    required property var editorVm
    required property var configVm
    property string processCategory: ""

    // Dock 状态（绑定 backend.sidebarXxx）
    required property bool collapsed
    required property int sidebarWidth      // 当前宽度（展开时）
    required property int minWidth
    required property int maxWidth

    // 宽度：折叠时为 0（viewportArea 独占），展开时为 sidebarWidth
    implicitWidth: collapsed ? 0 : sidebarWidth
    Behavior on implicitWidth { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }

    // 对外 alias（供外部访问内部 LeftSidebar）
    property alias contentSidebar: leftSidebar

    // 内部状态：拖拽中
    property bool _dragging: false

    // ── 主体（折叠时隐藏）──
    Rectangle {
        id: sidebarBody
        anchors.fill: parent
        // 折叠时整体隐藏（宽度已为 0，此处 visible 防止残留渲染）
        visible: !root.collapsed
        color: Theme.bgPanel
        radius: 18
        border.width: 1
        border.color: Theme.borderSubtle

        // ── 标题栏（折叠按钮 + dock 切换占位）──
        // 对齐上游 Sidebar 标题栏，右侧放折叠按钮（Phase 6 会迁到 GLCanvas overlay）
        Item {
            id: titleBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: dragHandle.left
            height: 36

            Text {
                anchors.left: parent.left
                anchors.leftMargin: Theme.spacingMD
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Settings")
                color: Theme.textSecondary
                font.pixelSize: 13
                font.weight: Font.Medium
            }

            // 折叠按钮（对齐上游 collapse_toolbar 图标）
            // onClicked 调用外部注入的 toggleRequested 回调（转发到 backend.requestToggleSidebar）
            CxButton {
                id: collapseBtn
                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                implicitWidth: 28
                implicitHeight: 28
                text: "◀"  // Phase 6 替换为 Theme 图标
                onClicked: {
                    if (root.toggleRequested)
                        root.toggleRequested()
                }
            }
        }

        // ── 内容区：LeftSidebar ──
        LeftSidebar {
            id: leftSidebar
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            // LeftSidebar 自带 Rectangle 外框，此处隐藏其外框避免双层
            // （LeftSidebar 的 color/radius/border 由其自身控制）
            editorVm: root.editorVm
            configVm: root.configVm
            processCategory: root.processCategory
        }
    }

    // ── 折叠后的展开按钮（贴在窗口边缘的窄条）──
    // 对齐上游折叠后 GLCanvas 边缘的展开 handle
    Rectangle {
        id: collapsedHandle
        anchors.fill: parent
        visible: root.collapsed
        color: Theme.bgPanel
        radius: 4

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            width: 6
            height: 48
            radius: 3
            color: Theme.borderStrong
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (root.toggleRequested)
                    root.toggleRequested()
            }
        }
    }

    // ── 拖拽 handle（调宽度）──
    // 放在 sidebar 朝向 viewportArea 的边缘（由 dockArea 决定左右，这里统一放右侧，
    //  PreparePage 在 dockArea=Right 时镜像翻转本组件）
    Rectangle {
        id: dragHandle
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 6
        color: dragArea.containsMouse || root._dragging ? Theme.accent : Theme.borderSubtle
        opacity: root.collapsed ? 0 : 0.6
        visible: !root.collapsed

        MouseArea {
            id: dragArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.SplitHCursor

            // 拖拽实时改 sidebarWidth（通过 backend 持久化）
            property real _startX: 0
            property int _startWidth: 0
            onPressed: (mouse) => {
                _startX = mapToGlobal(mouse.x, mouse.y).x
                _startWidth = root.sidebarWidth
                root._dragging = true
            }
            onPositionChanged: (mouse) => {
                if (!pressed) return
                const gx = mapToGlobal(mouse.x, mouse.y).x
                const delta = Math.round(gx - _startX)
                // dockArea=Right 时反向（拖右减小宽度）—— 由外部传入 isRightDocked
                const newW = root._isRightDocked ? (_startWidth - delta) : (_startWidth + delta)
                // 实时更新（clamp 在 backend）
                if (root.widthChanged)
                    root.widthChanged(newW)
            }
            onReleased: {
                root._dragging = false
            }
        }
    }

    // dockArea=Right 标志（外部传入，决定拖拽方向）
    property bool _isRightDocked: false
    // 宽度变更回调（外部注入，转发到 backend.requestSetSidebarWidth）
    property var widthChanged: null
    // 折叠/展开回调（外部注入，转发到 backend.requestToggleSidebar）
    property var toggleRequested: null
}
