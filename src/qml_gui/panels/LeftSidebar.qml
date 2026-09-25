pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"
import "../components"

Rectangle {
    id: root
    required property var editorVm
    required property var configVm
    // G-07: filament compatibility comes from a Q_INVOKABLE (one-shot in
    // bindings). Bump this tick on every configVm state change so the row
    // decorations re-evaluate instead of keeping stale preset-compat state.
    property int compatRefreshTick: 0
    Connections {
        target: root.configVm
        function onStateChanged() { ++root.compatRefreshTick }
    }
    property string processCategory: ""
    signal exportRequested()

    // Sidebar filament slot rows (upstream add_filament/delete_filament,
    // Plater.cpp:5380,5393): seeded once from the editor extruder count,
    // then driven by the filament title-bar buttons. Every change mirrors
    // into configVm.setExtruderCount (upstream
    // update_multi_material_filament_presets slot-vector resize) so the
    // per-slot preset vector keeps matching the visible rows.
    property int filamentRowCount: 1
    Component.onCompleted: {
        filamentRowCount = Math.max(1, root.editorVm ? root.editorVm.extruderCount : 1)
    }

    readonly property int targetSidebarWidth: 392
    readonly property color panelSurface: Theme.bgElevated
    readonly property color sectionSurface: Theme.bgHover
    readonly property color controlSurface: Theme.borderDefault
    readonly property color fieldSurface: Theme.chromePressed
    readonly property color dividerColor: Theme.bgPressed
    readonly property color mutedText: Theme.textSecondary

    property string paramsCurrentTab: "Quality"
    property string paramsSearchText: ""
    readonly property var paramsOptionModel: {
        if (!root.configVm) return null
        return root.configVm.printOptions
    }
    readonly property string paramsTier: "print"
    property var paramsFilteredIndices: []

    color: panelSurface
    radius: 0
    border.width: 0

    function rebuildParamsFilter() {
        if (!root.configVm || !root.paramsOptionModel) {
            root.paramsFilteredIndices = []
            return
        }
        var indices = root.configVm.filterOptionIndices(
                    root.paramsTier, root.paramsSearchText, true)
        if (root.paramsCurrentTab !== "")
            indices = root.paramsOptionModel.filterIndicesByPage(indices, root.paramsCurrentTab)
        root.paramsFilteredIndices = indices
    }

    // Sidebar layout contract (upstream Plater.cpp:2425-2429,3245,3257): the
    // sidebar itself does not scroll as one column. Printer/filament/process
    // sections stay fixed; the params area fills the remaining height and
    // scrolls on its own.
    ColumnLayout {
        id: sidebarContent
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        spacing: 8

        PixelHeader {
            id: printerHeader
            Layout.fillWidth: true
            title: qsTr("打印机")
            iconSource: "qrc:/qml/assets/icons/printer.svg"
            // Upstream printer title row carries connect/sync/settings
            // (Plater.cpp:2475-2495). The upstream monitor_signal_strong /
            // printer_sync_not bitmaps are absent from the project icon set,
            // so the registered send (same dialog as the row-level connect
            // button) and rotate (sync) glyphs stand in.
            extraActions: [
                { icon: "qrc:/qml/assets/icons/send-2.svg", tooltip: qsTr("打印机连接") },
                { icon: "qrc:/qml/assets/icons/rotate-2.svg", tooltip: qsTr("同步打印机信息") }
            ]
            onExtraActionTriggered: (index) => {
                if (index === 0)
                    backend.showPrintHostDialog()
                else if (index === 1 && backend.monitorViewModel)
                    backend.monitorViewModel.refresh()
            }
            actionIcon: "qrc:/qml/assets/icons/settings.svg"
            actionToolTip: qsTr("打印机设置")
            onActionTriggered: backend.forwardSettingsRequest("printer")
            // Whole-bar fold toggle (upstream Plater.cpp:2515-2525); while
            // folded the title carries the preset name (Plater.cpp:2518-2521).
            collapsible: true
            collapsedDetail: root.configVm ? root.configVm.currentPrinterPreset : ""
        }

        // v5.14: compact preset row replaces the 76px hero card (screenshot
        // truth shows preset selectors as dense single rows, matching the
        // filament/process rows below). Hidden while the printer title bar
        // is folded (upstream m_panel_printer_content toggle,
        // Plater.cpp:2515-2525).
        Rectangle {
            id: printerPresetRow
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            visible: printerHeader.expanded
            radius: 4
            color: root.sectionSurface
            border.width: 1
            border.color: root.dividerColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                anchors.topMargin: 4
                anchors.bottomMargin: 4
                spacing: 6

                Image {
                    Layout.preferredWidth: 18
                    Layout.preferredHeight: 18
                    source: "qrc:/qml/assets/icons/printer.svg"
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.8
                }

                CxComboBox {
                    Layout.fillWidth: true
                    // Upstream preset combos are 30px tall
                    // (PresetComboBoxes.cpp:828, Plater.cpp:2595).
                    Layout.preferredHeight: 30
                    font.pixelSize: Theme.fontSizeSM
                    // v5.16 (PSET2-05): decorated list — section
                    // separators + incompatibility gray-out.
                    model: root.configVm ? root.configVm.decoratedPrinterPresetNames : []
                    currentIndex: {
                        if (!root.configVm) return -1
                        return root.configVm.decoratedPrinterPresetNames.indexOf(root.configVm.currentPrinterPreset)
                    }
                    onActivated: (i) => {
                        if (!root.configVm) return
                        if (i >= 0 && i < model.length)
                            root.configVm.requestCurrentPrinterPreset(
                                root.configVm.plainPresetName(model[i]))
                    }
                }

                Rectangle {
                    visible: !!root.configVm && root.configVm.isPresetDirty
                    Layout.preferredWidth: 8
                    Layout.preferredHeight: 8
                    radius: 4
                    color: Theme.accent
                    ToolTip.text: qsTr("预设已修改（未保存）")
                    ToolTip.visible: printerDirtyMA.containsMouse
                    MouseArea { id: printerDirtyMA; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                }

                // v5.16 (PSET2-07): per-row preset edit affordance
                // (rename/delete, upstream preset combo right-click menu).
                // Row actions reveal on hover (upstream hides the printer
                // edit button until hover, Plater.cpp:2568-2570,2636-2646).
                PixelIconButton {
                    hoverShow: true
                    iconSource: "qrc:/qml/assets/icons/dots.svg"
                    toolTipText: qsTr("预设操作")
                    onClicked: root.editPreset(2, root.configVm ? root.configVm.currentPrinterPreset : "")
                }

                PixelIconButton {
                    hoverShow: true
                    iconSource: "qrc:/qml/assets/icons/settings.svg"
                    toolTipText: qsTr("编辑打印机预设")
                    onClicked: backend.forwardSettingsRequest("printer")
                }

                PixelIconButton {
                    hoverShow: true
                    iconSource: "qrc:/qml/assets/icons/send-2.svg"
                    toolTipText: qsTr("打印机连接")
                    onClicked: backend.showPrintHostDialog()
                }
            }
        }

        // Bed-type dropdown row under the printer preset row (reference
        // image). Upstream semantics: the printer section carries a
        // bed-surface card whose combo is bound to the global curr_bed_type
        // (Plater.cpp:2716-2786, reset_bed_type_combox_choices). The compact
        // single-line design renders it as a plain 30px dropdown row.
        // TODO: the selection below is display-only -- no QML-facing global
        // curr_bed_type getter/setter exists yet (ConfigViewModel has no
        // currentBedType property and EditorViewModel::setPlateBedType is
        // per-plate, not the upstream global default).
        Rectangle {
            id: printerBedTypeRow
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            visible: printerHeader.expanded
            radius: 4
            color: root.sectionSurface
            border.width: 1
            border.color: root.dividerColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                anchors.topMargin: 4
                anchors.bottomMargin: 4
                spacing: 6

                Image {
                    Layout.preferredWidth: 18
                    Layout.preferredHeight: 18
                    // Upstream bed card thumbnail is "printer_placeholder"
                    // (Plater.cpp:2727), absent from the project icon set;
                    // the registered bed param glyph stands in.
                    source: "qrc:/qml/assets/icons/param_bed_temp.svg"
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.8
                }

                Text {
                    text: qsTr("热床类型")
                    color: root.mutedText
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                }

                CxComboBox {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    font.pixelSize: Theme.fontSizeSM
                    // Labels come from the previously dead bedTypeName()
                    // helper (same file, function section).
                    model: [
                        root.bedTypeName(0),
                        root.bedTypeName(1),
                        root.bedTypeName(2),
                        root.bedTypeName(3),
                        root.bedTypeName(4)
                    ]
                    currentIndex: 0
                }
            }
        }

        PixelHeader {
            id: filamentHeader
            Layout.fillWidth: true
            title: qsTr("耗材丝")
            // Upstream filament titlebar leads with the spool glyph
            // ("filament" ScalableButton, Plater.cpp:2913); spool.svg is
            // absent from the project set, so the registered spool-family
            // param icon stands in.
            iconSource: "qrc:/qml/assets/icons/param_filament_for_features.svg"
            // Upstream add/delete filament title-bar buttons, delete first
            // (Plater.cpp:2989-2990); delete hides while a single filament
            // remains (Plater.cpp:5395,2992).
            extraActions: [
                { icon: "qrc:/qml/assets/icons/minus.svg", tooltip: qsTr("删除最后一根耗材丝"),
                  visible: root.filamentRowCount > 1 },
                { icon: "qrc:/qml/assets/icons/plus.svg", tooltip: qsTr("添加一根耗材丝") }
            ]
            onExtraActionTriggered: (index) => {
                if (index === 0) {
                    if (root.filamentRowCount > 1) {
                        root.filamentRowCount -= 1
                        if (root.configVm)
                            root.configVm.setExtruderCount(root.filamentRowCount)
                    }
                } else if (index === 1) {
                    if (root.filamentRowCount < 64) { // MAXIMUM_EXTRUDER_NUMBER (libslic3r.h:65)
                        root.filamentRowCount += 1
                        if (root.configVm)
                            root.configVm.setExtruderCount(root.filamentRowCount)
                    }
                }
            }
            actionIcon: "qrc:/qml/assets/icons/settings.svg"
            actionToolTip: qsTr("耗材设置")
            onActionTriggered: backend.forwardSettingsRequest("filament")
            // Whole-bar fold toggle (upstream Plater.cpp:2892-2911).
            collapsible: true
        }

        Repeater {
            model: Math.max(1, root.filamentRowCount)

            delegate: Rectangle {
                id: filamentPixelRow
                required property int index
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                // Folded with the filament section (upstream
                // m_filament_area_wrapper toggle, Plater.cpp:2906-2908).
                visible: filamentHeader.expanded
                radius: 4
                color: root.sectionSurface
                border.width: 1
                border.color: root.compatRefreshTick >= 0
                    ? (root.configVm && !root.configVm.isFilamentCompatibleForSlot(filamentPixelRow.index) ? Theme.statusError : root.dividerColor)
                    : root.dividerColor

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.topMargin: 4
                    anchors.bottomMargin: 4
                    spacing: 6

                    // Ref: the color chip carries the extruder number and sits
                    // flush against the preset combo (upstream labels the
                    // color picker with the slot number, Plater.cpp:3340-3342).
                    Rectangle {
                        Layout.preferredWidth: 22
                        Layout.preferredHeight: 22
                        radius: 3
                        color: root.filamentColor(filamentPixelRow.index)
                        border.width: 1
                        border.color: Qt.lighter(root.filamentColor(filamentPixelRow.index), 1.25)

                        Text {
                            anchors.centerIn: parent
                            text: String(filamentPixelRow.index + 1)
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSizeSM
                            font.bold: true
                        }
                    }

                    CxComboBox {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        font.pixelSize: Theme.fontSizeSM
                        // v5.16 (PSET2-05): decorated list — section
                        // separators + incompatibility gray-out.
                        model: root.configVm ? root.configVm.decoratedFilamentPresetNames : []
                        currentIndex: {
                            if (!root.configVm) return -1
                            // v5.16 (CIRC-04): each slot shows ITS OWN preset
                            // (was: the category list's Nth entry for all slots).
                            // PSET2-05: compare against plain names since the
                            // list entries may carry the display suffix.
                            var slotPreset = root.configVm.filamentPresetForSlot(filamentPixelRow.index)
                            var names = root.configVm.decoratedFilamentPresetNames
                            for (var i = 0; i < names.length; i++) {
                                if (root.configVm.plainPresetName(names[i]) === slotPreset) return i
                            }
                            return -1
                        }
                        onActivated: (i) => {
                            if (!root.configVm) return
                            var names = root.configVm.decoratedFilamentPresetNames
                            if (i >= 0 && i < names.length)
                                root.configVm.requestFilamentPresetForSlot(
                                    filamentPixelRow.index,
                                    root.configVm.plainPresetName(names[i]))
                        }
                    }

                    Rectangle {
                        visible: root.compatRefreshTick >= 0 && !!root.configVm
                                 && !root.configVm.isFilamentCompatibleForSlot(filamentPixelRow.index)
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: 4
                        color: Theme.statusError
                        ToolTip.text: root.configVm ? root.configVm.currentPresetCompatibilityMessage : ""
                        ToolTip.visible: filamentCompatMA.containsMouse
                        MouseArea { id: filamentCompatMA; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    }

                    // Ref/upstream row tail keeps a single edit button
                    // (Plater.cpp:3358,3382 "menu_filament" edit button).
                    // The upstream menu_filament bitmap is absent from the
                    // project set; the dots glyph (same affordance as the
                    // printer row's preset menu) stands in.
                    PixelIconButton {
                        iconSource: "qrc:/qml/assets/icons/dots.svg"
                        toolTipText: qsTr("点击编辑预设")
                        onClicked: root.editPreset(1, root.configVm
                            ? root.configVm.filamentPresetForSlot(filamentPixelRow.index) : "")
                    }
                }
            }
        }

        PixelHeader {
            Layout.fillWidth: true
            title: qsTr("工艺")
            iconSource: "qrc:/qml/assets/icons/list-details.svg"
            // Ref/upstream: Global|Objects scope pills sit inline in the
            // process title row (ParamsPanel.cpp:265-267,417).
            middleComponent: processScopePills
            actionIcon: "qrc:/qml/assets/icons/settings.svg"
            actionToolTip: qsTr("工艺设置")
            onActionTriggered: backend.forwardSettingsRequest("process")
        }

        Rectangle {
            id: processPresetRow
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            radius: 4
            color: root.sectionSurface
            border.width: 1
            border.color: root.dividerColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                anchors.topMargin: 4
                anchors.bottomMargin: 4
                spacing: 6

                CxComboBox {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    font.pixelSize: Theme.fontSizeSM
                    // v5.16 (PSET2-05): decorated list — section
                    // separators + incompatibility gray-out.
                    model: root.configVm ? root.configVm.decoratedPrintPresetNames : []
                    currentIndex: {
                        if (!root.configVm) return -1
                        return root.configVm.decoratedPrintPresetNames.indexOf(root.configVm.currentPrintPreset)
                    }
                    onActivated: (i) => {
                        if (root.configVm && i >= 0)
                            root.configVm.requestCurrentPrintPreset(
                                root.configVm.plainPresetName(model[i]))
                    }
                }

                Rectangle {
                    visible: !!root.configVm && root.configVm.isPresetDirty
                    Layout.preferredWidth: 8
                    Layout.preferredHeight: 8
                    radius: 4
                    color: Theme.accent
                    ToolTip.text: qsTr("预设已修改（未保存）")
                    ToolTip.visible: processDirtyMA.containsMouse
                    MouseArea { id: processDirtyMA; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                }

                // Ref/upstream process preset row tail: copy + search icon
                // pair; search opens the find dialog instead of a permanent
                // search field. The upstream copy_menu bitmap is absent from
                // the project set; the copy glyph stands in.
                PixelIconButton {
                    iconSource: "qrc:/qml/assets/icons/copy.svg"
                    iconSize: 16
                    toolTipText: qsTr("预设操作")
                    onClicked: root.editPreset(0, root.configVm ? root.configVm.currentPrintPreset : "")
                }

                PixelIconButton {
                    iconSource: "qrc:/qml/assets/icons/search.svg"
                    iconSize: 16
                    toolTipText: qsTr("搜索设置")
                    onClicked: root.openParamsSearch()
                }
            }
        }

        // Params area: full-height container outlined by a 1px accent border
        // (ref layout), filling the sidebar remainder; only the option list
        // scrolls (params-1/layout-6).
        Rectangle {
            id: paramsInlinePanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 0
            color: "transparent"
            border.width: 1
            border.color: Theme.accent

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 5
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 14

                    Repeater {
                        // Upstream six process pages incl. Speed
                        // (Tab.cpp:2637,2772,2842,2914,2981,3036).
                        model: [
                            { key: "Quality", label: qsTr("质量") },
                            { key: "Strength", label: qsTr("强度") },
                            { key: "Speed", label: qsTr("速度") },
                            { key: "Support", label: qsTr("支撑") },
                            { key: "Multimaterial", label: qsTr("材料") },
                            { key: "Others", label: qsTr("其他") }
                        ]
                        delegate: Item {
                            id: paramsTabDelegate
                            required property var modelData
                            readonly property bool selected: paramsTabDelegate.modelData.key === root.paramsCurrentTab
                            implicitWidth: paramsTabLabel.implicitWidth + 8
                            implicitHeight: 24

                            Text {
                                id: paramsTabLabel
                                anchors.centerIn: parent
                                text: paramsTabDelegate.modelData.label
                                color: paramsTabDelegate.selected ? Theme.accent : "#9e9e9e"
                                font.pixelSize: Theme.fontSizeLG
                                font.bold: paramsTabDelegate.selected
                            }

                            // 3px selected underline (TabCtrl.cpp:346-351).
                            Rectangle {
                                visible: paramsTabDelegate.selected
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 3
                                color: Theme.accent
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.paramsCurrentTab = paramsTabDelegate.modelData.key
                                    root.rebuildParamsFilter()
                                }
                            }
                        }
                    }
                }

                // 1px full-width baseline under the text tab strip
                // (TabCtrl.cpp:346-351).
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: "#4c4c55"
                }

                ListView {
                    id: paramsList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.paramsFilteredIndices
                    spacing: 0

                    ScrollBar.vertical: ScrollBar {
                        visible: paramsList.contentHeight > paramsList.height
                    }

                    delegate: Item {
                        id: paramsDelegate
                        required property int index
                        required property var modelData

                        readonly property int optIdx: modelData
                        readonly property string optGroup: root.paramsOptionModel
                            ? root.paramsOptionModel.optGroup(optIdx) : ""
                        readonly property bool showGroupHeader: {
                            if (paramsDelegate.index === 0) return optGroup !== ""
                            var prevGroup = root.paramsOptionModel
                                ? root.paramsOptionModel.optGroup(root.paramsFilteredIndices[paramsDelegate.index - 1]) : ""
                            return optGroup !== "" && optGroup !== prevGroup
                        }

                        width: paramsList.width
                        height: optRow.totalHeight

                        OptionRow {
                            id: optRow
                            anchors.left: parent.left
                            anchors.right: parent.right
                            optionModel: root.paramsOptionModel
                            optIdx: paramsDelegate.optIdx
                            rowIndex: paramsDelegate.index
                            searchText: root.paramsSearchText
                            showGroupHeader: paramsDelegate.showGroupHeader
                            oGroup: paramsDelegate.optGroup
                            compact: true
                            compactLabelWidth: 150
                            compactFieldWidth: 86
                            compactEnumWidth: 132
                            valueSource: {
                                if (!root.configVm || !root.paramsOptionModel) return ""
                                var key = root.paramsOptionModel.optKey(paramsDelegate.optIdx)
                                return root.configVm.valueSourceForKey(key)
                            }
                        }
                    }
                }
            }

            Component.onCompleted: root.rebuildParamsFilter()
        }
    }

    // ── preset search dialog (sidebar-9/layout-8) ──────────────────────────
    // Upstream keeps the sidebar search bar hidden (Plater.cpp:3246-3247);
    // searching happens through a dialog instead of a permanent field.
    function openParamsSearch() {
        paramsSearchDialog.open()
    }

    CxDialog {
        id: paramsSearchDialog
        modal: true
        dialogTitle: qsTr("搜索设置")
        width: 380
        height: 160
        padding: 0

        onAboutToShow: {
            searchDialogField.text = root.paramsSearchText
            searchDialogField.selectAll()
            searchDialogField.forceActiveFocus()
        }

        contentItem: Rectangle {
            color: Theme.bgPanel
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingXL
                spacing: Theme.spacingMD

                CxTextField {
                    id: searchDialogField
                    Layout.fillWidth: true
                    implicitHeight: 28
                    font.pixelSize: Theme.fontSizeSM
                    placeholderText: qsTr("搜索设置...")
                    onTextChanged: {
                        if (root.paramsSearchText !== text.trim()) {
                            root.paramsSearchText = text.trim()
                            root.rebuildParamsFilter()
                        }
                    }
                    onAccepted: paramsSearchDialog.accept()
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("筛选结果实时生效，关闭对话框后仍保持筛选。")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeXS
                    wrapMode: Text.WordWrap
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: Theme.spacingMD
                    CxButton {
                        text: qsTr("清除")
                        onClicked: {
                            searchDialogField.text = ""
                            root.paramsSearchText = ""
                            root.rebuildParamsFilter()
                        }
                    }
                    CxButton {
                        text: qsTr("关闭")
                        cxStyle: CxButton.Style.Primary
                        onClicked: paramsSearchDialog.accept()
                    }
                }
            }
        }
    }

    // ── v5.16 (PSET2-07): preset rename/delete affordance ────────────────
    // Upstream exposes rename/delete on each preset combo's context menu;
    // here each preset row's edit button opens the same actions.
    property int presetEditCategory: -1
    property string presetEditName: ""

    function editPreset(category, currentName) {
        if (!root.configVm || !currentName || currentName.length === 0)
            return
        root.presetEditCategory = category
        root.presetEditName = currentName
        presetEditMenu.popup()
    }

    CxMenu {
        id: presetEditMenu

        CxMenuItem {
            text: qsTr("重命名…")
            onTriggered: {
                renamePresetField.text = root.presetEditName
                renamePresetError.visible = false
                renamePresetDialog.open()
            }
        }
        CxMenuItem {
            text: qsTr("删除…")
            onTriggered: {
                // canDeletePreset guards read-only/built-ins (service-side
                // delete would reject them too); in-use presets warn first.
                if (!root.configVm)
                    return
                if (!root.configVm.canDeletePreset(root.presetEditName)) {
                    backend.postError(root.configVm.presetActionBlocker(
                        root.presetEditCategory, root.presetEditName, "delete"), 1)
                    return
                }
                deleteConfirmInUse.visible = root.configVm.isPresetInUse(root.presetEditName)
                deletePresetDialog.open()
            }
        }
    }

    // Inline rename dialog (upstream SavePresetDialog rename path).
    CxDialog {
        id: renamePresetDialog
        modal: true
        dialogTitle: qsTr("重命名预设")
        width: 380
        height: 160
        padding: 0

        onAccepted: {
            if (!root.configVm)
                return
            const newName = renamePresetField.text.trim()
            if (newName.length === 0 || newName === root.presetEditName)
                return
            if (!root.configVm.renamePreset(root.presetEditCategory, root.presetEditName, newName)) {
                renamePresetError.visible = true
                renamePresetDialog.open()
            } else {
                root.presetEditName = newName
            }
        }

        contentItem: Rectangle {
            color: Theme.bgPanel
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingXL
                spacing: Theme.spacingMD

                Text {
                    text: root.presetEditName
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                CxTextField {
                    id: renamePresetField
                    Layout.fillWidth: true
                    implicitHeight: 28
                    font.pixelSize: Theme.fontSizeSM
                    placeholderText: qsTr("输入新的预设名称")
                    onAccepted: renamePresetDialog.accept()
                }

                Text {
                    id: renamePresetError
                    text: qsTr("重命名失败（名称为空、重复或内置预设）")
                    color: Theme.statusError
                    font.pixelSize: Theme.fontSizeXS
                    visible: false
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: Theme.spacingMD
                    CxButton {
                        text: qsTr("取消")
                        onClicked: renamePresetDialog.reject()
                    }
                    CxButton {
                        text: qsTr("确定")
                        cxStyle: CxButton.Style.Primary
                        onClicked: renamePresetDialog.accept()
                    }
                }
            }
        }
    }

    // Delete confirmation (upstream deletes the selected user preset and
    // falls the selection back to the category default).
    CxDialog {
        id: deletePresetDialog
        modal: true
        dialogTitle: qsTr("删除预设")
        width: 380
        height: 170
        padding: 0

        onAccepted: {
            if (root.configVm)
                root.configVm.deletePreset(root.presetEditCategory, root.presetEditName)
        }

        contentItem: Rectangle {
            color: Theme.bgPanel
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingXL
                spacing: Theme.spacingMD

                Text {
                    Layout.fillWidth: true
                    text: qsTr("确定删除预设 “%1”？").arg(root.presetEditName)
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    wrapMode: Text.WordWrap
                }

                Text {
                    id: deleteConfirmInUse
                    Layout.fillWidth: true
                    text: qsTr("该预设正在使用，删除后将切换回默认预设。")
                    color: Theme.statusWarning
                    font.pixelSize: Theme.fontSizeXS
                    wrapMode: Text.WordWrap
                    visible: false
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: Theme.spacingMD
                    CxButton {
                        text: qsTr("取消")
                        onClicked: deletePresetDialog.reject()
                    }
                    CxButton {
                        text: qsTr("删除")
                        cxStyle: CxButton.Style.Primary
                        onClicked: deletePresetDialog.accept()
                    }
                }
            }
        }
    }

    // Scope pills live inside the process PixelHeader row (sidebar-5).
    // BUILDGATE restore (2026-09-24): the plate pill is back — PREPSB-04
    // (leftSidebarPresetControlsAreWiredAndHonest) locks the Global/Object/
    // Plate scope set, and requestPlateScope stays on ConfigViewModel.
    Component {
        id: processScopePills
        RowLayout {
            id: processScopeBar
            spacing: 6

            PixelSegment {
                text: qsTr("全局")
                selected: root.configVm && root.configVm.settingsScope === "global"
                onClicked: if (root.configVm) root.configVm.requestGlobalScope()
            }

            PixelSegment {
                text: qsTr("对象")
                selected: root.configVm && root.configVm.settingsScope !== "global" && root.configVm.settingsScope !== "plate"
                onClicked: {
                    if (root.editorVm && root.editorVm.selectedObjectIndex >= 0 && root.configVm) {
                        root.configVm.requestObjectScope("object", "",
                            root.editorVm.selectedObjectIndex, -1)
                    }
                }
            }

            PixelSegment {
                text: qsTr("盘")
                selected: root.configVm && root.configVm.settingsScope === "plate"
                enabled: root.editorVm && root.editorVm.currentPlateIndex >= 0
                onClicked: {
                    if (root.editorVm && root.editorVm.currentPlateIndex >= 0 && root.configVm)
                        root.configVm.requestPlateScope(root.editorVm.currentPlateIndex)
                }
            }
        }
    }

    function bedTypeName(index) {
        var names = [qsTr("PEI"), qsTr("EP"), qsTr("PC"), qsTr("纹理 PEI"), qsTr("自定义")]
        if (index >= 0 && index < names.length)
            return names[index]
        return names[0]
    }

    function filamentColor(index) {
        // v5.16 (CIRC-04): configured filament colours (same source as the
        // MMU paint palette) instead of a hardcoded theme palette.
        if (root.editorVm && root.editorVm.extrudersColors
            && index < root.editorVm.extrudersColors.length)
            return root.editorVm.extrudersColors[index]
        var colors = [Theme.statusWarning, Theme.textSecondary, Theme.textSecondary, "#214bc2", Theme.chromeDangerHover]
        return index < colors.length ? colors[index] : Theme.textSecondary
    }

    // Section header: 30px title bar (upstream filament title is
    // SetMinSize(-1, FromDIP(30)), Plater.cpp:2915, printer title 3*em) with
    // leading icon + title, optional inline middle content, then a
    // right-aligned icon group (sidebar-3). Collapsible headers carry the
    // upstream title-bar band (SetBackgroundColor(title_bg) #F8F8F8,
    // Plater.cpp:2463,2891, dark-adjusted) and a 2px bottom separator shown
    // while expanded (Plater.cpp:2527-2529).
    component PixelHeader: Item {
        id: headerRoot
        property string title: ""
        property url iconSource: ""
        property var extraActions: []
        property Component middleComponent: null
        property url actionIcon: ""
        property string actionToolTip: ""
        property bool collapsible: false
        property bool expanded: true
        // Extra title detail shown while folded ("title | detail", upstream
        // printer title swap, Plater.cpp:2518-2521).
        property string collapsedDetail: ""
        signal extraActionTriggered(int index)
        signal actionTriggered()

        implicitHeight: 30

        // Title-bar band.
        Rectangle {
            anchors.fill: parent
            color: root.sectionSurface
        }

        // 2px bottom separator; hidden together with the content while the
        // section is folded (upstream separator->Show(isShown),
        // Plater.cpp:2527-2529).
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 2
            color: root.dividerColor
            visible: headerRoot.collapsible && headerRoot.expanded
        }

        // Whole-bar fold toggle. Declared before the row so the icon buttons
        // above it keep their own clicks (upstream excludes the button zone
        // from the toggle, Plater.cpp:2897-2905).
        MouseArea {
            anchors.fill: parent
            enabled: headerRoot.collapsible
            cursorShape: Qt.PointingHandCursor
            onClicked: headerRoot.expanded = !headerRoot.expanded
        }

        RowLayout {
            anchors.fill: parent
            spacing: 6

            Image {
                visible: headerRoot.iconSource !== ""
                source: headerRoot.iconSource
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                fillMode: Image.PreserveAspectFit
                opacity: 0.85
            }

            Text {
                text: {
                    if (!headerRoot.collapsible || headerRoot.expanded
                        || headerRoot.collapsedDetail.length === 0)
                        return headerRoot.title
                    return headerRoot.title + "  |  " + headerRoot.collapsedDetail
                }
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
                elide: Text.ElideRight
            }

            Item { Layout.fillWidth: true }

            Loader {
                sourceComponent: headerRoot.middleComponent
                Layout.alignment: Qt.AlignVCenter
            }

            Repeater {
                model: headerRoot.extraActions
                delegate: PixelIconButton {
                    required property int index
                    required property var modelData
                    visible: modelData.visible !== false
                    iconSource: modelData.icon
                    toolTipText: modelData.tooltip
                    onClicked: headerRoot.extraActionTriggered(index)
                }
            }

            PixelIconButton {
                visible: headerRoot.actionIcon !== ""
                iconSource: headerRoot.actionIcon
                toolTipText: headerRoot.actionToolTip
                enabled: headerRoot.enabled
                onClicked: headerRoot.actionTriggered()
            }
        }
    }

    component PixelIconButton: Rectangle {
        id: iconButtonRoot
        property url iconSource: ""
        property string toolTipText: ""
        property bool hoverShow: false
        property int buttonSize: 24
        property int iconSize: 13
        signal clicked()

        Layout.preferredWidth: iconButtonRoot.buttonSize
        Layout.preferredHeight: iconButtonRoot.buttonSize
        implicitWidth: iconButtonRoot.buttonSize
        implicitHeight: iconButtonRoot.buttonSize
        radius: 4
        color: !enabled ? root.fieldSurface
              : iconMA.containsMouse ? Theme.bgPressed
              : root.controlSurface
        border.width: 1
        border.color: root.dividerColor
        // hoverShow mirrors upstream printer edit buttons that appear only
        // while the row is hovered (Plater.cpp:2636-2646); space stays
        // reserved so the row layout does not jump.
        opacity: !enabled ? 0.45
              : hoverShow ? (hoverHandler.hovered ? 1.0 : 0.0)
              : 1.0
        Behavior on opacity { NumberAnimation { duration: 120 } }

        HoverHandler {
            id: hoverHandler
            enabled: iconButtonRoot.hoverShow
        }

        Image {
            anchors.centerIn: parent
            width: iconButtonRoot.iconSize
            height: iconButtonRoot.iconSize
            source: iconButtonRoot.iconSource
            fillMode: Image.PreserveAspectFit
            opacity: 0.8
        }

        MouseArea {
            id: iconMA
            anchors.fill: parent
            hoverEnabled: true
            enabled: iconButtonRoot.enabled && (!iconButtonRoot.hoverShow || hoverHandler.hovered)
            cursorShape: Qt.PointingHandCursor
            onClicked: iconButtonRoot.clicked()
        }

        ToolTip.visible: iconMA.containsMouse && iconButtonRoot.toolTipText.length > 0
        ToolTip.text: iconButtonRoot.toolTipText
        ToolTip.delay: 400
    }

    // Inline scope pill (ref: small Global/Objects pills inside the title
    // row, selected = accent fill + white text).
    component PixelSegment: Rectangle {
        id: segmentRoot
        property string text: ""
        property bool selected: false
        signal clicked()

        implicitWidth: segmentLabel.implicitWidth + 20
        implicitHeight: 22
        radius: 11
        color: segmentRoot.selected ? Theme.accent : root.sectionSurface
        border.width: 1
        border.color: segmentRoot.selected ? Theme.accent : root.dividerColor
        opacity: enabled ? 1.0 : 0.45

        Text {
            id: segmentLabel
            anchors.centerIn: parent
            text: segmentRoot.text
            color: segmentRoot.selected ? Theme.textOnAccent : root.mutedText
            font.pixelSize: Theme.fontSizeSM
            font.bold: segmentRoot.selected
        }

        MouseArea {
            anchors.fill: parent
            enabled: segmentRoot.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: segmentRoot.clicked()
        }
    }
}
