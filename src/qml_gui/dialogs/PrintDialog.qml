import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../controls"

// D2 -- PrintDialog: send print / export G-code
// Usage: PrintDialog { id: printDlg; editorVm: ... }
// Trigger: printDlg.open()
CxDialog {
    id: root
    required property var editorVm
    property var monitorVm: null  // v2.5 DEV-05: SelectMachineDialog 需要

    dialogTitle: qsTr("发送打印")
    titleIcon: "🖨"

    anchors.centerIn: parent

    width:  480
    height: contentCol.implicitHeight + 80

    // -- Content area --
    contentItem: ColumnLayout {
        id: contentCol
        width: root.width - 32
        spacing: 14

        // Project info row
        Rectangle {
            Layout.fillWidth: true
            height: 44
            radius: 5
            color: "#111620"
            border.color: "#2e3848"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10

                Text { text: "📄"; font.pixelSize: 16 }
                ColumnLayout {
                    spacing: 2
                    Text {
                        text: root.editorVm ? root.editorVm.projectName : "—"
                        color: "#dde4ef"; font.pixelSize: 12; font.bold: true
                    }
                    Text {
                        text: root.editorVm
                              ? root.editorVm.objectCount + qsTr(" 个对象  ·  层高 0.20 mm")
                              : "—"
                        color: "#566070"; font.pixelSize: 10
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }

        // File output path
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text { text: qsTr("输出路径"); color: "#9daaba"; font.pixelSize: 11; Layout.preferredWidth: 60 }

            Rectangle {
                Layout.fillWidth: true
                height: 28; radius: 4
                color: "#0f1318"
                border.color: pathField.activeFocus ? "#22c564" : "#2e3848"

                TextInput {
                    id: pathField
                    anchors.fill: parent
                    anchors.leftMargin: 8; anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    verticalAlignment: TextInput.AlignVCenter
                    text: "C:/Users/Output/print_job.gcode"
                    color: "#c8d4e0"
                    font.pixelSize: 11
                    selectByMouse: true
                }
            }

            Rectangle {
                width: 26; height: 28; radius: 4
                color: browseHov.containsMouse ? "#2e3848" : "#1e2535"
                Text { anchors.centerIn: parent; text: "📂"; font.pixelSize: 12 }
                MouseArea {
                    id: browseHov; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                }
            }
        }

        // Quick options
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            CheckBox {
                text: qsTr("切片后自动发送")
                checked: true
                contentItem: Text { text: parent.text; color: "#9daaba"; font.pixelSize: 11; leftPadding: parent.indicator.width + 6 }
            }
            CheckBox {
                text: qsTr("完成后通知")
                checked: false
                contentItem: Text { text: parent.text; color: "#9daaba"; font.pixelSize: 11; leftPadding: parent.indicator.width + 6 }
            }
        }

        // Divider
        Rectangle { Layout.fillWidth: true; height: 1; color: "#1e2535" }

        // Button row
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            // Export G-code
            Rectangle {
                width: 110; height: 30; radius: 4
                color: exportHov.containsMouse ? "#2e3848" : "#1e2840"
                border.color: "#2e4060"
                Text { anchors.centerIn: parent; text: qsTr("导出 G-code"); color: "#8eaad0"; font.pixelSize: 11 }
                MouseArea {
                    id: exportHov; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: { root.close() }
                }
            }

            // Cancel
            Rectangle {
                width: 60; height: 30; radius: 4
                color: cancelHov.containsMouse ? "#2e2e2e" : "#232830"
                Text { anchors.centerIn: parent; text: qsTr("取消"); color: "#9daaba"; font.pixelSize: 11 }
                MouseArea {
                    id: cancelHov; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }

            // Print
            Rectangle {
                width: 80; height: 30; radius: 4
                color: printHov.containsMouse ? "#19a84e" : "#157a39"
                Text { anchors.centerIn: parent; text: qsTr("▶ 打印"); color: "white"; font.pixelSize: 11; font.bold: true }
                MouseArea {
                    id: printHov; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        // v2.5 DEV-05: 切片后弹 SelectMachine 发送（对齐上游 SelectMachinePop）
                        if (root.editorVm) root.editorVm.requestSlice()
                        // 弹 SelectMachineDialog 选择设备发送（gcodePath 待 SliceService 完成后填充）
                        selectMachineDialog.gcodePath = root.editorVm ? (root.editorVm.lastGcodePath || "") : ""
                        selectMachineDialog.open()
                        root.close()
                    }
                }
            }
        }
    }

    // v2.5 DEV-05: SelectMachineDialog 实例（发送打印到真机）
    SelectMachineDialog {
        id: selectMachineDialog
        deviceVm: root.monitorVm
        gcodePath: ""
    }
}
