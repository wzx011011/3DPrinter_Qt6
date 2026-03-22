import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.5 -- FirmwareDialog (aligns with upstream UpgradePanel / MachineInfoPanel)
// Firmware upgrade management: version display, upgrade check, OTA progress
// Usage: FirmwareDialog { id: dlg }  ->  dlg.open()

Dialog {
    id: root

    modal: true
    closePolicy: Popup.NoAutoClose
    anchors.centerIn: parent
    width: 460
    height: 420

    // Mock firmware state (aligns with upstream MachineObject firmware info)
    property string printerModel: "CR-10 SE"
    property string serialNumber: "CP04A00XXXXXXXX"
    property string currentVersion: "01.06.00.00"
    property string latestVersion: "01.07.01.00"
    property bool isBeta: false

    // Upgrade state (aligns with upstream UpgradingState)
    enum UpgradeState {
        UpgradeAvailable,
        UpgradeNotAvailable,
        Upgrading,
        UpgradeSuccess,
        UpgradeFailed
    }

    property int upgradeState: FirmwareDialog.UpgradeState.UpgradeAvailable
    property real upgradeProgress: 0.0

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
            text: qsTr("固件升级")
            color: "#e2e8f5"
            font.pixelSize: 14
            font.bold: true
        }
    }

    contentItem: ColumnLayout {
        width: parent.width
        spacing: 12
        anchors.margins: 20

        // ── Printer Info Section ──
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: infoCol.implicitHeight + 20
            radius: 6
            color: "#1e2330"
            border.color: "#2e3848"
            border.width: 1

            ColumnLayout {
                id: infoCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 12
                spacing: 8

                // Printer model
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        Layout.preferredWidth: 90
                        text: qsTr("打印机型号")
                        color: Theme.textTertiary
                        font.pixelSize: 11
                    }
                    Text {
                        text: root.printerModel
                        color: Theme.textPrimary
                        font.pixelSize: 11
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        visible: root.isBeta
                        width: 36
                        height: 16
                        radius: 3
                        color: "#F59E0B"
                        Text {
                            anchors.centerIn: parent
                            text: "BETA"
                            color: "#000"
                            font.pixelSize: 8
                            font.bold: true
                        }
                    }
                }

                // Serial number
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        Layout.preferredWidth: 90
                        text: qsTr("序列号")
                        color: Theme.textTertiary
                        font.pixelSize: 11
                    }
                    Text {
                        text: root.serialNumber
                        color: Theme.textPrimary
                        font.pixelSize: 11
                    }
                }

                // Current version
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        Layout.preferredWidth: 90
                        text: qsTr("当前版本")
                        color: Theme.textTertiary
                        font.pixelSize: 11
                    }
                    Text {
                        text: root.currentVersion
                        color: Theme.textPrimary
                        font.pixelSize: 11
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Text {
                        visible: root.upgradeState === FirmwareDialog.UpgradeAvailable
                        text: qsTr("新版本可用: %1").arg(root.latestVersion)
                        color: "#18c75e"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }

                // Latest version (when upgrade not available)
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    visible: root.upgradeState === FirmwareDialog.UpgradeNotAvailable
                    Text {
                        Layout.preferredWidth: 90
                        text: qsTr("最新版本")
                        color: Theme.textTertiary
                        font.pixelSize: 11
                    }
                    Text {
                        text: root.latestVersion
                        color: Theme.textSecondary
                        font.pixelSize: 11
                    }
                }
            }
        }

        // ── Release Notes ──
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 6
            color: "#161b24"
            border.color: "#2e3848"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                Text {
                    text: qsTr("更新日志")
                    color: Theme.textSecondary
                    font.pixelSize: 11
                    font.bold: true
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: availableWidth

                    Text {
                        width: parent.width
                        wrapMode: Text.Wrap
                        text: qsTr("v%1 更新内容:\n\n1. 优化切片算法，提升打印质量\n2. 修复 AMS 多耗材切换偶尔失败的问题\n3. 新增 timelapse 视频录制优化\n4. 改善 Wi-Fi 连接稳定性\n5. 修复部分情况下热床温度显示异常").arg(root.latestVersion)
                        color: Theme.textTertiary
                        font.pixelSize: 10
                        lineHeight: 1.5
                    }
                }
            }
        }

        // ── Progress Bar (visible during upgrade) ──
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: root.upgradeState === FirmwareDialog.UpgradeState.Upgrading

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                    text: qsTr("正在升级...")
                    color: Theme.accent
                    font.pixelSize: 11
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: Math.round(root.upgradeProgress * 100) + "%"
                    color: Theme.accent
                    font.pixelSize: 11
                    font.bold: true
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 8
                radius: 4
                color: "#1e2330"
                border.color: "#2e3848"
                border.width: 1

                Rectangle {
                    width: parent.width * root.upgradeProgress
                    height: parent.height
                    radius: 4
                    color: Theme.accent
                }
            }

            Text {
                text: qsTr("升级过程中请勿断开电源或网络连接")
                color: "#F59E0B"
                font.pixelSize: 10
            }
        }

        // ── Status message (success/fail) ──
        RowLayout {
            Layout.fillWidth: true
            visible: root.upgradeState === FirmwareDialog.UpgradeState.UpgradeSuccess
               || root.upgradeState === FirmwareDialog.UpgradeState.UpgradeFailed

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 30
                radius: 4
                color: root.upgradeState === FirmwareDialog.UpgradeState.UpgradeSuccess
                    ? "#1F18C75E" : "#1FEF4444"
                border.color: root.upgradeState === FirmwareDialog.UpgradeState.UpgradeSuccess
                    ? "#18c75e" : "#EF4444"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: root.upgradeState === FirmwareDialog.UpgradeState.UpgradeSuccess
                        ? qsTr("升级成功！打印机将自动重启。")
                        : qsTr("升级失败，请检查网络连接后重试。")
                    color: root.upgradeState === FirmwareDialog.UpgradeState.UpgradeSuccess
                        ? "#18c75e" : "#EF4444"
                    font.pixelSize: 11
                    font.bold: true
                }
            }
        }
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

            // Mock upgrade simulation button
            CxButton {
                visible: root.upgradeState === FirmwareDialog.UpgradeState.UpgradeAvailable
                text: qsTr("开始升级")
                cxStyle: CxButton.Style.Primary
                onClicked: simulateUpgrade()
            }

            CxButton {
                visible: root.upgradeState === FirmwareDialog.UpgradeState.UpgradeFailed
                text: qsTr("重试")
                cxStyle: CxButton.Style.Primary
                onClicked: {
                    root.upgradeState = FirmwareDialog.UpgradeState.UpgradeAvailable
                }
            }

            CxButton {
                text: qsTr("关闭")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.close()
            }
        }
    }

    // Mock upgrade simulation
    function simulateUpgrade() {
        root.upgradeState = FirmwareDialog.UpgradeState.Upgrading
        root.upgradeProgress = 0.0
        upgradeTimer.start()
    }

    Timer {
        id: upgradeTimer
        interval: 100
        repeat: true
        onTriggered: {
            root.upgradeProgress += 0.02
            if (root.upgradeProgress >= 1.0) {
                root.upgradeProgress = 1.0
                stop()
                root.upgradeState = FirmwareDialog.UpgradeState.UpgradeSuccess
            }
        }
    }

    onClosed: {
        upgradeTimer.stop()
        root.upgradeProgress = 0.0
        root.upgradeState = FirmwareDialog.UpgradeState.UpgradeAvailable
    }
}
