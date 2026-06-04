import QtQuick
import QtQuick.Controls
import ".."

Button {
    id: root

    property int cxStyle: CxButton.Style.Primary
    property bool compact: false

    enum Style { Primary, Secondary, Danger, Ghost }

    implicitHeight: compact ? Theme.controlHeightSM : Theme.controlHeightMD
    implicitWidth: Math.max(64, contentItem.implicitWidth + leftPadding + rightPadding)
    leftPadding: Theme.spacingLG
    rightPadding: Theme.spacingLG

    background: Rectangle {
        radius: Theme.radiusSM
        color: {
            const d = !root.enabled
            const h = root.hovered
            const p = root.pressed
            if (root.cxStyle === CxButton.Style.Primary) {
                if (d) return Theme.accentSubtle
                if (p) return Theme.accentDark
                if (h) return Theme.accentLight
                return Theme.accent
            }
            if (root.cxStyle === CxButton.Style.Danger) {
                if (d) return Qt.darker(Theme.statusError, 1.5)
                if (p) return Qt.darker(Theme.statusError, 1.1)
                return Theme.statusError
            }
            if (root.cxStyle === CxButton.Style.Ghost) {
                if (p) return Theme.bgPressed
                if (h) return Theme.bgHover
                return "transparent"
            }
            // Secondary (default)
            if (d) return Theme.bgPanel
            if (p) return Theme.bgPressed
            if (h) return Theme.bgHover
            return Theme.bgElevated
        }
        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
        opacity: root.enabled ? 1.0 : 0.45

        border.color: {
            if (root.cxStyle === CxButton.Style.Primary) return "transparent"
            if (root.cxStyle === CxButton.Style.Danger) return "transparent"
            if (root.cxStyle === CxButton.Style.Ghost) return root.hovered ? Theme.borderDefault : "transparent"
            return Theme.borderStrong
        }
        border.width: 1
    }

    contentItem: Text {
        text: root.text
        color: {
            if (!root.enabled) return Theme.textDisabled
            if (root.cxStyle === CxButton.Style.Primary) return Theme.textOnAccent
            if (root.cxStyle === CxButton.Style.Danger) return Theme.textOnAccent
            return Theme.textPrimary
        }
        font.pixelSize: root.compact ? Theme.fontSizeSM : Theme.fontSizeMD
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
