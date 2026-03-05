import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var previewVm

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        Label { text: qsTr("图例"); color: "#dfe6ef" }

        Repeater {
            model: root.previewVm.legendItems
            delegate: RowLayout {
                spacing: 6
                Rectangle {
                    width: 10
                    height: 10
                    radius: 2
                    color: modelData.color
                }
                Label {
                    text: modelData.label + " (" + modelData.count + ")"
                    color: "#cdd5e2"
                    font.pixelSize: 12
                }
            }
        }
    }
}
