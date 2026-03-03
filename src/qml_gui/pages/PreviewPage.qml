import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var previewVm

    Rectangle { anchors.fill: parent; color: "#30343b" }

    Image {
        anchors.fill: parent
        visible: backend.visualCompareMode
        source: "qrc:/qml/assets/preview_ref.png"
        sourceClipRect: Qt.rect(0, 40, 2560, 1360)
        fillMode: Image.Stretch
    }

    Image {
        anchors.fill: parent
        visible: !backend.visualCompareMode
        source: "qrc:/qml/assets/preview_ref.png"
        fillMode: Image.PreserveAspectCrop
        opacity: 0.92
    }

    ColumnLayout {
        visible: !backend.visualCompareMode
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: "#2a2f37"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                spacing: 6
                Repeater {
                    model: ["⛶","◻","◇","⌗","⛭","⊞","⊟","A"]
                    delegate: Rectangle {
                        width: 22; height: 22; radius: 2
                        color: "#2f343d"; border.color: "#434a57"
                        Text { anchors.centerIn: parent; text: modelData; color: "#8e98a8"; font.pixelSize: 11 }
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 232
                Layout.fillHeight: true
                color: "#2a2f36"
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#353941"

                Rectangle {
                    width: parent.width * 0.56
                    height: parent.height * 0.72
                    anchors.centerIn: parent
                    radius: 6
                    color: "#4a4d55"
                    border.color: "#666b76"
                    Text {
                        anchors.top: parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.topMargin: 12
                        text: "Creality Smooth PEI Plate"
                        color: "#e9edf3"
                        font.bold: true
                    }
                }

                Text {
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 40
                    text: root.previewVm.slicing ? (root.previewVm.progress + "%") : "01"
                    color: "#1fd86d"
                    font.pixelSize: 24
                    font.bold: true
                }

                Row {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: 6
                    anchors.bottomMargin: 6
                    spacing: 4
                    Repeater {
                        model: ["切片角色", "发送打印"]
                        delegate: Rectangle {
                            width: 76
                            height: 22
                            color: "#575c64"
                            radius: 2
                            Text { anchors.centerIn: parent; text: modelData; color: "#eef3fa"; font.pixelSize: 10 }
                        }
                    }
                }
            }

            Rectangle {
                Layout.preferredWidth: 302
                Layout.fillHeight: true
                color: "#4a4e56"

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 54
                        color: "#66db44"
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.margins: 8
                        spacing: 8

                        Label { text: "切片进度"; color: "#dfe6ef" }
                        ProgressBar {
                            Layout.fillWidth: true
                            from: 0
                            to: 100
                            value: root.previewVm.progress
                        }
                        Label { text: "预计: " + root.previewVm.estimatedTime; color: "#dfe6ef" }
                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }
    }
}
