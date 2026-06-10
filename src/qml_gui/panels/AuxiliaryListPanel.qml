import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../controls"

// Auxiliary file list panel (对齐 upstream GUI_AuxiliaryList)
// Manages supplementary files attached to a print project.
// Features: toolbar (import, new folder, delete), file list, context menu,
// double-click to open, drag not yet implemented.
Item {
    id: root
    implicitWidth: 260
    implicitHeight: 360

    property string selectedPath: ""
    property bool selectedIsFolder: false

    // ── Context menu (对齐 upstream on_context_menu) ──
    CxMenu {
        id: auxContextMenu
        property string targetPath: ""
        property bool targetIsFolder: false

        CxMenuItem {
            text: qsTr("New Folder")
            visible: auxContextMenu.targetPath === ""
            onTriggered: newFolderDialog.open()
        }
        CxMenuItem {
            text: qsTr("Import File")
            visible: auxContextMenu.targetPath === "" || auxContextMenu.targetIsFolder
            onTriggered: importFileDialog.open()
        }
        CxMenuItem {
            text: qsTr("Open")
            visible: auxContextMenu.targetPath !== "" && !auxContextMenu.targetIsFolder
            onTriggered: backend.auxiliaryService.openFile(auxContextMenu.targetPath)
        }
        CxMenuItem {
            text: qsTr("Delete")
            visible: auxContextMenu.targetPath !== ""
            onTriggered: backend.auxiliaryService.deleteItem(auxContextMenu.targetPath)
        }
        CxMenuItem {
            text: qsTr("Rename")
            visible: auxContextMenu.targetPath !== ""
            onTriggered: {
                renameField.text = ""
                renameDialog.open()
            }
        }
    }

    // ── Toolbar (对齐 upstream m_if_btn, m_of_btn, m_del_btn) ──
    Row {
        id: toolbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 32
        spacing: 2

        CxIconButton {
            iconSource: "qrc:/qml/assets/icons/plus.svg"
            toolTipText: qsTr("Import File")
            iconSize: 14
            onClicked: importFileDialog.open()
        }
        CxIconButton {
            iconSource: "qrc:/qml/assets/icons/folder-plus.svg"
            toolTipText: qsTr("New Folder")
            iconSize: 14
            onClicked: newFolderDialog.open()
        }
        CxIconButton {
            iconSource: "qrc:/qml/assets/icons/trash.svg"
            toolTipText: qsTr("Delete Selected")
            iconSize: 14
            enabled: root.selectedPath !== ""
            onClicked: backend.auxiliaryService.deleteItem(root.selectedPath)
        }
    }

    // ── File list (对齐 upstream wxDataView tree) ──
    ListView {
        id: fileListView
        anchors.top: toolbar.bottom
        anchors.topMargin: 4
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        model: backend.auxiliaryService ? backend.auxiliaryService.files : []

        delegate: ItemDelegate {
            width: fileListView.width
            height: 30

            required property var modelData
            required property int index

            contentItem: Row {
                spacing: 6
                leftPadding: 6

                Text {
                    text: {
                        if (modelData.isFolder) return "\u{1F4C1}"  // folder emoji
                        if (modelData.icon === "image") return "\u{1F5BC}"
                        if (modelData.icon === "document") return "\u{1F4C4}"
                        return "\u{1F4CE}"
                    }
                    font.pixelSize: 13
                    verticalAlignment: Text.AlignVCenter
                }
                Text {
                    text: modelData.name
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                    width: parent.width - parent.leftPadding - parent.spacing - 20
                }
            }

            background: Rectangle {
                color: {
                    if (root.selectedPath === modelData.path) return Theme.accentSubtle
                    if (delMA.containsMouse) return Theme.bgHover
                    return "transparent"
                }
                Behavior on color { ColorAnimation { duration: 100 } }
            }

            onDoubleClicked: {
                if (!modelData.isFolder)
                    backend.auxiliaryService.openFile(modelData.path)
            }

            onClicked: {
                root.selectedPath = modelData.path
                root.selectedIsFolder = modelData.isFolder
            }

            MouseArea {
                id: delMA
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.RightButton | Qt.LeftButton
                onClicked: function(mouse) {
                    root.selectedPath = modelData.path
                    root.selectedIsFolder = modelData.isFolder
                    if (mouse.button === Qt.RightButton) {
                        auxContextMenu.targetPath = modelData.path
                        auxContextMenu.targetIsFolder = modelData.isFolder
                        auxContextMenu.popup()
                    }
                }
            }
        }

        // Empty state
        Label {
            anchors.centerIn: parent
            visible: fileListView.count === 0
            text: qsTr("No auxiliary files")
            color: Theme.textDisabled
            font.pixelSize: Theme.fontSizeSM
        }
    }

    // ── Right-click on empty space (对齐 upstream empty-area context menu) ──
    MouseArea {
        anchors.fill: parent
        z: -1
        acceptedButtons: Qt.RightButton
        onClicked: function(mouse) {
            auxContextMenu.targetPath = ""
            auxContextMenu.targetIsFolder = false
            auxContextMenu.popup()
        }
    }

    // ── Import file dialog (对齐 upstream wxFileDialog) ──
    FileDialog {
        id: importFileDialog
        title: qsTr("Import File")
        fileMode: FileDialog.OpenFiles
        onAccepted: {
            // files is a list of URLs
            const svc = backend.auxiliaryService
            if (!svc) return
            for (let i = 0; i < selectedFiles.length; i++) {
                const url = selectedFiles[i].toString()
                const localPath = url.startsWith("file:///") ?
                    url.substring(8) : url.replace("file://", "")
                svc.importFile(localPath)
            }
        }
    }

    // ── New folder dialog (对齐 upstream create_new_folder) ──
    CxDialog {
        id: newFolderDialog
        dialogTitle: qsTr("New Folder")
        width: 320

        Column {
            spacing: Theme.spacingMD
            width: parent.width

            CxTextField {
                id: folderNameField
                width: parent.width
                placeholderText: qsTr("Folder name")
            }

            Row {
                anchors.right: parent.right
                spacing: Theme.spacingSM

                CxButton {
                    text: qsTr("Cancel")
                    cxStyle: CxButton.Style.Ghost
                    onClicked: newFolderDialog.reject()
                }
                CxButton {
                    text: qsTr("Create")
                    onClicked: {
                        if (folderNameField.text.length > 0) {
                            backend.auxiliaryService.createFolder(folderNameField.text)
                            folderNameField.text = ""
                            newFolderDialog.accept()
                        }
                    }
                }
            }
        }
    }

    // ── Rename dialog (对齐 upstream on_editing_done) ──
    CxDialog {
        id: renameDialog
        dialogTitle: qsTr("Rename")
        width: 320

        property string targetPath: auxContextMenu.targetPath

        Column {
            spacing: Theme.spacingMD
            width: parent.width

            CxTextField {
                id: renameField
                width: parent.width
                placeholderText: qsTr("New name")
            }

            Row {
                anchors.right: parent.right
                spacing: Theme.spacingSM

                CxButton {
                    text: qsTr("Cancel")
                    cxStyle: CxButton.Style.Ghost
                    onClicked: renameDialog.reject()
                }
                CxButton {
                    text: qsTr("Rename")
                    onClicked: {
                        if (renameField.text.length > 0) {
                            backend.auxiliaryService.renameItem(
                                renameDialog.targetPath, renameField.text)
                            renameField.text = ""
                            renameDialog.accept()
                        }
                    }
                }
            }
        }
    }
}
