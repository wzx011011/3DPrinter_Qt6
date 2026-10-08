import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.4 -- AMS Settings Dialog (aligns with upstream AMSSetting / AMSMaterialsSetting / AmsMappingPopup)
//
// U13 source-truth alignment (structure truth: AMSSetting.cpp, AMSMaterialsSetting.cpp,
// AmsMappingPopup.cpp; visual truth: restoration-map §2 -- no dedicated ref image):
//   - Five upstream setting groups (insert-material / startup-read / remaining-capacity /
//     filament-backup / air-print-detection): Head_13 bold titles + Body_13 conditional
//     tips (AMSSetting.cpp:58-231), tip visibility following update_insert_material_read_mode /
//     update_starting_read_mode / update_remain_mode / update_switch_filament /
//     update_air_printing_detection (:297-302, :345-497).
//   - AMS Type row: label + readonly combo width 240 + switching-tip placeholder
//     (m_type_combobox, AMSSetting.cpp:605-606). Capability-gated; hidden without a device.
//   - Product image panel at the list tail: 126px icon, top offset 26 (:243-244). The
//     upstream "ams_icon" bitmap is missing from src/qml_gui/assets/ -- a QML placeholder
//     (device outline + slot color chips) is rendered instead (asset gap registered for the
//     brand-asset batch).
//   - Single-slot editor aligned with the AMSMaterialsSetting field set: 250x30 preset
//     combo (hpp:28), 25x25 editable color picker (ColorPicker 25x25, cpp:1468-1470),
//     90x24 max/min nozzle-temperature pair with the 120..300 validation warning
//     (FILAMENT_MIN_TEMP/FILAMENT_MAX_TEMP, cpp:24-25/:235-243), readonly SN row
//     (hpp:28/:30, cpp:251-261) and right-aligned Confirm/Reset/Close with 20/24/16
//     button metrics (cpp:63-84). The per-card readonly color chips are kept and coexist
//     with the editor picker (card chips remain read-only indicators).
//   - Mapping section scoped to the slot-mapping rows themselves: the invented
//     "(温度覆盖 210°C)" annotation and the "添加映射" button are removed; the full
//     AmsMappingPopup mapping canvas is deferred (feature-level).
//   - Non-modal Show semantics (StatusPanel.cpp:4455; AMSSetting has no buttons at all):
//     the previous footer block is removed, Esc closes (Popup.CloseOnEscape).
//   - Surfaces: slot cards / mapping container use Theme.bgInset (was scrollBarTrackColor /
//     bgPanel); mapping rows drop the invented zebra striping for body-color rows with a
//     1px borderSubtle outline. Dialog main surface stays Theme.bgElevated (#4b4b4d) --
//     R1 surface-color migration is deferred to the global batch.
//
// Capability bits: upstream reads MachineObject flags (is_support_update_remain,
// is_support_filament_backup, is_support_air_print_detection, firmware switch).
// AmsMaterialsViewModel has no capability channel yet (local mock, no device), so the
// bits live here with mock-device defaults (4-slot AMS story; no type switch / air-print);
// a VM batch can replace them with bindings. The setting toggles themselves are
// session-local: upstream sends device commands (command_ams_user_settings etc.) that the
// mock stack cannot carry -- persistence channel deferred with the VM batch.
//
// Phase 201 (v5.6 AMS Architecture Cleanup): slot data comes from AmsMaterialsViewModel
// (backend.amsMaterialsViewModel) via the set* / removeMappingRule / resetToDefaults APIs,
// persisted to local QSettings (ams/materials/*). No network / device / cloud access.

CxDialog {
    id: root

    // 差距6: upstream opens AMSSetting with non-modal Show() and no buttons
    // (StatusPanel.cpp:4455); Esc is the only close affordance besides the ✕.
    closePolicy: Popup.CloseOnEscape
    modal: false

    dialogTitle: qsTr("AMS 设置")

    anchors.centerIn: parent
    width: 500
    // 差距3: content-adaptive height (the old fixed 440 overflows once the five
    // upstream setting groups are present). Capped so the scroll area engages
    // before the dialog outgrows the 721px-tall window. The +92 fudge covers the
    // 44px CxDialog header plus the Basic-style padding / content margins.
    implicitHeight: Math.min(contentCol.implicitHeight + 2 * Theme.spacingLG, 620) + 92

    // ViewModel binding (Phase 201). Set by main.qml. When null the dialog
    // renders with empty state safely.
    property var amsVm: null

    // ── Capability bits (upstream MachineObject flags; mock defaults) ──────
    // AMS Type firmware switching needs a live device -> hidden (AMSSetting
    // constructs m_ams_type with Show(false), :41).
    property bool capAmsTypeSwitch: false
    property bool capInsertMaterial: true
    property bool capStartingRead: true
    // update_remain_mode gate: m_sizer_remain_block->Show(obj->is_support_update_remain)
    property bool capUpdateRemain: true
    // update_switch_filament gate: obj->is_support_filament_backup
    property bool capFilamentBackup: true
    // update_air_printing_detection gate; hidden by default upstream too (:234-236)
    property bool capAirPrintDetection: false
    // AMS Type combo entries come from the device firmware list (AMSSetting.cpp:659-672);
    // no device/mock channel yet -> empty model, row hidden via capAmsTypeSwitch.
    property var amsTypeOptions: []

    // ── Session toggle state (upstream persists via device commands; mock: local) ──
    property bool insertMaterialRead: true
    property bool startingRead: true
    property bool updateRemain: true
    property bool filamentBackup: true
    property bool airPrintDetect: false

    // ── Single-slot editor staged state (AMSMaterialsSetting::Popup semantics) ──
    property int editSlot: -1
    property string editMaterial: ""
    property color editColor: "transparent"
    property string editTempMax: ""
    property string editTempMin: ""
    // Session map slot -> {max, min}; the nozzle-temp range has no VM channel yet
    // (upstream reads it from the filament preset), so committed values live here.
    property var slotTempRanges: ({})

    // FILAMENT_MIN_TEMP(120) / FILAMENT_MAX_TEMP(300), AMSMaterialsSetting.cpp:24-25
    readonly property int tempMinLimit: 120
    readonly property int tempMaxLimit: 300

    readonly property bool editTempOutOfRange: {
        function bad(s) {
            if (s === "") return false
            var v = parseInt(s)
            return isNaN(v) || v < root.tempMinLimit || v > root.tempMaxLimit
        }
        return bad(editTempMax) || bad(editTempMin)
    }

    // SN row: upstream shows the spool serial (cpp:259-261). The mock VM has no
    // SN channel; when a VM exposes slotSn() it lights up automatically, otherwise
    // an em-dash placeholder is shown (gap registered with the VM batch).
    readonly property string editSn: {
        if (editSlot < 0) return ""
        if (amsVm && amsVm.slotSn) {
            var sn = amsVm.slotSn(editSlot)
            if (sn !== undefined && sn !== null && sn !== "") return String(sn)
        }
        return "--"
    }

    function openSlotEditor(i) {
        editSlot = i
        editMaterial = _slotMaterials[i] || ""
        editColor = _slotColors[i] || "transparent"
        var t = slotTempRanges[i] || {}
        editTempMax = t.max || ""
        editTempMin = t.min || ""
    }

    // Upstream on_select_reset clears the fields after a confirmation dialog
    // (AMSMaterialsSetting.cpp:462-479). Our editor is staged over VM/session
    // state, so the honest equivalent is reverting the staged values (the
    // destructive clear would strand the readonly card chips).
    function resetSlotEdit() {
        if (editSlot < 0) return
        editMaterial = _slotMaterials[editSlot] || ""
        editColor = _slotColors[editSlot] || "transparent"
        editTempMax = ""
        editTempMin = ""
    }

    function confirmSlotEdit() {
        if (editSlot < 0) return
        if (amsVm) {
            amsVm.setSlotMaterial(editSlot, editMaterial)
            // Editable picker contract: applied through the VM color channel
            // once the VM batch provides setSlotColor (guarded forward call).
            if (amsVm.setSlotColor)
                amsVm.setSlotColor(editSlot, editColor.toString())
        }
        // Nozzle temps: session-scoped commit (VM/preset channel deferred).
        var t = {}
        t.max = editTempMax
        t.min = editTempMin
        slotTempRanges[editSlot] = t
        editSlot = -1
    }

    // Convenience read-only views over the viewmodel. Empty fallbacks keep the
    // dialog robust before the viewmodel is wired.
    readonly property var _slotColors: amsVm ? amsVm.slotColors : []
    readonly property var _slotNames: amsVm ? amsVm.slotNames : []
    readonly property var _slotMaterials: amsVm ? amsVm.slotMaterials : []
    readonly property var _slotAutoSwap: amsVm ? amsVm.slotAutoSwap : []
    readonly property var _remainingPct: amsVm ? amsVm.remainingPct : []
    readonly property var _materialTypes: amsVm ? amsVm.materialTypes : []
    readonly property var _mappingRules: amsVm ? amsVm.mappingRules : []
    readonly property int _slotCount: amsVm ? amsVm.slotCount : 4

    // Upstream setting group: ::CheckBox + Head_13 bold title (12px gap) and
    // Body_13 secondary tips whose visibility follows the checkbox state
    // (AMSSetting.cpp:50-232).
    component AmsSettingGroup: ColumnLayout {
        id: grp

        signal groupToggled()
        property alias boxChecked: box.checked
        property string title: ""
        // Capability gate (update_*_mode Show/Hide); true = always visible.
        property bool capability: true
        property string tipWhenChecked: ""
        property string tipWhenUnchecked: ""
        property string tipAlways: ""

        visible: capability
        Layout.fillWidth: true
        spacing: Theme.spacingXS

        Row {
            spacing: Theme.spacingLG // upstream checkbox->title wxLEFT 12 (:56)

            CxCheckBox {
                id: box
                anchors.verticalCenter: parent.verticalCenter
                font.pixelSize: Theme.fontSize13 // Body_13 checkbox text (差距8)
                onToggled: grp.groupToggled()
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: grp.title
                color: Theme.textPrimary // AMS_SETTING_GREY800 title tone
                font.pixelSize: Theme.fontSize13 // Head_13 (差距8, 字面值)
                font.bold: true
            }
        }

        // Checked-state tip (hidden while tipAlways carries the static tip).
        Text {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingLG // upstream tip indent 10 (:68)
            visible: grp.tipAlways === "" && grp.boxChecked && grp.tipWhenChecked !== ""
            text: grp.tipWhenChecked
            color: Theme.textSecondary // AMS_SETTING_GREY700 tip tone
            font.pixelSize: Theme.fontSize13 // Body_13
            wrapMode: Text.Wrap
        }

        Text {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingLG
            visible: grp.tipAlways === "" && !grp.boxChecked && grp.tipWhenUnchecked !== ""
            text: grp.tipWhenUnchecked
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSize13
            wrapMode: Text.Wrap
        }

        // Static tip (remain / switch-filament / air-print groups).
        Text {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingLG
            visible: grp.tipAlways !== ""
            text: grp.tipAlways
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSize13
            wrapMode: Text.Wrap
        }
    }

    contentItem: ScrollView {
        id: scrollView
        anchors.fill: parent
        anchors.margins: Theme.spacingLG
        clip: true
        contentWidth: availableWidth
        ScrollBar.vertical.policy: ScrollBar.AsNeeded

        ColumnLayout {
            id: contentCol
            width: scrollView.availableWidth
            spacing: Theme.spacingXL

            // ── 差距2: AMS Type row (AMSSetting.cpp:598-625) ────────────────
            // Whole row hidden without the firmware-switch capability (the
            // upstream panel is constructed Show(false) and only revealed for
            // devices supporting the switch).
            RowLayout {
                visible: root.capAmsTypeSwitch
                Layout.fillWidth: true
                spacing: Theme.spacingMD

                Text {
                    text: qsTr("AMS 类型")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSize13
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                CxComboBox {
                    Layout.preferredWidth: 240 // m_type_combobox 240 (:605-606)
                    model: root.amsTypeOptions
                    enabled: model.length > 0
                }

                // m_switching_tips placeholder (Body_14, :609-611): shown only
                // while the device reports a firmware switch; no channel yet.
                Text {
                    visible: false
                    text: qsTr("切换中")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                }
            }

            // ── 差距1: five upstream setting groups (AMSSetting.cpp:46-236) ──
            // 1) Insertion update (m_panel_Insert_material)
            AmsSettingGroup {
                title: qsTr("插入耗材时更新")
                boxChecked: root.insertMaterialRead
                capability: root.capInsertMaterial
                tipWhenChecked: qsTr("插入新耗材盘时，AMS 将自动读取耗材信息，该过程大约需要 20 秒。") + "\n" +
                                qsTr("注意：若在打印过程中插入新耗材，AMS 在打印完成前不会自动读取其信息。")
                tipWhenUnchecked: qsTr("插入新耗材时，AMS 不会自动读取其信息，留空供您手动填写。")
                onGroupToggled: root.insertMaterialRead = !root.insertMaterialRead
            }

            // 2) Update on startup (m_sizer_starting)
            AmsSettingGroup {
                title: qsTr("启动时更新")
                boxChecked: root.startingRead
                capability: root.capStartingRead
                tipWhenChecked: qsTr("启动时 AMS 将自动读取已插入耗材的信息，大约需要 1 分钟，读取过程会转动耗材盘。")
                tipWhenUnchecked: qsTr("启动时 AMS 不会自动读取耗材信息，将继续使用上次关机前记录的信息。")
                onGroupToggled: root.startingRead = !root.startingRead
            }

            // 3) Update remaining capacity (m_sizer_remain_block, capability-gated)
            AmsSettingGroup {
                title: qsTr("更新剩余容量")
                boxChecked: root.updateRemain
                capability: root.capUpdateRemain
                tipAlways: qsTr("AMS 将尝试估算耗材的剩余容量。")
                onGroupToggled: root.updateRemain = !root.updateRemain
            }

            // 4) AMS filament backup (m_sizer_switch_filament, capability-gated)
            AmsSettingGroup {
                title: qsTr("AMS 耗材自动续接")
                boxChecked: root.filamentBackup
                capability: root.capFilamentBackup
                tipAlways: qsTr("当前耗材耗尽时，AMS 将自动续接到耗材属性匹配的另一盘耗材。")
                onGroupToggled: root.filamentBackup = !root.filamentBackup
            }

            // 5) Air printing detection (m_sizer_air_print, capability-gated,
            //    hidden by default upstream as well :234-236)
            AmsSettingGroup {
                title: qsTr("空打检测")
                boxChecked: root.airPrintDetect
                capability: root.capAirPrintDetection
                tipAlways: qsTr("检测到堵料或耗材磨损断裂时立即暂停打印，以节省时间和耗材。")
                onGroupToggled: root.airPrintDetect = !root.airPrintDetect
            }

            // ── 差距4: single-slot editor (AMSMaterialsSetting field set) ────
            Rectangle {
                visible: root.editSlot >= 0
                Layout.fillWidth: true
                implicitHeight: editCol.implicitHeight + 40 // 2 x 20px row margins
                radius: Theme.radiusMD
                color: Theme.bgInset
                border.color: Theme.borderSubtle
                border.width: 1

                ColumnLayout {
                    id: editCol
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 20 // upstream row margins FromDIP(20) (:278-287)

                    Text {
                        text: qsTr("编辑槽位 %1").arg(root.editSlot + 1)
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSize13
                        font.bold: true
                    }

                    // Filament row: 80px label + 250x30 readonly preset combo
                    // (AMS_MATERIALS_SETTING_LABEL_WIDTH/COMBOX_WIDTH, hpp:27-28)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingLG

                        Text {
                            Layout.preferredWidth: 80
                            text: qsTr("耗材")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSize13
                        }

                        CxComboBox {
                            id: editMaterialCombo
                            Layout.preferredWidth: 250
                            Layout.preferredHeight: 30
                            font.pixelSize: Theme.fontSize13
                            model: root._materialTypes
                            currentIndex: root._materialTypes.indexOf(root.editMaterial)
                            onActivated: function(idx) {
                                root.editMaterial = root._materialTypes[idx]
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }

                    // Color row: label + 25x25 editable picker + color value text
                    // (ColorPicker 25x25, cpp:1468-1470; m_clr_name, cpp:179-183)
                    RowLayout {
                        id: pickerRow
                        Layout.fillWidth: true
                        spacing: Theme.spacingLG

                        Text {
                            Layout.preferredWidth: 80
                            text: qsTr("颜色")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSize13
                        }

                        Rectangle {
                            id: colorSwatch
                            width: 25
                            height: 25
                            radius: Theme.radiusSM
                            color: root.editColor
                            border.color: Theme.borderDefault
                            border.width: 1

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    // Anchor to the window overlay so the popup is
                                    // never clipped by the scrolling content.
                                    var p = colorSwatch.mapToItem(null, 0, colorSwatch.height)
                                    colorPopup.x = p.x
                                    colorPopup.y = p.y + Theme.spacingXS
                                    colorPopup.open()
                                }
                            }
                        }

                        Text {
                            text: root.editColor.toString()
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSize13
                        }

                        Item { Layout.fillWidth: true }
                    }

                    // Nozzle temperature row: max/min 90x24 pair with degree
                    // markers (AMS_MATERIALS_SETTING_INPUT_SIZE hpp:30, :197-230)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingLG

                        Text {
                            Layout.preferredWidth: 80
                            text: qsTr("喷嘴温度")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSize13
                        }

                        ColumnLayout {
                            spacing: Theme.spacingXS

                            // "max"/"min" labels above the inputs (:217-226)
                            RowLayout {
                                spacing: 10 // upstream label gap FromDIP(10) (:213)

                                Text {
                                    Layout.preferredWidth: 90
                                    horizontalAlignment: Text.AlignHCenter
                                    text: qsTr("最大")
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSize13
                                }

                                Text {
                                    Layout.preferredWidth: 90
                                    horizontalAlignment: Text.AlignHCenter
                                    text: qsTr("最小")
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSize13
                                }
                            }

                            RowLayout {
                                spacing: 10

                                CxTextField {
                                    id: tempMaxInput
                                    Layout.preferredWidth: 90
                                    Layout.preferredHeight: 24
                                    font.pixelSize: Theme.fontSize13
                                    horizontalAlignment: TextInput.AlignHCenter
                                    placeholderText: "--"
                                    text: root.editTempMax
                                    onEditingFinished: root.editTempMax = text
                                }

                                Text {
                                    text: "°C"
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSize13
                                }

                                CxTextField {
                                    id: tempMinInput
                                    Layout.preferredWidth: 90
                                    Layout.preferredHeight: 24
                                    font.pixelSize: Theme.fontSize13
                                    horizontalAlignment: TextInput.AlignHCenter
                                    placeholderText: "--"
                                    text: root.editTempMin
                                    onEditingFinished: root.editTempMin = text
                                }

                                Text {
                                    text: "°C"
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSize13
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }

                    // 120..300 validation warning (warning_text :235-243; the
                    // upstream orange #FF6F00 maps to the statusWarning token).
                    Text {
                        visible: root.editTempOutOfRange
                        text: qsTr("输入值应大于 %1 且小于 %2").arg(root.tempMinLimit).arg(root.tempMaxLimit)
                        color: Theme.statusWarning
                        font.pixelSize: Theme.fontSize13
                        wrapMode: Text.Wrap
                        Layout.fillWidth: true
                    }

                    // Readonly SN row (m_panel_SN, cpp:245-266)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingLG

                        Text {
                            Layout.preferredWidth: 80
                            text: qsTr("SN")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSize13
                        }

                        Text {
                            text: root.editSn
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSize13
                        }

                        Item { Layout.fillWidth: true }
                    }

                    // Confirm/Reset/Close, right-aligned (m_sizer_button :59-85):
                    // 20px between buttons, row margins 20 L/R, 24 above, 16 below.
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 8 // 8 + 16 row spacing = upstream 24 above buttons (:83)
                        spacing: 20 // upstream button spacing FromDIP(20) (:75-76)

                        Item { Layout.fillWidth: true }

                        CxButton {
                            text: qsTr("确认")
                            cxStyle: CxButton.Style.Primary
                            enabled: !root.editTempOutOfRange
                            onClicked: root.confirmSlotEdit()
                        }

                        CxButton {
                            text: qsTr("重置")
                            cxStyle: CxButton.Style.Secondary
                            onClicked: root.resetSlotEdit()
                        }

                        CxButton {
                            text: qsTr("关闭")
                            cxStyle: CxButton.Style.Secondary
                            onClicked: root.editSlot = -1
                        }
                    }
                }
            }

            // ── Section: filament slot management ────────────────────────────
            Text {
                text: qsTr("耗材槽位管理")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSize13
                font.bold: true
            }

            // 4 slot cards in a 2x2 grid
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 8
                rowSpacing: 8

                Repeater {
                    model: root._slotCount

                    Rectangle {
                        required property int index
                        Layout.fillWidth: true
                        implicitHeight: slotCol.implicitHeight + 16
                        radius: Theme.radiusSM
                        // 差距7: card surface on the inset token (was the
                        // scrollbar track color).
                        color: Theme.bgInset
                        border.color: Theme.borderInput
                        border.width: 1

                        ColumnLayout {
                            id: slotCol
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: Theme.spacingMD
                            spacing: Theme.spacingSM
                            // Slot header: color indicator + name
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.spacingSM
                                Rectangle {
                                    width: 16
                                    height: 16
                                    radius: 8
                                    color: root._slotColors[index] || Theme.borderDefault
                                    border.color: Theme.borderDefault
                                    border.width: 1
                                }

                                Text {
                                    text: qsTr("槽位 %1").arg(index + 1)
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeSM
                                    font.bold: true
                                }

                                Item { Layout.fillWidth: true }

                                CxCheckBox {
                                    text: qsTr("自动换色")
                                    font.pixelSize: Theme.fontSize13
                                    // Bind directly to the viewmodel so edits persist immediately.
                                    // QML-BINDING-LOOPS: onToggled instead of
                                    // onCheckedChanged -- the programmatic checked
                                    // refresh (VM stateChanged) no longer writes
                                    // back, so only user interaction mutates the VM.
                                    checked: root._slotAutoSwap[index] === true
                                    onToggled: {
                                        if (root.amsVm)
                                            root.amsVm.setSlotAutoSwap(index, checked)
                                    }
                                }

                                // 差距4: per-slot entry into the single-slot editor
                                // (upstream AMSMaterialsSetting opens per slot).
                                CxButton {
                                    text: qsTr("编辑")
                                    cxStyle: CxButton.Style.Secondary
                                    compact: true
                                    onClicked: root.openSlotEditor(index)
                                }
                            }

                            // Filament name
                            CxTextField {
                                Layout.fillWidth: true
                                implicitHeight: 24
                                font.pixelSize: Theme.fontSizeSM
                                text: root._slotNames[index] || ""
                                onEditingFinished: {
                                    if (root.amsVm)
                                        root.amsVm.setSlotName(index, text)
                                }
                            }

                            // Material type + color chip row
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: Theme.spacingSM
                                CxComboBox {
                                    Layout.fillWidth: true
                                    implicitHeight: 24
                                    font.pixelSize: Theme.fontSizeSM
                                    model: root._materialTypes
                                    currentIndex: {
                                        var m = root._slotMaterials[index] || ""
                                        return root._materialTypes.indexOf(m)
                                    }
                                    onActivated: function(idx) {
                                        if (root.amsVm)
                                            root.amsVm.setSlotMaterial(index, root._materialTypes[idx])
                                    }
                                }

                                // Readonly color indicators. The editable picker
                                // lives in the slot editor; these card chips stay
                                // read-only and coexist with it (差距4).
                                Row {
                                    spacing: Theme.spacingXS
                                    Repeater {
                                        model: root._slotColors
                                        Rectangle {
                                            required property int index
                                            required property string modelData
                                            width: 16
                                            height: 16
                                            radius: 3
                                            color: modelData
                                            border.color: Theme.textDisabled
                                            border.width: 1

                                            HoverHandler { id: amsColorHover }
                                            ToolTip.visible: amsColorHover.hovered
                                            ToolTip.text: qsTr("AMS slot color is read-only")
                                            ToolTip.delay: 500
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ── Section: slot mapping (差距5: scoped to the slot mapping rows) ──
            Text {
                text: qsTr("槽位映射")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSize13
                font.bold: true
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: mappingList.contentHeight + 8
                radius: Theme.radiusSM
                // 差距7: mapping container on the inset token (was bgPanel).
                color: Theme.bgInset
                border.color: Theme.borderInput
                border.width: 1

                ListView {
                    id: mappingList
                    anchors.fill: parent
                    anchors.margins: Theme.spacingXS
                    model: root._mappingRules
                    spacing: Theme.spacingXS
                    interactive: false

                    delegate: Rectangle {
                        required property int index
                        required property var modelData
                        width: ListView.view.width
                        height: 26
                        radius: Theme.radiusSM
                        // 差距9: no invented zebra striping -- rows share the
                        // container surface with a 1px subtle outline.
                        color: "transparent"
                        border.color: Theme.borderSubtle
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.spacingMD
                            anchors.rightMargin: Theme.spacingMD
                            spacing: Theme.spacingMD
                            Text {
                                text: qsTr("Slot %1").arg(modelData.slot)
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeSM
                                font.bold: true
                            }

                            Text {
                                text: "->"
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeSM
                            }

                            Text {
                                text: qsTr("Extruder %1").arg(modelData.extruder)
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeSM
                            }

                            Item { Layout.fillWidth: true }

                            // Remove a single mapping rule (Phase 201: previously no delete UI).
                            CxButton {
                                text: "x"
                                cxStyle: CxButton.Style.Secondary
                                compact: true
                                onClicked: {
                                    if (root.amsVm)
                                        root.amsVm.removeMappingRule(index)
                                }
                            }
                        }
                    }
                }
            }

            // ── Section: remaining filament per slot ─────────────────────────
            Text {
                text: qsTr("槽位耗材余量")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSize13
                font.bold: true
            }

            Repeater {
                model: root._slotCount

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingMD
                    Rectangle {
                        width: 12
                        height: 12
                        radius: 6
                        color: root._slotColors[index] || Theme.borderDefault
                        border.color: Theme.borderDefault
                        border.width: 1
                    }

                    Text {
                        text: qsTr("Slot %1:").arg(index + 1)
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        Layout.preferredWidth: 50
                    }

                    // Progress bar
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 10
                        radius: 5
                        color: Theme.bgInset
                        border.color: Theme.borderInput
                        border.width: 1

                        Rectangle {
                            width: parent.width * ((root._remainingPct[index] || 0) / 100)
                            height: parent.height
                            radius: 5
                            color: {
                                var pct = root._remainingPct[index] || 0
                                if (pct <= 20) return Theme.statusError
                                if (pct <= 50) return Theme.statusWarning
                                return Theme.accent
                            }
                        }
                    }

                    Text {
                        text: (root._remainingPct[index] || 0) + "%"
                        color: {
                            var pct = root._remainingPct[index] || 0
                            if (pct <= 20) return Theme.statusError
                            if (pct <= 50) return Theme.statusWarning
                            return Theme.textPrimary
                        }
                        font.pixelSize: Theme.fontSizeSM
                        font.bold: true
                        Layout.preferredWidth: 36
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }

            // ── 差距3: product image panel at the list tail (:239-248) ────────
            // Upstream renders the 126px "ams_icon" bitmap, top offset 26, on a
            // GREY200 panel. The asset is missing from src/qml_gui/assets/ --
            // placeholder: a QML device outline carrying the live slot colors
            // (asset gap registered for the brand-asset batch).
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 26 + 126 + 18 + Theme.spacingXS
                color: Theme.bgInset

                Column {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 26 // upstream wxTOP 26 (:244)
                    spacing: Theme.spacingSM

                    Rectangle {
                        width: 126
                        height: 100
                        radius: Theme.radiusMD
                        color: Theme.bgSurface
                        border.color: Theme.borderSubtle
                        border.width: 1

                        Column {
                            anchors.centerIn: parent
                            spacing: Theme.spacingMD

                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "AMS"
                                color: Theme.textTertiary
                                font.pixelSize: Theme.fontSizeLG
                                font.bold: true
                            }

                            Row {
                                anchors.horizontalCenter: parent.horizontalCenter
                                spacing: Theme.spacingMD

                                Repeater {
                                    model: Math.min(root._slotCount, 4)
                                    Rectangle {
                                        required property int index
                                        width: 14
                                        height: 14
                                        radius: 3
                                        color: root._slotColors[index] || Theme.borderDefault
                                        border.color: Theme.borderDefault
                                        border.width: 1
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Editable color picker popup (差距4). Upstream ColorPickerPopup offers the
    // default palette + AMS colours + a custom picker (AMSMaterialsSetting.cpp
    // ColorPickerPopup); the mock data channel here only carries the four slot
    // colours, so the popup offers those -- the full palette/custom picker is
    // deferred with the VM color channel.
    Popup {
        id: colorPopup
        parent: Overlay.overlay
        width: paletteGrid.implicitWidth + Theme.spacingXL
        height: paletteGrid.implicitHeight + Theme.spacingXL
        padding: Theme.spacingSM
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: Theme.bgElevated
            border.color: Theme.borderDefault
            border.width: 1
            radius: Theme.radiusSM
        }

        contentItem: GridLayout {
            id: paletteGrid
            columns: 4
            columnSpacing: Theme.spacingSM
            rowSpacing: Theme.spacingSM

            Repeater {
                model: root._slotColors
                Rectangle {
                    required property int index
                    required property string modelData
                    implicitWidth: 22
                    implicitHeight: 22
                    radius: Theme.radiusSM
                    color: modelData
                    border.color: root.editColor.toString().toUpperCase() === modelData.toUpperCase()
                                  ? Theme.borderFocus : Theme.borderDefault
                    border.width: 1

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.editColor = modelData
                            colorPopup.close()
                        }
                    }
                }
            }
        }
    }
}
