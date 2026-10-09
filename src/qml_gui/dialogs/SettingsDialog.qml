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
import OWzx.Models 1.0
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
    property string searchText: ""
    property bool searchExpanded: false
    property bool advancedMode: false
    property bool closeAfterSaveAs: false
    property bool closeAfterUnsavedResolution: false

    // Presentation-only labels; C++ remains the hierarchy and ordering authority.
    function processDisplayLabel(sourceKey) {
        var labels = {
            "Quality": qsTr("Quality"),
            "Strength": qsTr("Strength"),
            "Speed": qsTr("Speed"),
            "Support": qsTr("Support"),
            "Multimaterial": qsTr("Multimaterial"),
            "Others": qsTr("Others"),
            "Layer height": qsTr("Layer height"),
            "Line width": qsTr("Line width"),
            "Seam": qsTr("Seam"),
            "Precision": qsTr("Precision"),
            "Ironing": qsTr("Ironing"),
            "Wall generator": qsTr("Wall generator"),
            "Walls and surfaces": qsTr("Walls and surfaces"),
            "Bridging": qsTr("Bridging"),
            "Overhangs": qsTr("Overhangs"),
            "Walls": qsTr("Walls"),
            "Top/bottom shells": qsTr("Top/bottom shells"),
            "Infill": qsTr("Infill"),
            "Advanced": qsTr("Advanced"),
            "Initial layer speed": qsTr("Initial layer speed"),
            "Other layers speed": qsTr("Other layers speed"),
            "Overhang speed": qsTr("Overhang speed"),
            "Travel speed": qsTr("Travel speed"),
            "Acceleration": qsTr("Acceleration"),
            "Jerk(XY)": qsTr("Jerk(XY)"),
            "Raft": qsTr("Raft"),
            "Support filament": qsTr("Support filament"),
            "Tree supports": qsTr("Tree supports"),
            "Prime tower": qsTr("Prime tower"),
            "Filament for Features": qsTr("Filament for Features"),
            "Ooze prevention": qsTr("Ooze prevention"),
            "Flush options": qsTr("Flush options"),
            "Skirt": qsTr("Skirt"),
            "Brim": qsTr("Brim"),
            "Special mode": qsTr("Special mode"),
            "G-code output": qsTr("G-code output"),
            "Post-processing Scripts": qsTr("Post-processing Scripts"),
            "Notes": qsTr("Notes")
        }
        return labels[sourceKey] || sourceKey
    }

    // Tab pages per tier. The label is visual text; key stays aligned with upstream page ids.
    // v5.16 (PSET2-08): only tabs with real option-model sources are listed.
    // The printer "Material"/"Extruder" pages and the filament "Overrides"/
    // "Multimaterial"/"Dependencies" pages have no keys assigned by
    // ConfigOptionModel's printer/filament page-group maps (kMachineKeys /
    // kFilamentKeys never map those page names), so they rendered zero rows.
    // Extruder/Overrides/Multimaterial need the per-extruder (multi-nozzle)
    // model — deferred to that scope. Dependencies would map
    // compatible_printers, which is not in kFilamentKeys/kMachineKeys
    // (it stays a preset-level compatibility field, not an edited row).
    readonly property var tabPages: {
        if (presetTier === "printer") return [
            { key: "Basic information", label: qsTr("基础信息") },
            { key: "Machine G-code", label: qsTr("打印机G-code") },
            { key: "Motion ability", label: qsTr("移动能力") },
            { key: "Notes", label: qsTr("注释") }
        ]
        if (presetTier === "filament") return [
            { key: "Filament", label: qsTr("耗材丝") },
            { key: "Cooling", label: qsTr("冷却") },
            { key: "Advanced", label: qsTr("高级") },
            { key: "Notes", label: qsTr("注释") }
        ]
        if (presetTier === "print") {
            var processPages = optionModel ? optionModel.processPageNames() : []
            return processPages.map(function(page) {
                return { key: page, label: root.processDisplayLabel(page) }
            })
        }
        return []
    }

    // Current preset names list per tier (v5.16 PSET2-05: decorated lists —
    // section separators + incompatibility gray-out suffixes; plain names
    // recover via configVm.plainPresetName).
    readonly property var presetNames: {
        if (presetTier === "printer") return configVm ? configVm.decoratedPrinterPresetNames : []
        if (presetTier === "filament") return configVm ? configVm.decoratedFilamentPresetNames : []
        if (presetTier === "print") return configVm ? configVm.decoratedPrintPresetNames : []
        return []
    }

    // Current preset name per tier
    readonly property string currentPreset: {
        if (presetTier === "printer") return configVm ? configVm.currentPrinterPreset : ""
        if (presetTier === "filament") return configVm ? configVm.currentFilamentPreset : ""
        if (presetTier === "print") return configVm ? configVm.currentPrintPreset : ""
        return ""
    }

    function presetIndexForName(name) {
        if (!configVm || !name) return -1
        for (var i = 0; i < presetNames.length; ++i) {
            if (configVm.plainPresetName(presetNames[i]) === name)
                return i
        }
        return -1
    }

    // Set default tab on first load
    Component.onCompleted: {
        if (tabPages.length > 0) activeTab = tabPages[0].key
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

    function openUnsavedChangesGuard(closeOnResolve) {
        // v5.16 (PSET2-03): three SettingsDialog instances share this
        // configVm and all listen to pendingUnsavedChangesRequested; the
        // begin/endUnsavedDialog gate makes only the first listener open
        // its modal (upstream shows one UnsavedChangesDialog at a time).
        if (root.configVm && !root.configVm.beginUnsavedDialog())
            return
        closeAfterUnsavedResolution = closeOnResolve
        unsavedDialog.openDialog()
    }

    // Tier to category index (0=print, 1=filament, 2=printer)
    readonly property int tierCategory: {
        if (presetTier === "printer") return 2
        if (presetTier === "filament") return 1
        return 0
    }

    // Preset selection changed
    function onPresetActivated(presetName) {
        if (!configVm) return
        // v5.16 (PSET2-05): presetName comes from the decorated display
        // list — normalize before dispatching.
        const plain = configVm.plainPresetName(presetName)
        if (presetTier === "printer") configVm.requestCurrentPrinterPreset(plain)
        else if (presetTier === "filament") configVm.requestCurrentFilamentPreset(plain)
        else if (presetTier === "print") configVm.requestCurrentPrintPreset(plain)
    }

    // Dirty-guarded close
    function attemptClose() {
        if (configVm && configVm.isPresetDirty) {
            root.openUnsavedChangesGuard(true)
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
            root.openUnsavedChangesGuard(true)
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
                root.requestSaveAndMaybeClose(root.closeAfterUnsavedResolution)
                root.closeAfterUnsavedResolution = false
            } else if (unsavedDialog.action === "discard") {
                root.configVm.requestDiscardPendingChanges()
                if (root.closeAfterUnsavedResolution)
                    root.close()
                root.closeAfterUnsavedResolution = false
            } else if (unsavedDialog.action === "transfer") {
                // v5.16 (PSET2-03): move the checked keys onto the pending
                // target preset, then proceed with the pending switch.
                if (root.configVm)
                    root.configVm.transferPendingChanges(unsavedDialog.checkedKeys)
                if (root.closeAfterUnsavedResolution)
                    root.close()
                root.closeAfterUnsavedResolution = false
            }
            // v5.16 (PSET2-03): release the single-modal gate.
            if (root.configVm)
                root.configVm.endUnsavedDialog()
        }

        onRejected: {
            if (root.configVm) {
                root.configVm.requestCancelPendingChanges()
                root.configVm.endUnsavedDialog()
            }
            root.closeAfterUnsavedResolution = false
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
        onRejected: {
            if (root.configVm && root.configVm.hasPendingUnsavedChanges)
                root.configVm.requestCancelPendingChanges()
            root.closeAfterSaveAs = false
        }
    }

    // Phase 147 (PSET-02): CreatePresetsDialog instance (scoped to this dialog).
    // Minimal source-truth port of upstream CreatePresetsDialog — scope selector +
    // inherits-from dropdown + name + create button. Opens via onCreatePresetRequired.
    CreatePresetsDialog {
        id: createPresetDialog
        configVm: root.configVm
        onAccepted: {
            // Refresh the preset list so the new entry appears; the viewmodel
            // emits stateChanged after createCustomPreset which re-evaluates the
            // Q_PROPERTYs bound above.
            root.closeAfterSaveAs = false
        }
    }

    // Phase 154 (CLOS-01): PresetDiffDialog instance (scoped to this dialog).
    // Minimal source-truth port of upstream UnsavedChangesDialog diff view mode
    // — side-by-side 3-column diff of two presets. Opens via
    // onComparePresetsRequired; consumes the existing
    // comparePresetsDetailed(A,B) primitive (Phase 149).
    PresetDiffDialog {
        id: comparePresetsDialog
        configVm: root.configVm
    }

    Connections {
        target: root.configVm
        function onSaveAsRequired() {
            saveAsDialog.open()
        }
        // Phase 147 (PSET-02): create-preset flow. Opens CreatePresetsDialog.
        function onCreatePresetRequired() {
            createPresetDialog.open()
        }
        // Phase 154 (CLOS-01): compare-presets flow. Opens PresetDiffDialog.
        function onComparePresetsRequired() {
            comparePresetsDialog.open()
        }
        function onPendingUnsavedChangesRequested() {
            root.openUnsavedChangesGuard(false)
        }
    }

    // G7: overflow actions for the compact preset/action bar. The upstream top
    // bar keeps undo/undo-to-sys/save/search/mode (Tab.cpp:406-441) and comments
    // the compare button out (Tab.cpp:278); Save-As and Compare stay reachable
    // here without compressing the row.
    CxMenu {
        id: presetOverflowMenu

        CxMenuItem {
            text: qsTr("另存为")
            onTriggered: saveAsDialog.open()
        }
        CxMenuItem {
            // Phase 154 (CLOS-01): compare presets entry. Opens
            // PresetDiffDialog via ConfigViewModel.requestComparePresets
            // (emits comparePresetsRequired → Connections handler).
            text: qsTr("比较预设")
            enabled: root.configVm && root.presetNames && root.presetNames.length > 1
            onTriggered: {
                if (root.configVm)
                    root.configVm.requestComparePresets()
            }
        }
    }

    // Removed dead deleteConfirmDialog/resetAllConfirmDialog: their openers
    // (Preset bar Delete/Reset All buttons) were removed in the compact-layout
    // refactor. Preset deletion goes through the sidebar's per-row "⋮" edit
    // menu (LeftSidebar, PSET2-07). There is no per-row reset control in
    // OptionRow; resetting is per-group via ConfigViewModel::resetGroup,
    // which currently has no settings-dialog button (upstream renders a
    // reset button per group header — wiring it is deferred).

    // Dialog layout (top to bottom)
    Rectangle {
        anchors.fill: parent
        color: Theme.bgElevated

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.spacingXS
            // 1. Preset and action bar
            Rectangle {
                Layout.fillWidth: true
                height: 44
                color: Theme.chromeSurface

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingMD
                    anchors.rightMargin: Theme.spacingMD
                    spacing: Theme.spacingSM
                    // Preset selector
                    CxComboBox {
                        Layout.fillWidth: true
                        // G7: upstream preset combo min width 35em (Tab.cpp:821-823
                        // explicit-resize block, em≈10 → 350px) so the fixed action
                        // set can no longer compress the selector (754px > 720px).
                        Layout.minimumWidth: 350
                        model: root.presetNames
                        currentIndex: {
                            var idx = root.presetIndexForName(root.currentPreset)
                            return idx >= 0 ? idx : 0
                        }
                        onActivated: (i) => {
                            if (i >= 0 && i < root.presetNames.length)
                                root.onPresetActivated(root.presetNames[i])
                        }
                    }

                    // Phase 236 (DLG-01): create-preset entry. Upstream preset
                    // combos embed an "add" affordance that opens
                    // CreatePresetsDialog; here the "+" button requests
                    // ConfigViewModel::requestCreatePreset, which emits
                    // createPresetRequired -> onCreatePresetRequired above.
                    CxIconButton {
                        buttonSize: 28
                        iconSize: 15
                        cxStyle: CxIconButton.Style.Ghost
                        iconSource: "qrc:/qml/assets/icons/plus.svg"
                        toolTipText: qsTr("新建预设")
                        enabled: root.configVm !== null
                        onClicked: {
                            if (root.configVm)
                                root.configVm.requestCreatePreset()
                        }
                    }

                    // Compact dirty marker.
                    Rectangle {
                        visible: root.configVm && root.configVm.isPresetDirty
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: Theme.radiusSM
                        color: Theme.statusWarning
                        ToolTip.visible: dirtyHover.containsMouse
                        ToolTip.text: qsTr("预设已修改")
                        MouseArea {
                            id: dirtyHover
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                    }

                    // Compact compatibility marker.
                    Rectangle {
                        visible: root.configVm && !root.configVm.currentPresetCombinationValid
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: Theme.radiusSM
                        color: Theme.statusError
                        ToolTip.visible: compatHover.containsMouse
                        ToolTip.text: root.configVm && root.configVm.currentPresetCompatibilityMessage
                                      ? root.configVm.currentPresetCompatibilityMessage
                                      : qsTr("预设不兼容")
                        MouseArea {
                            id: compatHover
                            anchors.fill: parent
                            hoverEnabled: true
                        }
                    }

                    CxIconButton {
                        buttonSize: 28
                        iconSize: 15
                        cxStyle: CxIconButton.Style.Ghost
                        iconSource: "qrc:/qml/assets/icons/restore.svg"
                        enabled: root.configVm && root.configVm.globalModifiedCount > 0
                        toolTipText: qsTr("恢复系统值")
                        onClicked: {
                            if (root.configVm)
                                root.configVm.restoreAllSystemValues()
                        }
                    }

                    CxIconButton {
                        buttonSize: 28
                        iconSize: 15
                        cxStyle: CxIconButton.Style.Ghost
                        iconSource: "qrc:/qml/assets/icons/device-floppy.svg"
                        selected: root.configVm && root.configVm.isPresetDirty
                        enabled: root.configVm && root.configVm.isPresetDirty
                        toolTipText: qsTr("保存")
                        onClicked: root.requestSaveAndMaybeClose(false)
                    }

                    // G7: Save-As and Compare move into the overflow menu
                    // (presetOverflowMenu below). Upstream keeps the tab top bar to
                    // undo/undo-to-sys/save/search/mode (Tab.cpp:406-441) and has
                    // the compare button commented out (Tab.cpp:278).
                    CxIconButton {
                        buttonSize: 28
                        iconSize: 15
                        cxStyle: CxIconButton.Style.Ghost
                        iconSource: "qrc:/qml/assets/icons/dots.svg"
                        toolTipText: qsTr("更多操作")
                        onClicked: presetOverflowMenu.popup(this, 0, this.height)
                    }

                    CxIconButton {
                        buttonSize: 28
                        iconSize: 15
                        cxStyle: CxIconButton.Style.Ghost
                        iconSource: "qrc:/qml/assets/icons/search.svg"
                        selected: root.searchExpanded || root.searchText.length > 0
                        toolTipText: qsTr("搜索")
                        onClicked: {
                            root.searchExpanded = !root.searchExpanded
                            if (!root.searchExpanded)
                                root.searchText = ""
                        }
                    }

                    CxTextField {
                        id: compactSearchField
                        visible: root.searchExpanded || root.searchText.length > 0
                        // G7: upstream search box is a proportion-1 flexible
                        // StaticBox with no fixed width (Tab.cpp:321); 124 keeps
                        // the expanded row ≤720px next to the 35em combo floor.
                        Layout.preferredWidth: visible ? 124 : 0
                        Layout.preferredHeight: 28
                        opacity: visible ? 1 : 0
                        placeholderText: qsTr("搜索")
                        text: root.searchText
                        onTextChanged: {
                            if (root.searchText !== text)
                                root.searchText = text
                        }
                    }

                    CxSwitch {
                        text: ""
                        checked: root.advancedMode
                        Layout.preferredWidth: 42
                        Layout.preferredHeight: 24
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("高级模式")
                        onToggled: root.advancedMode = checked
                    }
                }
            }

            // 2. Process tab strip
            Rectangle {
                id: processTabStrip
                Layout.fillWidth: true
                height: 38
                visible: root.presetTier === "print"
                color: Theme.bgPanel

                TabBar {
                    id: processTabBar
                    anchors.fill: parent
                    spacing: Theme.spacingXS
                    background: Rectangle { color: Theme.bgPanel }
                    currentIndex: root.tabPages.length > 0
                                  ? Math.max(0, root.tabPages.map(function(page) { return page.key }).indexOf(root.activeTab))
                                  : 0
                    onCurrentIndexChanged: {
                        var tab = itemAt(currentIndex)
                        if (tab)
                            root.activeTab = tab.pageKey
                    }

                    Repeater {
                        model: root.tabPages

                        delegate: TabButton {
                            id: processTab
                            required property var modelData
                            required property int index
                            readonly property string pageKey: modelData.key
                            // G-12: size from the outer strip, not the
                            // TabBar itself -- referencing processTabBar.width
                            // closed a binding loop (implicitWidth aggregates
                            // the child buttons).
                            width: processTabStrip.width > 0
                                   ? (processTabStrip.width - processTabBar.spacing * (processTabBar.count - 1))
                                     / processTabBar.count
                                   : 0
                            height: 38
                            text: modelData.label
                            focusPolicy: Qt.StrongFocus
                            Accessible.name: processTab.text
                            Accessible.role: Accessible.PageTab
                            Accessible.focusable: true
                            Accessible.selected: root.activeTab === processTab.pageKey
                            Accessible.description: qsTr("%1 of %2").arg(index + 1).arg(processTabBar.count)

                            contentItem: Text {
                                text: processTab.text
                                // G4: active tab label is white (R12); the accent
                                // stays on the 2px underline below.
                                color: root.activeTab === processTab.pageKey ? Theme.textPrimary : Theme.textSecondary
                                font.pixelSize: root.activeTab === processTab.pageKey ? Theme.fontSizeMD : Theme.fontSizeSM
                                font.weight: root.activeTab === processTab.pageKey ? Font.DemiBold : Font.Normal
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                maximumLineCount: 1
                            }

                            background: Rectangle {
                                color: processTab.hovered ? Theme.bgHover : "transparent"
                                border.width: processTab.activeFocus ? 1 : 0
                                border.color: Theme.borderFocus

                                Rectangle {
                                    anchors.bottom: parent.bottom
                                    width: parent.width
                                    height: 2
                                    color: root.activeTab === processTab.pageKey ? Theme.accent : "transparent"
                                }
                            }

                            onClicked: root.activeTab = pageKey

                            Keys.onPressed: function(event) {
                                var targetIndex = index
                                if (event.key === Qt.Key_Left) {
                                    targetIndex = Math.max(0, index - 1)
                                } else if (event.key === Qt.Key_Right) {
                                    targetIndex = Math.min(processTabBar.count - 1, index + 1)
                                } else if (event.key === Qt.Key_Home) {
                                    targetIndex = 0
                                } else if (event.key === Qt.Key_End) {
                                    targetIndex = processTabBar.count - 1
                                } else if (event.key !== Qt.Key_Return
                                           && event.key !== Qt.Key_Enter
                                           && event.key !== Qt.Key_Space) {
                                    return
                                }

                                processTabBar.currentIndex = targetIndex
                                var targetTab = processTabBar.itemAt(targetIndex)
                                if (targetTab)
                                    targetTab.forceActiveFocus()
                                event.accepted = true
                            }
                        }
                    }
                }
            }

            // 2. Non-Process tab strip
            Rectangle {
                id: genericTabStrip
                Layout.fillWidth: true
                height: 38
                visible: root.presetTier !== "print"
                color: Theme.bgPanel

                RowLayout {
                    anchors.fill: parent
                    spacing: Theme.spacingXS
                    Repeater {
                        model: root.tabPages

                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            color: tabHov.containsMouse ? Theme.bgHover : "transparent"

                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 2
                                color: root.activeTab === modelData.key ? Theme.accent : "transparent"
                            }

                            Text {
                                anchors.centerIn: parent
                                text: modelData.label
                                // G4: active tab label is white (R12); the accent
                                // stays on the 2px underline below.
                                color: root.activeTab === modelData.key ? Theme.textPrimary : Theme.textSecondary
                                font.pixelSize: Theme.fontSizeSM
                                font.bold: root.activeTab === modelData.key
                                elide: Text.ElideRight
                            }

                            HoverHandler { id: tabHov }

                            TapHandler {
                                onTapped: root.activeTab = modelData.key
                            }
                        }
                    }
                }
            }

            // 3. Main content area
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.spacingXS
                // Option editing area
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Theme.bgBase

                    // G6: styled scroll chrome comes from CxScrollView (U01 token
                    // values); the bare ScrollBar.vertical overrides are gone.
                    CxScrollView {
                        anchors.fill: parent

                        ListView {
                            id: processOptionListComponent
                            anchors.fill: parent
                            visible: root.presetTier === "print"
                            clip: true
                            model: root.optionModel && root.activeTab !== ""
                                   ? root.optionModel.processGroupsForPage(root.activeTab) : []
                            // C++-side projection count: search + page filter plus
                            // (page, group) process-manifest membership — exactly
                            // the rows the per-group proxies below accept.
                            readonly property bool hasProjectedRows:
                                root.optionModel
                                ? root.optionModel.processProjectedRowCount(
                                      root.activeTab, root.searchText, root.advancedMode) > 0
                                : false
                            // G3: R11 row rhythm — 30px rows touch (pitch 30, was 34+4=38).
                            spacing: 0

                            Text {
                                anchors.centerIn: parent
                                visible: !processOptionListComponent.hasProjectedRows
                                text: qsTr("No options")
                                color: Theme.textDisabled
                                font.pixelSize: Theme.fontSizeMD
                            }

                            delegate: Item {
                                id: processGroupDelegate
                                required property var modelData
                                required property int index
                                readonly property string groupName: modelData

                                // Per-group filtered + upstream-manifest-ordered
                                // projection over the option model. count hides
                                // empty groups and drives the inner Repeater.
                                readonly property ConfigOptionFilterProxy groupRows: ConfigOptionFilterProxy {
                                    sourceModel: root.optionModel
                                    page: root.activeTab
                                    group: processGroupDelegate.groupName
                                    searchText: root.searchText
                                    advancedMode: root.advancedMode
                                    upstreamProcessOrder: true
                                }

                                visible: groupRows.count > 0
                                width: processOptionListComponent.width
                                height: visible ? processGroupColumn.implicitHeight : 0

                                Column {
                                    id: processGroupColumn
                                    width: parent.width
                                    // G3: R11 — keep the 30px row pitch inside process
                                    // groups too (the 28px header block carries the
                                    // group separation).
                                    spacing: 0

                                    Rectangle {
                                        width: parent.width
                                        height: 28
                                        color: "transparent"

                                        Text {
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.leftMargin: Theme.spacingMD
                                            anchors.rightMargin: Theme.spacingMD
                                            anchors.verticalCenter: parent.verticalCenter
                                            text: root.processDisplayLabel(processGroupDelegate.groupName)
                                            color: Theme.textSecondary
                                            font.pixelSize: Theme.fontSizeMD
                                            font.weight: Font.DemiBold
                                            elide: Text.ElideRight
                                        }

                                        Rectangle {
                                            anchors.left: parent.left
                                            anchors.right: parent.right
                                            anchors.bottom: parent.bottom
                                            height: 1
                                            color: Theme.borderSubtle
                                        }
                                    }

                                    Repeater {
                                        model: processGroupDelegate.groupRows

                                        // Wrapper delegate (same pattern as
                                        // optDelegate below): OptionRow declares
                                        // required properties, so the Repeater can
                                        // no longer inject modelData/index as
                                        // context roles -- unqualified references
                                        // here resolved to the outer group
                                        // delegate's scope instead (a QString for
                                        // index -> "Unable to assign QString to
                                        // int" on every row).
                                        delegate: Item {
                                            id: processOptionDelegate
                                            required property int index
                                            required property int optIdx
                                            required property string optPrevGroup
                                            required property string optType
                                            required property string optKey
                                            required property string displayLabel
                                            required property var optValue
                                            required property double optMin
                                            required property double optMax
                                            required property double optStep
                                            required property bool optReadonly
                                            required property bool optDirty
                                            required property string optTooltip
                                            required property string optUnit
                                            required property string optSidetext
                                            required property bool optNullable
                                            required property bool optIsVector
                                            required property var optEnumLabels
                                            required property string valueSource

                                            width: processGroupColumn.width
                                            height: processOptRow.totalHeight

                                            OptionRow {
                                                id: processOptRow
                                                anchors.left: parent.left
                                                anchors.right: parent.right
                                                optionModel: root.optionModel
                                                optIdx: processOptionDelegate.optIdx
                                                rowIndex: processOptionDelegate.index
                                                searchText: root.searchText
                                                showGroupHeader: false
                                                oGroup: processGroupDelegate.groupName
                                                compact: true
                                                compactLabelWidth: 210
                                                compactFieldWidth: 119
                                                compactEnumWidth: 190
                                                valueSource: processOptionDelegate.valueSource
                                                oType: processOptionDelegate.optType
                                                oKey: processOptionDelegate.optKey
                                                oLabel: processOptionDelegate.displayLabel
                                                oVal: processOptionDelegate.optValue
                                                oMin: processOptionDelegate.optMin
                                                oMax: processOptionDelegate.optMax
                                                oStep: processOptionDelegate.optStep
                                                oRO: processOptionDelegate.optReadonly
                                                oDirty: processOptionDelegate.optDirty
                                                oTip: processOptionDelegate.optTooltip
                                                oUnit: processOptionDelegate.optUnit
                                                oSidetext: processOptionDelegate.optSidetext
                                                oNullable: processOptionDelegate.optNullable
                                                oIsVector: processOptionDelegate.optIsVector
                                                oEnumLabels: processOptionDelegate.optEnumLabels
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // G6: styled scroll chrome comes from CxScrollView (U01 token
                    // values); the bare ScrollBar.vertical overrides are gone.
                    CxScrollView {
                        anchors.fill: parent

                        ListView {
                            id: genericOptionListComponent
                            anchors.fill: parent
                            visible: root.presetTier !== "print"
                            clip: true
                            // print tier skips the page filter (all pages shown in
                            // one list), matching the legacy filterOptionIndices
                            // page-skip; printer/filament narrow by active tab.
                            model: ConfigOptionFilterProxy {
                                id: genericListProxy
                                sourceModel: root.optionModel
                                page: root.presetTier !== "print" ? root.activeTab : ""
                                searchText: root.searchText
                                advancedMode: root.advancedMode
                            }
                            // G3: R11 row rhythm — 30px rows touch (pitch 30, was 34+4=38).
                            spacing: 0

                            // Empty state
                            Text {
                                anchors.centerIn: parent
                                visible: genericListProxy.count === 0
                                text: root.searchText !== "" ? qsTr("No matching options")
                                                             : qsTr("No options")
                                color: Theme.textDisabled
                                font.pixelSize: Theme.fontSizeMD
                            }

                            delegate: Item {
                                id: optDelegate
                                required property int index
                                required property int optIdx
                                required property string optGroup
                                required property string optPrevGroup
                                required property string optType
                                required property string optKey
                                required property string displayLabel
                                required property var optValue
                                required property double optMin
                                required property double optMax
                                required property double optStep
                                required property bool optReadonly
                                required property bool optDirty
                                required property string optTooltip
                                required property string optUnit
                                required property string optSidetext
                                required property bool optNullable
                                required property bool optIsVector
                                required property var optEnumLabels
                                required property string valueSource

                                // Show group header when group changes
                                readonly property bool showGroupHeader:
                                    optGroup !== "" && (index === 0 || optGroup !== optPrevGroup)

                                width: genericOptionListComponent.width
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
                                    compactFieldWidth: 119
                                    compactEnumWidth: 190
                                    valueSource: optDelegate.valueSource
                                    oType: optDelegate.optType
                                    oKey: optDelegate.optKey
                                    oLabel: optDelegate.displayLabel
                                    oVal: optDelegate.optValue
                                    oMin: optDelegate.optMin
                                    oMax: optDelegate.optMax
                                    oStep: optDelegate.optStep
                                    oRO: optDelegate.optReadonly
                                    oDirty: optDelegate.optDirty
                                    oTip: optDelegate.optTooltip
                                    oUnit: optDelegate.optUnit
                                    oSidetext: optDelegate.optSidetext
                                    oNullable: optDelegate.optNullable
                                    oIsVector: optDelegate.optIsVector
                                    oEnumLabels: optDelegate.optEnumLabels
                                }
                            }
                        }
                    }
                }
            }

            // SETPRINT-FOOTER (upstream Tab.cpp dialog bottom button row):
            // Save / Discard / Close. Save and Discard act on the pending
            // preset edits (gated on the dirty flag like the preset-bar
            // save icon); Close runs the same UnsavedChangesDialog guard
            // as the window close (openUnsavedChangesGuard(true)).
            // G1: the footer is the outer ColumnLayout's full-width 44px
            // bottom bar (upstream ParamsPanel.cpp:402-495 single vertical
            // stack); the option area above is the RowLayout's only
            // fillWidth/fillHeight child.
            Rectangle {
                Layout.fillWidth: true
                height: 44
                color: Theme.chromeSurface

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingMD
                    anchors.rightMargin: Theme.spacingMD
                    spacing: Theme.spacingSM

                    Item { Layout.fillWidth: true }

                    CxButton {
                        text: qsTr("保存")
                        enabled: root.configVm && root.configVm.isPresetDirty
                        onClicked: root.requestSaveAndMaybeClose(false)
                    }
                    CxButton {
                        text: qsTr("放弃")
                        enabled: root.configVm && root.configVm.isPresetDirty
                        onClicked: if (root.configVm) root.configVm.requestDiscardPendingChanges()
                    }
                    CxButton {
                        text: qsTr("关闭")
                        onClicked: root.openUnsavedChangesGuard(true)
                    }
                }
            }
        }
    }
}
