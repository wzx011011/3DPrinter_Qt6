import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: root
    required property var previewVm

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingSM

        Label { text: qsTr("统计"); color: Theme.textPrimary; font.bold: true; font.pixelSize: Theme.fontSizeLG }

        // Normal/Stealth mode toggle (对齐上游 PrintEstimatedStatistics modes[0]/modes[1])
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSM

            Text {
                text: qsTr("Normal")
                color: !root.previewVm.stealthMode ? Theme.accent : Theme.textTertiary
                font.pixelSize: Theme.fontSizeSM
                font.bold: !root.previewVm.stealthMode
            }

            Switch {
                checked: root.previewVm.stealthMode
                onToggled: if (root.previewVm) root.previewVm.setStealthMode(checked)
            }

            Text {
                text: qsTr("Stealth")
                color: root.previewVm.stealthMode ? Theme.accent : Theme.textTertiary
                font.pixelSize: Theme.fontSizeSM
                font.bold: root.previewVm.stealthMode
            }

            Item { Layout.fillWidth: true }

            Text {
                text: root.previewVm.stealthMode ? qsTr("(静音模式 ~1.4x)") : ""
                color: Theme.textTertiary
                font.pixelSize: 10
            }
        }

        Rectangle {
            Layout.fillWidth: true
            radius: 12
            color: Theme.bgElevated
            border.width: 1
            border.color: Theme.borderSubtle

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                // General statistics
                Label { text: qsTr("总时间: ") + root.previewVm.totalTime; color: Theme.textPrimary; font.pixelSize: 12 }
                Label { text: qsTr("层数: ") + root.previewVm.layerCount; color: Theme.textPrimary; font.pixelSize: 12 }
                Label { text: qsTr("总移动: ") + root.previewVm.moveCount; color: Theme.textPrimary; font.pixelSize: 12 }
                Label { text: qsTr("挤出移动: ") + root.previewVm.extrudeMoveCount; color: Theme.textPrimary; font.pixelSize: 12 }
                Label { text: qsTr("空驶移动: ") + root.previewVm.travelMoveCount; color: Theme.textPrimary; font.pixelSize: 12 }
                Label { text: qsTr("耗材长度: ") + root.previewVm.filamentUsed; color: Theme.textPrimary; font.pixelSize: 12 }
                Label { text: qsTr("耗材重量: ") + root.previewVm.filamentWeight; color: Theme.textPrimary; font.pixelSize: 12 }
                Label { text: qsTr("平均速度: ") + root.previewVm.avgSpeed; color: Theme.textPrimary; font.pixelSize: 12 }
                Label { text: qsTr("工具切换: ") + root.previewVm.toolChangeCount; color: Theme.textPrimary; font.pixelSize: 12 }
                Label { text: qsTr("预计成本: ") + root.previewVm.estimatedCost; color: Theme.textPrimary; font.pixelSize: 12 }

                // Per-extruder filament breakdown (对齐上游 render_all_plates_stats)
                Label {
                    text: qsTr("── 耗材用量 ──")
                    color: Theme.textSecondary
                    font.pixelSize: 11
                    font.bold: true
                    visible: root.previewVm.extruderCount() > 0
                    Layout.topMargin: 4
                }

                Repeater {
                    model: root.previewVm.extruderCount()
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        visible: root.previewVm.extruderUsedLength(index) > 0.001

                        // Color chip for extruder
                        Rectangle {
                            width: 10; height: 10; radius: 2
                            color: root.previewVm.extruderColor(index)
                        }
                        Label {
                            text: qsTr("挤出机 %1").arg(index)
                            color: Theme.textPrimary
                            font.pixelSize: 11
                        }
                        Label {
                            text: root.previewVm.extruderUsedLength(index).toFixed(2) + " m"
                            color: Theme.textPrimary
                            font.pixelSize: 10
                            font.family: "monospace"
                        }
                        Label {
                            text: root.previewVm.extruderUsedWeight(index).toFixed(1) + " g"
                            color: Theme.textSecondary
                            font.pixelSize: 10
                        }
                    }
                }

                // Per-role time breakdown (对齐上游 PrintEstimatedStatistics::roles_times)
                Label {
                    text: qsTr("── 按角色时间 ──")
                    color: Theme.textSecondary
                    font.pixelSize: 11
                    font.bold: true
                    visible: root.previewVm.roleTimeCount() > 0
                    Layout.topMargin: 4
                }

                Repeater {
                    model: root.previewVm.roleTimeCount()
                    delegate: RowLayout {
                        spacing: 6
                        visible: root.previewVm.roleTimePercent(index) >= 0.1
                        Layout.fillWidth: true

                        Label {
                            text: root.previewVm.roleTimeName(index)
                            color: Theme.textPrimary
                            font.pixelSize: 11
                            Layout.fillWidth: true
                        }
                        Label {
                            text: root.previewVm.roleTimeValue(index)
                            color: Theme.textPrimary
                            font.pixelSize: 11
                            font.family: "monospace"
                        }
                        Label {
                            text: root.previewVm.roleTimePercent(index) >= 0.1
                                  ? root.previewVm.roleTimePercent(index).toFixed(1) + "%"
                                  : "<0.1%"
                            color: Theme.textSecondary
                            font.pixelSize: 10
                            Layout.minimumWidth: 40
                            horizontalAlignment: Text.AlignRight
                        }
                    }
                }

                // Per-layer time distribution chart (对齐上游 IMSlider m_layers_times)
                Label {
                    text: qsTr("── 按层时间分布 ──")
                    color: Theme.textSecondary
                    font.pixelSize: 11
                    font.bold: true
                    visible: root.previewVm.layerTimeCount() > 1
                    Layout.topMargin: 4
                }

                // Mini bar chart
                Item {
                    Layout.fillWidth: true
                    height: 60
                    visible: root.previewVm.layerTimeCount() > 1

                    readonly property int totalLayers: root.previewVm.layerTimeCount()
                    readonly property real peakTime: root.previewVm.maxLayerTime()
                    readonly property int barCount: totalLayers > 100 ? 100 : totalLayers

                    Row {
                        anchors.fill: parent
                        spacing: 1

                        Repeater {
                            model: parent.barCount

                            Rectangle {
                                width: (parent.width - (modelData - 1)) / modelData
                                height: parent.height
                                color: "transparent"

                                readonly property real barTime: {
                                    const total = root.previewVm.layerTimeCount();
                                    if (total <= 100)
                                        return root.previewVm.layerTimeAt(index);
                                    const bucketSize = Math.ceil(total / 100);
                                    let sum = 0;
                                    const start = index * bucketSize;
                                    const end = Math.min(start + bucketSize, total);
                                    for (let i = start; i < end; ++i)
                                        sum += root.previewVm.layerTimeAt(i);
                                    return sum / (end - start);
                                }

                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    height: parent.peakTime > 0 ? Math.max(2, (parent.barTime / parent.peakTime) * parent.height) : 2
                                    radius: 1

                                    readonly property real ratio: parent.peakTime > 0 ? parent.barTime / parent.peakTime : 0
                                    color: ratio < 0.33 ? "#22c55e" : ratio < 0.66 ? "#eab308" : "#ef4444"

                                    HoverHandler {
                                        id: barHover
                                    }

                                    Rectangle {
                                        anchors.bottom: parent.top
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        width: tipCol.implicitWidth + 12
                                        height: tipCol.implicitHeight + 8
                                        visible: barHover.hovered
                                        z: 10
                                        radius: 4
                                        color: "#1e293b"
                                        border.width: 1
                                        border.color: Theme.borderSubtle

                                        Column {
                                            id: tipCol
                                            anchors.centerIn: parent
                                            spacing: 1
                                            Text {
                                                text: qsTr("层 %1-%2").arg(index * Math.ceil(root.previewVm.layerTimeCount() / 100) + 1).arg(Math.min((index + 1) * Math.ceil(root.previewVm.layerTimeCount() / 100), root.previewVm.layerTimeCount()))
                                                color: Theme.textTertiary
                                                font.pixelSize: 8
                                            }
                                            Text {
                                                text: parent.parent.parent.parent.barTime.toFixed(1) + "s"
                                                color: Theme.textPrimary
                                                font.pixelSize: 9
                                                font.family: "monospace"
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
