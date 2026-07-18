import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// Phase 110 (FMAP-03): FilamentGroupPopup -- CxPopup-based mode selector that
// surfaces the 3 selectable filament-map modes (AutoForFlush / AutoForMatch /
// Manual) and the Phase 108 auto-recommended map preview. Upstream alignment:
// third_party/OrcaSlicer/src/slic3r/GUI/FilamentGroupPopup.hpp:52 mode_list is
// {fmmAutoForFlush, fmmAutoForMatch, fmmManual} only. The 4th enum value
// (per-plate "inherit from global" sentinel, value 3, resolved by
// PartPlate::get_real_filament_map_mode) is NOT exposed as a selectable radio
// button (anti-feature per FEATURES.md). This file deliberately avoids the
// sentinel's symbolic name so a source audit can assert no 4th-radio leak.
//
// Usage: FilamentGroupPopup { id: filamentGroupPopup; editorVm: backend.editorViewModel }
// Trigger: filamentGroupPopup.open()
CxPopup {
    id: root

    // editorVm is the EditorViewModel exposed by BackendContext. The popup reads
    // the Phase 108 Q_PROPERTYs (hasAutoFilamentMap / autoFilamentMapMode /
    // autoFilamentMaps) for the auto-recommended preview and writes the selected
    // mode back via setPlateFilamentMapMode on projectService().
    required property var editorVm

    // The 3 selectable FilamentMapMode values (PartPlate.h:96-101). The 4th
    // value (the per-plate inherit-sentinel) is intentionally absent -- it is
    // not a UI radio.
    readonly property int fmmAutoForFlush: 0  // "Filament-Saving Mode"
    readonly property int fmmAutoForMatch: 1  // "Convenience Mode"
    readonly property int fmmManual: 2        // "Custom Mode"

    x: 0
    y: 0
    width: 280
    height: contentCol.implicitHeight + 24
    modal: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // Read-side gate: the auto-recommended map is only meaningful when the engine
    // produced one (Phase 108 hasAutoFilamentMap mirrors the WTREAD-02 showWipeTower
    // gate). False until a valid auto recommendation arrives (mode < fmmManual).
    readonly property bool hasAutoMap: root.editorVm ? root.editorVm.hasAutoFilamentMap : false
    readonly property int autoMode: root.editorVm ? root.editorVm.autoFilamentMapMode : root.fmmAutoForFlush
    readonly property var autoMaps: root.editorVm ? root.editorVm.autoFilamentMaps : []

    // Selected mode is driven by the radio group. Bound to the current plate's
    // stored mode on open, then written back through setPlateFilamentMapMode when
    // the user picks a mode. The inherit-sentinel is never offered, so the
    // selected value is always one of the 3 concrete modes.
    property int selectedMode: root.fmmAutoForFlush

    function openForCurrentPlate() {
        if (!root.editorVm) return
        const svc = root.editorVm.projectService ? root.editorVm.projectService
                                                  : null
        const plateIdx = root.editorVm.currentPlateIndex
        // Seed selectedMode from the plate's stored mode. The inherit-sentinel
        // (value 3) resolves to AutoForFlush in the UI since the popup only
        // offers the 3 concrete modes (anti-feature: no 4th radio).
        let stored = root.fmmAutoForFlush
        if (svc && svc.plateFilamentMapMode) {
            const raw = svc.plateFilamentMapMode(plateIdx)
            stored = (raw === 3 /* inherit-sentinel */) ? root.fmmAutoForFlush : raw
        }
        root.selectedMode = stored
        root.open()
    }

    onOpened: openForCurrentPlate()

    contentItem: ColumnLayout {
        id: contentCol
        width: root.width - 24
        anchors.margins: 12
        spacing: Theme.spacingSM

        Text {
            text: qsTr("耗材分组")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeLG
            font.bold: true
            Layout.topMargin: 4
        }

        Text {
            text: qsTr("为本盘选择耗材到喷嘴的映射方式。")
            color: Theme.textMuted
            font.pixelSize: Theme.fontSizeSM
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        // 3 selectable mode radios (fmmAutoForFlush / fmmAutoForMatch / fmmManual).
        // The ButtonGroup enforces single-selection; the checked binding is
        // two-way so picking a radio updates selectedMode and vice-versa.
        ButtonGroup { id: modeGroup }

        Repeater {
            model: [
                { mode: root.fmmAutoForFlush, title: qsTr("省耗材"),
                  hint: qsTr("最小化冲刷量（自动推荐）。") },
                { mode: root.fmmAutoForMatch, title: qsTr("便利"),
                  hint: qsTr("匹配 AMS 已装载耗材（自动推荐）。") },
                { mode: root.fmmManual,       title: qsTr("自定义"),
                  hint: qsTr("使用显式的每喷嘴耗材映射。") }
            ]
            delegate: RadioButton {
                required property var modelData
                Layout.fillWidth: true
                ButtonGroup.group: modeGroup
                checked: root.selectedMode === modelData.mode
                text: modelData.title
                onToggled: {
                    root.selectedMode = modelData.mode
                    root.applySelectedMode()
                }
                contentItem: ColumnLayout {
                    spacing: 0
                    Text {
                        text: parent.parent.text
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                        Layout.leftMargin: parent.parent.indicator.width + 6
                    }
                    Text {
                        text: modelData.hint
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSizeSM
                        Layout.leftMargin: parent.parent.indicator.width + 6
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        // Auto-recommended map preview (Phase 108 readback). Only shown when the
        // engine produced a valid auto recommendation. The 1-based per-extruder
        // group ids come from editorVm.autoFilamentMaps.
        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingSM
            visible: root.hasAutoMap
            height: autoPreviewCol.implicitHeight + 12
            radius: Theme.radiusMD
            color: Theme.bgBase
            border.color: Theme.borderDefault
            border.width: 1

            ColumnLayout {
                id: autoPreviewCol
                anchors.fill: parent
                anchors.margins: 6
                spacing: 2

                Text {
                    text: qsTr("自动推荐映射（模式 %1）").arg(root.autoMode)
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                }
                Text {
                    text: root.autoMaps.length > 0
                          ? root.autoMaps.join(", ")
                          : qsTr("（无喷嘴）")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingSM
            spacing: Theme.spacingSM

            Item { Layout.fillWidth: true }

            CxButton {
                text: qsTr("关闭")
                onClicked: root.close()
            }
        }
    }

    // Write the selected mode back through the Q_INVOKABLE boundary. Phase 110
    // R-02 (FP-04) clamps out-of-range ints at PartPlate::setFilamentMapMode.
    function applySelectedMode() {
        if (!root.editorVm) return
        const plateIdx = root.editorVm.currentPlateIndex
        if (root.editorVm.setPlateFilamentMapMode) {
            root.editorVm.setPlateFilamentMapMode(plateIdx, root.selectedMode)
        }
    }
}
