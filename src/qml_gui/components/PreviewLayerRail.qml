import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// Vertical layer rail for the Preview page, aligned with upstream OrcaSlicer IMSlider.
// Hosts: dual-thumb layer range slider, layer jump buttons, AND tick marks (pause /
// color-change / filament-change / custom-gcode / template). Right-clicking the
// ACTIVE slider handle opens the add menu (empty slot) or the tick edit/delete menu,
// mirroring the upstream IMSlider trigger model (IMSlider.cpp:1117-1121, :1456-1474).
// Consolidates the formerly-orphaned horizontal LayerSlider.qml tick functionality
// into this vertical source-truth-aligned rail (Phase 117, TICK-01).
Item {
    id: root
    required property var previewVm

    readonly property int totalLayers: root.previewVm ? root.previewVm.layerCount : 0
    readonly property int lastLayerIndex: Math.max(0, root.totalLayers - 1)
    readonly property bool hasTemplateGcode: root.previewVm
        && root.previewVm.fullConfig
        && String(root.previewVm.fullConfig["template_custom_gcode"] || "").length > 0

    // Tick mark editing state aligned with upstream IMSlider::render_edit_menu:
    // the tick (if any) at the ACTIVE handle's layer selects the edit menu.
    property int editMenuTickLayer: -1
    property int editMenuTickType: -1
    // Target layer for the add menu: the ACTIVE handle's layer (upstream
    // add_code_as_tick takes m_selection's handle value, IMSlider.cpp:386-392).
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

    // Layer of the active handle (upstream ssLower -> m_lower_value, otherwise
    // m_higher_value; Qt6 lowerHandleSelected mirrors m_selection).
    function activeHandleLayer() {
        return root.clampedLayer(layerRangeSlider.lowerHandleSelected
                                 ? layerRangeSlider.first.value
                                 : layerRangeSlider.second.value)
    }

    // Upstream IMSlider::render_menu (IMSlider.cpp:1456-1474): a tick at the
    // active handle's layer routes to the edit menu, an empty slot to the
    // add menu -- both anchored to that same handle layer.
    function openSliderMenu() {
        if (!root.previewVm || root.totalLayers <= 0)
            return
        var layer = root.activeHandleLayer()
        var tick = root.previewVm.tickAtLayer(layer)
        if (tick && tick.type !== undefined) {
            root.editMenuTickLayer = layer
            root.editMenuTickType = tick.type
            sliderEditMenu.popup()
        } else {
            root.addMenuTargetLayer = layer
            sliderAddMenu.popup()
        }
    }

    function removeEditMenuTick() {
        if (root.previewVm && root.editMenuTickLayer >= 0)
            root.previewVm.removeTickAtLayer(root.editMenuTickLayer)
    }

    // Handle-center hit test in railTrackHost coordinates: the right-click
    // area and the RangeSlider fill the same rect, so mapping the handle
    // center into slider space gives the click-space geometry.
    function pointOnHandle(handle, x, y) {
        if (!handle)
            return false
        var c = layerRangeSlider.mapFromItem(handle, handle.width / 2, handle.height / 2)
        return Math.abs(x - c.x) <= handle.width / 2
               && Math.abs(y - c.y) <= handle.height / 2
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingXS

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
                // G-07: the handles are synced IMPERATIVELY from the viewmodel
                // (syncFromVm below). The original declarative value bindings
                // broke permanently on the first user drag — RangeSlider writes
                // value imperatively while dragging, which severed the binding,
                // after which jump-to-layer/playback/plate switches no longer
                // moved the thumbs.
                function syncFromVm() {
                    if (!root.previewVm || first.pressed || second.pressed)
                        return
                    first.value = root.previewVm.currentLayerMin
                    second.value = root.previewVm.currentLayerMax
                }
                Component.onCompleted: syncFromVm()
                Connections {
                    target: root.previewVm
                    function onStateChanged() { layerRangeSlider.syncFromVm() }
                }
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
                        radius: Theme.radiusXS
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
                }
            }

            // Upstream IMSlider.cpp:1117-1121 (dual handle) / :1184-1188
            // (one-layer): the ONLY menu entry is a right-click on the ACTIVE
            // handle; a right-click on the groove or the inactive handle just
            // closes the menu (no-op here). Declared after the RangeSlider so
            // it sits above it and receives the right button -- the previous
            // z:-1 groove area could never see a click through the slider.
            MouseArea {
                id: handleMenuMA
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: function(mouse) {
                    if (!root.previewVm || root.totalLayers <= 0)
                        return
                    var activeHandle = layerRangeSlider.lowerHandleSelected
                                       ? layerRangeSlider.first.handle
                                       : layerRangeSlider.second.handle
                    if (root.pointOnHandle(activeHandle, mouse.x, mouse.y))
                        root.openSliderMenu()
                }
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

    // Filament N submenu entry (upstream menu_item_with_icon, IMSlider.cpp:1529
    // / :1579): CxMenuItem look with a reserved column for the 14x14 extruder
    // color swatch. CxMenuItem itself has no icon slot, so the row is built
    // here; Triggered handling stays with each use site.
    component FilamentMenuItem: MenuItem {
        id: filamentItem
        required property int index
        text: qsTr("Filament %1").arg(index + 1)
        implicitHeight: 28
        leftPadding: Theme.spacingLG + 16
        background: Rectangle {
            color: filamentItem.enabled && filamentItem.highlighted
                   ? (filamentItem.pressed ? Theme.bgPressed : Theme.bgHover)
                   : "transparent"
            Behavior on color { ColorAnimation { duration: Theme.motionFast; easing.type: Theme.easingStandard } }
        }
        contentItem: Text {
            text: filamentItem.text
            color: filamentItem.enabled ? Theme.textPrimary : Theme.textDisabled
            font.pixelSize: Theme.fontSizeMD
            verticalAlignment: Text.AlignVCenter
        }
        Rectangle {
            x: Theme.spacingLG
            y: (filamentItem.height - height) / 2
            width: 14
            height: 14
            radius: Theme.radiusXS
            color: root.previewVm ? root.previewVm.extruderColor(filamentItem.index) : Theme.accent
            border.width: 1
            border.color: Theme.bgBase
        }
    }

    // Slider add menu aligned with upstream IMSlider::render_add_menu
    // (IMSlider.cpp:1483-1538): shown on right-click on the ACTIVE handle
    // when that layer has no tick. Item order 1:1 upstream: Pause, Custom
    // G-code, Template (only when configured), Jump to layer, and the Change
    // Filament submenu last. Pause / Custom G-code / Template are disabled in
    // the sequential-print draw mode (IMSlider.cpp:1492). The delegate feeds
    // the submenu row so it can be hidden per the upstream extruder-count
    // gate (nested Menu rows do not follow the child Menu's own visibility).
    CxMenu {
        id: sliderAddMenu
        delegate: CxMenuItem {
            visible: subMenu ? subMenu.rowVisible : true
        }

        CxMenuItem {
            text: qsTr("Add Pause")
            // Upstream disables the insertion items in the sequential-print
            // draw mode (menu_item_enable = m_draw_mode != dmSequentialFffPrint,
            // IMSlider.cpp:1492).
            enabled: root.previewVm && !root.previewVm.sequentialPrint
            HoverHandler { id: addPauseHover }
            ToolTip.visible: addPauseHover.hovered
            ToolTip.delay: 400
            ToolTip.text: qsTr("Insert a pause command at the beginning of this layer.")
            onTriggered: {
                if (root.previewVm && root.addMenuTargetLayer >= 0)
                    root.previewVm.addPauseAtLayer(root.addMenuTargetLayer)
            }
        }
        CxMenuItem {
            text: qsTr("Add Custom G-code")
            enabled: root.previewVm && !root.previewVm.sequentialPrint
            HoverHandler { id: addGcodeHover }
            ToolTip.visible: addGcodeHover.hovered
            ToolTip.delay: 400
            ToolTip.text: qsTr("Insert custom G-code at the beginning of this layer.")
            onTriggered: {
                customGcodeAddDialog.targetLayer = root.addMenuTargetLayer
                customGcodeAddDialog.gcodeText = ""
                customGcodeAddDialog.open()
            }
        }
        // Upstream renders this entry only when a template gcode is configured
        // (IMSlider.cpp:1506-1511).
        CxMenuItem {
            text: qsTr("Add Custom Template")
            visible: root.hasTemplateGcode
            enabled: root.previewVm && !root.previewVm.sequentialPrint
                     && root.addMenuTargetLayer >= 0
            HoverHandler { id: addTemplateHover }
            ToolTip.visible: addTemplateHover.hovered
            ToolTip.delay: 400
            ToolTip.text: qsTr("Insert template custom G-code at the beginning of this layer.")
            onTriggered: {
                if (root.previewVm && root.addMenuTargetLayer >= 0)
                    root.previewVm.addTemplateAtLayer(root.addMenuTargetLayer)
            }
        }
        CxMenuItem {
            text: qsTr("Jump to layer")
            onTriggered: jumpToLayerDialog.open()
        }
        // Change Filament submenu, upstream IMSlider.cpp:1519-1534: rendered
        // only for multi-extruder profiles (row hidden otherwise) and fully
        // disabled when m_can_change_color is false. A click on Filament N
        // inserts the ToolChange tick immediately at the active handle layer.
        CxMenu {
            title: qsTr("Change Filament")
            property bool rowVisible: root.previewVm
                                      && root.previewVm.configuredExtruderCount() > 1
            enabled: root.previewVm && root.previewVm.canChangeColor
            Repeater {
                model: root.previewVm ? root.previewVm.configuredExtruderCount() : 0
                delegate: FilamentMenuItem {
                    HoverHandler { id: addFilamentHover }
                    ToolTip.visible: addFilamentHover.hovered
                    ToolTip.delay: 400
                    ToolTip.text: qsTr("Change filament at the beginning of this layer.")
                    onTriggered: {
                        if (root.previewVm && root.addMenuTargetLayer >= 0)
                            root.previewVm.addFilamentChangeAtLayer(root.addMenuTargetLayer, index)
                    }
                }
            }
        }
    }

    // Slider edit menu aligned with upstream IMSlider::render_edit_menu
    // (IMSlider.cpp:1540-1596). One entry group per tick type; ColorChange /
    // Unknown ticks render NO entries (upstream :1589-1592). The delegate
    // hides the ToolChange submenu row when the extruder-count gate hides it
    // upstream (nested Menu rows do not follow the child Menu's visibility).
    CxMenu {
        id: sliderEditMenu
        delegate: CxMenuItem {
            visible: subMenu ? subMenu.rowVisible : true
        }

        // PausePrint tick (type 0, upstream :1549-1553)
        CxMenuItem {
            text: qsTr("Delete Pause")
            visible: root.editMenuTickType === 0
            onTriggered: root.removeEditMenuTick()
        }

        // Template tick (type 2, upstream :1554-1560 -- offered only while a
        // template gcode is configured)
        CxMenuItem {
            text: qsTr("Delete Custom Template")
            visible: root.editMenuTickType === 2 && root.hasTemplateGcode
            onTriggered: root.removeEditMenuTick()
        }

        // CustomGcode tick (type 1, upstream :1561-1568)
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
            onTriggered: root.removeEditMenuTick()
        }

        // ToolChange tick (type 3, upstream :1569-1587): the whole block
        // exists only for multi-extruder profiles -- a single-extruder edit
        // menu stays empty like upstream. Change Filament is a submenu that
        // re-picks the tick's extruder in place (editFilamentChangeAtLayer).
        CxMenu {
            title: qsTr("Change Filament")
            property bool rowVisible: root.editMenuTickType === 3
                                      && root.previewVm
                                      && root.previewVm.configuredExtruderCount() > 1
            enabled: root.previewVm && root.previewVm.canChangeColor
            Repeater {
                model: root.previewVm ? root.previewVm.configuredExtruderCount() : 0
                delegate: FilamentMenuItem {
                    onTriggered: {
                        if (root.previewVm && root.editMenuTickLayer >= 0)
                            root.previewVm.editFilamentChangeAtLayer(root.editMenuTickLayer, index)
                    }
                }
            }
        }
        CxMenuItem {
            text: qsTr("Delete Filament Change")
            visible: root.editMenuTickType === 3
                     && root.previewVm
                     && root.previewVm.configuredExtruderCount() > 1
            onTriggered: root.removeEditMenuTick()
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
                            radius: Theme.radiusSM
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
                        radius: Theme.radiusSM
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
