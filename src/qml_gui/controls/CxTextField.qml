import QtQuick
import QtQuick.Controls

TextField {
    id: root

    implicitHeight: 28
    leftPadding: 8
    rightPadding: 8
    font.pixelSize: 12
    color: "#d8e0ec"
    selectionColor: "#18c75e"
    selectedTextColor: "#0d1017"
    placeholderTextColor: "#566070"

    background: Rectangle {
        radius: 4
        color: root.activeFocus ? "#1e2330" : "#252a34"
        border.color: root.activeFocus ? "#18c75e" : root.hovered ? "#4e5568" : "#3a4050"
        border.width: 1
    }
}
