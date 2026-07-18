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
    // Phase 147 (PSET-02): scope enum mirrors upstream Preset::Type
    //   0 = printer, 1 = filament (material), 2 = print (process)
    property int selectedScope: 2

    onOpened: {
        // Default scope = process (the most common create flow).
        scopeCombo.currentIndex = 2
        root.selectedScope = 2
        refreshInheritsList()
    }

    function refreshInheritsList() {
        if (!root.configVm) return
        // Phase 147 (PSET-02): pull the existing-scope preset list. ConfigViewModel
        // exposes per-scope QStringList Q_PROPERTYs (printerPresetNames /
        // filamentPresetNames / printPresetNames) — use the one matching the
        // selected scope so the "inherits from" dropdown only shows relevant presets.
        var names = []
        if (root.selectedScope === 0 && root.configVm.printerPresetNames)
            names = root.configVm.printerPresetNames
        else if (root.selectedScope === 1 && root.configVm.filamentPresetNames)
            names = root.configVm.filamentPresetNames
        else if (root.selectedScope === 2 && root.configVm.printPresetNames)
            names = root.configVm.printPresetNames
        inheritsCombo.model = (names && names.length > 0) ? names : [qsTr("（无）")]
        inheritsCombo.currentIndex = 0
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        Text {
            text: qsTr("创建新的用户预设")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMD
            font.bold: true
        }

        // Scope selector (对齐上游 Preset::Type radio group)
        RowLayout {
            spacing: 8
            Text { text: qsTr("范围："); color: Theme.textMuted; font.pixelSize: Theme.fontSizeSM }
            CxComboBox {
                id: scopeCombo
                Layout.preferredWidth: 160
                model: [qsTr("打印机"), qsTr("材料"), qsTr("工艺")]
                onActivated: {
                    // 0=Printer, 1=Material, 2=Process (matches upstream Type enum order)
                    root.selectedScope = currentIndex
                    root.refreshInheritsList()
                }
            }
        }

        // Inherits-from selector (对齐上游 "Inherits from" combo)
        RowLayout {
            spacing: 8
            Text { text: qsTr("继承自："); color: Theme.textMuted; font.pixelSize: Theme.fontSizeSM }
            CxComboBox {
                id: inheritsCombo
                Layout.preferredWidth: 260
                // model is set by refreshInheritsList()
            }
        }

        // Name input + duplicate warning
        RowLayout {
            spacing: 8
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
            spacing: 8
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
                    // Phase 147 (PSET-02): delegate to ConfigViewModel.createCustomPreset
                    // which proxies to PresetServiceMock::createCustomPreset. The existing
                    // 2-arg signature (category, name) creates an empty preset that
                    // inherits from the currently-selected preset in that scope; the user
                    // then edits values in the settings panel and saves via the dirty flow.
                    const ok = root.configVm.createCustomPreset(root.selectedScope, name)
                    if (ok) {
                        root.accept()
                    } else {
                        dupWarning.text = qsTr("创建失败（名称重复或范围无效）")
                        dupWarning.visible = true
                    }
                }
            }
        }
    }
}
