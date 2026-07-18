import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.6c -- PrintHostDialog (aligns with upstream PhysicalPrinterDialog)
// Print host upload configuration: host URL, API key, connection test
// Usage: PrintHostDialog { id: dlg }  ->  dlg.open()

CxDialog {
    id: root

    closePolicy: Popup.NoAutoClose

    dialogTitle: qsTr("打印主机设置")

    anchors.centerIn: parent
    width: 420
    height: 340

    // Mock print host data
    property string presetName: "OrcaSlicer Default"
    property string hostType: "OrcaSlicer Cloud"
    property string hostUrl: "https://api.orcaslicer.org"
    property int hostPort: 443
    property string apiKey: ""
    property string authType: qsTr("API Key")

    contentItem: ColumnLayout {
        width: root.width
        spacing: 10
        anchors.margins: 16

        // Preset name
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.preferredWidth: 80
                text: qsTr("预设名称")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeSM
            }
            CxTextField {
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
                text: root.presetName
            }
        }

        // Host type
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.preferredWidth: 80
                text: qsTr("主机类型")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeSM
            }
            CxComboBox {
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
                model: ["OrcaSlicer Cloud", "OctoPrint", "PrusaLink", "Moonraker", "Klipper"]
                currentIndex: 0
            }
        }

        // Host URL + port
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.preferredWidth: 80
                text: qsTr("主机地址")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeSM
            }
            CxTextField {
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
                text: root.hostUrl
            }
            CxTextField {
                Layout.preferredWidth: 60
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
                text: root.hostPort.toString()
            }
        }

        // API Key
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.preferredWidth: 80
                text: qsTr("API Key")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeSM
            }
            CxTextField {
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
                text: root.apiKey
                echoMode: TextInput.Password
            }
        }

        // Auth type
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Text {
                Layout.preferredWidth: 80
                text: qsTr("认证方式")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeSM
            }
            CxComboBox {
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
                model: [qsTr("API Key"), qsTr("用户名/密码"), qsTr("无认证")]
                currentIndex: 0
            }
        }

        // Test connection button
        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.preferredWidth: 80 }

            CxButton {
                text: qsTr("测试连接")
                cxStyle: CxButton.Style.Secondary
                compact: true
                enabled: false
            }

            Text {
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
                text: qsTr("需要有效的网络连接")
            }
        }

        Item { Layout.fillHeight: true }
    }

    footer: Rectangle {
        width: parent.width
        height: 48
        color: Theme.bgSurface
        radius: 8
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 12
            color: parent.color
        }

        RowLayout {
            anchors.fill: parent
            anchors.rightMargin: 16
            spacing: 10

            Item { Layout.fillWidth: true }

            CxButton {
                text: qsTr("确定")
                cxStyle: CxButton.Style.Primary
                onClicked: root.accept()
            }

            CxButton {
                text: qsTr("取消")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }
        }
    }
}
