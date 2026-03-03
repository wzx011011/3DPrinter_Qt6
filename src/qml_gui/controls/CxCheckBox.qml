import QtQuick
import QtQuick.Controls

CheckBox {
    id: root

    font.pixelSize: 12

    indicator: Rectangle {
        implicitWidth: 16
        implicitHeight: 16
        x: root.leftPadding
        y: (root.height - height) / 2
        radius: 3
        color: root.checked ? "#18c75e" : "#1e2330"
        border.color: root.checked ? "#18c75e" : (root.hovered ? "#4e5568" : "#3a4050")
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: "✓"
            color: "white"
            font.pixelSize: 10
            font.bold: true
            visible: root.checked
        }
    }

    contentItem: Text {
        leftPadding: root.indicator.width + root.spacing
        text: root.text
        color: root.enabled ? "#c8d0dc" : "#566070"
        font: root.font
        verticalAlignment: Text.AlignVCenter
    }
}
