import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P10.1 -- PluginManagerDialog (aligns with upstream WebDownPluginDlg / plugin management)
// Plugin download, install, enable/disable management
// Usage: PluginManagerDialog { id: dlg }  ->  dlg.open()

Dialog {
    id: root

    modal: true
    closePolicy: Popup.NoAutoClose
    anchors.centerIn: parent
    width: 440
    height: 340

    // Mock plugin data (aligns with upstream plugin registry)
    property var plugins: [
        {
            name: qsTr("网络通信插件"),
            version: "1.2.0",
            description: qsTr("Bambu Lab 打印机网络通信支持"),
            installed: true,
            enabled: true,
            size: qsTr("12.5 MB"),
            status: qsTr("已安装")
        },
        {
            name: qsTr("高级支撑生成器"),
            version: "2.0.1",
            description: qsTr("基于树形结构的智能支撑生成"),
            installed: false,
            enabled: false,
            size: qsTr("8.3 MB"),
            status: qsTr("可下载")
        },
        {
            name: qsTr("AI 切片优化"),
            version: "0.9.0",
            description: qsTr("基于 AI 模型的切片参数自动优化"),
            installed: false,
            enabled: false,
            size: qsTr("45.2 MB"),
            status: qsTr("可下载")
        }
    ]

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
            text: qsTr("插件管理")
            color: "#e2e8f5"
            font.pixelSize: 14
            font.bold: true
        }
    }

    contentItem: ColumnLayout {
        width: parent.width
        spacing: 8
        anchors.margins: 16

        // Plugin list
        Repeater {
            model: root.plugins

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: pluginCol.implicitHeight + 16
                radius: 6
                color: "#1e2330"
                border.color: "#2e3848"
                border.width: 1

                ColumnLayout {
                    id: pluginCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 10
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: modelData.name
                            color: Theme.textPrimary
                            font.pixelSize: 12
                            font.bold: true
                        }

                        Rectangle {
                            width: versionText.implicitWidth + 8
                            height: 16
                            radius: 3
                            color: "#2618C75E"
                            Text {
                                id: versionText
                                anchors.centerIn: parent
                                text: "v" + modelData.version
                                color: "#18c75e"
                                font.pixelSize: 9
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            text: modelData.size
                            color: Theme.textTertiary
                            font.pixelSize: 10
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: modelData.description
                        color: Theme.textTertiary
                        font.pixelSize: 10
                        wrapMode: Text.Wrap
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        CxCheckBox {
                            text: modelData.installed ? qsTr("启用") : qsTr("安装")
                            checked: modelData.enabled
                            enabled: modelData.installed
                        }

                        Text {
                            text: modelData.status
                            color: modelData.installed ? "#18c75e" : Theme.textTertiary
                            font.pixelSize: 10
                        }

                        Item { Layout.fillWidth: true }

                        CxButton {
                            visible: !modelData.installed
                            text: qsTr("下载")
                            cxStyle: CxButton.Style.Secondary
                            compact: true
                        }

                        CxButton {
                            visible: modelData.installed
                            text: qsTr("卸载")
                            cxStyle: CxButton.Style.Secondary
                            compact: true
                        }
                    }
                }
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

            Text {
                text: qsTr("从插件市场浏览更多插件")
                color: Theme.textTertiary
                font.pixelSize: 10
            }

            Item { Layout.fillWidth: true }

            CxButton {
                text: qsTr("关闭")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.close()
            }
        }
    }
}
