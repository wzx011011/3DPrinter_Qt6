import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: root
    required property var monitorVm

    readonly property color pageBg: "#0d0f12"
    readonly property color panelBg: "#1a1e28"
    readonly property color elevatedBg: "#252b38"
    readonly property color borderCol: "#2a3040"
    readonly property color textMain: "#e8edf6"
    readonly property color textMuted: "#a0abbe"
    readonly property int gap: 8
    readonly property int outerMargin: 12

    Rectangle {
        anchors.fill: parent
        color: root.pageBg
    }

    Image {
        anchors.fill: parent
        visible: backend.visualCompareMode
        source: "qrc:/qml/assets/monitor_ref.png"
        sourceClipRect: Qt.rect(0, 40, 2560, 1360)
        fillMode: Image.Stretch
    }

    Image {
        anchors.fill: parent
        visible: !backend.visualCompareMode
        source: "qrc:/qml/assets/monitor_ref.png"
        fillMode: Image.PreserveAspectCrop
        opacity: 0.24
    }

    Rectangle {
        anchors.fill: parent
        color: "#0b1118cc"
        visible: !backend.visualCompareMode
    }

    ColumnLayout {
        visible: !backend.visualCompareMode
        anchors.fill: parent
        spacing: root.gap
        anchors.margins: root.outerMargin

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            radius: 14
            color: root.panelBg
            border.width: 1
            border.color: root.borderCol

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                Rectangle {
                    width: 84
                    height: 24
                    radius: 8
                    color: root.elevatedBg
                    border.width: 1
                    border.color: root.borderCol

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("管理分组")
                        color: root.textMain
                        font.pixelSize: 10
                    }
                }

                Rectangle {
                    width: 84
                    height: 24
                    radius: 8
                    color: root.elevatedBg
                    border.width: 1
                    border.color: root.borderCol

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("查看任务")
                        color: root.textMain
                        font.pixelSize: 10
                    }
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    width: 24
                    height: 24
                    radius: 8
                    color: root.elevatedBg
                    border.width: 1
                    border.color: root.borderCol

                    Text {
                        anchors.centerIn: parent
                        text: "[]"
                        color: root.textMuted
                        font.pixelSize: 10
                    }
                }

                Rectangle {
                    width: 24
                    height: 24
                    radius: 8
                    color: root.elevatedBg
                    border.width: 1
                    border.color: root.borderCol

                    Text {
                        anchors.centerIn: parent
                        text: "="
                        color: root.textMuted
                        font.pixelSize: 12
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            radius: 14
            color: root.panelBg
            border.width: 1
            border.color: root.borderCol

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12

                Text {
                    text: qsTr("New Group1")
                    color: root.textMain
                    font.pixelSize: 14
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Rectangle {
                    width: 88
                    height: 26
                    radius: 8
                    color: root.elevatedBg
                    border.width: 1
                    border.color: root.borderCol

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("+扫描添加")
                        color: root.textMain
                        font.pixelSize: 10
                    }
                }

                Rectangle {
                    width: 88
                    height: 26
                    radius: 8
                    color: root.elevatedBg
                    border.width: 1
                    border.color: root.borderCol

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("+手动添加")
                        color: root.textMain
                        font.pixelSize: 10
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            radius: 12
            color: root.panelBg
            border.width: 1
            border.color: root.borderCol

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                spacing: 24

                Text {
                    text: qsTr("设备名称/卡片")
                    color: root.textMuted
                    font.pixelSize: 11
                }

                Text {
                    text: qsTr("设备状态")
                    color: root.textMuted
                    font.pixelSize: 11
                }

                Item { Layout.fillWidth: true }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 18
            color: root.panelBg
            border.width: 1
            border.color: root.borderCol

            Column {
                anchors.centerIn: parent
                spacing: 10

                Text {
                    text: qsTr("设备")
                    color: root.textMuted
                    font.pixelSize: 28
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    width: 120
                }

                Text {
                    text: qsTr("No Data")
                    color: root.textMuted
                    horizontalAlignment: Text.AlignHCenter
                    width: 120
                }
            }
        }
    }
}
