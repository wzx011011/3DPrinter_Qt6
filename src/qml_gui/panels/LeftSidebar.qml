pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OWzx

Rectangle {
    id: root
    required property var editorVm
    required property var configVm
    // G-07 (v5.17): filament compatibility now comes from the NOTIFY-ful
    // configVm.filamentSlotCompatibility list property (item i =
    // isFilamentCompatible(filamentPresetForSlot(i))), so the row
    // decorations below re-evaluate through normal binding dependencies
    // instead of a manual stateChanged tick. slotIncompatible() at the root
    // maps the list, falling back to list[0] for out-of-range rows — the
    // same currentFilamentPreset_ fallback filamentPresetForSlot applies,
    // so the retired per-slot compatibility Q_INVOKABLE's semantics are
    // preserved exactly.
    property string processCategory: ""
    signal exportRequested()
    // X6: the object-list panel resolves the shared context-menu family; the
    // sidebar bubbles it up so PreparePage can popup the same
    // PrepareContextMenus instance the canvas uses.
    signal objectListMenuRequested(int family, real menuX, real menuY)

    /// X6: entry point for the shared menus' Rename item (upstream
    /// append_menu_item_rename -> obj_list()->rename_item()); forwarded to
    /// the object-list panel's inline editor.
    function beginObjectListRename() {
        objectListPanel.beginRenameSelection()
    }

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
    // CP 工艺行同款：搜索框展开态 + 精简/完整模式开关（advancedMode=false 时
    // 过滤代理剔除 optMode>=1 的进阶参数——upstream comSimple view）。
    property bool paramsSearchOpen: false
    property bool paramsAdvancedMode: AppSettings.value("paramsAdvancedMode", "1") === "1"
    readonly property var paramsOptionModel: {
        if (!root.configVm) return null
        return root.configVm.printOptions
    }
    readonly property string paramsTier: "print"
    // Upstream six process pages incl. Speed
    // (Tab.cpp:2637,2772,2842,2914,2981,3036). Shared by the tab strip and
    // the one-persistent-page-per-tab host below.
    readonly property var paramsTabs: [
        { key: "Quality", label: qsTr("质量") },
        { key: "Strength", label: qsTr("强度") },
        { key: "Speed", label: qsTr("速度") },
        { key: "Support", label: qsTr("支撑") },
        { key: "Multimaterial", label: qsTr("多材料") },
        { key: "Others", label: qsTr("其他") }
    ]

    // ── Orca objects mode (ParamsPanel::set_active_tab + show_object_list):
    // the Global/Objects toggle swaps the whole params stack. 对象 = object
    // tree above a model-config page set; 全局 = the standard process pages
    // with no tree.
    readonly property bool objectsMode: !!root.configVm && root.configVm.settingsScope !== "global"
    // Plate page when the scope IS plate, object pages when the scope is
    // object/volume — TabPrintPlate vs TabPrintObject. Scope-driven (not
    // selection-driven): the tree selection re-targets the scope, and the
    // page set follows it.
    readonly property bool objectScopeActive: root.objectsMode
        && !!root.configVm && root.configVm.settingsScope !== "plate"
    readonly property var paramsPages: {
        if (!root.objectsMode) return root.paramsTabs
        if (root.objectScopeActive)
            return [{ key: "Frequent", label: qsTr("常用") }].concat(root.paramsTabs)
        return [{ key: "Plate", label: qsTr("盘参数设置") }]
    }
    // The tab strip mirrors the page set — plate mode shows its single
    // 盘参数设置 tab as selected: the strip (and everything below it)
    // never moves when the tree selection flips plate ↔ object.
    readonly property var visibleParamsTabs: root.paramsPages
    onObjectsModeChanged: {
        paramsCurrentTab = root.objectsMode
            ? (root.objectScopeActive ? "Frequent" : "Plate") : "Quality"
    }
    // The current tab must always name a page of the ACTIVE page set: a
    // scope flip between plate and object swaps the set, so re-seat the tab
    // (otherwise the host shows no page at all — every delegate is shifted
    // out of the window).
    onObjectScopeActiveChanged: {
        paramsCurrentTab = root.objectScopeActive ? "Frequent"
                                                  : "Plate"
    }
    function updateScopeForSelection(force) {
        // force=true = the 对象 pill click (entering objects mode FROM
        // global, when objectsMode is still false); selection-driven calls
        // only re-target inside objects mode.
        if (!root.configVm || !root.editorVm)
            return
        if (!root.objectsMode && !force)
            return
        if (root.editorVm.selectedObjectIndex >= 0
            && root.editorVm.canOpenSelectionSettings) {
            root.configVm.requestObjectScope(
                root.editorVm.settingsTargetType,
                root.editorVm.settingsTargetName,
                root.editorVm.settingsTargetObjectIndex, -1)
        } else if (root.editorVm.currentPlateIndex >= 0) {
            root.configVm.requestPlateScope(root.editorVm.currentPlateIndex)
        }
    }
    Connections {
        // Selection drives the model page (Orca: part → layer → object →
        // plate tab by selection type). null-safe target for the smoke
        // harness (pages instantiate without a configVm/editorVm pair).
        target: root.editorVm ? root.editorVm : null
        function onSelectionChanged() { root.updateScopeForSelection() }
        function onCurrentPlateIndexChanged() { root.updateScopeForSelection() }
    }


    color: panelSurface
    radius: 0
    border.width: 0

    // Sidebar layout contract (upstream Plater.cpp:2425-2429,3245,3257): the
    // sidebar itself does not scroll as one column. Printer/filament/process
    // sections stay fixed; the params area fills the remaining height and
    // scrolls on its own.

    // ── Prepare-sidebar reference restoration (Orca Plater.cpp Sidebar) ────
    // Nozzle-variant cell (panel_nozzle_dia + combo_nozzle_dia,
    // Plater.cpp:2652-2690): sibling printer presets that share the
    // "<model> (<d> nozzle)" base name form the variant list; switching a
    // variant = switching to that sibling preset. Name parsing stays QML-side
    // (3-line regex); the preset SWITCH goes through the same
    // requestCurrentPrinterPreset the printer combo uses.
    function printerBaseName(name) {
        var s = String(name)
        var m = s.match(/^(.*) \([^()]* nozzle\)$/)
        return m ? m[1] : s
    }
    function variantDiameterLabel(name) {
        var m = String(name).match(/\(([^()]*) nozzle\)$/)
        if (m) return m[1]
        var v = root.configVm
                ? root.configVm.wizardPresetValue(name, "nozzle_diameter") : null
        return (v === null || v === undefined || v === "") ? "0.4" : String(v)
    }
    readonly property var printerVariantEntries: {
        var out = []
        if (!root.configVm) return out
        var base = printerBaseName(root.configVm.currentPrinterPreset)
        var names = root.configVm.printerPresetNames
        for (var i = 0; i < names.length; i++) {
            if (printerBaseName(names[i]) === base) out.push(names[i])
        }
        return out
    }
    readonly property string nozzleTypeLabelText: {
        if (!root.configVm) return ""
        var v = root.configVm.wizardPresetValue(
                    root.configVm.currentPrinterPreset, "nozzle_type")
        var map = { "brass": qsTr("黄铜"), "hardened": qsTr("硬化钢"),
                    "steel": qsTr("不锈钢"), "bi_metal": qsTr("双金属") }
        return map[String(v)] !== undefined ? map[String(v)]
                                            : (v ? String(v) : "")
    }
    // Bed-type → thumbnail, indexed by the libslic3r BedType enum
    // (btDefault=0..btSuperTack=6) — the same image table as upstream
    // bed_type_thumbnails (Plater.cpp:320-327); btDefault follows the preset
    // default and renders the generic textured-PEI tile.
    readonly property var bedTypeThumbnails: [
        "qrc:/qml/assets/icons/orca_bed_pei.png",
        "qrc:/qml/assets/icons/orca_bed_cool.png",
        "qrc:/qml/assets/icons/orca_bed_engineering.png",
        "qrc:/qml/assets/icons/orca_bed_high_templ.png",
        "qrc:/qml/assets/icons/orca_bed_pei.png",
        "qrc:/qml/assets/icons/orca_bed_pei_cool.png",
        "qrc:/qml/assets/icons/orca_bed_cool_supertack.png"
    ]
    // Preset-card cell chrome (Orca PanelColors, Plater.cpp:2537-2543):
    // #DBDBDB resting border stepping to the #009688 sidebar accent on
    // hover/focus.
    component PresetCell: Rectangle {
        default property alias content: cellInner.data
        property alias hoverActive: cellHover.hovered
        radius: Theme.radiusMD
        color: cellHover.hovered ? Theme.presetFocusBg : Theme.bgElevated
        border.width: 1
        border.color: cellHover.hovered ? Theme.sidebarAccent
                                        : Theme.presetBorder
        Behavior on border.color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
        HoverHandler { id: cellHover }
        Item { id: cellInner; anchors.fill: parent }
    }

    ColumnLayout {
        id: sidebarContent
        anchors.fill: parent
        // Orca SidebarProps::ContentMargin (Plater.hpp:128) — 12px side
        // inset for the section stack.
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 6
        anchors.bottomMargin: 8
        spacing: 6

        // ── 打印机 title band (Plater.cpp:2460-2515) ─────────────────────
        PixelHeader {
            id: printerHeader
            Layout.fillWidth: true
            title: qsTr("打印机")
            iconSource: "qrc:/qml/assets/icons/orca_printer.svg"
            // ORCA titlebar keeps connect + settings on the band
            // (m_printer_connect / m_printer_setting, Plater.cpp:2475-2495);
            // the sync glyph shows only with a live printer and has no mock
            // data plane yet.
            extraActions: [
                { icon: "qrc:/qml/assets/icons/orca_monitor_signal_strong.svg", tooltip: qsTr("打印机连接") }
            ]
            onExtraActionTriggered: (index) => {
                if (index === 0)
                    Backend.showPrintHostDialog()
            }
            actionIcon: "qrc:/qml/assets/icons/orca_settings.svg"
            actionToolTip: qsTr("打印机设置")
            onActionTriggered: Backend.forwardSettingsRequest("printer")
            collapsible: true
            collapsedDetail: root.configVm ? root.configVm.currentPrinterPreset : ""
        }

        // Printer preset card row (Plater.cpp:2540-2830): [photo+combo cell]
        // [nozzle-variant cell] [bed-type cell], each a bordered PresetCell.
        RowLayout {
            id: printerCardRow
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            spacing: 4
            visible: printerHeader.expanded

            // Main preset cell: thumbnail + borderless combo + hover edit.
            PresetCell {
                id: printerPresetRow
                Layout.fillWidth: true
                Layout.fillHeight: true

                MouseArea {
                    anchors.fill: parent
                    anchors.rightMargin: 24
                    enabled: printerPresetCombo.enabled
                    cursorShape: Qt.PointingHandCursor
                    onClicked: printerPresetCombo.popup.open()
                }

                RowLayout {
                    anchors.fill: parent
                    spacing: 4

                    Image {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        Layout.alignment: Qt.AlignVCenter
                        Layout.leftMargin: Theme.insetMD
                        // Upstream seeds the cell with "printer_placeholder"
                        // (Plater.cpp:2587) and swaps in the printer model's
                        // rendered image when the bundle carries one; the
                        // flat preset store has no per-model renders yet, so
                        // the same placeholder stands in.
                        source: "qrc:/qml/assets/icons/orca_printer_placeholder.png"
                        fillMode: Image.PreserveAspectFit
                    }

                    CxComboBox {
                        id: printerPresetCombo
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        frameless: true
                        font.pixelSize: Theme.fontSizeMD
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

                    // Dirty marker (PREPSB-03): the preset card surfaces
                    // isPresetDirty — 8px accent dot with the compatibility
                    // tooltip, matching the process row's marker.
                    Rectangle {
                        visible: !!root.configVm && root.configVm.isPresetDirty
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        Layout.rightMargin: 2
                        radius: Theme.radiusSM
                        color: Theme.sidebarAccent
                        ToolTip.text: qsTr("预设已修改（未保存）")
                        ToolTip.visible: printerDirtyMA.containsMouse
                        MouseArea { id: printerDirtyMA; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    }

                    // Hover-revealed edit button (Orca btn_edit_printer,
                    // Plater.cpp:2551-2585 shows it only while the cell is
                    // hovered; a disabled control never reports hovered, so
                    // the gate is opacity-only).
                    CxIconButton {
                        buttonSize: 24
                        iconSize: 14
                        cxStyle: CxIconButton.Style.Surface
                        opacity: (hovered || printerPresetRow.hoverActive) ? 1.0 : 0.0
                        Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
                        iconSource: "qrc:/qml/assets/icons/orca_edit.svg"
                        toolTipText: qsTr("点击编辑预设")
                        onClicked: root.editPreset(2, root.configVm ? root.configVm.currentPrinterPreset : "")
                    }

                    Item { Layout.rightMargin: 6; Layout.fillHeight: true }
                }
            }

            // Nozzle-variant cell (panel_nozzle_dia, Plater.cpp:2652-2690):
            // 变种 title, diameter combo, nozzle-type caption; 70x60.
            PresetCell {
                id: printerVariantCell
                Layout.preferredWidth: 70
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.topMargin: 6
                        text: qsTr("变种")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                    }

                    CxComboBox {
                        id: variantCombo
                        Layout.fillWidth: true
                        Layout.leftMargin: 2
                        Layout.rightMargin: 2
                        Layout.preferredHeight: 26
                        frameless: true
                        font.pixelSize: Theme.fontSizeMD
                        font.bold: true
                        model: {
                            var out = []
                            for (var i = 0; i < root.printerVariantEntries.length; i++)
                                out.push(root.variantDiameterLabel(root.printerVariantEntries[i]))
                            return out
                        }
                        currentIndex: {
                            if (!root.configVm) return -1
                            return root.printerVariantEntries.indexOf(root.configVm.currentPrinterPreset)
                        }
                        onActivated: (i) => {
                            if (root.configVm && i >= 0 && i < root.printerVariantEntries.length)
                                root.configVm.requestCurrentPrinterPreset(
                                    root.printerVariantEntries[i])
                        }
                    }

                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        Layout.bottomMargin: 6
                        text: root.nozzleTypeLabelText
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: variantCombo.enabled && variantCombo.count > 1
                    cursorShape: Qt.PointingHandCursor
                    z: -1
                    onClicked: variantCombo.popup.open()
                }
            }

            // Bed-type cell (panel_printer_bed, Plater.cpp:2725-2800): an
            // arrow-only combo over the bed thumbnail; the label source stays
            // editorVm.bedTypeLabels and the write stays setPlateBedType
            // (Q11-④ single-source contract).
            PresetCell {
                id: printerBedTypeRow
                Layout.preferredWidth: 70
                Layout.fillHeight: true

                RowLayout {
                    anchors.fill: parent
                    spacing: 0

                    CxComboBox {
                        id: bedTypeCombo
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 26
                        Layout.alignment: Qt.AlignVCenter
                        Layout.leftMargin: 2
                        frameless: true
                        font.pixelSize: Theme.fontSizeXS
                        model: root.editorVm ? root.editorVm.bedTypeLabels : []
                        enabled: root.editorVm && root.editorVm.currentPlateIndex >= 0
                        currentIndex: root.editorVm ? root.editorVm.currentPlateBedType : 0
                        onActivated: function(index) {
                            if (root.editorVm && root.editorVm.currentPlateIndex >= 0)
                                root.editorVm.setPlateBedType(root.editorVm.currentPlateIndex, index)
                        }
                        // Honest scope disclosure: this writes the current
                        // plate's curr_bed_type (the slice-effective value),
                        // not an app-wide override.
                        ToolTip.visible: bedTypeComboHover.hovered
                        ToolTip.text: root.editorVm && currentIndex >= 0
                                      ? root.editorVm.bedTypeLabels[currentIndex]
                                      : ""
                        ToolTip.delay: 400
                        HoverHandler { id: bedTypeComboHover }
                    }

                    Image {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        Layout.alignment: Qt.AlignVCenter
                        Layout.leftMargin: 2
                        Layout.rightMargin: 2
                        source: root.bedTypeThumbnails[
                                  Math.min(Math.max(root.editorVm
                                    ? root.editorVm.currentPlateBedType : 0, 0),
                                    root.bedTypeThumbnails.length - 1)]
                        fillMode: Image.PreserveAspectFit
                    }

                    Item { Layout.fillWidth: true }
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: bedTypeCombo.enabled
                    cursorShape: Qt.PointingHandCursor
                    z: -1
                    onClicked: bedTypeCombo.popup.open()
                }
            }
        }

        // ── 耗材丝 title band (Plater.cpp:2890-3040) ─────────────────────
        PixelHeader {
            id: filamentHeader
            Layout.fillWidth: true
            title: qsTr("耗材丝")
            iconSource: "qrc:/qml/assets/icons/orca_filament.svg"
            // Upstream titlebar: add/delete slot buttons + flushing volumes
            // (m_bpButton_add_filament / m_bpButton_del_filament /
            // m_flushing_volume_btn, Plater.cpp:2984-3010; delete hides with
            // a single filament, Plater.cpp:3013). Flushing volumes maps to
            // the WipeTowerDialog (the WipingDialog.html port).
            extraActions: [
                { icon: "qrc:/qml/assets/icons/orca_add_filament.svg", tooltip: qsTr("添加一根耗材丝") },
                { icon: "qrc:/qml/assets/icons/orca_flush_volumes.svg", tooltip: qsTr("冲刷体积") },
                { icon: "qrc:/qml/assets/icons/orca_delete_filament.svg", tooltip: qsTr("删除最后一根耗材丝"),
                  visible: root.filamentRowCount > 1 }
            ]
            onExtraActionTriggered: (index) => {
                if (index === 0) {
                    if (root.filamentRowCount < 64) { // MAXIMUM_EXTRUDER_NUMBER (libslic3r.h:65)
                        root.filamentRowCount += 1
                        if (root.configVm)
                            root.configVm.setExtruderCount(root.filamentRowCount)
                    }
                } else if (index === 1) {
                    Backend.showWipeTowerDialog()
                } else if (index === 2) {
                    if (root.filamentRowCount > 1) {
                        root.filamentRowCount -= 1
                        if (root.configVm)
                            root.configVm.setExtruderCount(root.filamentRowCount)
                    }
                }
            }
            actionIcon: "qrc:/qml/assets/icons/orca_settings.svg"
            actionToolTip: qsTr("耗材设置")
            onActionTriggered: Backend.forwardSettingsRequest("filament")
            // Whole-bar fold toggle (upstream Plater.cpp:2892-2911).
            collapsible: true
        }

        Repeater {
            model: Math.max(1, root.filamentRowCount)

            delegate: Rectangle {
                id: filamentPixelRow
                required property int index
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                // Folded with the filament section (upstream
                // m_filament_area_wrapper toggle, Plater.cpp:2906-2908).
                visible: filamentHeader.expanded
                radius: Theme.radiusSM
                color: Theme.bgElevated
                border.width: 1
                // 2026-10-26 pixel audit: incompatibility keeps its 8px
                // status dot + tooltip (below) and the combo's gray-out —
                // the row border no longer double-alarms in red for what is
                // a common resting state (upstream leaves the row clean).
                border.color: filamentRowHover.hovered ? Theme.sidebarAccent : Theme.presetBorder
                Behavior on border.color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
                HoverHandler { id: filamentRowHover }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    anchors.topMargin: 4
                    anchors.bottomMargin: 4
                    spacing: 6

                    // Color chip carries the extruder number, flush against
                    // the preset combo (upstream labels the color picker with
                    // the slot number, Plater.cpp:3340-3342).
                    Rectangle {
                        Layout.preferredWidth: 20
                        Layout.preferredHeight: 20
                        Layout.alignment: Qt.AlignVCenter
                        radius: Theme.radiusSM
                        color: root.filamentColor(filamentPixelRow.index)
                        border.width: 1
                        border.color: Qt.lighter(root.filamentColor(filamentPixelRow.index), 1.25)  // style-exempt: dynamic data-color op

                        Text {
                            anchors.centerIn: parent
                            text: String(filamentPixelRow.index + 1)
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSizeXS
                            font.bold: true
                        }
                    }

                    CxComboBox {
                        id: filamentCombo
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        font.pixelSize: Theme.fontSizeMD
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
                        visible: !!root.configVm
                                 && root.slotIncompatible(filamentPixelRow.index)
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: Theme.radiusSM
                        color: Theme.statusError
                        ToolTip.text: root.configVm ? root.configVm.currentPresetCompatibilityMessage : ""
                        ToolTip.visible: filamentCompatMA.containsMouse
                        MouseArea { id: filamentCompatMA; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    }

                    // Row tail edit button (upstream menu_filament,
                    // Plater.cpp:3358,3382; the pencil glyph is Orca edit.svg).
                    CxIconButton {
                        buttonSize: 24
                        iconSize: 14
                        cxStyle: CxIconButton.Style.Surface
                        iconSource: "qrc:/qml/assets/icons/orca_edit.svg"
                        toolTipText: qsTr("点击编辑预设")
                        onClicked: root.editPreset(1, root.configVm
                            ? root.configVm.filamentPresetForSlot(filamentPixelRow.index) : "")
                    }
                }
            }
        }

        // ── 工艺 title band: process icon + Global/Objects pills +
        //    visibility controls (ParamsPanel.cpp:255-300). ───────────────
        PixelHeader {
            id: processHeader
            Layout.fillWidth: true
            title: qsTr("工艺")
            iconSource: "qrc:/qml/assets/icons/orca_process.svg"
            // Ref/upstream: Global|Objects scope pills sit inline in the
            // process title row (ParamsPanel.cpp:265-267,417).
            middleComponent: processScopePills
            // ORCA row tail (ParamsPanel.cpp:271-297): advanced-visibility
            // cycle icon, the mode switch, the object-settings table, and
            // preset compare. The 简/全 toggle and the molecule icon drive
            // the same paramsAdvancedMode (upstream m_mode_view +
            // m_mode_icon both cycle settings visibility).
            extraActions: [
                { icon: "qrc:/qml/assets/icons/orca_advanced.svg", tooltip: qsTr("切换参数显示模式") },
                { icon: "qrc:/qml/assets/icons/orca_table.svg", tooltip: qsTr("参数清单") },
                { icon: "qrc:/qml/assets/icons/orca_compare.svg", tooltip: qsTr("比较预设") }
            ]
            onExtraActionTriggered: (index) => {
                if (index === 0) {
                    root.paramsAdvancedMode = !root.paramsAdvancedMode
                    AppSettings.setValue("paramsAdvancedMode",
                                             root.paramsAdvancedMode ? "1" : "0")
                } else if (index === 1) {
                    paramsListDialog.open()
                } else if (index === 2) {
                    presetCompareDialog.open()
                }
            }
            actionIcon: "qrc:/qml/assets/icons/orca_settings.svg"
            actionToolTip: qsTr("工艺设置")
            onActionTriggered: Backend.forwardSettingsRequest("process")
        }

        // Process preset row (Plater.cpp Tab sidebar): revert arrow (dirty
        // only), preset combo, save, search — ParamsPanel top-panel order.
        Rectangle {
            id: processPresetRow
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            radius: Theme.radiusSM
            color: Theme.bgElevated
            border.width: 1
            border.color: processRowHover.hovered ? Theme.sidebarAccent : Theme.presetBorder
            Behavior on border.color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
            HoverHandler { id: processRowHover }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                anchors.topMargin: 2
                anchors.bottomMargin: 2
                spacing: 2

                // Modified-preset revert (Orca m_btn_reset shows only when
                // the preset carries unsaved edits): re-selecting the current
                // print preset re-merges the persisted preset values over the
                // user overrides.
                CxIconButton {
                    buttonSize: 24
                    iconSize: 14
                    cxStyle: CxIconButton.Style.Surface
                    visible: !!root.configVm && root.configVm.isPresetDirty
                    iconSource: "qrc:/qml/assets/icons/orca_undo.svg"
                    toolTipText: qsTr("放弃未保存的修改")
                    onClicked: {
                        if (root.configVm)
                            root.configVm.setCurrentPrintPreset(root.configVm.currentPrintPreset)
                    }
                }

                CxComboBox {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 26
                    font.pixelSize: Theme.fontSizeMD
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
                    radius: Theme.radiusSM
                    color: Theme.sidebarAccent
                    ToolTip.text: qsTr("预设已修改（未保存）")
                    ToolTip.visible: processDirtyMA.containsMouse
                    MouseArea { id: processDirtyMA; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                }

                CxIconButton {
                    buttonSize: 24
                    iconSize: 14
                    cxStyle: CxIconButton.Style.Surface
                    iconSource: "qrc:/qml/assets/icons/orca_save.svg"
                    toolTipText: qsTr("保存当前工艺预设")
                    onClicked: {
                        if (root.configVm)
                            root.configVm.saveCurrentPreset()
                    }
                }

                CxIconButton {
                    buttonSize: 24
                    iconSize: 14
                    cxStyle: CxIconButton.Style.Surface
                    iconSource: "qrc:/qml/assets/icons/search.svg"
                    toolTipText: qsTr("搜索设置")
                    onClicked: root.openParamsSearch()
                }
            }
        }

        // ── U14 (Prepare 页信息密度)：常驻"切片信息"卡 ─────────────────────
        // 上游 Plater 侧栏 SlicedInfo 对齐（third_party/OrcaSlicer
        // Plater.cpp:358-414）：侧栏常驻盒子，切片完成后回填；每行初值 "N/A"，
        // SetTextAndShow(:403-412) 对仍无值的行整行隐藏。本 fork 的这批数值
        // 一直存在于 EditorViewModel（sliceEstimatedTime / sliceResultCost /
        // sliceResultFilament / sliceResultWeight，全部按当前盘有效结果门控），
        // 但此前只有预览页 StatsPanel 可见——现在侧栏常驻，Prepare 页无需切页
        // 即可读数。对象数是场景态（非切片派生），首片前也显示；切片派生各行
        // 跟随各自 getter（当前盘无有效结果/被编辑置 stale 时返回空串 → 整行
        // 隐藏），与上游"切片成功才显示、无值行隐藏"语义一致。数值沿用 U12 的
        // 只读 TextEdit 可选中样式，便于抄录。
        Rectangle {
            id: slicedInfoCard
            Layout.fillWidth: true
            visible: root.editorVm
                     && (root.editorVm.modelCount > 0 || root.editorVm.hasSliceResult)
            radius: Theme.radiusSM
            color: root.sectionSurface
            border.width: 1
            border.color: root.dividerColor
            implicitHeight: slicedInfoLayout.implicitHeight + 12

            ColumnLayout {
                id: slicedInfoLayout
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 6
                spacing: 2

                Text {
                    text: qsTr("切片信息")
                    color: root.mutedText
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                }

                SlicedInfoRow {
                    label: qsTr("对象")
                    value: root.editorVm ? String(root.editorVm.modelCount) : "0"
                }
                SlicedInfoRow {
                    label: qsTr("预计耗时")
                    value: root.editorVm ? root.editorVm.sliceEstimatedTime : ""
                }
                SlicedInfoRow {
                    label: qsTr("总成本")
                    value: root.editorVm ? root.editorVm.sliceResultCost : ""
                }
                SlicedInfoRow {
                    label: qsTr("耗材")
                    value: root.editorVm ? root.editorVm.sliceResultFilament : ""
                }
                SlicedInfoRow {
                    label: qsTr("重量")
                    value: root.editorVm ? root.editorVm.sliceResultWeight : ""
                }
            }
        }

        // ── X6: object list section (upstream Plater sidebar section) ──────
        // ORCA hides the object list inside the settings sidebar
        // (Plater.cpp:3241-3245: `m_object_list = new ObjectList(...);
        // m_object_list->Hide()` — the reference sidebar renders params
        // only). The section stays instantiated (rename entry point for the
        // shared context menus) but out of the visible stack, exactly like
        // upstream.
        PixelHeader {
            id: objectListHeader
            Layout.fillWidth: true
            visible: false
            title: qsTr("对象列表")
            iconSource: "qrc:/qml/assets/icons/list-details.svg"
            collapsible: true
        }

        ObjectListPanel {
            id: objectListPanel
            Layout.fillWidth: true
            // Orca show_object_list(m_mode_region): the tree shows exactly in
            // objects mode. FIXED slot (120px, scrolls inside) — the params
            // area's top edge must never move when the tree content grows or
            // a node expands.
            visible: root.objectsMode
            Layout.preferredHeight: root.objectsMode ? 120 : 0
            Layout.maximumHeight: root.objectsMode ? 120 : 0
            editorVm: root.editorVm
            // 盘与对象统一在对象按钮下：选中树的盘节点 → 盘参数设置，
            // 选中模型节点 → 对象页集（用户对照 Orca 2026-10-29）。
            onTreeRowActivated: function(nodeType, plateIndex, sourceObjectIndex) {
                if (!root.objectsMode || !root.configVm || !root.editorVm)
                    return
                if (nodeType === objectListPanel.nodePlate && plateIndex >= 0) {
                    root.configVm.requestPlateScope(plateIndex)
                } else if (nodeType === objectListPanel.nodeObject
                           && sourceObjectIndex >= 0
                           && root.editorVm.canOpenSelectionSettings) {
                    root.configVm.requestObjectScope(
                        root.editorVm.settingsTargetType,
                        root.editorVm.settingsTargetName,
                        root.editorVm.settingsTargetObjectIndex, -1)
                }
            }
            onContextMenuResolved: function(family, menuX, menuY) {
                root.objectListMenuRequested(family, menuX, menuY)
            }
        }

        // Params area: fills the sidebar remainder; only the option list
        // scrolls (params-1/layout-6).
        //
        // The explicit preferredHeight is load-bearing: Qt Quick Layouts
        // splits space between fillHeight siblings in proportion to their
        // preferred sizes, so a plain Rectangle (implicitHeight 0) receives
        // nothing. With the object list hidden the params panel is the only
        // fillHeight child and takes the whole remainder.
        Rectangle {
            id: paramsInlinePanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: sidebarContent.height * 3 / 4
            radius: 0
            color: "transparent"
            border.width: 0

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 0
                spacing: 0

                // Tab strip (TabCtrl.cpp:346-351): selected = textPrimary
                // bold over a 3px #009688 underline, unselected =
                // textSecondary, 58px slots, 1px #CECECE baseline.
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 2
                    spacing: 0

                    Repeater {
                        model: root.visibleParamsTabs
                        delegate: Item {
                            id: paramsTabDelegate
                            required property var modelData
                            readonly property bool selected: paramsTabDelegate.modelData.key === root.paramsCurrentTab
                            implicitWidth: 58
                            implicitHeight: 34

                            Text {
                                id: paramsTabLabel
                                anchors.verticalCenter: parent.verticalCenter
                                x: 0
                                text: paramsTabDelegate.modelData.label
                                color: paramsTabDelegate.selected ? Theme.textPrimary : Theme.textSecondary
                                font.pixelSize: Theme.fontSizeLG
                                font.bold: paramsTabDelegate.selected
                            }

                            // 3px selected underline, text-width
                            // (TabCtrl.cpp:346-351).
                            Rectangle {
                                visible: paramsTabDelegate.selected
                                anchors.bottom: parent.bottom
                                x: 0
                                width: paramsTabLabel.implicitWidth
                                height: 3
                                color: Theme.sidebarAccent
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                // Tab flips only change visibility — the
                                // per-page proxy inputs (page/searchText) are
                                // untouched, so no re-filtering happens.
                                onClicked: root.paramsCurrentTab = paramsTabDelegate.modelData.key
                            }
                        }
                    }
                }

                // 1px full-width baseline under the text tab strip
                // (TabCtrl.cpp:346-351).
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.tabStripLine
                }

                    // One persistent page per process tab (upstream Tab.cpp
                    // keeps a widget tree per page and only shows/hides it —
                    // pages are built once, never rebuilt on a switch). Each
                    // page loads on its first visit; a flip to an
                    // already-built page is a visibility change only, so
                    // repeated switching stays free.
                    Item {
                        id: paramsPagesHost
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        // Prewarm: once the panel has painted, build the six
                        // pages one per tick so the first click on any tab is
                        // a flip instead of a ~350ms delegate build. A click
                        // racing the prewarm still builds its own page on
                        // visit.
                        property int prewarmIdx: 0
                        Timer {
                            id: prewarmDelay
                            interval: 1200
                            onTriggered: prewarmStep.restart()
                        }
                        Timer {
                            id: prewarmStep
                            // Step wider than one page build (~550ms with the
                            // cacheBuffer rows) so a click landing mid-prewarm
                            // queues behind at most one build.
                            interval: 900
                            repeat: true
                            triggeredOnStart: true
                            onTriggered: {
                                if (paramsPagesHost.prewarmIdx >= paramsPagesRepeater.count) {
                                    stop()
                                    return
                                }
                                var page = paramsPagesRepeater.itemAt(paramsPagesHost.prewarmIdx)
                                if (page) page.visited = true
                                paramsPagesHost.prewarmIdx = paramsPagesHost.prewarmIdx + 1
                            }
                        }
                        Component.onCompleted: prewarmDelay.restart()

                        Repeater {
                            id: paramsPagesRepeater
                            model: root.paramsPages
                            delegate: Item {
                                id: paramsPageHost
                                required property var modelData
                                readonly property string pageKey: paramsPageHost.modelData.key
                                readonly property bool isCurrent: root.paramsCurrentTab === paramsPageHost.pageKey
                                property bool visited: paramsPageHost.isCurrent
                                onIsCurrentChanged: if (paramsPageHost.isCurrent) paramsPageHost.visited = true
                                width: parent.width
                                height: parent.height
                                // QQuickItemView releases every delegate when
                                // the view's effective visibility drops, so a
                                // visible=false flip silently destroyed and
                                // rebuilt the incoming page's rows on every
                                // switch. Keep pages effectively visible and
                                // shift the non-current ones out of the window
                                // instead: the ListView's own clip keeps them
                                // culled and unhittable, so a flip is a pure
                                // reposition and revisits reuse the live rows.
                                x: paramsPageHost.isCurrent ? 0 : -100000

                                Loader {
                                    anchors.fill: parent
                                    active: paramsPageHost.visited
                                    sourceComponent: ListView {
                                        id: paramsPage
                                        anchors.fill: parent
                                        clip: true
                                        // Objects mode routes the whole host
                                        // at ONE page: Plate when the scope is
                                        // plate, the clicked tab otherwise.
                                        property string effectivePage:
                                            root.objectsMode && !root.objectScopeActive
                                            ? "Plate" : paramsPageHost.pageKey
                                        // Reference rhythm: 26px rows on a
                                        // 30px pitch (4px inter-row gap).
                                        spacing: 4
                                        cacheBuffer: 600
                                        // Plate page leads with curr_bed_type
                                        // (TabPrintPlate appends it before
                                        // skirt_start_angle). A ListView
                                        // header — page CONTENT, not page
                                        // frame — so the list's top edge stays
                                        // identical across modes; the same
                                        // single source as the printer card
                                        // cell (Q11-④).
                                        header: paramsPage.effectivePage === "Plate"
                                                ? plateBedHeader : null
                                        // Per-page C++ filter proxy (replaces
                                        // the QML-side index-array filter
                                        // orchestration). Tab flips only
                                        // toggle this page's visibility — no
                                        // proxy input changes, so no
                                        // re-filtering, and value dataChanged
                                        // now refreshes visible rows in place.
                                        model: ConfigOptionFilterProxy {
                                            sourceModel: root.paramsOptionModel
                                            page: paramsPage.effectivePage
                                            searchText: root.paramsSearchText
                                            advancedMode: root.paramsAdvancedMode
                                        }

                                        Component {
                                            id: plateBedHeader
                                            RowLayout {
                                                width: paramsPage.width
                                                height: 30
                                                spacing: 8

                                                Text {
                                                    text: qsTr("热床类型")
                                                    color: Theme.textPrimary
                                                    font.pixelSize: Theme.fontSizeLG
                                                    elide: Text.ElideRight
                                                    Layout.leftMargin: Theme.insetXL
                                                }

                                                Item { Layout.fillWidth: true }

                                                CxComboBox {
                                                    Layout.preferredWidth: 150
                                                    Layout.preferredHeight: 26
                                                    Layout.rightMargin: Theme.insetMD
                                                    font.pixelSize: Theme.fontSizeMD
                                                    model: root.editorVm ? root.editorVm.bedTypeLabels : []
                                                    enabled: root.editorVm && root.editorVm.currentPlateIndex >= 0
                                                    currentIndex: root.editorVm ? root.editorVm.currentPlateBedType : 0
                                                    onActivated: function(index) {
                                                        if (root.editorVm && root.editorVm.currentPlateIndex >= 0)
                                                            root.editorVm.setPlateBedType(root.editorVm.currentPlateIndex, index)
                                                    }
                                                }
                                            }
                                        }

                                        ScrollBar.vertical: ScrollBar {
                                            visible: paramsPage.contentHeight > paramsPage.height
                                            // 2026-10-26 light-mode catch: the
                                            // bare Basic bar clipped at the
                                            // panel border (read as a stray
                                            // "Y") — CxScrollView's themed bar,
                                            // inset 3px.
                                            anchors.top: paramsPage.top
                                            anchors.bottom: paramsPage.bottom
                                            anchors.right: paramsPage.right
                                            anchors.rightMargin: Theme.spacingXS
                                            background: Rectangle {
                                                implicitWidth: 8
                                                radius: Theme.radiusSM
                                                color: Theme.scrollBarTrackColor
                                            }
                                            contentItem: Rectangle {
                                                implicitWidth: 8
                                                implicitHeight: 100
                                                radius: Theme.radiusSM
                                                opacity: parent.active ? 1.0 : 0.5
                                                color: Theme.scrollBarColor
                                                Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
                                            }
                                        }

                                        delegate: Item {
                                            id: paramsDelegate
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

                                            // optPrevGroup is the proxy-fed
                                            // GroupRole of the previous
                                            // *accepted* row; indexing
                                            // neighbors by source row is wrong
                                            // under a filtered proxy.
                                            // Frequent/Plate pages render
                                            // their rows bare (Orca: a single
                                            // heading, no per-group
                                            // StaticLines).
                                            readonly property bool showGroupHeader:
                                                optGroup !== ""
                                                && (index === 0 || optGroup !== optPrevGroup)
                                                && paramsPage.effectivePage !== "Frequent"
                                                && paramsPage.effectivePage !== "Plate"

                                            width: paramsPage.width
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
                                                compactIndent: 20
                                                compactLabelWidth: 150
                                                compactFieldWidth: 120
                                                compactEnumWidth: 150
                                                valueSource: paramsDelegate.valueSource
                                                oType: paramsDelegate.optType
                                                oKey: paramsDelegate.optKey
                                                oLabel: paramsDelegate.displayLabel
                                                oVal: paramsDelegate.optValue
                                                oMin: paramsDelegate.optMin
                                                oMax: paramsDelegate.optMax
                                                oStep: paramsDelegate.optStep
                                                oRO: paramsDelegate.optReadonly
                                                oDirty: paramsDelegate.optDirty
                                                oTip: paramsDelegate.optTooltip
                                                oUnit: paramsDelegate.optUnit
                                                oSidetext: paramsDelegate.optSidetext
                                                oNullable: paramsDelegate.optNullable
                                                oIsVector: paramsDelegate.optIsVector
                                                oEnumLabels: paramsDelegate.optEnumLabels
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
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
                        if (root.paramsSearchText !== text.trim())
                            root.paramsSearchText = text.trim()
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

    // Preset compare (Orca ParamsPanel m_compare_btn, ParamsPanel.cpp:291):
    // self-contained diff dialog over comparePresetsDetailed; scoped here so
    // the sidebar compare icon reaches it without opening SettingsDialog
    // (which carries its own scoped instance for the settings-side entry).
    PresetDiffDialog {
        id: presetCompareDialog
        configVm: root.configVm
    }

    // Dirty-preset guard for sidebar actions (2026-10-28 user report):
    // queuePendingAction blocks scope/preset switches while the preset
    // carries unsaved edits and emits pendingUnsavedChangesRequested — the
    // settings dialog listens, the sidebar did not, so the pills looked
    // dead the moment a value was modified. Same single-modal gate
    // (beginUnsavedDialog) keeps the two listeners from doubling up.
    Connections {
        // null-safe target: pages instantiate the sidebar without a configVm
        // in the smoke harness, and a null Connections target is legal while
        // an undefined one warns.
        target: root.configVm ? root.configVm : null
        function onPendingUnsavedChangesRequested() {
            if (root.configVm && root.configVm.beginUnsavedDialog()
                && sidebarUnsavedLoader.item)
                sidebarUnsavedLoader.item.openDialog()
        }
    }

    Loader {
        id: sidebarUnsavedLoader
        active: !!root.configVm
        sourceComponent: UnsavedChangesDialog {
            id: sidebarUnsavedDialog
            configVm: root.configVm
            presetTier: root.configVm ? root.configVm.activePresetTier : "print"

            onAccepted: {
                if (!root.configVm)
                    return
                if (sidebarUnsavedDialog.action === "save") {
                    root.configVm.requestSavePendingChanges()
                } else if (sidebarUnsavedDialog.action === "discard") {
                    root.configVm.requestDiscardPendingChanges()
                } else if (sidebarUnsavedDialog.action === "transfer") {
                    root.configVm.transferPendingChanges(sidebarUnsavedDialog.checkedKeys)
                }
                root.configVm.endUnsavedDialog()
            }

            onRejected: {
                if (root.configVm) {
                    root.configVm.requestCancelPendingChanges()
                    root.configVm.endUnsavedDialog()
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
                    Backend.postError(root.configVm.presetActionBlocker(
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

    // Scope pills live inside the process PixelHeader row (sidebar-5):
    // the Global/Object pair, joined capsule like the reference band. The
    // PLATE scope is reached through the object tree's plate node
    // (treeRowActivated → requestPlateScope) — PREPSB-04's plate wiring
    // stays in this file without a third pill.
    // Scope pills live inside the process PixelHeader row (sidebar-5):
    // the Global/Object pair, joined capsule like the reference band. The
    // PLATE scope is reached through the object tree's plate node
    // (treeRowActivated → requestPlateScope) — PREPSB-04's plate wiring
    // stays in this file without a third pill.
    Component {
        id: processScopePills
        RowLayout {
            id: processScopeBar
            spacing: 0

            // Joined capsule track (Orca SwitchButton): #E7E7E7 rounded
            // rail; the selected segment paints the accent capsule inside.
            Rectangle {
                id: scopeTrack
                Layout.preferredHeight: 24
                Layout.minimumWidth: scopeRow.implicitWidth + 8
                radius: 12
                color: Theme.bgInset

                RowLayout {
                    id: scopeRow
                    anchors.centerIn: parent
                    spacing: 0

                    ScopeSegment {
                        text: qsTr("全局")
                        joined: true
                        // M2.5/G3 fix (2026-10-14, Qt 6.12): bare `a && b` yields
                        // undefined when a is undefined, and Qt 6.12 now WARNS on
                        // assigning undefined to a bool property ("Unable to assign
                        // [undefined] to bool") — surfaced by
                        // QmlSmokeTests::pagesInstantiate(AssemblePage) where
                        // root.configVm is undefined. `!!` forces the guard to a real
                        // boolean so the expression is bool in every branch.
                        selected: !!root.configVm && root.configVm.settingsScope === "global"
                        onClicked: if (root.configVm) root.configVm.requestGlobalScope()
                    }

                    ScopeSegment {
                        text: qsTr("对象")
                        joined: true
                        // 盘并入对象模式后，非全局即对象选中（盘参数
                        // 设置也是对象模式的一页）。
                        selected: !!root.configVm && root.configVm.settingsScope !== "global"
                        toolTipText: qsTr("编辑选中对象/盘的参数")
                        // Orca OnToggled: flipping to 对象 lands on the
                        // model-config page the current selection implies —
                        // an object → its Frequent/tabs, nothing → the plate
                        // page. Never a dead click.
                        onClicked: {
                            if (!root.configVm)
                                return
                            if (root.configVm.settingsScope !== "global")
                                return
                            root.updateScopeForSelection(true)
                        }
                    }
                }
            }

            // Mode switch (upstream ModeSwitchButton m_mode_view,
            // ParamsPanel.cpp:281-284): 简 = simple view (advanced options
            // filtered by the proxy), 全 = complete. Sits right of the scope
            // pills like the reference band.
            Item { Layout.preferredWidth: Theme.spacingMD }

            CxSwitch {
                Layout.preferredHeight: 20
                checkedColor: Theme.sidebarAccent
                checked: root.paramsAdvancedMode
                onToggled: {
                    root.paramsAdvancedMode = checked
                    AppSettings.setValue("paramsAdvancedMode",
                                             checked ? "1" : "0")
                }
                ToolTip.text: qsTr("精简/完整参数模式")
                ToolTip.visible: hovered
            }
        }
    }

    // Filament slot compatibility (G-07): index into configVm
    // .filamentSlotCompatibility; rows beyond the list fall back to list[0],
    // mirroring filamentPresetForSlot's out-of-range fallback to
    // currentFilamentPreset_ (an out-of-range slot judges identically to
    // slot 0).
    // Property reads inside the function are still tracked by the calling
    // bindings, so filamentSlotCompatibilityChanged re-evaluates them.
    function slotIncompatible(i) {
        var list = root.configVm ? root.configVm.filamentSlotCompatibility : []
        return list.length > i ? list[i] === 0 : (list.length > 0 ? list[0] === 0 : false)
    }

    function filamentColor(index) {
        // v5.16 (CIRC-04): configured filament colours (same source as the
        // MMU paint palette) instead of a hardcoded theme palette.
        if (root.editorVm && root.editorVm.extrudersColors
            && index < root.editorVm.extrudersColors.length)
            return root.editorVm.extrudersColors[index]
        var colors = [Theme.statusWarning, Theme.textSecondary, Theme.textSecondary, Theme.statusInfo, Theme.chromeDangerHover]
        return index < colors.length ? colors[index] : Theme.textSecondary
    }

    // Section header: 30px title bar (upstream filament title is
    // SetMinSize(-1, FromDIP(30)), Plater.cpp:2915, printer title 3*em) with
    // leading icon + title, optional inline middle content, then a
    // right-aligned icon group (sidebar-3). The band is the upstream
    // title-bar gradient (SetBackgroundColor(title_bg) #F8F8F8 →
    // BackgroundColor2 #F1F1F1, Plater.cpp:2449,2462-2463); expanded
    // sections show no divider — the reference band sits flush on the white
    // sidebar (the 2px white separator only paces folded sections, and the
    // stack spacing already does that).
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

        // Title-bar band (vertical #F8F8F8 → #F1F1F1 gradient).
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: Theme.titleBandTop }
                GradientStop { position: 1.0; color: Theme.titleBandBottom }
            }
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
            anchors.leftMargin: 8   // SidebarProps::TitlebarMargin (Plater.hpp:126)
            anchors.rightMargin: 6
            spacing: 6

            Image {
                visible: headerRoot.iconSource !== ""
                source: headerRoot.iconSource
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                fillMode: Image.PreserveAspectFit
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

            // Inline middle content hugs the title (the reference 工艺 band
            // renders the scope capsule right of the label); the stretch
            // comes after it so the right-side icon group stays flush.
            Loader {
                sourceComponent: headerRoot.middleComponent
                Layout.alignment: Qt.AlignVCenter
            }

            Item { Layout.fillWidth: true }

            Repeater {
                model: headerRoot.extraActions
                // X2: system CxIconButton (was the PixelIconButton copy).
                delegate: CxIconButton {
                    required property int index
                    required property var modelData
                    buttonSize: 24
                    iconSize: 13
                    cxStyle: CxIconButton.Style.Surface
                    visible: modelData.visible !== false
                    iconSource: modelData.icon
                    toolTipText: modelData.tooltip
                    onClicked: headerRoot.extraActionTriggered(index)
                }
            }

            CxIconButton {
                buttonSize: 24
                iconSize: 13
                cxStyle: CxIconButton.Style.Surface
                visible: headerRoot.actionIcon !== ""
                iconSource: headerRoot.actionIcon
                toolTipText: headerRoot.actionToolTip
                enabled: headerRoot.enabled
                onClicked: headerRoot.actionTriggered()
            }
        }
    }

    // X2 (audit 2026-10-10 ④): the private PixelIconButton component (a
    // near-verbatim CxIconButton clone flagged by the audit) is retired --
    // the sidebar row/header icon buttons are system CxIconButtons now, and
    // the scope pills below are system CxButton Segments instead of the
    // private PixelSegment pseudo-segment.

    // Inline scope pill (ref: small Global/Objects pills inside the title
    // row). X2: system CxButton Segment pills (was the private PixelSegment
    // pseudo-segment) -- same selected = accent capsule + white text look,
    // sourced from the shared control.
    component ScopeSegment: CxButton {
        compact: true
        cxStyle: CxButton.Style.Segment
        // Joined-capsule pills render the Orca #009688 sidebar accent, not
        // the brand green; clicks must not leave a focus ring on the
        // selected segment (the reference SwitchButton has none).
        selectedColor: Theme.sidebarAccent
        focusPolicy: Qt.TabFocus
    }

    // U14: one label/value pair of the persistent Sliced Info card. The row
    // hides itself when the value is empty -- the EditorViewModel slice-stat
    // getters return QString{} when the current plate has no valid (non-stale)
    // slice result, which reproduces upstream SlicedInfo::SetTextAndShow's
    // "N/A rows stay hidden" (Plater.cpp:403-412). Value cell is a read-only
    // selectable TextEdit (U12 pattern, Theme selection tokens).
    component SlicedInfoRow: RowLayout {
        id: slicedInfoRowRoot
        property string label: ""
        property string value: ""

        visible: slicedInfoRowRoot.value.length > 0
        Layout.fillWidth: true
        spacing: 8

        Text {
            text: slicedInfoRowRoot.label
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeXS
        }
        Item { Layout.fillWidth: true }
        TextEdit {
            text: slicedInfoRowRoot.value
            readOnly: true
            selectByMouse: true
            color: Theme.textPrimary
            selectionColor: Theme.selectionColor
            selectedTextColor: Theme.selectionText
            font.pixelSize: Theme.fontSizeSM
            font.family: Theme.fontMono
            wrapMode: TextEdit.NoWrap
        }
    }
// CP 工艺行"参数清单"弹窗（收尾）：按当前过滤结果列出全部参数名与当前值，
// 只读浏览——编辑仍在左侧参数页内进行（与 CP 的清单语义一致：快速核对）。
Dialog {
    id: paramsListDialog
    anchors.centerIn: parent
    modal: true
    width: 420
    height: 560
    title: qsTr("参数清单")

    background: Rectangle {
        radius: Theme.radiusLG
        color: Theme.bgPanel
        border.color: Theme.borderSubtle
    }

    header: Label {
        text: qsTr("参数清单")
        color: Theme.textPrimary
        font.bold: true
        font.pixelSize: Theme.fontSizeLG
        padding: 12
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        ListView {
            id: paramsListview
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 2

            model: root.configVm ? root.configVm.printOptions : null

            delegate: Rectangle {
                // pragma ComponentBehavior: Bound -- model roles must be
                // declared, implicit lookups throw ReferenceError.
                required property int index
                required property string optLabel
                required property var optValue
                width: paramsListview.width
                height: 34
                radius: Theme.radiusSM
                color: index % 2 === 0 ? "transparent" : Theme.bgInset
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 8
                    Text {
                        Layout.preferredWidth: 190
                        text: optLabel
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        elide: Text.ElideRight
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: optValue === null ? "—" : String(optValue)
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        elide: Text.ElideRight
                    }
                }
            }
        }

        CxButton {
            Layout.alignment: Qt.AlignRight
            text: qsTr("关闭")
            onClicked: paramsListDialog.close()
        }
    }
}
}
