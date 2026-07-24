import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// KBShortcutsDialog.qml - keyboard shortcut overview dialog.
//
// Extracted from the inline Dialog in main.qml (Phase 195, UI-02) and
// reorganized as a 5-group view aligned with upstream OrcaSlicer
// KBShortcutsDialog.cpp (Global / Prepare / Toolbar / Objects List / Preview).
//
// Presentation only: this dialog documents the user-visible shortcuts. Some
// shortcuts (F/W/E/R/Space/arrows/PgUp/PgDn) are handled in C++ keyPressEvent
// handlers (EditorViewModel/GLViewport); the rest are QML Shortcut{} bindings
// in main.qml.

Dialog {
    id: root
    title: qsTr("Keyboard Shortcuts")
    modal: true
    anchors.centerIn: parent
    width: 560
    height: 480
    padding: 0

    // Group selector model aligned with upstream KBShortcutsDialog.cpp groups.
    readonly property var groups: [
        { id: "global", name: qsTr("Global") },
        { id: "prepare", name: qsTr("Prepare") },
        { id: "toolbar", name: qsTr("Toolbar") },
        { id: "objects", name: qsTr("Objects List") },
        { id: "preview", name: qsTr("Preview") }
    ]

    // Shortcut entries mapped to the upstream 5-group structure.
    // Global: app-wide actions bound via QML Shortcut{} in main.qml.
    readonly property var globalShortcuts: [
        { key: "Ctrl+Z", desc: qsTr("Undo") },
        { key: "Ctrl+Y", desc: qsTr("Redo") },
        { key: "Ctrl+Shift+Z", desc: qsTr("Redo (alternate)") },
        { key: "Ctrl+I", desc: qsTr("Import model") },
        { key: "Ctrl+O", desc: qsTr("Open project") },
        { key: "Ctrl+S", desc: qsTr("Save project") },
        { key: "Ctrl+P", desc: qsTr("Preferences") }
    ]
    // Prepare: canvas interaction (handled in C++ keyPressEvent).
    readonly property var prepareShortcuts: [
        { key: "F", desc: qsTr("Fit view") },
        { key: "W", desc: qsTr("Move mode") },
        { key: "E", desc: qsTr("Rotate mode") },
        { key: "R", desc: qsTr("Scale mode") },
        { key: "Escape", desc: qsTr("Deselect / cancel gizmo") }
    ]
    // Toolbar: preset camera views (handled in C++ keyPressEvent).
    readonly property var toolbarShortcuts: [
        { key: "Ctrl+1", desc: qsTr("Top view") },
        { key: "Ctrl+3", desc: qsTr("Right view") },
        { key: "Ctrl+6", desc: qsTr("Isometric view") },
        { key: "Ctrl+0", desc: qsTr("Front view") }
    ]
    // Objects List: selection/edit actions bound via QML Shortcut{} in main.qml.
    readonly property var objectsShortcuts: [
        { key: "Ctrl+A", desc: qsTr("Select all") },
        { key: "Ctrl+X", desc: qsTr("Cut selection") },
        { key: "Ctrl+C", desc: qsTr("Copy selection") },
        { key: "Ctrl+V", desc: qsTr("Paste") },
        { key: "Ctrl+D", desc: qsTr("Duplicate selection") },
        { key: "Ctrl+K", desc: qsTr("Duplicate (alternate)") },
        { key: "Delete", desc: qsTr("Delete selection") }
    ]
    // Preview: playback and layer navigation (handled in C++ keyPressEvent).
    readonly property var previewShortcuts: [
        { key: "Space", desc: qsTr("Play / pause preview animation") },
        { key: "Left / Right", desc: qsTr("Step move +/-100") },
        { key: "Home / End", desc: qsTr("Jump to start / end") },
        { key: "PgUp / PgDn", desc: qsTr("Layer range +/-1") },
        { key: "Shift+PgUp/Dn", desc: qsTr("Layer range +/-10") }
    ]

    property string currentGroup: "global"

    function shortcutsForGroup(groupId) {
        if (groupId === "global") return root.globalShortcuts
        if (groupId === "prepare") return root.prepareShortcuts
        if (groupId === "toolbar") return root.toolbarShortcuts
        if (groupId === "objects") return root.objectsShortcuts
        if (groupId === "preview") return root.previewShortcuts
        return []
    }

    background: Rectangle {
        color: Theme.bgPanel
        border.width: 1
        border.color: Theme.borderDefault
        radius: Theme.radiusMD
    }

    contentItem: Item {
        implicitWidth: 560
        implicitHeight: 480

        // Header
        Text {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 20
            text: root.title
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeLG
            font.bold: true
        }

        RowLayout {
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: footerRow.top
            anchors.margins: 16
            spacing: 12

            // Left group selector
            ColumnLayout {
                Layout.preferredWidth: 140
                Layout.fillHeight: true
                spacing: 4

                Repeater {
                    model: root.groups
                    delegate: Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 34
                        radius: Theme.radiusSM
                        color: root.currentGroup === modelData.id ? Theme.accent : Theme.bgElevated
                        opacity: groupMouse.containsMouse && root.currentGroup !== modelData.id ? 0.8 : 1.0
                        border.width: root.currentGroup === modelData.id ? 0 : 1
                        border.color: Theme.borderSubtle

                        Text {
                            anchors.centerIn: parent
                            text: modelData.name
                            color: root.currentGroup === modelData.id ? Theme.textOnAccent : Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSM
                            font.bold: root.currentGroup === modelData.id
                        }

                        MouseArea {
                            id: groupMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.currentGroup = modelData.id
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }

            // Right content: shortcut list for the selected group
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth

                Column {
                    width: parent.width
                    spacing: 4

                    Repeater {
                        model: root.shortcutsForGroup(root.currentGroup)
                        delegate: RowLayout {
                            width: parent.width
                            spacing: 14

                            Rectangle {
                                Layout.preferredWidth: 96
                                Layout.preferredHeight: 24
                                radius: 4
                                color: Theme.bgTooltip
                                border.width: 1
                                border.color: Theme.borderSubtle

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.key
                                    color: Theme.accent
                                    font.pixelSize: Theme.fontSizeSM
                                    font.bold: true
                                    font.family: Theme.fontMono
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: modelData.desc
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeMD
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                }
            }
        }

        // Footer close button
        RowLayout {
            id: footerRow
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 16
            spacing: 8

            CxButton {
                text: qsTr("Close")
                compact: true
                implicitWidth: 80
                onClicked: root.close()
            }
        }
    }
}
