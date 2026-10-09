pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// U-WIZ -- ConfigWizardDialog: first-run configuration wizard, restructured
// 1:1 against the upstream ConfigWizard dialog
// (third_party/OrcaSlicer/src/slic3r/GUI/ConfigWizard.cpp):
//   - Left ConfigWizardIndex panel (:1475-1509, :1608-1655): one bullet +
//     short-name row per page (past pages = bullet_black, current/hover =
//     bullet_blue, future = bullet_white), click jumps to the page, hover
//     highlights, and the 192px brand logo is pinned to the panel bottom.
//     The index column and the content column are separated by
//     INDEX_MARGIN=40 (ConfigWizard_private.hpp:43). Row height =
//     item_height() = max(bullet 16, em) + em (hpp:515).
//   - Page set follows load_pages (:1769-1801): Printer (the Qt6 vendor
//     preset picker rendered in the upstream PrinterPicker grid form,
//     :198-373 -- thumbnail + bold model name + per-variant checkboxes +
//     All standard/All/None), Custom Printer (:1181-1211), then while the
//     custom checkbox is ticked the indented Firmware (:1213-1241), Bed
//     Shape (:1262-1273) and Print Diameters (:1323-1363) pages, then
//     Filaments (:631-694, four-column list grid + compatible-printer
//     preview). PageWelcome and a summary page are never instantiated in
//     this upstream pin, so neither exists here; Finish applies the config
//     directly (:2739-2785).
//   - Bottom row (:2663-2674, :2705-2707): 1px separator + right-aligned
//     <Back / Next> / Finish / Cancel with BTN_SPACING=10; Next hides on the
//     last page, Cancel closes without applying anything.
//   - Geometry (init_dialog_size :1803-1824): width = index panel + 90*em
//     (em fixed 10, hpp:616), clamped to 9/10 of the screen client area; the
//     height fits the content instead of being fixed.
// Data comes from backend.configViewModel (PresetServiceMock enumeration;
// the vendor/model/variant persistence is AppConfig-lite QSettings). The
// network PresetUpdater remains deferred.
//
// Usage: ConfigWizardDialog { id: wizard }  ->  wizard.open()
CxDialog {
    id: root

    closePolicy: Popup.CloseOnEscape

    // U-TITLE (ConfigWizard.cpp:2631-2632): "<App> - Configuration Wizard"
    // two-segment title carrying the application name prefix.
    dialogTitle: qsTr("OWzx - 配置向导")

    // ── U-GEO: init_dialog_size geometry constants ──
    // Upstream fixes em() to 10 (ConfigWizard_private.hpp:616).
    readonly property int emUnit: 10
    // Index panel minimum width = the 192px logo bitmap (ConfigWizard.cpp:1484).
    readonly property int indexPanelWidth: 192
    // DIALOG_MARGIN=15 / BTN_SPACING=10 (ConfigWizard_private.hpp:42-45).
    readonly property int dialogMargin: 15
    readonly property int btnSpacing: 10
    // Index item row height: max(bullet 16px, em_w) + em_w (hpp:515).
    readonly property int indexItemHeight: 26
    // Content area height fits the tallest (Filaments) page: title + 30em
    // list grid + All/None row + 20em preview, plus page padding.
    readonly property int pageAreaHeight: 34 * emUnit + 20 * emUnit + 60

    // Screen sizes are read through the contentItem: the attached Screen
    // property is Item-scoped and the Popup root does not carry it.
    width: Math.min(indexPanelWidth + 90 * emUnit,
                    contentRoot.screenW * 0.9)
    height: Math.min(pageAreaHeight + 120,
                     contentRoot.screenH * 0.9)

    // Public properties for reading selected values after completion
    // (main.qml applies them via requestCurrentPrinterPreset /
    // requestCurrentFilamentPreset on wizardFinished).
    property string selectedPrinter: ""
    property string selectedFilament: ""

    signal wizardFinished()

    // Lazily resolve the preset service proxy; null-safe (typeof guard
    // matches the PreparePage.qml convention) so the dialog still renders in
    // designer / contexts without a backend context property.
    readonly property var configVm: typeof backend !== "undefined" && backend ? backend.configViewModel : null
    // All available vendor names (filename scan, no preset load). The vendor
    // picker lists all availableVendorNames() and loads the chosen vendor's
    // presets on demand (wizardLoadVendor).
    readonly property var availableVendors: configVm ? configVm.wizardAvailableVendorNames() : []
    // The selected vendor is user-selectable. Defaults to the persisted value
    // (AppConfig-lite), else the first already-loaded vendor, else empty.
    property string activeVendor: {
        if (!configVm) return ""
        const saved = configVm.wizardSelectedVendor()
        if (saved.length > 0) return saved
        const loaded = configVm.wizardVendors()
        return loaded.length > 0 ? loaded[0] : ""
    }
    // Ensure the active vendor's presets are loaded when it changes.
    onActiveVendorChanged: if (configVm && activeVendor.length > 0) configVm.wizardLoadVendor(activeVendor)

    // U-WIZ: PrinterPicker cells for the active vendor
    // (ConfigViewModel::wizardPrinterPickerGroups groups the flat preset
    // names into {model, variants} entries).
    readonly property var pickerGroups: configVm && activeVendor.length > 0
        ? configVm.wizardPrinterPickerGroups(activeVendor) : []
    // Any grid variant picker having more than one nozzle (upstream
    // is_variants, ConfigWizard.cpp:283-291) -- gates the "All standard".
    readonly property bool pickerHasAlternates: {
        for (let i = 0; i < pickerGroups.length; i++)
            if (pickerGroups[i].variants.length > 1) return true
        return false
    }
    // Variant checkbox mirror. Keys are "<model>\u001f<preset>"; seeded from
    // the AppConfig-lite store and re-assigned wholesale so the
    // anyVariantSelected binding tracks edits.
    property var variantStates: ({})
    readonly property bool anyVariantSelected: {
        const keys = Object.keys(variantStates)
        for (let i = 0; i < keys.length; i++)
            if (variantStates[keys[i]]) return true
        return false
    }
    onPickerGroupsChanged: refreshVariantStates()
    Component.onCompleted: refreshVariantStates()

    // ── Custom-printer path state (PageCustom / PageFirmware / PageBedShape /
    // PageDiameters feed upstream custom_config) ──
    property bool customWanted: false
    property string customProfileName: "My Settings"
    property int gcodeFlavorIndex: 0
    // Upstream gcode_flavor enum_values (PrintConfig.cpp:4294-4299),
    // index-aligned with wizardGcodeFlavorLabels().
    readonly property var gcodeFlavorValues: ["marlin", "klipper", "reprapfirmware", "repetier", "marlin2"]
    property int bedShapeType: 0
    property string bedSizeX: "200"
    property string bedSizeY: "200"
    property string nozzleDiameter: "0.4"
    property string filamentDiameter: "3.0"

    // ── Filaments page state (PageMaterials list selections) ──
    // Multi-select printer filter ("(All)" handled by the sentinel key);
    // empty = all printers (upstream list_printer starts with "(All)"
    // selected, ConfigWizard.cpp:741-748).
    readonly property string allPrintersKey: "(All)"
    property var selectedPrinterFilters: []
    property string selectedTypeFilter: ""
    property var selectedVendorFilters: []
    property var checkedProfiles: []
    property string hoveredProfile: ""

    // Filament presets of every loaded vendor, grouped per vendor (upstream
    // Materials holds all bundles' filament presets).
    readonly property var profilesByVendor: {
        const map = {}
        if (configVm) {
            const vendors = configVm.wizardVendors()
            for (let i = 0; i < vendors.length; i++) {
                const names = configVm.wizardMaterialsForVendorAndPrinter(vendors[i], "")
                const list = []
                for (let j = 0; j < names.length; j++)
                    list.push(names[j])
                map[vendors[i]] = list
            }
        }
        return map
    }
    readonly property var filamentVendorList: Object.keys(profilesByVendor)
    readonly property var filamentTypeList: configVm ? configVm.wizardFilamentTypes() : []

    // Printer column: "(All)" first (upstream reload_presets appends
    // "(All)" before the printer presets, ConfigWizard.cpp:741), then the
    // active vendor's printer preset names.
    readonly property var filamentPrinterList: {
        const list = [allPrintersKey]
        const names = configVm && activeVendor.length > 0
            ? configVm.wizardPrinterModelsForVendor(activeVendor) : []
        for (let i = 0; i < names.length; i++)
            list.push(names[i])
        return list
    }

    // update_lists analog (ConfigWizard.cpp:784-884): intersect the printer /
    // type / vendor dimensions.
    readonly property var filteredProfiles: {
        let list = []
        const vendors = filamentVendorList
        for (let vi = 0; vi < vendors.length; vi++) {
            const vendor = vendors[vi]
            if (selectedVendorFilters.length > 0 && selectedVendorFilters.indexOf(vendor) < 0)
                continue
            let names = profilesByVendor[vendor] || []
            if (selectedPrinterFilters.length > 0) {
                const allowed = []
                for (let pi = 0; pi < selectedPrinterFilters.length; pi++) {
                    const compatible = configVm
                        ? configVm.wizardMaterialsForVendorAndPrinter(vendor, selectedPrinterFilters[pi]) : []
                    for (let ci = 0; ci < compatible.length; ci++)
                        if (allowed.indexOf(compatible[ci]) < 0)
                            allowed.push(compatible[ci])
                }
                names = names.filter(function (n) { return allowed.indexOf(n) >= 0 })
            }
            if (selectedTypeFilter.length > 0)
                names = names.filter(function (n) { return n.toUpperCase().indexOf(selectedTypeFilter) >= 0 })
            list = list.concat(names)
        }
        return list
    }

    // Preview window profile: hover wins, else the first checked profile
    // (upstream on_material_highlighted re-renders the html window).
    readonly property string previewProfile: {
        if (hoveredProfile.length > 0) return hoveredProfile
        for (let i = 0; i < filteredProfiles.length; i++)
            if (checkedProfiles.indexOf(filteredProfiles[i]) >= 0)
                return filteredProfiles[i]
        return ""
    }
    readonly property string previewProfileVendor: profileVendor(previewProfile)
    readonly property var previewPrinters: (configVm && previewProfile.length > 0)
        ? configVm.wizardCompatiblePrinters(previewProfileVendor, previewProfile) : []

    // ── Page definitions (upstream constructor order + load_pages
    // visibility) ──
    readonly property var pageDefs: [
        { key: "printer",   title: qsTr("选择打印机"),       short: qsTr("打印机"),       indent: 0 },
        { key: "custom",    title: qsTr("自定义打印机设置"),  short: qsTr("自定义打印机"), indent: 0 },
        { key: "firmware",  title: qsTr("固件类型"),         short: qsTr("固件"),         indent: 1 },
        { key: "bed",       title: qsTr("热床形状和尺寸"),    short: qsTr("热床形状"),     indent: 1 },
        { key: "diams",     title: qsTr("耗材与喷嘴直径"),    short: qsTr("打印直径"),     indent: 1 },
        { key: "filaments", title: qsTr("耗材配置文件选择"),  short: qsTr("耗材"),         indent: 0 }
    ]
    // load_pages (ConfigWizard.cpp:1769-1801): the custom sub-pages join the
    // index only while custom_wanted; the Filaments page only while any FFF
    // printer variant is selected.
    readonly property var visibleKeys: {
        const keys = ["printer", "custom"]
        if (customWanted) keys.push("firmware", "bed", "diams")
        if (anyVariantSelected) keys.push("filaments")
        return keys
    }
    // Delegate-to-page component map (keys match visibleKeys entries).
    readonly property var pageComponents: ({
        "printer": printerPageComp,
        "custom": customPageComp,
        "firmware": firmwarePageComp,
        "bed": bedPageComp,
        "diams": diamsPageComp,
        "filaments": filamentsPageComp
    })
    onVisibleKeysChanged: {
        if (typeof swipeView !== "undefined" && swipeView.currentIndex >= swipeView.count)
            swipeView.currentIndex = Math.max(0, swipeView.count - 1)
    }

    function pageDef(key) {
        for (let i = 0; i < pageDefs.length; i++)
            if (pageDefs[i].key === key)
                return pageDefs[i]
        return pageDefs[0]
    }

    function profileVendor(profile) {
        const vendors = filamentVendorList
        for (let i = 0; i < vendors.length; i++)
            if ((profilesByVendor[vendors[i]] || []).indexOf(profile) >= 0)
                return vendors[i]
        return ""
    }

    function variantKey(model, preset) {
        return model + "\u001f" + preset
    }

    function variantLabel(model, preset) {
        // "<model> <v> nozzle" -> "<v>"; other names fall back to the
        // preset's nozzle_diameter value, then to 0.4 (upstream
        // PageDiameters defaults, ConfigWizard.cpp:1328-1334).
        const suffix = " nozzle"
        if (preset.length > model.length + suffix.length + 1
                && preset.indexOf(model) === 0
                && preset.lastIndexOf(suffix) === preset.length - suffix.length)
            return preset.substring(model.length + 1, preset.length - suffix.length)
        const v = configVm ? configVm.wizardPresetValue(preset, "nozzle_diameter") : ""
        const n = parseFloat(v)
        return isNaN(n) ? "0.4" : String(n)
    }

    function refreshVariantStates() {
        const states = {}
        if (configVm) {
            for (let gi = 0; gi < pickerGroups.length; gi++) {
                const group = pickerGroups[gi]
                const variants = group.variants
                for (let vi = 0; vi < variants.length; vi++)
                    states[variantKey(group.model, variants[vi])] =
                        configVm.wizardPrinterVariantEnabled(activeVendor, group.model, variants[vi])
            }
        }
        variantStates = states
    }

    function setVariantState(model, preset, enabled) {
        const states = Object.assign({}, variantStates)
        states[variantKey(model, preset)] = enabled
        variantStates = states
        if (configVm)
            configVm.wizardSetPrinterVariantEnabled(activeVendor, model, preset, enabled)
    }

    function isVariantChecked(model, preset) {
        return variantStates[variantKey(model, preset)] === true
    }

    // PrinterPicker::select_all(select, alternates) analog
    // (ConfigWizard.cpp:379-397): standard = first variant of each model.
    function selectPickerAll(select, alternates) {
        for (let gi = 0; gi < pickerGroups.length; gi++) {
            const group = pickerGroups[gi]
            const variants = group.variants
            for (let vi = 0; vi < variants.length; vi++) {
                if (!select) {
                    if (isVariantChecked(group.model, variants[vi]))
                        setVariantState(group.model, variants[vi], false)
                } else if (alternates || vi === 0) {
                    setVariantState(group.model, variants[vi], true)
                }
            }
        }
    }

    function finishWizard() {
        // on_bnt_finish -> EndModal(wxID_OK) -> run -> apply_config
        // (ConfigWizard.cpp:2739-2785). The first selected variant wins as
        // the applied printer preset; the first checked profile wins as the
        // filament.
        let firstPreset = ""
        for (let gi = 0; gi < pickerGroups.length && firstPreset.length === 0; gi++) {
            const group = pickerGroups[gi]
            const variants = group.variants
            for (let vi = 0; vi < variants.length; vi++) {
                if (isVariantChecked(group.model, variants[vi])) {
                    firstPreset = variants[vi]
                    break
                }
            }
        }
        let firstProfile = ""
        for (let fi = 0; fi < filteredProfiles.length && firstProfile.length === 0; fi++)
            if (checkedProfiles.indexOf(filteredProfiles[fi]) >= 0)
                firstProfile = filteredProfiles[fi]

        // Custom-printer path: persist the collected custom config as a USER
        // printer preset (upstream apply_custom_config writers,
        // ConfigWizard.cpp:1243-1260/1275-1292/1365-1392).
        if (customWanted && customProfileName.trim().length > 0 && configVm) {
            const nozzle = isNaN(parseFloat(nozzleDiameter)) ? 0.5 : parseFloat(nozzleDiameter)
            const values = {
                "gcode_flavor": gcodeFlavorValues[gcodeFlavorIndex] || "marlin",
                "printable_area": "0,0\n" + bedSizeX + "," + bedSizeY,
                "nozzle_diameter": nozzle,
                "filament_diameter": isNaN(parseFloat(filamentDiameter)) ? 3.0 : parseFloat(filamentDiameter)
            }
            // set_extrusion_width analog (ConfigWizard.cpp:1380-1392): each
            // width = dmr * nozzle / 0.4, written as an absolute value.
            const widths = {
                "support_line_width": 0.35,
                "top_surface_line_width": 0.40,
                "initial_layer_line_width": 0.42,
                "line_width": 0.45,
                "inner_wall_line_width": 0.45,
                "outer_wall_line_width": 0.45,
                "sparse_infill_line_width": 0.45,
                "internal_solid_infill_line_width": 0.45
            }
            for (const key in widths)
                values[key] = (widths[key] * nozzle / 0.4).toFixed(2)
            if (configVm.wizardCreateCustomPrinterPreset(customProfileName.trim(), values)
                    && firstPreset.length === 0)
                firstPreset = customProfileName.trim()
        }

        root.selectedPrinter = firstPreset
        root.selectedFilament = firstProfile
        if (configVm) {
            if (activeVendor.length > 0)
                configVm.wizardSetSelectedVendor(activeVendor)
            if (root.selectedPrinter.length > 0)
                configVm.wizardSetSelectedPrinterModel(root.selectedPrinter)
        }
        // Finishing the wizard APPLIES the selections (upstream
        // ConfigWizard::apply_config): the current presets switch to what the
        // user picked when a preset of that exact name exists.
        const cfgVm = typeof backend !== "undefined" && backend ? backend.configViewModel : null
        if (cfgVm) {
            if (root.selectedFilament.length > 0
                    && cfgVm.filamentPresetNames.indexOf(root.selectedFilament) >= 0)
                cfgVm.setCurrentFilamentPreset(root.selectedFilament)
            if (root.selectedPrinter.length > 0
                    && cfgVm.printerPresetNames.indexOf(root.selectedPrinter) >= 0)
                cfgVm.setCurrentPrinterPreset(root.selectedPrinter)
        }
        backend.configWizardCompleted = true
        root.wizardFinished()
        root.close()
    }

    contentItem: ColumnLayout {
        id: contentRoot

        // init_dialog_size clamps to 9/10 of the screen client area
        // (ConfigWizard.cpp:1811-1815).
        readonly property int screenW: contentRoot.Screen.desktopAvailableWidth
        readonly property int screenH: contentRoot.Screen.desktopAvailableHeight

        spacing: 0

        // topsizer: index column + INDEX_MARGIN(40) + content column
        // (ConfigWizard.cpp:2659-2661), outer margin DIALOG_MARGIN=15
        // (:2705).
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: root.dialogMargin
            spacing: 40  // upstream INDEX_MARGIN(40), ConfigWizard.cpp:2659-2661 -- layout truth, exempt from the spacing scale

            // ── ConfigWizardIndex (left navigation panel) ──
            Item {
                id: indexPanel
                Layout.fillHeight: true
                Layout.preferredWidth: root.indexPanelWidth

                // Positioner (not a Layout) so each entry keeps its exact
                // item_height() row size (hpp:515).
                Column {
                    id: indexItems
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 0

                    Repeater {
                        model: root.visibleKeys

                        delegate: Item {
                            id: indexEntry
                            required property string modelData
                            required property int index

                            readonly property var def: root.pageDef(modelData)
                            readonly property bool isActive: index === swipeView.currentIndex
                            readonly property bool isPast: index < swipeView.currentIndex

                            width: indexPanel.width
                            height: root.indexItemHeight

                            Row {
                                // x = em_w/2 + indent * em_w
                                // (ConfigWizard.cpp:1626).
                                x: root.emUnit / 2 + indexEntry.def.indent * root.emUnit
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: root.emUnit / 2

                                // Current/hover draw bullet_blue (recolored to
                                // the brand accent), past bullet_black, future
                                // bullet_white (ConfigWizard.cpp:1628-1632).
                                Image {
                                    anchors.verticalCenter: parent.verticalCenter
                                    source: indexEntry.isActive || entryHover.containsMouse
                                        ? "qrc:/qml/assets/icons/bullet_blue.png"
                                        : (indexEntry.isPast
                                            ? "qrc:/qml/assets/icons/bullet_black.png"
                                            : "qrc:/qml/assets/icons/bullet_white.png")
                                    sourceSize.width: 16
                                    sourceSize.height: 16
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: indexEntry.def.short
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeMD
                                }
                            }

                            // Click jumps to the page (LEFT_UP -> go_to,
                            // ConfigWizard.cpp:1506-1508); motion sets the
                            // hover highlight (:1635-1650).
                            MouseArea {
                                id: entryHover
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: swipeView.currentIndex = indexEntry.index
                            }
                        }
                    }
                }

                // Panel-bottom 192px brand logo (ConfigWizard.cpp:1644-1647).
                Image {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    source: "qrc:/qml/assets/icons/OrcaSlicer_192px_transparent.png"
                    sourceSize.width: 192
                    sourceSize.height: 192
                    fillMode: Image.PreserveAspectFit
                }
            }

            // ── hscroll content area (wxScrolledWindow, scroll rate 30,30) ──
            SwipeView {
                id: swipeView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                currentIndex: 0
                interactive: false

                Repeater {
                    model: root.visibleKeys

                    delegate: Item {
                        id: pageHolder
                        required property string modelData

                        Loader {
                            anchors.fill: parent
                            sourceComponent: root.pageComponents[pageHolder.modelData]
                        }
                    }
                }
            }
        }

        // StaticLine between content and the button row
        // (ConfigWizard.cpp:2648/2705-2707).
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.preferredHeight: 1
            color: Theme.separator
        }

        // btnsizer: stretch spacer + <Back / Next> / Finish / Cancel, right
        // aligned with BTN_SPACING=10 (ConfigWizard.cpp:2663-2674).
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: root.dialogMargin
            spacing: root.btnSpacing

            Item { Layout.fillWidth: true }

            CxButton {
                id: btnPrev
                cxStyle: CxButton.Style.Secondary
                text: qsTr("上一步")
                enabled: swipeView.currentIndex > 0
                onClicked: swipeView.decrementCurrentIndex()
            }
            CxButton {
                id: btnNext
                cxStyle: CxButton.Style.Primary
                text: qsTr("下一步")
                // btn_next->Show(!is_last) (ConfigWizard.cpp:2745-2747).
                visible: swipeView.currentIndex < swipeView.count - 1
                onClicked: swipeView.incrementCurrentIndex()
            }
            CxButton {
                id: btnFinish
                cxStyle: CxButton.Style.Primary
                text: qsTr("完成")
                // btn_finish->Enable(any_fff_selected || custom) — at least
                // one printer variant or the custom path must be selected
                // (ConfigWizard.cpp:1795-1797).
                enabled: root.anyVariantSelected || root.customWanted
                onClicked: root.finishWizard()
            }
            CxButton {
                id: btnCancel
                cxStyle: CxButton.Style.Secondary
                text: qsTr("取消")
                // Cancel closes without applying (run() non-wxID_OK branch
                // logs "cancelled", ConfigWizard.cpp:2787-2789).
                onClicked: root.close()
            }
        }
    }

    // ── Page: Printer (vendor picker + PrinterPicker grid) ──
    Component {
        id: printerPageComp

        // Page-level scroll container (upstream hscroll wxScrolledWindow,
        // SetScrollRate(30,30), ConfigWizard.cpp:2653/2713).
        CxScrollView {
            id: printerPageScroll
            contentHeight: printerPageBody.implicitHeight

            ColumnLayout {
                id: printerPageBody
                width: printerPageScroll.availableWidth
                spacing: Theme.spacingMD

                Text {
                    text: qsTr("选择打印机")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXL
                    font.bold: true
                }

                Row {
                    spacing: root.emUnit / 2
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("厂商:")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeMD
                    }
                    CxComboBox {
                        id: vendorCombo
                        width: 200
                        model: root.availableVendors
                        // Bind currentIndex to activeVendor (two-way via onActivated).
                        currentIndex: {
                            const idx = root.availableVendors.indexOf(root.activeVendor)
                            return idx >= 0 ? idx : 0
                        }
                        onActivated: (idx) => {
                            if (idx >= 0 && idx < root.availableVendors.length)
                                root.activeVendor = root.availableVendors[idx]
                        }
                    }
                }

                // Picker title row: family title + right-aligned selection
                // buttons (ConfigWizard.cpp:336-366). "All standard" only makes
                // sense when some model has alternate nozzles.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: root.btnSpacing

                    Text {
                        text: root.activeVendor.length > 0
                            ? qsTr("%1 机型").arg(root.activeVendor)
                            : qsTr("机型")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXXL
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    CxButton {
                        compact: true
                        cxStyle: CxButton.Style.Secondary
                        visible: root.pickerHasAlternates
                        text: root.pickerGroups.length > 1 ? qsTr("全部标准") : qsTr("标准")
                        onClicked: root.selectPickerAll(true, false)
                    }
                    CxButton {
                        compact: true
                        cxStyle: CxButton.Style.Secondary
                        text: qsTr("全部")
                        onClicked: root.selectPickerAll(true, true)
                    }
                    CxButton {
                        compact: true
                        cxStyle: CxButton.Style.Secondary
                        text: qsTr("无")
                        onClicked: root.selectPickerAll(false, false)
                    }
                }

                // Printer grid: <=MAX_COLS=4 columns, hgap=0, vgap=20 with a 30px
                // row separator (wxFlexGridSizer(cols, 0, 20) + Add(1, 30),
                // ConfigWizard.cpp:301-335). Each cell: thumbnail + bold wrapped
                // model name (MODEL_MIN_WRAP=150) + variant checkboxes with a
                // 0.9x "Alternate nozzles:" label before the second variant.
                GridLayout {
                    Layout.fillWidth: true
                    columns: 4
                    columnSpacing: 0
                    rowSpacing: 30

                    Repeater {
                        model: root.pickerGroups

                        delegate: ColumnLayout {
                            id: pickerCell
                            required property var modelData
                            required property int index

                            readonly property var group: modelData
                            readonly property string modelName: group.model

                            Layout.preferredWidth: 150
                            spacing: Theme.spacingXS

                            // Model thumbnail. The Qt6 preset store carries no
                            // per-model bitmap, so the printer icon plays the
                            // upstream printer_placeholder.png role
                            // (ConfigWizard.cpp:230-236).
                            Rectangle {
                                Layout.bottomMargin: 20
                                Layout.preferredWidth: 120
                                Layout.preferredHeight: 120
                                color: Theme.bgInset
                                radius: Theme.radiusMD
                                Image {
                                    anchors.centerIn: parent
                                    source: "qrc:/qml/assets/icons/printer.svg"
                                    sourceSize.width: 72
                                    sourceSize.height: 72
                                }
                            }

                            Text {
                                text: pickerCell.modelName
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeMD
                                font.bold: true
                                Layout.preferredWidth: 150
                                wrapMode: Text.Wrap
                            }

                            Repeater {
                                model: pickerCell.group.variants

                                delegate: ColumnLayout {
                                    id: variantBlock
                                    required property string modelData
                                    required property int index

                                    spacing: 0

                                    // "Alternate nozzles:" 0.9x font label
                                    // before the second checkbox
                                    // (ConfigWizard.cpp:274-277).
                                    Text {
                                        visible: variantBlock.index === 1
                                        text: qsTr("备用喷嘴:")
                                        color: Theme.textTertiary
                                        font.pixelSize: Theme.fontSizeSM
                                        bottomPadding: Theme.spacingXS
                                    }

                                    CxCheckBox {
                                        text: qsTr("%1 mm 喷嘴").arg(
                                            root.variantLabel(pickerCell.modelName, variantBlock.modelData))
                                        checked: root.isVariantChecked(pickerCell.modelName, variantBlock.modelData)
                                        onToggled: root.setVariantState(
                                            pickerCell.modelName, variantBlock.modelData, checked)
                                    }
                                }
                            }
                        }
                    }
                }

                // Empty-state hint when no printer preset is available
                Text {
                    Layout.fillWidth: true
                    visible: root.pickerGroups.length === 0
                    wrapMode: Text.WordWrap
                    color: Theme.statusWarning
                    font.pixelSize: Theme.fontSizeSM
                    text: qsTr("未发现打印机预设，请稍后在设置中导入厂商配置。")
                }
            }
        }
    }

    // ── Page: Custom Printer (PageCustom, ConfigWizard.cpp:1181-1211) ──
    Component {
        id: customPageComp

        // Page-level scroll container (upstream hscroll wxScrolledWindow,
        // SetScrollRate(30,30), ConfigWizard.cpp:2653/2713).
        CxScrollView {
            id: customPageScroll
            contentHeight: customPageBody.implicitHeight

            ColumnLayout {
                id: customPageBody
                width: customPageScroll.availableWidth
                spacing: Theme.spacingMD

                Text {
                    text: qsTr("自定义打印机设置")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXL
                    font.bold: true
                }

                CxCheckBox {
                    id: cbCustom
                    text: qsTr("定义自定义打印机配置")
                    checked: root.customWanted
                    // cb_custom -> on_custom_setup -> load_pages: the indented
                    // Firmware / Bed Shape / Diameters pages exist only while this
                    // is ticked (ConfigWizard.cpp:1196-1200/1780-1786).
                    onToggled: root.customWanted = checked
                }

                Text {
                    text: qsTr("自定义配置名称:")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeMD
                }

                CxTextField {
                    id: tcProfileName
                    Layout.preferredWidth: 8 * root.emUnit
                    enabled: root.customWanted
                    text: root.customProfileName
                    // KILL_FOCUS: empty input falls back to the previous value or
                    // "My Settings" (ConfigWizard.cpp:1188-1194).
                    onActiveFocusChanged: {
                        if (!activeFocus) {
                            if (text.trim().length === 0)
                                text = root.customProfileName.length > 0
                                    ? root.customProfileName : "My Settings"
                            root.customProfileName = text
                        }
                    }
                }
            }
        }
    }
    // ── Page: Firmware (PageFirmware, ConfigWizard.cpp:1213-1241) ──
    Component {
        id: firmwarePageComp

        // Page-level scroll container (upstream hscroll wxScrolledWindow,
        // SetScrollRate(30,30), ConfigWizard.cpp:2653/2713).
        CxScrollView {
            id: firmwarePageScroll
            contentHeight: firmwarePageBody.implicitHeight

            ColumnLayout {
                id: firmwarePageBody
                width: firmwarePageScroll.availableWidth
                spacing: Theme.spacingMD

                Text {
                    text: qsTr("固件类型")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXL
                    font.bold: true
                }

                Text {
                    text: qsTr("选择打印机所使用的固件类型。")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Text {
                    // gcode_opt.tooltip ("What kind of G-code the printer is
                    // compatible with.", PrintConfig.cpp:4292).
                    text: qsTr("打印机兼容的 G-code 类型。")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                CxComboBox {
                    id: gcodePicker
                    Layout.preferredWidth: 200
                    model: root.configVm ? root.configVm.wizardGcodeFlavorLabels() : []
                    // Default selection = the upstream gcode_flavor default
                    // (gcfMarlinLegacy, index 0).
                    currentIndex: root.gcodeFlavorIndex
                    onActivated: (idx) => root.gcodeFlavorIndex = idx
                }
            }
        }
    }
    // ── Page: Bed Shape (PageBedShape + BedShapePanel, ConfigWizard.cpp:
    // 1262-1273) ──
    Component {
        id: bedPageComp

        // Page-level scroll container (upstream hscroll wxScrolledWindow,
        // SetScrollRate(30,30), ConfigWizard.cpp:2653/2713).
        CxScrollView {
            id: bedPageScroll
            contentHeight: bedPageBody.implicitHeight

            ColumnLayout {
                id: bedPageBody
                width: bedPageScroll.availableWidth
                spacing: Theme.spacingMD

                Text {
                    text: qsTr("热床形状和尺寸")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXL
                    font.bold: true
                }

                Text {
                    text: qsTr("设置打印机热床的形状。")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                // BedShapePanel: read-only 3-entry shape combo (Rectangle /
                // Circle / Custom).
                Row {
                    spacing: root.emUnit / 2
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("热床形状:")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeMD
                    }
                    CxComboBox {
                        id: bedShapeCombo
                        width: 200
                        model: [qsTr("矩形"), qsTr("圆形"), qsTr("自定义")]
                        currentIndex: root.bedShapeType
                        onActivated: (idx) => root.bedShapeType = idx
                    }
                }

                // Settings: Size (X)/(Y) point fields (mm).
                Row {
                    spacing: root.emUnit / 2
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("尺寸 (X) (mm):")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeMD
                    }
                    CxTextField {
                        id: bedSizeXField
                        width: 8 * root.emUnit
                        text: root.bedSizeX
                        onActiveFocusChanged: {
                            if (!activeFocus) {
                                if (isNaN(parseFloat(text)) || parseFloat(text) <= 0)
                                    text = root.bedSizeX
                                else
                                    root.bedSizeX = text
                            }
                        }
                    }
                }
                Row {
                    spacing: root.emUnit / 2
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("尺寸 (Y) (mm):")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeMD
                    }
                    CxTextField {
                        id: bedSizeYField
                        width: 8 * root.emUnit
                        text: root.bedSizeY
                        onActiveFocusChanged: {
                            if (!activeFocus) {
                                if (isNaN(parseFloat(text)) || parseFloat(text) <= 0)
                                    text = root.bedSizeY
                                else
                                    root.bedSizeY = text
                            }
                        }
                    }
                }
            }
        }
    }
    // ── Page: Diameters (PageDiameters, ConfigWizard.cpp:1323-1363) ──
    Component {
        id: diamsPageComp

        // Page-level scroll container (upstream hscroll wxScrolledWindow,
        // SetScrollRate(30,30), ConfigWizard.cpp:2653/2713).
        CxScrollView {
            id: diamsPageScroll
            contentHeight: diamsPageBody.implicitHeight

            ColumnLayout {
                id: diamsPageBody
                width: diamsPageScroll.availableWidth
                spacing: Theme.spacingMD

                Text {
                    text: qsTr("耗材与喷嘴直径")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXL
                    font.bold: true
                }

                Text {
                    text: qsTr("输入打印机喷头喷嘴的直径。")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                // "Nozzle Diameter:" [editable field] "mm" 3-column flex row with
                // 5px gaps (ConfigWizard.cpp:1341-1349).
                Row {
                    spacing: Theme.spacingXS
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("喷嘴直径:")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                    }
                    CxTextField {
                        id: diamNozzleField
                        // DiamTextCtrl width = Field::def_width_thinner() * em
                        // (ConfigWizard.cpp:1307-1321).
                        width: 4 * root.emUnit
                        leftPadding: 4
                        rightPadding: 4
                        text: root.nozzleDiameter
                        // KILL_FOCUS validation: invalid input falls back to 0.5
                        // (focus_event, ConfigWizard.cpp:1285-1305).
                        onActiveFocusChanged: {
                            if (!activeFocus) {
                                if (isNaN(parseFloat(text)))
                                    text = root.nozzleDiameter
                                else
                                    root.nozzleDiameter = text
                            }
                        }
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("mm")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                    }
                }

                Item { Layout.preferredHeight: 10 }

                Text {
                    text: qsTr("输入耗材的直径。")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Text {
                    text: qsTr("精度要求较高，请使用卡尺沿耗材多点测量后取平均值。")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Row {
                    spacing: Theme.spacingXS
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("耗材直径:")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                    }
                    CxTextField {
                        id: diamFilamField
                        width: 4 * root.emUnit
                        leftPadding: 4
                        rightPadding: 4
                        text: root.filamentDiameter
                        // Invalid input falls back to 3.0.
                        onActiveFocusChanged: {
                            if (!activeFocus) {
                                if (isNaN(parseFloat(text)))
                                    text = root.filamentDiameter
                                else
                                    root.filamentDiameter = text
                            }
                        }
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("mm")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                    }
                }
            }
        }
    }
    // ── Page: Filaments (PageMaterials, ConfigWizard.cpp:631-694) ──
    Component {
        id: filamentsPageComp

        // Page-level scroll container (upstream hscroll wxScrolledWindow,
        // SetScrollRate(30,30), ConfigWizard.cpp:2653/2713).
        CxScrollView {
            id: filamentsPageScroll
            contentHeight: filamentsPageBody.implicitHeight

            ColumnLayout {
                id: filamentsPageBody
                width: filamentsPageScroll.availableWidth
                spacing: Theme.spacingMD

                Text {
                    text: qsTr("耗材配置文件选择")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeXL
                    font.bold: true
                }

                // 4-column flex grid: header row + list row, hgap = em/2,
                // vgap = em, 4th column and 2nd row growable
                // (ConfigWizard.cpp:651-658). List sizes 23/13/13/23 em wide,
                // 30 em high (:643-649).
                GridLayout {
                    Layout.fillWidth: true
                    columns: 4
                    columnSpacing: root.emUnit / 2
                    rowSpacing: root.emUnit

                    Text {
                        text: qsTr("打印机:")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                    }
                    Text {
                        text: qsTr("类型:")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                    }
                    Text {
                        text: qsTr("厂商:")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                    }
                    Text {
                        text: qsTr("配置文件:")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                    }

                    // Printer list: multi-select with "(All)" first entry
                    // (StringList wxLB_MULTIPLE, ConfigWizard.cpp:634/741).
                    ListView {
                        id: printerFilterList
                        Layout.preferredWidth: 23 * root.emUnit
                        Layout.preferredHeight: 30 * root.emUnit
                        Layout.fillHeight: true
                        clip: true
                        model: root.filamentPrinterList
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar {}
                        delegate: Item {
                            id: printerFilterRow
                            required property string modelData
                            required property int index
                            width: printerFilterList.width
                            height: 20
                            Rectangle {
                                anchors.fill: parent
                                color: listRowHover.containsMouse || printerFilterList.currentIndex === printerFilterRow.index
                                    ? Theme.bgHover : "transparent"
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 4
                                text: printerFilterRow.modelData === root.allPrintersKey
                                    ? qsTr("(全部)") : printerFilterRow.modelData
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeSM
                                elide: Text.ElideRight
                                width: parent.width - 8
                            }
                            MouseArea {
                                id: listRowHover
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    // Multi-select toggle; "(All)" clears the
                                    // explicit selection (upstream select (All) =
                                    // no printer restriction).
                                    const list = root.selectedPrinterFilters.slice()
                                    const idx = list.indexOf(printerFilterRow.modelData)
                                    if (idx >= 0)
                                        list.splice(idx, 1)
                                    else if (printerFilterRow.modelData === root.allPrintersKey)
                                        list.length = 0
                                    else
                                        list.push(printerFilterRow.modelData)
                                    root.selectedPrinterFilters = list
                                    printerFilterList.currentIndex = printerFilterRow.index
                                }
                            }
                        }
                    }

                    // Type list (single select; empty selection = all).
                    ListView {
                        id: typeFilterList
                        Layout.preferredWidth: 13 * root.emUnit
                        Layout.preferredHeight: 30 * root.emUnit
                        Layout.fillHeight: true
                        clip: true
                        model: root.filamentTypeList
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar {}
                        delegate: Item {
                            id: typeFilterRow
                            required property string modelData
                            required property int index
                            width: typeFilterList.width
                            height: 20
                            Rectangle {
                                anchors.fill: parent
                                color: typeRowHover.containsMouse || typeFilterList.currentIndex === typeFilterRow.index
                                    ? Theme.bgHover : "transparent"
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 4
                                text: typeFilterRow.modelData
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeSM
                            }
                            MouseArea {
                                id: typeRowHover
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.selectedTypeFilter =
                                        root.selectedTypeFilter === typeFilterRow.modelData
                                            ? "" : typeFilterRow.modelData
                                    typeFilterList.currentIndex = typeFilterRow.index
                                }
                            }
                        }
                    }

                    // Vendor list (single select; empty selection = all).
                    ListView {
                        id: vendorFilterList
                        Layout.preferredWidth: 13 * root.emUnit
                        Layout.preferredHeight: 30 * root.emUnit
                        Layout.fillHeight: true
                        clip: true
                        model: root.filamentVendorList
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar {}
                        delegate: Item {
                            id: vendorFilterRow
                            required property string modelData
                            required property int index
                            width: vendorFilterList.width
                            height: 20
                            Rectangle {
                                anchors.fill: parent
                                color: vendorRowHover.containsMouse || vendorFilterList.currentIndex === vendorFilterRow.index
                                    ? Theme.bgHover : "transparent"
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 4
                                text: vendorFilterRow.modelData
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeSM
                                elide: Text.ElideRight
                                width: parent.width - 8
                            }
                            MouseArea {
                                id: vendorRowHover
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    const list = root.selectedVendorFilters.slice()
                                    const idx = list.indexOf(vendorFilterRow.modelData)
                                    if (idx >= 0)
                                        list.splice(idx, 1)
                                    else
                                        list.push(vendorFilterRow.modelData)
                                    root.selectedVendorFilters = list
                                    vendorFilterList.currentIndex = vendorFilterRow.index
                                }
                            }
                        }
                    }

                    // Profile column: wxCheckListBox (checkbox multi-select).
                    ListView {
                        id: profileList
                        Layout.preferredWidth: 23 * root.emUnit
                        Layout.preferredHeight: 30 * root.emUnit
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: root.filteredProfiles
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar {}
                        delegate: Item {
                            id: profileRow
                            required property string modelData
                            required property int index
                            width: profileList.width
                            height: 22
                            CxCheckBox {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 2
                                width: parent.width - 4
                                text: profileRow.modelData
                                font.pixelSize: Theme.fontSizeSM
                                checked: root.checkedProfiles.indexOf(profileRow.modelData) >= 0
                                onToggled: {
                                    const list = root.checkedProfiles.slice()
                                    const idx = list.indexOf(profileRow.modelData)
                                    if (idx >= 0)
                                        list.splice(idx, 1)
                                    else
                                        list.push(profileRow.modelData)
                                    root.checkedProfiles = list
                                }
                                MouseArea {
                                    id: profileHover
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.NoButton
                                    onContainsMouseChanged: root.hoveredProfile =
                                        containsMouse ? profileRow.modelData : ""
                                }
                            }
                        }
                    }

                    // Empty first grid columns under the three filter lists, the
                    // All/None buttons sit right-aligned in the profile column
                    // (ConfigWizard.cpp:665-686).
                    Item { Layout.preferredWidth: 23 * root.emUnit; Layout.preferredHeight: 1 }
                    Item { Layout.preferredWidth: 13 * root.emUnit; Layout.preferredHeight: 1 }
                    Item { Layout.preferredWidth: 13 * root.emUnit; Layout.preferredHeight: 1 }
                    RowLayout {
                        Layout.alignment: Qt.AlignRight
                        spacing: root.emUnit / 2
                        CxButton {
                            compact: true
                            cxStyle: CxButton.Style.Secondary
                            text: qsTr("全部")
                            onClicked: root.checkedProfiles = root.filteredProfiles.slice()
                        }
                        CxButton {
                            compact: true
                            cxStyle: CxButton.Style.Secondary
                            text: qsTr("无")
                            onClicked: root.checkedProfiles = []
                        }
                    }
                }

                // Compatible-printers preview window (wxHtmlWindow 60em x 20em,
                // ConfigWizard.cpp:692-693/758).
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 20 * root.emUnit
                    color: Theme.bgInset
                    border.color: Theme.borderInput
                    border.width: 1
                    radius: Theme.radiusSM

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 4

                        Text {
                            text: root.previewProfile.length > 0
                                ? qsTr("兼容打印机 (%1):").arg(root.previewProfile)
                                : qsTr("兼容打印机:")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeMD
                            font.bold: true
                        }

                        ListView {
                            id: previewList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: root.previewPrinters
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar {}
                            delegate: Item {
                                id: previewRow
                                required property string modelData
                                width: previewList.width
                                height: 18
                                Row {
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 4
                                    Text {
                                        text: "✓"
                                        color: Theme.accent
                                        font.pixelSize: Theme.fontSizeSM
                                        font.bold: true
                                    }
                                    Text {
                                        text: previewRow.modelData
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeSM
                                    }
                                }
                            }
                        }

                        Text {
                            visible: root.previewPrinters.length === 0
                            text: qsTr("未发现耗材预设，请稍后在设置中导入厂商配置。")
                            color: Theme.textDisabled
                            font.pixelSize: Theme.fontSizeSM
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }
    }
}
