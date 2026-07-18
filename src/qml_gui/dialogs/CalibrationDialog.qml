import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../controls"

// D3 -- CalibrationDialog: progress + status + cancel
// Usage: CalibrationDialog { id: calibDlg; calibrationVm: ... }  ->  calibDlg.open()
// CalibrationPage "Start Calibration" button opens this dialog
CxDialog {
    id: root
    required property var calibrationVm

    // Hardware calibration options (aligns with upstream CalibrationDialog checkboxes)
    property bool hardwareLidar: true
    property bool hardwareBedLevel: true
    property bool hardwareVibration: true
    property bool hardwareMotor: false

    dialogTitle: root.calibrationVm ? root.calibrationVm.selectedTitle : qsTr("校准")
    titleIcon: "⚙"
    showCloseButton: !root.calibrationVm || !root.calibrationVm.isRunning

    closePolicy: root.calibrationVm && root.calibrationVm.isRunning
                 ? Popup.NoAutoClose          // Prevent closing outside during calibration
                 : Popup.CloseOnEscape | Popup.CloseOnPressOutside

    anchors.centerIn: parent
    width:  400
    height: contentCol.implicitHeight + 80

    contentItem: ColumnLayout {
        id: contentCol
        width: root.width - 32
        spacing: 16

        // Status icon
        Rectangle {
            Layout.fillWidth: true; height: 72; radius: 6; color: Theme.bgSurface
            border.color: root.calibrationVm && root.calibrationVm.isRunning ? Theme.accent : Theme.borderInput

            ColumnLayout {
                anchors.centerIn: parent; spacing: 6
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.calibrationVm && root.calibrationVm.isRunning ? "⚙"
                         : root.calibrationVm && root.calibrationVm.progress >= 100 ? "✔" : "◎"
                    font.pixelSize: 26
                    color: root.calibrationVm && root.calibrationVm.isRunning ? Theme.accent
                         : root.calibrationVm && root.calibrationVm.progress >= 100 ? Theme.accentDark : Theme.textTertiary
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.calibrationVm && root.calibrationVm.isRunning ? qsTr("校准进行中，请勿移动打印机…")
                         : root.calibrationVm && root.calibrationVm.progress >= 100 ? qsTr("校准完成！")
                         : qsTr("准备开始校准")
                    color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM
                }
            }
        }

        // Progress bar
        ColumnLayout {
            Layout.fillWidth: true; spacing: 4
            RowLayout {
                Layout.fillWidth: true
                Text { text: qsTr("进度"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM }
                Item { Layout.fillWidth: true }
                Text {
                    text: root.calibrationVm ? root.calibrationVm.progress + "%" : "0%"
                    color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; font.bold: true
                }
            }
            CxProgressBar {
                Layout.fillWidth: true; from: 0; to: 100
                value: root.calibrationVm ? root.calibrationVm.progress : 0
            }
        }

        // Divider
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.bgCard }

        // Hardware calibration options (aligns with upstream CalibrationDialog::create_check_option)
        // Upstream has 4 checkboxes: xcam_cali, bed_leveling, vibration, motor_noise
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: !root.calibrationVm || !root.calibrationVm.isRunning

            Text {
                text: qsTr("硬件校准选项")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
            }

            // Micro lidar calibration (aligns with upstream xcam_cali)
            Rectangle {
                Layout.fillWidth: true
                height: 28
                radius: 4
                color: lidarMA.containsMouse ? Theme.bgCard : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    spacing: 8

                    Rectangle {
                        width: 16; height: 16; radius: 3
                        color: hardwareLidar ? Theme.accent : "transparent"
                        border.color: hardwareLidar ? Theme.accent : Theme.scrollBarHoverColor
                        border.width: 1.5
                        Text {
                            anchors.centerIn: parent
                            text: hardwareLidar ? "✓" : ""
                            color: "white"
                            font.pixelSize: Theme.fontSizeXS
                            font.bold: true
                        }
                    }

                    Text {
                        text: qsTr("微型激光雷达校准")
                        color: Theme.chromeText
                        font.pixelSize: Theme.fontSizeSM
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: qsTr("AI 监控")
                        color: Theme.borderActive
                        font.pixelSize: Theme.fontSizeXS
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

            // Bed leveling (aligns with upstream bed_leveling)
            Rectangle {
                Layout.fillWidth: true
                height: 28
                radius: 4
                color: bedMA.containsMouse ? Theme.bgCard : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    spacing: 8

                    Rectangle {
                        width: 16; height: 16; radius: 3
                        color: hardwareBedLevel ? Theme.accent : "transparent"
                        border.color: hardwareBedLevel ? Theme.accent : Theme.scrollBarHoverColor
                        border.width: 1.5
                        Text {
                            anchors.centerIn: parent
                            text: hardwareBedLevel ? "✓" : ""
                            color: "white"
                            font.pixelSize: Theme.fontSizeXS
                            font.bold: true
                        }
                    }

                    Text {
                        text: qsTr("热床调平")
                        color: Theme.chromeText
                        font.pixelSize: Theme.fontSizeSM
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: qsTr("自动")
                        color: Theme.borderActive
                        font.pixelSize: Theme.fontSizeXS
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

            // Vibration compensation (aligns with upstream vibration, always shown)
            Rectangle {
                Layout.fillWidth: true
                height: 28
                radius: 4
                color: vibMA.containsMouse ? Theme.bgCard : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    spacing: 8

                    Rectangle {
                        width: 16; height: 16; radius: 3
                        color: hardwareVibration ? Theme.accent : "transparent"
                        border.color: hardwareVibration ? Theme.accent : Theme.scrollBarHoverColor
                        border.width: 1.5
                        Text {
                            anchors.centerIn: parent
                            text: hardwareVibration ? "✓" : ""
                            color: "white"
                            font.pixelSize: Theme.fontSizeXS
                            font.bold: true
                        }
                    }

                    Text {
                        text: qsTr("振动补偿")
                        color: Theme.chromeText
                        font.pixelSize: Theme.fontSizeSM
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

            // Motor noise cancellation (aligns with upstream motor_noise)
            Rectangle {
                Layout.fillWidth: true
                height: 28
                radius: 4
                color: motorMA.containsMouse ? Theme.bgCard : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    spacing: 8

                    Rectangle {
                        width: 16; height: 16; radius: 3
                        color: hardwareMotor ? Theme.accent : "transparent"
                        border.color: hardwareMotor ? Theme.accent : Theme.scrollBarHoverColor
                        border.width: 1.5
                        Text {
                            anchors.centerIn: parent
                            text: hardwareMotor ? "✓" : ""
                            color: "white"
                            font.pixelSize: Theme.fontSizeXS
                            font.bold: true
                        }
                    }

                    Text {
                        text: qsTr("电机降噪")
                        color: Theme.chromeText
                        font.pixelSize: Theme.fontSizeSM
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        text: qsTr("可选")
                        color: Theme.borderActive
                        font.pixelSize: Theme.fontSizeXS
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

        // Divider
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.bgCard }

        // Phase 125 (CALIB-02): calibration sweep range inputs (start/end/step).
        // Defaults come from the selected CalibrationType (Phase 124 hardcoded
        // seeds); the user can override before starting. The edited values flow
        // CalibrationViewModel -> CalibrationServiceMock::setCalibParams ->
        // startSlice. Shown only when idle and a software-sliceable mode is
        // selected (hardware modes have no sweep range).
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: (!root.calibrationVm || !root.calibrationVm.isRunning)
                     && root.calibrationVm && root.calibrationVm.selectedCategory === "slice"

            Text {
                text: qsTr("校准范围")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
            }
            Text {
                text: qsTr("编辑扫描范围（起始 / 结束 / 步长），覆盖默认值")
                color: Theme.borderActive
                font.pixelSize: Theme.fontSizeXS
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                // Start
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: qsTr("起始"); color: Theme.borderActive; font.pixelSize: Theme.fontSizeXS }
                    CxTextField {
                        id: rangeStartField
                        Layout.fillWidth: true
                        text: root.calibrationVm ? root.calibrationVm.calibStart.toFixed(3) : "0"
                        validator: DoubleValidator {
                            bottom: 0.0
                            decimals: 4
                            notation: DoubleValidator.StandardNotation
                        }
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        onEditingFinished: {
                            if (!root.calibrationVm) return
                            var v = parseFloat(text)
                            if (!isNaN(v)) root.calibrationVm.calibStart = v
                            else text = root.calibrationVm.calibStart.toFixed(3)
                        }
                    }
                }

                // End
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: qsTr("结束"); color: Theme.borderActive; font.pixelSize: Theme.fontSizeXS }
                    CxTextField {
                        id: rangeEndField
                        Layout.fillWidth: true
                        text: root.calibrationVm ? root.calibrationVm.calibEnd.toFixed(3) : "0"
                        validator: DoubleValidator {
                            bottom: 0.0
                            decimals: 4
                            notation: DoubleValidator.StandardNotation
                        }
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        onEditingFinished: {
                            if (!root.calibrationVm) return
                            var v = parseFloat(text)
                            if (!isNaN(v)) root.calibrationVm.calibEnd = v
                            else text = root.calibrationVm.calibEnd.toFixed(3)
                        }
                    }
                }

                // Step
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: qsTr("步长"); color: Theme.borderActive; font.pixelSize: Theme.fontSizeXS }
                    CxTextField {
                        id: rangeStepField
                        Layout.fillWidth: true
                        text: root.calibrationVm ? root.calibrationVm.calibStep.toFixed(4) : "0"
                        validator: DoubleValidator {
                            bottom: 0.0001
                            decimals: 4
                            notation: DoubleValidator.StandardNotation
                        }
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        onEditingFinished: {
                            if (!root.calibrationVm) return
                            var v = parseFloat(text)
                            if (!isNaN(v)) root.calibrationVm.calibStep = v
                            else text = root.calibrationVm.calibStep.toFixed(4)
                        }
                    }
                }
            }

            // Reset to defaults: switching away and back to the mode reloads its
            // hardcoded seeds (Phase 124 defaults). The text fields re-bind via
            // the calibStart/calibEnd/calibStep NOTIFY on selectionChanged.
        }

        // Divider
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.bgCard }

        // Button row
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Item { Layout.fillWidth: true }

            // Cancel calibration (visible during calibration)
            Rectangle {
                width: 88; height: 30; radius: 4; visible: root.calibrationVm && root.calibrationVm.isRunning
                color: stopHov.containsMouse ? Theme.bgErrorSubtle : Theme.bgErrorSubtle
                border.color: Theme.statusErrorPressed
                Text { anchors.centerIn: parent; text: qsTr("✕ 取消"); color: "#ff9090"; font.pixelSize: Theme.fontSizeSM }
                MouseArea {
                    id: stopHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root.calibrationVm) root.calibrationVm.cancelCalibration() }
                }
            }

            // Close (visible when complete)
            Rectangle {
                width: 80; height: 30; radius: 4
                visible: !root.calibrationVm || !root.calibrationVm.isRunning
                color: doneHov.containsMouse ? Theme.accentDark : Theme.accentSubtle
                Text {
                    anchors.centerIn: parent
                    text: root.calibrationVm && root.calibrationVm.progress >= 100 ? qsTr("完成") : qsTr("开始")
                    color: "white"; font.pixelSize: Theme.fontSizeSM; font.bold: true
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
