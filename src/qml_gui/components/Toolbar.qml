import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    height: 36
    color: "#131720"

    property var model: []
    signal toolClicked(string toolId)

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 2

        Repeater {
            model: root.model
            delegate: Rectangle {
                required property var modelData
                Layout.preferredWidth: 32
                Layout.preferredHeight: 28
                radius: 4
                color: toolHover.containsMouse ? "#2e3444" : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: parent.modelData.icon || "⧉"
                    color: "#a0abbe"
                    font.pixelSize: 14
                }

                ToolTip.visible: toolHover.containsMouse && parent.modelData.tooltip
                ToolTip.text: parent.modelData.tooltip || ""
                ToolTip.delay: 600

                HoverHandler { id: toolHover }
                TapHandler { onTapped: root.toolClicked(parent.modelData.id) }
            }
        }

        Item { Layout.fillWidth: true }
    }
}
