import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../controls"

// D4 -- CaliHistoryDialog: calibration history records dialog
// Usage: CaliHistoryDialog { id: historyDlg; calibrationVm: ... }  ->  historyDlg.open()
// CalibrationPage "History" button opens this dialog
// Aligns with upstream FlowCalibHeaderView history list
CxDialog {
    id: root
    required property var calibrationVm

    dialogTitle: qsTr("校准历史记录")

    anchors.centerIn: parent
    width: 480
    height: 420

    property var _historyItems: []

    // Phase 171 (CL-01): destructive-action confirm for 清空 (clear history).
    ConfirmDialog {
        id: clearConfirm
        dialogTitle: qsTr("清空校准历史")
        message: qsTr("确定要清空所有校准历史记录吗？此操作不可撤销。")
        confirmText: qsTr("清空")
        cancelText: qsTr("取消")
        destructive: true
        onAccepted: {
            if (root.calibrationVm)
                root.calibrationVm.clearHistory()
        }
    }

    function reloadHistory() {
        var arr = []
        var n = calibrationVm ? calibrationVm.historyCount() : 0
        for (var i = 0; i < n; ++i) {
            arr.push({
                name: calibrationVm.historyName(i),
                filamentId: calibrationVm.historyFilamentId(i),
                kValue: calibrationVm.historyKValue(i),
                nozzleDiameter: calibrationVm.historyNozzleDiameter(i),
                timestamp: calibrationVm.historyTimestamp(i)
            })
        }
        _historyItems = arr
    }

    Connections {
        target: root.calibrationVm
        function onHistoryChanged() { reloadHistory() }
    }

    onOpened: reloadHistory()

    contentItem: ColumnLayout {
        id: contentCol
        width: root.width - 32
        spacing: Theme.spacingLG
        // Empty state
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root._historyItems.length === 0
            spacing: Theme.spacingMD
            Item { Layout.fillHeight: true }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "⚙"
                font.pixelSize: 36
                color: Theme.borderDefault
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("暂无校准历史记录")
                color: Theme.borderActive
                font.pixelSize: Theme.fontSizeMD
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("完成校准后会在此显示记录")
                color: Theme.scrollBarHoverColor
                font.pixelSize: Theme.fontSizeXS
            }

            Item { Layout.fillHeight: true }
        }

        // History list
        ListView {
            id: historyListView
            visible: root._historyItems.length > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root._historyItems
            spacing: Theme.spacingXS
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Rectangle {
                width: historyListView.width
                height: 68
                radius: 6
                color: delegateHov.containsMouse ? Theme.bgCard : Theme.bgPanel
                border.color: Theme.borderInput
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingLG
                    anchors.rightMargin: Theme.spacingLG
                    spacing: Theme.spacingLG
                    // Type icon
                    Rectangle {
                        width: 36; height: 36; radius: 6
                        color: Theme.chromePressed
                        Text {
                            anchors.centerIn: parent
                            text: "⚙"
                            font.pixelSize: Theme.fontSizeXL
                            color: Theme.textTertiary
                        }
                    }

                    // Details
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingXS
                        Text {
                            text: modelData.name
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                            font.bold: true
                        }

                        RowLayout {
                            spacing: Theme.spacingLG
                            Text {
                                text: qsTr("K值: %1").arg(modelData.kValue.toFixed(3))
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                            Text {
                                text: qsTr("喷嘴: %1mm").arg(modelData.nozzleDiameter.toFixed(2))
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                            Text {
                                text: modelData.filamentId
                                color: Theme.borderActive
                                font.pixelSize: Theme.fontSizeXS
                            }
                        }

                        Text {
                            text: {
                                // Format timestamp for display
                                var ts = modelData.timestamp
                                if (!ts) return ""
                                try {
                                    var dt = new Date(ts)
                                    return Qt.formatDateTime(dt, "yyyy-MM-dd hh:mm")
                                } catch (e) {
                                    return ts
                                }
                            }
                            color: Theme.scrollBarHoverColor
                            font.pixelSize: Theme.fontSizeXS
                        }
                    }

                    // Action buttons
                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: exportHov.containsMouse ? Theme.borderInput : "transparent"
                        Text { anchors.centerIn: parent; text: "⤓"; color: Theme.textTertiary; font.pixelSize: Theme.fontSizeMD }
                        MouseArea { id: exportHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: {} }
                    }
                }

                HoverHandler { id: delegateHov }
            }
        }

        // Footer with clear button
        Rectangle {
            Layout.fillWidth: true; height: 1; color: Theme.bgCard
            visible: root._historyItems.length > 0
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root._historyItems.length > 0

            Text {
                text: qsTr("共 %1 条记录").arg(root._historyItems.length)
                color: Theme.borderActive
                font.pixelSize: Theme.fontSizeXS
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: 72; height: 28; radius: 4
                color: clearHov.containsMouse ? Theme.bgErrorSubtle : Theme.bgErrorSubtle
                border.color: Theme.statusErrorPressed
                Text { anchors.centerIn: parent; text: qsTr("清空"); color: "#ff9090"; font.pixelSize: Theme.fontSizeSM }
                MouseArea {
                    id: clearHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        // Phase 171 (CL-01): confirm before clearing (was firing immediately).
                        clearConfirm.open()
                    }
                }
            }
        }
    }
}
