import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// C5 — 切片进度面板（绑定真实 SliceService 状态）
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
    readonly property bool hasSliceResult: root.editorVm ? root.editorVm.hasSliceResult : false
    readonly property bool canRequestSlice: root.editorVm ? root.editorVm.canRequestSlice : false
    readonly property string actionLabel: root.editorVm ? root.editorVm.sliceActionLabel : qsTr("▶ 开始切片")
    readonly property string actionHint: root.editorVm ? root.editorVm.sliceActionHint : ""

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

        // ── 估算用时（完成后显示）────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: infoColumn.implicitHeight + 20
            radius: 4
            color: "#1e2229"
            visible: root.hasSliceResult

            ColumnLayout {
                id: infoColumn
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: qsTr("预计打印时长")
                        color: "#9daaba"
                        font.pixelSize: 11
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.estimatedTime.length > 0 ? root.estimatedTime : qsTr("待补充")
                        color: "#22c564"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: root.resultPlateLabel.length > 0

                    Text {
                        text: qsTr("当前平板")
                        color: "#9daaba"
                        font.pixelSize: 11
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.resultPlateLabel
                        color: "#dde4ef"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: root.resultWeight.length > 0

                    Text {
                        text: qsTr("耗材重量")
                        color: "#9daaba"
                        font.pixelSize: 11
                        Layout.fillWidth: true
                    }
                    Text {
                        text: root.resultWeight
                        color: "#dde4ef"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    visible: root.outputPath.length > 0

                    Text {
                        text: qsTr("输出文件")
                        color: "#9daaba"
                        font.pixelSize: 11
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.outputPath
                        color: "#dde4ef"
                        font.pixelSize: 11
                        wrapMode: Text.WrapAnywhere
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: hintText.implicitHeight + 18
            radius: 4
            color: root.canRequestSlice ? "#192128" : "#241d1d"
            border.color: root.canRequestSlice ? "#2f4c5f" : "#5d2f2f"

            Text {
                id: hintText
                anchors.fill: parent
                anchors.margins: 9
                text: root.actionHint
                color: root.canRequestSlice ? "#9daaba" : "#e7b4b4"
                font.pixelSize: 11
                wrapMode: Text.Wrap
            }
        }

        // ── 切片 / 取消按钮 ───────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 30
            radius: 4
            color: !root.slicingNow && !root.canRequestSlice ? "#3b4048"
                   : actionMA.containsMouse
                     ? (root.slicingNow ? "#7d2020" : "#19a84e")
                     : (root.slicingNow ? "#5e1818" : "#157a39")

            Text {
                anchors.centerIn: parent
                text: root.slicingNow ? qsTr("✕ 取消切片") : root.actionLabel
                color: !root.slicingNow && !root.canRequestSlice ? "#aab2bf" : "white"
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

        Item { Layout.fillHeight: true }
    }
}
