import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Controls
import QtQuick.Layouts
import OWzx

// PluginsConfigDialog.qml - per-preset plugin capability overrides.
//
// Presentation of upstream PluginsConfigDialog (PluginsConfigDialog.cpp,
// web/dialog/PluginsConfigDialog): capability list on the left, the
// selected capability's configuration JSON on the right, "恢复默认" drops
// the preset's override, "保存" stores the edited JSON. A pure editor over
// the option's JSON text: the owning OptionRow writes the edited text back
// on close, through the normal option value / dirty path.
//
// OWzx has no plugin runtime (PluginService registry is a mock), so the
// list shows the overrides stored in the preset; upstream additionally
// lists capabilities referenced by plugin-backed options and their
// effective global configuration.
CxDialog {
    id: root

    property var optionModel: null
    property string overridesText: ""   // working copy (compact JSON or "")
    property string originalText: ""
    property int currentIndex: 0
    property string statusText: ""
    property string validationText: ""

    signal finished(string text)

    readonly property var entries: root.optionModel ? root.optionModel.pluginOverrideEntries(root.overridesText) : []
    readonly property string parseError: root.optionModel ? root.optionModel.pluginOverridesError(root.overridesText) : ""
    readonly property var currentEntry: root.currentIndex >= 0 && root.currentIndex < root.entries.length
                                        ? root.entries[root.currentIndex] : null

    function openWith(text) {
        root.overridesText = text
        root.originalText = text
        root.currentIndex = 0
        root.statusText = ""
        root.validationText = ""
        root.open()
    }

    dialogTitle: root.optionModel ? root.optionModel.pluginConfigTitle() : qsTr("插件")
    width: 640
    height: 520

    onClosed: root.finished(root.overridesText)

    contentItem: ColumnLayout {
        spacing: Theme.spacingSM

        Text {
            visible: root.entries.length === 0 && root.parseError === ""
            Layout.fillWidth: true
            Layout.fillHeight: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
            text: qsTr("此预设没有使用任何插件能力")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeMD
        }

        Text {
            visible: root.parseError !== ""
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: root.parseError
            color: Theme.statusError
            font.pixelSize: Theme.fontSizeSM
        }

        RowLayout {
            visible: root.entries.length > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingMD

            ListView {
                id: capabilityList
                Layout.preferredWidth: 200
                Layout.fillHeight: true
                clip: true
                model: root.entries
                currentIndex: root.currentIndex
                delegate: Rectangle {
                    id: capabilityRow
                    required property var modelData
                    required property int index
                    width: capabilityList.width
                    height: 44
                    radius: Theme.radiusSM
                    color: capabilityRow.index === root.currentIndex ? Theme.bgPanel : "transparent"

                    Column {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.leftMargin: Theme.spacingSM
                        spacing: 2
                        Text {
                            width: parent.width
                            text: capabilityRow.modelData.capability !== ""
                                  ? capabilityRow.modelData.capability : qsTr("（未命名）")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            text: capabilityRow.modelData.capabilityType + "  " + capabilityRow.modelData.pluginKey
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeXS
                            elide: Text.ElideRight
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            root.currentIndex = capabilityRow.index
                            root.validationText = ""
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.spacingSM

                CxTextArea {
                    id: configText
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    font.family: "Consolas"
                    font.pixelSize: Theme.fontSizeSM
                    readOnly: root.parseError !== ""
                    text: root.currentEntry ? root.currentEntry.configText : ""
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSM

                    Text {
                        Layout.fillWidth: true
                        text: root.validationText
                        color: Theme.statusError
                        font.pixelSize: Theme.fontSizeSM
                        wrapMode: Text.WordWrap
                    }

                    CxButton {
                        text: qsTr("恢复默认")
                        cxStyle: CxButton.Style.Secondary
                        compact: true
                        enabled: root.currentEntry !== null && root.parseError === ""
                        toolTipText: qsTr("放弃此预设的覆盖，改用全局配置")
                        onClicked: restoreConfirm.open()
                    }

                    CxButton {
                        text: qsTr("保存")
                        compact: true
                        enabled: root.currentEntry !== null && root.parseError === ""
                        onClicked: {
                            const result = root.optionModel.pluginOverrideSetConfig(
                                root.overridesText, root.currentIndex, configText.text)
                            if (result.ok) {
                                root.overridesText = result.text
                                root.validationText = ""
                                root.statusText = qsTr("配置已更新。保存预设以持久化。")
                            } else {
                                root.validationText = result.error
                            }
                        }
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: root.statusText
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            elide: Text.ElideRight
        }
    }

    CxDialog {
        id: restoreConfirm
        dialogTitle: qsTr("恢复默认")
        width: 380
        height: 200
        contentItem: ColumnLayout {
            spacing: Theme.spacingMD
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: qsTr("恢复“%1”的默认配置？\n\n这会放弃此预设的覆盖，改用全局配置。")
                      .arg(root.currentEntry ? root.currentEntry.capability : "")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                CxButton {
                    text: qsTr("否")
                    cxStyle: CxButton.Style.Secondary
                    onClicked: restoreConfirm.close()
                }
                CxButton {
                    text: qsTr("是")
                    onClicked: {
                        root.overridesText = root.optionModel.pluginOverrideRemove(root.overridesText, root.currentIndex)
                        root.currentIndex = 0
                        root.statusText = qsTr("已改用全局配置。保存预设以持久化。")
                        restoreConfirm.close()
                    }
                }
            }
        }
    }
}
