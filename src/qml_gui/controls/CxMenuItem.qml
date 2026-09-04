import QtQuick
import QtQuick.Controls
import ".."

MenuItem {
    id: root

    // Width floor from the qtquickcontrols2 Basic MenuItem template
    // (background implicitWidth 200). Without it a customized background
    // reports zero implicit width and the owning CxMenu can collapse to
    // zero width. Height is left content-driven to keep rows compact.
    background: Rectangle {
        implicitWidth: 200
        color: {
            if (!root.enabled) return "transparent"
            if (root.highlighted) return root.pressed ? Theme.bgPressed : Theme.bgHover
            return "transparent"
        }
        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
    }

    contentItem: Text {
        text: root.text
        color: root.enabled ? Theme.textPrimary : Theme.textDisabled
        font.pixelSize: Theme.fontSizeMD
        leftPadding: Theme.spacingLG
        verticalAlignment: Text.AlignVCenter
    }

    arrow: Canvas {
        x: parent.width - width - Theme.spacingMD
        y: (parent.height - height) / 2
        width: 8
        height: 8
        contextType: "2d"
        visible: root.subMenu
        onPaint: {
            if (!context) return
            context.reset()
            context.fillStyle = Theme.textMuted
            context.moveTo(0, 0)
            context.lineTo(width, height / 2)
            context.lineTo(0, height)
            context.fill()
        }
    }
}
