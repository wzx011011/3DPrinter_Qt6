import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// D3 — CalibrationDialog：进度 + 状态 + 取消
// 用法：CalibrationDialog { id: calibDlg; calibrationVm: ... }  →  calibDlg.open()
// CalibrationPage 的"开始校准"按钮打开此对话框
Dialog {
    id: root
    required property var calibrationVm

    // Hardware calibration options (对齐上游 CalibrationDialog checkboxes)
    property bool hardwareLidar: true
    property bool hardwareBedLevel: true
    property bool hardwareVibration: true
    property bool hardwareMotor: false

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

        // 硬件校准选项（对齐上游 CalibrationDialog::create_check_option）
        // 上游有 4 个 checkbox: xcam_cali, bed_leveling, vibration, motor_noise
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: !root.calibrationVm || !root.calibrationVm.isRunning

            Text {
                text: qsTr("硬件校准选项")
                color: "#9daaba"
                font.pixelSize: 11
                font.bold: true
            }

            // Micro lidar calibration (对齐上游 xcam_cali)
            Rectangle {
                Layout.fillWidth: true
                height: 28
                radius: 4
                color: lidarMA.containsMouse ? "#1e2535" : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    spacing: 8

                    Rectangle {
                        width: 16; height: 16; radius: 3
                        color: hardwareLidar ? "#22c55e" : "transparent"
                        border.color: hardwareLidar ? "#22c55e" : "#4a5568"
                        border.width: 1.5
                        Text {
                            anchors.centerIn: parent
                            text: hardwareLidar ? "\u2713" : ""
                            color: "white"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }

                    Text {
                        text: qsTr("微型激光雷达校准")
                        color: "#c8d4e0"
                        font.pixelSize: 11
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: qsTr("AI 监控")
                        color: "#6b7a8d"
                        font.pixelSize: 9
                    }
                }

                MouseArea {
                    id: lidarMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: hardwareLidar = !hardwareLidar
                }
            }

            // Bed leveling (对齐上游 bed_leveling)
            Rectangle {
                Layout.fillWidth: true
                height: 28
                radius: 4
                color: bedMA.containsMouse ? "#1e2535" : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    spacing: 8

                    Rectangle {
                        width: 16; height: 16; radius: 3
                        color: hardwareBedLevel ? "#22c55e" : "transparent"
                        border.color: hardwareBedLevel ? "#22c55e" : "#4a5568"
                        border.width: 1.5
                        Text {
                            anchors.centerIn: parent
                            text: hardwareBedLevel ? "\u2713" : ""
                            color: "white"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }

                    Text {
                        text: qsTr("热床调平")
                        color: "#c8d4e0"
                        font.pixelSize: 11
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: qsTr("自动")
                        color: "#6b7a8d"
                        font.pixelSize: 9
                    }
                }

                MouseArea {
                    id: bedMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: hardwareBedLevel = !hardwareBedLevel
                }
            }

            // Vibration compensation (对齐上游 vibration, always shown)
            Rectangle {
                Layout.fillWidth: true
                height: 28
                radius: 4
                color: vibMA.containsMouse ? "#1e2535" : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    spacing: 8

                    Rectangle {
                        width: 16; height: 16; radius: 3
                        color: hardwareVibration ? "#22c55e" : "transparent"
                        border.color: hardwareVibration ? "#22c55e" : "#4a5568"
                        border.width: 1.5
                        Text {
                            anchors.centerIn: parent
                            text: hardwareVibration ? "\u2713" : ""
                            color: "white"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }

                    Text {
                        text: qsTr("振动补偿")
                        color: "#c8d4e0"
                        font.pixelSize: 11
                    }
                }

                MouseArea {
                    id: vibMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: hardwareVibration = !hardwareVibration
                }
            }

            // Motor noise cancellation (对齐上游 motor_noise)
            Rectangle {
                Layout.fillWidth: true
                height: 28
                radius: 4
                color: motorMA.containsMouse ? "#1e2535" : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    spacing: 8

                    Rectangle {
                        width: 16; height: 16; radius: 3
                        color: hardwareMotor ? "#22c55e" : "transparent"
                        border.color: hardwareMotor ? "#22c55e" : "#4a5568"
                        border.width: 1.5
                        Text {
                            anchors.centerIn: parent
                            text: hardwareMotor ? "\u2713" : ""
                            color: "white"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }

                    Text {
                        text: qsTr("电机降噪")
                        color: "#c8d4e0"
                        font.pixelSize: 11
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: qsTr("可选")
                        color: "#6b7a8d"
                        font.pixelSize: 9
                    }
                }

                MouseArea {
                    id: motorMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: hardwareMotor = !hardwareMotor
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
