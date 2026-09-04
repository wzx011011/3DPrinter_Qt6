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
        // R-P1.E: show the REAL active slice result path (read-only). The old
        // editable field carried a hardcoded "C:/Users/Output/print_job.gcode"
        // placeholder that was never used by the send flow.
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            Text { text: qsTr("G-code"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; Layout.preferredWidth: 60 }

            Rectangle {
                Layout.fillWidth: true
                height: 28; radius: 4
                color: Theme.bgInset
                border.color: Theme.borderInput

                Text {
                    anchors.fill: parent
                    anchors.leftMargin: 8; anchors.rightMargin: Theme.spacingMD
                    anchors.verticalCenter: parent.verticalCenter
                    verticalAlignment: TextInput.AlignVCenter
                    text: root.editorVm && root.editorVm.lastGcodePath
                          ? root.editorVm.lastGcodePath
                          : qsTr("（暂无切片结果，请先切片）")
                    color: root.editorVm && root.editorVm.lastGcodePath
                           ? Theme.chromeText : Theme.textDisabled
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                }
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
            // R-P1.E: the send flow needs BOTH a sliced G-code and the device
            // VM. Previously it read a nonexistent `lastGcodePath` and opened
            // SelectMachineDialog with an empty path + empty device list, which
            // still accepted() -- a fake-completed print send. The button is
            // now gated and disabled with an honest reason.
            Rectangle {
                width: 80; height: 30; radius: 4
                readonly property bool canPrint: root.editorVm !== null
                    && (root.editorVm.lastGcodePath || "") !== ""
                    && root.monitorVm !== null
                color: !canPrint ? Theme.bgPressed
                      : printHov.containsMouse ? Theme.accentDark : Theme.accentSubtle
                Text { anchors.centerIn: parent; text: qsTr("▶ 打印"); color: "white"; font.pixelSize: Theme.fontSizeSM; font.bold: true }
                MouseArea {
                    id: printHov; anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: parent.canPrint ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                    enabled: parent.canPrint
                    onClicked: {
                        selectMachineDialog.gcodePath = root.editorVm.lastGcodePath
                        selectMachineDialog.open()
                        root.close()
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: root.editorVm === null || (root.editorVm.lastGcodePath || "") === ""
            text: qsTr("请先对当前平板切片，再发送打印。")
            color: Theme.textDisabled
            font.pixelSize: Theme.fontSizeXS
            horizontalAlignment: Text.AlignHCenter
        }
    }

    // v2.5 DEV-05: SelectMachineDialog 实例（发送打印到真机）
    SelectMachineDialog {
        id: selectMachineDialog
        deviceVm: root.monitorVm
        gcodePath: ""
    }
}
