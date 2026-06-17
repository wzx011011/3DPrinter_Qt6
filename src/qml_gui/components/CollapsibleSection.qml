import QtQuick
import QtQuick.Layouts
import ".."

// 可折叠区域组件（对齐上游 Sidebar 可折叠面板行为）
// 点击标题栏切换展开/折叠，带高度和透明度动画
// G2 修复: 整个 section 作为统一卡片（背景+圆角+边框），标题栏与内容共用一层卡片背景
Item {
    id: root
    required property string title
    property string iconText: ""
    property bool expanded: true
    property alias rightActions: actionsRow.data
    default property alias content: contentContainer.data

    implicitWidth: parent ? parent.width : 300
    implicitHeight: card.height

    // G2: 外层卡片背景（统一标题栏+内容为一个卡片，对齐上游 OrcaSlicer 卡片式区块）
    Rectangle {
        id: card
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        // 高度 = 标题栏 + (展开时内容+间距)
        height: titleBar.height + (root.expanded ? (contentContainer.implicitHeight + 8) : 0)
        color: Theme.bgElevated      // 卡片背景（比主背景稍亮，形成层次）
        radius: 8
        border.width: 1
        border.color: Theme.borderSubtle

        Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
    }

    // 标题栏（可点击切换折叠）— 透明背景，继承卡片背景
    Rectangle {
        id: titleBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 34
        color: "transparent"           // G2: 透明，用卡片背景
        radius: 8

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 8
            spacing: 8

            Text {
                visible: root.iconText !== ""
                text: root.iconText
                color: Theme.accent
                font.pixelSize: 11
                font.bold: true
            }

            Text {
                text: root.title
                color: Theme.textPrimary
                font.pixelSize: 12
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            // 标题栏操作按钮（由外部注入）
            Row {
                id: actionsRow
                spacing: 2
            }

            // 展开/折叠箭头
            Text {
                text: root.expanded ? "▾" : "▸"
                color: Theme.textDisabled
                font.pixelSize: 10
                rotation: root.expanded ? 0 : -90
                Behavior on rotation { NumberAnimation { duration: 200 } }
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.expanded = !root.expanded
        }
    }

    // 标题栏与内容之间的分隔线（展开时显示，增强卡片内部层次）
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        y: titleBar.height - 1
        height: 1
        color: Theme.borderSubtle
        visible: root.expanded && contentContainer.implicitHeight > 0
        opacity: 0.6
    }

    // 内容容器（折叠时裁剪隐藏）— 透明背景，继承卡片背景
    Item {
        id: contentContainer
        anchors.top: titleBar.bottom
        anchors.topMargin: root.expanded ? 6 : 0
        anchors.left: parent.left
        anchors.right: parent.right
        implicitHeight: childrenRect.height
        height: root.expanded ? implicitHeight : 0
        clip: true
        visible: root.expanded || height > 0

        Behavior on height {
            id: heightAnimation
            NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
        }
        Behavior on opacity {
            NumberAnimation { duration: 150 }
        }
        opacity: root.expanded ? 1.0 : 0.0
    }
}
