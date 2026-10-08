pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// CreatePresetsDialog.qml — upstream CreatePresetsDialog.cpp port (two dialogs)
//
// Upstream splits preset creation per scope:
//   - CreateFilamentPresetDialog (CreatePresetsDialog.cpp:659-724): "Create
//     Filament", 600x480 (@667), two sections — "Basic Information"
//     (Vendor / Type / Serial, @676-682) + "Add Filament Preset under this
//     filament" (@690) with the RadioBox base selection (@948/1018) and the
//     3-column printer CheckBox grid inside a max-350 scrolled panel (@700,
//     wxGridSizer(3,...) @1034). Preset name is composed from
//     vendor + type + serial (@1130), never typed freely.
//   - CreatePrinterPresetDialog (CreatePresetsDialog.cpp:1574-1622): "Create
//     Printer/Nozzle" two-step wizard — step bar (@1637-1672), Page1 (create
//     type RadioBox pair @1714-1715, printer row @1733-1815, nozzle @1875,
//     bed shape "Rectangle" @1953, printable space / origin / hot bed STL/SVG
//     / max print height @1960-2095), Page2 (printer preset vendor/model
//     combos @2603-2622, Presets RadioBox pair @2643-2644, filament/process
//     template CheckBox grids @2667-2726).
// There is no upstream process-create dialog: process presets are created by
// saving the edited Tab under a new name (SavePresetDialog flow,
// Tab.cpp:7350-7402). The Qt6 entry is one dialog scoped by the selector
// below, so the process pane keeps the legacy name + inherits form as a
// documented concession (the scope list itself is test-locked).
//
// Test locks honored in this file (tests/QmlUiAuditTests.cpp):
//   3397  `scopeCategories: [2, 1, 0]`   3399  `createCustomPreset`
//   7913  literal "Inherits from"        7925  `root.configVm.lastPresetError`
//   8890  literal `qsTr("创建预设")`
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    padding: 0

    property var configVm: null

    // v5.16 (PSET2-02): the UI scope order is 打印机/耗材/工艺, which maps to
    // PresetServiceMock::Category PrinterCat=2 / FilamentCat=1 / PrintCat=0
    // (PresetServiceMock.h:18). Test-locked mapping (QmlUiAuditTests:3397).
    readonly property var scopeCategories: [2, 1, 0]
    // Selected category (PresetServiceMock::Category int). Default: process.
    property int selectedCategory: 0

    // Per-scope window titles: upstream "Create Printer/Nozzle" (@1575) and
    // "Create Filament" (@660). The process scope has no upstream dialog, so
    // it keeps the legacy generic title.
    dialogTitle: root.selectedCategory === 2 ? qsTr("创建打印机/喷嘴")
               : root.selectedCategory === 1 ? qsTr("创建耗材")
               : qsTr("创建预设")

    // Geometry: filament SetSize(600, 480) (@667); the printer wizard is
    // Fit()-sized per page (@1614) and page2 grows for the min-660 template
    // panel (@2665). The process pane keeps the legacy 480x280 shell.
    width: root.selectedCategory === 2 ? 700 : 600
    height: {
        if (root.selectedCategory === 0) return 280
        if (root.selectedCategory !== 2) return 480
        if (root.printerPage === 2) return 690
        return root.createNozzle ? 380 : 640
    }

    // Upstream static filament vendor list (CreatePresetsDialog.cpp:43-64),
    // case-insensitively sorted like create_vendor_item (@784).
    readonly property var upstreamFilamentVendors: [
        "3D BEST-Q", "3D Hero", "3D-Fuel", "3Dgenius", "3DJake", "3DXTECH",
        "Aceaddity", "AddNorth", "Amazon Basics", "AMOLEN", "Ankermake",
        "Anycubic", "Atomic", "AzureFilm", "BASF", "Bblife", "BCN3D",
        "Beyond Plastic", "California Filament", "Capricorn", "CC3D",
        "CERPRiSE", "colorFabb", "Comgrow", "Cookiecad", "Creality",
        "Das Filament", "DO3D", "DOW", "DREMC", "DSM", "Duramic", "ELEGOO",
        "Eryone", "Essentium", "eSUN", "Extrudr", "Fiberforce", "Fiberlogy",
        "Fil X", "FilaCube", "Filamentive", "FilamentOne", "Fillamentum",
        "FLASHFORGE", "Formfutura", "Francofil", "FusRock", "GEEETECH",
        "Giantarm", "Gizmo Dorks", "GreenGate3D", "HATCHBOX", "Hello3D",
        "IC3D", "IEMAI", "IIID Max", "INLAND", "iProspect", "iSANMATE",
        "Justmaker", "Keene Village Plastics", "Kexcelled", "LDO", "MakerBot",
        "MatterHackers", "MIKA3D", "NinjaTek", "Nobufil", "Novamaker",
        "OVERTURE", "OVVNYXE", "Polymaker", "Priline", "Printed Solid",
        "Protopasta", "Prusament", "Push Plastic", "R3D", "RatRig",
        "Re-pet3D", "re3D", "Recreus", "Regen", "Sain SMART", "SliceWorx",
        "Snapmaker", "SnoLabs", "Spectrum", "SUNLU", "Tianse", "TTYT3D",
        "UltiMaker", "Valment", "Verbatim", "VO3D", "Voxelab", "VOXELPLA",
        "YOOPAI", "Yousu", "Ziro", "Zyltech"
    ]

    // Upstream static filament type fallback (CreatePresetsDialog.cpp:66-69),
    // sorted like create_type_item (@873). Only used while no vendor bundle
    // provides filament_type values.
    readonly property var upstreamFilamentTypes: [
        "ABS", "ABST", "ASA", "CPE", "Carbon Fiber", "FLEX", "GLAZE", "HIPS",
        "METAL", "Misc", "NYLON", "Nylon", "PA", "PACF", "PC", "PCABS",
        "PCCF", "PCTG", "PEEK", "PEI", "PET", "PETG", "PETGCF", "PHA", "PLA",
        "PLA Tough", "PLA+", "PP", "PTBA", "PTBA90A", "PVA", "PVB", "SBS",
        "TPE", "TPU", "TPU75D", "TPU92A", "TPU93A", "TPU98A", "rPLA"
    ]

    // Upstream nozzle_diameter_vec (CreatePresetsDialog.cpp:166).
    readonly property var nozzleDiameters: [
        "0.4", "0.15", "0.2", "0.25", "0.3", "0.35", "0.5", "0.6", "0.75",
        "0.8", "1.0", "1.2", "1.75"
    ]

    // ── wizard / pane state ──
    // Printer wizard page (upstream show_page1/show_page2 @2951-2962).
    property int printerPage: 1
    // Page1 create-type RadioBox pair (@1714-1715), default item 0 (@1606).
    property bool createNozzle: false
    // Filament base RadioBox pair (@948/1018), default item 0 (@711).
    property bool filamentFromCurrent: true
    // "Can't find ..." CheckBox swap states (@833-849, @1832-1853, @1916-1933).
    property bool canNotFindVendorChk: false
    property bool canNotFindPrinter: false
    property bool canNotFindNozzle: false
    // Page2 Presets RadioBox pair (@2643-2644), default item 0 (@1607).
    property bool fromTemplate: true
    // Hot bed STL / SVG picks (upstream load_model_stl / load_texture).
    property string bedStlPath: ""
    property string bedSvgPath: ""

    onOpened: {
        // Default scope = process (the legacy entry semantics) → PrintCat.
        scopeCombo.currentIndex = 2
        root.selectedCategory = root.scopeCategories[2]
        resetFilamentPane()
        resetPrinterPane()
        refreshInheritsList()
    }

    // ── helpers ──
    function presetStringValue(v) {
        if (v === null || v === undefined) return ""
        if (Array.isArray(v)) return v.length > 0 ? String(v[0]) : ""
        return String(v)
    }

    // upstream remove_special_key (@176) over the cannot_input_key set (@172).
    function removeSpecialKeys(s) {
        var specials = "!#$%&()*,./:<>?@\\^_|~"
        var out = s
        for (var i = 0; i < specials.length; ++i)
            out = out.split(specials[i]).join("")
        return out.replace(/[\t\n\r]/g, "")
    }

    // upstream get_all_filament_presets → m_system_filament_types_set: the
    // filament_type values of all loaded filament presets, sorted (@869-873);
    // static list fallback while no vendor bundle is loaded.
    function filamentTypeChoices() {
        var types = []
        if (root.configVm) {
            var names = root.configVm.filamentPresetNames || []
            for (var i = 0; i < names.length; ++i) {
                var t = presetStringValue(
                            root.configVm.wizardPresetValue(names[i], "filament_type"))
                if (t.length > 0 && types.indexOf(t) < 0) types.push(t)
            }
        }
        if (types.length === 0) types = root.upstreamFilamentTypes.slice()
        types.sort()
        return types
    }

    // upstream get_filament_preset_choices (@1211-1270): public names (the
    // " @machine" suffix stripped) of filament presets matching the type.
    function filamentPresetChoices(typeName) {
        var out = []
        if (!root.configVm || typeName.length === 0) return out
        var names = root.configVm.filamentPresetNames || []
        for (var i = 0; i < names.length; ++i) {
            var t = presetStringValue(
                        root.configVm.wizardPresetValue(names[i], "filament_type"))
            if (t !== typeName) continue
            var n = names[i]
            var at = n.indexOf(" @")
            if (at > 0) n = n.substring(0, at)
            if (out.indexOf(n) < 0) out.push(n)
        }
        return out
    }

    // Resolve a public combo name back to a stored preset name.
    function filamentBaseSelection() {
        var pub = filPresetCombo.currentText
        if (pub.length === 0 || !root.configVm) return ""
        var names = root.configVm.filamentPresetNames || []
        for (var i = 0; i < names.length; ++i) {
            var n = names[i]
            var at = n.indexOf(" @")
            if (at > 0) n = n.substring(0, at)
            if (n === pub) return names[i]
        }
        return pub
    }

    function pickedFromModel(model) {
        var out = []
        for (var i = 0; i < model.count; ++i) {
            var it = model.get(i)
            if (it.picked) out.push(it.name)
        }
        return out
    }

    function showInfo(message) {
        msgBox.message = message
        msgBox.confirmText = qsTr("确定")
        msgBox.cancelText = qsTr("取消")
        msgBox.height = message.length > 120 ? 340 : (message.length > 60 ? 280 : 220)
        msgBox.openWithAction(null)
    }

    function showConfirm(message, onYes) {
        msgBox.message = message
        msgBox.confirmText = qsTr("确定")
        msgBox.cancelText = qsTr("取消")
        msgBox.height = message.length > 120 ? 340 : (message.length > 60 ? 280 : 220)
        msgBox.openWithAction(onYes)
    }

    // ── filament pane state ──
    function resetFilamentPane() {
        canNotFindVendorChk = false
        filamentFromCurrent = true
        filVendorCombo.currentIndex = -1
        filCustomVendor.text = ""
        filTypeCombo.model = filamentTypeChoices()
        filTypeCombo.currentIndex = -1
        filPresetCombo.model = []
        filPresetCombo.currentIndex = -1
        filSerialInput.text = ""
        rebuildPrinterGrid()
    }

    // 3-column printer CheckBox grid content: every visible printer preset
    // (upstream get_all_visible_printer_name; the compatible_printers filter
    // degenerates to the full list while profiles carry no compat data).
    function rebuildPrinterGrid() {
        printerGridModel.clear()
        var names = root.configVm ? (root.configVm.printerPresetNames || []) : []
        for (var i = 0; i < names.length; ++i)
            printerGridModel.append({ name: names[i], picked: false })
    }

    function createFilamentClicked() {
        // upstream Create button (CreatePresetsDialog.cpp:1047-1202)
        if (!root.configVm) { root.reject(); return }
        var vendorName = ""
        if (!canNotFindVendorChk) {
            if (filVendorCombo.currentText.length === 0) {
                showInfo(qsTr("尚未选择厂商，请重新选择厂商。"))
                return
            }
            vendorName = filVendorCombo.currentText
        } else {
            if (filCustomVendor.text.length === 0) {
                showInfo(qsTr("自定义厂商缺失，请输入自定义厂商。"))
                return
            }
            vendorName = filCustomVendor.text
            if (vendorName === "Bambu" || vendorName === "Generic") {
                showInfo(qsTr("自定义耗材的厂商不能使用 \"Bambu\" 或 \"Generic\"。"))
                return
            }
        }
        if (filTypeCombo.currentText.length === 0) {
            showInfo(qsTr("尚未选择耗材类型，请重新选择类型。"))
            return
        }
        if (filSerialInput.text.length === 0) {
            showInfo(qsTr("耗材系列缺失，请输入系列。"))
            return
        }
        var typeRaw = filTypeCombo.currentText
        // upstream name composition (@1130): "PLA-AERO" displays as "PLA Aero"
        var typeName = typeRaw === "PLA-AERO" ? "PLA Aero" : typeRaw
        vendorName = removeSpecialKeys(vendorName)
        var serialName = removeSpecialKeys(filSerialInput.text)
        if (vendorName.length === 0 || serialName.length === 0) {
            showInfo(qsTr("耗材的厂商或系列输入中可能含有不允许的字符，请删除后重新输入。"))
            return
        }
        vendorName = vendorName.trim()
        serialName = serialName.trim()
        if (vendorName.length === 0 || serialName.length === 0) {
            showInfo(qsTr("自定义厂商或系列输入全为空格，请重新输入。"))
            return
        }
        if (canNotFindVendorChk && /^([0-9]*)$/.test(vendorName)) {
            showInfo(qsTr("厂商不能为数字，请重新输入。"))
            return
        }
        var targets = pickedFromModel(printerGridModel)
        if (targets.length === 0) {
            showInfo(qsTr("尚未选择任何打印机或预设，请至少选择一项。"))
            return
        }
        var presetBaseName = vendorName + " " + typeName + " " + serialName
        var basePreset = filamentFromCurrent
                ? filamentBaseSelection()
                : String(root.configVm.currentFilamentPreset || "")
        if (basePreset.length === 0) {
            showInfo(qsTr("尚未选择基于哪个耗材预设创建，请先选择耗材预设。"))
            return
        }
        // Duplicate names are not disabled upstream — the dialog asks and
        // continues (@1132-1138). Existing targets stay reserved.
        var pending = []
        for (var t = 0; t < targets.length; ++t) {
            var target = presetBaseName + " @ " + targets[t]
            if (!root.configVm.presetExists(target))
                pending.push({ printer: targets[t], name: target })
        }
        if (pending.length === 0) {
            showInfo(qsTr("同名耗材预设均已存在，已保留原有预设。"))
            return
        }
        if (pending.length < targets.length) {
            showConfirm(qsTr("你创建的耗材名称 %1 已存在。\n如果继续，创建的预设将以全名显示。是否继续？").arg(presetBaseName),
                        function() { createFilamentPresets(pending, basePreset, vendorName, typeRaw) })
            return
        }
        createFilamentPresets(pending, basePreset, vendorName, typeRaw)
    }

    function createFilamentPresets(pending, basePreset, vendorName, typeRaw) {
        // upstream clone_presets_for_filament loop (@1144-1199): one preset
        // per checked printer, seeded from the base preset.
        var failures = 0
        for (var i = 0; i < pending.length; ++i) {
            var ok = root.configVm.createPresetFromBase(
                        1 /* FilamentCat */, pending[i].name, basePreset, {
                            filament_vendor: vendorName,
                            compatible_printers: [pending[i].printer],
                            filament_type: typeRaw
                        }, false)
            if (!ok) ++failures
        }
        if (failures > 0) {
            showInfo(qsTr("部分预设创建失败：")
                     + (root.configVm.lastPresetError || qsTr("创建预设失败")))
            return
        }
        openSuccess(false)
    }

    // ── printer wizard state ──
    function resetPrinterPane() {
        printerPage = 1
        createNozzle = false
        canNotFindPrinter = false
        canNotFindNozzle = false
        fromTemplate = true
        p1VendorCombo.model = root.configVm ? root.configVm.wizardVendors() : []
        p1VendorCombo.currentIndex = -1
        p1ModelCombo.model = []
        p1ModelCombo.currentIndex = -1
        p1PrinterCombo.model = root.configVm ? (root.configVm.printerPresetNames || []) : []
        p1PrinterCombo.currentIndex = -1
        p1CustomVendor.text = ""
        p1CustomModel.text = ""
        p1CustomNozzle.text = ""
        p1NozzleCombo.model = root.nozzleDiameters.map(function(n) { return n + " mm" })
        p1NozzleCombo.currentIndex = 0  // upstream SetSelection(0) (@1885)
        p1SpaceX.text = "200"           // upstream defaults (@1974/1984)
        p1SpaceY.text = "200"
        p1OriginX.text = "0"            // upstream defaults (@2007/2017)
        p1OriginY.text = "0"
        p1Height.text = "200"           // upstream default (@2088)
        bedStlPath = ""
        bedSvgPath = ""
        p2VendorCombo.model = root.configVm ? root.configVm.wizardVendors() : []
        p2VendorCombo.currentIndex = -1
        p2ModelCombo.model = []
        p2ModelCombo.currentIndex = -1
        filTplModel.clear()
        procTplModel.clear()
    }

    function rebuildTemplatePanel() {
        // upstream select_curr_radiobox Page2 branch (@2510-2529): the grids
        // clear while no model is selected; otherwise from-template lists the
        // template presets and from-current-printer lists the presets
        // compatible with the selected printer (update_presets_list).
        filTplModel.clear()
        procTplModel.clear()
        var basePreset = p2ModelCombo.currentText
        if (basePreset.length === 0 || !root.configVm) return
        var filNames = fromTemplate
                ? (root.configVm.filamentPresetNames || [])
                : (root.configVm.compatiblePresetsForPrinter(1, basePreset) || [])
        var procNames = fromTemplate
                ? (root.configVm.printPresetNames || [])
                : (root.configVm.compatiblePresetsForPrinter(0, basePreset) || [])
        for (var f = 0; f < filNames.length; ++f)
            filTplModel.append({ name: filNames[f], picked: false })
        for (var p = 0; p < procNames.length; ++p)
            procTplModel.append({ name: procNames[p], picked: false })
    }

    function printerValidatePage1() {
        // upstream validate_input_valid + show_page2 (@2101-2105)
        if (!createNozzle) {
            if (canNotFindPrinter) {
                if (p1CustomVendor.text.trim().length === 0
                        || p1CustomModel.text.trim().length === 0) {
                    showInfo(qsTr("请输入自定义打印机厂商与型号。"))
                    return false
                }
            } else if (p1VendorCombo.currentText.length === 0
                       || p1ModelCombo.currentText.length === 0) {
                showInfo(qsTr("尚未选择打印机厂商与型号，请重新选择。"))
                return false
            }
        } else if (p1PrinterCombo.currentText.length === 0) {
            showInfo(qsTr("尚未选择打印机预设，请重新选择。"))
            return false
        }
        if (canNotFindNozzle && p1CustomNozzle.text.trim().length === 0) {
            showInfo(qsTr("请输入自定义喷嘴直径。"))
            return false
        }
        return true
    }

    // upstream get_nozzle_diameter (@2425-2442)
    function nozzleDiameter() {
        var d = canNotFindNozzle ? p1CustomNozzle.text.trim()
                                 : p1NozzleCombo.currentText
        if (d.lastIndexOf(" mm") === d.length - 3 && d.length > 3)
            d = d.substring(0, d.length - 3)
        d = d.split(",").join(".")
        var f = parseFloat(d)
        if (isNaN(f) || f === 0) d = "0.4"
        return d
    }

    // upstream get_custom_printer_model (@2444-2469)
    function customPrinterModel() {
        if (!createNozzle) {
            var vendor = canNotFindPrinter ? p1CustomVendor.text.trim()
                                           : p1VendorCombo.currentText
            var model = canNotFindPrinter ? p1CustomModel.text.trim()
                                          : p1ModelCombo.currentText
            return vendor + " " + model
        }
        var base = p1PrinterCombo.currentText
        var pm = presetStringValue(root.configVm.wizardPresetValue(base, "printer_model"))
        return pm.length > 0 ? pm : base
    }

    function printerCreateClicked() {
        // upstream Page2 Create (CreatePresetsDialog.cpp:2744-2788)
        if (!root.configVm) { root.reject(); return }
        var basePreset = p2ModelCombo.currentText
        if (basePreset.length === 0) {
            showInfo(qsTr("你尚未选择基于哪个打印机预设创建，请选择打印机的厂商与型号"))
            return
        }
        var model = customPrinterModel()
        var nozzle = nozzleDiameter()
        // upstream name composition (@2767-2775): "<model> <nozzle> nozzle"
        var printerPresetName = model + " " + nozzle + " nozzle"
        if (root.configVm.presetExists(printerPresetName)) {
            showConfirm(qsTr("你创建的打印机预设已存在同名预设。是否覆盖它？\n\t是：覆盖同名打印机预设，同名耗材与工艺预设将被重建，不同名的将被保留。\n\t取消：不创建预设，返回创建界面。"),
                        function() { doPrinterCreate(printerPresetName, basePreset, model, nozzle, true) })
            return
        }
        doPrinterCreate(printerPresetName, basePreset, model, nozzle, false)
    }

    function doPrinterCreate(printerPresetName, basePreset, model, nozzle, overwrite) {
        // template CheckBox grids (upstream @2790-2818)
        var filPicks = pickedFromModel(filTplModel)
        var procPicks = pickedFromModel(procTplModel)
        if (filPicks.length === 0) {
            showInfo(qsTr("你需要至少选择一个耗材预设。"))
            return
        }
        if (procPicks.length === 0) {
            showInfo(qsTr("你需要至少选择一个工艺预设。"))
            return
        }
        var ov = { nozzle_diameter: parseFloat(nozzle) }
        if (!createNozzle) {
            // printable geometry from Page1 (upstream save_printable_area_config)
            var vendor = canNotFindPrinter ? p1CustomVendor.text.trim()
                                           : p1VendorCombo.currentText
            ov.printable_area = (p1SpaceX.text.length > 0 ? p1SpaceX.text : "200")
                    + "x" + (p1SpaceY.text.length > 0 ? p1SpaceY.text : "200")
            ov.printable_origin = (p1OriginX.text.length > 0 ? p1OriginX.text : "0")
                    + "," + (p1OriginY.text.length > 0 ? p1OriginY.text : "0")
            ov.print_height = p1Height.text.length > 0 ? parseFloat(p1Height.text) : 200
            ov.printer_vendor = vendor
            ov.printer_model = model
            if (bedStlPath.length > 0) ov.bed_model = bedStlPath
            if (bedSvgPath.length > 0) ov.bed_texture = bedSvgPath
        }
        if (!root.configVm.createPresetFromBase(
                    2 /* PrinterCat */, printerPresetName, basePreset, ov, overwrite)) {
            showInfo(qsTr("创建预设失败：")
                     + (root.configVm.lastPresetError || qsTr("创建预设失败")))
            return
        }
        // Clone the picked templates under the new printer. Upstream alias
        // naming: filament "alias @machine" (@2795), process "alias @machine"
        // with a missing space (@2810) — kept verbatim.
        var failures = 0
        for (var f = 0; f < filPicks.length; ++f) {
            var filTarget = filPicks[f] + " @ " + printerPresetName
            if (!overwrite && root.configVm.presetExists(filTarget)) continue  // reserved
            if (!root.configVm.createPresetFromBase(
                        1 /* FilamentCat */, filTarget, filPicks[f], {}, overwrite))
                ++failures
        }
        for (var p = 0; p < procPicks.length; ++p) {
            var procTarget = procPicks[p] + " @" + printerPresetName
            if (!overwrite && root.configVm.presetExists(procTarget)) continue  // reserved
            if (!root.configVm.createPresetFromBase(
                        0 /* PrintCat */, procTarget, procPicks[p], {}, overwrite))
                ++failures
        }
        if (failures > 0) {
            showInfo(qsTr("部分预设创建失败：")
                     + (root.configVm.lastPresetError || qsTr("创建预设失败")))
            return
        }
        openSuccess(true)
    }

    function openSuccess(printerMode) {
        // upstream: CreatePresetSuccessfulDialog opens right after the create
        // modal ends OK (Plater.cpp:3313-3324 / 14312-14318). Here the create
        // dialog stays mounted under the success page and closes with it.
        successDialog.printerMode = printerMode
        successDialog.open()
    }

    // ── inline components (upstream widget replicas) ────────────────────────

    // upstream OPTION_SIZE 100x24 label column (@25, @774/866/912 ...)
    component OptionLabel: Text {
        Layout.preferredWidth: 100
        Layout.preferredHeight: 24
        color: Theme.textSecondary
        font.pixelSize: Theme.fontSizeMD
        verticalAlignment: Text.AlignVCenter
    }

    // upstream ComboBox + SetLabel placeholder + SetLabelColor(#ACACAC)
    // (@792-794 etc). QQuickComboBox has no placeholder support, so the
    // placeholder is declared here and rendered through the muted token.
    component PromptComboBox: CxComboBox {
        id: pcbRoot
        property string placeholderText: ""
        implicitHeight: 24
        font.pixelSize: Theme.fontSizeMD
        contentItem: Text {
            leftPadding: Theme.spacingMD
            rightPadding: pcbRoot.indicator.width + Theme.spacingXS
            text: pcbRoot.displayText.length > 0 ? pcbRoot.displayText
                                                 : pcbRoot.placeholderText
            color: pcbRoot.displayText.length > 0 ? Theme.textPrimary
                                                  : Theme.textMuted
            font: pcbRoot.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }

    // upstream create_radio_item (@1273-1290): RadioBox indicator left,
    // text right; the checked color follows the OWzx accent.
    component UpstreamRadio: Item {
        id: radioRoot
        property string label: ""
        property bool checked: false
        signal picked()
        implicitWidth: radioRow.implicitWidth
        implicitHeight: 24
        Row {
            id: radioRow
            spacing: 5  // upstream 5px indicator/text gap (@1278)
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 14
                height: 14
                radius: 7
                color: "transparent"
                border.color: radioRoot.checked ? Theme.accent : Theme.borderDefault
                border.width: 1
                Rectangle {
                    anchors.centerIn: parent
                    width: 6
                    height: 6
                    radius: 3
                    color: Theme.accent
                    visible: radioRoot.checked
                }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: radioRoot.label
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
            }
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: radioRoot.picked()
        }
    }

    // upstream TextInput + SetMaxLength(50) + cannot_input_key char filter
    // (@802-814, @916-926).
    component PresetNameField: CxTextField {
        implicitHeight: 24
        font.pixelSize: Theme.fontSizeMD
        maximumLength: 50
        validator: RegularExpressionValidator {
            regularExpression: /^[^!#$%&()*,.\/:<>?@\\^_|~\t\n\r]*$/
        }
    }

    // upstream TextInput with the "mm" side suffix + wxFILTER_DIGITS validator
    // (@1974-1976, @2007-2018, @2088-2090).
    component DigitsField: Item {
        id: fieldRoot
        property alias text: input.text
        implicitWidth: 100
        implicitHeight: 24
        CxTextField {
            id: input
            anchors.fill: parent
            implicitHeight: 24
            font.pixelSize: Theme.fontSizeMD
            rightPadding: unitText.width + Theme.spacingSM
            validator: RegularExpressionValidator { regularExpression: /^[0-9]*$/ }
        }
        Text {
            id: unitText
            anchors.right: parent.right
            anchors.rightMargin: Theme.spacingSM
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("mm")
            color: Theme.textMuted
            font.pixelSize: Theme.fontSizeSM
        }
    }

    // upstream DialogButtons full-width bottom strip (@708/1694/2575); Page2
    // renders the left-aligned Return button (@2738).
    component DialogButtonsBar: Rectangle {
        id: barRoot
        Layout.fillWidth: true
        height: Theme.dialogFooterHeight
        color: Theme.bgSurface
        property string okText: qsTr("创建")
        property bool okEnabled: true
        property bool showReturn: false
        signal okClicked()
        signal cancelClicked()
        signal returnClicked()
        RowLayout {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: Theme.spacingLG
            spacing: Theme.spacingMD
            CxButton {
                visible: barRoot.showReturn
                text: qsTr("返回")
                cxStyle: CxButton.Style.Secondary
                onClicked: barRoot.returnClicked()
            }
        }
        RowLayout {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.rightMargin: Theme.spacingLG
            spacing: Theme.spacingMD
            CxButton {
                text: barRoot.okText
                enabled: barRoot.okEnabled
                cxStyle: CxButton.Style.Primary
                onClicked: barRoot.okClicked()
            }
            CxButton {
                text: qsTr("取消")
                cxStyle: CxButton.Style.Secondary
                onClicked: barRoot.cancelClicked()
            }
        }
    }

    // upstream CreatePresetSuccessfulDialog (@3445-3518): 450x200, 24px
    // create_success bitmap + Head_18 heading + next-step text. The OK label
    // becomes "Printer Setting" for printers (@3497); the filament
    // "Sync user presets" branch has no Qt6 sync system and stays "OK".
    component CreateSuccessPage: CxDialog {
        id: successRoot
        modal: true
        padding: 0
        property bool printerMode: true
        dialogTitle: printerMode ? qsTr("打印机创建成功") : qsTr("耗材创建成功")
        width: 450
        height: 200
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingLG
            spacing: Theme.spacingLG
            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.spacingMD
                Image {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    source: "qrc:/qml/assets/icons/create_success.svg"
                    sourceSize: Qt.size(24, 24)
                    fillMode: Image.PreserveAspectFit
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    Text {
                        text: successRoot.printerMode ? qsTr("打印机已创建") : qsTr("耗材已创建")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXL
                        font.bold: true
                    }
                    Text {
                        Layout.fillWidth: true
                        text: successRoot.printerMode
                              ? qsTr("请前往打印机设置编辑你的预设。")
                              : qsTr("如需修改，请前往耗材设置编辑你的预设。\n请注意：喷嘴温度、热床温度与最大体积速度对打印质量影响显著，请谨慎设置。")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        wrapMode: Text.WordWrap
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignRight
                spacing: Theme.spacingMD
                CxButton {
                    visible: successRoot.printerMode
                    text: qsTr("取消")
                    cxStyle: CxButton.Style.Secondary
                    onClicked: successRoot.reject()
                }
                CxButton {
                    text: successRoot.printerMode ? qsTr("打印机设置") : qsTr("确定")
                    cxStyle: CxButton.Style.Primary
                    onClicked: successRoot.accept()
                }
            }
        }
    }

    // ── content ─────────────────────────────────────────────────────────────

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Scope selector. Upstream has one dialog per entry point; the Qt6
        // entry is a single dialog with this selector (test-locked, see the
        // file header) so all three panes live here.
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: Theme.spacingMD
            spacing: Theme.spacingMD
            Text {
                text: qsTr("范围：")
                color: Theme.textMuted
                font.pixelSize: Theme.fontSizeSM
            }
            CxComboBox {
                id: scopeCombo
                Layout.preferredWidth: 160
                model: [qsTr("打印机"), qsTr("耗材"), qsTr("工艺")]
                onActivated: function(i) {
                    // UI 打印机/耗材/工艺 → PrinterCat/FilamentCat/PrintCat
                    // via scopeCategories (PSET2-02 mapping fix).
                    if (i >= 0 && i < root.scopeCategories.length)
                        root.selectedCategory = root.scopeCategories[i]
                    if (root.selectedCategory === 1) root.resetFilamentPane()
                    if (root.selectedCategory === 2) root.resetPrinterPane()
                    if (root.selectedCategory === 0) root.refreshInheritsList()
                }
            }
        }

        // ── process pane (legacy concession; no upstream counterpart) ──────
        ColumnLayout {
            visible: root.selectedCategory === 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.spacingXL
            spacing: Theme.spacingMD

            // Inherits-from selector. Upstream has no generic inherits combo
            // (the literal below satisfies the PSET-02 source lock); the
            // process scope keeps this legacy form because upstream creates
            // process presets through the Tab SavePresetDialog flow instead.
            RowLayout {
                spacing: Theme.spacingMD
                Text {
                    text: qsTr("继承自：")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSizeSM
                }
                CxComboBox {
                    id: inheritsCombo
                    implicitHeight: Theme.controlHeightSM
                    Layout.preferredWidth: 260
                    // model is set by refreshInheritsList()
                    onActivated: function(i) {
                        var names = inheritsCombo.model
                        root.selectedInherits =
                                (names && i >= 0 && i < names.length) ? names[i] : ""
                    }
                }
            }

            RowLayout {
                spacing: Theme.spacingMD
                Text {
                    text: qsTr("名称：")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSizeSM
                }
                CxTextField {
                    id: nameInput
                    Layout.preferredWidth: 260
                    implicitHeight: 26
                    font.pixelSize: Theme.fontSizeSM
                    placeholderText: qsTr("输入预设名称")
                }
            }
            Text {
                id: dupWarning
                text: qsTr("该名称的预设已存在")
                color: Theme.accentDark
                font.pixelSize: Theme.fontSizeXS
                visible: false
                Layout.leftMargin: 80
            }

            Item { Layout.fillHeight: true } // spacer

            DialogButtonsBar {
                okEnabled: nameInput.text.length > 0 && !dupWarning.visible
                onOkClicked: {
                    if (!root.configVm) { root.reject(); return }
                    const name = nameInput.text.trim()
                    // v5.16 (PSET2-02): pass the inherits selection through.
                    // ConfigViewModel::createCustomPreset(category, name,
                    // inherits) seeds the new preset from the parent's
                    // resolved chain.
                    const ok = root.configVm.createCustomPreset(
                        root.selectedCategory, name, root.selectedInherits)
                    if (ok) {
                        root.accept()
                    } else {
                        dupWarning.text = root.configVm.lastPresetError
                            ? root.configVm.lastPresetError
                            : qsTr("创建预设失败")
                        dupWarning.visible = true
                    }
                }
                onCancelClicked: root.reject()
            }
        }

        // ── filament pane (CreateFilamentPresetDialog, 600x480) ─────────────
        ColumnLayout {
            visible: root.selectedCategory === 1
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 5  // upstream 5px row gaps (@680-694)

            // "Basic Information" section header, Head_16 (@676-678)
            Text {
                Layout.leftMargin: 10
                text: qsTr("基本信息")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeXL
                font.bold: true
            }

            // Vendor row (@767-857)
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 5
                Layout.rightMargin: 5
                spacing: 5
                OptionLabel { text: qsTr("厂商") }
                PromptComboBox {
                    id: filVendorCombo
                    Layout.preferredWidth: 200  // NAME_OPTION_COMBOBOX_SIZE
                    placeholderText: qsTr("选择厂商")
                    model: root.upstreamFilamentVendors
                    visible: !root.canNotFindVendorChk
                    onActivated: function(i) { currentIndex = i }
                }
                PresetNameField {
                    id: filCustomVendor
                    Layout.preferredWidth: 200
                    placeholderText: qsTr("输入自定义厂商")
                    visible: root.canNotFindVendorChk
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    CxCheckBox {
                        // "Can't find vendor I want" swap (@820-849)
                        text: qsTr("找不到想要的厂商")
                        font.pixelSize: Theme.fontSizeMD
                        checked: root.canNotFindVendorChk
                        onToggled: root.canNotFindVendorChk = checked
                    }
                }
            }

            // Type row (@859-903)
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 5
                Layout.rightMargin: 5
                spacing: 5
                OptionLabel { text: qsTr("类型") }
                PromptComboBox {
                    id: filTypeCombo
                    Layout.preferredWidth: 200
                    placeholderText: qsTr("选择类型")
                    onActivated: function(i) {
                        currentIndex = i
                        // upstream type-selection rebuild (@883-900)
                        filPresetCombo.model = root.filamentPresetChoices(currentText)
                        filPresetCombo.currentIndex = -1
                        root.rebuildPrinterGrid()
                    }
                }
            }

            // Serial row (@905-935)
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 5
                Layout.rightMargin: 5
                spacing: 5
                OptionLabel { text: qsTr("系列") }
                ColumnLayout {
                    spacing: 5
                    PresetNameField {
                        id: filSerialInput
                        Layout.preferredWidth: 200
                    }
                    // "e.g. Basic, Matte, Silk, Marble" hint, Body_12 grey (@928-930)
                    Text {
                        text: qsTr("例如：基础、哑光、丝绸、大理石")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSizeSM
                    }
                }
            }

            // divider line (@684-688)
            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                Layout.preferredHeight: 1
                color: Theme.borderSubtle
            }

            // "Add Filament Preset under this filament" header (@690-692)
            Text {
                Layout.leftMargin: 15
                text: qsTr("在此耗材下添加耗材预设")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeXL
                font.bold: true
            }

            // Filament Preset row: RadioBox pair + base preset combo
            // (@937-1026)
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 5
                spacing: 5
                OptionLabel { text: qsTr("耗材预设") }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10  // upstream 10px between the radio pair (@1018)
                    UpstreamRadio {
                        label: qsTr("基于当前耗材创建")
                        checked: root.filamentFromCurrent
                        onPicked: root.filamentFromCurrent = true
                    }
                    PromptComboBox {
                        id: filPresetCombo
                        Layout.preferredWidth: 300  // FILAMENT_PRESET_COMBOBOX_SIZE
                        placeholderText: qsTr("选择耗材预设")
                        visible: root.filamentFromCurrent
                        onActivated: function(i) {
                            currentIndex = i
                            root.rebuildPrinterGrid()
                        }
                    }
                    UpstreamRadio {
                        label: qsTr("复制当前耗材预设")
                        checked: !root.filamentFromCurrent
                        onPicked: root.filamentFromCurrent = false
                    }
                }
            }

            // Per-mode guidance text (@696 / @1311-1312)
            Text {
                Layout.leftMargin: 15
                Layout.rightMargin: 15
                text: root.filamentFromCurrent
                      ? qsTr("我们可以为您的以下打印机创建耗材预设：")
                      : qsTr("我们将把预设重命名为 \"厂商 类型 系列 @所选打印机\"。\n要为更多打印机添加预设，请前往打印机选择")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.WordWrap
            }

            // 3-column printer CheckBox grid in a max-350 scrolled panel
            // (@699-707, wxGridSizer(3, 5, 5) @1034)
            CxScrollView {
                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 10
                Layout.maximumHeight: 350
                Layout.fillHeight: true
                Rectangle {
                    // implicit sizes drive the ScrollView content size
                    implicitWidth: filGridViewport.implicitWidth + 10
                    implicitHeight: filGridViewport.implicitHeight + 10
                    color: Theme.bgSurface  // upstream PRINTER_LIST_COLOUR
                    ColumnLayout {
                        id: filGridViewport
                        anchors.centerIn: parent
                        width: parent.width - 10
                        Grid {
                            id: filGrid
                            columns: 3
                            columnSpacing: 5
                            rowSpacing: 5
                            Repeater {
                                model: ListModel {
                                    id: printerGridModel
                                }
                                delegate: CxCheckBox {
                                    id: printerGridCell
                                    required property string name
                                    required property bool picked
                                    required property int index
                                    text: name
                                    font.pixelSize: Theme.fontSizeMD
                                    checked: picked
                                    onToggled: printerGridModel.setProperty(printerGridCell.index, "picked", checked)
                                }
                            }
                        }
                    }
                }
            }

            // upstream DialogButtons {"OK"→"Create", "Cancel"} (@708, @1043-1046)
            DialogButtonsBar {
                onOkClicked: root.createFilamentClicked()
                onCancelClicked: root.reject()
            }
        }

        // ── printer pane (CreatePrinterPresetDialog two-step wizard) ────────
        ColumnLayout {
            visible: root.selectedCategory === 2
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // step switch bar (upstream create_step_switch_item @1637-1672):
            // step_1 / step_2_ready bitmaps @20px + 50px divider (@1653)
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                Row {
                    anchors.centerIn: parent
                    spacing: 3
                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        source: "qrc:/qml/assets/icons/step_1.svg"
                        sourceSize: Qt.size(20, 20)
                        fillMode: Image.PreserveAspectFit
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("创建打印机")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                    }
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 50
                        height: 1
                        color: Theme.bgSurface
                    }
                    Image {
                        anchors.verticalCenter: parent.verticalCenter
                        source: "qrc:/qml/assets/icons/step_2_ready.svg"
                        sourceSize: Qt.size(20, 20)
                        fillMode: Image.PreserveAspectFit
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: qsTr("导入预设")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                    }
                }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.borderSubtle
            }

            // Page1 / Page2 share a scrolled viewport (upstream m_page1 is a
            // wxScrolledWindow @1594)
            CxScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Flickable {
                    id: printerFlick
                    contentWidth: width
                    contentHeight: printerPages.implicitHeight
                    ColumnLayout {
                        id: printerPages
                        width: printerFlick.width
                        spacing: 5

                        // ── Page1 ──
                        ColumnLayout {
                            visible: root.printerPage === 1
                            Layout.fillWidth: true
                            spacing: 5

                            // Create Type row (@1702-1719)
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.margins: 10
                                spacing: 10
                                OptionLabel { text: qsTr("创建类型") }
                                ColumnLayout {
                                    spacing: 10
                                    UpstreamRadio {
                                        label: qsTr("创建打印机")
                                        checked: !root.createNozzle
                                        onPicked: {
                                            root.createNozzle = false
                                            root.canNotFindPrinter = false
                                        }
                                    }
                                    UpstreamRadio {
                                        label: qsTr("为现有打印机创建喷嘴")
                                        checked: root.createNozzle
                                        onPicked: root.createNozzle = true
                                    }
                                }
                            }

                            // Printer row (@1721-1861)
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: 10
                                Layout.rightMargin: 10
                                Layout.bottomMargin: 5
                                spacing: 10
                                OptionLabel { text: qsTr("打印机") }
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    RowLayout {
                                        spacing: 5
                                        PromptComboBox {
                                            id: p1VendorCombo
                                            Layout.preferredWidth: 200
                                            placeholderText: qsTr("选择厂商")
                                            visible: !root.createNozzle && !root.canNotFindPrinter
                                            onActivated: function(i) {
                                                currentIndex = i
                                                // upstream vendor selection
                                                // fills the model combo (@1742-1767)
                                                p1ModelCombo.model =
                                                    root.configVm
                                                    ? root.configVm.wizardPrinterModelsForVendor(currentText)
                                                    : []
                                                p1ModelCombo.currentIndex =
                                                    p1ModelCombo.count > 0 ? 0 : -1
                                                p1PrinterCombo.currentIndex = -1
                                            }
                                        }
                                        PromptComboBox {
                                            id: p1ModelCombo
                                            Layout.preferredWidth: 200
                                            placeholderText: qsTr("选择型号")
                                            visible: !root.createNozzle && !root.canNotFindPrinter
                                            onActivated: function(i) { currentIndex = i }
                                        }
                                        PromptComboBox {
                                            // existing-printer combo for the
                                            // nozzle path (@1780-1789, hidden
                                            // until create_nozzle)
                                            id: p1PrinterCombo
                                            Layout.preferredWidth: 280
                                            placeholderText: qsTr("选择打印机")
                                            visible: root.createNozzle
                                            onActivated: function(i) { currentIndex = i }
                                        }
                                        PresetNameField {
                                            id: p1CustomVendor
                                            Layout.preferredWidth: 200
                                            placeholderText: qsTr("输入自定义厂商")
                                            visible: root.canNotFindPrinter
                                        }
                                        PresetNameField {
                                            id: p1CustomModel
                                            Layout.preferredWidth: 200
                                            placeholderText: qsTr("输入自定义型号")
                                            visible: root.canNotFindPrinter
                                        }
                                    }
                                    CxCheckBox {
                                        // "Can't find my printer model" swap (@1818-1853)
                                        text: qsTr("找不到我的打印机型号")
                                        font.pixelSize: Theme.fontSizeMD
                                        visible: !root.createNozzle
                                        checked: root.canNotFindPrinter
                                        onToggled: root.canNotFindPrinter = checked
                                    }
                                }
                            }

                            // Nozzle Diameter row (@1863-1939)
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: 10
                                Layout.rightMargin: 10
                                Layout.bottomMargin: 5
                                spacing: 10
                                OptionLabel { text: qsTr("喷嘴直径") }
                                ColumnLayout {
                                    spacing: 5
                                    RowLayout {
                                        spacing: 5
                                        PromptComboBox {
                                            id: p1NozzleCombo
                                            Layout.preferredWidth: 100  // OPTION_SIZE
                                            visible: !root.canNotFindNozzle
                                            onActivated: function(i) { currentIndex = i }
                                        }
                                        PresetNameField {
                                            id: p1CustomNozzle
                                            Layout.preferredWidth: 200
                                            placeholderText: qsTr("输入自定义喷嘴直径")
                                            visible: root.canNotFindNozzle
                                            // upstream allows ',' and '.' here
                                            // (@1890-1897)
                                            validator: RegularExpressionValidator {
                                                regularExpression:
                                                    /^[^!#$%&()\/:<>?@\\^_|~\t\n\r]*$/
                                            }
                                        }
                                    }
                                    CxCheckBox {
                                        text: qsTr("找不到我的喷嘴直径")
                                        font.pixelSize: Theme.fontSizeMD
                                        checked: root.canNotFindNozzle
                                        onToggled: root.canNotFindNozzle = checked
                                    }
                                }
                            }

                            // printer info panel (@1683-1693), hidden on the
                            // nozzle path (@2553)
                            ColumnLayout {
                                visible: !root.createNozzle
                                Layout.fillWidth: true
                                spacing: 5

                                // Bed Shape row (@1942-1958): static "Rectangle"
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.margins: 10
                                    spacing: 10
                                    OptionLabel { text: qsTr("热床形状") }
                                    Text {
                                        text: qsTr("矩形")
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeMD
                                    }
                                }

                                // Printable Space row (@1960-1991)
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 10
                                    Layout.rightMargin: 10
                                    Layout.bottomMargin: 5
                                    spacing: 10
                                    OptionLabel { text: qsTr("可打印空间") }
                                    DigitsField { id: p1SpaceX; Layout.leftMargin: 5 }
                                    DigitsField { id: p1SpaceY }
                                }

                                // Origin row (@1993-2023)
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 10
                                    Layout.rightMargin: 10
                                    Layout.bottomMargin: 5
                                    spacing: 10
                                    OptionLabel { text: qsTr("原点") }
                                    DigitsField { id: p1OriginX; Layout.leftMargin: 5 }
                                    DigitsField { id: p1OriginY }
                                }

                                // Hot Bed STL row (@2025-2049)
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 10
                                    Layout.rightMargin: 10
                                    Layout.bottomMargin: 5
                                    spacing: 10
                                    OptionLabel { text: qsTr("热床 STL") }
                                    CxButton {
                                        text: qsTr("加载...")
                                        compact: true
                                        cxStyle: CxButton.Style.Secondary
                                        onClicked: bedStlDialog.open()
                                    }
                                    Text {
                                        Layout.maximumWidth: 200  // upstream Ellipsize 200 (@2168)
                                        text: root.bedStlPath.length > 0
                                              ? root.bedStlPath.split("/").pop()
                                              : qsTr("空")
                                        color: Theme.textSecondary
                                        font.pixelSize: Theme.fontSizeMD
                                        elide: Text.ElideRight
                                    }
                                }

                                // Hot Bed SVG row (@2051-2075)
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.leftMargin: 10
                                    Layout.rightMargin: 10
                                    Layout.bottomMargin: 5
                                    spacing: 10
                                    OptionLabel { text: qsTr("热床 SVG") }
                                    CxButton {
                                        text: qsTr("加载...")
                                        compact: true
                                        cxStyle: CxButton.Style.Secondary
                                        onClicked: bedSvgDialog.open()
                                    }
                                    Text {
                                        Layout.maximumWidth: 200
                                        text: root.bedSvgPath.length > 0
                                              ? root.bedSvgPath.split("/").pop()
                                              : qsTr("空")
                                        color: Theme.textSecondary
                                        font.pixelSize: Theme.fontSizeMD
                                        elide: Text.ElideRight
                                    }
                                }

                                // Max Print Height row (@2077-2095)
                                RowLayout {
                                    Layout.fillWidth: true
                                    Layout.margins: 10
                                    spacing: 10
                                    OptionLabel { text: qsTr("最大打印高度") }
                                    DigitsField { id: p1Height; Layout.leftMargin: 5 }
                                }
                            }
                        }

                        // ── Page2 ──
                        ColumnLayout {
                            visible: root.printerPage === 2
                            Layout.fillWidth: true
                            spacing: 5

                            // Printer Preset row (@2584-2629)
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.margins: 10
                                spacing: 10
                                OptionLabel { text: qsTr("打印机预设") }
                                ColumnLayout {
                                    spacing: 5
                                    // upstream combobox_title carries the
                                    // base_curr_printer string (@2595)
                                    Text {
                                        text: qsTr("基于当前打印机创建")
                                        color: Theme.textPrimary
                                        font.pixelSize: Theme.fontSizeMD
                                    }
                                    RowLayout {
                                        spacing: 10
                                        PromptComboBox {
                                            id: p2VendorCombo
                                            Layout.preferredWidth: 150  // PRINTER_PRESET_VENDOR_SIZE
                                            placeholderText: qsTr("选择厂商")
                                            onActivated: function(i) {
                                                currentIndex = i
                                                p2ModelCombo.model =
                                                    root.configVm
                                                    ? root.configVm.wizardPrinterModelsForVendor(currentText)
                                                    : []
                                                p2ModelCombo.currentIndex = -1
                                                root.rebuildTemplatePanel()
                                            }
                                        }
                                        PromptComboBox {
                                            id: p2ModelCombo
                                            Layout.preferredWidth: 280  // PRINTER_PRESET_MODEL_SIZE
                                            placeholderText: qsTr("选择型号")
                                            onActivated: function(i) {
                                                currentIndex = i
                                                root.rebuildTemplatePanel()
                                            }
                                        }
                                    }
                                }
                            }

                            // Presets row: RadioBox pair (@2631-2648)
                            RowLayout {
                                Layout.fillWidth: true
                                Layout.leftMargin: 10
                                Layout.rightMargin: 10
                                Layout.bottomMargin: 5
                                spacing: 10
                                OptionLabel { text: qsTr("预设") }
                                ColumnLayout {
                                    spacing: 10
                                    UpstreamRadio {
                                        label: qsTr("从模板创建")
                                        checked: root.fromTemplate
                                        onPicked: {
                                            root.fromTemplate = true
                                            root.rebuildTemplatePanel()
                                        }
                                    }
                                    UpstreamRadio {
                                        label: qsTr("基于当前打印机创建")
                                        checked: !root.fromTemplate
                                        onPicked: {
                                            root.fromTemplate = false
                                            root.rebuildTemplatePanel()
                                        }
                                    }
                                }
                            }

                            // template panel: min width 660 (@2665), two
                            // 3-column CheckBox grids with Select All /
                            // Deselect All strips (@2667-2726)
                            CxScrollView {
                                Layout.fillWidth: true
                                Layout.leftMargin: 10
                                Layout.rightMargin: 10
                                Layout.fillHeight: true
                                Rectangle {
                                    // implicit sizes drive the ScrollView content
                                    // size; min width 660 (upstream @2665)
                                    implicitWidth: Math.max(660, tplViewport.implicitWidth + 10)
                                    implicitHeight: tplViewport.implicitHeight + 10
                                    color: Theme.bgSurface  // upstream PRINTER_LIST_COLOUR
                                    ColumnLayout {
                                        id: tplViewport
                                        anchors.centerIn: parent
                                        width: parent.width - 10
                                        spacing: 5

                                        Text {
                                            Layout.margins: 5
                                            text: qsTr("耗材预设模板")
                                            color: Theme.textPrimary
                                            font.pixelSize: Theme.fontSizeMD
                                        }
                                        Grid {
                                            Layout.fillWidth: true
                                            columns: 3
                                            columnSpacing: 5
                                            rowSpacing: 5
                                            Repeater {
                                                model: ListModel { id: filTplModel }
                                                delegate: CxCheckBox {
                                                    id: filTplCell
                                                    required property string name
                                                    required property bool picked
                                                    required property int index
                                                    text: name
                                                    font.pixelSize: Theme.fontSizeMD
                                                    checked: picked
                                                    onToggled: filTplModel.setProperty(filTplCell.index, "picked", checked)
                                                }
                                            }
                                        }
                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 24
                                            color: Theme.bgPanel  // upstream FILAMENT_OPTION_COLOUR strip
                                            RowLayout {
                                                anchors.centerIn: parent
                                                spacing: Theme.spacingLG
                                                Text {
                                                    text: qsTr("全选")
                                                    color: Theme.accent  // upstream SELECT_ALL_OPTION_COLOUR → OWzx accent
                                                    font.pixelSize: Theme.fontSizeMD
                                                    MouseArea {
                                                        anchors.fill: parent
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: {
                                                            for (var i = 0; i < filTplModel.count; ++i)
                                                                filTplModel.setProperty(i, "picked", true)
                                                        }
                                                    }
                                                }
                                                Text {
                                                    text: qsTr("取消全选")
                                                    color: Theme.accent
                                                    font.pixelSize: Theme.fontSizeMD
                                                    MouseArea {
                                                        anchors.fill: parent
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: {
                                                            for (var i = 0; i < filTplModel.count; ++i)
                                                                filTplModel.setProperty(i, "picked", false)
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        Rectangle {
                                            // upstream white split panel (@2695-2697)
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 10
                                            color: Theme.bgElevated
                                        }

                                        Text {
                                            Layout.margins: 5
                                            text: qsTr("工艺预设模板")
                                            color: Theme.textPrimary
                                            font.pixelSize: Theme.fontSizeMD
                                        }
                                        Grid {
                                            Layout.fillWidth: true
                                            columns: 3
                                            columnSpacing: 5
                                            rowSpacing: 5
                                            Repeater {
                                                model: ListModel { id: procTplModel }
                                                delegate: CxCheckBox {
                                                    id: procTplCell
                                                    required property string name
                                                    required property bool picked
                                                    required property int index
                                                    text: name
                                                    font.pixelSize: Theme.fontSizeMD
                                                    checked: picked
                                                    onToggled: procTplModel.setProperty(procTplCell.index, "picked", checked)
                                                }
                                            }
                                        }
                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: 24
                                            color: Theme.bgPanel
                                            RowLayout {
                                                anchors.centerIn: parent
                                                spacing: Theme.spacingLG
                                                Text {
                                                    text: qsTr("全选")
                                                    color: Theme.accent
                                                    font.pixelSize: Theme.fontSizeMD
                                                    MouseArea {
                                                        anchors.fill: parent
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: {
                                                            for (var i = 0; i < procTplModel.count; ++i)
                                                                procTplModel.setProperty(i, "picked", true)
                                                        }
                                                    }
                                                }
                                                Text {
                                                    text: qsTr("取消全选")
                                                    color: Theme.accent
                                                    font.pixelSize: Theme.fontSizeMD
                                                    MouseArea {
                                                        anchors.fill: parent
                                                        cursorShape: Qt.PointingHandCursor
                                                        onClicked: {
                                                            for (var i = 0; i < procTplModel.count; ++i)
                                                                procTplModel.setProperty(i, "picked", false)
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
                }
            }

            // Page1 buttons {"OK"→next, "Cancel"} (@2097-2110)
            DialogButtonsBar {
                visible: root.printerPage === 1
                okText: qsTr("下一步")
                onOkClicked: {
                    if (root.printerValidatePage1()) {
                        root.printerPage = 2
                        root.rebuildTemplatePanel()
                    }
                }
                onCancelClicked: root.reject()
            }
            // Page2 buttons {"Return", "OK"→"Create", "Cancel"}, Return
            // left-aligned (@2736-2744)
            DialogButtonsBar {
                visible: root.printerPage === 2
                showReturn: true
                onReturnClicked: root.printerPage = 1
                onOkClicked: root.printerCreateClicked()
                onCancelClicked: root.reject()
            }
        }
    }

    // ── popups ──────────────────────────────────────────────────────────────

    // Shared MessageDialog replica (upstream MessageDialog info / confirm
    // prompts throughout CreatePresetsDialog.cpp).
    ConfirmDialog {
        id: msgBox
        destructive: false
        dialogTitle: qsTr("提示")
        width: 480
        height: 220
    }

    // Hot Bed STL picker (upstream load_model_stl, wxFileDialog FT_STL)
    FileDialog {
        id: bedStlDialog
        nameFilters: [qsTr("STL 文件 (*.stl)"), qsTr("所有文件 (*)")]
        onAccepted: {
            root.bedStlPath = selectedFile.toString().replace(/^file:\/\/\//, "")
        }
    }

    // Hot Bed SVG picker (upstream load_texture, wxFileDialog FT_TEX)
    FileDialog {
        id: bedSvgDialog
        nameFilters: [qsTr("图片文件 (*.png *.svg)"), qsTr("所有文件 (*)")]
        onAccepted: {
            root.bedSvgPath = selectedFile.toString().replace(/^file:\/\/\//, "")
        }
    }

    CreateSuccessPage {
        id: successDialog
        onAccepted: root.close()
        onRejected: root.close()
    }

    // ── process-pane helper (kept from the legacy port) ─────────────────────
    // Selected parent preset name ("" = no inheritance).
    property string selectedInherits: ""

    function refreshInheritsList() {
        if (!root.configVm) return
        // Pull the existing-scope preset list. ConfigViewModel exposes
        // per-scope QStringList Q_PROPERTYs (printerPresetNames /
        // filamentPresetNames / printPresetNames) — use the one matching the
        // selected category so the "inherits from" dropdown only shows
        // relevant presets.
        var names = []
        if (root.selectedCategory === 2 && root.configVm.printerPresetNames)
            names = root.configVm.printerPresetNames
        else if (root.selectedCategory === 1 && root.configVm.filamentPresetNames)
            names = root.configVm.filamentPresetNames
        else if (root.selectedCategory === 0 && root.configVm.printPresetNames)
            names = root.configVm.printPresetNames
        inheritsCombo.model = (names && names.length > 0) ? names : []
        inheritsCombo.currentIndex = 0
        root.selectedInherits = (names && names.length > 0) ? names[0] : ""
        dupWarning.visible = false
        dupWarning.text = ""
    }
}
