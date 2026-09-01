import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// CreatePresetsDialog.qml — Phase 147 (PSET-02) Create User Preset
//
// Upstream: third_party/OrcaSlicer/src/slic3r/GUI/CreatePresetsDialog.cpp
//   - Batch preset creator (used for printer/material/process setup)
//   - Source-type selection + name + scope + base-inherits + save
//
// OWzx implementation (Phase 147 minimal source-truth port):
//   - Modal CxDialog form
//   - Scope selector (printer / material / process)
//   - "Inherits from" preset dropdown (filtered by scope)
//   - Name input + duplicate-name warning
//   - [Create] [Cancel] buttons
//   - Calls PresetServiceMock.createCustomPreset via ConfigViewModel proxy
//
// The dialog intentionally mirrors the upstream layout at a coarse level.
// Batch/multi-create features (upstream allows creating several presets in one
// session) are deferred — this port handles the single-preset-create flow.
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    dialogTitle: qsTr("创建预设")
    width: 480
    height: 280
    padding: 0

    property var editorVm: null
    property var configVm: null
    // v5.16 (PSET2-02): the UI scope order is 打印机/耗材/工艺, which maps to
    // PresetServiceMock::Category PrinterCat=2 / FilamentCat=1 / PrintCat=0
    // (PresetServiceMock.h:18). The previous code passed the raw combo index
    // as the category, so choosing "打印机" created a PRINT preset. Index
    // order below follows the combo: 0=打印机, 1=材料, 2=工艺.
    readonly property var scopeCategories: [2, 1, 0]
    // Selected category (PresetServiceMock::Category int). Default: process.
    property int selectedCategory: 0
    // Selected parent preset name ("" = no inheritance).
    property string selectedInherits: ""

    onOpened: {
        // Default scope = process (the most common create flow) → PrintCat.
        scopeCombo.currentIndex = 2
        root.selectedCategory = root.scopeCategories[2]
        refreshInheritsList()
    }

    function refreshInheritsList() {
        if (!root.configVm) return
        // Pull the existing-scope preset list. ConfigViewModel exposes
        // per-scope QStringList Q_PROPERTYs (printerPresetNames /
        // filamentPresetNames / printPresetNames) — use the one matching the
        // selected category so the "inherits from" dropdown only shows
        // relevant presets.
        var names = []
        if (root.selectedCategory === 2 && root.configVm.printerPresetNames)
            names = root.configVm.printerPresetNames
        else if (root.selectedCategory === 1 && root.configVm.filamentPresetNames)
            names = root.configVm.filamentPresetNames
        else if (root.selectedCategory === 0 && root.configVm.printPresetNames)
            names = root.configVm.printPresetNames
        inheritsCombo.model = (names && names.length > 0) ? names : []
        inheritsCombo.currentIndex = 0
        root.selectedInherits = (names && names.length > 0) ? names[0] : ""
        dupWarning.visible = false
        dupWarning.text = ""
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingXL
        spacing: Theme.spacingMD
        Text {
            text: qsTr("创建新的用户预设")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMD
            font.bold: true
        }

        // Scope selector (对齐上游 Preset::Type radio group)
        RowLayout {
            spacing: Theme.spacingMD
            Text { text: qsTr("范围："); color: Theme.textMuted; font.pixelSize: Theme.fontSizeSM }
            CxComboBox {
                id: scopeCombo
                Layout.preferredWidth: 160
                model: [qsTr("打印机"), qsTr("材料"), qsTr("工艺")]
                onActivated: function(i) {
                    // UI 打印机/耗材/工艺 → PrinterCat/FilamentCat/PrintCat
                    // via scopeCategories (PSET2-02 mapping fix).
                    if (i >= 0 && i < root.scopeCategories.length)
                        root.selectedCategory = root.scopeCategories[i]
                    root.refreshInheritsList()
                }
            }
        }

        // Inherits-from selector (对齐上游 "Inherits from" combo)
        RowLayout {
            spacing: Theme.spacingMD
            Text { text: qsTr("继承自："); color: Theme.textMuted; font.pixelSize: Theme.fontSizeSM }
            CxComboBox {
                id: inheritsCombo
                Layout.preferredWidth: 260
                // model is set by refreshInheritsList()
                onActivated: function(i) {
                    var names = inheritsCombo.model
                    root.selectedInherits = (names && i >= 0 && i < names.length) ? names[i] : ""
                }
            }
        }

        // Name input + duplicate warning
        RowLayout {
            spacing: Theme.spacingMD
            Text { text: qsTr("名称："); color: Theme.textMuted; font.pixelSize: Theme.fontSizeSM }
            CxTextField {
                id: nameInput
                Layout.preferredWidth: 260
                implicitHeight: 26
                font.pixelSize: Theme.fontSizeSM
                placeholderText: qsTr("输入预设名称")
            }
        }
        Text {
            id: dupWarning
            text: qsTr("该名称的预设已存在")
            color: Theme.accentDark
            font.pixelSize: Theme.fontSizeXS
            visible: false
            Layout.leftMargin: 80
        }

        Item { Layout.fillHeight: true } // spacer

        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: Theme.spacingMD
            CxButton {
                text: qsTr("取消")
                onClicked: root.reject()
            }
            CxButton {
                text: qsTr("创建")
                enabled: nameInput.text.length > 0 && !dupWarning.visible
                onClicked: {
                    if (!root.configVm) { root.reject(); return }
                    const name = nameInput.text.trim()
                    // v5.16 (PSET2-02): pass the inherits selection through.
                    // ConfigViewModel::createCustomPreset(category, name,
                    // inherits) seeds the new preset from the parent's
                    // resolved chain (upstream CreatePresetsDialog inherits).
                    const ok = root.configVm.createCustomPreset(
                        root.selectedCategory, name, root.selectedInherits)
                    if (ok) {
                        root.accept()
                    } else {
                        dupWarning.text = root.configVm.lastPresetError
                            ? root.configVm.lastPresetError
                            : qsTr("创建预设失败")
                        dupWarning.visible = true
                    }
                }
            }
        }
    }
}
