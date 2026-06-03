import QtQuick
import QtQuick.Controls
import ".."

TextField {
    id: root

    implicitHeight: Theme.controlHeightSM
    leftPadding: Theme.spacingMD
    rightPadding: Theme.spacingMD
    font.pixelSize: Theme.fontSizeMD
    color: Theme.textPrimary
    selectionColor: Theme.selectionColor
    selectedTextColor: Theme.selectionText
    placeholderTextColor: Theme.textDisabled

    background: Rectangle {
        radius: Theme.radiusSM
        color: {
            if (!root.enabled) return Theme.bgPanel
            if (root.activeFocus) return Theme.bgPanel
            return Theme.bgElevated
        }
        border.color: {
            if (!root.enabled) return Theme.borderSubtle
            if (root.activeFocus) return Theme.borderFocus
            if (root.hovered) return Theme.borderStrong
            return Theme.borderDefault
        }
        border.width: 1
        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
        Behavior on border.color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
        opacity: root.enabled ? 1.0 : 0.45
    }
}
