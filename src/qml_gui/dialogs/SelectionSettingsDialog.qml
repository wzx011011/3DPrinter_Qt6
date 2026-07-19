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
//     showing the object's effective print/filament params with per-object
//     override capability.
//
// OWzx implementation (Phase 174 minimal source-truth port):
//   - Non-modal CxDialog
//   - Shows the 6 most-common FDM override keys (layer_height, fill_density,
//     wall_loops, support_material, nozzle_temperature, bed_temperature).
//   - Each row: key label + current value (read via scopedOptionValue) +
//     editable input (write via setScopedOptionValue) + reset button
//     (resetScopedOptionValue).
//   - "Overridden keys" footer list showing non-default overrides.
//
// Backend: ProjectServiceMock already exposes scopedOptionValue/
// setScopedOptionValue/scopedOverrideCount/scopedOverriddenKey/
// resetScopedOptionValue. Phase 174 adds QML proxies on EditorViewModel
// + this dialog wired via selectionSettingsRequested.
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

    function refreshModel() {
        // Re-read all values from the backend (cheap; called on open + after each edit).
        overrideModel.clear()
        for (var i = 0; i < keyModel.count; ++i) {
            var entry = keyModel.get(i)
            var val = ""
            if (root.editorVm && root.objectIndex >= 0) {
                val = root.editorVm.scopedOptionValue(root.objectIndex, root.volumeIndex, entry.key, "")
            }
            overrideModel.append({
                key: entry.key,
                label: entry.label,
                unit: entry.unit,
                value: val,
                type: entry.type,
                min: entry.min,
                max: entry.max,
                step: entry.step
            })
        }
    }

    // The 6 most-common FDM override keys (对齐上游 ObjectSettings common options).
    ListModel {
        id: keyModel
        ListElement { key: "layer_height";      label: "层高";        unit: "mm"; type: "double"; min: "0.05"; max: "0.5";  step: "0.01" }
        ListElement { key: "fill_density";      label: "填充密度";    unit: "%";  type: "int";    min: "0";    max: "100"; step: "5" }
        ListElement { key: "wall_loops";        label: "墙层数";      unit: "";   type: "int";    min: "1";    max: "8";   step: "1" }
        ListElement { key: "support_material";  label: "生成支撑";    unit: "";   type: "bool";   min: "";     max: "";    step: "" }
        ListElement { key: "nozzle_temperature"; label: "喷嘴温度";   unit: "°C"; type: "int";    min: "150";  max: "320"; step: "5" }
        ListElement { key: "bed_temperature";   label: "热床温度";    unit: "°C"; type: "int";    min: "0";    max: "150"; step: "5" }
    }

    ListModel { id: overrideModel }

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

        // Override rows
        Repeater {
            model: overrideModel
            delegate: RowLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: Theme.spacingMD

                Text {
                    Layout.preferredWidth: 100
                    text: modelData.label
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                }

                // Value input — CxTextField for numeric, CxCheckBox for bool
                Loader {
                    Layout.fillWidth: true
                    sourceComponent: modelData.type === "bool" ? boolEditComp : numEditComp
                    Component {
                        id: numEditComp
                        CxTextField {
                            text: modelData.value
                            font.pixelSize: Theme.fontSizeSM
                            placeholderText: qsTr("（继承预设）")
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
                            checked: modelData.value === "1" || modelData.value === "true"
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

                CxIconButton {
                    buttonSize: 24
                    iconSize: 12
                    cxStyle: CxIconButton.Style.Ghost
                    iconSource: ""
                    toolTipText: qsTr("重置为预设值")
                    Text {
                        anchors.centerIn: parent
                        text: "×"
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSizeMD
                    }
                    onClicked: {
                        if (root.editorVm) {
                            root.editorVm.resetScopedOptionValue(root.objectIndex, root.volumeIndex, modelData.key)
                            root.refreshModel()
                        }
                    }
                }
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
