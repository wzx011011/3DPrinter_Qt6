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
        // ctl-12: flat base, same color as the hosting panel (upstream input
        // background maps to the window base in dark mode; no raised step).
        color: {
            if (!root.enabled) return Theme.bgPanel
            if (root.activeFocus) {
                // ctl-10: focused field picks up a ~10% accent tint over the
                // panel base (upstream ComboBox.cpp:54-59 focused #E5F0EE
                // semantics, adapted to the OWzx accent).
                const a = Theme.accent
                const b = Theme.bgPanel
                return Qt.rgba(b.r + (a.r - b.r) * 0.1,
                               b.g + (a.g - b.g) * 0.1,
                               b.b + (a.b - b.b) * 0.1, 1)
            }
            return Theme.bgPanel
        }
        border.color: {
            if (!root.enabled) return Theme.borderSubtle
            if (root.activeFocus) return Theme.borderFocus
            // ctl-10: hover border turns accent (upstream TextInput.cpp:33-35
            // Normal #DBDBDB -> Hovered teal); OWzx accent dark tier per the
            // adjudicated value.
            if (root.hovered) return Theme.accentDark
            return Theme.borderDefault
        }
        border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
        Behavior on border.color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
        opacity: root.enabled ? 1.0 : 0.45
    }
}
