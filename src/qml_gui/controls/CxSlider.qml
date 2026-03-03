import QtQuick
import QtQuick.Controls

Slider {
    id: root

    implicitHeight: 20

    background: Rectangle {
        x: root.leftPadding
        y: root.topPadding + root.availableHeight / 2 - height / 2
        width: root.availableWidth
        height: 4
        radius: 2
        color: "#2e3440"

        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height
            color: "#18c75e"
            radius: 2
        }
    }

    handle: Rectangle {
        x: root.leftPadding + root.visualPosition * (root.availableWidth - width)
        y: root.topPadding + root.availableHeight / 2 - height / 2
        width: 14
        height: 14
        radius: 7
        color: root.pressed ? "#1ed36b" : "#18c75e"
        border.color: "#0a1a0f"
        border.width: 2
    }
}
