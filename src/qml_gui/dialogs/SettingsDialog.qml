// SettingsDialog.qml — non-modal ApplicationWindow settings dialog shell.
// Parameterized by presetTier ("printer"/"filament"/"print").
// Three instances, one per category. Reuses existing SavePresetDialog and
// UnsavedChangesDialog for save/dirty flows.
//
// Region IDs: SETPRINT-SHELL, SETMAT-SHELL, SETPROC-SHELL
//
// Note: the old SettingsPage/ParamsPage/ConfigPage/SearchDialog were removed
// in Phase 57-01 (CLEAN-01); this dialog is the sole settings surface.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"
import "../components"

ApplicationWindow {
    id: root

    // Required properties
    required property var configVm
    required property string presetTier
    required property var optionModel

    // Window properties
    flags: Qt.Window | Qt.WindowCloseButtonHint
    modality: Qt.NonModal
    width: 736
    height: 593
    minimumWidth: 736
    minimumHeight: 593
    color: Theme.bgElevated

    // Title derived from presetTier
    title: {
        if (presetTier === "printer") return qsTr("打印机设置")
        if (presetTier === "filament") return qsTr("材料设置")
        if (presetTier === "print") return qsTr("工艺设置")
        return qsTr("Settings")
    }

    // Internal state
    property string activeTab: ""
    property string selectedGroup: ""
    property string searchText: ""
    property bool advancedMode: false
    property bool closeAfterSaveAs: false
    property var filteredIndices: []

    // Tab pages per tier. The label is visual text; key stays aligned with upstream page ids.
    readonly property var tabPages: {
        if (presetTier === "printer") return [
            { key: "Basic information", label: qsTr("基础信息") },
            { key: "Machine G-code", label: qsTr("打印机G-code") },
            { key: "Material", label: qsTr("材料") },
            { key: "Extruder", label: qsTr("挤出机") },
            { key: "Motion ability", label: qsTr("移动能力") },
            { key: "Notes", label: qsTr("注释") }
        ]
        if (presetTier === "filament") return [
            { key: "Filament", label: qsTr("耗材丝") },
            { key: "Cooling", label: qsTr("冷却") },
            { key: "Overrides", label: qsTr("参数覆盖") },
            { key: "Advanced", label: qsTr("高级") },
            { key: "Multimaterial", label: qsTr("材料") },
            { key: "Dependencies", label: qsTr("依赖") },
            { key: "Notes", label: qsTr("注释") }
        ]
        if (presetTier === "print") return [
            { key: "Quality", label: qsTr("质量") },
            { key: "Strength", label: qsTr("强度") },
            { key: "Speed", label: qsTr("速度") },
            { key: "Support", label: qsTr("支撑") },
            { key: "Base", label: qsTr("底板") },
            { key: "Cooling", label: qsTr("冷却") },
            { key: "Retraction", label: qsTr("回抽") },
            { key: "Other", label: qsTr("其他") }
        ]
        return []
    }

    // Current preset names list per tier
    readonly property var presetNames: {
        if (presetTier === "printer") return configVm ? configVm.printerPresetNames : []
        if (presetTier === "filament") return configVm ? configVm.filamentPresetNames : []
        if (presetTier === "print") return configVm ? configVm.printPresetNames : []
        return []
    }

    // Current preset name per tier
    readonly property string currentPreset: {
        if (presetTier === "printer") return configVm ? configVm.currentPrinterPreset : ""
        if (presetTier === "filament") return configVm ? configVm.currentFilamentPreset : ""
        if (presetTier === "print") return configVm ? configVm.currentPrintPreset : ""
        return ""
    }

    // Set default tab on first load
    Component.onCompleted: {
        if (tabPages.length > 0) activeTab = tabPages[0].key
        selectedGroup = qsTr("All")
        rebuildFilter()
    }

    // Rebuild filtered indices when search/group/tab changes
    function rebuildFilter() {
        if (!configVm || !optionModel) { filteredIndices = []; return }
        var indices = configVm.filterOptionIndices(presetTier, searchText, advancedMode)
        if (activeTab !== "")
            indices = optionModel.filterIndicesByPage(indices, activeTab)
        if (selectedGroup !== "" && selectedGroup !== qsTr("All"))
            indices = optionModel.filterIndicesByGroup(indices, selectedGroup)
        filteredIndices = indices
    }

    function requestSaveAndMaybeClose(closeOnSuccess) {
        if (!configVm) return false
        closeAfterSaveAs = closeOnSuccess
        var ok = configVm.requestSavePendingChanges()
        if (ok) {
            closeAfterSaveAs = false
            if (closeOnSuccess)
                root.close()
        }
        return ok
    }

    onSearchTextChanged: rebuildFilter()
    onAdvancedModeChanged: rebuildFilter()
    onActiveTabChanged: rebuildFilter()
    onSelectedGroupChanged: rebuildFilter()

    // Tier to category index (0=print, 1=filament, 2=printer)
    readonly property int tierCategory: {
        if (presetTier === "printer") return 2
        if (presetTier === "filament") return 1
        return 0
    }

    // Preset selection changed
    function onPresetActivated(presetName) {
        if (!configVm) return
        if (presetTier === "printer") configVm.requestCurrentPrinterPreset(presetName)
        else if (presetTier === "filament") configVm.requestCurrentFilamentPreset(presetName)
        else if (presetTier === "print") configVm.requestCurrentPrintPreset(presetName)
    }

    // Dirty-guarded close
    function attemptClose() {
        if (configVm && configVm.isPresetDirty) {
            unsavedDialog.openDialog()
        } else {
            root.close()
        }
    }

    // Show function (non-modal dialog must requestActivate)
    function show() {
        visible = true
        requestActivate()
    }

    // Close handler with dirty guard
    onClosing: function(close) {
        if (configVm && configVm.isPresetDirty) {
            close.accepted = false
            unsavedDialog.openDialog()
        }
    }

    // Esc key handler
    Shortcut {
        sequence: "Escape"
        onActivated: root.attemptClose()
    }

    // Unsaved changes dialog instance (scoped to this dialog)
    UnsavedChangesDialog {
        id: unsavedDialog
        configVm: root.configVm
        presetTier: root.presetTier

        onAccepted: {
            if (unsavedDialog.action === "save") {
                root.requestSaveAndMaybeClose(true)
            } else if (unsavedDialog.action === "discard") {
                root.configVm.requestDiscardPendingChanges()
                root.close()
            }
            // "cancel" -> do nothing, dialog stays open
        }
    }

    // SavePresetDialog instance (scoped to this dialog)
    SavePresetDialog {
        id: saveAsDialog
        configVm: root.configVm
        presetTier: root.presetTier
        onAccepted: {
            if (root.closeAfterSaveAs) {
                root.closeAfterSaveAs = false
                root.close()
            }
        }
        onRejected: root.closeAfterSaveAs = false
    }

    Connections {
        target: root.configVm
        function onSaveAsRequired() {
            saveAsDialog.open()
        }
    }

    // Removed dead deleteConfirmDialog/resetAllConfirmDialog: their openers
    // (Preset bar Delete/Reset All buttons) were removed in the compact-layout
    // refactor. Preset deletion now goes through the sidebar's per-row edit
    // affordance; Reset is per-group via OptionRow's reset control.

    // Dialog layout (top to bottom)
    Rectangle {
        anchors.fill: parent
        color: Theme.bgElevated

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // 1. Preset and action bar
            Rectangle {
                Layout.fillWidth: true
                height: 44
                color: Theme.chromeSurface

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 8
                    spacing: 6

                    // Preset selector
                    CxComboBox {
                        Layout.fillWidth: true
                        model: root.presetNames
                        currentIndex: {
                            var idx = root.presetNames.indexOf(root.currentPreset)
                            return idx >= 0 ? idx : 0
                        }
                        onActivated: (i) => {
                            if (i >= 0 && i < root.presetNames.length)
                                root.onPresetActivated(root.presetNames[i])
                        }
                    }

                    // Modified badge
                    Rectangle {
                        visible: root.configVm && root.configVm.isPresetDirty
                        height: 22
                        radius: Theme.radiusSM
                        color: Theme.bgWarningSubtle
                        width: modBadge.implicitWidth + 16
                        Text {
                            id: modBadge
                            anchors.centerIn: parent
                            text: qsTr("Modified")
                            color: Theme.statusWarning
                            font.pixelSize: Theme.fontSizeXS
                            font.bold: true
                        }
                    }

                    // Compatibility warning chip (shown when preset combination is invalid)
                    Rectangle {
                        visible: root.configVm && !root.configVm.currentPresetCombinationValid
                        height: 22
                        radius: Theme.radiusSM
                        color: Theme.bgErrorSubtle
                        width: compatBadge.implicitWidth + 16
                        Text {
                            id: compatBadge
                            anchors.centerIn: parent
                            text: root.configVm && root.configVm.currentPresetCompatibilityMessage
                                  ? root.configVm.currentPresetCompatibilityMessage
                                  : qsTr("Incompatible")
                            color: Theme.statusError
                            font.pixelSize: Theme.fontSizeXS
                            font.bold: true
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }
                        ToolTip.visible: compatHover.containsMouse
                        ToolTip.text: root.configVm ? root.configVm.currentPresetCompatibilityMessage : ""
                        MouseArea {
                            id: compatHover
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                    }

                    CxTextField {
                        id: topSearchField
                        placeholderText: qsTr("Search")
                        implicitWidth: 132
                        onTextChanged: root.searchText = text
                    }

                    RowLayout {
                        spacing: 2
                        Text {
                            text: qsTr("Advanced")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeXS
                        }
                        CxSwitch {
                            checked: root.advancedMode
                            onToggled: root.advancedMode = checked
                        }
                    }

                    CxButton {
                        text: qsTr("Save")
                        height: 28
                        compact: true
                        cxStyle: CxButton.Style.Primary
                        visible: root.configVm && root.configVm.isPresetDirty
                        enabled: root.configVm && root.configVm.isPresetDirty
                        onClicked: root.requestSaveAndMaybeClose(false)
                    }

                    // Save As button
                    CxButton {
                        text: qsTr("Save As...")
                        height: 28
                        compact: true
                        onClicked: saveAsDialog.open()
                    }

                    // Close button
                    Rectangle {
                        width: 28
                        height: 28
                        radius: Theme.radiusSM
                        color: closeMouseArea.containsMouse ? Theme.chromeDangerHover : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "✕"
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSizeMD
                        }

                        MouseArea {
                            id: closeMouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.attemptClose()
                        }
                    }
                }
            }

            // 2. Tab strip
            Rectangle {
                Layout.fillWidth: true
                height: 38
                color: Theme.bgPanel

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    Repeater {
                        model: root.tabPages

                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: tabHov.containsMouse ? Theme.bgHover : "transparent"

                            // Active tab underline
                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 2
                                color: root.activeTab === modelData.key ? Theme.accent : "transparent"
                            }

                            Text {
                                anchors.centerIn: parent
                                text: modelData.label
                                color: root.activeTab === modelData.key ? Theme.accent : Theme.textSecondary
                                font.pixelSize: Theme.fontSizeSM
                                font.bold: root.activeTab === modelData.key
                                elide: Text.ElideRight
                            }

                            HoverHandler { id: tabHov }

                            TapHandler {
                                onTapped: {
                                    root.activeTab = modelData.key
                                    root.selectedGroup = qsTr("All")
                                }
                            }
                        }
                    }
                }
            }

            // 3. Main content area
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // Option editing area
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Theme.bgBase

                    ListView {
                        id: optionList
                        anchors.fill: parent
                        clip: true
                        model: root.filteredIndices
                        spacing: 0

                        ScrollBar.vertical: ScrollBar {
                            visible: optionList.contentHeight > optionList.height
                        }

                        // Empty state
                        Text {
                            anchors.centerIn: parent
                            visible: root.filteredIndices.length === 0
                            text: root.searchText !== "" ? qsTr("No matching options")
                                                         : qsTr("No options")
                            color: Theme.textDisabled
                            font.pixelSize: Theme.fontSizeMD
                        }

                        delegate: Item {
                            id: optDelegate
                            required property int index
                            required property var modelData

                            readonly property int optIdx: modelData
                            readonly property string optGroup: root.optionModel ? root.optionModel.optGroup(optIdx) : ""

                            // Show group header when group changes
                            readonly property bool showGroupHeader: {
                                if (root.selectedGroup !== "" && root.selectedGroup !== qsTr("All"))
                                    return false // group filter active, no need for headers
                                if (optDelegate.index === 0) return optGroup !== ""
                                var prevGroup = root.optionModel
                                    ? root.optionModel.optGroup(root.filteredIndices[optDelegate.index - 1]) : ""
                                return optGroup !== "" && optGroup !== prevGroup
                            }

                            width: optionList.width
                            height: optRow.totalHeight

                            // OptionRow inlined in the delegate (not via Loader/Component)
                            // so its bindings resolve the delegate's scope (optDelegate).
                            OptionRow {
                                id: optRow
                                anchors.left: parent.left
                                anchors.right: parent.right
                                optionModel: root.optionModel
                                optIdx: optDelegate.optIdx
                                rowIndex: optDelegate.index
                                searchText: root.searchText
                                showGroupHeader: optDelegate.showGroupHeader
                                oGroup: optDelegate.optGroup
                                compact: true
                                compactLabelWidth: 210
                                compactFieldWidth: 96
                                compactEnumWidth: 190
                                valueSource: {
                                    if (!root.configVm || !root.optionModel) return ""
                                    var key = root.optionModel.optKey(optDelegate.optIdx)
                                    return root.configVm.valueSourceForKey(key)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
