import QtQuick
import QtQuick.Controls
import ".."

SpinBox {
    id: root

    implicitHeight: Theme.controlHeightSM
    implicitWidth: 90
    font.pixelSize: Theme.fontSizeMD

    background: Rectangle {
        radius: Theme.radiusSM
        color: Theme.bgElevated
        border.color: root.activeFocus ? Theme.borderFocus : Theme.borderDefault
        border.width: 1
        opacity: root.enabled ? 1.0 : 0.45
    }

    contentItem: TextInput {
        z: 2
        text: root.textFromValue(root.value, root.locale)
        color: Theme.textPrimary
        font: root.font
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !root.editable
        validator: root.validator
        selectionColor: Theme.selectionColor
        selectedTextColor: Theme.selectionText
    }

    up.indicator: Rectangle {
        x: parent.width - width
        height: parent.height / 2
        implicitWidth: 22
        color: root.up.pressed ? Theme.bgPressed : root.up.hovered ? Theme.bgHover : "transparent"
        Text { anchors.centerIn: parent; text: "▲"; color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS - 2 }
    }

    down.indicator: Rectangle {
        x: parent.width - up.indicator.width
        y: parent.height / 2
        height: parent.height / 2
        implicitWidth: 22
        color: root.down.pressed ? Theme.bgPressed : root.down.hovered ? Theme.bgHover : "transparent"
        Text { anchors.centerIn: parent; text: "▼"; color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS - 2 }
    }
}
