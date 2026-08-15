import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// UnsavedChangesDialog.qml — PRESET-02 切换 preset 时的 diff 守卫
//
// 上游: third_party/OrcaSlicer/src/slic3r/GUI/UnsavedChangesDialog.cpp (95KB)
//   - DiffModel 树形对比 preset A vs B
//   - 按钮 Save/Transfer/Discard/Cancel
//   - 逐项勾选（选中才 Save/Transfer，未勾选项回退）
//
// OWzx 实现:
//   - 模态对话框（单实例由 ConfigViewModel::beginUnsavedDialog 守卫，
//     PSET2-03 修复三个 SettingsDialog 共享 configVm 导致的三连弹窗）
//   - 参数 diff 列表（平铺 + 每行 checkbox，默认全选）
//   - 按钮 [转移到新预设][丢弃修改][保存为预设...][取消]
//   - 接 ConfigViewModel.globalModified* / transferPendingChanges API
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    dialogTitle: qsTr("未保存的修改")
    width: 560
    height: 420
    padding: 0

    required property var configVm
    /// 当前 preset tier（用于显示）
    property string presetTier: ""
    /// 用户选择的动作: "save" / "transfer" / "discard" / "cancel"
    property string action: "cancel"
    /// v5.16 (PSET2-03): per-item selection — the checked keys are the ones
    /// Save/Transfer act on (upstream checkbox column, default all checked).
    property var checkedKeys: []
    /// Transfer only applies when the pending action is a preset switch
    /// (upstream shows Transfer for preset switches, Keep/Discard otherwise).
    readonly property bool transferAvailable: root.configVm
        && root.configVm.pendingUnsavedAction
        && root.configVm.pendingUnsavedAction.indexOf("switch-") === 0

    function openDialog() {
        root.action = "cancel"
        var keys = []
        var count = root.configVm ? root.configVm.globalModifiedCount : 0
        for (var i = 0; i < count; ++i)
            keys.push(root.configVm.globalModifiedKey(i))
        root.checkedKeys = keys
        root.open()
    }

    function toggleKey(key, checked) {
        var keys = root.checkedKeys.slice()
        var idx = keys.indexOf(key)
        if (checked && idx < 0)
            keys.push(key)
        else if (!checked && idx >= 0)
            keys.splice(idx, 1)
        root.checkedKeys = keys
    }

    contentItem: Rectangle {
        color: Theme.bgPanel
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingXXL
            spacing: Theme.spacingLG
            // 标题说明
            Text {
                Layout.fillWidth: true
                text: qsTr("当前预设已修改但未保存。切换前请选择如何处理这些修改：")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.WordWrap
            }

            // 修改的参数数量
            Text {
                text: qsTr("已修改 %1 个参数，已勾选 %2 个").arg(root.configVm ? root.configVm.globalModifiedCount : 0).arg(root.checkedKeys.length)
                color: Theme.statusWarning
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
            }

            // diff 列表
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.bgInset
                radius: 6
                border.width: 1
                border.color: Theme.borderSubtle

                // 滚动区
                Flickable {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingMD
                    contentHeight: diffCol.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    ColumnLayout {
                        id: diffCol
                        width: parent.width
                        spacing: Theme.spacingXS
                        Repeater {
                            model: root.configVm ? root.configVm.globalModifiedCount : 0
                            delegate: RowLayout {
                                id: diffRow
                                readonly property string keyName: root.configVm
                                    ? root.configVm.globalModifiedKey(index) : ""

                                Layout.fillWidth: true
                                spacing: Theme.spacingMD

                                // v5.16 (PSET2-03): per-item checkbox
                                // (default checked — upstream selection column).
                                CxCheckBox {
                                    checked: diffRow.keyName !== ""
                                        && root.checkedKeys.indexOf(diffRow.keyName) >= 0
                                    onToggled: root.toggleKey(diffRow.keyName, checked)
                                }
                                Text {
                                    text: diffRow.keyName
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSizeXS
                                    font.family: "monospace"
                                    Layout.preferredWidth: 150
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: root.configVm ? root.configVm.globalModifiedDefaultValue(diffRow.keyName) : ""
                                    color: Theme.textTertiary
                                    font.pixelSize: Theme.fontSizeXS
                                    font.family: "monospace"
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: "→"
                                    color: Theme.textTertiary
                                    font.pixelSize: Theme.fontSizeXS
                                }
                                Text {
                                    text: root.configVm ? root.configVm.globalModifiedCurrentValue(diffRow.keyName) : ""
                                    color: Theme.accent
                                    font.pixelSize: Theme.fontSizeXS
                                    font.family: "monospace"
                                    Layout.fillWidth: true
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }

            // 按钮区
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                CxButton {
                    text: qsTr("取消")
                    onClicked: { root.action = "cancel"; root.reject() }
                }
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("丢弃修改")
                    onClicked: { root.action = "discard"; root.accept() }
                }
                // v5.16 (PSET2-03): Transfer — apply the checked keys onto
                // the target preset without saving the source (upstream
                // Action::Transfer, UnsavedChangesDialog.cpp:1008-1011).
                CxButton {
                    text: qsTr("转移到新预设")
                    visible: root.transferAvailable
                    enabled: root.checkedKeys.length > 0
                    cxStyle: CxButton.Style.Primary
                    onClicked: { root.action = "transfer"; root.accept() }
                }
                CxButton {
                    text: qsTr("保存为预设...")
                    onClicked: { root.action = "save"; root.accept() }
                }
            }
        }
    }
}
