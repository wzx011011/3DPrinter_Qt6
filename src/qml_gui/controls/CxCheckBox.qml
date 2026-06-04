import QtQuick
import QtQuick.Controls
import ".."

CheckBox {
    id: root

    font.pixelSize: Theme.fontSizeMD

    indicator: Rectangle {
        implicitWidth: 16
        implicitHeight: 16
        x: root.leftPadding
        y: (root.height - height) / 2
        radius: Theme.radiusSM
        color: root.checked ? Theme.accent : Theme.bgPanel
        border.color: root.checked ? Theme.accent : (root.hovered ? Theme.borderStrong : Theme.borderDefault)
        border.width: 1
        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
        Behavior on border.color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
        opacity: root.enabled ? 1.0 : 0.45

        Text {
            anchors.centerIn: parent
            text: "✓"
            color: Theme.textOnAccent
            font.pixelSize: Theme.fontSizeXS
            font.bold: true
            visible: root.checked
        }
    }

    contentItem: Text {
        leftPadding: root.indicator.width + root.spacing
        text: root.text
        color: root.enabled ? Theme.textPrimary : Theme.textDisabled
        font: root.font
        verticalAlignment: Text.AlignVCenter
    }
}
