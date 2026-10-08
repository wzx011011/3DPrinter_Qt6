import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// SelectionSettingsDialog.qml — Phase 174 (FEAT-01) Per-Object Settings Override
//
// Upstream: third_party/OrcaSlicer/src/slic3r/GUI/GUI_ObjectSettings.cpp
//   - Right-click → Settings on a selected object/volume opens an inspector
//     that lists the object's OVERRIDDEN params, each row carrying a delete
//     button (tooltip "Remove parameter") that erases the key from the scoped
//     config and rebuilds the list (config->erase + update_settings_list,
//     GUI_ObjectSettings.cpp:113-129).
//   - Options are added from SettingsFactory category bundles; the FFF
//     frequent set is FREQ_SETTINGS_BUNDLE_FFF (GUI_Factories.cpp:56-69):
//     质量 Quality=layer_height；外壳 Shell=wall_loops/top_shell_layers/
//     bottom_shell_layers；填充 Infill=sparse_infill_density/
//     sparse_infill_pattern；支撑 Support=enable_support/support_type/
//     support_threshold_angle；冲刷选项 Flush options=flush_into_infill/
//     flush_into_objects/flush_into_support.
//     (Legacy names fill_density/support_material and the nozzle/bed
//     temperature keys are NOT part of the upstream object-override set.)
//
// OWzx implementation:
//   - Non-modal CxDialog (dialog container kept: FEAT-01 locks tests:4454/
//     :10368 — registered deviation from the upstream settings side-panel).
//   - Overridden rows driven by scopedOverrideCount/scopedOverriddenKey
//     (ProjectServiceMock scoped-config keys()).
//   - "添加覆盖参数" entry: category combo → option combo → 添加；the add
//     writes an empty value, which seeds the override through the backend
//     default-clone channel (writeConfigValue, ProjectServiceMock.cpp:367-413)
//     — the user then edits the seeded row (no inherited-preset read channel
//     exists on the current VM surface).
//   - Row remove (x.svg): resetScopedOptionValue (= config->erase) and the
//     row disappears on refresh.
//   - Row rhythm R11: row height 30 / pitch 30 (zero gap), input 119×25,
//     label column 100 / unit column 30.
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: false
    dialogTitle: qsTr("对象设置")
    width: 520
    height: 480

    property var editorVm: null
    property int objectIndex: -1
    property int volumeIndex: -1

    onOpened: refreshModel()

    // Category-grouped FFF override catalog — replica of the upstream
    // FREQ_SETTINGS_BUNDLE_FFF (GUI_Factories.cpp:56-69).
    property var optionCatalog: [
        { category: qsTr("质量"), options: [
              { key: "layer_height", label: qsTr("层高"), unit: "mm", type: "double" }
          ] },
        { category: qsTr("外壳"), options: [
              { key: "wall_loops", label: qsTr("墙层数"), unit: "", type: "int" },
              { key: "top_shell_layers", label: qsTr("顶部外壳层数"), unit: "", type: "int" },
              { key: "bottom_shell_layers", label: qsTr("底部外壳层数"), unit: "", type: "int" }
          ] },
        { category: qsTr("填充"), options: [
              { key: "sparse_infill_density", label: qsTr("稀疏填充密度"), unit: "%", type: "percent" },
              { key: "sparse_infill_pattern", label: qsTr("稀疏填充图案"), unit: "", type: "enum" }
          ] },
        { category: qsTr("支撑"), options: [
              { key: "enable_support", label: qsTr("启用支撑"), unit: "", type: "bool" },
              { key: "support_type", label: qsTr("支撑类型"), unit: "", type: "enum" },
              { key: "support_threshold_angle", label: qsTr("支撑阈值角度"), unit: "°", type: "int" }
          ] },
        { category: qsTr("冲刷选项"), options: [
              { key: "flush_into_infill", label: qsTr("冲刷至填充"), unit: "", type: "bool" },
              { key: "flush_into_objects", label: qsTr("冲刷至物体"), unit: "", type: "bool" },
              { key: "flush_into_support", label: qsTr("冲刷至支撑"), unit: "", type: "bool" }
          ] }
    ]

    readonly property var categoryNames: {
        var names = []
        for (var i = 0; i < optionCatalog.length; ++i)
            names.push(optionCatalog[i].category)
        return names
    }

    // Overridden rows, rebuilt from the backend scoped config (upstream
    // update_settings_list rebuilds from config->keys(), :84-87).
    property var overrideRows: []

    function keyMeta(key) {
        for (var c = 0; c < optionCatalog.length; ++c) {
            var options = optionCatalog[c].options
            for (var i = 0; i < options.length; ++i) {
                if (options[i].key === key)
                    return options[i]
            }
        }
        return null
    }

    function refreshModel() {
        var rows = []
        if (root.editorVm && root.objectIndex >= 0) {
            var count = root.editorVm.scopedOverrideCount(root.objectIndex, root.volumeIndex)
            for (var i = 0; i < count; ++i) {
                var key = root.editorVm.scopedOverriddenKey(root.objectIndex, root.volumeIndex, i)
                if (!key || key.length === 0)
                    continue
                var meta = root.keyMeta(key)
                rows.push({
                    key: key,
                    label: meta ? meta.label : key,
                    unit: meta ? meta.unit : "",
                    type: meta ? meta.type : "text",
                    value: root.editorVm.scopedOptionValue(root.objectIndex, root.volumeIndex, key, "")
                })
            }
        }
        overrideRows = rows
    }

    function addOverride() {
        if (!root.editorVm || root.objectIndex < 0)
            return
        var category = optionCatalog[addCategoryCombo.currentIndex]
        if (!category)
            return
        var option = category.options[addKeyCombo.currentIndex]
        if (!option)
            return
        // Empty-value write seeds the override via the backend default-clone
        // channel (writeConfigValue, ProjectServiceMock.cpp:367-413); the
        // seeded row then shows up in the list for editing.
        root.editorVm.setScopedOptionValue(root.objectIndex, root.volumeIndex, option.key, "")
        root.refreshModel()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingXL
        spacing: Theme.spacingMD

        Text {
            Layout.fillWidth: true
            text: root.volumeIndex >= 0
                  ? qsTr("对象 %1 · 部件 %2").arg(root.objectIndex).arg(root.volumeIndex)
                  : qsTr("对象 %1").arg(root.objectIndex)
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
        }

        Text {
            Layout.fillWidth: true
            text: qsTr("已覆盖参数")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
        }

        // Empty state — nothing overridden yet.
        Text {
            Layout.fillWidth: true
            visible: root.overrideRows.length === 0
            text: qsTr("无覆盖参数 — 全部继承预设值")
            color: Theme.textMuted
            font.pixelSize: Theme.fontSizeSM
        }

        ColumnLayout {
            Layout.fillWidth: true
            // R11 rhythm: row height 30 with zero gap → 30px pitch.
            spacing: 0

            Repeater {
                model: root.overrideRows
                delegate: RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    spacing: Theme.spacingMD

                    Text {
                        Layout.preferredWidth: 100
                        text: modelData.label
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        elide: Text.ElideRight
                    }

                    // Value editor — CxCheckBox for bool keys, CxTextField
                    // otherwise. Enum keys carry the upstream label string
                    // (readConfigValue coEnum branch, ProjectServiceMock.cpp:
                    // 310-318) and accept it back (label→index map :381-393).
                    Loader {
                        Layout.preferredWidth: 119
                        Layout.preferredHeight: 25
                        sourceComponent: modelData.type === "bool" ? boolEditComp : numEditComp
                        Component {
                            id: numEditComp
                            CxTextField {
                                text: modelData.value
                                font.pixelSize: Theme.fontSizeSM
                                onEditingFinished: {
                                    if (root.editorVm && text.length > 0) {
                                        root.editorVm.setScopedOptionValue(root.objectIndex, root.volumeIndex, modelData.key, text)
                                        root.refreshModel()
                                    }
                                }
                            }
                        }
                        Component {
                            id: boolEditComp
                            CxCheckBox {
                                checked: modelData.value === true || modelData.value === "1" || modelData.value === "true"
                                onToggled: {
                                    if (root.editorVm) {
                                        root.editorVm.setScopedOptionValue(root.objectIndex, root.volumeIndex, modelData.key, checked ? "1" : "0")
                                        root.refreshModel()
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        Layout.preferredWidth: 30
                        text: modelData.unit
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSizeXS
                    }

                    // Row remove — upstream delete button ("Remove parameter"
                    // tooltip, GUI_ObjectSettings.cpp:113-129): erases the key
                    // from the scoped config and rebuilds the list, so the row
                    // disappears.
                    CxIconButton {
                        buttonSize: 24
                        iconSize: 12
                        cxStyle: CxIconButton.Style.Ghost
                        iconSource: "qrc:/qml/assets/icons/x.svg"
                        toolTipText: qsTr("移除参数")
                        onClicked: {
                            if (root.editorVm) {
                                root.editorVm.resetScopedOptionValue(root.objectIndex, root.volumeIndex, modelData.key)
                                root.refreshModel()
                            }
                        }
                    }
                }
            }
        }

        // Add-override entry — category-grouped like the upstream
        // "Add settings" bundles (GUI_Factories.cpp:56-69).
        Text {
            Layout.fillWidth: true
            text: qsTr("添加覆盖参数")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD

            CxComboBox {
                id: addCategoryCombo
                Layout.preferredWidth: 110
                model: root.categoryNames
                // Reset the option picker when the category changes
                // (onActivated: user interaction only, no init-order hazard).
                onActivated: addKeyCombo.currentIndex = 0
            }

            CxComboBox {
                id: addKeyCombo
                Layout.fillWidth: true
                model: {
                    var labels = []
                    var category = root.optionCatalog[addCategoryCombo.currentIndex]
                    if (category) {
                        for (var i = 0; i < category.options.length; ++i)
                            labels.push(category.options[i].label)
                    }
                    return labels
                }
            }

            CxButton {
                text: qsTr("添加")
                onClicked: root.addOverride()
            }
        }

        Item { Layout.fillHeight: true } // spacer

        // Overridden keys footer
        Text {
            Layout.fillWidth: true
            text: {
                if (!root.editorVm || root.objectIndex < 0) return ""
                var n = root.editorVm.scopedOverrideCount(root.objectIndex, root.volumeIndex)
                return qsTr("当前覆盖项：%1").arg(n)
            }
            color: Theme.textMuted
            font.pixelSize: Theme.fontSizeXS
            horizontalAlignment: Text.AlignRight
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignRight
            spacing: Theme.spacingMD

            CxButton {
                text: qsTr("关闭")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }
        }
    }
}
