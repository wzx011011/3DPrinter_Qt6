import QtQuick
import QtQuick.Controls
import ".."

// Enhanced icon button with animation feedback (aligns with upstream GLToolbar button states)
// Supports: Normal, Hover, Pressed, Disabled, Selected/Active states
// Animations: color transitions, press scale, active glow, smooth opacity
ToolButton {
    id: root

    enum Style { Surface, Ghost, Chrome, ChromeDanger }

    property int cxStyle: CxIconButton.Style.Surface
    property url iconSource: ""
    property string toolTipText: ""
    property bool selected: false
    property int buttonSize: Theme.iconButtonSizeMD
    property int iconSize: 16

    implicitWidth: buttonSize
    implicitHeight: buttonSize
    flat: true
    hoverEnabled: true

    // Press scale animation (对齐上游 GLToolbar press feedback)
    scale: _pressScale
    property real _pressScale: 1.0
    Behavior on _pressScale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

    onPressedChanged: _pressScale = pressed ? 0.92 : 1.0

    background: Rectangle {
        id: bgRect
        radius: Math.round(root.height / 4)
        scale: root.down ? 0.96 : 1.0
        Behavior on scale { NumberAnimation { duration: 100; easing.type: Easing.OutCubic } }

        color: {
            if (!root.enabled)
                // U09 (G3): a disabled Ghost button must not render as a
                // filled box -- only its icon dims (the old bgPanel fill was
                // the inverted weighting: disabled looked heavier than
                // enabled).
                return root.cxStyle === CxIconButton.Style.Ghost ? "transparent" : Theme.bgPanel
            // U09 (G5): the selected check must precede the style branches --
            // the Ghost return below made the selected legs unreachable.
            if (root.selected)
                // Solid accent capsule (ref solid green #17cc5f family, R1
                // accent #18c75e); pressed keeps the DS-02
                // Theme.accentSubtlePressed token.
                return root.down ? Theme.accentSubtlePressed : Theme.accent
            if (root.cxStyle === CxIconButton.Style.ChromeDanger)
                return root.down ? Theme.chromeDangerPressed : (root.hovered ? Theme.chromeDangerHover : "transparent")
            if (root.cxStyle === CxIconButton.Style.Chrome)
                return root.down ? Theme.chromePressed : (root.hovered ? Theme.chromeHover : "transparent")
            if (root.cxStyle === CxIconButton.Style.Ghost)
                return root.down ? Theme.bgPressed : (root.hovered ? Theme.bgHover : "transparent")
            return root.down ? Theme.bgPressed : (root.hovered ? Theme.bgHover : Theme.bgPanel)
        }
        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }

        border.width: root.cxStyle === CxIconButton.Style.ChromeDanger ? 0 : 1
        border.color: {
            if (!root.enabled)
                // U09 (G3): disabled Ghost keeps no box at all.
                return root.cxStyle === CxIconButton.Style.Ghost ? "transparent" : Theme.borderSubtle
            if (root.selected)
                // U09 (G5): selected border follows the accent capsule.
                return Theme.accent
            if (root.cxStyle === CxIconButton.Style.Chrome)
                return root.hovered ? Theme.chromeBorder : "transparent"
            if (root.cxStyle === CxIconButton.Style.Ghost)
                // U09 (G3): enabled Ghost carries a persistent 1px rounded
                // box (ref ≈#464749; borderSubtle is the nearest border
                // token, Δ ≈ +11/channel -- converges with the R1 border
                // batch), strengthening to borderDefault on hover.
                return root.hovered ? Theme.borderDefault : Theme.borderSubtle
            return Theme.borderSubtle
        }
        Behavior on border.color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }

        opacity: root.enabled ? 1.0 : 0.45
        Behavior on opacity { NumberAnimation { duration: 150 } }

        // U09 (G5): the former low-opacity accent glow overlay is gone --
        // the selected state is now the solid accent capsule itself.
    }

    contentItem: Item {
        Image {
            anchors.centerIn: parent
            width: root.iconSize
            height: root.iconSize
            source: root.iconSource
            fillMode: Image.PreserveAspectFit
            smooth: true
            opacity: root.enabled ? 1.0 : 0.65
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }
    }

    ToolTip.visible: root.hovered && root.toolTipText.length > 0
    ToolTip.text: root.toolTipText
    ToolTip.delay: 400
}
