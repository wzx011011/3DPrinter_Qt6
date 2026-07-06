import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

Item {
    id: root
    required property var previewVm

    implicitHeight: statsLayout.implicitHeight

    ColumnLayout {
        id: statsLayout
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 6

        Label {
            text: qsTr("统计")
            color: Theme.textPrimary
            font.bold: true
            font.pixelSize: Theme.fontSizeMD
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: qsTr("标准")
                color: root.previewVm && !root.previewVm.stealthMode ? Theme.accentLight : Theme.textTertiary
                font.pixelSize: Theme.fontSizeSM
                font.bold: root.previewVm && !root.previewVm.stealthMode
            }

            CxSwitch {
                checked: root.previewVm ? root.previewVm.stealthMode : false
                onToggled: if (root.previewVm) root.previewVm.setStealthMode(checked)
            }

            Text {
                text: qsTr("静音")
                color: root.previewVm && root.previewVm.stealthMode ? Theme.accentLight : Theme.textTertiary
                font.pixelSize: Theme.fontSizeSM
                font.bold: root.previewVm && root.previewVm.stealthMode
            }

            Item { Layout.fillWidth: true }
        }

        ToggleRow {
            label: qsTr("显示空驶")
            checked: root.previewVm ? root.previewVm.showTravelMoves : false
            onChanged: if (root.previewVm) root.previewVm.setShowTravelMoves(checked)
        }

        ToggleRow {
            label: qsTr("显示热床")
            checked: root.previewVm ? root.previewVm.showBed : false
            onChanged: if (root.previewVm) root.previewVm.setShowBed(checked)
        }

        ToggleRow {
            label: qsTr("显示工具位置")
            checked: root.previewVm ? root.previewVm.showMarker : false
            onChanged: if (root.previewVm) root.previewVm.setShowMarker(checked)
        }

        Rectangle {
            Layout.fillWidth: true
            radius: 5
            color: "#24272e"
            border.width: 1
            border.color: Theme.borderSubtle
            implicitHeight: statValues.implicitHeight + 16

            ColumnLayout {
                id: statValues
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 8
                spacing: 5

                StatRow { label: qsTr("总时间"); value: root.previewVm ? root.previewVm.totalTime : "--:--:--" }
                StatRow { label: qsTr("层数"); value: root.previewVm ? String(root.previewVm.layerCount) : "0" }
                StatRow { label: qsTr("移动"); value: root.previewVm ? String(root.previewVm.moveCount) : "0" }
                StatRow { label: qsTr("挤出移动"); value: root.previewVm ? String(root.previewVm.extrudeMoveCount) : "0" }
                StatRow { label: qsTr("空驶移动"); value: root.previewVm ? String(root.previewVm.travelMoveCount) : "0" }
                StatRow { label: qsTr("耗材长度"); value: root.previewVm ? root.previewVm.filamentUsed : "--" }
                StatRow { label: qsTr("耗材重量"); value: root.previewVm ? root.previewVm.filamentWeight : "--" }
                StatRow { label: qsTr("平均速度"); value: root.previewVm ? root.previewVm.avgSpeed : "--" }
                StatRow { label: qsTr("工具切换"); value: root.previewVm ? String(root.previewVm.toolChangeCount) : "0" }
                StatRow { label: qsTr("预计成本"); value: root.previewVm ? root.previewVm.estimatedCost : "--" }

                Label {
                    visible: root.previewVm && root.previewVm.extruderCount() > 0
                    text: qsTr("耗材用量")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                    Layout.topMargin: 3
                }

                Repeater {
                    model: root.previewVm ? root.previewVm.extruderCount() : 0
                    delegate: RowLayout {
                        id: extruderRow
                        required property int index
                        Layout.fillWidth: true
                        spacing: 6
                        visible: root.previewVm && root.previewVm.extruderUsedLength(extruderRow.index) > 0.001

                        Rectangle {
                            Layout.preferredWidth: 10
                            Layout.preferredHeight: 10
                            radius: 2
                            color: root.previewVm ? root.previewVm.extruderColor(extruderRow.index) : Theme.accent
                        }
                        Label {
                            Layout.fillWidth: true
                            text: qsTr("挤出机 %1").arg(extruderRow.index + 1)
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSM
                            elide: Text.ElideRight
                        }
                        Label {
                            text: root.previewVm ? root.previewVm.extruderUsedLength(extruderRow.index).toFixed(2) + " m" : "0.00 m"
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXS
                            font.family: "Consolas"
                        }
                        Label {
                            text: root.previewVm ? root.previewVm.extruderUsedWeight(extruderRow.index).toFixed(1) + " g" : "0.0 g"
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeXS
                        }
                    }
                }

                Label {
                    visible: root.previewVm && root.previewVm.roleTimeCount() > 0
                    text: qsTr("按类型耗时")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                    Layout.topMargin: 3
                }

                Repeater {
                    model: root.previewVm ? Math.min(root.previewVm.roleTimeCount(), 8) : 0
                    delegate: RowLayout {
                        id: roleTimeRow
                        required property int index
                        Layout.fillWidth: true
                        spacing: 6
                        visible: root.previewVm && root.previewVm.roleTimePercent(roleTimeRow.index) >= 0.1

                        Label {
                            Layout.fillWidth: true
                            text: root.previewVm ? root.previewVm.roleTimeName(roleTimeRow.index) : ""
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeSM
                            elide: Text.ElideRight
                        }
                        Label {
                            text: root.previewVm ? root.previewVm.roleTimeValue(roleTimeRow.index) : ""
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXS
                            font.family: "Consolas"
                        }
                        Label {
                            Layout.preferredWidth: 44
                            text: root.previewVm ? root.previewVm.roleTimePercent(roleTimeRow.index).toFixed(1) + "%" : "0%"
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeXS
                            horizontalAlignment: Text.AlignRight
                        }
                    }
                }

                Label {
                    visible: root.previewVm && root.previewVm.layerTimeCount() > 1
                    text: qsTr("层时间分布")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                    Layout.topMargin: 3
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: root.previewVm && root.previewVm.layerTimeCount() > 1

                    StatPill { label: qsTr("最短"); value: root.previewVm ? root.previewVm.minLayerTime().toFixed(1) + "s" : "--" }
                    StatPill { label: qsTr("平均"); value: root.previewVm ? root.previewVm.avgLayerTime().toFixed(1) + "s" : "--" }
                    StatPill { label: qsTr("最长"); value: root.previewVm ? root.previewVm.maxLayerTime().toFixed(1) + "s" : "--" }
                }
            }
        }
    }

    component ToggleRow: RowLayout {
        id: toggleRoot
        property string label: ""
        property bool checked: false
        signal changed(bool checked)

        Layout.fillWidth: true
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: toggleRoot.label
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            elide: Text.ElideRight
        }
        CxSwitch {
            checked: toggleRoot.checked
            onToggled: toggleRoot.changed(checked)
        }
    }

    component StatRow: RowLayout {
        id: statRowRoot
        property string label: ""
        property string value: ""

        Layout.fillWidth: true
        spacing: 8

        Label {
            Layout.fillWidth: true
            text: statRowRoot.label
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            elide: Text.ElideRight
        }
        Label {
            text: statRowRoot.value
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            font.family: "Consolas"
            horizontalAlignment: Text.AlignRight
        }
    }

    component StatPill: Rectangle {
        id: pillRoot
        property string label: ""
        property string value: ""

        Layout.fillWidth: true
        height: 28
        radius: 4
        color: "#1c2027"
        border.width: 1
        border.color: Theme.borderSubtle

        Column {
            anchors.centerIn: parent
            spacing: 1
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: pillRoot.label
                color: Theme.textTertiary
                font.pixelSize: 9
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: pillRoot.value
                color: Theme.textPrimary
                font.pixelSize: 10
                font.family: "Consolas"
            }
        }
    }
}
