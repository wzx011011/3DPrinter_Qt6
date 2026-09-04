import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// SelectMachineDialog.qml — DEV-04 选择打印机发送 G-code
//
// 上游: SelectMachinePop / SelectMachineDialog
// 功能: 列出已连接设备 → 选择目标 → MQTT 发送打印任务
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    dialogTitle: qsTr("选择打印机")
    width: 480
    height: 400
    padding: 0

    required property var deviceVm   // MonitorViewModel
    required property string gcodePath  // 要发送的 G-code 路径

    contentItem: Rectangle {
        color: Theme.bgPanel
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingXXL
            spacing: Theme.spacingLG
            Text {
                Layout.fillWidth: true
                text: qsTr("选择目标打印机发送 G-code：")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
            }

            // 设备列表
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 6
                color: Theme.bgInset
                border.width: 1
                border.color: Theme.borderSubtle

                ListView {
                    id: deviceList
                    anchors.fill: parent
                    anchors.margins: Theme.spacingMD
                    clip: true
                    model: root.deviceVm ? root.deviceVm.filteredDeviceCount : 0
                    delegate: Rectangle {
                        width: deviceList.width
                        height: 56
                        radius: 6
                        color: deviceMa.containsMouse ? Theme.bgHover : "transparent"
                        border.width: index === deviceList.currentIndex ? 2 : 0
                        border.color: Theme.accent

                        property var devData: root.deviceVm ? root.deviceVm.deviceAt(index) : ({})

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: Theme.spacingMD
                            spacing: Theme.spacingXS
                            RowLayout {
                                spacing: Theme.spacingMD
                                Text {
                                    text: devData.name || devData.model || "Unknown"
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeMD
                                    font.bold: true
                                }
                                Text {
                                    text: devData.online ? qsTr("● 在线") : qsTr("● 离线")
                                    color: devData.online ? Theme.statusSuccess : Theme.textTertiary
                                    font.pixelSize: Theme.fontSizeXS
                                }
                            }
                            Text {
                                text: devData.ip || qsTr("IP: 未知")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeXS
                            }
                        }

                        MouseArea {
                            id: deviceMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: deviceList.currentIndex = index
                        }
                    }
                }
            }

            // G-code 路径显示
            Text {
                Layout.fillWidth: true
                text: qsTr("G-code: %1").arg(root.gcodePath || qsTr("（未选择）"))
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
                elide: Text.ElideRight
            }

            // 按钮区
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                CxButton {
                    text: qsTr("取消")
                    onClicked: root.reject()
                }
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("发送打印")
                    cxStyle: CxButton.Style.Primary
                    // R-P1.E: require an online device AND a real G-code path.
                    // Offline devices were selectable and accept() ran even
                    // without a device VM or path (silent no-op shown as
                    // success).
                    enabled: {
                        if (!root.deviceVm || root.gcodePath === "" || deviceList.currentIndex < 0)
                            return false
                        const dev = root.deviceVm.deviceAt(deviceList.currentIndex)
                        return !!dev && dev.online === true
                    }
                    onClicked: {
                        root.deviceVm.startPrint(deviceList.currentIndex, root.gcodePath)
                        root.accept()
                    }
                }
            }

            // R-P1.E: user-visible disclosure required by
            // docs/依赖与协议边界审计.md -- the device stack is mock; real
            // MQTT push to hardware is externally blocked.
            Text {
                Layout.fillWidth: true
                text: qsTr("演示模式：设备列表为本地模拟数据，发送为演示流程（真实设备推送依赖 MQTT，当前为外部阻塞项）。")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.Wrap
            }
        }
    }
}
