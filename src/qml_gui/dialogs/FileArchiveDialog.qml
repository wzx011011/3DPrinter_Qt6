import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// FileArchiveDialog.qml — Phase 236 (DLG-03) archive import (zip) tree view.
//
// Upstream: OrcaSlicer/Creality Print open a file-tree dialog with checkboxes
// when a zip archive is imported (FileArchiveDialog) — the user picks which
// models inside the archive to import. OWzx enumerates the importable model
// entries (*.stl/*.obj/*.3mf/*.amf) via
// EditorViewModel::listArchiveEntries (miniz central directory, no
// extraction) and imports the checked entries through
// EditorViewModel::importArchiveEntries (extract to temp + normal loadFile).
//
// Usage:
//   FileArchiveDialog {
//       editorVm: backend.editorViewModel
//       // openFor(path) lists entries; onImportRequested carries the archive path.
//   }
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    closePolicy: Popup.NoAutoClose
    dialogTitle: qsTr("压缩包导入")
    width: 460
    height: 400
    padding: 0

    required property var editorVm

    property string archivePath: ""
    property var entries: []          // string list from listArchiveEntries
    property var checkedEntries: ({}) // entry name -> true

    signal importRequested(string archivePath, var selectedEntries)

    readonly property int checkedCount: {
        var count = 0
        for (var key in checkedEntries)
            if (checkedEntries[key])
                ++count
        return count
    }

    function openFor(path) {
        archivePath = path
        entries = editorVm ? editorVm.listArchiveEntries(path) : []
        // All entries checked by default (upstream default). Build a fresh
        // object so the var-property change signal fires.
        var initial = {}
        for (var i = 0; i < entries.length; ++i)
            initial[entries[i]] = true
        checkedEntries = initial
        open()
    }

    function selectedEntries() {
        var selected = []
        for (var i = 0; i < entries.length; ++i)
            if (checkedEntries[entries[i]])
                selected.push(entries[i])
        return selected
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingMD
        anchors.margins: Theme.spacingXL

        Text {
            Layout.fillWidth: true
            text: root.entries.length > 0
                ? qsTr("压缩包内发现 %1 个可导入的模型文件：").arg(root.entries.length)
                : qsTr("压缩包内没有可导入的模型文件（支持 STL/OBJ/3MF/AMF）。")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMD
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.bgInset
            radius: 4
            border.color: Theme.borderSubtle
            border.width: 1
            clip: true

            ListView {
                id: entryList
                anchors.fill: parent
                anchors.margins: Theme.spacingXS
                clip: true
                model: root.entries
                spacing: 2

                delegate: Rectangle {
                    required property string modelData
                    required property int index
                    width: entryList.width
                    height: 30
                    radius: 3
                    color: entryHover.containsMouse ? Theme.bgHover : "transparent"

                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: Theme.spacingSM
                        spacing: Theme.spacingSM
                        width: parent.width - Theme.spacingSM * 2

                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 14
                            height: 14
                            radius: 2
                            color: root.checkedEntries[modelData] ? Theme.accent : Theme.bgCard
                            border.color: root.checkedEntries[modelData] ? Theme.accent : Theme.borderInput
                            border.width: 1

                            Text {
                                anchors.centerIn: parent
                                visible: root.checkedEntries[modelData]
                                text: "✓"
                                color: Theme.textOnAccent
                                font.pixelSize: 10
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            width: parent.width - 20
                            text: modelData
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSM
                            elide: Text.ElideMiddle
                        }
                    }

                    HoverHandler { id: entryHover }
                    TapHandler {
                        onTapped: {
                            var next = {}
                            for (var key in root.checkedEntries)
                                next[key] = root.checkedEntries[key]
                            next[modelData] = !root.checkedEntries[modelData]
                            root.checkedEntries = next
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD

            Text {
                Layout.fillWidth: true
                text: qsTr("已选择 %1 / %2").arg(root.checkedCount).arg(root.entries.length)
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
            }

            CxButton {
                text: qsTr("取消")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }
            CxButton {
                text: qsTr("导入选中")
                cxStyle: CxButton.Style.Primary
                enabled: root.checkedCount > 0
                onClicked: {
                    root.importRequested(root.archivePath, root.selectedEntries())
                    root.accept()
                }
            }
        }
    }
}
