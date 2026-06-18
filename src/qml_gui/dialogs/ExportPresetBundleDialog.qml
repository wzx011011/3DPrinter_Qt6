import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// ExportPresetBundleDialog.qml — V21-02 PRESET-03 导出预设包（简化版）
//
// 上游: third_party/OrcaSlicer/src/slic3r/GUI/ExportPresetBundleDialog.cpp (23KB)
//   - 多选预设 + 压缩导出 .zip/.bbscfg
//
// OWzx 简化版:
//   - 选路径 → 导出全部自定义预设（占位，真实导出需 PresetService 扩展）
//   - 复用 FileDialog SaveFile 模式
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    title: qsTr("导出预设包")
    width: 420
    height: 180
    padding: 0

    required property var configVm

    contentItem: Rectangle {
        color: Theme.bgPanel
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 12

            Text {
                Layout.fillWidth: true
                text: qsTr("将当前所有自定义预设导出为可分享的预设包文件。")
                color: Theme.textPrimary
                font.pixelSize: 12
                wrapMode: Text.WordWrap
            }

            Text {
                text: qsTr("格式: .zip（含 print/filament/printer 自定义预设）")
                color: Theme.textSecondary
                font.pixelSize: 10
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("取消")
                    onClicked: root.reject()
                }
                CxButton {
                    text: qsTr("选择路径...")
                    cxStyle: CxButton.Style.Primary
                    onClicked: exportFileDialog.open()
                }
            }
        }
    }

    FileDialog {
        id: exportFileDialog
        title: qsTr("导出预设包")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("预设包 (*.zip)")]
        defaultSuffix: "zip"
        onAccepted: {
            // V21-02: 真实导出需 PresetService.exportBundle(path) 扩展
            // 当前占位：记录路径（预设导出逻辑待 service 实现）
            console.log("[ExportPresetBundle] target: " + currentFile)
            root.accept()
        }
    }
}
