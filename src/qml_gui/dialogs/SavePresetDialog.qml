import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// SavePresetDialog.qml — PRESET-01 保存预设（对齐上游 SavePresetDialog）
//
// 上游: third_party/OrcaSlicer/src/slic3r/GUI/SavePresetDialog.cpp (18KB)
//   - DPIDialog 模态
//   - Item 列表（每个 Preset::Type 一个）
//   - 名称 ComboBox + 重名校验 + "Save to project" + Detach
//
// OWzx 实现（简化）:
//   - 模态对话框（基于 CxDialog）
//   - 名称输入（ComboBox 已有预设 + 可编辑新名）
//   - 重名校验（红色警告）
//   - 按钮 [Save] [Cancel]
//   - 接 ConfigViewModel.createCustomPreset（新名）/ saveCurrentPreset（覆盖当前）
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    dialogTitle: qsTr("另存为预设")
    width: 440
    height: 200
    padding: 0

    // 注入
    required property var configVm
    /// 当前 preset tier ("print"/"filament"/"printer")
    required property string presetTier
    /// 建议名称（默认 = 当前 preset 名 + "(modified)"）
    property string suggestedName: configVm ? (presetTier === "print" ? configVm.currentPrintPreset
                                                : presetTier === "filament" ? configVm.currentFilamentPreset
                                                : configVm.currentPrinterPreset) + " (modified)" : ""

    /// 用户输入的名称（默认 = suggestedName）
    property string enteredName: suggestedName
    property string saveError: ""

    /// tier → category 索引（对齐 createCustomPreset 的 category 参数）
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

    function userPresetNamesForTier(tier) {
        var category = root.tierToCategory(tier)
        if (!configVm || category < 0) return []
        return configVm.userPresetNamesForCategory(category)
    }

    function isExistingName(name) {
        if (!configVm) return false
        return root.presetNamesForTier(root.presetTier).indexOf(name) >= 0
    }

    /// G-01: an existing USER preset is replaceable (upstream warns and
    /// replaces, SavePresetDialog.cpp:216-222); builtin/vendor duplicates are
    /// not.
    function isOverwriteTarget(name) {
        return name.length > 0 && root.isExistingName(name)
            && root.userPresetNamesForTier(root.presetTier).indexOf(name) >= 0
    }

    /// G-13: current preset's parent (non-empty = inherited from a
    /// system/vendor preset, so the upstream "detach" option applies).
    function currentPresetParent() {
        return (configVm && currentPresetNameForTier().length > 0)
            ? configVm.presetInheritsParent(currentPresetNameForTier()) : ""
    }

    /// G-13 (upstream SavePresetDialog.cpp:191-232): name legality beyond
    /// duplicates — forbidden filesystem characters and the reserved
    /// "Default" prefix used by bundled presets.
    readonly property var illegalNameRe: /[<>:"\/\\|?*\u0000-\u001f]/
    function illegalNameError(name) {
        if (illegalNameRe.test(name))
            return qsTr("Preset name contains illegal characters.")
        if (name.indexOf("Default") === 0)
            return qsTr("'Default…' is a reserved name for bundled presets.")
        return ""
    }

    /// 校验：名称非空合法；重名仅允许覆盖当前预设（saveCurrentPreset）或既有用户
    /// 预设（overwriteUserPreset），内建/厂商重名仍拒绝
    function isValidName() {
        var name = nameInput.text.trim()
        if (name.length === 0 || root.tierToCategory(root.presetTier) < 0) return false
        // Duplicate validation uses this dialog's tier, not shared page state.
        if (!configVm) return false
        // G-13: filesystem-illegal characters and the reserved "Default"
        // prefix (upstream SavePresetDialog.cpp:191-232).
        if (root.illegalNameError(name) !== "") return false
        // R-P1.J: saving over the CURRENT preset's own name is the upstream
        // primary path (SavePresetDialog overwrite); the suggested name IS the
        // current preset name, so rejecting it made the suggested save
        // impossible.
        if (name === root.currentPresetNameForTier()) return true
        // G-01: overwriting another existing USER preset is the upstream
        // replace path (with a visible warning below).
        if (root.isExistingName(name))
            return root.isOverwriteTarget(name)
        return true
    }

    contentItem: Rectangle {
        color: Theme.bgPanel
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingXXL
            spacing: Theme.spacingLG
            // Tier 标签
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                Text {
                    text: qsTr("预设类型：")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                }
                Text {
                    text: {
                        if (root.presetTier === "print") return qsTr("打印")
                        if (root.presetTier === "filament") return qsTr("耗材")
                        if (root.presetTier === "printer") return qsTr("打印机")
                        return root.presetTier
                    }
                    color: Theme.accent
                    font.pixelSize: Theme.fontSizeMD
                    font.bold: true
                }
            }

            // 名称输入（ComboBox 可编辑）
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                Text {
                    text: qsTr("Name:")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    Layout.preferredWidth: 60
                }
                TextField {
                    id: nameInput
                    Layout.fillWidth: true
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
                    onTextEdited: root.enteredName = text
                    // 重名校验视觉反馈
                    Rectangle {
                        visible: !root.isValidName() && nameInput.text.length > 0
                        anchors.fill: parent
                        color: "transparent"
                        border.width: 1
                        border.color: Theme.statusError
                        radius: 4
                    }
                }
            }

            // G-13: Detach option -- applies when saving over the CURRENT
            // preset and that preset inherits from a system/vendor parent
            // (upstream SavePresetDialog.cpp:135-165 detach branch).
            CxCheckBox {
                id: detachFromParent
                visible: nameInput.text.trim() === root.currentPresetNameForTier()
                    && root.currentPresetParent().length > 0
                text: qsTr("脱离继承的系统预设（Detach）")
                font.pixelSize: Theme.fontSizeXS
            }

            // 重名/空名/非法名警告 + G-01 覆盖提示
            Text {
                readonly property string typedName: nameInput.text.trim()
                readonly property string illegalReason: typedName.length > 0
                    ? root.illegalNameError(typedName) : ""
                readonly property bool overwriteHint: root.isOverwriteTarget(typedName)
                readonly property bool blocked: (root.saveError.length > 0)
                                               || (illegalReason !== "")
                                               || (!root.isValidName() && typedName.length > 0)
                visible: blocked || overwriteHint
                text: {
                    if (root.saveError.length > 0)
                        return root.saveError
                    if (illegalReason !== "")
                        return illegalReason
                    if (overwriteHint)
                        return qsTr("A preset with this name already exists and will be replaced.")
                    return qsTr("A preset with this name already exists. Choose another name.")
                }
                color: blocked ? Theme.statusError : Theme.textSecondary
                font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Item { Layout.fillHeight: true }

            // 按钮区
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("取消")
                    onClicked: root.reject()
                }
                CxButton {
                    text: qsTr("保存")
                    enabled: root.isValidName()
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
                            ok = root.detachFromParent.checked
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
                        } else {
                            root.saveError = root.configVm.lastPresetError
                            if (root.saveError.length === 0)
                                root.saveError = qsTr("Failed to save the preset.")
                        }
                    }
                }
            }
        }
    }
}
