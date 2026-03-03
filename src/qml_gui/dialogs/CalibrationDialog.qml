import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// D3 — CalibrationDialog：进度 + 状态 + 取消
// 用法：CalibrationDialog { id: calibDlg; calibrationVm: ... }  →  calibDlg.open()
// CalibrationPage 的"开始校准"按钮打开此对话框
Dialog {
    id: root
    required property var calibrationVm

    modal: true
    closePolicy: root.calibrationVm && root.calibrationVm.isRunning
                 ? Popup.NoAutoClose          // 校准进行中禁止点外部关闭
                 : Popup.CloseOnEscape | Popup.CloseOnPressOutside

    anchors.centerIn: parent
    width:  400
    height: contentCol.implicitHeight + 80

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
                text: "⚙  " + (root.calibrationVm ? root.calibrationVm.selectedTitle : qsTr("校准"))
                color: "#e2e8f5"; font.pixelSize: 14; font.bold: true
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                width: 24; height: 24; radius: 4; visible: !root.calibrationVm || !root.calibrationVm.isRunning
                color: xHov.containsMouse ? "#3d2020" : "transparent"
                Text { anchors.centerIn: parent; text: "✕"; color: "#7a8fa3"; font.pixelSize: 12 }
                MouseArea { id: xHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.close() }
            }
        }
    }

    contentItem: ColumnLayout {
        id: contentCol
        width: root.width - 32
        spacing: 16

        // 状态图标
        Rectangle {
            Layout.fillWidth: true; height: 72; radius: 6; color: "#111620"
            border.color: root.calibrationVm && root.calibrationVm.isRunning ? "#22c564" : "#2e3848"

            ColumnLayout {
                anchors.centerIn: parent; spacing: 6
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.calibrationVm && root.calibrationVm.isRunning ? "⚙"
                         : root.calibrationVm && root.calibrationVm.progress >= 100 ? "✔" : "◎"
                    font.pixelSize: 26
                    color: root.calibrationVm && root.calibrationVm.isRunning ? "#22c564"
                         : root.calibrationVm && root.calibrationVm.progress >= 100 ? "#1baa52" : "#7a8fa3"
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.calibrationVm && root.calibrationVm.isRunning ? qsTr("校准进行中，请勿移动打印机…")
                         : root.calibrationVm && root.calibrationVm.progress >= 100 ? qsTr("校准完成！")
                         : qsTr("准备开始校准")
                    color: "#9daaba"; font.pixelSize: 11
                }
            }
        }

        // 进度条
        ColumnLayout {
            Layout.fillWidth: true; spacing: 4
            RowLayout {
                Layout.fillWidth: true
                Text { text: qsTr("进度"); color: "#9daaba"; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
                Text {
                    text: root.calibrationVm ? root.calibrationVm.progress + "%" : "0%"
                    color: "#e2e8f1"; font.pixelSize: 11; font.bold: true
                }
            }
            ProgressBar {
                id: progressBar
                Layout.fillWidth: true; from: 0; to: 100
                value: root.calibrationVm ? root.calibrationVm.progress : 0
                background: Rectangle { implicitHeight: 6; radius: 3; color: "#2a3040" }
                contentItem: Item {
                    Rectangle {
                        width: progressBar.visualPosition * parent.width; height: parent.height; radius: 3
                        color: "#22c564"
                        Behavior on width { NumberAnimation { duration: 200 } }
                    }
                }
            }
        }

        // 分割线
        Rectangle { Layout.fillWidth: true; height: 1; color: "#1e2535" }

        // 按钮行
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Item { Layout.fillWidth: true }

            // 取消校准（进行中才显示）
            Rectangle {
                width: 88; height: 30; radius: 4; visible: root.calibrationVm && root.calibrationVm.isRunning
                color: stopHov.containsMouse ? "#5e1818" : "#3a1010"
                border.color: "#6b2020"
                Text { anchors.centerIn: parent; text: qsTr("✕ 取消"); color: "#ff9090"; font.pixelSize: 11 }
                MouseArea {
                    id: stopHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root.calibrationVm) root.calibrationVm.cancelCalibration() }
                }
            }

            // 关闭（完成后显示）
            Rectangle {
                width: 80; height: 30; radius: 4
                visible: !root.calibrationVm || !root.calibrationVm.isRunning
                color: doneHov.containsMouse ? "#19a84e" : "#157a39"
                Text {
                    anchors.centerIn: parent
                    text: root.calibrationVm && root.calibrationVm.progress >= 100 ? qsTr("完成") : qsTr("开始")
                    color: "white"; font.pixelSize: 11; font.bold: true
                }
                MouseArea {
                    id: doneHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root.calibrationVm && root.calibrationVm.progress < 100)
                            root.calibrationVm.startCalibration()
                        else
                            root.close()
                    }
                }
            }
        }
    }
}
