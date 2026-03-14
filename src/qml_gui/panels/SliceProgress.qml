import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// SliceProgress — 切片进度与结果面板（对齐上游 SliceInfoPanel）
Item {
    id: root
    required property var editorVm

    readonly property int pct: root.editorVm ? root.editorVm.sliceProgress() : 0
    readonly property bool slicingNow: root.editorVm ? root.editorVm.isSlicing() : false
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
    readonly property string actionLabel: root.editorVm ? root.editorVm.sliceActionLabel : qsTr("▶ 开始切片")
    readonly property string actionHint: root.editorVm ? root.editorVm.sliceActionHint : ""
    readonly property string modelSize: root.editorVm ? root.editorVm.modelSizeText : ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 12

        // ── 状态图标区 ────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 80
            radius: 6
            color: "#1e2229"
            border.color: root.slicingNow ? "#22c564" : "#2e3540"

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 6

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.slicingNow ? "⚙" : (root.pct >= 100 ? "✔" : "☁")
                    font.pixelSize: 28
                    color: root.slicingNow ? "#22c564"
                         : root.pct >= 100 ? "#1baa52"
                         : "#566070"
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: root.statusLabel
                    color: root.slicingNow ? "#dde4ef" : "#7a8fa3"
                    font.pixelSize: 12
                }
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
                    color: "#9daaba"
                    font.pixelSize: 11
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: root.pct + "%"
                    color: "#e2e8f1"
                    font.pixelSize: 11
                    font.bold: true
                }
            }

            ProgressBar {
                id: progressBar
                Layout.fillWidth: true
                from: 0; to: 100
                value: root.pct

                background: Rectangle {
                    implicitHeight: 6
                    radius: 3
                    color: "#2a3040"
                }
                contentItem: Item {
                    Rectangle {
                        width: progressBar.visualPosition * parent.width
                        height: parent.height
                        radius: 3
                        color: root.slicingNow ? "#22c564" : (root.pct >= 100 ? "#1baa52" : "#405060")
                        Behavior on width { NumberAnimation { duration: 150 } }
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
                    font.pixelSize: 12
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
                        font.pixelSize: 11
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.modelSize
                        color: Theme.textPrimary
                        font.pixelSize: 11
                        font.family: "monospace"
                    }
                }

                // Estimated print time
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: qsTr("预计打印时长")
                        color: Theme.textTertiary
                        font.pixelSize: 11
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.estimatedTime.length > 0 ? root.estimatedTime : "--:--:--"
                        color: Theme.accent
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                // Current plate
                RowLayout {
                    Layout.fillWidth: true
                    visible: root.resultPlateLabel.length > 0
                    Text {
                        text: qsTr("当前平板")
                        color: Theme.textTertiary
                        font.pixelSize: 11
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.resultPlateLabel
                        color: Theme.textPrimary
                        font.pixelSize: 12
                    }
                }

                // Layer count (对齐上游 SliceInfoPanel)
                RowLayout {
                    Layout.fillWidth: true
                    visible: root.resultLayerCount > 0
                    Text {
                        text: qsTr("切片层数")
                        color: Theme.textTertiary
                        font.pixelSize: 11
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.resultLayerCount + qsTr(" 层")
                        color: Theme.textPrimary
                        font.pixelSize: 12
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
                        font.pixelSize: 11
                        Layout.fillWidth: true
                    }
                    // Filament color chip (对齐上游 SliceInfoPopup filament color badge)
                    Rectangle {
                        width: 10; height: 10; radius: 2
                        color: "#18c75e"
                    }
                    Text {
                        text: root.resultWeight
                        color: Theme.textPrimary
                        font.pixelSize: 12
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
                        font.pixelSize: 11
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.resultFilament
                        color: Theme.textPrimary
                        font.pixelSize: 12
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
                        font.pixelSize: 11
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.resultCost
                        color: Theme.statusWarning
                        font.pixelSize: 12
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
                        font.pixelSize: 12
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
                                color: index === 0 ? "#18c75e" : index === 1 ? "#3b82f6" : index === 2 ? "#f59e0b" : "#ef4444"
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Text {
                                text: qsTr("挤出机") + (index + 1)
                                color: Theme.textTertiary
                                font.pixelSize: 11
                                Layout.fillWidth: true
                            }

                            Text {
                                text: root.editorVm ? root.editorVm.extruderUsedLength(index) : ""
                                color: Theme.textPrimary
                                font.pixelSize: 11
                                font.family: "monospace"
                            }

                            Text {
                                text: root.editorVm ? root.editorVm.extruderUsedWeight(index) : ""
                                color: Theme.textSecondary
                                font.pixelSize: 10
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
                        font.pixelSize: 11
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.outputPath
                        color: Theme.textSecondary
                        font.pixelSize: 10
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
            color: root.canRequestSlice ? Theme.bgPanel : "#241d1d"
            border.color: root.canRequestSlice ? Theme.borderSubtle : "#5d2f2f"

            Text {
                id: hintText
                anchors.fill: parent
                anchors.margins: 9
                text: root.actionHint
                color: root.canRequestSlice ? Theme.textTertiary : Theme.statusError
                font.pixelSize: 11
                wrapMode: Text.Wrap
            }
        }

        // ── 切片 / 取消按钮 ───────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 30
            radius: Theme.radiusMD
            color: !root.slicingNow && !root.canRequestSlice ? Theme.bgHover
                   : actionMA.containsMouse
                     ? (root.slicingNow ? "#7d2020" : "#19a84e")
                     : (root.slicingNow ? "#5e1818" : "#157a39")

            Text {
                anchors.centerIn: parent
                text: root.slicingNow ? qsTr("✕ 取消切片") : root.actionLabel
                color: !root.slicingNow && !root.canRequestSlice ? Theme.textDisabled : "white"
                font.pixelSize: 12
                font.bold: true
            }

            MouseArea {
                id: actionMA
                anchors.fill: parent
                hoverEnabled: true
                enabled: root.slicingNow || root.canRequestSlice
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
            visible: root.hasSliceResult && !root.slicingNow

            // 预览（对齐上游 Plater::priv::on_preview）
            Rectangle {
                Layout.fillWidth: true
                height: 28
                radius: Theme.radiusSM
                color: previewMA.containsMouse ? "#7c3aed" : "#6d28d9"
                Text {
                    anchors.centerIn: parent
                    text: qsTr("预览")
                    color: "white"
                    font.pixelSize: 11
                    font.bold: true
                }
                MouseArea {
                    id: previewMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
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
                color: exportMA.containsMouse ? "#2563eb" : "#1d4ed8"
                Text {
                    anchors.centerIn: parent
                    text: qsTr("导出 G-code")
                    color: "white"
                    font.pixelSize: 11
                    font.bold: true
                }
                MouseArea {
                    id: exportMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (!root.editorVm) return
                        const path = root.outputPath.length > 0
                            ? root.outputPath
                            : "output.gcode"
                        root.editorVm.requestExportGCode(path)
                    }
                }
            }

            // 切片全部平板
            Rectangle {
                Layout.fillWidth: true
                height: 28
                radius: Theme.radiusSM
                color: sliceAllMA.containsMouse ? Theme.bgHover : Theme.bgElevated
                border.width: 1
                border.color: Theme.borderSubtle
                Text {
                    anchors.centerIn: parent
                    text: qsTr("全部切片")
                    color: Theme.textPrimary
                    font.pixelSize: 11
                }
                MouseArea {
                    id: sliceAllMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root.editorVm)
                            root.editorVm.requestSliceAll()
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
