import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// D4 — CaliHistoryDialog：校准历史记录对话框
// 用法：CaliHistoryDialog { id: historyDlg; calibrationVm: ... }  →  historyDlg.open()
// CalibrationPage 的"历史记录"按钮打开此对话框
// 对齐上游 FlowCalibHeaderView 历史记录列表
Dialog {
    id: root
    required property var calibrationVm

    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    anchors.centerIn: parent
    width: 480
    height: 420

    property var _historyItems: []

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

    background: Rectangle {
        color: "#1a1f28"; radius: 8
        border.color: "#2e3848"; border.width: 1
    }

    header: Rectangle {
        width: parent.width; height: 44
        color: "#141920"; radius: 8
        Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; height: 12; color: parent.color }

        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 12; spacing: 10
            Text {
                text: qsTr("校准历史记录")
                color: "#e2e8f5"; font.pixelSize: 14; font.bold: true
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                width: 24; height: 24; radius: 4
                color: xHov.containsMouse ? "#3d2020" : "transparent"
                Text { anchors.centerIn: parent; text: "\u2715"; color: "#7a8fa3"; font.pixelSize: 12 }
                MouseArea { id: xHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.close() }
            }
        }
    }

    contentItem: ColumnLayout {
        id: contentCol
        width: root.width - 32
        spacing: 12

        // Empty state
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root._historyItems.length === 0
            spacing: 8

            Item { Layout.fillHeight: true }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "\u2699"
                font.pixelSize: 36
                color: "#3a4250"
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("暂无校准历史记录")
                color: "#6b7a8d"
                font.pixelSize: 12
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("完成校准后会在此显示记录")
                color: "#4a5568"
                font.pixelSize: 10
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
            spacing: 4

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Rectangle {
                width: historyListView.width
                height: 68
                radius: 6
                color: delegateHov.containsMouse ? "#1e2535" : "#151a24"
                border.color: "#2e3848"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 12

                    // Type icon
                    Rectangle {
                        width: 36; height: 36; radius: 6
                        color: "#252d3a"
                        Text {
                            anchors.centerIn: parent
                            text: "\u2699"
                            font.pixelSize: 16
                            color: "#7a8fa3"
                        }
                    }

                    // Details
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: modelData.name
                            color: "#e2e8f5"
                            font.pixelSize: 12
                            font.bold: true
                        }

                        RowLayout {
                            spacing: 12
                            Text {
                                text: qsTr("K值: %1").arg(modelData.kValue.toFixed(3))
                                color: "#7a8fa3"
                                font.pixelSize: 10
                            }
                            Text {
                                text: qsTr("喷嘴: %1mm").arg(modelData.nozzleDiameter.toFixed(2))
                                color: "#7a8fa3"
                                font.pixelSize: 10
                            }
                            Text {
                                text: modelData.filamentId
                                color: "#6b7a8d"
                                font.pixelSize: 9
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
                            color: "#4a5568"
                            font.pixelSize: 9
                        }
                    }

                    // Action buttons
                    Rectangle {
                        width: 28; height: 28; radius: 4
                        color: exportHov.containsMouse ? "#2a3a4a" : "transparent"
                        Text { anchors.centerIn: parent; text: "\u2913"; color: "#7a8fa3"; font.pixelSize: 12 }
                        MouseArea { id: exportHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: {} }
                    }
                }

                HoverHandler { id: delegateHov }
            }
        }

        // Footer with clear button
        Rectangle {
            Layout.fillWidth: true; height: 1; color: "#1e2535"
            visible: root._historyItems.length > 0
        }

        RowLayout {
            Layout.fillWidth: true
            visible: root._historyItems.length > 0

            Text {
                text: qsTr("共 %1 条记录").arg(root._historyItems.length)
                color: "#6b7a8d"
                font.pixelSize: 10
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: 72; height: 28; radius: 4
                color: clearHov.containsMouse ? "#5e1818" : "#3a1010"
                border.color: "#6b2020"
                Text { anchors.centerIn: parent; text: qsTr("清空"); color: "#ff9090"; font.pixelSize: 11 }
                MouseArea {
                    id: clearHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root.calibrationVm)
                            root.calibrationVm.clearHistory()
                    }
                }
            }
        }
    }
}
