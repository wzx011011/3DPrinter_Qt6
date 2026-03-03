import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    height: 24
    color: "#0a0c10"
    border.color: "#242a33"
    border.width: 0

    // Publicly settable status strings
    property string statusText:  qsTr("就绪")
    property string sliceInfo:   ""
    property string coordText:   ""
    property int    objectCount: 0

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 16

        Text {
            text: root.statusText
            color: "#7a8898"
            font.pixelSize: 11
        }

        Rectangle { width: 1; height: 14; color: "#2e3444" }

        Text {
            text: root.objectCount > 0 ? (qsTr("对象: ") + root.objectCount) : ""
            color: "#7a8898"
            font.pixelSize: 11
            visible: root.objectCount > 0
        }

        Text {
            text: root.sliceInfo
            color: "#7a8898"
            font.pixelSize: 11
            visible: root.sliceInfo !== ""
        }

        Item { Layout.fillWidth: true }

        Text {
            text: root.coordText
            color: "#566070"
            font.pixelSize: 11
            visible: root.coordText !== ""
        }

        Text {
            text: Qt.formatDateTime(new Date(), "hh:mm")
            color: "#566070"
            font.pixelSize: 11
            // Update every minute
            Timer {
                interval: 60000
                running: true
                repeat: true
                triggeredOnStart: true
                onTriggered: parent.text = Qt.formatDateTime(new Date(), "hh:mm")
            }
        }
    }
}
