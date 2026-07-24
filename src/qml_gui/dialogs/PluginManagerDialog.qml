import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P10.1 -- PluginManagerDialog (Phase 202: real PluginService backend).
// Aligns with upstream WebDownPluginDlg (plugin download / install /
// enable-disable management). The data source is PluginService (C++) with
// QSettings persistence under plugins/*. installPlugin is a mock state flip
// (no real HTTP download / no Python) -- see PLAN.md.
//
// Usage: PluginManagerDialog { id: dlg; pluginService: backend.pluginService }
//   -> dlg.open()

CxDialog {
    id: root

    closePolicy: Popup.NoAutoClose

    dialogTitle: qsTr("插件管理")

    anchors.centerIn: parent
    width: 440
    height: 340

    // Phase 202: backend binding. Caller (main.qml) sets
    // pluginService: backend.pluginService. Falls back to a null-safe render
    // when unset so the dialog stays usable in isolation.
    property var pluginService: null

    // Live view of the registry. Re-evaluated on every pluginService
    // stateChanged (the single batch NOTIFY signal).
    readonly property var _plugins: pluginService ? pluginService.plugins : []

    contentItem: ColumnLayout {
        width: root.width
        spacing: Theme.spacingMD
        anchors.margins: Theme.spacingXL
        // Plugin list
        Repeater {
            model: root._plugins

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: pluginCol.implicitHeight + 16
                radius: 6
                color: Theme.scrollBarTrackColor
                border.color: Theme.borderInput
                border.width: 1

                ColumnLayout {
                    id: pluginCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: Theme.spacingMD
                    spacing: Theme.spacingSM
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingMD
                        Text {
                            text: modelData.name
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
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
                                color: Theme.accent
                                font.pixelSize: Theme.fontSizeXS
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            text: modelData.size
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeXS
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: modelData.description
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        wrapMode: Text.Wrap
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingMD
                        // Phase 202: enable/disable routes through the
                        // PluginService (persisted). Disabled until the row is
                        // installed (mirrors pre-Phase-202 behavior).
                        CxCheckBox {
                            text: modelData.isInstalled ? qsTr("启用") : qsTr("安装")
                            checked: modelData.isEnabled
                            enabled: modelData.isInstalled
                            onCheckedChanged: {
                                if (!root.pluginService)
                                    return
                                // Guard against the binding seeding the value:
                                // only act on a real user-initiated delta.
                                if (checked !== modelData.isEnabled)
                                    root.pluginService.setPluginEnabled(index, checked)
                            }
                        }

                        Text {
                            text: modelData.status
                            color: modelData.isInstalled ? Theme.accent : Theme.textTertiary
                            font.pixelSize: Theme.fontSizeXS
                        }

                        Item { Layout.fillWidth: true }

                        // Phase 202: mock install (state flip, no real download).
                        CxButton {
                            visible: !modelData.isInstalled
                            text: qsTr("下载")
                            cxStyle: CxButton.Style.Secondary
                            compact: true
                            onClicked: {
                                if (root.pluginService)
                                    root.pluginService.installPlugin(index)
                            }
                        }

                        CxButton {
                            visible: modelData.isInstalled
                            text: qsTr("卸载")
                            cxStyle: CxButton.Style.Secondary
                            compact: true
                            onClicked: {
                                if (root.pluginService)
                                    root.pluginService.uninstallPlugin(index)
                            }
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
            spacing: Theme.spacingMD
            Text {
                text: qsTr("从插件市场浏览更多插件")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
            }

            Item { Layout.fillWidth: true }

            // Phase 202: refresh reloads the registry + persisted state.
            CxButton {
                text: qsTr("刷新")
                cxStyle: CxButton.Style.Secondary
                onClicked: {
                    if (root.pluginService)
                        root.pluginService.refreshPluginList()
                }
            }

            CxButton {
                text: qsTr("关闭")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.close()
            }
        }
    }
}
