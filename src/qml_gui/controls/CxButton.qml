import QtQuick
import QtQuick.Controls
import ".."

Button {
    id: root

    property int cxStyle: CxButton.Style.Primary
    property bool compact: false
    // Phase 161 (DS-02): ToolTip support for parity with CxIconButton.
    property string toolTipText: ""

    enum Style { Primary, Secondary, Danger, Ghost }

    implicitHeight: compact ? Theme.controlHeightSM : Theme.controlHeightMD
    implicitWidth: Math.max(64, contentItem.implicitWidth + leftPadding + rightPadding)
    leftPadding: Theme.spacingLG
    rightPadding: Theme.spacingLG

    // Phase 161 (DS-02): press-scale for parity with CxIconButton's 0.92.
    scale: root.pressed ? 0.96 : 1.0
    Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

    background: Rectangle {
        // ctl-6: upstream corner radius is per style tier (Button.cpp:191-221,
        // FromDIP): Choice/Parameter/Icon/Expanded = 4, Compact = 8,
        // Window = 12. The Qt6 style enum has no Window-tier equivalent;
        // `compact` maps to the upstream Compact tier, all others use the
        // standard 4px tier.
        radius: root.compact ? 8 : 4
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
                // Phase 161 (DS-02): replaced the previous runtime color
                // manipulation with explicit Phase 160 tokens (statusErrorDark /
                // statusErrorPressed). No runtime darker/lighter calls — those
                // bypass the Theme token system.
                if (d) return Theme.statusErrorDark
                if (p) return Theme.statusErrorPressed
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
            // Phase 161 (DS-02): focus border for accessibility (keyboard nav).
            if (root.activeFocus) return Theme.borderFocus
            if (root.cxStyle === CxButton.Style.Primary) return "transparent"
            if (root.cxStyle === CxButton.Style.Danger) return "transparent"
            if (root.cxStyle === CxButton.Style.Ghost) return root.hovered ? Theme.borderDefault : "transparent"
            return Theme.borderStrong
        }
        border.width: root.activeFocus ? 2 : 1
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }

    contentItem: Text {
        text: root.text
        color: {
            if (!root.enabled) return Theme.textDisabled
            if (root.cxStyle === CxButton.Style.Primary) return Theme.textOnAccent
            if (root.cxStyle === CxButton.Style.Danger) return Theme.textOnAccent
            return Theme.textPrimary
        }
        // ctl-5: upstream buttons default to Body_14 (Button.cpp:54, and the
        // Choice/Parameter/Expanded tiers at :204/210/222); only the Compact
        // tier steps down (Body_10). Align to the 14px tier, compact to 12px.
        font.pixelSize: root.compact ? Theme.fontSizeMD : Theme.fontSizeLG
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    // Phase 161 (DS-02): ToolTip mirror CxIconButton's contract.
    ToolTip.visible: root.hovered && root.toolTipText.length > 0
    ToolTip.text: root.toolTipText
    ToolTip.delay: 400
}
