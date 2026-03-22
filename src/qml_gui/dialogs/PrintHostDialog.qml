import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.6c -- PrintHostDialog (aligns with upstream PhysicalPrinterDialog)
// Print host upload configuration: host URL, API key, connection test
// Usage: PrintHostDialog { id: dlg }  ->  dlg.open()

Dialog {
    id: root

    modal: true
    closePolicy: Popup.NoAutoClose
    anchors.centerIn: parent
    width: 420
    height: 340

    // Mock print host data
    property string presetName: "Creality CR-10 SE"
    property string hostType: "Creality Cloud"
    property string hostUrl: "https://api.creality.com"
    property int hostPort: 443
    property string apiKey: ""
    property string authType: qsTr("API Key")

    background: Rectangle {
        color: "#1a1f28"
        radius: 8
        border.color: "#2e3848"
        border.width: 1
    }

    ColumnLayout {
        width: parent.width
        height: 40
        spacing: 0

        Item { Layout.fillWidth: true; Layout.fillHeight: true }

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.alignment: Qt.AlignVCenter
            text: qsTr("打印主机设置")
            color: "#e2e8f5"
            font.pixelSize: 14
            font.bold: true
        }
    }

    contentItem: ColumnLayout {
        width: parent.width
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
                font.pixelSize: 11
            }
            CxTextField {
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: 11
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
                font.pixelSize: 11
            }
            CxComboBox {
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: 11
                model: ["Creality Cloud", "OctoPrint", "PrusaLink", "Moonraker", "Klipper"]
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
                font.pixelSize: 11
            }
            CxTextField {
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: 11
                text: root.hostUrl
            }
            CxTextField {
                Layout.preferredWidth: 60
                implicitHeight: 24
                font.pixelSize: 11
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
                font.pixelSize: 11
            }
            CxTextField {
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: 11
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
                font.pixelSize: 11
            }
            CxComboBox {
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: 11
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
                font.pixelSize: 10
                text: qsTr("需要有效的网络连接")
            }
        }

        Item { Layout.fillHeight: true }
    }

    footer: Rectangle {
        width: parent.width
        height: 48
        color: "#141920"
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
