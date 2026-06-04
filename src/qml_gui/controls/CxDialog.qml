import QtQuick
import QtQuick.Controls
import ".."

Dialog {
    id: root

    property string dialogTitle: ""
    property string titleIcon: ""
    property bool showCloseButton: true
    property int contentSpacing: Theme.spacingLG

    // Suppress default Dialog.title to avoid double header
    title: ""

    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: Theme.bgElevated
        border.color: Theme.borderInput
        border.width: 1
        radius: Theme.radiusLG
    }

    header: Rectangle {
        height: 44
        color: Theme.bgSurface
        radius: Theme.radiusLG

        // Bottom corners: square (clip the rounded top)
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: parent.radius
            color: parent.color
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: Theme.spacingXL
            text: (root.titleIcon ? root.titleIcon + " " : "") + root.dialogTitle
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeLG
            font.bold: true
        }

        // Close button
        Rectangle {
            visible: root.showCloseButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: Theme.spacingMD
            width: 28
            height: 28
            radius: Theme.radiusSM
            color: closeMouse.containsMouse ? Theme.chromeDangerHover : "transparent"
            opacity: closeMouse.containsMouse ? 0.3 : 1.0

            Text {
                anchors.centerIn: parent
                text: "✕"
                color: Theme.textMuted
                font.pixelSize: Theme.fontSizeMD
            }

            MouseArea {
                id: closeMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.reject()
            }
        }
    }

    footer: null
}
