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
    property string suggestedName: configVm ? (configVm.activePresetTier === "print" ? configVm.currentPrintPreset
                                                : configVm.activePresetTier === "filament" ? configVm.currentFilamentPreset
                                                : configVm.currentPrinterPreset) + " (modified)" : ""

    /// 用户输入的名称（默认 = suggestedName）
    property string enteredName: suggestedName

    /// tier → category 索引（对齐 createCustomPreset 的 category 参数）
    /// 0=print, 1=filament, 2=printer
    function tierToCategory(tier) {
        if (tier === "print") return 0
        if (tier === "filament") return 1
        if (tier === "printer") return 2
        return 0
    }

    /// 校验：名称非空 + 不重名
    function isValidName() {
        var name = nameInput.text.trim()
        if (name.length === 0) return false
        // 重名校验（已有预设列表）
        if (!configVm) return true
        var existing = configVm.activePresetTier === "print" ? configVm.printPresetNames
                     : configVm.activePresetTier === "filament" ? configVm.filamentPresetNames
                     : configVm.printerPresetNames
        return existing.indexOf(name) < 0
    }

    contentItem: Rectangle {
        color: Theme.bgPanel
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            // Tier 标签
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
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
                spacing: 8
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

            // 重名/空名警告
            Text {
                visible: !root.isValidName() && nameInput.text.length > 0
                text: qsTr("A preset with this name already exists. Choose another name.")
                color: Theme.statusError
                font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Item { Layout.fillHeight: true }

            // 按钮区
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
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
                        if (!root.configVm) { root.reject(); return }
                        var name = nameInput.text.trim()
                        var category = root.tierToCategory(root.presetTier)
                        // 创建新预设（createCustomPreset 内部调 PresetService.savePresetValues）
                        root.configVm.createCustomPreset(category, name)
                        root.accept()
                    }
                }
            }
        }
    }
}
