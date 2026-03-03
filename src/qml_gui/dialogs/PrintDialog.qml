import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// D2 — PrintDialog：发送打印 / 导出 G-code
// 用法：PrintDialog { id: printDlg; editorVm: ... }
// 触发：printDlg.open()
Dialog {
    id: root
    required property var editorVm

    title: ""
    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // 居中于父窗口
    anchors.centerIn: parent

    width:  480
    height: contentCol.implicitHeight + 80

    // ── 背景样式 ──────────────────────────────────────────────────
    background: Rectangle {
        color: "#1a1f28"
        radius: 8
        border.color: "#2e3848"
        border.width: 1
        layer.enabled: true
        layer.effect: null   // 避免引入额外模块
    }

    // ── 自定义标题栏 ─────────────────────────────────────────────
    header: Rectangle {
        width: parent.width
        height: 44
        color: "#141920"
        radius: 8

        // 只有上角圆角，下边直角
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 12
            color: parent.color
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 12
            spacing: 10

            Text {
                text: qsTr("🖨  发送打印")
                color: "#e2e8f5"
                font.pixelSize: 14
                font.bold: true
            }
            Item { Layout.fillWidth: true }

            Rectangle {
                width: 24; height: 24; radius: 4
                color: closeHov.containsMouse ? "#3d2020" : "transparent"
                Text {
                    anchors.centerIn: parent
                    text: "✕"; color: "#7a8fa3"; font.pixelSize: 12
                }
                MouseArea {
                    id: closeHov; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }
    }

    // ── 内容区 ────────────────────────────────────────────────────
    contentItem: ColumnLayout {
        id: contentCol
        width: root.width - 32
        spacing: 14

        // 项目信息行
        Rectangle {
            Layout.fillWidth: true
            height: 44
            radius: 5
            color: "#111620"
            border.color: "#2e3848"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 10

                Text { text: "📄"; font.pixelSize: 16 }
                ColumnLayout {
                    spacing: 2
                    Text {
                        text: root.editorVm ? root.editorVm.projectName : "—"
                        color: "#dde4ef"; font.pixelSize: 12; font.bold: true
                    }
                    Text {
                        text: root.editorVm
                              ? root.editorVm.objectCount + qsTr(" 个对象  ·  层高 0.20 mm")
                              : "—"
                        color: "#566070"; font.pixelSize: 10
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }

        // 文件输出路径
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text { text: qsTr("输出路径"); color: "#9daaba"; font.pixelSize: 11; Layout.preferredWidth: 60 }

            Rectangle {
                Layout.fillWidth: true
                height: 28; radius: 4
                color: "#0f1318"
                border.color: pathField.activeFocus ? "#22c564" : "#2e3848"

                TextInput {
                    id: pathField
                    anchors.fill: parent
                    anchors.leftMargin: 8; anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    verticalAlignment: TextInput.AlignVCenter
                    text: "C:/Users/Output/print_job.gcode"
                    color: "#c8d4e0"
                    font.pixelSize: 11
                    selectByMouse: true
                }
            }

            Rectangle {
                width: 26; height: 28; radius: 4
                color: browseHov.containsMouse ? "#2e3848" : "#1e2535"
                Text { anchors.centerIn: parent; text: "📂"; font.pixelSize: 12 }
                MouseArea {
                    id: browseHov; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                }
            }
        }

        // 快速选项
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            CheckBox {
                text: qsTr("切片后自动发送")
                checked: true
                contentItem: Text { text: parent.text; color: "#9daaba"; font.pixelSize: 11; leftPadding: parent.indicator.width + 6 }
            }
            CheckBox {
                text: qsTr("完成后通知")
                checked: false
                contentItem: Text { text: parent.text; color: "#9daaba"; font.pixelSize: 11; leftPadding: parent.indicator.width + 6 }
            }
        }

        // 分割线
        Rectangle { Layout.fillWidth: true; height: 1; color: "#1e2535" }

        // 按钮行
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            // 导出 G-code
            Rectangle {
                width: 110; height: 30; radius: 4
                color: exportHov.containsMouse ? "#2e3848" : "#1e2840"
                border.color: "#2e4060"
                Text { anchors.centerIn: parent; text: qsTr("导出 G-code"); color: "#8eaad0"; font.pixelSize: 11 }
                MouseArea {
                    id: exportHov; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: { root.close() }
                }
            }

            // 取消
            Rectangle {
                width: 60; height: 30; radius: 4
                color: cancelHov.containsMouse ? "#2e2e2e" : "#232830"
                Text { anchors.centerIn: parent; text: qsTr("取消"); color: "#9daaba"; font.pixelSize: 11 }
                MouseArea {
                    id: cancelHov; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }

            // 打印
            Rectangle {
                width: 80; height: 30; radius: 4
                color: printHov.containsMouse ? "#19a84e" : "#157a39"
                Text { anchors.centerIn: parent; text: qsTr("▶ 打印"); color: "white"; font.pixelSize: 11; font.bold: true }
                MouseArea {
                    id: printHov; anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        // 触发切片（若未切片）
                        if (root.editorVm) root.editorVm.requestSlice()
                        root.close()
                    }
                }
            }
        }
    }
}
