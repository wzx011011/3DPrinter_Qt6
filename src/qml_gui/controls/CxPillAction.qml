import QtQuick
import QtQuick.Controls
import ".."

Button {
    id: root

    property url iconSource: ""
    property bool primary: false

    implicitHeight: Theme.pillHeight
    implicitWidth: Math.max(96, pillRow.implicitWidth + leftPadding + rightPadding)
    leftPadding: 14
    rightPadding: 14
    topPadding: 0
    bottomPadding: 0
    hoverEnabled: true

    background: Rectangle {
        radius: Math.round(root.height / 2)
        color: root.primary
            ? (root.pressed ? Theme.accentDark : (root.hovered ? Theme.accentLight : Theme.accent))
            : (root.pressed ? Theme.bgPressed : (root.hovered ? Theme.bgHover : Theme.bgPanel))
        border.width: 1
        border.color: root.primary ? Theme.accentDark : Theme.borderSubtle
    }

    contentItem: Item {
        implicitWidth: pillRow.implicitWidth
        implicitHeight: pillRow.implicitHeight

        Row {
            id: pillRow
            anchors.centerIn: parent
            spacing: 6

            Image {
                visible: root.iconSource !== ""
                width: 14
                height: 14
                source: root.iconSource
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            Text {
                text: root.text
                color: root.primary ? Theme.textOnAccent : Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                font.bold: root.primary
            }
        }
    }
}