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
    readonly property bool hasTemplateGcode: root.previewVm
        && root.previewVm.fullConfig
        && String(root.previewVm.fullConfig["template_custom_gcode"] || "").length > 0

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

        CxStepButton {
            label: root.previewVm && root.previewVm.singleLayer ? "1" : "2"
            tooltip: qsTr("Toggle single layer")
            preferredWidth: 30
            preferredHeight: 24
            smallFont: true
            controlEnabled: root.previewVm && root.totalLayers > 0
            onTriggered: root.previewVm.setSingleLayer(!root.previewVm.singleLayer)
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
                property bool lowerHandleSelected: false
                first.value: root.previewVm ? root.previewVm.currentLayerMin : 0
                second.value: root.previewVm ? root.previewVm.currentLayerMax : 0
                first.onPressedChanged: if (first.pressed) layerRangeSlider.lowerHandleSelected = true
                second.onPressedChanged: if (second.pressed) layerRangeSlider.lowerHandleSelected = false
                first.onMoved: root.commitRange(first.value, second.value)
                second.onMoved: root.commitRange(first.value, second.value)
                // IMSlider::on_mouse_wheel changes the active handle. The rail
                // itself owns wheel input so the backend remains the source of truth.
                WheelHandler {
                    acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                    onWheel: function(event) {
                        if (root.previewVm)
                            root.previewVm.wheelLayer(event.angleDelta.y > 0 ? 1 : -1,
                                                      event.modifiers & Qt.ShiftModifier,
                                                      layerRangeSlider.lowerHandleSelected)
                        event.accepted = true
                    }
                }
            }

            // Tick marks rendered on the slider track, aligned with upstream IMSlider::draw_ticks.
            // Adapted from the horizontal LayerSlider.qml to the vertical orientation. Qt's
            // vertical controls increase upward, matching IMSlider::get_pos_from_value.
            Repeater {
                model: root.previewVm ? root.previewVm.tickMarks : []
                delegate: Item {
                    id: tickDelegate
                    readonly property real tickY: railTrackHost.trackMargin
                        + (root.lastLayerIndex > 0
                           ? (1 - modelData.tick / root.lastLayerIndex) * railTrackHost.trackHeight
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
                            case 4: return (tickDelegate.modelData.color && tickDelegate.modelData.color !== "")
                                     ? tickDelegate.modelData.color : Theme.accent  // ColorChange - picked color
                            default: return Theme.textSecondary   // Template - gray
                            }
                        }
                    }

                    // Phase 238 (PREV-04): tick hover tooltip aligned with the
                    // upstream IMSlider::show_tooltip(TickCode) -- elapsed time
                    // at the END of the previous layer (ticks sit at the START
                    // of their layer) plus the type-specific gcode info
                    // (IMSlider.cpp:774-797: Pause "M601", Change Filament,
                    // Custom G-code extra, ...). Attached ToolTip form matches
                    // the BBLTopbar/GroupNavSidebar convention.
                    HoverHandler { id: tickHover }
                    ToolTip.visible: tickHover.hovered
                    ToolTip.delay: 400
                    ToolTip.text: {
                        if (!root.previewVm)
                            return ""
                        const timePart = tickLayer > 0
                            ? root.previewVm.layerTimeLabel(tickLayer - 1) : ""
                        switch (tickType) {
                        case 0: return timePart + (timePart ? "\n" : "") + qsTr("Pause") + ": M601"
                        case 1: return timePart + (timePart ? "\n" : "")
                                + qsTr("Custom G-code") + ": " + (tickDelegate.modelData.extra || "")
                        case 2: return timePart + (timePart ? "\n" : "") + qsTr("Custom Template")
                        case 3: return timePart + (timePart ? "\n" : "") + qsTr("Change Filament")
                        case 4: return timePart + (timePart ? "\n" : "") + qsTr("Color Change")
                                + ": " + (tickDelegate.modelData.color || "")
                        default: return timePart
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
                            var targetLayer = Math.round((1 - relY / railTrackHost.trackHeight) * root.lastLayerIndex)
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
                    var clickedLayer = Math.round((1 - relY / railTrackHost.trackHeight) * root.lastLayerIndex)
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
            text: root.previewVm ? root.previewVm.currentLayerMin + 1 : 0
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
        // Phase 238 (PREV-04): upstream exposes Pause, Custom G-code, an
        // optional configured Template, Jump to Layer, and multi-extruder
        // Change Filament (IMSlider.cpp:1328-1377).
        CxMenuItem {
            text: qsTr("Change Filament...")
            // Upstream gates the entry on m_extruder_colors.size() > 1
            // (IMSlider.cpp:1374).
            enabled: root.previewVm && root.previewVm.configuredExtruderCount() > 1
            onTriggered: {
                filamentChangeDialog.targetLayer = root.addMenuTargetLayer
                filamentChangeDialog.open()
            }
        }
        CxMenuItem {
            text: qsTr("Add Custom Template")
            visible: root.hasTemplateGcode
            enabled: root.previewVm && root.addMenuTargetLayer >= 0
            onTriggered: {
                if (root.previewVm && root.addMenuTargetLayer >= 0)
                    root.previewVm.addTemplateAtLayer(root.addMenuTargetLayer)
            }
        }
        CxMenuItem {
            text: qsTr("Jump to Layer")
            onTriggered: jumpToLayerDialog.open()
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
            text: qsTr("Change Filament...")
            visible: root.editMenuTickType === 3
            onTriggered: {
                filamentChangeDialog.targetLayer = root.editMenuTickLayer
                filamentChangeDialog.editMode = true
                filamentChangeDialog.open()
            }
        }
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

    // Phase 238 (PREV-04): filament (ToolChange) picker. Lists the configured
    // extruders with their color swatches and calls addFilamentChangeAtLayer
    // (previously zero QML callers). Edit mode re-picks an existing tick via
    // editFilamentChangeAtLayer (upstream edit menu, IMSlider.cpp:1414-1424).
    CxDialog {
        id: filamentChangeDialog

        property int targetLayer: -1
        property bool editMode: false

        dialogTitle: editMode ? qsTr("Change Filament") : qsTr("Add Filament Change")
        width: 300
        modal: true

        ColumnLayout {
            spacing: Theme.spacingSM

            Text {
                text: qsTr("Select the filament to use from layer %1 on:").arg(filamentChangeDialog.targetLayer + 1)
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeMD
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            Repeater {
                model: root.previewVm ? root.previewVm.configuredExtruderCount() : 0

                delegate: Rectangle {
                    id: filamentRow
                    required property int index
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    radius: Theme.radiusSM
                    color: filamentRowMouse.containsMouse ? Theme.bgHover : "transparent"
                    border.width: 1
                    border.color: Theme.borderSubtle

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 8

                        Rectangle {
                            Layout.preferredWidth: 14
                            Layout.preferredHeight: 14
                            radius: 3
                            color: root.previewVm ? root.previewVm.extruderColor(filamentRow.index) : Theme.accent
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Filament %1").arg(filamentRow.index + 1)
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                        }
                    }

                    MouseArea {
                        id: filamentRowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (!root.previewVm || filamentChangeDialog.targetLayer < 0)
                                return
                            if (filamentChangeDialog.editMode)
                                root.previewVm.editFilamentChangeAtLayer(
                                            filamentChangeDialog.targetLayer, filamentRow.index)
                            else
                                root.previewVm.addFilamentChangeAtLayer(
                                            filamentChangeDialog.targetLayer, filamentRow.index)
                            filamentChangeDialog.close()
                        }
                    }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: Theme.spacingSM
                CxButton {
                    text: qsTr("Cancel")
                    onClicked: filamentChangeDialog.close()
                }
            }
        }

        onClosed: filamentChangeDialog.editMode = false
    }

    // Phase 238 (PREV-04): color-change picker replacing the hardcoded
    // extruder 1 + #FF0000. Extruder row + the upstream default color palette
    // (GCodeProcessor Default_Colors, exposed by
    // PreviewViewModel::defaultColorChangePalette).
    CxDialog {
        id: colorChangeDialog

        property int targetLayer: -1
        property int selectedExtruder: 0
        property string selectedColor: ""

        dialogTitle: qsTr("Add Color Change")
        width: 320
        modal: true

        onOpened: {
            selectedExtruder = 0
            selectedColor = root.previewVm ? root.previewVm.defaultColorChangePalette()[0] : ""
        }

        ColumnLayout {
            spacing: Theme.spacingSM

            Text {
                text: qsTr("Pick the extruder and color for layer %1:").arg(colorChangeDialog.targetLayer + 1)
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeMD
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            Repeater {
                model: root.previewVm ? root.previewVm.configuredExtruderCount() : 0

                delegate: Rectangle {
                    id: colorExtruderRow
                    required property int index
                    Layout.fillWidth: true
                    Layout.preferredHeight: 32
                    radius: Theme.radiusSM
                    color: colorExtruderMouse.containsMouse ? Theme.bgHover : "transparent"
                    border.width: 1
                    border.color: colorChangeDialog.selectedExtruder === colorExtruderRow.index
                                   ? Theme.accent : Theme.borderSubtle

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        spacing: 8

                        Rectangle {
                            Layout.preferredWidth: 14
                            Layout.preferredHeight: 14
                            radius: 3
                            color: root.previewVm ? root.previewVm.extruderColor(colorExtruderRow.index) : Theme.accent
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Filament %1").arg(colorExtruderRow.index + 1)
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                        }
                    }

                    MouseArea {
                        id: colorExtruderMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: colorChangeDialog.selectedExtruder = colorExtruderRow.index
                    }
                }
            }

            Text {
                text: qsTr("Color")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeSM
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: root.previewVm ? root.previewVm.defaultColorChangePalette() : []

                    delegate: Rectangle {
                        id: paletteSwatch
                        required property string modelData
                        Layout.preferredWidth: 28
                        Layout.preferredHeight: 28
                        radius: 4
                        color: paletteSwatch.modelData
                        border.width: colorChangeDialog.selectedColor === paletteSwatch.modelData ? 2 : 1
                        border.color: colorChangeDialog.selectedColor === paletteSwatch.modelData
                                      ? Theme.textPrimary : Theme.borderDefault

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: colorChangeDialog.selectedColor = paletteSwatch.modelData
                        }
                    }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: Theme.spacingSM

                CxButton {
                    text: qsTr("Cancel")
                    onClicked: colorChangeDialog.close()
                }
                CxButton {
                    text: qsTr("OK")
                    highlighted: true
                    onClicked: {
                        if (root.previewVm && colorChangeDialog.targetLayer >= 0
                                && colorChangeDialog.selectedColor !== "")
                            root.previewVm.addColorChangeAtLayer(
                                        colorChangeDialog.targetLayer,
                                        colorChangeDialog.selectedExtruder,
                                        colorChangeDialog.selectedColor)
                        colorChangeDialog.close()
                    }
                }
            }
        }
    }

    // Phase 238 (PREV-04): Jump-to-Layer dialog (upstream
    // IMSlider::render_go_to_layer_dialog, IMSlider.cpp:1221-1313 -- a
    // number input clamped to the layer range with OK/Cancel; OK jumps the
    // layer range to the picked 1-indexed layer).
    CxDialog {
        id: jumpToLayerDialog

        dialogTitle: qsTr("Jump to Layer")
        width: 280
        modal: true

        onOpened: jumpLayerSpin.value = (root.previewVm ? root.previewVm.currentLayerMax : 0) + 1

        ColumnLayout {
            spacing: Theme.spacingSM

            Text {
                text: qsTr("Please enter the layer number (%1 - %2):")
                        .arg(1).arg(root.totalLayers)
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeMD
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            CxSpinBox {
                id: jumpLayerSpin
                Layout.fillWidth: true
                from: 1
                to: Math.max(1, root.totalLayers)
                editable: true
            }

            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: Theme.spacingSM

                CxButton {
                    text: qsTr("Cancel")
                    onClicked: jumpToLayerDialog.close()
                }
                CxButton {
                    text: qsTr("OK")
                    highlighted: true
                    onClicked: {
                        if (root.previewVm && root.totalLayers > 0)
                            root.previewVm.jumpToLayer(jumpLayerSpin.value)
                        jumpToLayerDialog.close()
                    }
                }
            }
        }
    }
}
