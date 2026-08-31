import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// Firmware OTA is unavailable until a device protocol implementation exists.
CxDialog {
    id: root

    closePolicy: Popup.NoAutoClose
    dialogTitle: qsTr("固件升级")

    anchors.centerIn: parent
    width: 460
    height: 250

    contentItem: ColumnLayout {
        width: root.width
        spacing: Theme.spacingLG
        anchors.margins: Theme.spacingXXL

        Text {
            Layout.fillWidth: true
            text: qsTr("固件更新当前不可用")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeLG
            font.bold: true
            wrapMode: Text.Wrap
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("此版本尚未集成打印机固件检查或 OTA 升级协议，因此无法显示版本信息或执行固件更新。")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            wrapMode: Text.Wrap
            lineHeight: 1.4
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
            anchors.rightMargin: Theme.spacingXL
            Item { Layout.fillWidth: true }

            CxButton {
                text: qsTr("关闭")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.close()
            }
        }
    }
}
