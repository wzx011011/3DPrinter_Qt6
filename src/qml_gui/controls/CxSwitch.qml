import QtQuick
import QtQuick.Controls
import ".."

Switch {
    id: root

    indicator: Rectangle {
        x: root.leftPadding
        y: (root.height - height) / 2
        width: 36
        height: 20
        radius: 10

        color: {
            if (!root.enabled) return Theme.bgPanel
            if (root.checked) return root.hovered ? Theme.accentLight : Theme.accent
            return root.hovered ? Theme.bgHover : Theme.switchTrackOff
        }
        Behavior on color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }
        opacity: root.enabled ? 1.0 : 0.45

        Rectangle {
            x: root.checked ? parent.width - width - 2 : 2
            y: (parent.height - height) / 2
            width: 16
            height: 16
            radius: 8
            color: root.enabled ? (root.checked ? Theme.textOnAccent : Theme.switchKnob) : Theme.textDisabled
            Behavior on x { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
            Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
            scale: root.pressed ? 0.9 : 1.0
            Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }
        }
    }

    contentItem: Text {
        leftPadding: root.indicator.width + root.spacing
        text: root.text
        color: root.enabled ? Theme.textPrimary : Theme.textDisabled
        font.pixelSize: Theme.fontSizeMD
        verticalAlignment: Text.AlignVCenter
    }
}
