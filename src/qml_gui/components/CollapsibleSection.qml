import QtQuick
import QtQuick.Layouts
import ".."

// 可折叠区域组件（对齐上游 Sidebar 可折叠面板行为）
// 点击标题栏切换展开/折叠，带高度和透明度动画
Item {
    id: root
    required property string title
    property string iconText: ""
    property bool expanded: true
    property alias rightActions: actionsRow.data
    default property alias content: contentContainer.data

    implicitWidth: parent ? parent.width : 300
    implicitHeight: titleBar.implicitHeight + (root.expanded ? contentContainer.implicitHeight : 0)

    Behavior on implicitHeight {
        enabled: contentContainer.implicitHeight > 0
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
    }

    // 标题栏（可点击切换折叠）
    Rectangle {
        id: titleBar
        anchors.top: parent.top
        width: parent.width
        height: 34
        color: "#151a22"
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

    // 内容容器（折叠时裁剪隐藏）
    Item {
        id: contentContainer
        anchors.top: titleBar.bottom
        anchors.topMargin: root.expanded ? 6 : 0
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.expanded ? implicitHeight : 0
        clip: true
        visible: root.expanded || heightAnimation.running

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
