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
// U02 alignment (upstream FilamentGroupPopup.cpp):
//   - Geometry: width 372 (upstream 33*em ≈ 528 was measured to the fork
//     chrome; the ask locks 372), padding top/bottom 15 / horizontal 16 /
//     mode-row gap 12 / radio->text 4 (:105-108).
//   - Mode rows: title 14px / description 12px, selected title bold
//     (Body_14 :133, Head_14 :401); self-drawn 18px radio indicator with the
//     R1 accent when selected (in-file implementation, no CxRadioButton).
//   - Convenience gate: disabled + grayed with a "(Sync with printer)" hint
//     until editorVm.machineSyncReady (:238-257); picking is rejected
//     (:335) and a stale AutoForMatch falls back to AutoForFlush on open
//     (:343-347). Radio seeding uses editorVm.resolvedFilamentMapMode()
//     (PartPlate.cpp:337-349) so the inherit-sentinel resolves to the real
//     global mode instead of always seeding AutoForFlush.
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

    // U02: upstream m_connected (FilamentGroupPopup.cpp:271 tryPopup <->
    // Plater::get_machine_sync_status). Drives the Convenience-mode gate.
    readonly property bool syncReady: root.editorVm ? root.editorVm.machineSyncReady : false

    // U02: explicit instance radius 16 (upstream DrawRoundedCorner(16),
    // FilamentGroupPopup.cpp:296).
    popupRadius: 16

    // U02: modeless popup (modal: false) -- drop CxPopup's dim overlay so the
    // plater stays live while the popup is open (upstream PopupWindow).
    Overlay.modeless: Item {}

    x: 0
    y: 0
    width: 372
    height: contentCol.implicitHeight + 30  // 15 top + 15 bottom (:106)
    modal: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // Read-side gate: the auto-recommended map is only meaningful when the engine
    // produced one (Phase 108 hasAutoFilamentMap mirrors the WTREAD-02 showWipeTower
    // gate). False until a valid auto recommendation arrives (mode < fmmManual).
    readonly property bool hasAutoMap: root.editorVm ? root.editorVm.hasAutoFilamentMap : false
    readonly property int autoMode: root.editorVm ? root.editorVm.autoFilamentMapMode : root.fmmAutoForFlush
    readonly property var autoMaps: root.editorVm ? root.editorVm.autoFilamentMaps : []

    // Selected mode is driven by the radio group. Bound to the current plate's
    // resolved mode on open, then written back through setPlateFilamentMapMode
    // when the user picks a mode. The inherit-sentinel is never offered, so the
    // selected value is always one of the 3 concrete modes.
    property int selectedMode: root.fmmAutoForFlush

    function openForCurrentPlate() {
        if (!root.editorVm) return
        // U02: seed from the RESOLVED mode (PartPlate.cpp:337-349) instead of
        // the raw plate value, so the inherit-sentinel (value 3) surfaces as
        // the real global filament_map_mode rather than collapsing to 0.
        let stored = root.fmmAutoForFlush
        if (root.editorVm.resolvedFilamentMapMode) {
            const resolved = root.editorVm.resolvedFilamentMapMode()
            stored = (resolved >= root.fmmAutoForFlush && resolved <= root.fmmManual)
                ? resolved : root.fmmAutoForFlush
        }
        // U02: upstream update_dialog (:343-347) — a stale Convenience mode
        // with no machine connection falls back to AutoForFlush (persisted
        // through the same write path as a radio pick, :344).
        if (stored === root.fmmAutoForMatch && !root.syncReady) {
            root.selectedMode = root.fmmAutoForFlush
            root.applySelectedMode()
        } else {
            root.selectedMode = stored
        }
        root.open()
    }

    onOpened: openForCurrentPlate()

    contentItem: ColumnLayout {
        id: contentCol
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.leftMargin: 16   // upstream horizontal_margin, .cpp:105
        anchors.rightMargin: 16
        anchors.topMargin: 15    // upstream vertical_margin, .cpp:106
        anchors.bottomMargin: 15
        spacing: Theme.spacingSM

        Text {
            text: qsTr("耗材分组")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeLG
            font.bold: true
            Layout.topMargin: Theme.spacingXS
        }

        Text {
            text: qsTr("为本盘选择耗材到喷嘴的映射方式。")
            color: Theme.textMuted
            font.pixelSize: Theme.fontSizeSM
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        // 3 selectable mode rows (fmmAutoForFlush / fmmAutoForMatch /
        // fmmManual). U02: self-drawn radio rows (18px indicator + 4px gap +
        // 14px title + 12px description; selected title bold) replacing the
        // stock RadioButton -- upstream draws bitmap radios beside
        // Body_14/Head_14 labels (:133/:401). Convenience is disabled and
        // grayed without a machine sync (:238-257) and rejects picks (:335).
        Repeater {
            model: [
                { mode: root.fmmAutoForFlush, title: qsTr("省耗材"),
                  hint: qsTr("最小化冲刷量（自动推荐）。") },
                { mode: root.fmmAutoForMatch, title: qsTr("便利"),
                  hint: qsTr("匹配 AMS 已装载耗材（自动推荐）。") },
                { mode: root.fmmManual,       title: qsTr("自定义"),
                  hint: qsTr("使用显式的每喷嘴耗材映射。") }
            ]
            delegate: Item {
                id: modeRow
                required property int index
                required property var modelData

                readonly property bool checked: root.selectedMode === modelData.mode
                // U02: Convenience requires the machine-sync gate (:335).
                readonly property bool enabledMode:
                    modelData.mode !== root.fmmAutoForMatch || root.syncReady
                readonly property color titleColor:
                    enabledMode ? Theme.textPrimary : Theme.textDisabled
                readonly property color hintColor:
                    enabledMode ? Theme.textMuted : Theme.textDisabled

                Layout.fillWidth: true
                Layout.topMargin: index > 0 ? 12 : 0  // upstream vertical_padding, .cpp:107
                implicitHeight: rowLayout.implicitHeight

                ColumnLayout {
                    id: rowLayout
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 2

                    Row {
                        spacing: 4  // upstream ratio_spacing, .cpp:108

                        // Self-drawn radio indicator: 18px ring, R1 accent
                        // when selected (upstream radio_on/off bitmaps).
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 18
                            height: 18
                            radius: 9
                            color: "transparent"
                            border.width: modeRow.checked ? 2 : 1.5
                            border.color: modeRow.checked
                                ? Theme.accent
                                : (modeRow.enabledMode ? Theme.borderDefault : Theme.borderSubtle)

                            Rectangle {
                                anchors.centerIn: parent
                                width: 10
                                height: 10
                                radius: 5
                                visible: modeRow.checked
                                color: Theme.accent
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: modeRow.modelData.title
                            color: modeRow.titleColor
                            font.pixelSize: Theme.fontSizeLG  // 14px, Body_14/Head_14
                            font.bold: modeRow.checked        // selected -> Head_14 bold
                        }
                    }

                    Text {
                        Layout.leftMargin: 22  // indicator 18 + gap 4
                        Layout.fillWidth: true
                        wrapMode: Text.WordWrap
                        text: modeRow.enabledMode
                            ? modeRow.modelData.hint
                            : qsTr("（与打印机同步）")  // upstream MachineSyncTip
                        color: modeRow.hintColor
                        font.pixelSize: Theme.fontSizeMD   // 12px description
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: modeRow.enabledMode
                        ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: {
                        if (!modeRow.enabledMode)
                            return  // upstream OnRadioBtn early-return, :335
                        if (root.selectedMode === modeRow.modelData.mode)
                            return
                        root.selectedMode = modeRow.modelData.mode
                        root.applySelectedMode()
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
                anchors.margins: Theme.spacingSM
                spacing: Theme.spacingXS
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
