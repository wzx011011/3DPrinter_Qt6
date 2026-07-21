import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
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
        spacing: Theme.spacingLG
        // Project info row
        Rectangle {
            Layout.fillWidth: true
            height: 44
            radius: 5
            color: Theme.bgSurface
            border.color: Theme.borderInput

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spacingLG
                anchors.rightMargin: Theme.spacingLG
                spacing: Theme.spacingMD
                Text { text: "📄"; font.pixelSize: Theme.fontSizeXL }
                ColumnLayout {
                    spacing: Theme.spacingXS
                    Text {
                        text: root.editorVm ? root.editorVm.projectName : "—"
                        color: Theme.textPrimary; font.pixelSize: Theme.fontSizeMD; font.bold: true
                    }
                    Text {
                        text: root.editorVm
                              ? root.editorVm.objectCount + qsTr(" 个对象  ·  层高 0.20 mm")
                              : "—"
                        color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }

        // File output path
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            Text { text: qsTr("输出路径"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; Layout.preferredWidth: 60 }

            Rectangle {
                Layout.fillWidth: true
                height: 28; radius: 4
                color: Theme.bgInset
                border.color: pathField.activeFocus ? Theme.accent : Theme.borderInput

                TextInput {
                    id: pathField
                    anchors.fill: parent
                    anchors.leftMargin: 8; anchors.rightMargin: Theme.spacingMD
                    anchors.verticalCenter: parent.verticalCenter
                    verticalAlignment: TextInput.AlignVCenter
                    text: "C:/Users/Output/print_job.gcode"
                    color: Theme.chromeText
                    font.pixelSize: Theme.fontSizeSM
                    selectByMouse: true
                }
            }

            Rectangle {
                width: 26; height: 28; radius: 4
                color: browseHov.containsMouse ? Theme.borderInput : Theme.bgCard
                Text { anchors.centerIn: parent; text: "📂"; font.pixelSize: Theme.fontSizeMD }
                MouseArea {
                    id: browseHov; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                }
            }
        }

        // Quick options
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingXL
            CheckBox {
                text: qsTr("切片后自动发送")
                checked: true
                contentItem: Text { text: parent.text; color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; leftPadding: parent.indicator.width + 6 }
            }
            CheckBox {
                text: qsTr("完成后通知")
                checked: false
                contentItem: Text { text: parent.text; color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; leftPadding: parent.indicator.width + 6 }
            }
        }

        // Divider
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.bgCard }

        // Button row
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            Item { Layout.fillWidth: true }

            // Export G-code
            Rectangle {
                width: 110; height: 30; radius: 4
                color: exportHov.containsMouse ? Theme.borderInput : Theme.chromePressed
                border.color: Theme.bgPressed
                Text { anchors.centerIn: parent; text: qsTr("导出 G-code"); color: Theme.chromeTextMuted; font.pixelSize: Theme.fontSizeSM }
                MouseArea {
                    id: exportHov; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: { root.close() }
                }
            }

            // Cancel
            Rectangle {
                width: 60; height: 30; radius: 4
                color: cancelHov.containsMouse ? Theme.chromePressed : Theme.bgCard
                Text { anchors.centerIn: parent; text: qsTr("取消"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM }
                MouseArea {
                    id: cancelHov; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }

            // Print
            Rectangle {
                width: 80; height: 30; radius: 4
                color: printHov.containsMouse ? Theme.accentDark : Theme.accentSubtle
                Text { anchors.centerIn: parent; text: qsTr("▶ 打印"); color: "white"; font.pixelSize: Theme.fontSizeSM; font.bold: true }
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
