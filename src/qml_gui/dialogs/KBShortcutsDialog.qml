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
    // Global: app-wide actions bound via QML Shortcut{} in main.qml
    // (upstream global list, KBShortcutsDialog.cpp:173-215).
    readonly property var globalShortcuts: [
        { key: "Ctrl+Z", desc: qsTr("Undo") },
        { key: "Ctrl+Y", desc: qsTr("Redo") },
        { key: "Ctrl+Shift+Z", desc: qsTr("Redo (alternate)") },
        { key: "Ctrl+N", desc: qsTr("New Project") },
        { key: "Ctrl+O", desc: qsTr("Open Project") },
        { key: "Ctrl+S", desc: qsTr("Save Project") },
        { key: "Ctrl+Shift+S", desc: qsTr("Save Project as") },
        { key: "Ctrl+I", desc: qsTr("Import geometry data from STL/STEP/3MF/OBJ/AMF files") },
        { key: "Ctrl+G", desc: qsTr("Export plate sliced file") },
        { key: "Ctrl+R", desc: qsTr("Slice plate") },
        { key: "Ctrl+Shift+G", desc: qsTr("Print plate") },
        { key: "Ctrl+X", desc: qsTr("Cut") },
        { key: "Ctrl+C", desc: qsTr("Copy to clipboard") },
        { key: "Ctrl+V", desc: qsTr("Paste from clipboard") },
        { key: "Ctrl+P", desc: qsTr("Preferences") },
        { key: "Del", desc: qsTr("Delete selected") }
    ]
    // GAP-4 (HOTKEYS-UPSTREAM-ALIGN): the plater canvas keys now mirror the
    // upstream table (KBShortcutsDialog.cpp:222-274) implemented in
    // PreparePage.qml Keys; OWzx extensions are listed explicitly.
    readonly property var prepareShortcuts: [
        { key: "M", desc: qsTr("Gizmo move") },
        { key: "S", desc: qsTr("Gizmo scale") },
        { key: "R", desc: qsTr("Gizmo rotate") },
        { key: "C", desc: qsTr("Gizmo cut") },
        { key: "B", desc: qsTr("Gizmo mesh boolean") },
        { key: "L", desc: qsTr("Gizmo SLA support points") },
        { key: "F", desc: qsTr("Gizmo Place face on bed") },
        { key: "P", desc: qsTr("Gizmo FDM paint-on seam") },
        { key: "T", desc: qsTr("Gizmo Text emboss / engrave") },
        { key: "U", desc: qsTr("Gizmo measure") },
        { key: "Y", desc: qsTr("Gizmo assemble") },
        { key: "A", desc: qsTr("Arrange all objects") },
        { key: "Shift+A", desc: qsTr("Arrange objects on selected plates") },
        { key: "Q", desc: qsTr("Auto orientate selected objects (or all)") },
        { key: "Shift+Q", desc: qsTr("Auto orientate objects on the active plate") },
        { key: "Shift+R", desc: qsTr("Auto orientate selected objects") },
        { key: "V", desc: qsTr("Toggle printable for selected object/part") },
        { key: "I", desc: qsTr("Zoom in") },
        { key: "O", desc: qsTr("Zoom out") },
        { key: "Tab", desc: qsTr("Switch between Prepare/Preview") },
        { key: "Shift+Tab", desc: qsTr("Collapse/Expand the sidebar") },
        { key: "1-9", desc: qsTr("Set filament for object/part (0 as second digit for 10..16)") },
        { key: "Shift+Left mouse", desc: qsTr("Select objects by rectangle") },
        { key: "Alt+Left mouse", desc: qsTr("Select a part") },
        { key: "Ctrl+Left mouse", desc: qsTr("Select multiple objects") },
        { key: "Arrow keys", desc: qsTr("Move selection 10 mm (Shift: 1 mm)") },
        { key: "Ctrl+U", desc: qsTr("Measure gizmo (OWzx extension)") }
    ]
    // Toolbar: preset camera views (Phase 237 VIEW-01, bound via QML
    // Shortcut{} in main.qml; upstream list at KBShortcutsDialog.cpp:247-253).
    readonly property var toolbarShortcuts: [
        { key: "Ctrl+0", desc: qsTr("Camera view - Default") },
        { key: "Ctrl+1", desc: qsTr("Camera view - Top") },
        { key: "Ctrl+2", desc: qsTr("Camera view - Bottom") },
        { key: "Ctrl+3", desc: qsTr("Camera view - Front") },
        { key: "Ctrl+4", desc: qsTr("Camera view - Behind") },
        { key: "Ctrl+5", desc: qsTr("Camera Angle - Left side") },
        { key: "Ctrl+6", desc: qsTr("Camera Angle - Right side") }
    ]
    // Objects List: selection/edit actions bound via QML Shortcut{} in main.qml.
    readonly property var objectsShortcuts: [
        { key: "Ctrl+A", desc: qsTr("Select all objects") },
        { key: "Ctrl+X", desc: qsTr("Cut selection") },
        { key: "Ctrl+C", desc: qsTr("Copy selection") },
        { key: "Ctrl+V", desc: qsTr("Paste") },
        { key: "Ctrl+D", desc: qsTr("Delete all") },
        { key: "Ctrl+K", desc: qsTr("Clone selected") },
        { key: "Delete", desc: qsTr("Delete selection") },
        { key: "Escape", desc: qsTr("Deselect all") }
    ]
    // Preview: playback and layer navigation (handled in C++ keyPressEvent).
    readonly property var previewShortcuts: [
        { key: "Space", desc: qsTr("Play / pause preview animation") },
        { key: "Left / Right", desc: qsTr("Step move +/-100") },
        { key: "Home / End", desc: qsTr("Jump to start / end") },
        { key: "PgUp / PgDn", desc: qsTr("Layer range +/-1") },
        { key: "Shift+PgUp/Dn", desc: qsTr("Layer range +/-10") },
        { key: "L", desc: qsTr("Toggle single-layer mode") },
        { key: "C", desc: qsTr("Toggle G-code window") }
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
