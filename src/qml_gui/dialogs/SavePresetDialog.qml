import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// SavePresetDialog.qml — PRESET-01 save-preset dialog (upstream
// SavePresetDialog structure, 1:1 element layout).
//
// Upstream: third_party/OrcaSlicer/src/slic3r/GUI/SavePresetDialog.cpp / .hpp
//   - one Item per tier, single caption "Save <tab title> as" (cpp:56-58,100)
//   - name input fixed 360x24 DIP (hpp:21, cpp:89-90), no "Name:" prefix
//   - validation label right below the input (cpp:97-102); update() checks in
//     order (cpp:175-262): illegal chars -> " (modified)" suffix -> the three
//     exact reserved names -> system-overwrite ban -> existing-preset
//     overwrite warning -> empty name / leading space / trailing space -> alias
//   - printer tier: physical-printer info line + 3-action radio group
//     (cpp:104,394-451,472-493)
//   - "User Preset" / "Preset Inside Project" radio group (cpp:107-112,
//     163-168): initial selection from the edited preset's
//     is_project_embedded; forced + disabled when the name hits an existing
//     preset (cpp:240-252)
//   - Detach checkbox + parent info line (cpp:114-161, built in comDevelop)
//   - DialogButtons [OK][Cancel], OK is the primary button (cpp:326-330,
//     DialogButtons.cpp:130-153)
//
// OWzx notes: colors come from Theme tokens (no upstream teal/#2D2D31); the
// save path stays on the four ConfigViewModel routes (saveCurrentPreset /
// detachPresetFromParent / overwriteUserPreset / createCustomPreset, locked by
// tests/QmlUiAuditTests.cpp:3462-3465 -- this file keeps 4-space indentation
// so the locked inline token "if (ok) {\n<28sp>root.accept()" stays intact).
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    dialogTitle: qsTr("保存预设")  // upstream _L("Save preset"), cpp:293
    // cpp:336 SetSizeHints: content width = input 360 + 2 * BORDER_W(10)
    width: 400
    padding: 0

    // Injections
    required property var configVm
    /// Current preset tier ("print"/"filament"/"printer")
    required property string presetTier
    /// Upstream build() suffix default (cpp:317 "Copy"); Tab passes
    /// "Detached" when the dialog opens from the detach flow (Tab.cpp:7360/7371)
    property string saveSuffix: qsTr("Copy")

    /// Upstream BORDER_W (cpp:26)
    readonly property int borderWidth: 10

    /// tier -> category index (matches createCustomPreset's category param)
    /// 0=print, 1=filament, 2=printer
    function tierToCategory(tier) {
        if (tier === "print") return 0
        if (tier === "filament") return 1
        if (tier === "printer") return 2
        return -1
    }

    function presetNamesForTier(tier) {
        if (!configVm) return []
        if (tier === "print") return configVm.printPresetNames
        if (tier === "filament") return configVm.filamentPresetNames
        if (tier === "printer") return configVm.printerPresetNames
        return []
    }

    /// R-P1.J: name of the CURRENT preset in this tier. Upstream
    /// SavePresetDialog's primary path saves over exactly this name.
    function currentPresetNameForTier() {
        if (!configVm) return ""
        if (presetTier === "print") return configVm.currentPrintPreset
        if (presetTier === "filament") return configVm.currentFilamentPreset
        return configVm.currentPrinterPreset
    }

    /// G-01: an existing USER preset is replaceable (upstream warns and
    /// replaces, SavePresetDialog.cpp:216-222); builtin/vendor duplicates
    /// are rejected by overwriteUserPreset.
    function isOverwriteTarget(name) {
        if (!configVm || name.length === 0) return false
        return configVm.userPresetNamesForCategory(root.category).indexOf(name) >= 0
    }

    /// Upstream cpp:56-58: the tab title inside "Save %s as"
    function tierTitle(tier) {
        if (tier === "print") return qsTr("打印设置")
        if (tier === "filament") return qsTr("耗材设置")
        if (tier === "printer") return qsTr("打印机设置")
        return tier
    }

    readonly property int category: tierToCategory(presetTier)

    /// Upstream cpp:39-45 suggested-name derivation (ConfigViewModel::
    /// suggestedSavePresetName): system preset -> "<name> - <suffix>",
    /// trailing ".ini" stripped. No more legacy " (modified)" suffix --
    /// upstream treats that as an illegal suffix (cpp:193-196).
    readonly property string suggestedName: configVm
        ? configVm.suggestedSavePresetName(category, saveSuffix) : ""

    /// The typed name hits an existing preset (incl. system, cpp:204)
    readonly property bool nameHitsExisting: configVm
        && presetNamesForTier(presetTier).indexOf(nameInput.text.trim()) >= 0

    /// Upstream cpp:240-252: when the name hits an existing preset the radio
    /// group is forced to that preset's is_project_embedded and disabled;
    /// otherwise it follows m_save_to_project (cpp:163-165).
    readonly property bool saveToProject: {
        if (!configVm) return false
        if (nameHitsExisting)
            return configVm.presetIsProjectEmbedded(nameInput.text.trim())
        return configVm.saveToProjectSelection
    }

    /// Upstream cpp:175-237 check order (ConfigViewModel::
    /// savePresetNameErrorCode). 0=Valid 1=illegal chars 2=illegal suffix
    /// 3=reserved name 4=system overwrite 5=empty 6=leading space 7=trailing
    /// space 8=alias conflict (upstream SavePresetNameError)
    readonly property int validationError: configVm
        ? configVm.savePresetNameErrorCode(category, nameInput.text) : 0
    /// Upstream cpp:210-217 overwrite warning. 0=none 1=exists 2=exists and
    /// incompatible
    readonly property int validationWarning: validationError === 0 && configVm
        ? configVm.savePresetNameWarningCode(category, nameInput.text) : 0

    /// Parent name of the Detach block (upstream cpp:116: a system preset is
    /// its own parent, a user preset uses its inherits link; empty =
    /// "Unique preset")
    readonly property string detachParentName: configVm
        ? configVm.savePresetParentName(category) : ""
    readonly property bool hasDetachParent: detachParentName.length > 0

    /// Upstream cpp:394-451: the physical-printer block shows on the printer
    /// tier only when a physical printer is selected AND the typed name
    /// differs from its current preset name (cpp:428).
    readonly property bool phPrinterVisible: presetTier === "printer" && configVm
        && configVm.physicalPrinterHasSelection()
        && configVm.physicalPrinterSelectedPresetName() !== nameInput.text

    /// Upstream m_action (hpp:87): ChangePreset initial (cpp:406)
    property int physicalPrinterAction: 0

    property string saveError: ""

    function resetDialogState() {
        saveError = ""
        physicalPrinterAction = 0  // upstream m_action = ChangePreset
        detachCheck.checked = false  // upstream hpp:77 m_detach{false}
        nameInput.text = suggestedName
        if (configVm) {
            // Upstream cpp:167-168: initial radio state from the edited
            // preset's is_project_embedded.
            configVm.saveToProjectSelection =
                configVm.editedPresetIsProjectEmbedded(category)
        }
        syncSaveRadios()
    }

    // Upstream update() cpp:240-252 radio sync (imperative on purpose: a
    // checked binding would fight autoExclusive on user clicks). The typeof
    // guard covers the creation phase, where onTextChanged can fire before
    // the radio column exists.
    function syncSaveRadios() {
        if (typeof userPresetRadio === "undefined" || !userPresetRadio)
            return
        userPresetRadio.checked = !saveToProject
        projectPresetRadio.checked = saveToProject
    }

    function syncPhPrinterRadios() {
        if (typeof phChangePresetRadio === "undefined" || !phChangePresetRadio)
            return
        phChangePresetRadio.checked = physicalPrinterAction === 0
        phAddPresetRadio.checked = physicalPrinterAction === 1
        phSwitchRadio.checked = physicalPrinterAction === 2
    }

    onAboutToShow: {
        resetDialogState()
        syncPhPrinterRadios()
    }

    // Radio indicator: mirrors CxCheckBox's 18px hit area + 15px stroked
    // shape, with the checked dot in Theme.accent (branded stand-in for the
    // upstream RadioGroup bitmaps).
    component SaveRadio: RadioButton {
        id: radioCtl
        font.pixelSize: Theme.fontSizeMD
        hoverEnabled: true
        indicator: Item {
            implicitWidth: 18
            implicitHeight: 18
            x: radioCtl.leftPadding
            y: (radioCtl.height - height) / 2
            opacity: radioCtl.enabled ? 1.0 : 0.45
            Rectangle {
                anchors.centerIn: parent
                width: 15
                height: 15
                radius: 7.5
                color: "transparent"
                border.color: Theme.borderDefault
                border.width: 1
            }
            Rectangle {
                anchors.centerIn: parent
                width: 7
                height: 7
                radius: 3.5
                color: radioCtl.checked ? Theme.accent : "transparent"
                Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
            }
        }
        contentItem: Text {
            leftPadding: radioCtl.indicator.width + radioCtl.spacing
            text: radioCtl.text
            color: radioCtl.enabled ? Theme.textPrimary : Theme.textDisabled
            font: radioCtl.font
            verticalAlignment: Text.AlignVCenter
        }
    }

    contentItem: Rectangle {
        color: Theme.bgPanel
        implicitHeight: contentColumn.implicitHeight + root.borderWidth

        ColumnLayout {
            id: contentColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: root.borderWidth
            anchors.rightMargin: root.borderWidth
            anchors.topMargin: root.borderWidth
            spacing: 0

            // Upstream cpp:56-58,100: single "Save <tab title> as" label
            // (Body_14) directly above the input.
            Text {
                text: qsTr("将%1另存为").arg(root.tierTitle(root.presetTier))
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                Layout.fillWidth: true
                Layout.leftMargin: root.borderWidth
                Layout.topMargin: root.borderWidth
                Layout.bottomMargin: root.borderWidth
                elide: Text.ElideRight
            }

            // Name input: fixed 360x24 (hpp:21 SAVE_PRESET_DIALOG_INPUT_SIZE,
            // cpp:89-90 SetMinSize=SetMaxSize); upstream has no "Name:" label.
            TextField {
                id: nameInput
                Layout.preferredWidth: 360
                Layout.preferredHeight: 24
                Layout.leftMargin: root.borderWidth
                Layout.rightMargin: root.borderWidth
                text: root.suggestedName
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                selectByMouse: true
                background: Rectangle {
                    color: Theme.bgInset
                    radius: 4
                    border.width: 1
                    border.color: nameInput.activeFocus ? Theme.accent : Theme.borderSubtle
                }
                onTextChanged: root.syncSaveRadios()  // cpp:88 update() on wxEVT_TEXT
            }

            // Validation label: directly below the input (upstream cpp:97-102
            // m_valid_label, foreground wxColour(255,111,0) -> statusWarning).
            Text {
                visible: text.length > 0  // cpp:255 Show(!info_line.IsEmpty())
                text: {
                    if (root.saveError.length > 0)
                        return root.saveError
                    switch (root.validationError) {
                    case 1:
                        return qsTr("Name is invalid;") + "\n"
                            + qsTr("illegal characters:") + " <>[]:/\\|?*\""
                    case 2:
                        return qsTr("Name is invalid;") + "\n"
                            + qsTr("illegal suffix:") + "\n\t" + " (modified)"
                    case 3:
                        return qsTr("Name is unavailable.")
                    case 4:
                        return qsTr("Overwriting a system profile is not allowed.")
                    case 5:
                        return qsTr("The name field is not allowed to be empty.")
                    case 6:
                        return qsTr("The name is not allowed to start with a space.")
                    case 7:
                        return qsTr("The name is not allowed to end with a space.")
                    case 8:
                        return qsTr("The name cannot be the same as a preset alias name.")
                    }
                    if (root.validationWarning === 1)
                        return qsTr("Preset \"%1\" already exists.\nPlease note that saving will overwrite the current preset.")
                            .arg(nameInput.text.trim())
                    if (root.validationWarning === 2)
                        return qsTr("Preset \"%1\" already exists and is incompatible with the current printer.\nPlease note that saving will overwrite the current preset.")
                            .arg(nameInput.text.trim())
                    return ""
                }
                color: Theme.statusWarning
                font.pixelSize: Theme.fontSizeSM
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.leftMargin: root.borderWidth
                Layout.rightMargin: root.borderWidth
            }

            // Printer tier: physical-printer info line (upstream cpp:400-404,
            // bold font).
            Text {
                visible: root.phPrinterVisible
                text: qsTr("打印机 \"%1\" 已选中，使用预设 \"%2\"")
                    .arg(root.configVm ? root.configVm.physicalPrinterSelectedName() : "")
                    .arg(root.configVm ? root.configVm.physicalPrinterSelectedPresetName() : "")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                font.bold: true
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                Layout.leftMargin: 3 * root.borderWidth  // cpp:422
                Layout.topMargin: 2 * root.borderWidth
            }

            // Printer tier: 3-action radio group in a static box (upstream
            // cpp:409-423 wxStaticBoxSizer).
            ColumnLayout {
                visible: root.phPrinterVisible
                spacing: 0
                Layout.fillWidth: true
                Layout.leftMargin: 3 * root.borderWidth  // cpp:423
                Layout.topMargin: 2 * root.borderWidth   // cpp:420

                Rectangle {
                    Layout.fillWidth: true
                    color: "transparent"
                    border.width: 1
                    border.color: Theme.borderSubtle
                    radius: Theme.radiusSM
                    implicitHeight: phRadioColumn.implicitHeight + 2 * Theme.spacingSM

                    ColumnLayout {
                        id: phRadioColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        anchors.margins: Theme.spacingSM
                        spacing: 5  // cpp:418 wxTOP 5

                        Text {
                            // cpp:439 box caption, refreshed with the input name
                            text: qsTr("保存后请为 \"%1\" 预设选择一个操作")
                                .arg(nameInput.text)
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                            font.bold: true
                            Layout.fillWidth: true
                        }
                        SaveRadio {
                            id: phChangePresetRadio
                            // cpp:442 (upstream label carries a trailing space)
                            text: qsTr("对 \"%1\"，将 \"%2\" 更改为 \"%3\" ")
                                .arg(root.configVm ? root.configVm.physicalPrinterSelectedName() : "")
                                .arg(root.configVm ? root.configVm.physicalPrinterSelectedPresetName() : "")
                                .arg(nameInput.text)
                            onToggled: root.physicalPrinterAction = 0
                        }
                        SaveRadio {
                            id: phAddPresetRadio
                            text: qsTr("对 \"%1\"，将 \"%2\" 添加为新预设")
                                .arg(root.configVm ? root.configVm.physicalPrinterSelectedName() : "")
                                .arg(nameInput.text)
                            onToggled: root.physicalPrinterAction = 1
                        }
                        SaveRadio {
                            id: phSwitchRadio
                            text: qsTr("仅切换到 \"%1\"").arg(nameInput.text)
                            onToggled: root.physicalPrinterAction = 2
                        }
                    }
                }
            }

            // Save-destination radio group (upstream cpp:107-112 vertical
            // RadioGroup; both items disabled together on an existing preset,
            // cpp:248/250).
            ColumnLayout {
                spacing: 5
                Layout.fillWidth: true
                Layout.leftMargin: root.borderWidth  // cpp:112 wxLEFT
                Layout.topMargin: root.borderWidth   // cpp:112 wxTOP

                SaveRadio {
                    id: userPresetRadio
                    text: qsTr("用户预设")  // upstream _L("User Preset")
                    enabled: !root.nameHitsExisting
                    onToggled: {
                        if (root.configVm && !root.nameHitsExisting)
                            root.configVm.saveToProjectSelection = checked
                    }
                }
                SaveRadio {
                    id: projectPresetRadio
                    text: qsTr("项目内预设")  // upstream _L("Preset Inside Project")
                    enabled: !root.nameHitsExisting
                    onToggled: {
                        if (root.configVm && !root.nameHitsExisting)
                            root.configVm.saveToProjectSelection = checked
                    }
                }
            }

            // Detach block (upstream cpp:114-161, built when mode==comDevelop;
            // the OWzx app state corresponds to the develop mode, so the
            // block is always present).
            RowLayout {
                id: detachRow
                spacing: 5  // cpp:132 FromDIP(5)
                Layout.fillWidth: true
                Layout.leftMargin: root.borderWidth  // cpp:131 wxLEFT
                Layout.topMargin: root.borderWidth   // cpp:133 wxTOP

                // cpp:121 detach tooltip
                readonly property string detachTooltip: qsTr("将父级继承的所有值复制到该预设中，并移除父级关系。仅与父级兼容的预设可能变为不受支持。")

                CxCheckBox {
                    id: detachCheck
                    checked: false
                    hoverEnabled: true
                    ToolTip.delay: 500
                    ToolTip.text: detachRow.detachTooltip
                    ToolTip.visible: hovered
                }
                Text {
                    // cpp:126 two-state label
                    text: root.hasDetachParent ? qsTr("脱离父级") : qsTr("无父级保存")
                    color: Theme.textPrimary  // upstream #363636 -> neutral token
                    font.pixelSize: Theme.fontSizeMD
                    Layout.fillWidth: true
                    ToolTip.delay: 500
                    ToolTip.text: detachRow.detachTooltip
                    ToolTip.visible: detachLabelHover.containsMouse
                    MouseArea {
                        id: detachLabelHover
                        anchors.fill: parent
                        hoverEnabled: true
                        // cpp:159-160: clicking the label flips the checkbox
                        onClicked: detachCheck.toggle()
                    }
                }
            }

            // Upstream cpp:134 FromDIP(5) spacer
            Item { Layout.fillWidth: true; Layout.preferredHeight: 5 }

            // Parent info line (upstream cpp:136-141: parent preset name or
            // "Unique preset", Body_12, indented BORDER_W + FromDIP(24)).
            Text {
                text: root.hasDetachParent ? root.detachParentName : qsTr("独立预设")
                color: Theme.textMuted  // upstream #6B6B6B -> neutral token
                font.pixelSize: Theme.fontSizeSM
                elide: Text.ElideRight
                Layout.fillWidth: true
                Layout.leftMargin: root.borderWidth + 24
                ToolTip.delay: 500
                ToolTip.text: root.hasDetachParent
                    ? qsTr("父级预设")
                    : qsTr("该预设不继承自其他预设。")
                ToolTip.visible: parentHover.containsMouse
                MouseArea {
                    id: parentHover
                    anchors.fill: parent
                    hoverEnabled: true
                }
            }

            // Upstream cpp:143 FromDIP(5) spacer
            Item { Layout.fillWidth: true; Layout.preferredHeight: 5 }

            // Button row: upstream DialogButtons(this,{"OK","Cancel"})
            // (cpp:326); the zero left_aligned_buttons_count default lays out
            // [gap][stretch][OK][Cancel] right-aligned with OK styled primary
            // via SetPrimaryButton (DialogButtons.cpp:130-153).
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: root.borderWidth
                spacing: root.borderWidth
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("确定")  // upstream "OK"
                    enabled: root.category >= 0 && root.validationError === 0
                    cxStyle: CxButton.Style.Primary
                    onClicked: {
                        root.saveError = ""
                        if (!root.configVm) {
                            root.saveError = qsTr("Preset service is unavailable.")
                            return
                        }
                        var name = nameInput.text.trim()
                        var category = root.tierToCategory(root.presetTier)
                        if (category < 0) {
                            root.saveError = qsTr("Unsupported preset category.")
                            return
                        }
                        // R-P1.J + G-01: three save routes mirroring upstream
                        // SavePresetDialog -- overwrite the CURRENT preset
                        // (save-in-place), replace an existing USER preset
                        // (warned replace), or create a NEW custom preset.
                        var ok
                        if (name === root.currentPresetNameForTier()) {
                            // G-13: Detach flattens the inherited preset with
                            // the current edits; otherwise the normal
                            // save-in-place runs.
                            ok = detachCheck.checked
                                ? root.configVm.detachPresetFromParent(category, name)
                                : root.configVm.saveCurrentPreset()
                        } else if (root.isOverwriteTarget(name)) {
                            ok = root.configVm.overwriteUserPreset(category, name)
                        } else {
                            ok = root.configVm.createCustomPreset(category, name)
                        }
                        // Keep the dialog open when persistence rejects the save.
                        if (ok) {
                            root.accept()
                            // Upstream accept() (cpp:495-503): the printer
                            // tier applies the chosen physical-printer action
                            // on accept (update_physical_printers,
                            // cpp:472-493).
                            if (category === 2)
                                root.configVm.applyPhysicalPrinterAction(
                                    root.physicalPrinterAction, name)
                            // Upstream save_current_preset(name, detach,
                            // save_to_project): saving into the project marks
                            // the preset project-embedded
                            // (is_project_embedded).
                            if (root.configVm.saveToProjectSelection)
                                root.configVm.markPresetProjectEmbedded(name)
                        } else {
                            root.saveError = root.configVm.lastPresetError
                            if (root.saveError.length === 0)
                                root.saveError = qsTr("Failed to save the preset.")
                        }
                    }
                }
                CxButton {
                    text: qsTr("取消")  // upstream "Cancel"
                    onClicked: root.reject()
                }
            }
        }
    }
}
