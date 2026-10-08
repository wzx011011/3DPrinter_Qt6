import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// NetworkTestDialog.qml — UI-03 网络测试（对齐上游 NetworkTestDialog）
//
// 上游: NetworkTestDialog.cpp — Basic Info（版本/系统版本 :86-129）+ 三列测试行
// （按钮|标题|值，初值 "N/A" :131-164）+ "Log Info" 多行滚动日志 txt_log
// （:176-186），EVT_UPDATE_RESULT 按时间戳追加事件并含 ip/步骤事件
// （:195-219/:246-252/:285-288）。
// OWzx: 三行（DNS/HTTPS/延迟）由 backend.runNetworkTest() 单探针驱动；每行
// 附 Regular 独立测试按钮（触发同一探针，测试中禁用）。
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    dialogTitle: qsTr("网络测试")
    width: 480
    height: 560
    padding: 0

    required property var networkVm

    // True while backend.runNetworkTest() is in flight.
    property bool testRunning: false

    // Timestamped log buffer (upstream txt_log, NetworkTestDialog.cpp:195-219:
    // each EVT_UPDATE_RESULT appends "<timestamp>:<event>\n").
    property string logText: ""

    // Live test rows driven by backend.runNetworkTest() (upstream
    // create_content_sizer :131-164: each row = 按钮|标题|值, value starts
    // at "N/A"). Declared on the dialog root: unqualified lookups from the
    // Repeater binding and startTest() resolve along the component root
    // chain and cannot see properties on intermediate Items.
    property var testRows: [
        { name: qsTr("DNS 解析"), status: "N/A", ok: false },
        { name: qsTr("云端连通性 (HTTPS)"), status: "N/A", ok: false },
        { name: qsTr("网络延迟"), status: "N/A", ok: false }
    ]

    // Basic Info source (upstream create_info_sizer :86-129 binds the studio
    // version + OS version here).
    readonly property var sysInfo: (typeof backend !== "undefined" && backend && backend.systemInfo)
        ? backend.systemInfo() : ({})

    function appendLog(message) {
        logText += Qt.formatDateTime(new Date(), "yyyy-MM-dd HH:mm:ss") + ": " + message + "\n"
    }

    function startTest() {
        testRunning = true
        var rows = testRows
        for (var i = 0; i < rows.length; ++i)
            rows[i].status = qsTr("测试中…")
        testRows = rows
        // Upstream step events (NetworkTestDialog.cpp:246-252 start_test_url).
        appendLog("test github start...")
        appendLog("[test github]: url=https://api.github.com")
        backend.runNetworkTest()
    }

    contentItem: Rectangle {
        color: Theme.bgPanel
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingXXL
            spacing: Theme.spacingMD

            Text {
                Layout.fillWidth: true
                text: qsTr("测试本机到更新服务器的网络连接性：")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
            }

            // Basic Info（upstream NetworkTestDialog.cpp:86-129）
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingXS

                Text {
                    text: qsTr("Basic Info")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSM
                    Text {
                        text: qsTr("OWzx Slicer 版本")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeSM
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.sysInfo.appVersion !== undefined ? root.sysInfo.appVersion : qsTr("不可用")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        elide: Text.ElideMiddle
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSM
                    Text {
                        text: qsTr("系统版本")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeSM
                    }
                    Text {
                        Layout.fillWidth: true
                        text: root.sysInfo.platform !== undefined ? root.sysInfo.platform : qsTr("不可用")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        elide: Text.ElideMiddle
                    }
                }
            }

            Repeater {
                model: testRows
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
                    // Per-row Regular test button (upstream rows carry a per
                    // target test Button; OWzx rows share the single backend
                    // probe channel). Upstream ButtonStyle::Regular maps to
                    // the CxButton Secondary tier.
                    CxButton {
                        text: qsTr("测试")
                        cxStyle: CxButton.Style.Secondary
                        enabled: !root.testRunning
                        onClicked: root.startTest()
                    }
                }
            }

            // Log Info（upstream create_result_sizer :176-186: title +
            // multiline wxTextCtrl）
            Text {
                text: qsTr("日志信息")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.bgSurface
                radius: Theme.radiusLG
                border.color: Theme.borderInput
                border.width: 1

                CxScrollView {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingSM
                    clip: true

                    TextArea {
                        id: logArea
                        readOnly: true
                        wrapMode: TextArea.Wrap
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: Theme.fontMono
                        text: root.logText
                        background: null
                        persistentSelection: false
                        onTextChanged: {
                            if (logArea.cursorPosition === logArea.length)
                                logArea.cursorPosition = logArea.length
                        }
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
                    onClicked: root.startTest()
                }
            }

            Connections {
                target: backend
                function onNetworkTestFinished(dnsOk, online, latencyMs, detail) {
                    root.testRunning = false
                    var rows = testRows
                    rows[0].status = dnsOk ? qsTr("正常") : qsTr("失败")
                    rows[0].ok = dnsOk
                    rows[1].status = online ? qsTr("正常") : qsTr("失败")
                    rows[1].ok = online
                    rows[2].status = latencyMs >= 0 ? latencyMs + " ms" : qsTr("失败")
                    rows[2].ok = online
                    testRows = rows
                    // Upstream step + ip events appended to txt_log
                    // (NetworkTestDialog.cpp:195-219/:285-288; the resolved ip
                    // and HTTP status travel inside the backend detail).
                    root.appendLog(online ? "test github ok" : "test github failed")
                    root.appendLog(detail)
                    networkTestDetail.text = detail
                    networkTestDetail.color = online ? Theme.statusSuccess : Theme.statusError
                }
            }
        }
    }
}
