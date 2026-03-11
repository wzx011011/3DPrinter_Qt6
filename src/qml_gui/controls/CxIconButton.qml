import QtQuick
import QtQuick.Controls
import ".."

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

    background: Rectangle {
        radius: Math.round(root.height / 4)
        color: {
            if (root.cxStyle === CxIconButton.Style.ChromeDanger)
                return root.down ? Theme.chromeDangerPressed : (root.hovered ? Theme.chromeDangerHover : "transparent")
            if (root.cxStyle === CxIconButton.Style.Chrome)
                return root.down ? Theme.chromePressed : (root.hovered ? Theme.chromeHover : "transparent")
            if (root.cxStyle === CxIconButton.Style.Ghost)
                return root.down ? Theme.bgPressed : (root.hovered ? Theme.bgHover : "transparent")
            if (root.selected)
                return root.down ? Theme.accentSubtle : (root.hovered ? Theme.accentSubtle : Theme.accentSubtle)
            return root.down ? Theme.bgPressed : (root.hovered ? Theme.bgHover : Theme.bgPanel)
        }
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
        opacity: root.enabled ? 1.0 : 0.45
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
        }
    }

    ToolTip.visible: root.hovered && root.toolTipText.length > 0
    ToolTip.text: root.toolTipText
    ToolTip.delay: 400
}