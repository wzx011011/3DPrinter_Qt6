import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl
import ".."

ComboBox {
    id: root

    implicitHeight: Theme.controlHeightSM
    font.pixelSize: Theme.fontSizeMD

    background: Rectangle {
        radius: Theme.radiusSM
        color: {
            if (!root.enabled) return Theme.bgPanel
            if (root.pressed) return Theme.bgPressed
            if (root.hovered) return Theme.bgHover
            return Theme.bgElevated
        }
        border.color: root.activeFocus ? Theme.borderFocus : Theme.borderStrong
        border.width: 1
        Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
        opacity: root.enabled ? 1.0 : 0.45
    }

    contentItem: Text {
        leftPadding: Theme.spacingMD
        rightPadding: root.indicator.width + Theme.spacingXS
        text: root.displayText
        color: root.enabled ? Theme.textPrimary : Theme.textDisabled
        font: root.font
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    indicator: Text {
        x: root.width - width - Theme.spacingMD
        y: (root.height - height) / 2
        text: "▾"
        color: Theme.textMuted
        font.pixelSize: Theme.fontSizeXS
    }

    popup: Popup {
        y: root.height + 2
        width: root.width
        implicitHeight: contentItem.implicitHeight
        padding: 0

        background: Rectangle {
            color: Theme.bgElevated
            border.color: Theme.borderDefault
            border.width: 1
            radius: Theme.radiusSM
        }

        contentItem: ListView {
            clip: true
            implicitHeight: Math.min(contentHeight, 240)
            model: root.popup.visible ? root.delegateModel : null
            ScrollIndicator.vertical: ScrollIndicator {}
        }
    }

    delegate: ItemDelegate {
        width: root.width
        height: Theme.controlHeightSM - 2
        highlighted: root.highlightedIndex === index
        background: Rectangle {
            color: highlighted ? Theme.accentSubtle : "transparent"
        }
        contentItem: Text {
            leftPadding: Theme.spacingLG
            text: root.textRole ? (Array.isArray(root.model) ? modelData[root.textRole] : model[root.textRole]) : modelData
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMD
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }
}
