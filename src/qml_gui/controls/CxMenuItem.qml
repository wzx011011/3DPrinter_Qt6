import QtQuick
import QtQuick.Controls
import ".."

MenuItem {
    id: root

    // Upstream dropdown row metrics (BBL Topbar file menu): compact ~28px
    // rows, 13px text, 12px leading inset, room on the right for the arrow.
    implicitHeight: 30
    leftPadding: 12
    rightPadding: 28

    // U08: checkable indicator. The Basic style puts its indicator at
    // x=leftPadding(6) while this control's contentItem uses a fixed
    // leftPadding, so a stock check mark would be drawn under the text.
    // Custom indicator + reserved check column; rendered only for checkable
    // items (upstream native check items, GUI_Factories.cpp
    // append_menu_check_item), so non-checkable menus keep their layout.
    indicator: Item {
        x: Theme.spacingSM
        y: (parent.height - height) / 2
        implicitWidth: 16
        implicitHeight: 16
        visible: root.checkable
        Text {
            anchors.centerIn: parent
            visible: root.checked
            text: "✓"
            color: Theme.accent
            font.pixelSize: 12
            font.bold: true
        }
    }

    background: Rectangle {
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
        font.pixelSize: Theme.fontSize13
        // Reserve the check column only when the item is checkable; plain
        // items keep the original leftPadding (zero change for existing
        // menus, U08 incremental requirement).
        leftPadding: root.checkable
                     ? root.indicator.x + root.indicator.width + root.spacing
                     : Theme.spacingLG
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
