import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// ExportPresetBundleDialog.qml — V21-02 PRESET-03 导出预设包
//
// 上游: third_party/OrcaSlicer/src/slic3r/GUI/ExportPresetBundleDialog.cpp (23KB)
//   - 多选预设 + 压缩导出 .zip/.bbscfg
//
// OWzx 实现 (v5.16 PSET2-04):
//   - 选目录 → 导出全部自定义预设为逐预设上游形状 JSON 文件
//     （<目录>/{printer,filament,process}/<名称>.json + index.json 清单；
//     PresetServiceMock::exportBundleIni。上游是打 zip 包，zip 打包暂缓，
//     目录树 + 清单为互操作单元，importBundleIni 两者皆可读）
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    dialogTitle: qsTr("导出预设包")
    width: 420
    height: 180
    padding: 0

    required property var configVm

    contentItem: Rectangle {
        color: Theme.bgPanel
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingXXL
            spacing: Theme.spacingLG
            Text {
                Layout.fillWidth: true
                text: qsTr("将当前所有自定义预设导出为可分享的预设包目录。")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.WordWrap
            }

            Text {
                text: qsTr("格式: 目录（printer/filament/process 逐预设 JSON + index.json，对齐上游用户预设格式）")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("取消")
                    onClicked: root.reject()
                }
                CxButton {
                    text: qsTr("选择目录...")
                    cxStyle: CxButton.Style.Primary
                    onClicked: exportFolderDialog.open()
                }
            }
        }
    }

    FolderDialog {
        id: exportFolderDialog
        title: qsTr("导出预设包")
        onAccepted: {
            // v5.16 (PSET2-04): per-preset upstream-shape JSON export
            // (configVm.exportBundleIni → PresetServiceMock::exportBundleIni).
            var path = selectedFolder.toString().replace("file:///", "")
            if (root.configVm && path.length > 0) {
                var count = root.configVm.exportBundleIni(path)
                console.log("[ExportPresetBundle] export to: " + path + " count=" + count)
            }
            root.accept()
        }
    }
}
