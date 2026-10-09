import QtQuick
import QtQuick.Controls
import ".."

Slider {
    id: root

    implicitHeight: 20

    background: Rectangle {
        x: root.leftPadding
        y: root.topPadding + root.availableHeight / 2 - height / 2
        width: root.availableWidth
        height: 4
        radius: Theme.radiusXS
        color: Theme.borderSubtle
        opacity: root.enabled ? 1.0 : 0.45

        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height
            color: root.enabled ? Theme.accent : Theme.textDisabled
            radius: Theme.radiusXS
        }
    }

    handle: Rectangle {
        x: root.leftPadding + root.visualPosition * (root.availableWidth - width)
        y: root.topPadding + root.availableHeight / 2 - height / 2
        width: 14
        height: 14
        radius: Theme.radiusLG
        color: root.pressed ? Theme.accentLight : Theme.accent
        // Phase 170 (P0-2): keyboard focus ring on the handle.
        border.color: root.activeFocus ? Theme.borderFocus : Theme.accentDark
        border.width: 2
        opacity: root.enabled ? 1.0 : 0.45
        Behavior on color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
        scale: root.pressed ? 0.95 : 1.0
        Behavior on scale { NumberAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
    }
}
