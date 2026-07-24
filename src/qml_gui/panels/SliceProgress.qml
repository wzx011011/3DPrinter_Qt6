import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// SliceProgress — 切片进度与结果面板（对齐上游 SliceInfoPanel）
Item {
    id: root
    required property var editorVm
    signal exportRequested()

    readonly property int pct: root.editorVm ? root.editorVm.sliceProgress() : 0
    readonly property bool slicingNow: root.editorVm ? root.editorVm.isSlicing() : false
    // Phase 196 (FEAT-01): SliceService::State (Idle=0,Slicing=1,Exporting=2,
    // Completed=3,Cancelled=4,Error=5). Drives dedicated Cancelled/Error banners.
    readonly property int sliceStateEnum: root.editorVm ? root.editorVm.sliceState() : 0
    readonly property bool sliceCancelled: root.sliceStateEnum === 4
    readonly property bool sliceError: root.sliceStateEnum === 5
    readonly property string statusLabel: root.editorVm ? root.editorVm.sliceStatusLabel : qsTr("等待切片")
    readonly property string estimatedTime: root.editorVm ? root.editorVm.sliceEstimatedTime : ""
    readonly property string outputPath: root.editorVm ? root.editorVm.sliceOutputPath : ""
    readonly property string resultWeight: root.editorVm ? root.editorVm.sliceResultWeight : ""
    readonly property string resultPlateLabel: root.editorVm ? root.editorVm.sliceResultPlateLabel : ""
    readonly property string resultFilament: root.editorVm ? root.editorVm.sliceResultFilament : ""
    readonly property string resultCost: root.editorVm ? root.editorVm.sliceResultCost : ""
    readonly property int resultLayerCount: root.editorVm ? root.editorVm.sliceResultLayerCount : 0
    readonly property bool hasSliceResult: root.editorVm ? root.editorVm.hasSliceResult : false
    readonly property bool canRequestSlice: root.editorVm ? root.editorVm.canRequestSlice : false
    readonly property bool canPreview: root.editorVm ? root.editorVm.canPreview : false
    readonly property bool canExportGCode: root.editorVm ? root.editorVm.canExportGCode : false
    readonly property bool primaryActionEnabled: root.slicingNow || root.canRequestSlice
    readonly property bool canSliceAll: !root.slicingNow && (root.canRequestSlice || root.hasSliceResult)
    readonly property string actionLabel: root.editorVm ? root.editorVm.sliceActionLabel : qsTr("▶ 开始切片")
    readonly property string actionHint: root.editorVm ? root.editorVm.sliceActionHint : ""
    readonly property string previewHint: root.editorVm ? root.editorVm.previewActionHint : ""
    readonly property string exportHint: root.editorVm ? root.editorVm.exportActionHint : ""
    readonly property string modelSize: root.editorVm ? root.editorVm.modelSizeText : ""
    readonly property string avgSpeed: root.editorVm ? root.editorVm.avgPrintSpeed : ""
    readonly property string estimatedPrintTime: root.editorVm ? root.editorVm.estimatedPrintTime : ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        // ── 状态图标区 ────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 80
            radius: 6
            color: Theme.bgPanel
            border.color: root.slicingNow ? Theme.accent : Theme.borderSubtle

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 6

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.slicingNow ? "⚙" : (root.pct >= 100 ? "✔" : "☁")
                    font.pixelSize: 28
                    color: root.slicingNow ? Theme.accent
                         : root.pct >= 100 ? Theme.accentDark
                         : Theme.textDisabled
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.statusLabel
                    color: root.slicingNow ? Theme.textPrimary : Theme.textTertiary
                    font.pixelSize: Theme.fontSizeMD
                }
            }
        }

        // ── Phase 196 (FEAT-01): Cancelled/Error 状态横幅（对齐上游
        // BackgroundSlicingProcess / ProgressIndicator 的状态展示） ──────
        Rectangle {
            Layout.fillWidth: true
            visible: root.sliceCancelled || root.sliceError
            height: visible ? 36 : 0
            radius: 4
            color: root.sliceError ? Theme.bgErrorSubtle : Theme.bgWarningSubtle
            border.width: 1
            border.color: root.sliceError ? Theme.statusError : Theme.statusWarning

            Text {
                anchors.centerIn: parent
                text: root.sliceError
                      ? qsTr("✖ 切片失败，请检查模型与参数后重试")
                      : qsTr("✖ 切片已取消")
                color: root.sliceError ? Theme.statusError : Theme.statusWarning
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
            }
        }

        // ── 进度条 ────────────────────────────────────────────────
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: qsTr("切片进度")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: root.pct + "%"
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                }
            }

            CxProgressBar {
                id: progressBar
                Layout.fillWidth: true
                from: 0; to: 100
                value: root.pct
            }
        }

        // ── 全部平板切片完成提示（对齐上游 SliceAll 完成后通知）──
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: allSlicedRow.implicitHeight + 12
            radius: 6
            color: Theme.accentSubtle
            border.width: 1
            border.color: Theme.accent
            visible: root.editorVm && root.editorVm.allPlatesSliced && root.editorVm.plateCount > 1 && !root.slicingNow

            RowLayout {
                id: allSlicedRow
                anchors.centerIn: parent
                spacing: 6

                Text {
                    text: "✓"
                    color: Theme.accent
                    font.pixelSize: Theme.fontSizeXL
                }

                Text {
                    text: qsTr("所有平板已切片完成")
                    color: Theme.accent
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                }
            }
        }

        // ── 多平板切片状态摘要（对齐上游 PartPlateList slice status）──
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            visible: root.editorVm && root.editorVm.plateCount > 1

            Text {
                text: qsTr("平板切片状态")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
            }

            Row {
                spacing: 4
                Layout.fillWidth: true
                Repeater {
                    model: root.editorVm ? root.editorVm.plateCount : 0
                    delegate: Rectangle {
                        readonly property int sliceResultStatus: root.editorVm ? root.editorVm.plateSliceResultStatus(index) : 0
                        width: plateStatusRow.implicitWidth + 8
                        height: 20
                        radius: 4
                        color: !root.editorVm ? Theme.bgElevated
                             : sliceResultStatus === 1 ? Theme.accentSubtle
                             : sliceResultStatus === 2 ? Theme.bgWarningSubtle
                             : Theme.bgElevated
                        border.width: 1
                        border.color: root.editorVm && root.editorVm.currentPlateIndex === index ? Theme.accent
                                      : sliceResultStatus === 2 ? Theme.statusWarning
                                      : Theme.borderSubtle

                        RowLayout {
                            id: plateStatusRow
                            anchors.centerIn: parent
                            spacing: 3

                            Text {
                                text: root.editorVm ? root.editorVm.plateName(index) : ""
                                color: root.editorVm && root.editorVm.currentPlateIndex === index ? Theme.accent : Theme.textSecondary
                                font.pixelSize: Theme.fontSizeXS
                            }

                            Text {
                                text: root.editorVm && root.editorVm.isPlateSliced(index) ? "✓" : ""
                                color: Theme.accent
                                font.pixelSize: Theme.fontSizeXS
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }

        // ── 切片结果摘要（对齐上游 SliceInfoPanel）────────────────
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: infoColumn.implicitHeight + 20
            radius: Theme.radiusMD
            color: Theme.bgPanel
            visible: root.hasSliceResult

            ColumnLayout {
                id: infoColumn
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                // Section header
                Text {
                    text: qsTr("切片结果")
                    color: Theme.accent
                    font.pixelSize: Theme.fontSizeMD
                    font.bold: true
                    Layout.fillWidth: true
                }

                // Divider
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.borderSubtle
                }

                // Model size (对齐上游 SliceInfoPanel model dimensions)
                RowLayout {
                    Layout.fillWidth: true
                    visible: root.modelSize.length > 0
                    Text {
                        text: qsTr("模型尺寸")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.modelSize
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        font.family: "monospace"
                    }
                }

                // Estimated print time
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: qsTr("预计打印时长")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.estimatedTime.length > 0 ? root.estimatedTime : "--:--:--"
                        color: Theme.accent
                        font.pixelSize: Theme.fontSizeMD
                        font.bold: true
                    }

                    // 预估打印时间（对齐上游 PrintEstimatedStatistics::total_time）
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Text {
                            text: qsTr("预估时间:")
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeSM
                        }
                        Text {
                            text: root.estimatedPrintTime.length > 0 ? root.estimatedPrintTime : "--:--:--"
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSM
                            font.family: "monospace"
                        }
                    }
                }

                // Average print speed (对齐上游 PrintEstimatedStatistics)
                RowLayout {
                    Layout.fillWidth: true
                    visible: root.avgSpeed.length > 0
                    Text {
                        text: qsTr("平均打印速度")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.avgSpeed
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        font.family: "monospace"
                    }
                }

                // Current plate
                RowLayout {
                    Layout.fillWidth: true
                    visible: root.resultPlateLabel.length > 0
                    Text {
                        text: qsTr("当前平板")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.resultPlateLabel
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                    }
                }

                // Layer count (对齐上游 SliceInfoPanel)
                RowLayout {
                    Layout.fillWidth: true
                    visible: root.resultLayerCount > 0
                    Text {
                        text: qsTr("切片层数")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.resultLayerCount + qsTr(" 层")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                        font.bold: true
                    }
                }

                // Divider
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.borderSubtle
                    visible: root.resultWeight.length > 0 || root.resultFilament.length > 0 || root.resultCost.length > 0
                }

                // Filament weight (对齐上游 SliceInfoPanel weight display with color chip)
                RowLayout {
                    Layout.fillWidth: true
                    visible: root.resultWeight.length > 0
                    Text {
                        text: qsTr("耗材重量")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.fillWidth: true
                    }
                    // Filament color chip (对齐上游 SliceInfoPopup filament color badge)
                    Rectangle {
                        width: 10; height: 10; radius: 2
                        color: Theme.accent
                    }
                    Text {
                        text: root.resultWeight
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                        font.bold: true
                    }
                }

                // Filament used (length, 对齐上游 SliceInfoPanel filament)
                RowLayout {
                    Layout.fillWidth: true
                    visible: root.resultFilament.length > 0
                    Text {
                        text: qsTr("耗材用量")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.resultFilament
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                        font.bold: true
                    }
                }

                // Estimated cost (对齐上游 PrintEstimatedStatistics)
                RowLayout {
                    Layout.fillWidth: true
                    visible: root.resultCost.length > 0
                    Text {
                        text: qsTr("预估成本")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.resultCost
                        color: Theme.statusWarning
                        font.pixelSize: Theme.fontSizeMD
                        font.bold: true
                    }
                }

                // Divider
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.borderSubtle
                    visible: root.editorVm && root.editorVm.extruderCount > 0
                }

                // Per-extruder filament breakdown (对齐上游 SliceInfoPanel per-extruder grid)
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: root.editorVm && root.editorVm.extruderCount > 0
                    spacing: 6

                    Text {
                        text: qsTr("耗材用量明细")
                        color: Theme.accent
                        font.pixelSize: Theme.fontSizeMD
                        font.bold: true
                        Layout.fillWidth: true
                    }

                    Repeater {
                        model: root.editorVm ? root.editorVm.extruderCount : 0
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            // Extruder color chip
                            Rectangle {
                                width: 10; height: 10; radius: 2
                                color: index === 0 ? Theme.accent : index === 1 ? Theme.statusInfo : index === 2 ? Theme.statusWarning : Theme.statusError
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Text {
                                text: qsTr("挤出机") + (index + 1)
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeSM
                                Layout.fillWidth: true
                            }

                            Text {
                                text: root.editorVm ? root.editorVm.extruderUsedLength(index) : ""
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeSM
                                font.family: "monospace"
                            }

                            Text {
                                text: root.editorVm ? root.editorVm.extruderUsedWeight(index) : ""
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeXS
                                font.family: "monospace"
                            }
                        }
                    }
                }

                // Divider
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.borderSubtle
                    visible: root.outputPath.length > 0
                }

                // Output file path
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    visible: root.outputPath.length > 0

                    Text {
                        text: qsTr("输出文件")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeSM
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.outputPath
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "monospace"
                        wrapMode: Text.WrapAnywhere
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: hintText.implicitHeight + 18
            radius: Theme.radiusMD
            color: root.canRequestSlice ? Theme.bgPanel : Theme.bgErrorSubtle
            border.color: root.canRequestSlice ? Theme.borderSubtle : Qt.darker(Theme.statusError, 1.5)

            Text {
                id: hintText
                anchors.fill: parent
                anchors.margins: 9
                text: root.actionHint
                color: root.canRequestSlice ? Theme.textTertiary : Theme.statusError
                font.pixelSize: Theme.fontSizeSM
                wrapMode: Text.Wrap
            }
        }

        // ── 切片 / 取消按钮 ───────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 30
            radius: Theme.radiusMD
            color: !root.primaryActionEnabled ? Theme.bgHover
                   : actionMA.containsMouse
                     ? (root.slicingNow ? Theme.statusErrorPressed : Theme.accentDark)
                     : (root.slicingNow ? Theme.bgErrorSubtle : Theme.accentSubtle)

            Text {
                anchors.centerIn: parent
                text: root.slicingNow ? qsTr("✕ 取消切片") : root.actionLabel
                color: !root.primaryActionEnabled ? Theme.textDisabled : "white"
                font.pixelSize: Theme.fontSizeMD
                font.bold: true
            }

            MouseArea {
                id: actionMA
                anchors.fill: parent
                hoverEnabled: true
                enabled: root.primaryActionEnabled
                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                onClicked: {
                    if (!root.editorVm) return
                    if (root.slicingNow) {
                        root.editorVm.cancelSlice()
                    } else {
                        root.editorVm.requestSlice()
                    }
                }
            }
        }

        // ── 切片后操作栏（对齐上游 SliceInfoPanel 后续操作）──────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: !root.slicingNow

            // 预览（对齐上游 Plater::priv::on_preview）
            Rectangle {
                Layout.fillWidth: true
                height: 28
                radius: Theme.radiusSM
                color: !root.canPreview ? Theme.bgHover
                       : previewMA.containsMouse ? "#7c3aed" : "#6d28d9"
                Text {
                    anchors.centerIn: parent
                    text: qsTr("预览")
                    color: root.canPreview ? "white" : Theme.textDisabled
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                }
                MouseArea {
                    id: previewMA
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: root.canPreview
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                    onClicked: {
                        if (root.editorVm)
                            root.editorVm.switchToPreview()
                    }
                }
            }

            // 导出 G-code（对齐 upstream Plater::export_gcode）
            Rectangle {
                Layout.fillWidth: true
                height: 28
                radius: Theme.radiusSM
                color: !root.canExportGCode ? Theme.bgHover
                       : exportMA.containsMouse ? Theme.statusInfo : "#1d4ed8"
                Text {
                    anchors.centerIn: parent
                    text: qsTr("导出 G-code")
                    color: root.canExportGCode ? "white" : Theme.textDisabled
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                }
                MouseArea {
                    id: exportMA
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: root.canExportGCode
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                    onClicked: {
                        root.exportRequested()
                    }
                }
            }

            // 切片全部平板
            Rectangle {
                Layout.fillWidth: true
                height: 28
                radius: Theme.radiusSM
                color: !root.canSliceAll ? Theme.bgHover
                       : sliceAllMA.containsMouse ? Theme.bgPressed : Theme.bgElevated
                border.width: 1
                border.color: Theme.borderSubtle
                Text {
                    anchors.centerIn: parent
                    text: qsTr("全部切片")
                    color: root.canSliceAll ? Theme.textPrimary : Theme.textDisabled
                    font.pixelSize: Theme.fontSizeSM
                }
                MouseArea {
                    id: sliceAllMA
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: root.canSliceAll
                    cursorShape: enabled ? Qt.PointingHandCursor : Qt.ForbiddenCursor
                    onClicked: {
                        if (root.editorVm)
                            root.editorVm.requestSliceAll()
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: !root.slicingNow && (!root.canPreview || !root.canExportGCode)
            text: !root.canPreview ? root.previewHint : root.exportHint
            color: Theme.textTertiary
            font.pixelSize: Theme.fontSizeXS
            wrapMode: Text.Wrap
        }

        Item { Layout.fillHeight: true }
    }
}
