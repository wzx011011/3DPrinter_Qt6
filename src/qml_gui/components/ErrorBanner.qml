import QtQuick
import QtQuick.Layouts

// Warning-level (severity=1) banner — spans full width below title bar
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
            text:            backend.lastErrorMessage
            color:           "#e8b870"
            font.pixelSize:  12
            elide:           Text.ElideRight
            Layout.fillWidth: true
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
            TapHandler   { onTapped: backend.clearError() }
        }
    }
}
