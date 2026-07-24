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

        CxStepButton {
            label: "|^"
            tooltip: qsTr("Top layer")
            preferredWidth: 30
            preferredHeight: 24
            smallFont: true
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

        CxStepButton {
            label: "+"
            tooltip: qsTr("Move layer range up")
            preferredWidth: 30
            preferredHeight: 24
            smallFont: true
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
                    id: tickDelegate
                    readonly property real tickY: railTrackHost.trackMargin
                        + (root.lastLayerIndex > 0
                           ? (modelData.tick / root.lastLayerIndex) * railTrackHost.trackHeight
                           : 0)
                    readonly property int tickType: modelData.type
                    readonly property int tickLayer: modelData.tick

                    // Phase 119 (TICK-05): drag-to-relocate state. While dragging,
                    // dragY overrides the layer-derived position so the tick follows
                    // the cursor; on release the target layer is computed and
                    // previewVm.moveTick is called. A false return (target occupied
                    // or source missing) leaves tickY re-bound -> the tick snaps back.
                    property real dragY: 0
                    property bool dragging: false
                    property int dragFromLayer: -1

                    // Position the tick horizontally beside the slider, vertically at the layer.
                    x: layerRangeSlider.width / 2 + 10
                    y: dragging ? dragY : (tickY - 4)
                    width: 8
                    height: 8
                    z: dragging ? 5 : 2

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

                    // Phase 119 (TICK-05): left-button vertical drag-to-relocate,
                    // aligned with upstream IMSlider on_mouse_drag. Computes the
                    // target layer from the released y and calls previewVm.moveTick.
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -3
                        acceptedButtons: Qt.LeftButton
                        cursorShape: Qt.SizeVerCursor
                        preventStealing: true
                        onPressed: function(mouse) {
                            if (!root.previewVm || root.lastLayerIndex <= 0) return
                            tickDelegate.dragFromLayer = tickLayer
                            tickDelegate.dragging = true
                            tickDelegate.dragY = tickY - 4
                        }
                        onPositionChanged: function(mouse) {
                            if (!tickDelegate.dragging) return
                            // Follow the cursor vertically (map to track host coords).
                            var mapped = parent.mapToItem(railTrackHost, mouse.x, mouse.y)
                            tickDelegate.dragY = Math.max(railTrackHost.trackMargin - 4,
                                                          Math.min(mapped.y,
                                                                   railTrackHost.trackMargin + railTrackHost.trackHeight - 4))
                        }
                        onReleased: {
                            if (!tickDelegate.dragging) return
                            var relY = tickDelegate.dragY + 4 - railTrackHost.trackMargin
                            var targetLayer = Math.round((relY / railTrackHost.trackHeight) * root.lastLayerIndex)
                            targetLayer = Math.max(0, Math.min(targetLayer, root.lastLayerIndex))
                            var fromLayer = tickDelegate.dragFromLayer
                            tickDelegate.dragging = false
                            // moveTick returns false when the target is occupied or
                            // the source is gone; the y re-binds to tickY -> snap back.
                            if (root.previewVm && fromLayer >= 0)
                                root.previewVm.moveTick(fromLayer, targetLayer)
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

        CxStepButton {
            label: "-"
            tooltip: qsTr("Move layer range down")
            preferredWidth: 30
            preferredHeight: 24
            smallFont: true
            controlEnabled: root.previewVm && root.totalLayers > 0
            onTriggered: root.previewVm.moveLayerRange(-1)
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: root.previewVm ? root.previewVm.currentLayerMax + 1 : 0
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            font.family: Theme.fontMono
        }

        CxStepButton {
            label: "|v"
            tooltip: qsTr("First layer")
            preferredWidth: 30
            preferredHeight: 24
            smallFont: true
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
        // Phase 119 (TICK-04): close the 5-type coverage gap. ColorChange +
        // Template were reachable in the data + convert layers but missing from
        // the Add menu. ColorChange uses a default extruder + color (extruder 1,
        // red) for now; a dedicated extruder+color picker is a future enhancement
        // (CustomGcodeDialog extension). Template needs no extra input.
        CxMenuItem {
            text: qsTr("Add Color Change")
            onTriggered: {
                if (root.previewVm && root.addMenuTargetLayer >= 0)
                    root.previewVm.addColorChangeAtLayer(root.addMenuTargetLayer, 1, "#FF0000")
            }
        }
        CxMenuItem {
            text: qsTr("Add Template")
            onTriggered: {
                if (root.previewVm && root.addMenuTargetLayer >= 0)
                    root.previewVm.addTemplateAtLayer(root.addMenuTargetLayer)
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
}
