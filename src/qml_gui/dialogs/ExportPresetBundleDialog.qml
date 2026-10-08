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
//
// Browser chrome follows the upstream web dialog
// (resources/web/dialog/ExportPresetDialog/index.html:21-66 + styles.css):
// three equal-width columns inside a 1px-bordered container, per-column
// search with clear button, per-column "all" master checkbox and a
// "No items" empty state. Token mapping for the web CSS variables:
//   --border → Theme.borderSubtle, --bg → Theme.bgBase,
//   --panel → Theme.bgSurface, --muted → Theme.textTertiary/textSecondary.
// OrcaCloud export is intentionally omitted (brand domain keep).
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    dialogTitle: qsTr("导出预设包")
    // Upstream ExportPresetBundleDialog.cpp:40 — default 820x660, min 640x640.
    width: 820
    height: 660
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

    function setSectionSelected(section, checked) {
        // Upstream ChooseAll* (index.js:134-171): the master checkbox flips the
        // whole section, rows hidden by the search filter included.
        var next = {}
        for (var existing in root.selectedPresets)
            next[existing] = root.selectedPresets[existing]
        for (var i = 0; i < section.names.length; ++i)
            next[section.names[i]] = checked
        root.selectedPresets = next
    }

    function clearSelection() {
        // Upstream BuildNameRows (index.js:104) seeds every row checked:false —
        // the dialog opens with nothing selected.
        root.selectedPresets = ({})
    }

    onOpened: {
        root.presetSections = [
            { label: qsTr("打印机"), names: configVm ? configVm.userPresetNamesForCategory(2) : [] },
            { label: qsTr("耗材"), names: configVm ? configVm.userPresetNamesForCategory(1) : [] },
            { label: qsTr("工艺"), names: configVm ? configVm.userPresetNamesForCategory(0) : [] }
        ]
        clearSelection()
    }

    contentItem: Rectangle {
        color: Theme.bgPanel
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // Format disclosures (V21-02 PRESET-03): the supported interchange
            // format is stated first, the unsupported archive formats are
            // disclosed right below instead of being advertised as parity.
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.spacingXL
                Layout.rightMargin: Theme.spacingXL
                Layout.topMargin: Theme.spacingLG
                spacing: Theme.spacingSM
                Text {
                    Layout.fillWidth: true
                    text: qsTr("选择要导出的用户预设。导出结果是本地目录格式：分类 JSON 文件 + index.json。")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    wrapMode: Text.WordWrap
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingLG
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("当前不支持 .zip 或 .bbscfg；不会生成或声称生成这些格式。")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeXS
                        wrapMode: Text.WordWrap
                    }
                    Text {
                        text: qsTr("已选择 %1 项").arg(root.selectedCount())
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeXS
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            // Three equal-width column browser (index.html:21-66,
            // styles.css:11-21: 1px container border, three 1fr columns).
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: Theme.spacingXL   // styles.css:14 margin 15px
                Layout.rightMargin: Theme.spacingXL
                Layout.topMargin: Theme.spacingLG    // styles.css:14 margin-top 10px
                Layout.bottomMargin: Theme.spacingLG
                color: Theme.bgBase
                border.color: Theme.borderSubtle
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    Repeater {
                        model: root.presetSections

                        delegate: Rectangle {
                            id: columnRoot
                            property var section: modelData
                            readonly property bool isLast: index === root.presetSections.length - 1

                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: "transparent"

                            Rectangle {
                                id: columnBody
                                anchors.fill: parent
                                anchors.rightMargin: columnRoot.isLast ? 0 : 1
                                color: Theme.bgBase   // styles.css --cbr-panel-bg = var(--bg)

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 0

                                    // Column header: search bar row, 36px
                                    // (styles.css:35-43 min-height 36, bg var(--panel),
                                    // 1px bottom border).
                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: Theme.tabBarHeight   // 36
                                        color: Theme.bgSurface

                                        Rectangle {
                                            anchors.bottom: parent.bottom
                                            width: parent.width
                                            height: 1
                                            color: Theme.borderSubtle
                                        }

                                        // Magnifier glyph (styles.css:101-119).
                                        Item {
                                            x: 11   // styles.css:97 left: 11px
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: 16
                                            height: 16
                                            Rectangle {
                                                width: 9
                                                height: 9
                                                radius: 4.5
                                                color: "transparent"
                                                border.color: Theme.textTertiary
                                                border.width: 1.5
                                                anchors.centerIn: parent
                                                anchors.verticalCenterOffset: -2
                                                anchors.horizontalCenterOffset: -2
                                            }
                                            Rectangle {
                                                width: 5
                                                height: 1.5
                                                radius: 0.75
                                                color: Theme.textTertiary
                                                rotation: 45
                                                anchors.verticalCenter: parent.verticalCenter
                                                anchors.verticalCenterOffset: 4
                                                anchors.horizontalCenter: parent.horizontalCenter
                                                anchors.horizontalCenterOffset: 4
                                            }
                                        }

                                        CxTextField {
                                            id: searchField
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.leftMargin: Theme.spacingSM   // styles.css:40 padding 6px
                                            anchors.rightMargin: Theme.spacingSM
                                            anchors.verticalCenter: parent.verticalCenter
                                            font.pixelSize: Theme.fontSizeLG      // styles.css:49 14px
                                            leftPadding: Theme.spacingXXL         // room for the magnifier
                                            rightPadding: searchField.text.length > 0
                                                          ? Theme.spacingXXL : Theme.spacingMD
                                            placeholderText: columnRoot.section.label
                                        }

                                        // Clear button (index.js:259-278): visible
                                        // while the query is non-empty, restores
                                        // the unfiltered list and refocuses.
                                        Item {
                                            visible: searchField.text.length > 0
                                            anchors.right: parent.right
                                            anchors.rightMargin: 11   // styles.css:122 right: 11px
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: 16
                                            height: 16
                                            Rectangle {
                                                width: 10
                                                height: 1.5
                                                radius: 0.75
                                                color: Theme.textTertiary
                                                rotation: 45
                                                anchors.centerIn: parent
                                            }
                                            Rectangle {
                                                width: 10
                                                height: 1.5
                                                radius: 0.75
                                                color: Theme.textTertiary
                                                rotation: -45
                                                anchors.centerIn: parent
                                            }
                                            MouseArea {
                                                anchors.fill: parent
                                                anchors.margins: -4
                                                hoverEnabled: true
                                                cursorShape: Qt.PointingHandCursor
                                                onClicked: {
                                                    searchField.clear()
                                                    searchField.forceActiveFocus()
                                                }
                                            }
                                        }
                                    }

                                    // Column list (styles.css:149-153 padding 4px 8px).
                                    CxScrollView {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true
                                        clip: true

                                        Column {
                                            id: columnList
                                            // Real-time case-insensitive substring
                                            // filter (index.js:199-245).
                                            property var filtered: {
                                                var query = searchField.text.toLowerCase()
                                                var names = columnRoot.section ? columnRoot.section.names : []
                                                var out = []
                                                for (var i = 0; i < names.length; ++i) {
                                                    if (query.length === 0
                                                            || names[i].toLowerCase().indexOf(query) >= 0)
                                                        out.push(names[i])
                                                }
                                                return out
                                            }
                                            x: Theme.spacingMD
                                            y: Theme.spacingXS
                                            width: parent.width - 2 * Theme.spacingMD
                                            spacing: Theme.spacingXS

                                            // Master "all" checkbox with tri-state
                                            // display (upstream SyncMasterCheckbox
                                            // index.js:173-184 is binary; the partial
                                            // state is the OWzx superset for clarity).
                                            CxCheckBox {
                                                id: masterCheck
                                                width: parent.width
                                                text: qsTr("全选")
                                                font.pixelSize: Theme.fontSizeLG
                                                tristate: true
                                                // Click toggles select-all/deselect-all;
                                                // partial sections select all on click.
                                                nextCheckState: function () {
                                                    return checkState === Qt.Checked ? Qt.Unchecked : Qt.Checked
                                                }
                                                onToggled: root.setSectionSelected(
                                                               columnRoot.section, checkState === Qt.Checked)

                                                function syncState() {
                                                    var names = columnRoot.section ? columnRoot.section.names : []
                                                    var checkedCount = 0
                                                    for (var i = 0; i < names.length; ++i)
                                                        if (root.selectedPresets[names[i]] === true) ++checkedCount
                                                    checkState = checkedCount === 0 ? Qt.Unchecked
                                                               : checkedCount === names.length ? Qt.Checked
                                                               : Qt.PartiallyChecked
                                                }
                                                Component.onCompleted: syncState()
                                                Connections {
                                                    target: root
                                                    function onSelectedPresetsChanged() { masterCheck.syncState() }
                                                }
                                            }

                                            Repeater {
                                                model: columnList.filtered
                                                delegate: CxCheckBox {
                                                    width: parent.width
                                                    // 14px case-level override (upstream web
                                                    // rows render at 14px); CxCheckBox keeps
                                                    // its 12px default for everyone else.
                                                    font.pixelSize: Theme.fontSizeLG
                                                    text: modelData
                                                    checked: root.selectedPresets[modelData] === true
                                                    onToggled: root.togglePreset(modelData, checked)
                                                }
                                            }

                                            // Empty column / no search hits
                                            // (index.html:33 + index.js:247-257).
                                            Text {
                                                visible: columnList.filtered.length === 0
                                                text: qsTr("暂无预设")
                                                color: Theme.textSecondary
                                                font.pixelSize: Theme.fontSizeMD
                                                topPadding: Theme.spacingSM
                                            }
                                        }
                                    }
                                }
                            }

                            // Column separator (styles.css:31-33: 1px right border
                            // on every column except the last).
                            Rectangle {
                                visible: !columnRoot.isLast
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: 1
                                color: Theme.borderSubtle
                            }
                        }
                    }
                }
            }

            // Footer accept area (styles.css:175-177 #AcceptArea border-top).
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.borderSubtle
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.spacingXL
                Layout.rightMargin: Theme.spacingXL
                Layout.topMargin: Theme.spacingMD
                Layout.bottomMargin: Theme.spacingXL
                spacing: Theme.spacingMD
                Item { Layout.fillWidth: true }
                // Upstream "Close" renders ButtonStyleRegular (index.html:71) —
                // the cancel affordance is secondary, not another primary.
                CxButton {
                    text: qsTr("取消")
                    cxStyle: CxButton.Style.Secondary
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
