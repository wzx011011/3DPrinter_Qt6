import QtQuick
import QtQuick.Controls

SpinBox {
    id: root

    implicitHeight: 28
    implicitWidth: 90
    font.pixelSize: 12

    background: Rectangle {
        radius: 4
        color: "#252a34"
        border.color: root.activeFocus ? "#18c75e" : "#3a4050"
        border.width: 1
    }

    contentItem: TextInput {
        z: 2
        text: root.textFromValue(root.value, root.locale)
        color: "#d8e0ec"
        font: root.font
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !root.editable
        validator: root.validator
        selectionColor: "#18c75e"
        selectedTextColor: "#0d1017"
    }

    up.indicator: Rectangle {
        x: parent.width - width
        height: parent.height / 2
        implicitWidth: 22
        color: root.up.pressed ? "#3a4050" : root.up.hovered ? "#2e3440" : "transparent"
        Text { anchors.centerIn: parent; text: "▲"; color: "#8a96a8"; font.pixelSize: 8 }
    }

    down.indicator: Rectangle {
        x: parent.width - up.indicator.width
        y: parent.height / 2
        height: parent.height / 2
        implicitWidth: 22
        color: root.down.pressed ? "#3a4050" : root.down.hovered ? "#2e3440" : "transparent"
        Text { anchors.centerIn: parent; text: "▼"; color: "#8a96a8"; font.pixelSize: 8 }
    }
}
