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
    color:   "#251a08"

    // Subtle left accent
    Rectangle { width: 3; height: parent.height; color: "#c87840" }

    RowLayout {
        anchors.fill:        parent
        anchors.leftMargin:  12
        anchors.rightMargin: 8
        spacing: 8

        Text { text: "⚠"; color: "#c87840"; font.pixelSize: 13 }
        Text {
            visible: backend.lastErrorTitle !== ""
            text: backend.lastErrorTitle
            color: "#c87840"
            font.pixelSize: 10
            font.bold: true
        }

        Text {
            text:            backend.lastErrorMessage
            color:           "#e8b870"
            font.pixelSize:  12
            elide:           Text.ElideRight
            Layout.fillWidth: true
        }

        // Pending notification count
        Rectangle {
            visible: backend.pendingNotificationCount > 0
            width: 20; height: 20; radius: 10
            color: "#c87840"

            Text {
                anchors.centerIn: parent
                text: backend.pendingNotificationCount > 9 ? "9+" : backend.pendingNotificationCount.toString()
                color: "#fff"
                font.pixelSize: 9
                font.bold: true
            }
        }

        Rectangle {
            width: 48; height: 22; radius: 4
            color: closeHov.containsMouse ? "#3a2412" : "transparent"
            border.color: "#5a3418"; border.width: 1

            Text {
                anchors.centerIn: parent
                text:  qsTr("关闭")
                color: "#c0824a"
                font.pixelSize: 11
            }
            HoverHandler { id: closeHov }
            TapHandler   { onTapped: backend.dismissNotification() }
        }
    }
}
