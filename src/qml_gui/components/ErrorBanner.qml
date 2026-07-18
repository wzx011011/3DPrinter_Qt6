import QtQuick
import QtQuick.Layouts

// Warning-level (severity=1) banner — spans full width below title bar
// Supports notification queue with pending count badge
Rectangle {
    id: root

    Layout.fillWidth: true
    height:  visible ? 36 : 0
    visible: backend.lastErrorSeverity === 1 && backend.lastErrorMessage !== ""
    clip:    true
    color:   Theme.bgBase

    // Subtle left accent
    Rectangle { width: 3; height: parent.height; color: Theme.statusError }

    RowLayout {
        anchors.fill:        parent
        anchors.leftMargin:  12
        anchors.rightMargin: 8
        spacing: 8

        Text { text: "⚠"; color: Theme.statusError; font.pixelSize: 13 }
        Text {
            visible: backend.lastErrorTitle !== ""
            text: backend.lastErrorTitle
            color: Theme.statusError
            font.pixelSize: 10
            font.bold: true
        }

        Text {
            text:            backend.lastErrorMessage
            color:           Theme.statusWarning
            font.pixelSize:  12
            elide:           Text.ElideRight
            Layout.fillWidth: true
        }

        // Pending notification count
        Rectangle {
            visible: backend.pendingNotificationCount > 0
            width: 20; height: 20; radius: 10
            color: Theme.statusError

            Text {
                anchors.centerIn: parent
                text: backend.pendingNotificationCount > 9 ? "9+" : backend.pendingNotificationCount.toString()
                color: Theme.accentDark
                font.pixelSize: 9
                font.bold: true
            }
        }

        Rectangle {
            width: 48; height: 22; radius: 4
            color: closeHov.containsMouse ? Theme.bgErrorSubtle : "transparent"
            border.color: Theme.bgErrorSubtle; border.width: 1

            Text {
                anchors.centerIn: parent
                text:  qsTr("关闭")
                color: Theme.statusError
                font.pixelSize: 11
            }
            HoverHandler { id: closeHov }
            TapHandler   { onTapped: backend.dismissNotification() }
        }
    }
}
