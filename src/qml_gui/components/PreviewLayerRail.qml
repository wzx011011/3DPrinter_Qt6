import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// Vertical layer rail for the Preview page, aligned with upstream OrcaSlicer IMSlider.
// Hosts: dual-thumb layer range slider, layer jump buttons, AND tick marks (pause /
// color-change / filament-change / custom-gcode / template) with right-click add/edit/delete
// menus. Consolidates the formerly-orphaned horizontal LayerSlider.qml tick functionality
// into this vertical source-truth-aligned rail (Phase 117, TICK-01).
Item {
    id: root
    required property var previewVm

    readonly property int totalLayers: root.previewVm ? root.previewVm.layerCount : 0
    readonly property int lastLayerIndex: Math.max(0, root.totalLayers - 1)

    // Tick mark editing state aligned with upstream IMSlider::render_edit_menu.
    property int editMenuTickLayer: -1
    property int editMenuTickType: -1
    // Target layer for add menu (computed from right-click position on the rail track).
    property int addMenuTargetLayer: -1

    function clampedLayer(value) {
        return Math.max(0, Math.min(root.lastLayerIndex, Math.round(value)))
    }

    function commitRange(firstLayer, secondLayer) {
        if (!root.previewVm || root.totalLayers <= 0)
            return
        const minLayer = Math.min(root.clampedLayer(firstLayer), root.clampedLayer(secondLayer))
        const maxLayer = Math.max(root.clampedLayer(firstLayer), root.clampedLayer(secondLayer))
        root.previewVm.setLayerRange(minLayer, maxLayer)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 5

        RailButton {
            label: "|^"
            tooltip: qsTr("Top layer")
            controlEnabled: root.previewVm && root.totalLayers > 0
            onTriggered: root.previewVm.jumpToLayer(root.totalLayers)
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: root.totalLayers
            color: Theme.accentLight
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
        }

        RailButton {
            label: "+"
            tooltip: qsTr("Move layer range up")
            controlEnabled: root.previewVm && root.totalLayers > 0
            onTriggered: root.previewVm.moveLayerRange(1)
        }

        // Vertical dual-thumb layer range slider with overlaid tick marks.
        // The RangeSlider track hosts both the range thumbs and the tick Repeater.
        Item {
            id: railTrackHost
            Layout.fillHeight: true
            Layout.preferredWidth: 30
            Layout.alignment: Qt.AlignHCenter

            // Track geometry (vertical: height is the long axis).
            readonly property real trackMargin: 8
            readonly property real trackHeight: height - trackMargin * 2

            RangeSlider {
                id: layerRangeSlider
                anchors.fill: parent
                orientation: Qt.Vertical
                from: 0
                to: root.lastLayerIndex
                stepSize: 1
                snapMode: RangeSlider.SnapAlways
                enabled: root.previewVm && root.totalLayers > 0
                first.value: root.previewVm ? root.previewVm.currentLayerMin : 0
                second.value: root.previewVm ? root.previewVm.currentLayerMax : 0
                first.onMoved: root.commitRange(first.value, second.value)
                second.onMoved: root.commitRange(first.value, second.value)
            }

            // Tick marks rendered on the slider track, aligned with upstream IMSlider::draw_ticks.
            // Adapted from the horizontal LayerSlider.qml to the vertical orientation: the tick
            // position along the track is now y (top=layer 0, bottom=last layer).
            Repeater {
                model: root.previewVm ? root.previewVm.tickMarks : []
                delegate: Item {
                    readonly property real tickY: railTrackHost.trackMargin
                        + (root.lastLayerIndex > 0
                           ? (modelData.tick / root.lastLayerIndex) * railTrackHost.trackHeight
                           : 0)
                    readonly property int tickType: modelData.type
                    readonly property int tickLayer: modelData.tick

                    // Position the tick horizontally beside the slider, vertically at the layer.
                    x: layerRangeSlider.width / 2 + 10
                    y: tickY - 4
                    width: 8
                    height: 8
                    z: 2

                    Rectangle {
                        anchors.fill: parent
                        radius: 2
                        border.width: 1
                        border.color: Theme.bgBase
                        color: {
                            // TickType: PausePrint=0, CustomGcode=1, Template=2, ToolChange=3, ColorChange=4
                            switch(tickType) {
                            case 0: return Theme.statusWarning    // PausePrint - orange
                            case 1: return Theme.accentSubtle     // CustomGcode - deep green (distinct from ColorChange)
                            case 3: return Theme.statusInfo       // ToolChange - blue
                            case 4: return Theme.accent           // ColorChange - green
                            default: return Theme.textSecondary   // Template - gray
                            }
                        }
                    }

                    // Right-click on a tick mark shows the edit/delete menu (upstream IMSlider edit menu).
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -3
                        acceptedButtons: Qt.RightButton
                        onClicked: {
                            root.editMenuTickLayer = tickLayer
                            root.editMenuTickType = tickType
                            sliderEditMenu.popup()
                        }
                    }
                }
            }

            // Groove interaction area: right-click on empty track area opens the add menu,
            // aligned with upstream IMSlider groove right-click add_menu behavior.
            MouseArea {
                id: grooveMA
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.RightButton
                // Right-click computes the layer at the click y-position for the add menu.
                onClicked: function(mouse) {
                    if (!root.previewVm || root.lastLayerIndex <= 0) return
                    if (mouse.button !== Qt.RightButton) return
                    var relY = mouse.y - railTrackHost.trackMargin
                    var clickedLayer = Math.round((relY / railTrackHost.trackHeight) * root.lastLayerIndex)
                    clickedLayer = Math.max(0, Math.min(clickedLayer, root.lastLayerIndex))
                    root.addMenuTargetLayer = clickedLayer
                    sliderAddMenu.popup()
                }
                // Do not steal the RangeSlider thumb drag (only handle right-click).
                z: -1
            }
        }

        RailButton {
            label: "-"
            tooltip: qsTr("Move layer range down")
            controlEnabled: root.previewVm && root.totalLayers > 0
            onTriggered: root.previewVm.moveLayerRange(-1)
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: root.previewVm ? root.previewVm.currentLayerMax + 1 : 0
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            font.family: "Consolas"
        }

        RailButton {
            label: "|v"
            tooltip: qsTr("First layer")
            controlEnabled: root.previewVm && root.totalLayers > 0
            onTriggered: root.previewVm.jumpToLayer(1)
        }
    }

    // Slider add menu aligned with upstream IMSlider::render_add_menu.
    // Shown on right-click on slider groove (empty area).
    CxMenu {
        id: sliderAddMenu

        CxMenuItem {
            text: qsTr("Add Pause")
            onTriggered: {
                if (root.previewVm && root.addMenuTargetLayer >= 0)
                    root.previewVm.addPauseAtLayer(root.addMenuTargetLayer)
            }
        }
        CxMenuItem {
            text: qsTr("Add Custom G-code...")
            onTriggered: {
                customGcodeAddDialog.targetLayer = root.addMenuTargetLayer
                customGcodeAddDialog.gcodeText = ""
                customGcodeAddDialog.open()
            }
        }
    }

    // Slider edit menu aligned with upstream IMSlider::render_edit_menu.
    // Shown on right-click on existing tick mark.
    CxMenu {
        id: sliderEditMenu

        // PausePrint tick (type 0)
        CxMenuItem {
            text: qsTr("Delete Pause")
            visible: root.editMenuTickType === 0
            onTriggered: {
                if (root.previewVm && root.editMenuTickLayer >= 0)
                    root.previewVm.removeTickAtLayer(root.editMenuTickLayer)
            }
        }

        // Template tick (type 2)
        CxMenuItem {
            text: qsTr("Delete Custom Template")
            visible: root.editMenuTickType === 2
            onTriggered: {
                if (root.previewVm && root.editMenuTickLayer >= 0)
                    root.previewVm.removeTickAtLayer(root.editMenuTickLayer)
            }
        }

        // CustomGcode tick (type 1)
        CxMenuItem {
            text: qsTr("Edit Custom G-code")
            visible: root.editMenuTickType === 1
            onTriggered: {
                if (!root.previewVm || root.editMenuTickLayer < 0) return
                var existing = root.previewVm.tickAtLayer(root.editMenuTickLayer)
                customGcodeEditDialog.targetLayer = root.editMenuTickLayer
                customGcodeEditDialog.gcodeText = existing.extra || ""
                customGcodeEditDialog.open()
            }
        }
        CxMenuItem {
            text: qsTr("Delete Custom G-code")
            visible: root.editMenuTickType === 1
            onTriggered: {
                if (root.previewVm && root.editMenuTickLayer >= 0)
                    root.previewVm.removeTickAtLayer(root.editMenuTickLayer)
            }
        }

        // ToolChange tick (type 3)
        CxMenuItem {
            text: qsTr("Delete Filament Change")
            visible: root.editMenuTickType === 3
            onTriggered: {
                if (root.previewVm && root.editMenuTickLayer >= 0)
                    root.previewVm.removeTickAtLayer(root.editMenuTickLayer)
            }
        }

        // ColorChange tick (type 4)
        CxMenuItem {
            text: qsTr("Delete Color Change")
            visible: root.editMenuTickType === 4
            onTriggered: {
                if (root.previewVm && root.editMenuTickLayer >= 0)
                    root.previewVm.removeTickAtLayer(root.editMenuTickLayer)
            }
        }
    }

    // Custom G-code add dialog aligned with the upstream IMSlider custom G-code window.
    CustomGcodeDialog {
        id: customGcodeAddDialog
        previewVm: root.previewVm
        anchors.centerIn: parent.parent ? parent.parent : parent
    }

    // Custom G-code edit dialog.
    CustomGcodeDialog {
        id: customGcodeEditDialog
        previewVm: root.previewVm
        dialogTitle: qsTr("Edit Custom G-code")
        isEditMode: true
        anchors.centerIn: parent.parent ? parent.parent : parent
    }

    component RailButton: Rectangle {
        id: railButtonRoot
        property string label: ""
        property string tooltip: ""
        property bool controlEnabled: true
        signal triggered()

        Layout.preferredWidth: 30
        Layout.preferredHeight: 24
        Layout.alignment: Qt.AlignHCenter
        radius: 4
        color: railMouse.containsMouse && railButtonRoot.controlEnabled ? Theme.bgHover : Theme.bgElevated
        border.width: 1
        border.color: railMouse.containsMouse && railButtonRoot.controlEnabled ? Theme.accentDark : Theme.borderSubtle
        opacity: railButtonRoot.controlEnabled ? 1.0 : 0.45

        Text {
            anchors.centerIn: parent
            text: railButtonRoot.label
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeXS
            font.family: "Consolas"
        }

        MouseArea {
            id: railMouse
            anchors.fill: parent
            enabled: railButtonRoot.controlEnabled
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: railButtonRoot.triggered()
        }

        ToolTip.visible: railMouse.containsMouse && railButtonRoot.tooltip.length > 0
        ToolTip.text: railButtonRoot.tooltip
        ToolTip.delay: 450
    }
}
