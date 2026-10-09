import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

// KBShortcutsDialog.qml - keyboard shortcut overview dialog.
//
// 1:1 skeleton with upstream OrcaSlicer KBShortcutsDialog.cpp: a 1px top
// separator line (cpp:33-36), a full-height left selector panel of 150x28
// square tab buttons (cpp:39-43, 116-126) and a scrolling shortcuts page on
// the right (cpp:58-64, 338-365). Content area is 1032x501 = 150 + 12 + 870
// wide and 1 + 500 high, centered on the parent (cpp:77-78).
//
// The 5 selector tabs mirror the upstream page order (cpp:45-49 bound via
// cpp:67-71): "Global" -> Global shortcuts, "Prepare" -> Plater, "Toolbar" ->
// Gizmo, "Objects list" -> Objects List, "Preview" -> Preview. The camera
// preset rows therefore live in the Prepare tab (upstream Plater page,
// cpp:242-248) and the Toolbar tab shows the gizmo keys (cpp:276-282).
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
    // Upstream content geometry (KBShortcutsDialog.cpp:53-64, Fit() +
    // CenterOnParent() at :77-78).
    width: 1032
    height: 501
    padding: 0

    // Group selector model aligned with the upstream tab labels
    // (KBShortcutsDialog.cpp:45-49); the ids keep the historical names.
    readonly property var groups: [
        { id: "global", name: qsTr("Global") },
        { id: "prepare", name: qsTr("Prepare") },
        { id: "toolbar", name: qsTr("Toolbar") },
        { id: "objects", name: qsTr("Objects list") },
        { id: "preview", name: qsTr("Preview") }
    ]

    // Shortcut entries mapped to the upstream pages.
    // Global: app-wide actions (upstream Global page, cpp:173-208); Undo/Redo
    // live on the Plater page upstream and are listed under Prepare below.
    readonly property var globalShortcuts: [
        { key: "Ctrl+N", desc: qsTr("New Project") },
        { key: "Ctrl+O", desc: qsTr("Open Project") },
        { key: "Ctrl+S", desc: qsTr("Save Project") },
        { key: "Ctrl+Shift+S", desc: qsTr("Save Project as") },
        { key: "Ctrl+Shift+E", desc: qsTr("Publish 3MF") },
        { key: "Ctrl+I", desc: qsTr("Import geometry data from STL/STEP/3MF/OBJ/AMF files") },
        { key: "Ctrl+G", desc: qsTr("Export plate sliced file") },
        { key: "Ctrl+R", desc: qsTr("Slice plate") },
        { key: "Ctrl+Shift+G", desc: qsTr("Print plate") },
        { key: "Ctrl+X", desc: qsTr("Cut") },
        { key: "Ctrl+C", desc: qsTr("Copy to clipboard") },
        { key: "Ctrl+V", desc: qsTr("Paste from clipboard") },
        { key: "Ctrl+P", desc: qsTr("Preferences") },
        { key: "Ctrl+M", desc: qsTr("Show/Hide 3Dconnexion devices settings dialog") },
        { key: "Ctrl+Tab", desc: qsTr("Switch table page") },
        { key: "Del", desc: qsTr("Delete selected") },
        { key: "?", desc: qsTr("Show keyboard shortcuts list") }
    ]
    // Prepare: the upstream Plater page (KBShortcutsDialog.cpp:219-273) as
    // implemented in PreparePage.qml Keys / RhiViewport mouse handling. The
    // mouse-action mapping reflects the default navigation style
    // (RhiViewport.cpp:1755-1763: left drag orbits, middle drag pans, wheel
    // zooms); Shift+R and the 2-digit filament form are OWzx extensions.
    readonly property var prepareShortcuts: [
        { key: "Left mouse button", desc: qsTr("Rotate View") },
        { key: "Middle mouse button", desc: qsTr("Pan View") },
        { key: "Right mouse button", desc: qsTr("None") },
        { key: "Mouse wheel", desc: qsTr("Zoom View") },
        { key: "A", desc: qsTr("Arrange all objects") },
        { key: "Shift+A", desc: qsTr("Arrange objects on selected plates") },
        { key: "Q", desc: qsTr("Auto orientate selected objects (or all)") },
        { key: "Shift+Q", desc: qsTr("Auto orientate objects on the active plate") },
        { key: "Shift+R", desc: qsTr("Auto orientate selected objects") },
        { key: "Shift+Tab", desc: qsTr("Collapse/Expand the sidebar") },
        { key: "Ctrl+Any arrow", desc: qsTr("Movement in camera space") },
        { key: "Alt+Left mouse", desc: qsTr("Select a part") },
        { key: "Ctrl+Left mouse", desc: qsTr("Select multiple objects") },
        { key: "Shift+Left mouse", desc: qsTr("Select objects by rectangle") },
        { key: "Arrow Up", desc: qsTr("Move selection 10mm in positive Y direction") },
        { key: "Arrow Down", desc: qsTr("Move selection 10mm in negative Y direction") },
        { key: "Arrow Left", desc: qsTr("Move selection 10mm in negative X direction") },
        { key: "Arrow Right", desc: qsTr("Move selection 10mm in positive X direction") },
        { key: "Shift+Any arrow", desc: qsTr("Movement step set to 1mm") },
        { key: "Esc", desc: qsTr("Deselect All") },
        { key: "1-9", desc: qsTr("Set filament for object/part (0 as second digit for 10..16)") },
        { key: "Ctrl+0", desc: qsTr("Camera view - Default") },
        { key: "Ctrl+1", desc: qsTr("Camera view - Top") },
        { key: "Ctrl+2", desc: qsTr("Camera view - Bottom") },
        { key: "Ctrl+3", desc: qsTr("Camera view - Front") },
        { key: "Ctrl+4", desc: qsTr("Camera view - Behind") },
        { key: "Ctrl+5", desc: qsTr("Camera Angle - Left side") },
        { key: "Ctrl+6", desc: qsTr("Camera Angle - Right side") },
        { key: "Ctrl+A", desc: qsTr("Select all objects") },
        { key: "Ctrl+D", desc: qsTr("Delete all") },
        { key: "Ctrl+Z", desc: qsTr("Undo") },
        { key: "Ctrl+Y", desc: qsTr("Redo") },
        { key: "M", desc: qsTr("Gizmo move") },
        { key: "R", desc: qsTr("Gizmo rotate") },
        { key: "S", desc: qsTr("Gizmo scale") },
        { key: "F", desc: qsTr("Gizmo Place face on bed") },
        { key: "C", desc: qsTr("Gizmo cut") },
        { key: "B", desc: qsTr("Gizmo mesh boolean") },
        { key: "H", desc: qsTr("Gizmo FDM paint-on fuzzy skin") },
        { key: "L", desc: qsTr("Gizmo SLA support points") },
        { key: "P", desc: qsTr("Gizmo FDM paint-on seam") },
        { key: "T", desc: qsTr("Gizmo Text emboss / engrave") },
        { key: "U", desc: qsTr("Gizmo measure") },
        { key: "Y", desc: qsTr("Gizmo assemble") },
        { key: "E", desc: qsTr("Gizmo brim ears") },
        { key: "I", desc: qsTr("Zoom in") },
        { key: "O", desc: qsTr("Zoom out") },
        { key: "V", desc: qsTr("Toggle printable for selected object/part") },
        { key: "Tab", desc: qsTr("Switch between Prepare/Preview") },
        { key: "Space", desc: qsTr("Open actions speed dial") }
    ]
    // Toolbar: the upstream Gizmo page (KBShortcutsDialog.cpp:276-282); the
    // third "Toolbar" tab shows the gizmo keys per the page binding order
    // (cpp:45-49 + cpp:67-71).
    readonly property var toolbarShortcuts: [
        { key: "Esc", desc: qsTr("Deselect All") },
        { key: "Shift+", desc: qsTr("Move: press to snap by 1mm") },
        { key: "Ctrl+Mouse wheel", desc: qsTr("Support/Color Painting: adjust pen radius") },
        { key: "Alt+Mouse wheel", desc: qsTr("Support/Color Painting: adjust section position") }
    ]
    // Objects List: selection/edit actions bound via QML Shortcut{} in
    // main.qml (upstream Objects List page, cpp:284-297).
    readonly property var objectsShortcuts: [
        { key: "1-9", desc: qsTr("Set extruder number for the objects and parts") },
        { key: "Delete", desc: qsTr("Delete selection") },
        { key: "Escape", desc: qsTr("Deselect all") },
        { key: "Ctrl+C", desc: qsTr("Copy selection") },
        { key: "Ctrl+V", desc: qsTr("Paste") },
        { key: "Ctrl+X", desc: qsTr("Cut selection") },
        { key: "Ctrl+A", desc: qsTr("Select all objects") },
        { key: "Ctrl+K", desc: qsTr("Clone selected") },
        { key: "Ctrl+Shift+P", desc: qsTr("Command palette") },
        { key: "Ctrl+Z", desc: qsTr("Undo") },
        { key: "Ctrl+Y", desc: qsTr("Redo") },
        { key: "Space", desc: qsTr("Select the object/part and press space to change the name") },
        { key: "Mouse click", desc: qsTr("Select the object/part and mouse click to change the name") }
    ]
    // Preview: slider navigation keys mirroring the upstream Preview page
    // (cpp:301-315); L/C are handled in the C++ keyPressEvent.
    readonly property var previewShortcuts: [
        { key: "Arrow Up", desc: qsTr("Vertical slider - Move active thumb Up") },
        { key: "Arrow Down", desc: qsTr("Vertical slider - Move active thumb Down") },
        { key: "Arrow Left", desc: qsTr("Horizontal slider - Move active thumb Left") },
        { key: "Arrow Right", desc: qsTr("Horizontal slider - Move active thumb Right") },
        { key: "L", desc: qsTr("Toggle single-layer mode") },
        { key: "C", desc: qsTr("Toggle G-code window") },
        { key: "Tab", desc: qsTr("Switch between Prepare/Preview") },
        { key: "Shift+Any arrow", desc: qsTr("Move slider 5x faster") },
        { key: "Shift+Mouse wheel", desc: qsTr("Move slider 5x faster") },
        { key: "Ctrl+Any arrow", desc: qsTr("Move slider 5x faster") },
        { key: "Ctrl+Mouse wheel", desc: qsTr("Move slider 5x faster") },
        { key: "Home", desc: qsTr("Horizontal slider - Move to start position") },
        { key: "End", desc: qsTr("Horizontal slider - Move to last position") }
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

        // Full-width 1px top separator line (upstream m_top_line, cpp:33-36);
        // the neutral border token stands in for upstream RGB(166,169,170).
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: Theme.borderDefault
        }
    }

    contentItem: Item {
        implicitWidth: 1032
        implicitHeight: 501

        // Full-height left selector panel (upstream m_panel_selects, full
        // height via wxEXPAND, cpp:39-40 + :54); the neutral elevated surface
        // token stands in for upstream RGB(248,248,248).
        Rectangle {
            id: leftPanel
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: 150
            color: Theme.bgElevated

            // 20 DIP above the first button (upstream cpp:43), then flush
            // 150x28 square buttons (cpp:116).
            Column {
                anchors.top: parent.top
                anchors.topMargin: 20
                width: parent.width
                spacing: 0

                Repeater {
                    model: root.groups
                    delegate: Rectangle {
                        width: 150
                        height: 28
                        radius: 0
                        color: root.currentGroup === modelData.id ? Theme.accent : "transparent"
                        opacity: groupMouse.containsMouse && root.currentGroup !== modelData.id ? 0.8 : 1.0

                        Text {
                            // 22px left indent, vertically centered
                            // (upstream cpp:120-126); 13px label switching
                            // between body and bold weights on selection.
                            x: 22
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.name
                            color: root.currentGroup === modelData.id ? Theme.textOnAccent : Theme.textSecondary
                            font.pixelSize: Theme.fontSize13
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
            }
        }

        // Right content page: 12 DIP left of the selector panel, scrolling
        // shortcuts grid (upstream simplebook + scrolled page, cpp:58-64,
        // 338-365).
        ScrollView {
            anchors.top: parent.top
            anchors.topMargin: 1
            anchors.left: leftPanel.right
            anchors.leftMargin: 12
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            clip: true
            contentWidth: availableWidth

            // 20 DIP margin around the grid (upstream cpp:360); rows 10 DIP
            // apart with a 20 DIP column gap (upstream wxFlexGridSizer,
            // cpp:341). Keys are plain bold text (cpp:349-351) and the
            // description wraps (upstream Wrap(600), cpp:356).
            Column {
                x: 20
                y: 20
                width: parent.width - 40
                spacing: Theme.spacingMD

                Repeater {
                    model: root.shortcutsForGroup(root.currentGroup)
                    delegate: RowLayout {
                        width: parent.width
                        spacing: Theme.spacingXL

                        Text {
                            Layout.preferredWidth: 150
                            text: modelData.key
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSize13
                            font.bold: true
                            verticalAlignment: Text.AlignVCenter
                        }

                        Text {
                            Layout.fillWidth: true
                            Layout.maximumWidth: 600
                            text: modelData.desc
                            wrapMode: Text.Wrap
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSize13
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }
        }
    }
}
