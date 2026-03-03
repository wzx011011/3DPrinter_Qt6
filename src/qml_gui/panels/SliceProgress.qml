import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// C5 — 切片进度面板（绑定 EditorViewModel.sliceProgress / isSlicing）
Item {
    id: root
    required property var editorVm

    readonly property int  pct:       root.editorVm ? root.editorVm.sliceProgress() : 0
    readonly property bool slicingNow: root.editorVm ? root.editorVm.isSlicing()    : false

    // 刷新绑定（stateChanged 触发）
    Connections {
        target: root.editorVm
        function onStateChanged() {
            progressBar.value = root.editorVm ? root.editorVm.sliceProgress() : 0
        }
    }

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
                    text: root.slicingNow ? qsTr("切片进行中...") : (root.pct >= 100 ? qsTr("切片完成") : qsTr("等待切片"))
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
            height: 36
            radius: 4
            color: "#1e2229"
            visible: root.pct >= 100 && !root.slicingNow

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                Text { text: qsTr("预计打印时长"); color: "#9daaba"; font.pixelSize: 11; Layout.fillWidth: true }
                Text { text: "01:42:16"; color: "#22c564"; font.pixelSize: 12; font.bold: true }
            }
        }

        // ── 切片 / 取消按钮 ───────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 30
            radius: 4
            color: actionMA.containsMouse
                   ? (root.slicingNow ? "#7d2020" : "#19a84e")
                   : (root.slicingNow ? "#5e1818" : "#157a39")

            Text {
                anchors.centerIn: parent
                text: root.slicingNow ? qsTr("✕ 取消切片") : qsTr("▶ 开始切片")
                color: "white"
                font.pixelSize: 12
                font.bold: true
            }

            MouseArea {
                id: actionMA
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (!root.editorVm) return
                    if (root.slicingNow) {
                        // SliceServiceMock 暂无 cancel，用 requestSlice 触发重置
                    } else {
                        root.editorVm.requestSlice()
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
