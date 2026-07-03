// SettingsDialog.qml — non-modal ApplicationWindow settings dialog shell.
// Parameterized by presetTier ("printer"/"filament"/"print").
// Three instances, one per category. Reuses existing SavePresetDialog and
// UnsavedChangesDialog for save/dirty flows.
//
// Region IDs: SETPRINT-SHELL, SETMAT-SHELL, SETPROC-SHELL
//
// Note: per CONTEXT.md decision, old SettingsPage/ParamsPage/ConfigPage/SearchDialog
// are NOT removed here; cleanup is Phase 57.

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
    width: 920
    height: 640
    minimumWidth: 720
    minimumHeight: 480
    color: Theme.bgElevated

    // Title derived from presetTier
    title: {
        if (presetTier === "printer") return qsTr("Printer Settings")
        if (presetTier === "filament") return qsTr("Material Settings")
        if (presetTier === "print") return qsTr("Process Settings")
        return qsTr("Settings")
    }

    // Internal state
    property string activeTab: ""
    property string selectedGroup: ""
    property string searchText: ""
    property bool advancedMode: false
    property var filteredIndices: []

    // Tab labels per tier (upstream Tab.cpp)
    readonly property var tabLabels: {
        if (presetTier === "printer") return [
            qsTr("Basic information"), qsTr("Dependencies"), qsTr("Notes")
        ]
        if (presetTier === "filament") return [
            qsTr("Filament"), qsTr("Cooling"), qsTr("Advanced"),
            qsTr("Multimaterial"), qsTr("Dependencies"), qsTr("Notes")
        ]
        if (presetTier === "print") return [
            qsTr("Quality"), qsTr("Strength"), qsTr("Speed"),
            qsTr("Support"), qsTr("Multimaterial"), qsTr("Others")
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
        if (tabLabels.length > 0) activeTab = tabLabels[0]
        rebuildFilter()
    }

    // Rebuild filtered indices when search/group/tab changes
    function rebuildFilter() {
        if (!configVm || !optionModel) { filteredIndices = []; return }
        filteredIndices = configVm.filterOptionIndices(presetTier, searchText, advancedMode)
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
                root.configVm.requestSavePendingChanges()
                root.close()
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
    }

    // Delete preset confirmation dialog
    CxDialog {
        id: deleteConfirmDialog
        title: qsTr("Delete Preset")
        dialogTitle: qsTr("Delete")
        width: 400
        height: 160
        contentItem: Rectangle {
            color: Theme.bgPanel
            anchors.fill: parent
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 12
                Text {
                    text: qsTr("Delete preset '%1'?").arg(root.currentPreset)
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Text {
                    text: qsTr("This action cannot be undone.")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeSM
                    Layout.fillWidth: true
                }
                Item { Layout.fillHeight: true }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Item { Layout.fillWidth: true }
                    CxButton {
                        text: qsTr("Cancel")
                        onClicked: deleteConfirmDialog.reject()
                    }
                    CxButton {
                        text: qsTr("Delete")
                        cxStyle: CxButton.Style.Danger
                        onClicked: {
                            deleteConfirmDialog.accept()
                        }
                    }
                }
            }
        }
        onAccepted: {
            if (root.configVm) {
                root.configVm.deletePreset(root.tierCategory, root.currentPreset)
            }
        }
    }

    // Reset all confirmation dialog
    CxDialog {
        id: resetAllConfirmDialog
        title: qsTr("Reset All")
        dialogTitle: qsTr("Reset All")
        width: 400
        height: 160
        contentItem: Rectangle {
            color: Theme.bgPanel
            anchors.fill: parent
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 12
                Text {
                    text: qsTr("Reset all options to default values?")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Text {
                    text: qsTr("All modifications will be lost.")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeSM
                    Layout.fillWidth: true
                }
                Item { Layout.fillHeight: true }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Item { Layout.fillWidth: true }
                    CxButton {
                        text: qsTr("Cancel")
                        onClicked: resetAllConfirmDialog.reject()
                    }
                    CxButton {
                        text: qsTr("Reset")
                        cxStyle: CxButton.Style.Danger
                        onClicked: resetAllConfirmDialog.accept()
                    }
                }
            }
        }
        onAccepted: {
            if (root.configVm) root.configVm.resetAllGlobalOptions()
        }
    }

    // Dialog layout (top to bottom)
    Rectangle {
        anchors.fill: parent
        color: Theme.bgElevated

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // 1. Title bar
            Rectangle {
                Layout.fillWidth: true
                height: 40
                color: Theme.chromeSurface

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingXL
                    anchors.rightMargin: Theme.spacingMD
                    spacing: Theme.spacingMD

                    // Preset selector
                    CxComboBox {
                        width: 240
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

                    Item { Layout.fillWidth: true }

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

                    // Save As button
                    CxButton {
                        text: qsTr("Save As...")
                        height: 28
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

            // 2. Preset bar
            Rectangle {
                Layout.fillWidth: true
                height: Theme.controlHeightMD
                color: Theme.bgPanel

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingXL
                    anchors.rightMargin: Theme.spacingXL
                    spacing: Theme.spacingMD

                    CxButton {
                        text: qsTr("Save")
                        height: Theme.controlHeightSM
                        cxStyle: CxButton.Style.Primary
                        visible: root.configVm && !root.configVm.isPresetDirty
                        enabled: root.configVm && root.configVm.isPresetDirty
                        onClicked: root.configVm.requestSavePendingChanges()
                    }

                    CxButton {
                        text: qsTr("Delete")
                        height: Theme.controlHeightSM
                        cxStyle: CxButton.Style.Danger
                        visible: root.configVm && root.configVm.canDeletePreset(root.currentPreset)
                        onClicked: deleteConfirmDialog.open()
                    }

                    CxButton {
                        text: qsTr("Reset All")
                        height: Theme.controlHeightSM
                        cxStyle: CxButton.Style.Ghost
                        onClicked: resetAllConfirmDialog.open()
                    }

                    Item { Layout.fillWidth: true }

                    // Compatibility indicator
                    Text {
                        visible: root.configVm && !root.configVm.currentPresetCombinationValid
                        text: root.configVm ? root.configVm.currentPresetCompatibilityMessage : ""
                        color: Theme.statusError
                        font.pixelSize: Theme.fontSizeXS
                        elide: Text.ElideRight
                    }
                }
            }

            // 3. Tab strip
            Rectangle {
                Layout.fillWidth: true
                height: Theme.tabBarHeight
                color: Theme.bgPanel

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    Repeater {
                        model: root.tabLabels

                        delegate: Rectangle {
                            required property string modelData
                            required property int index
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: tabHov.containsMouse ? Theme.bgHover : "transparent"

                            // Active tab underline
                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 2
                                color: root.activeTab === modelData ? Theme.accent : "transparent"
                            }

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.activeTab === modelData ? Theme.accent : Theme.textSecondary
                                font.pixelSize: Theme.fontSizeMD
                                font.bold: root.activeTab === modelData
                            }

                            HoverHandler { id: tabHov }

                            TapHandler {
                                onTapped: {
                                    root.activeTab = modelData
                                    root.selectedGroup = ""
                                }
                            }
                        }
                    }
                }
            }

            // 4. Main content area (group nav + option list)
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // Group navigation sidebar
                GroupNavSidebar {
                    id: groupNav
                    Layout.preferredWidth: 200
                    Layout.fillHeight: true
                    optionModel: root.optionModel
                    groups: root.configVm ? root.configVm.groupNames(root.presetTier) : []
                    selectedGroup: root.selectedGroup
                    onGroupSelected: function(groupName) {
                        root.selectedGroup = groupName
                    }
                }

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
                                                         : qsTr("No options in this group")
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
                            height: optionRowLoader.totalHeight

                            Loader {
                                id: optionRowLoader
                                anchors.fill: parent
                                sourceComponent: optionRowComponent
                                readonly property int totalHeight: item ? item.totalHeight : 0
                            }
                        }

                        Component {
                            id: optionRowComponent

                            OptionRow {
                                optionModel: root.optionModel
                                optIdx: optDelegate.optIdx
                                rowIndex: optDelegate.index
                                searchText: root.searchText
                                showGroupHeader: optDelegate.showGroupHeader
                                oGroup: optDelegate.optGroup
                                valueSource: {
                                    if (!root.configVm || !optionModel) return ""
                                    var key = optionModel.optKey(optDelegate.optIdx)
                                    return root.configVm.valueSourceForKey(key)
                                }
                            }
                        }
                    }
                }
            }

            // 5. Search bar
            Rectangle {
                Layout.fillWidth: true
                height: 44
                color: Theme.bgSurface

                Rectangle {
                    anchors.top: parent.top
                    width: parent.width
                    height: 1
                    color: Theme.borderSubtle
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingXL
                    anchors.rightMargin: Theme.spacingXL
                    spacing: Theme.spacingMD

                    Text {
                        text: root.title + " - " + root.filteredIndices.length + " " + qsTr("items")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeLG
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    // Reset Group button
                    CxButton {
                        text: qsTr("Reset Group")
                        height: 28
                        cxStyle: CxButton.Style.Ghost
                        visible: root.selectedGroup !== "" && root.selectedGroup !== qsTr("All")
                        onClicked: root.configVm.resetGroup(root.presetTier, root.selectedGroup)
                    }

                    // Search input
                    CxTextField {
                        id: searchField
                        placeholderText: qsTr("Search options...")
                        implicitWidth: 200
                        onTextChanged: root.searchText = text
                    }

                    // Advanced toggle
                    RowLayout {
                        spacing: Theme.spacingXS
                        Text {
                            text: qsTr("Advanced")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSM
                        }
                        CxSwitch {
                            checked: root.advancedMode
                            onToggled: root.advancedMode = checked
                        }
                    }

                    // Match count
                    Text {
                        visible: root.searchText !== ""
                        text: root.filteredIndices.length + " " + qsTr("matched")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                    }

                    // Modified count
                    Text {
                        visible: root.optionModel && root.optionModel.dirtyCount > 0
                        text: root.optionModel.dirtyCount + " " + qsTr("modified")
                        color: Theme.statusWarning
                        font.pixelSize: Theme.fontSizeXS
                        font.bold: true
                    }
                }
            }

            // 6. Footer action bar
            Rectangle {
                Layout.fillWidth: true
                height: 44
                color: Theme.bgSurface

                // Top border
                Rectangle {
                    anchors.top: parent.top
                    width: parent.width
                    height: 1
                    color: Theme.borderSubtle
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.rightMargin: Theme.spacingXL
                    spacing: Theme.spacingMD

                    Item { Layout.fillWidth: true }

                    CxButton {
                        text: qsTr("Save")
                        cxStyle: CxButton.Style.Primary
                        enabled: root.configVm && root.configVm.isPresetDirty
                        onClicked: root.configVm.requestSavePendingChanges()
                    }

                    CxButton {
                        text: qsTr("Discard")
                        cxStyle: CxButton.Style.Secondary
                        onClicked: root.attemptClose()
                    }

                    CxButton {
                        text: qsTr("Cancel")
                        cxStyle: CxButton.Style.Ghost
                        onClicked: root.attemptClose()
                    }
                }
            }
        }
    }
}
