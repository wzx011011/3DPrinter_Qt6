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
    // U01: upstream modal dialogs close via Esc / ✕ / their buttons, not by
    // clicking outside (CenterOnParent family). Instances that need different
    // behavior override closePolicy explicitly.
    closePolicy: Popup.CloseOnEscape
    // U01: upstream centers dialogs on the parent overlay (CenterOnParent,
    // ObjColorDialog.cpp:175) -- fixes instances opening stuck at (0,0).
    anchors.centerIn: Overlay.overlay

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

        // U01: 1px full-width separator under the header -- upstream dialog
        // chrome uses a 1px line between title area and body
        // (KBShortcutsDialog.cpp:33-36 / RecenterDialog.cpp:28-29,
        // wxSize(-1, 1)).
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.separator
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
