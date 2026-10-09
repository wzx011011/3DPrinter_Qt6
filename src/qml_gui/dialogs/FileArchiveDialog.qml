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
// models inside the archive to import. Paths are grouped into folder nodes
// (upstream builds the tree by walking each entry's parent path,
// FileArchiveDialog.cpp:204-252) with every folder expanded by default
// (AddFile → Expand, :43). OWzx enumerates the importable model entries
// (*.stl/*.obj/*.3mf/*.amf/*.step/*.stp) via
// EditorViewModel::listArchiveEntries (miniz central directory, no
// extraction) and imports the checked entries through
// EditorViewModel::importArchiveEntries (extract to temp + normal loadFile).
//
// Usage:
//   FileArchiveDialog {
//       editorVm: backend.editorViewModel
//       // openFor(path) lists entries; onImportRequested carries the archive path.
//   }
//
// Registered deviation: upstream is resizable/maximizable
// (wxRESIZE_BORDER|wxMAXIMIZE_BOX, min 400×300 — FileArchiveDialog.cpp:173-175/:263);
// QML Popup has no native resize affordance, so the dialog keeps a fixed 450×400.
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: true
    // Upstream runs as a modal wx dialog — Esc cancels, outside presses are
    // blocked by modality (FileArchiveDialog.cpp:173-175 wxDEFAULT_DIALOG_STYLE).
    closePolicy: Popup.CloseOnEscape
    dialogTitle: qsTr("压缩包导入")
    width: 450   // upstream 45 * em_unit() (FileArchiveDialog.cpp:174)
    height: 400
    padding: 0

    required property var editorVm

    property string archivePath: ""
    property var entries: []           // flat importable file list from listArchiveEntries (full archive paths)
    property var checkedEntries: ({})  // file path -> true
    property var folderChecked: ({})   // folder path -> bool (upstream keeps a toggle on folder nodes too)
    property var folderExpanded: ({})  // folder path -> bool (default true, upstream AddFile expands)
    property var treeNodes: []         // flattened display model: {name, path, depth, isFolder}

    signal importRequested(string archivePath, var selectedEntries)

    readonly property int checkedCount: {
        var count = 0
        for (var key in checkedEntries)
            if (checkedEntries[key])
                ++count
        return count
    }

    function isFolderExpanded(path) {
        return folderExpanded[path] !== undefined ? folderExpanded[path] : true
    }

    // Collect every folder prefix of the archive entries that sits below
    // (and including) the given folder path.
    function collectFoldersUnder(path, list) {
        for (var i = 0; i < entries.length; ++i) {
            var parts = entries[i].split("/")
            for (var d = 0; d < parts.length - 1; ++d) {
                var folderPath = parts.slice(0, d + 1).join("/")
                if ((path === "" || folderPath.indexOf(path + "/") === 0) && list.indexOf(folderPath) < 0)
                    list.push(folderPath)
            }
        }
    }

    // Upstream ArchiveViewModel::SetValue (FileArchiveDialog.cpp:116-133):
    // toggling a folder applies the same value to every descendant; unchecking
    // any node untoggles its whole ancestor folder chain (untoggle_folders, :107-114).
    function setEntryChecked(path, isFolder, checked) {
        var nextFiles = {}
        for (var key in checkedEntries)
            nextFiles[key] = checkedEntries[key]
        var nextFolders = {}
        for (var folderKey in folderChecked)
            nextFolders[folderKey] = folderChecked[folderKey]

        if (isFolder) {
            nextFolders[path] = checked
            var prefix = path + "/"
            for (var i = 0; i < entries.length; ++i)
                if (entries[i].indexOf(prefix) === 0)
                    nextFiles[entries[i]] = checked
            var descendants = []
            collectFoldersUnder(path, descendants)
            for (var j = 0; j < descendants.length; ++j)
                nextFolders[descendants[j]] = checked
        } else {
            nextFiles[path] = checked
        }

        if (!checked) {
            var parts = path.split("/")
            for (var d = 0; d < parts.length - 1; ++d)
                nextFolders[parts.slice(0, d + 1).join("/")] = false
        }

        checkedEntries = nextFiles
        folderChecked = nextFolders
    }

    // Upstream on_all_button / on_none_button (FileArchiveDialog.cpp:312-365)
    // deep-toggle every node in the tree.
    function setAllChecked(checked) {
        var nextFiles = {}
        var nextFolders = {}
        for (var i = 0; i < entries.length; ++i) {
            nextFiles[entries[i]] = checked
            var parts = entries[i].split("/")
            for (var d = 0; d < parts.length - 1; ++d)
                nextFolders[parts.slice(0, d + 1).join("/")] = checked
        }
        checkedEntries = nextFiles
        folderChecked = nextFolders
    }

    function toggleFolderExpanded(path) {
        var next = {}
        for (var key in folderExpanded)
            next[key] = folderExpanded[key]
        next[path] = !root.isFolderExpanded(path)
        folderExpanded = next
        rebuildTree()
    }

    // Build folder hierarchy from the sorted paths (upstream sorts entries and
    // walks the common-parent stack, FileArchiveDialog.cpp:234-252), then
    // flatten with expanded folders interleaved in tree order.
    function rebuildTree() {
        var folderIndex = {}
        var roots = []
        var sorted = entries.slice().sort()
        for (var i = 0; i < sorted.length; ++i) {
            var parts = sorted[i].split("/")
            var siblings = roots
            for (var d = 0; d < parts.length - 1; ++d) {
                var folderPath = parts.slice(0, d + 1).join("/")
                var folder = folderIndex[folderPath]
                if (!folder) {
                    folder = { name: parts[d], path: folderPath, depth: d, isFolder: true, children: [] }
                    folderIndex[folderPath] = folder
                    siblings.push(folder)
                }
                siblings = folder.children
            }
            siblings.push({ name: parts[parts.length - 1], path: sorted[i], depth: parts.length - 1, isFolder: false })
        }
        var flat = []
        var walk = function (list) {
            for (var j = 0; j < list.length; ++j) {
                flat.push(list[j])
                if (list[j].isFolder && root.isFolderExpanded(list[j].path))
                    walk(list[j].children)
            }
        }
        walk(roots)
        treeNodes = flat
    }

    function openFor(path) {
        archivePath = path
        entries = editorVm ? editorVm.listArchiveEntries(path) : []
        // Upstream default: nothing checked (ArchiveViewNode toggle defaults to
        // false); a single importable entry is all-checked via on_all_button
        // (FileArchiveDialog.cpp:253-254). Fresh objects so the var-property
        // change signals fire.
        checkedEntries = {}
        folderChecked = {}
        folderExpanded = {}
        rebuildTree()
        if (entries.length === 1)
            setAllChecked(true)
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
        anchors.fill: parent
        // Upstream places the tree and the button row with wxALL 10
        // (FileArchiveDialog.cpp:260-261).
        anchors.margins: Theme.spacingMD

        Text {
            Layout.fillWidth: true
            text: root.entries.length > 0
                ? qsTr("压缩包内发现 %1 个可导入的模型文件：").arg(root.entries.length)
                : qsTr("压缩包内没有可导入的模型文件（支持 STL/OBJ/3MF/AMF/STEP）。")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeMD
            wrapMode: Text.WordWrap
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.bgInset
            radius: Theme.radiusSM
            border.color: Theme.borderSubtle
            border.width: 1
            clip: true

            ListView {
                id: entryList
                anchors.fill: parent
                anchors.margins: Theme.spacingXS
                clip: true
                model: root.treeNodes
                spacing: Theme.spacingXXS

                delegate: Rectangle {
                    id: entryRow
                    required property var modelData
                    required property int index
                    width: entryList.width
                    height: 30
                    radius: Theme.radiusSM
                    color: rowHover.containsMouse ? Theme.bgHover : "transparent"

                    readonly property bool isChecked: modelData.isFolder
                        ? root.folderChecked[modelData.path] === true
                        : root.checkedEntries[modelData.path] === true

                    MouseArea {
                        id: rowArea
                        anchors.fill: parent
                        onClicked: {
                            if (entryRow.modelData.isFolder)
                                root.toggleFolderExpanded(entryRow.modelData.path)
                            else
                                root.setEntryChecked(entryRow.modelData.path, false, !entryRow.isChecked)
                        }
                    }

                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        // Depth indentation mirrors the upstream tree hierarchy.
                        anchors.leftMargin: Theme.spacingSM + entryRow.modelData.depth * 16
                        spacing: Theme.spacingSM
                        width: parent.width - (Theme.spacingSM + entryRow.modelData.depth * 16) - Theme.spacingSM

                        Item {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 18
                            height: 14

                            Rectangle {
                                anchors.centerIn: parent
                                width: 14
                                height: 14
                                radius: Theme.radiusXS
                                color: entryRow.isChecked ? Theme.accent : Theme.bgCard
                                border.color: entryRow.isChecked ? Theme.accent : Theme.borderInput
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    visible: entryRow.isChecked
                                    text: "✓"
                                    color: Theme.textOnAccent
                                    font.pixelSize: 10
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -3
                                onClicked: root.setEntryChecked(entryRow.modelData.path,
                                                                entryRow.modelData.isFolder,
                                                                !entryRow.isChecked)
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            visible: entryRow.modelData.isFolder
                            width: visible ? implicitWidth : 0
                            text: root.isFolderExpanded(entryRow.modelData.path) ? "▾" : "▸"
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeXS
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            width: Math.max(0, parent.width - x - Theme.spacingSM)
                            text: entryRow.modelData.name
                            color: entryRow.modelData.isFolder ? Theme.textPrimary : Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSM
                            elide: Text.ElideMiddle
                        }
                    }

                    HoverHandler { id: rowHover }
                }

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD

            // Upstream footer starts with All/None (FileArchiveDialog.cpp:368-401).
            CxButton {
                text: qsTr("全部")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.setAllChecked(true)
            }
            CxButton {
                text: qsTr("无")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.setAllChecked(false)
            }

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
