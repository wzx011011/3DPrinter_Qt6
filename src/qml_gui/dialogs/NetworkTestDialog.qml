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
    dialogTitle: qsTr("网络测试")
    width: 420
    height: 320
    padding: 0

    required property var networkVm

    // True while backend.runNetworkTest() is in flight.
    property bool testRunning: false

    contentItem: Rectangle {
        color: Theme.bgPanel
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingXXL
            spacing: Theme.spacingLG
            Text {
                Layout.fillWidth: true
                text: qsTr("测试本机到更新服务器的网络连接性：")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
            }

            // Live test rows driven by backend.runNetworkTest() (upstream
            // NetworkTestDialog connectivity probe).
            property var testRows: [
                { name: qsTr("DNS 解析"), status: qsTr("未测试"), ok: false },
                { name: qsTr("云端连通性 (HTTPS)"), status: qsTr("未测试"), ok: false },
                { name: qsTr("网络延迟"), status: qsTr("未测试"), ok: false }
            ]

            Repeater {
                model: root.testRows
                delegate: RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMD
                    Text {
                        text: modelData.name
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.fillWidth: true
                    }
                    Text {
                        text: modelData.status
                        color: modelData.ok ? Theme.statusSuccess : Theme.textTertiary
                        font.pixelSize: Theme.fontSizeSM
                    }
                }
            }

            Text {
                id: networkTestDetail
                Layout.fillWidth: true
                text: ""
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.WordWrap
                visible: text !== ""
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
                    text: root.testRunning ? qsTr("测试中…") : qsTr("开始测试")
                    cxStyle: CxButton.Style.Primary
                    enabled: !root.testRunning
                    onClicked: {
                        root.testRunning = true
                        networkTestDetail.text = ""
                        var rows = root.testRows
                        for (var i = 0; i < rows.length; ++i)
                            rows[i].status = qsTr("测试中…")
                        root.testRows = rows
                        backend.runNetworkTest()
                    }
                }
            }

            Connections {
                target: backend
                function onNetworkTestFinished(dnsOk, online, latencyMs, detail) {
                    root.testRunning = false
                    var rows = root.testRows
                    rows[0].status = dnsOk ? qsTr("正常") : qsTr("失败")
                    rows[0].ok = dnsOk
                    rows[1].status = online ? qsTr("正常") : qsTr("失败")
                    rows[1].ok = online
                    rows[2].status = latencyMs >= 0 ? latencyMs + " ms" : qsTr("失败")
                    rows[2].ok = online
                    root.testRows = rows
                    networkTestDetail.text = detail
                    networkTestDetail.color = online ? Theme.statusSuccess : Theme.statusError
                }
            }
        }
    }
}
