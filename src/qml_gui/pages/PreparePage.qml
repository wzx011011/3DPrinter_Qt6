import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../panels"
import "../dialogs"
import CrealityGL 1.0

Item {
    id: root
    required property var editorVm
    required property var configVm

    PrintDialog {
        id: printDlg
        editorVm: root.editorVm
    }

    Rectangle {
        anchors.fill: parent
        color: "#30343b"
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: "#2a2f37"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6
                Repeater {
                    model: ["⛶","◻","◇","⌗","⛭","⊞","⊟","A"]
                    delegate: Rectangle {
                        width: 22
                        height: 22
                        radius: 2
                        color: "#2f343d"
                        border.color: "#434a57"
                        Text { anchors.centerIn: parent; text: modelData; color: "#8e98a8"; font.pixelSize: 11 }
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            anchors.margins: 0
            spacing: 0

            Rectangle {
                Layout.preferredWidth: 232
                Layout.fillHeight: true
                color: "#2a2f36"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

                    Rectangle {
                        Layout.preferredWidth: 56
                        Layout.preferredHeight: 56
                        color: "#3f2b2b"
                        border.color: "#00d36c"
                        Text { anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 4; text: "1"; color: "#e8edf4" }
                    }

                    Label { text: qsTr("打印机"); color: "#dfe6ef"; font.pixelSize: 12 }
                    ComboBox {
                        Layout.fillWidth: true
                        model: ["Creality K1C 0.4 nozzle", "K1 Max 0.4 nozzle"]
                    }
                    ComboBox {
                        Layout.fillWidth: true
                        model: [qsTr("光面PEI板/涂层板"), qsTr("普通PEI板")]
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Rectangle { width: 28; height: 16; radius: 2; color: "#28be63"; Text { anchors.centerIn: parent; text: qsTr("全部"); color: "white"; font.pixelSize: 10 } }
                        Rectangle { width: 28; height: 16; radius: 2; color: "#3d434f"; Text { anchors.centerIn: parent; text: qsTr("对象"); color: "#c8d0dc"; font.pixelSize: 10 } }
                        Item { Layout.fillWidth: true }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // E6 — real OpenGL viewport replaces the placeholder Rectangle
            GLViewport {
                id: viewport3d
                Layout.fillWidth: true
                Layout.fillHeight: true
                canvasType: GLViewport.CanvasView3D

                Column {
                    spacing: 8
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    Repeater {
                        model: ["＋","☰","AUTO","🔒","⚙"]
                        delegate: Rectangle {
                            width: 36
                            height: 28
                            radius: 14
                            color: "#5b616b"
                            Text { anchors.centerIn: parent; text: modelData; color: "#dde4ed"; font.pixelSize: 11 }
                        }
                    }
                }

                Rectangle {
                    width: 34
                    height: 64
                    anchors.right: parent.right
                    anchors.rightMargin: 8
                    anchors.top: parent.top
                    anchors.topMargin: 16
                    color: "#8b8f96"
                    radius: 2
                    Text { anchors.centerIn: parent; text: qsTr("上\n前"); color: "#eef3fa"; horizontalAlignment: Text.AlignHCenter }
                }

                Row {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin: 6
                    anchors.bottomMargin: 6
                    spacing: 4
                    // 切片角色
                    Rectangle {
                        width: 76; height: 22; color: "#575c64"; radius: 2
                        Text { anchors.centerIn: parent; text: qsTr("切片角色"); color: "#eef3fa"; font.pixelSize: 10 }
                    }
                    // 发送打印 — 打开 PrintDialog
                    Rectangle {
                        width: 76; height: 22
                        color: sendHov.containsMouse ? "#19a84e" : "#157a39"
                        radius: 2
                        Text { anchors.centerIn: parent; text: qsTr("发送打印"); color: "white"; font.pixelSize: 10 }
                        MouseArea {
                            id: sendHov
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: printDlg.open()
                        }
                    }
                }
            }

            Sidebar {
                id: sidebar
                Layout.fillHeight: true
                editorVm: root.editorVm
                configVm: root.configVm
            }
        }
    }
}
