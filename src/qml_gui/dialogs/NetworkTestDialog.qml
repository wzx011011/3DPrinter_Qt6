import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// NetworkTestDialog.qml — UI-03 网络测试（对齐上游 NetworkTestDialog）
//
// 上游: 测试设备连接性（MQTT/lan/cloud）
// OWzx 占位: 显示测试步骤 + 结果（真实测试需 NetworkService 真实化, v2.4+）
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    title: qsTr("网络测试")
    width: 420
    height: 320
    padding: 0

    required property var networkVm

    contentItem: Rectangle {
        color: Theme.bgPanel
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            Text {
                Layout.fillWidth: true
                text: qsTr("测试与打印机的网络连接性：")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
            }

            // 测试项列表（占位）
            Repeater {
                model: [
                    { name: qsTr("局域网发现 (SSDP)"), status: qsTr("待实现") },
                    { name: qsTr("MQTT 连接"), status: qsTr("待实现") },
                    { name: qsTr("云端连通性"), status: qsTr("待实现") },
                    { name: qsTr("DNS 解析"), status: qsTr("待实现") }
                ]
                delegate: RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        text: modelData.name
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.fillWidth: true
                    }
                    Text {
                        text: modelData.status
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeSM
                    }
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("关闭")
                    onClicked: root.reject()
                }
                CxButton {
                    text: qsTr("开始测试")
                    cxStyle: CxButton.Style.Primary
                    enabled: false  // TODO: NetworkService 真实化后启用
                }
            }
        }
    }
}
