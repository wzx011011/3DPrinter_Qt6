import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// TroubleshootDialog.qml — UI-04 设备排错（对齐上游 TroubleshootDialog）
//
// 上游: 设备连接问题诊断 + 常见问题排查
// OWzx 占位: 显示排错步骤列表（真实诊断需 DeviceService 真实化, v2.4+）
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    dialogTitle: qsTr("设备排错")
    width: 480
    height: 400
    padding: 0

    required property var monitorVm

    contentItem: Rectangle {
        color: Theme.bgPanel
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            Text {
                Layout.fillWidth: true
                text: qsTr("设备连接问题排查（按顺序检查）：")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                font.bold: true
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ColumnLayout {
                    width: parent.width
                    spacing: 8

                    Repeater {
                        model: [
                            { step: "1", title: qsTr("检查设备电源"), desc: qsTr("确保打印机已开机且启动完成") },
                            { step: "2", title: qsTr("检查网络连接"), desc: qsTr("确保打印机和电脑在同一局域网") },
                            { step: "3", title: qsTr("检查 IP 地址"), desc: qsTr("在打印机设置中查看 IP，确认可 ping 通") },
                            { step: "4", title: qsTr("检查访问码"), desc: qsTr("Bambu 打印机需在设置中启用局域网访问码") },
                            { step: "5", title: qsTr("防火墙设置"), desc: qsTr("确保防火墙未阻止 MQTT(8883)/lan 通信") },
                            { step: "6", title: qsTr("固件版本"), desc: qsTr("确保打印机固件为最新版本") }
                        ]
                        delegate: Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 50
                            radius: 6
                            color: Theme.bgInset
                            border.width: 1
                            border.color: Theme.borderSubtle

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 10

                                Rectangle {
                                    width: 24; height: 24; radius: 12
                                    color: Theme.accent
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.step
                                        color: Theme.textOnAccent
                                        font.pixelSize: Theme.fontSizeSM
                                        font.bold: true
                                    }
                                }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Text {
                                        text: modelData.title
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeSM
                                        font.bold: true
                                    }
                                    Text {
                                        text: modelData.desc
                                        color: Theme.textSecondary
                                        font.pixelSize: Theme.fontSizeXS
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("关闭")
                    onClicked: root.reject()
                }
            }
        }
    }
}
