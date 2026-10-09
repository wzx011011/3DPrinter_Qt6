import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../controls"

// P8.6c -- PrintHostDialog (upstream truth: PhysicalPrinterDialog.cpp)
// "Physical Printer" dialog = preset-name save block ("Save Printer as",
// PhysicalPrinterDialog.cpp:69-95) + the "Print Host upload" option group
// (build_printhost_settings :133-404) + a single OK footer
// (DialogButtons(this, {"OK"}) :103).
// Geometry / visibility cite PhysicalPrinterDialog.cpp unless noted; colors
// stay on the OWzx neutral/accent tokens. Usage:
//   PrintHostDialog { id: dlg }  ->  dlg.open()

CxDialog {
    id: root

    // Upstream binds the window close to EndModal(wxID_NO) -- closing
    // discards, and no Cancel button exists (PhysicalPrinterDialog.cpp:117).
    closePolicy: Popup.CloseOnEscape

    dialogTitle: qsTr("物理打印机")  // _L("Physical Printer") :55

    anchors.centerIn: parent
    // wxSize(45 * em, -1) = 450px wide at em=10 with content-driven height;
    // DPI min size 45x35em = 450x350 (:55, :795-796).
    width: 450
    implicitHeight: bodyColumn.implicitHeight + 2 * 10
                    + Theme.dialogHeaderHeight + Theme.dialogFooterHeight
    padding: Theme.spacingMD  // BORDER_W = FromDIP(10) :52

    // Preset-service handle for the OK save (upstream OnOK writes the printer
    // preset, :792-797; OWzx routes through ConfigViewModel).
    readonly property var configVm: typeof backend !== "undefined" && backend
        ? backend.configViewModel : null

    // ── Host-type enum (PrintConfig.cpp:5385-5424) ────────────────────────
    // Per-host capability flags consumed by update_printhost_buttons
    // (:594-602): browse = has_auto_discovery (Browse button), cloud =
    // is_cloud ("Login/Test" relabel :509), login = is_logged_in (Log Out),
    // multi = supports_multiple_printers (Printer line :722-723). Values from
    // Utils/{OctoPrint,Duet,FlashAir,AstroBox,Repetier,MKS,ESP3D,
    // CrealityPrint,Obico,Flashforge,SimplyPrint,ElegooLink,3DPrinterOS,
    // Moonraker}.hpp overrides; everything unset keeps the base defaults.
    readonly property var hostTypes: [
        { label: qsTr("PrusaLink"),          browse: true,  cloud: false, login: false, multi: false },
        { label: qsTr("PrusaConnect"),       browse: true,  cloud: false, login: false, multi: false },
        { label: qsTr("Octo/Klipper"),       browse: true,  cloud: false, login: false, multi: false },
        { label: qsTr("Duet"),               browse: false, cloud: false, login: false, multi: false },
        { label: qsTr("FlashAir"),           browse: false, cloud: false, login: false, multi: false },
        { label: qsTr("AstroBox"),           browse: true,  cloud: false, login: false, multi: false },
        { label: qsTr("Repetier"),           browse: false, cloud: false, login: false, multi: true },
        { label: qsTr("MKS"),                browse: false, cloud: false, login: false, multi: false },
        { label: qsTr("ESP3D"),              browse: false, cloud: false, login: false, multi: false },
        { label: qsTr("CrealityPrint"),      browse: true,  cloud: false, login: false, multi: false },
        { label: qsTr("Obico"),              browse: false, cloud: true,  login: false, multi: true },
        { label: qsTr("Flashforge"),         browse: true,  cloud: false, login: false, multi: false },
        { label: qsTr("SimplyPrint"),        browse: false, cloud: true,  login: true,  multi: false },
        { label: qsTr("Elegoo Link"),        browse: false, cloud: false, login: false, multi: false },
        { label: qsTr("3DPrinterOS"),        browse: false, cloud: true,  login: true,  multi: false },
        { label: qsTr("Moonraker (Klipper)"), browse: false, cloud: false, login: false, multi: false }
    ]
    // Default htOctoPrint = "Octo/Klipper" (PrintConfig.cpp:5424).
    property int hostTypeIndex: 2
    readonly property var hostCaps: hostTypes[hostTypeIndex]

    // ── update() visibility branches (:601-730) ───────────────────────────
    readonly property bool isPrusaLink: hostTypeIndex === 0
    // printhost_authorization_type default atKeyPassword (PrintConfig.cpp:1083);
    // combo labels "API key"/"HTTP digest" (PrintConfig.cpp:1079-1080).
    property int authTypeIndex: 0
    readonly property bool authIsKey: authTypeIndex === 0
    // PrusaLink shows the combo and swaps apikey <-> user/password (:624-629);
    // every other host hides the combo, always shows the apikey (except
    // SimplyPrint/3DPrinterOS which hide it, :683/:701) and hides the pair.
    readonly property bool showAuthTypeRow: isPrusaLink
    readonly property bool showApiKeyRow: isPrusaLink ? authIsKey
        : !(hostTypeIndex === 12 || hostTypeIndex === 14)
    readonly property bool showUserPasswordRows: isPrusaLink && !authIsKey
    readonly property bool showPortRow: hostCaps.multi
    readonly property bool showWebuiRow: !(hostTypeIndex === 12 || hostTypeIndex === 14)
    // The fake coBool only shows for SimplyPrint + BBL vendor (:672-673);
    // this BBL-lineage fork always satisfies the vendor half.
    readonly property bool showBblWebuiRow: hostTypeIndex === 12
    // SimplyPrint disables printhost_cafile and its Browse button (:686-692).
    readonly property bool caRowEnabled: hostTypeIndex !== 12

    // Async probe state (backend.testPrintHost -> onPrintHostTestFinished).
    property bool testRunning: false
    // OnOK persistence failure text; empty while the name check rules the label.
    property string saveError: ""

    // ── update_preset_input (:513-589) ────────────────────────────────────
    // First failing rule wins; "warning" keeps OK enabled, "invalid" gates it
    // (btnOK->Disable() :581-584).
    function validatePresetName(name) {
        const unusableSymbols = "<>[]:/\\|?*\""
        for (let i = 0; i < unusableSymbols.length; ++i) {
            if (name.indexOf(unusableSymbols[i]) >= 0)
                return { state: "invalid",
                         info: qsTr("名称无效；\n非法字符：") + " " + unusableSymbols }
        }
        if (name.indexOf(" (modified)") >= 0)
            return { state: "invalid",
                     info: qsTr("名称无效；\n非法后缀：\n\t(modified)") }
        if (name === "Default Setting" || name === "Default Filament"
                || name === "Default Printer")
            return { state: "invalid", info: qsTr("该名称不可用。") }
        if (configVm) {
            const idx = configVm.printerPresetNames.indexOf(name)
            if (idx >= 0) {
                // Default/system presets are not overwrite targets; an
                // existing USER preset warns and allows the overwrite.
                const isUser = configVm.userPresetNamesForCategory(2).indexOf(name) >= 0
                if (!isUser)
                    return { state: "invalid", info: qsTr("不允许覆盖系统配置。") }
                if (name !== configVm.currentPrinterPreset)
                    return { state: "warning",
                             info: qsTr("预设 %1 已存在。\n请注意保存将覆盖当前预设。").arg(name) }
            }
        }
        if (name.length === 0)
            return { state: "invalid", info: qsTr("名称不允许为空。") }
        if (name.charAt(0) === " ")
            return { state: "invalid", info: qsTr("名称不允许以空格开头。") }
        if (name.charAt(name.length - 1) === " ")
            return { state: "invalid", info: qsTr("名称不允许以空格结尾。") }
        return { state: "valid", info: "" }
    }
    readonly property var presetNameCheck: validatePresetName(presetNameField.text)
    readonly property string validationInfo: saveError.length > 0
        ? saveError : presetNameCheck.info
    readonly property bool okEnabled: saveError.length === 0
        && presetNameCheck.state !== "invalid"

    // Constructor preseed (:69-71): is_default -> "Untitled", is_system ->
    // "<name> - Copy", else the plain name. OWzx derives the system case from
    // the preset's inheritance parent.
    Component.onCompleted: {
        const current = configVm ? configVm.currentPrinterPreset : ""
        if (current === "")
            presetNameField.text = "Untitled"
        else if (configVm.presetInheritsParent(current) !== "")
            presetNameField.text = current + " - " + qsTr("副本")
        else
            presetNameField.text = current
    }

    contentItem: ColumnLayout {
        id: bodyColumn
        anchors.fill: parent
        spacing: Theme.spacingMD

        // ── Preset-name input block (input_sizer :69-95) ──────────────────
        Text {
            text: qsTr("将打印机保存为")  // "Save %s as" x printer tab title
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeLG  // Label::Body_14
        }
        // m_input_area RoundedRectangle: radius 3, 1px border, min 360x32
        // (:76-77); the text ctrl is inset 12px left/right (:84).
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 32
            radius: Theme.radiusSM
            color: Theme.bgPanel
            border.width: 1
            border.color: Theme.borderDefault
            TextField {
                id: presetNameField
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                background: null
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                selectByMouse: true
                verticalAlignment: TextInput.AlignVCenter
                onTextEdited: root.saveError = ""
            }
        }
        // m_valid_label, orange (upstream wxColour(255,111,0) :89); hidden
        // while the name is clean (:585-587).
        Text {
            visible: root.validationInfo.length > 0
            text: root.validationInfo
            color: Theme.statusWarning
            font.pixelSize: Theme.fontSizeSM
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // ── "Print Host upload" group (ConfigOptionsGroup :99) ────────────
        Text {
            text: qsTr("打印主机上传")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeMD
            font.bold: true
        }

        // host_type (append_single_option_line "host_type" :141)
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            OptionLabel {
                text: qsTr("主机类型")  // L("Host Type")
            }
            CxComboBox {
                id: hostTypeCombo
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
                textRole: "label"
                model: root.hostTypes
                currentIndex: root.hostTypeIndex
                onActivated: function(idx) {
                    root.hostTypeIndex = idx
                    // update() default-address branches (:644-707).
                    if (idx === 1 && hostUrlField.text.length === 0) {
                        hostUrlField.text = "https://connect.prusa3d.com"
                    } else if (idx === 10 && hostUrlField.text.length === 0) {
                        hostUrlField.text = "https://app.obico.io"
                    } else if (idx === 12) {
                        // SimplyPrint pins the host and mirrors the webui
                        // state into the Device-tab checkbox (:662-682).
                        hostUrlField.text = "https://simplyprint.io/panel"
                        bblWebuiCheck.checked = webuiField.text.length > 0
                    } else if (idx === 14 && hostUrlField.text.length === 0) {
                        hostUrlField.text = "https://cloud.3dprinteros.com"
                    }
                }
            }
        }

        // print_host line with inline Browse / Test / Log Out (:300-306)
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            OptionLabel {
                text: qsTr("主机地址")  // L("Hostname, IP or URL")
            }
            CxTextField {
                id: hostUrlField
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
                onTextEdited: {
                    // Upstream strips leading/trailing spaces on every edit
                    // (PhysicalPrinterDialog.cpp:642-651).
                    const trimmed = text.trim()
                    if (trimmed !== text)
                        text = trimmed
                }
            }
            ParameterButton {
                // m_printhost_browse_btn: shown iff has_auto_discovery (:598).
                visible: root.hostCaps.browse
                label: qsTr("浏览…")  // _L("Browse") + dots :153
                iconSource: "qrc:/qml/assets/icons/printer_host_browser.svg"
                onClicked: discoveryPopup.open()
            }
            ParameterButton {
                // m_printhost_test_btn: "Login/Test" for cloud hosts (:509).
                label: root.hostCaps.cloud ? qsTr("登录/测试") : qsTr("测试")
                iconSource: "qrc:/qml/assets/icons/printer_host_test.svg"
                // Enable(!print_host.empty() && can_test) :596; the probe is
                // async so the button also parks while a test is in flight.
                enabled: root.testRunning === false
                    && hostUrlField.text.trim().length > 0
                onClicked: {
                    root.testRunning = true
                    // show_error branch for a missing host never triggers
                    // here: can_test() is true for every host type.
                    backend.testPrintHost(hostUrlField.text.trim(),
                                          root.showApiKeyRow ? apiKeyField.text : "",
                                          root.showUserPasswordRows ? userField.text : "",
                                          root.showUserPasswordRows ? passwordField.text : "")
                }
            }
            ParameterButton {
                // m_printhost_logout_btn: created with an empty icon (:268)
                // and shown iff is_logged_in (:599) -- only SimplyPrint /
                // 3DPrinterOS report it (and only with stored credentials).
                visible: root.hostCaps.login && apiKeyField.text.length > 0
                label: qsTr("退出登录")
                onClicked: logoutConfirm.open()
            }
        }

        // print_host_webui (append_single_option_line :308-310)
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            visible: root.showWebuiRow
            OptionLabel {
                text: qsTr("设备界面")  // L("Device UI")
            }
            CxTextField {
                id: webuiField
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
            }
        }

        // bbl_use_print_host_webui fake coBool (:312-323), SimplyPrint only
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            visible: root.showBblWebuiRow
            OptionLabel {
                text: qsTr("在设备页显示打印主机 Web UI")
                Layout.preferredWidth: 150
            }
            CxCheckBox {
                id: bblWebuiCheck
                onToggled: {
                    // update_webui (:459-487): checked fills the SimplyPrint
                    // panel URL, unchecked clears the field.
                    webuiField.text = checked ? "https://simplyprint.io/panel" : ""
                }
            }
            Item { Layout.fillWidth: true }
        }

        // printhost_authorization_type (:395 append order: before apikey)
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            visible: root.showAuthTypeRow
            OptionLabel {
                text: qsTr("认证方式")  // L("Authorization Type")
            }
            CxComboBox {
                id: authTypeCombo
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
                model: [qsTr("API 密钥"), qsTr("HTTP 摘要")]
                currentIndex: root.authTypeIndex
                onActivated: function(idx) {
                    // Swaps the API-key field against the User/Password pair
                    // (:624-629).
                    root.authTypeIndex = idx
                }
            }
        }

        // printhost_apikey (:397-398)
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            visible: root.showApiKeyRow
            OptionLabel {
                text: qsTr("API 密钥/密码")  // L("API Key / Password")
            }
            CxTextField {
                id: apiKeyField
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
            }
        }

        // printhost_port line with the Refresh widget (:335-339), hidden
        // unless Repetier/Obico (:722-723)
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            visible: root.showPortRow
            OptionLabel {
                text: qsTr("打印机")  // L("Printer")
            }
            CxComboBox {
                id: portCombo
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
                // Always seed from the config, even when empty (:656-661).
                model: [""]
            }
            ParameterButton {
                // m_printhost_port_browse_btn "Refresh …" (:290-297); shown
                // together with the line (:723).
                visible: root.showPortRow
                label: qsTr("刷新…")  // _L("Refresh") + dots :291
                iconSource: "qrc:/qml/assets/icons/monitor_signal_strong.svg"
                onClicked: {
                    // update_printers (:732-758) mock: OWzx has no Repetier /
                    // Obico server probe, so the combo fills from the device
                    // roster and disables when it comes back empty (:753).
                    const vm = typeof backend !== "undefined" && backend
                        ? backend.monitorViewModel : null
                    const roster = vm ? vm.discoveredPrinters() : []
                    const names = roster.map(function(d) { return d.name })
                    portCombo.model = names
                    portCombo.enabled = names.length > 0
                }
            }
        }

        // printhost_cafile line + Browse widget (:343-360)
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            OptionLabel {
                text: qsTr("HTTPS CA 证书")  // L("HTTPS CA File")
            }
            CxTextField {
                id: caFileField
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
                enabled: root.caRowEnabled
            }
            ParameterButton {
                // m_printhost_cafile_browse_btn; disabled for SimplyPrint
                // together with the field (:686-692).
                label: qsTr("浏览…")
                iconSource: "qrc:/qml/assets/icons/monitor_signal_strong.svg"
                enabled: root.caRowEnabled
                onClicked: caFileDialog.open()
            }
        }
        // Full-width CA hint line (:361-373); Qt Network supports custom CA
        // files, so the ca_file_supported() branch applies.
        Text {
            text: qsTr("HTTPS CA 证书为可选项。仅在使用自签名证书的 HTTPS 连接时才需要。")
            color: Theme.textTertiary
            font.pixelSize: Theme.fontSizeSM
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        // printhost_user / printhost_password (:395-399)
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            visible: root.showUserPasswordRows
            OptionLabel {
                text: qsTr("用户名")  // L("User")
            }
            CxTextField {
                id: userField
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
            }
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD
            visible: root.showUserPasswordRows
            OptionLabel {
                text: qsTr("密码")  // L("Password")
            }
            CxTextField {
                id: passwordField
                Layout.fillWidth: true
                implicitHeight: 24
                font.pixelSize: Theme.fontSizeSM
            }
        }
    }

    // ── Footer: the single upstream OK button (DialogButtons {"OK"} :103) ──
    footer: Rectangle {
        width: parent.width
        height: Theme.dialogFooterHeight  // 52 = OK 32 + 2x10 btn_gap
        color: Theme.bgSurface
        radius: Theme.radiusLG
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 12
            color: parent.color
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacingLG
            anchors.rightMargin: Theme.spacingLG  // btn_gap margins
            spacing: Theme.spacingLG
            // AddStretchSpacer before the right-aligned OK
            // (DialogButtons.cpp:136-138).
            Item { Layout.fillWidth: true }

            CxButton {
                text: qsTr("确定")
                cxStyle: CxButton.Style.Primary  // Confirm/Choice styling
                implicitWidth: 100   // Choice min size 100x32 (Button.cpp:200)
                implicitHeight: 32
                // btnOK->Disable() while NoValid (:581-584).
                enabled: root.okEnabled
                onClicked: {
                    // OnOK (:792-797): save the printer preset under the
                    // typed name; keep the dialog open when persistence
                    // rejects it.
                    if (root.configVm) {
                        const name = presetNameField.text.trim()
                        if (!root.configVm.createCustomPreset(2, name)) {
                            root.saveError = root.configVm.lastPresetError.length > 0
                                ? root.configVm.lastPresetError : qsTr("保存预设失败。")
                            return
                        }
                    }
                    root.accept()
                }
            }
        }
    }

    // ── Inline components ─────────────────────────────────────────────────
    // Left column of an option line (ConfigOptionsGroup label gutter).
    component OptionLabel: Text {
        Layout.preferredWidth: 80
        color: Theme.textTertiary
        font.pixelSize: Theme.fontSizeSM
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    // Upstream Button ButtonType::Parameter (Button.cpp:206-211): min 120x26,
    // corner radius 4, 1px border, Body_14 label, optional 16px icon.
    component ParameterButton: Rectangle {
        id: paramButton
        property string label: ""
        property url iconSource: ""
        signal clicked()
        implicitWidth: Math.max(120, paramContent.implicitWidth + 24)  // padding 12x8
        implicitHeight: 26
        radius: Theme.radiusSM
        color: paramMouse.containsMouse ? Theme.bgHover : Theme.bgPanel
        border.width: 1
        border.color: paramMouse.containsMouse ? Theme.borderStrong : Theme.borderDefault
        opacity: enabled ? 1.0 : 0.45
        Row {
            id: paramContent
            anchors.centerIn: parent
            spacing: 6
            Image {
                visible: paramButton.iconSource !== ""
                source: paramButton.iconSource
                sourceSize.width: 16
                sourceSize.height: 16
                fillMode: Image.PreserveAspectFit
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: paramButton.label
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG  // Label::Body_14
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        MouseArea {
            id: paramMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: paramButton.clicked()
        }
    }

    // ── Discovery picker (Browse …, :151-208) ─────────────────────────────
    // Upstream opens BonjourDialog / CrealityDiscoveryDialog / the Flashforge
    // scanner; OWzx lists the mock device roster instead and writes the same
    // "http://<ip>" form the Creality branch uses (:171-174).
    Popup {
        id: discoveryPopup
        property var printers: []
        modal: true
        dim: true
        anchors.centerIn: parent
        width: 320
        height: Math.min(320, discoveryBody.implicitHeight + 2 * Theme.spacingXL)
        padding: Theme.spacingXL
        background: Rectangle {
            color: Theme.bgElevated
            border.width: 1
            border.color: Theme.borderInput
            radius: Theme.radiusLG
        }
        onOpened: {
            const vm = typeof backend !== "undefined" && backend
                ? backend.monitorViewModel : null
            discoveryPopup.printers = vm ? vm.discoveredPrinters() : []
        }
        contentItem: ColumnLayout {
            id: discoveryBody
            spacing: Theme.spacingMD
            Text {
                text: qsTr("发现的打印机")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                font.bold: true
            }
            Text {
                visible: discoveryPopup.printers.length === 0
                text: qsTr("未发现可用的打印机。")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeSM
                Layout.fillWidth: true
            }
            ListView {
                id: discoveryList
                visible: discoveryPopup.printers.length > 0
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(240, contentHeight)
                clip: true
                spacing: Theme.spacingXXS
                model: discoveryPopup.printers
                delegate: ItemDelegate {
                    id: discoveryRow
                    required property var modelData
                    width: discoveryList.width
                    height: Theme.controlHeightSM
                    contentItem: Text {
                        leftPadding: Theme.spacingMD
                        text: discoveryRow.modelData.name + " (" + discoveryRow.modelData.ip + ")"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }
                    background: Rectangle {
                        color: discoveryRow.hovered ? Theme.bgHover : "transparent"
                        radius: Theme.radiusSM
                    }
                    onClicked: {
                        hostUrlField.text = "http://" + discoveryRow.modelData.ip
                        discoveryPopup.close()
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("取消")
                    cxStyle: CxButton.Style.Secondary
                    compact: true
                    onClicked: discoveryPopup.close()
                }
            }
        }
    }

    // ── Test result modal (show_info / show_error, :256-259) ──────────────
    Popup {
        id: testResultPopup
        property bool isError: false
        property string message: ""
        modal: true
        dim: true
        anchors.centerIn: parent
        width: 320
        padding: Theme.spacingXL
        background: Rectangle {
            color: Theme.bgElevated
            border.width: 1
            border.color: Theme.borderInput
            radius: Theme.radiusLG
        }
        contentItem: ColumnLayout {
            spacing: Theme.spacingMD
            Text {
                text: testResultPopup.isError ? qsTr("错误") : qsTr("成功！")
                color: testResultPopup.isError ? Theme.statusError : Theme.statusSuccess
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
            }
            Text {
                text: testResultPopup.message
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("确定")
                    cxStyle: CxButton.Style.Primary
                    onClicked: testResultPopup.close()
                }
            }
        }
    }
    Connections {
        target: typeof backend !== "undefined" ? backend : null
        function onPrintHostTestFinished(ok, detail) {
            root.testRunning = false
            testResultPopup.isError = !ok
            testResultPopup.message = detail
            testResultPopup.open()
        }
    }

    // ── Log Out confirm ("Are you sure to log out?", :279-282) ────────────
    Popup {
        id: logoutConfirm
        modal: true
        dim: true
        anchors.centerIn: parent
        width: 320
        padding: Theme.spacingXL
        background: Rectangle {
            color: Theme.bgElevated
            border.width: 1
            border.color: Theme.borderInput
            radius: Theme.radiusLG
        }
        contentItem: ColumnLayout {
            spacing: Theme.spacingMD
            Text {
                text: qsTr("确定要退出登录吗？")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("取消")
                    cxStyle: CxButton.Style.Secondary
                    compact: true
                    onClicked: logoutConfirm.close()
                }
                CxButton {
                    text: qsTr("确定")
                    cxStyle: CxButton.Style.Primary
                    compact: true
                    onClicked: {
                        // host->log_out() mock: clears the stored credential,
                        // which also hides the Log Out button (:599).
                        apiKeyField.text = ""
                        logoutConfirm.close()
                    }
                }
            }
        }
    }

    // CA certificate file picker (masks from PhysicalPrinterDialog.cpp:351).
    FileDialog {
        id: caFileDialog
        nameFilters: [qsTr("证书文件 (*.crt *.pem)"), qsTr("所有文件 (*)")]
        onAccepted: {
            caFileField.text = selectedFile.toString().replace(/^file:\/\/\//, "")
        }
    }
}
