import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

ComboBox {
    id: root

    implicitHeight: 28
    font.pixelSize: 12

    background: Rectangle {
        radius: 4
        color: root.pressed ? "#3a4050" : root.hovered ? "#353c4a" : "#2d3340"
        border.color: root.activeFocus ? "#18c75e" : "#454d5e"
        border.width: 1
    }

    contentItem: Text {
        leftPadding: 8
        rightPadding: root.indicator.width + 4
        text: root.displayText
        color: "#d8e0ec"
        font: root.font
        elide: Text.ElideRight
        verticalAlignment: Text.AlignVCenter
    }

    indicator: Text {
        x: root.width - width - 8
        y: (root.height - height) / 2
        text: "▾"
        color: "#8a96a8"
        font.pixelSize: 10
    }

    popup: Popup {
        y: root.height + 2
        width: root.width
        implicitHeight: contentItem.implicitHeight
        padding: 0

        background: Rectangle {
            color: "#252a34"
            border.color: "#454d5e"
            border.width: 1
            radius: 4
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
        height: 26
        highlighted: root.highlightedIndex === index
        background: Rectangle {
            color: highlighted ? "#1c6e42" : "transparent"
        }
        contentItem: Text {
            leftPadding: 10
            text: root.textRole ? (Array.isArray(root.model) ? modelData[root.textRole] : model[root.textRole]) : modelData
            color: "#d8e0ec"
            font.pixelSize: 12
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }
}
