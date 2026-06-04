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
        radius: 2
        color: Theme.borderSubtle
        opacity: root.enabled ? 1.0 : 0.45

        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height
            color: root.enabled ? Theme.accent : Theme.textDisabled
            radius: 2
        }
    }

    handle: Rectangle {
        x: root.leftPadding + root.visualPosition * (root.availableWidth - width)
        y: root.topPadding + root.availableHeight / 2 - height / 2
        width: 14
        height: 14
        radius: 7
        color: root.pressed ? Theme.accentLight : Theme.accent
        border.color: Theme.accentDark
        border.width: 2
        opacity: root.enabled ? 1.0 : 0.45
        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
        scale: root.pressed ? 0.95 : 1.0
        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }
    }
}
