import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// D3 -- CalibrationDialog: upstream two-column hardware calibration dialog
// (Calibration.cpp:23-170). Left 303-wide "step selection" panel with the
// seven hardware checkboxes + "Calibration program" note (:38-93); right
// 182x200 "Calibration Flow" panel with the step indicator and the start
// button (:123-153); body margins 25 (:164).
// Usage: CalibrationDialog { id: calibDlg; calibrationVm: ... }  ->  calibDlg.open()
// CalibrationPage "Start Calibration" button opens this dialog
CxDialog {
    id: root
    required property var calibrationVm

    // Hardware calibration options - the seven upstream checkboxes
    // (Calibration.cpp:53-59). Every option defaults to checked except
    // bed_cali, per STUDIO-10091 (Calibration.cpp:62-65).
    property bool hardwareLidar: true          // Micro lidar calibration (xcam_cali)
    property bool hardwareBedLevel: true       // Bed leveling (bed_leveling)
    property bool hardwareVibration: true      // Vibration compensation (vibration)
    property bool hardwareMotor: true          // Motor noise cancellation (motor_noise)
    property bool hardwareNozzleOffset: true   // Nozzle offset calibration (nozzle_cali)
    property bool hardwareHeatbed: false       // High-temperature Heatbed Calibration (bed_cali)
    property bool hardwareClumpDetection: true // Nozzle clumping detection (clump_pos_cali)

    // U03: capability-gated options hide and force-unselect per upstream
    // update_cali (Calibration.cpp:210-254). Vibration is never gated.
    readonly property bool caliRunning: root.calibrationVm ? root.calibrationVm.isRunning : false
    readonly property bool caliDone: root.calibrationVm
                                     ? (!root.calibrationVm.isRunning
                                        && root.calibrationVm.progress >= 100) : false
    // Upstream idle "no step selected" check inspects exactly these six
    // options -- clump_pos_cali is intentionally not part of the upstream
    // list (Calibration.cpp:298-300).
    readonly property bool noStepSelected: !hardwareLidar && !hardwareBedLevel
                                           && !hardwareVibration && !hardwareMotor
                                           && !hardwareNozzleOffset && !hardwareHeatbed

    // U03: flush the seven checkboxes onto the shared VM right before start,
    // mirroring upstream on_start_calibration -> command_start_calibration
    // (Calibration.cpp:324-341). Previously these selections were dead
    // dialog-local state that nothing consumed.
    function startCali() {
        if (!root.calibrationVm) return
        root.calibrationVm.setHardwareOptions(hardwareLidar, hardwareBedLevel,
                                              hardwareVibration, hardwareMotor,
                                              hardwareNozzleOffset, hardwareHeatbed,
                                              hardwareClumpDetection)
        root.calibrationVm.startCalibration()
    }

    // Step indicator snapshot, rebuilt on every VM step/run/state change
    // (Q_INVOKABLE step accessors do not auto-refresh bindings).
    property var _stepItems: []
    function reloadSteps() {
        var arr = []
        if (root.calibrationVm && (root.caliRunning || root.caliDone)) {
            var n = root.calibrationVm.stepCount()
            for (var i = 0; i < n; ++i) {
                arr.push({
                    title: root.calibrationVm.stepTitle(i),
                    state: root.calibrationVm.stepState(i) // 0 pending / 1 active / 2 done
                })
            }
        }
        _stepItems = arr
    }
    onOpened: reloadSteps()
    Connections {
        target: root.calibrationVm
        function onStepChanged() { root.reloadSteps() }
        function onRunningChanged() { root.reloadSteps() }
        function onProgressChanged() { root.reloadSteps() }
        function onSelectionChanged() { root.reloadSteps() }
    }

    dialogTitle: root.calibrationVm ? root.calibrationVm.selectedTitle : qsTr("校准")
    titleIcon: "⚙"
    showCloseButton: !root.calibrationVm || !root.calibrationVm.isRunning

    closePolicy: root.calibrationVm && root.calibrationVm.isRunning
                 ? Popup.NoAutoClose          // Prevent closing outside during calibration
                 : Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // Centering comes from the CxDialog default (anchors.centerIn:
    // Overlay.overlay, U01 global fix)
    // 303 left panel + 8 gap + 182 right panel + 2 x 25 body margins
    // (Calibration.cpp:38/:118/:123/:164)
    width:  553
    height: contentCol.implicitHeight + 80

    // One hardware option row: 18px checkbox + 5px vertical padding + 11px
    // gap + 13px label, 15px indent, tooltip (upstream create_check_option,
    // Calibration.cpp:176-208).
    component CaliOptionRow: Rectangle {
        id: optRow
        property string label: ""
        property bool checked: false
        signal toggled()

        visible: capabilityFlag.length === 0
                 || (root.calibrationVm ? root.calibrationVm[capabilityFlag] : true)
        // Upstream hides the option and force-unchecks it when the device
        // lacks the capability (update_cali, Calibration.cpp:210-254).
        onVisibleChanged: if (!visible && optRow.checked) optRow.toggled()

        // Per-row capability flag name on the VM (supportXxxCali family);
        // empty = never gated (vibration).
        property string capabilityFlag: ""

        Layout.fillWidth: true
        Layout.leftMargin: 15
        height: 28
        radius: Theme.radiusSM
        color: optMA.containsMouse ? Theme.bgHover : "transparent"

        RowLayout {
            anchors.fill: parent
            spacing: Theme.spacingLG

            Item {
                Layout.preferredWidth: 18
                Layout.preferredHeight: 28
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 18; height: 18; radius: Theme.radiusSM
                    color: optRow.checked ? Theme.accent : "transparent"
                    border.color: optRow.checked ? Theme.accent : Theme.borderInput
                    border.width: 1.5
                    Text {
                        anchors.centerIn: parent
                        text: optRow.checked ? "✓" : ""
                        color: Theme.textOnAccent
                        font.pixelSize: Theme.fontSizeSM
                        font.bold: true
                    }
                }
            }

            Text {
                text: optRow.label
                color: Theme.textSecondary
                font.pixelSize: 13
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        MouseArea {
            id: optMA
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: optRow.toggled()
        }

        ToolTip.visible: optMA.containsMouse && optRow.label.length > 0
        ToolTip.text: optRow.label
        ToolTip.delay: 400
    }

    contentItem: ColumnLayout {
        id: contentCol
        width: root.width - 50   // upstream body margins wxALL 25 (Calibration.cpp:164)
        spacing: Theme.spacingLG

        // ---- Two-column body (Calibration.cpp:34-38/:116-123) ----
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            // LEFT panel: step selection + calibration program note
            Rectangle {
                Layout.preferredWidth: 303
                Layout.fillHeight: true
                Layout.preferredHeight: leftCol.implicitHeight + 45
                radius: Theme.radiusSM
                color: Theme.bgSurface

                ColumnLayout {
                    id: leftCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 15
                    anchors.rightMargin: 15
                    anchors.topMargin: 25
                    spacing: Theme.spacingXS

                    // "Calibration step selection" Head_14 (Calibration.cpp:46-51)
                    Text {
                        text: qsTr("校准步骤选择")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeLG
                        font.bold: true
                    }

                    Item { Layout.preferredHeight: 12; Layout.fillWidth: true }

                    // Micro lidar calibration (aligns with upstream xcam_cali,
                    // gated by SupportAIMonitor && SupportCalibrationLidar)
                    CaliOptionRow {
                        label: qsTr("微型激光雷达校准")
                        checked: root.hardwareLidar
                        capabilityFlag: "supportLidarCali"
                        onToggled: root.hardwareLidar = !root.hardwareLidar
                    }

                    // Bed leveling (aligns with upstream bed_leveling,
                    // gated by is_support_bed_leveling)
                    CaliOptionRow {
                        label: qsTr("热床调平")
                        checked: root.hardwareBedLevel
                        capabilityFlag: "supportBedLeveling"
                        onToggled: root.hardwareBedLevel = !root.hardwareBedLevel
                    }

                    // Vibration compensation (aligns with upstream vibration,
                    // never gated by update_cali)
                    CaliOptionRow {
                        label: qsTr("振动补偿")
                        checked: root.hardwareVibration
                        onToggled: root.hardwareVibration = !root.hardwareVibration
                    }

                    // Motor noise cancellation (aligns with upstream
                    // motor_noise, gated by is_support_motor_noise_cali)
                    CaliOptionRow {
                        label: qsTr("电机降噪")
                        checked: root.hardwareMotor
                        capabilityFlag: "supportMotorNoiseCali"
                        onToggled: root.hardwareMotor = !root.hardwareMotor
                    }

                    // Nozzle offset calibration (aligns with upstream
                    // nozzle_cali, gated by SupportCalibrationNozzleOffset)
                    CaliOptionRow {
                        label: qsTr("喷嘴偏移校准")
                        checked: root.hardwareNozzleOffset
                        capabilityFlag: "supportNozzleOffsetCali"
                        onToggled: root.hardwareNozzleOffset = !root.hardwareNozzleOffset
                    }

                    // High-temperature Heatbed Calibration (aligns with
                    // upstream bed_cali, gated by
                    // SupportCalibrationHighTempBed; the only option that
                    // defaults to unchecked, STUDIO-10091)
                    CaliOptionRow {
                        label: qsTr("高温热床校准")
                        checked: root.hardwareHeatbed
                        capabilityFlag: "supportHighTempBedCali"
                        onToggled: root.hardwareHeatbed = !root.hardwareHeatbed
                    }

                    // Nozzle clumping detection Calibration (aligns with
                    // upstream clump_pos_cali, gated by SupportCaliClumpPos)
                    CaliOptionRow {
                        label: qsTr("喷嘴堵塞检测校准")
                        checked: root.hardwareClumpDetection
                        capabilityFlag: "supportClumpPosCali"
                        onToggled: root.hardwareClumpDetection = !root.hardwareClumpDetection
                    }

                    Item { Layout.preferredHeight: 18; Layout.fillWidth: true }

                    // "Calibration program" Head_14 (Calibration.cpp:77-83)
                    Text {
                        text: qsTr("校准程序")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeLG
                        font.bold: true
                    }

                    // Description body 13px, wraps ~260 (Calibration.cpp:85-93)
                    Text {
                        text: qsTr("校准程序会自动检测设备状态以减小偏差。\n它能让设备持续保持最佳性能。")
                        color: Theme.textTertiary
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
            }

            // RIGHT panel: calibration flow + start button (182x200 min,
            // Calibration.cpp:123-153)
            Rectangle {
                Layout.preferredWidth: 182
                Layout.fillHeight: true
                Layout.minimumHeight: 200
                Layout.preferredHeight: Math.max(200, rightCol.implicitHeight + 50)
                radius: Theme.radiusSM
                color: Theme.bgSurface

                ColumnLayout {
                    id: rightCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    anchors.topMargin: 14
                    spacing: Theme.spacingSM

                    // "Calibration Flow" Head_14 in brand colour, centered
                    // (Calibration.cpp:127-131)
                    Text {
                        text: qsTr("校准流程")
                        color: Theme.accent
                        font.pixelSize: Theme.fontSizeLG
                        font.bold: true
                        Layout.alignment: Qt.AlignHCenter
                    }

                    // StaticLine in brand colour (Calibration.cpp:133-134)
                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.accent
                    }

                    // U03: step indicator. Upstream populates it at runtime
                    // from the device's stage_list_info and highlights the
                    // current stage, each row 35px tall (:256-287). The OWzx
                    // VM has no device runtime-stage enumeration, so the
                    // indicator maps the selected mode's wizard step flow
                    // (stepCount/stepTitle/stepState) -- the device runtime
                    // stage channel is the registered gap. Idle shows no
                    // items, mirroring upstream DeleteAllItems (:296) --
                    // the empty _stepItems model already renders zero
                    // delegates. (A previous bare
                    // `visible: root.caliRunning || root.caliDone` placed
                    // here bound to rightCol itself and hid the
                    // always-visible "Calibration Flow" title + StaticLine
                    // when idle; upstream keeps both, Calibration.cpp:127-134.)
                    Repeater {
                        model: root._stepItems

                        delegate: Item {
                            id: stepDelegate
                            Layout.fillWidth: true
                            Layout.preferredHeight: 35

                            RowLayout {
                                anchors.fill: parent
                                spacing: 8

                                Rectangle {
                                    width: 8; height: 8; radius: Theme.radiusSM
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: stepDelegate.modelData.state >= 1 ? Theme.accent : Theme.borderSubtle
                                }

                                Text {
                                    text: stepDelegate.modelData.title
                                    color: stepDelegate.modelData.state === 1 ? Theme.accent
                                         : stepDelegate.modelData.state === 2 ? Theme.textPrimary
                                         : Theme.textTertiary
                                    font.pixelSize: 13
                                    font.bold: stepDelegate.modelData.state === 1
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    visible: stepDelegate.modelData.state === 2
                                    text: "✓"
                                    color: Theme.accent
                                    font.pixelSize: 13
                                }
                            }
                        }
                    }
                }

                // Main button: 100x32, radius 4, 14px -- upstream
                // ButtonType::Choice (Widgets/Button.cpp:200-204). Three
                // states mirror update_cali (:256-308): running -> disabled
                // "Calibrating"; done -> "Completed" closes on click
                // (:327-330); idle with nothing selected -> disabled
                // "No step selected"; otherwise "Start Calibration".
                CxButton {
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottomMargin: 14
                    width: 100
                    height: 32
                    cxStyle: CxButton.Style.Primary
                    enabled: !root.caliRunning && !(root.noStepSelected && !root.caliDone)
                    text: root.caliRunning ? qsTr("校准中")
                        : root.caliDone ? qsTr("已完成")
                        : root.noStepSelected ? qsTr("未选择步骤")
                        : qsTr("开始校准")
                    onClicked: {
                        if (root.caliDone) {
                            root.close()
                            return
                        }
                        root.startCali()
                    }
                }
            }
        }

        // ---- Progress (OWzx extension kept: the mock calibration reports
        // live progress; upstream reads the device state instead) ----
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingXS
            visible: root.calibrationVm
                     && (root.caliRunning || root.calibrationVm.progress > 0)

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: root.caliRunning ? qsTr("校准进行中，请勿移动打印机…")
                        : root.caliDone ? qsTr("校准完成！")
                        : qsTr("进度")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: root.calibrationVm ? root.calibrationVm.progress + "%" : "0%"
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                }
            }
            CxProgressBar {
                Layout.fillWidth: true
                from: 0; to: 100
                value: root.calibrationVm ? root.calibrationVm.progress : 0
            }
        }

        // Phase 125 (CALIB-02): calibration sweep range inputs (start/end/step).
        // Defaults come from the selected CalibrationType (Phase 124 hardcoded
        // seeds); the user can override before starting. The edited values flow
        // CalibrationViewModel -> CalibrationServiceMock::setCalibParams ->
        // startSlice. Shown only when idle and a software-sliceable mode is
        // selected (hardware modes have no sweep range).
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
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
                spacing: Theme.spacingMD
                // Start
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingXS
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
                    spacing: Theme.spacingXS
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
                    spacing: Theme.spacingXS
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
    }
}
