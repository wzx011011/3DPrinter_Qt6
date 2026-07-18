import QtQuick
import QtQuick.Controls
import ".."

SpinBox {
    id: root

    // Unit suffix property for rendering unit text right of the numeric input
    // (UI-SPEC int-type row: unit suffix rendered as Text right of CxSpinBox)
    property string suffix: ""

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

    contentItem: Item {
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height

        TextInput {
            id: spinInput
            z: 2
            anchors.left: parent.left
            anchors.right: suffixText.visible ? suffixText.left : parent.right
            anchors.verticalCenter: parent.verticalCenter
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

        Text {
            id: suffixText
            visible: root.suffix !== ""
            anchors.right: parent.right
            anchors.rightMargin: root.up.indicator.width + 6
            anchors.verticalCenter: parent.verticalCenter
            text: root.suffix
            color: Theme.textTertiary
            font.pixelSize: Theme.fontSizeXS
            font.family: root.font.family
        }
    }

    up.indicator: Rectangle {
        x: parent.width - width
        height: parent.height / 2
        implicitWidth: 22
        color: root.up.pressed ? Theme.bgPressed : root.up.hovered ? Theme.bgHover : "transparent"
        Text { anchors.centerIn: parent; text: "▲"; color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
    }

    down.indicator: Rectangle {
        x: parent.width - up.indicator.width
        y: parent.height / 2
        height: parent.height / 2
        implicitWidth: 22
        color: root.down.pressed ? Theme.bgPressed : root.down.hovered ? Theme.bgHover : "transparent"
        Text { anchors.centerIn: parent; text: "▼"; color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
    }
}
