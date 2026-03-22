import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P10.2 -- EnableLiteModeDialog (aligns with upstream EnableLiteModeDialog)
// G-code preview lite mode toggle for low-memory systems
// Usage: EnableLiteModeDialog { id: dlg }  ->  dlg.open()

Dialog {
    id: root

    modal: true
    closePolicy: Popup.NoAutoClose
    anchors.centerIn: parent
    width: 400
    height: 280

    // Lite mode state (aligns with upstream gcode_preview_lite_mode config)
    property bool liteModeEnabled: false

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
            text: qsTr("预览模式设置")
            color: "#e2e8f5"
            font.pixelSize: 14
            font.bold: true
        }
    }

    contentItem: ColumnLayout {
        width: parent.width
        spacing: 12
        anchors.margins: 20

        // Info icon + description
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                width: 40
                height: 40
                radius: 8
                color: "#1F18C75E"

                Text {
                    anchors.centerIn: parent
                    text: "⚡"
                    font.pixelSize: 20
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Text {
                    text: qsTr("精简预览模式")
                    color: Theme.textPrimary
                    font.pixelSize: 13
                    font.bold: true
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("启用精简模式可隐藏内部填充结构，仅显示关键工具路径，显著提升预览响应速度。建议在内存较低的设备上启用。")
                    color: Theme.textTertiary
                    font.pixelSize: 11
                    wrapMode: Text.Wrap
                    lineHeight: 1.5
                }
            }
        }

        // Feature comparison
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: featureCol.implicitHeight + 16
            radius: 6
            color: "#1e2330"
            border.color: "#2e3848"
            border.width: 1

            ColumnLayout {
                id: featureCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 6

                Text {
                    text: qsTr("模式对比")
                    color: Theme.textSecondary
                    font.pixelSize: 11
                    font.bold: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text { Layout.preferredWidth: 100; text: qsTr("功能"); color: Theme.textTertiary; font.pixelSize: 10 }
                    Text { Layout.preferredWidth: 80; text: qsTr("完整模式"); color: Theme.textPrimary; font.pixelSize: 10; font.bold: true }
                    Text { Layout.fillWidth: true; text: qsTr("精简模式"); color: "#18c75e"; font.pixelSize: 10; font.bold: true }
                }

                Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: "#2e3848" }

                Repeater {
                    model: [
                        { feature: qsTr("外壳轮廓"), full: "✓", lite: "✓" },
                        { feature: qsTr("内部填充"), full: "✓", lite: "✗" },
                        { feature: qsTr("支撑结构"), full: "✓", lite: "✗" },
                        { feature: qsTr("工具路径"), full: "✓", lite: "✓" },
                        { feature: qsTr("移动路径"), full: "✓", lite: "✗" }
                    ]

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text { Layout.preferredWidth: 100; text: modelData.feature; color: Theme.textSecondary; font.pixelSize: 10 }
                        Text { Layout.preferredWidth: 80; text: modelData.full; color: Theme.textPrimary; font.pixelSize: 10 }
                        Text { Layout.fillWidth: true; text: modelData.lite; color: modelData.lite === "✓" ? "#18c75e" : "#EF4444"; font.pixelSize: 10 }
                    }
                }
            }
        }

        // Toggle
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            CxCheckBox {
                text: qsTr("启用精简预览模式")
                font.pixelSize: 12
                checked: root.liteModeEnabled
                onCheckedChanged: root.liteModeEnabled = checked
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("可在设置中随时切换")
                color: Theme.textTertiary
                font.pixelSize: 10
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
