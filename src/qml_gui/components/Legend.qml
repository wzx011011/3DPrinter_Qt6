import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: root
    required property var previewVm

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingSM

        Label { text: qsTr("图例"); color: Theme.textPrimary; font.bold: true; font.pixelSize: Theme.fontSizeLG }

        Rectangle {
            Layout.fillWidth: true
            radius: 12
            color: Theme.bgElevated
            border.width: 1
            border.color: Theme.borderSubtle

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                Repeater {
                    model: root.previewVm.legendItems
                    delegate: RowLayout {
                        spacing: 8
                        Rectangle {
                            width: 10
                            height: 10
                            radius: 3
                            color: modelData.color
                        }
                        Label {
                            text: modelData.label + " (" + modelData.count + ")"
                            color: Theme.textPrimary
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }
}
