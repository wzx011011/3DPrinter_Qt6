import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../components"
import "../controls"

// v2.7 P2-A: Bambu LAN access code input dialog.
// Opened before connecting to a printer that has no access code set.
Dialog {
    id: root
    modal: true
    anchors.centerIn: parent
    width: 420
    // manual mode carries one extra field row; +20 over the old 420/360 to fit
    // the persistent two-line hint below the access code field (upstream
    // ConnectPrinter.cpp:53-59)
    height: manualAddMode ? 440 : 380
    padding: 0
    background: Rectangle {
        color: Theme.bgPanel
        radius: Theme.radiusLG
        border.width: 1
        border.color: Theme.borderSubtle
    }

    property string deviceIp: ""        // 预填 IP（从设备列表传入）
    property string deviceName: ""      // 设备名（显示用）
    property string accessCode: ""      // 用户输入的 access code（结果）
    property int mqttPort: 8883         // MQTT 端口（默认 8883）
    property bool accepted_: false      // 是否确认连接
    // monitor-6: manual-add form mode (name / IP / access code) opened from
    // the MonitorPage "+ 手动添加" entry.
    property bool manualAddMode: false

    signal connectRequested(string ip, string accessCode, int port)
    signal manualAddRequested(string name, string ip, string accessCode, int port)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingXXL
        spacing: Theme.spacingXL
        // 标题
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingLG
            Text {
                text: "\u{1F5A5}\u{FE0F}"  // 🖥️ 打印机
                font.pixelSize: Theme.fontSizeDisplay
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingXS
                Text {
                    text: {
                        if (root.manualAddMode) return qsTr("手动添加设备")
                        if (deviceName.length > 0) return qsTr("连接到 %1").arg(deviceName)
                        return qsTr("连接 Bambu 打印机")
                    }
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXL
                    font.bold: true
                }
                Text {
                    text: root.manualAddMode
                          ? qsTr("输入设备名称、IP 与局域网访问码以加入设备列表")
                          : qsTr("输入局域网访问码以建立 MQTT 连接")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                }
            }
        }

        // monitor-6: device name (manual add only)
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSM
            visible: root.manualAddMode
            Text {
                text: qsTr("设备名称")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
            }
            CxTextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: qsTr("例如：工作室 X1C")
                selectByMouse: true
            }
        }

        // IP 输入
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSM
            Text {
                text: qsTr("打印机 IP 地址")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
            }
            CxTextField {
                id: ipField
                Layout.fillWidth: true
                text: root.deviceIp
                placeholderText: "192.168.1.100"
                selectByMouse: true
            }
        }

        // Access code 输入
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingSM
            Text {
                text: qsTr("局域网访问码")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
            }
            CxTextField {
                id: accessCodeField
                Layout.fillWidth: true
                // 占位符不含「>」：zh_CN.ts:175 对旧占位符的翻译把 XML 实体
                // 双重转义成字面 “&gt;”（qsTr 返回译文即带出），且 .ts 不归本
                // 单元所有——改写源串避开该字符后旧条目失配失效。
                placeholderText: qsTr("请输入访问码")
                selectByMouse: true
                // 上游 ConnectPrinter.cpp:36 SetMaxLength(10)
                validator: RegularExpressionValidator {
                    regularExpression: /[A-Za-z0-9 ]{0,10}/
                }
            }
            // 上游常驻两行提示（ConnectPrinter.cpp:53-59 “Settings > Network >
            // Access code” 两行 StaticText）；路径分隔用「→」规避 &gt; 转义链
            Text {
                Layout.fillWidth: true
                text: qsTr("可在打印机屏幕上找到访问码：\n设置 → 网络 → 局域网访问码")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                wrapMode: Text.WordWrap
            }
        }

        // 端口（高级，默认隐藏）
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            visible: advancedToggle.checked
            Text {
                text: qsTr("MQTT 端口")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
            }
            CxTextField {
                id: portField
                Layout.preferredWidth: 80
                text: root.mqttPort.toString()
                selectByMouse: true
                validator: IntValidator { bottom: 1; top: 65535 }
            }
            Item { Layout.fillWidth: true }
        }

        Item { Layout.fillHeight: true }

        // 按钮
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingLG
            CxCheckBox {
                id: advancedToggle
                text: qsTr("高级")
                checked: false
            }
            Item { Layout.fillWidth: true }

            CxButton {
                text: qsTr("取消")
                // 上游 Cancel 为非主色次级（MsgDialog 家族仅 OK/Confirm 主样式）；
                // 绿仅主/激活，双绿失层级
                cxStyle: CxButton.Style.Secondary
                onClicked: {
                    root.accepted_ = false;
                    root.reject();
                }
            }
            CxButton {
                text: root.manualAddMode ? qsTr("添加") : qsTr("连接")
                highlighted: true
                enabled: ipField.text.trim().length > 0 &&
                         accessCodeField.text.trim().length > 0 &&
                         (!root.manualAddMode || nameField.text.trim().length > 0)
                onClicked: {
                    root.accessCode = accessCodeField.text.trim();
                    root.mqttPort = parseInt(portField.text) || 8883;
                    root.accepted_ = true;
                    if (root.manualAddMode) {
                        root.manualAddRequested(nameField.text.trim(), ipField.text.trim(),
                                                root.accessCode, root.mqttPort);
                    } else {
                        root.connectRequested(ipField.text.trim(), root.accessCode, root.mqttPort);
                    }
                    root.accept();
                }
            }
        }
    }
}
