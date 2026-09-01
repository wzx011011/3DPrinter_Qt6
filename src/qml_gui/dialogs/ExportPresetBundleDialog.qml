import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// ExportPresetBundleDialog.qml — V21-02 PRESET-03 导出预设包
//
// Upstream ExportPresetBundleDialog offers selectable presets and zip/bbscfg
// packaging. Qt6 currently supports the local directory JSON interchange
// format only: category JSON files plus index.json. The unsupported archive
// formats are disclosed in the dialog instead of being advertised as parity.
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    dialogTitle: qsTr("导出预设包")
    width: 520
    height: 620
    padding: 0

    required property var configVm
    property var selectedPresets: ({})
    property var presetSections: []

    function selectedCount() {
        var count = 0
        for (var name in root.selectedPresets)
            if (root.selectedPresets[name]) ++count
        return count
    }

    function togglePreset(name, checked) {
        var next = {}
        for (var existing in root.selectedPresets)
            next[existing] = root.selectedPresets[existing]
        next[name] = checked
        root.selectedPresets = next
    }

    function resetSelection() {
        root.selectedPresets = ({})
        for (var i = 0; i < root.presetSections.length; ++i) {
            var section = root.presetSections[i]
            for (var j = 0; j < section.names.length; ++j)
                root.togglePreset(section.names[j], true)
        }
    }

    onOpened: {
        root.presetSections = [
            { label: qsTr("打印机"), names: configVm ? configVm.userPresetNamesForCategory(2) : [] },
            { label: qsTr("耗材"), names: configVm ? configVm.userPresetNamesForCategory(1) : [] },
            { label: qsTr("工艺"), names: configVm ? configVm.userPresetNamesForCategory(0) : [] }
        ]
        resetSelection()
    }

    contentItem: Rectangle {
        color: Theme.bgPanel
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingXXL
            spacing: Theme.spacingMD
            Text {
                Layout.fillWidth: true
                text: qsTr("选择要导出的用户预设。导出结果是本地目录格式：分类 JSON 文件 + index.json。")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.WordWrap
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("当前不支持 .zip 或 .bbscfg；不会生成或声称生成这些格式。")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                CxButton {
                    text: qsTr("全选")
                    onClicked: root.resetSelection()
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("已选择 %1 项").arg(root.selectedCount())
                    color: Theme.textSecondary
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                Column {
                    width: parent.width
                    spacing: Theme.spacingSM
                    Repeater {
                        model: presetSections
                        delegate: Column {
                            property var section: modelData
                            width: parent.width
                            spacing: Theme.spacingXS
                            Text {
                                text: parent.section.label
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeSM
                                font.bold: true
                            }
                            Repeater {
                                model: parent.section.names
                                delegate: CxCheckBox {
                                    width: parent.width
                                    text: modelData
                                    checked: root.selectedPresets[modelData] === true
                                    onToggled: root.togglePreset(modelData, checked)
                                }
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("取消")
                    onClicked: root.reject()
                }
                CxButton {
                    text: qsTr("选择目录...")
                    enabled: root.selectedCount() > 0
                    cxStyle: CxButton.Style.Primary
                    onClicked: exportFolderDialog.open()
                }
            }
        }
    }

    FolderDialog {
        id: exportFolderDialog
        title: qsTr("导出预设包")
        onAccepted: {
            // v5.16 (PSET2-04): per-preset upstream-shape JSON export
            // (configVm.exportBundleIni → PresetServiceMock::exportBundleIni).
            var path = selectedFolder.toString().replace("file:///", "")
            if (root.configVm && path.length > 0) {
                var selected = []
                for (var name in root.selectedPresets)
                    if (root.selectedPresets[name]) selected.push(name)
                var count = root.configVm.exportBundleIni(path, selected)
                if (backend)
                    backend.postNotification(count > 0
                        ? qsTr("已导出 %1 个用户预设到 %2").arg(count).arg(path)
                        : qsTr("未导出任何预设（写入失败或选择为空）"),
                        qsTr("导出预设包"), count > 0 ? 0 : 2)
            }
            root.accept()
        }
    }
}
