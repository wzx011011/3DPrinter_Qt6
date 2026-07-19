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
//
// OWzx 实现（简化）:
//   - 模态对话框
//   - 参数 diff 列表（平铺，对齐 globalModified* API）
//   - 按钮 [保存为预设][丢弃修改][取消]
//   - 接 ConfigViewModel.globalModified* API
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
    /// 用户选择的动作: "save" / "discard" / "cancel"
    property string action: "cancel"

    function openDialog() {
        root.action = "cancel"
        root.open()
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
                text: qsTr("已修改 %1 个参数").arg(root.configVm ? root.configVm.globalModifiedCount : 0)
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
                                Layout.fillWidth: true
                                spacing: Theme.spacingMD
                                Text {
                                    text: root.configVm ? root.configVm.globalModifiedKey(index) : ""
                                    color: Theme.textSecondary
                                    font.pixelSize: Theme.fontSizeXS
                                    font.family: "monospace"
                                    Layout.preferredWidth: 160
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: root.configVm ? root.configVm.globalModifiedDefaultValue(root.configVm.globalModifiedKey(index)) : ""
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
                                    text: root.configVm ? root.configVm.globalModifiedCurrentValue(root.configVm.globalModifiedKey(index)) : ""
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
                CxButton {
                    text: qsTr("保存为预设...")
                    cxStyle: CxButton.Style.Primary
                    onClicked: { root.action = "save"; root.accept() }
                }
            }
        }
    }
}
