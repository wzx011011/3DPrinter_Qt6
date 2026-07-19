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
    height: 360
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

    signal connectRequested(string ip, string accessCode, int port)

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
                font.pixelSize: 28
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingXS
                Text {
                    text: deviceName.length > 0
                          ? qsTr("连接到 %1").arg(deviceName)
                          : qsTr("连接 Bambu 打印机")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXL
                    font.bold: true
                }
                Text {
                    text: qsTr("输入局域网访问码以建立 MQTT 连接")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                }
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
                placeholderText: qsTr("在打印机屏幕：设置 > 网络 > 局域网访问码")
                selectByMouse: true
                // access code 通常是 8 位数字（带空格），允许字母数字空格
                validator: RegularExpressionValidator {
                    regularExpression: /[A-Za-z0-9 ]{0,12}/
                }
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
            CheckBox {
                id: advancedToggle
                text: qsTr("高级")
                checked: false
            }
            Item { Layout.fillWidth: true }

            CxButton {
                text: qsTr("取消")
                onClicked: {
                    root.accepted_ = false;
                    root.reject();
                }
            }
            CxButton {
                text: qsTr("连接")
                highlighted: true
                enabled: ipField.text.trim().length > 0 &&
                         accessCodeField.text.trim().length > 0
                onClicked: {
                    root.accessCode = accessCodeField.text.trim();
                    root.mqttPort = parseInt(portField.text) || 8883;
                    root.accepted_ = true;
                    root.connectRequested(ipField.text.trim(), root.accessCode, root.mqttPort);
                    root.accept();
                }
            }
        }
    }
}
