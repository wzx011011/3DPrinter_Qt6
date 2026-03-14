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
                return Theme.bgPanel
            if (root.cxStyle === CxIconButton.Style.ChromeDanger)
                return root.down ? Theme.chromeDangerPressed : (root.hovered ? Theme.chromeDangerHover : "transparent")
            if (root.cxStyle === CxIconButton.Style.Chrome)
                return root.down ? Theme.chromePressed : (root.hovered ? Theme.chromeHover : "transparent")
            if (root.cxStyle === CxIconButton.Style.Ghost)
                return root.down ? Theme.bgPressed : (root.hovered ? Theme.bgHover : "transparent")
            if (root.selected)
                return root.down ? Qt.darker(Theme.accentSubtle, 1.1) : (root.hovered ? Theme.accentSubtle : Theme.accentSubtle)
            return root.down ? Theme.bgPressed : (root.hovered ? Theme.bgHover : Theme.bgPanel)
        }
        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }

        border.width: root.cxStyle === CxIconButton.Style.ChromeDanger ? 0 : 1
        border.color: {
            if (root.cxStyle === CxIconButton.Style.Chrome)
                return root.hovered ? "#2f3d54" : "transparent"
            if (root.cxStyle === CxIconButton.Style.Ghost)
                return root.hovered ? Theme.borderDefault : "transparent"
            if (root.selected)
                return Theme.accent
            return Theme.borderSubtle
        }
        Behavior on border.color { ColorAnimation { duration: 150; easing.type: Easing.OutCubic } }

        opacity: root.enabled ? 1.0 : 0.45
        Behavior on opacity { NumberAnimation { duration: 150 } }

        // Active/Selected state glow (对齐上游 GLToolbar highlight)
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            visible: root.selected && root.enabled
            color: Theme.accent
            opacity: root.hovered ? 0.08 : 0.04
            Behavior on opacity { NumberAnimation { duration: 200 } }
        }
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
