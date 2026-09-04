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
    // G-03: true while an on-demand slice requested from this dialog is in
    // flight; printSliceReady() then continues into device selection.
    property bool slicingForPrint: false

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
            // R-P1.E + G-03: the send flow needs the device VM and a G-code
            // target. With no slice result yet, clicking print starts an
            // on-demand slice and the flow continues automatically when
            // printSliceReady fires (upstream Plater.cpp:7172 slices from the
            // print flow; SelectMachine.cpp:2049 gates Send on readiness).
            Rectangle {
                width: 80; height: 30; radius: 4
                readonly property bool hasResult: root.editorVm !== null
                    && (root.editorVm.lastGcodePath || "") !== ""
                readonly property bool canPrint: root.editorVm !== null
                    && root.monitorVm !== null
                    && !root.slicingForPrint
                    && (hasResult || !root.editorVm.isSlicing())
                color: !canPrint ? Theme.bgPressed
                      : printHov.containsMouse ? Theme.accentDark : Theme.accentSubtle
                Text { anchors.centerIn: parent; text: root.slicingForPrint ? qsTr("切片中…") : qsTr("▶ 打印"); color: "white"; font.pixelSize: Theme.fontSizeSM; font.bold: true }
                MouseArea {
                    id: printHov; anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: parent.canPrint ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                    enabled: parent.canPrint
                    onClicked: {
                        if (parent.hasResult) {
                            selectMachineDialog.gcodePath = root.editorVm.lastGcodePath
                            selectMachineDialog.open()
                            root.close()
                        } else {
                            root.slicingForPrint = true
                            root.editorVm.requestSlice()
                        }
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: {
                if (root.slicingForPrint)
                    return qsTr("正在切片，完成后将自动选择设备…")
                if (root.editorVm !== null && (root.editorVm.lastGcodePath || "") !== "")
                    return qsTr("G-code 已就绪，选择设备后发送。")
                return qsTr("当前平板还没有切片结果——点击打印将自动切片。")
            }
            visible: true
            color: root.slicingForPrint ? Theme.textSecondary : Theme.textDisabled
            font.pixelSize: Theme.fontSizeXS
            horizontalAlignment: Text.AlignHCenter
        }
    }

    // G-03: continue into device selection once the on-demand slice lands.
    Connections {
        target: root.slicingForPrint ? root.editorVm : null
        enabled: root.slicingForPrint
        function onPrintSliceReady(gcodePath) {
            root.slicingForPrint = false
            if (root.monitorVm === null || !gcodePath)
                return
            selectMachineDialog.gcodePath = gcodePath
            selectMachineDialog.open()
            root.close()
        }
        // A failed or cancelled slice releases the on-demand state (the
        // status line returns to the actionable hint).
        function onStateChanged() {
            if (root.editorVm && !root.editorVm.isSlicing()
                && (root.editorVm.lastGcodePath || "") === "")
                root.slicingForPrint = false
        }
    }

    // v2.5 DEV-05: SelectMachineDialog 实例（发送打印到真机）
    SelectMachineDialog {
        id: selectMachineDialog
        deviceVm: root.monitorVm
        gcodePath: ""
    }
}
