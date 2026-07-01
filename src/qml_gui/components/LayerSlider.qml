import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

Item {
    id: root
    required property var previewVm

    // Upstream IMSlider displays layers as 1-indexed values.
    property int displayMin: root.previewVm ? root.previewVm.currentLayerMin + 1 : 1
    property int displayMax: root.previewVm ? root.previewVm.currentLayerMax + 1 : 1
    property int maxDisplay: root.previewVm ? Math.max(1, root.previewVm.layerCount) : 1
    property int totalLayers: root.previewVm ? Math.max(0, root.previewVm.layerCount - 1) : 0

    // Tick mark editing state aligned with upstream IMSlider::render_edit_menu.
    property int editMenuTickLayer: -1
    property int editMenuTickType: -1
    // Target layer for add menu (computed from right-click position on groove)
    property int addMenuTargetLayer: -1

    // Mouse wheel changes the layer range, aligned with upstream IMSlider::on_mouse_wheel.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        onWheel: function(wheel) {
            if (!root.previewVm || root.previewVm.layerCount <= 0)
                return
            var delta = wheel.angleDelta.y > 0 ? 1 : -1
            if (wheel.modifiers & Qt.ShiftModifier)
                delta *= 10
            root.previewVm.moveLayerRange(delta)
            wheel.accepted = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingSM

        Label { text: qsTr("层范围"); color: Theme.textPrimary; font.bold: true; font.pixelSize: Theme.fontSizeLG }

        // Dual-thumb range slider aligned with upstream IMSlider layer-range drag behavior.
        Item {
            id: rangeSliderItem
            Layout.fillWidth: true
            Layout.preferredHeight: 36

            readonly property real trackMargin: 12
            readonly property real thumbWidth: 14
            readonly property real trackWidth: width - trackMargin * 2
            property int dragThumb: -1  // -1=none, 0=min, 1=max

            // Track background
            Rectangle {
                x: rangeSliderItem.trackMargin
                y: rangeSliderItem.height / 2 - 3
                width: rangeSliderItem.trackWidth
                height: 6
                radius: 3
                color: Theme.bgElevated
            }

            // Active range track between the two thumbs, aligned with upstream IMSlider scroll_line.
            Rectangle {
                x: rangeSliderItem.trackMargin + (root.totalLayers > 0
                     ? (root.previewVm.currentLayerMin / root.totalLayers) * rangeSliderItem.trackWidth
                     : 0)
                y: rangeSliderItem.height / 2 - 3
                width: root.totalLayers > 0
                    ? ((root.previewVm.currentLayerMax - root.previewVm.currentLayerMin) / root.totalLayers) * rangeSliderItem.trackWidth
                    : rangeSliderItem.trackWidth
                height: 6
                radius: 3
                color: Theme.accent
                opacity: 0.7
            }

            // Tick marks rendered on the slider track, aligned with upstream IMSlider::draw_ticks.
            Repeater {
                model: root.previewVm ? root.previewVm.tickMarks : []
                delegate: Item {
                    // Tick mark position and interaction
                    readonly property real tickX: rangeSliderItem.trackMargin
                        + (root.totalLayers > 0
                           ? (modelData.tick / root.totalLayers) * rangeSliderItem.trackWidth
                           : 0)
                    readonly property int tickType: modelData.type
                    readonly property int tickLayer: modelData.tick

                    x: tickX - 1
                    y: rangeSliderItem.height / 2 - 8
                    width: 2
                    height: 16
                    z: 2

                    Rectangle {
                        anchors.fill: parent
                        radius: 1
                        color: {
                            // TickType: PausePrint=0, CustomGcode=1, Template=2, ToolChange=3, ColorChange=4
                            switch(tickType) {
                            case 0: return Theme.statusWarning    // PausePrint - orange
                            case 3: return Theme.statusInfo       // ToolChange - blue
                            case 4: return Theme.accent           // ColorChange - green
                            default: return Theme.textSecondary   // Custom/Template - gray
                            }
                        }
                    }

                    // Right-click on a tick mark shows the edit menu, aligned with upstream IMSlider.
                    MouseArea {
                        anchors.fill: parent
                        anchors.margins: -4
                        acceptedButtons: Qt.RightButton
                        onClicked: {
                            root.editMenuTickLayer = tickLayer
                            root.editMenuTickType = tickType
                            sliderEditMenu.popup()
                        }
                    }
                }
            }

            // Non-selected range dimming aligned with the upstream IMSlider groove.
            // Groove hover tooltip aligned with upstream IMSlider Z-height hover labels.
            Rectangle {
                id: grooveHoverTooltip
                x: {
                    if (!grooveHoverMA.containsMouse || root.totalLayers <= 0) return -200
                    var relX = grooveHoverMA.mouseX - rangeSliderItem.trackMargin
                    return grooveHoverMA.mouseX - width / 2
                }
                y: -28
                width: grooveHoverTipCol.implicitWidth + 16
                height: grooveHoverTipCol.implicitHeight + 10
                radius: 4
                color: Theme.bgTooltip
                border.width: 1
                border.color: Theme.borderSubtle
                visible: grooveHoverMA.containsMouse && root.totalLayers > 0

                Column {
                    id: grooveHoverTipCol
                    anchors.centerIn: parent
                    spacing: 1
                    Text {
                        id: grooveHoverTipText
                        text: {
                            if (!grooveHoverMA.containsMouse || !root.previewVm || root.totalLayers <= 0) return ""
                            var relX = grooveHoverMA.mouseX - rangeSliderItem.trackMargin
                            var hoverLayer = Math.round((relX / rangeSliderItem.trackWidth) * root.totalLayers)
                            hoverLayer = Math.max(0, Math.min(hoverLayer, root.totalLayers))
                            var zHeight = root.previewVm.layerZAt(hoverLayer).toFixed(2)
                            return "L" + (hoverLayer + 1) + "  Z:" + zHeight
                        }
                        color: Theme.textPrimary
                        font.pixelSize: 10
                        font.family: "monospace"
                    }
                    // Show layer time on hover, aligned with upstream IMSlider::draw_tick_on_mouse_position.
                    Text {
                        text: {
                            if (!grooveHoverMA.containsMouse || !root.previewVm || root.totalLayers <= 0) return ""
                            var relX = grooveHoverMA.mouseX - rangeSliderItem.trackMargin
                            var hoverLayer = Math.round((relX / rangeSliderItem.trackWidth) * root.totalLayers)
                            hoverLayer = Math.max(0, Math.min(hoverLayer, root.totalLayers))
                            var lt = root.previewVm.layerTimeAt(hoverLayer)
                            if (lt <= 0) return ""
                            return "~" + lt.toFixed(1) + "s"
                        }
                        color: Theme.textSecondary
                        font.pixelSize: 9
                        font.family: "monospace"
                    }
                }

                // Arrow
                Rectangle {
                    anchors.top: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 5
                    height: 5
                    rotation: 45
                    color: Theme.bgTooltip
                }
            }

            // Groove interaction area: hover tooltip + click-to-jump + right-click add menu
            // Aligned with upstream IMSlider groove click and right-click add_menu behavior.
            MouseArea {
                id: grooveHoverMA
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                // Left-click on the groove jumps the nearest thumb to the clicked position.
                onClicked: function(mouse) {
                    if (!root.previewVm || root.totalLayers <= 0) return

                    if (mouse.button === Qt.RightButton) {
                        // Compute layer at click position for the add menu
                        var relX = mouse.x - rangeSliderItem.trackMargin
                        var clickedLayer = Math.round((relX / rangeSliderItem.trackWidth) * root.totalLayers)
                        clickedLayer = Math.max(0, Math.min(clickedLayer, root.totalLayers))
                        addMenuTargetLayer = clickedLayer
                        sliderAddMenu.popup()
                        return
                    }

                    // Left-click moves the nearest thumb.
                    var relX2 = mouse.x - rangeSliderItem.trackMargin
                    var clickedLayer2 = Math.round((relX2 / rangeSliderItem.trackWidth) * root.totalLayers)
                    clickedLayer2 = Math.max(0, Math.min(clickedLayer2, root.totalLayers))
                    var distMin = Math.abs(clickedLayer2 - root.previewVm.currentLayerMin)
                    var distMax = Math.abs(clickedLayer2 - root.previewVm.currentLayerMax)
                    if (distMin <= distMax)
                        root.previewVm.setLayerRange(clickedLayer2, root.previewVm.currentLayerMax)
                    else
                        root.previewVm.setLayerRange(root.previewVm.currentLayerMin, clickedLayer2)
                }
                // Propagate hover to tooltip (prevent blocking thumb drag)
                z: -1
            }

            Rectangle {
                x: rangeSliderItem.trackMargin
                y: rangeSliderItem.height / 2 - 3
                width: root.totalLayers > 0
                    ? (root.previewVm.currentLayerMin / root.totalLayers) * rangeSliderItem.trackWidth
                    : 0
                height: 6
                radius: 3
                color: Theme.bgPanel
                opacity: 0.5
            }
            Rectangle {
                x: rangeSliderItem.trackMargin + (root.totalLayers > 0
                     ? (root.previewVm.currentLayerMax / root.totalLayers) * rangeSliderItem.trackWidth
                     : 0)
                y: rangeSliderItem.height / 2 - 3
                width: root.totalLayers > 0
                    ? ((root.totalLayers - root.previewVm.currentLayerMax) / root.totalLayers) * rangeSliderItem.trackWidth
                    : 0
                height: 6
                radius: 3
                color: Theme.bgPanel
                opacity: 0.5
            }

            // Min thumb
            Rectangle {
                id: minThumb
                x: rangeSliderItem.trackMargin + (root.totalLayers > 0
                     ? (root.previewVm.currentLayerMin / root.totalLayers) * rangeSliderItem.trackWidth
                     : 0) - rangeSliderItem.thumbWidth / 2
                y: rangeSliderItem.height / 2 - 10
                width: rangeSliderItem.thumbWidth
                height: 20
                radius: 4
                color: rangeSliderItem.dragThumb === 0 ? Theme.accent : Theme.textSecondary
                border.width: 1
                border.color: rangeSliderItem.dragThumb === 0 ? Theme.accent : Theme.textTertiary
                Behavior on x { NumberAnimation { duration: 50 } }
                Behavior on color { ColorAnimation { duration: 100 } }

                Text {
                    anchors.centerIn: parent
                    text: "|"
                    color: rangeSliderItem.dragThumb === 0 ? "#fff" : Theme.textDisabled
                    font.pixelSize: 14
                    font.bold: true
                }

                MouseArea {
                    id: minThumbMA
                    anchors.fill: parent
                    anchors.margins: -4  // slightly larger grab area
                    hoverEnabled: true
                    drag.target: minThumb
                    drag.axis: Drag.XAxis
                    drag.minimumX: rangeSliderItem.trackMargin - rangeSliderItem.thumbWidth / 2
                    drag.maximumX: rangeSliderItem.trackMargin + rangeSliderItem.trackWidth - rangeSliderItem.thumbWidth / 2
                    cursorShape: Qt.SizeHorCursor
                    onPressed: rangeSliderItem.dragThumb = 0
                    onReleased: rangeSliderItem.dragThumb = -1
                    onPositionChanged: {
                        if (!root.previewVm || root.totalLayers <= 0) return
                        var relX = minThumb.x + rangeSliderItem.thumbWidth / 2 - rangeSliderItem.trackMargin
                        var newMin = Math.round((relX / rangeSliderItem.trackWidth) * root.totalLayers)
                        newMin = Math.max(0, Math.min(newMin, root.previewVm.currentLayerMax))
                        root.previewVm.setLayerRange(newMin, root.previewVm.currentLayerMax)
                    }
                }

                ToolTip.visible: minThumbMA.containsMouse
                ToolTip.delay: 300
                ToolTip.text: qsTr("起始: %1 (Z: %2 mm)").arg(root.displayMin)
                            .arg(root.previewVm ? root.previewVm.layerZAt(Math.max(0, root.previewVm.currentLayerMin)).toFixed(2) : "0.00")
            }

            // Max thumb
            Rectangle {
                id: maxThumb
                x: rangeSliderItem.trackMargin + (root.totalLayers > 0
                     ? (root.previewVm.currentLayerMax / root.totalLayers) * rangeSliderItem.trackWidth
                     : 0) - rangeSliderItem.thumbWidth / 2
                y: rangeSliderItem.height / 2 - 10
                width: rangeSliderItem.thumbWidth
                height: 20
                radius: 4
                color: rangeSliderItem.dragThumb === 1 ? Theme.accent : Theme.textSecondary
                border.width: 1
                border.color: rangeSliderItem.dragThumb === 1 ? Theme.accent : Theme.textTertiary
                Behavior on x { NumberAnimation { duration: 50 } }
                Behavior on color { ColorAnimation { duration: 100 } }

                Text {
                    anchors.centerIn: parent
                    text: "|"
                    color: rangeSliderItem.dragThumb === 1 ? "#fff" : Theme.textDisabled
                    font.pixelSize: 14
                    font.bold: true
                }

                MouseArea {
                    id: maxThumbMA
                    anchors.fill: parent
                    anchors.margins: -4
                    drag.target: maxThumb
                    drag.axis: Drag.XAxis
                    drag.minimumX: rangeSliderItem.trackMargin - rangeSliderItem.thumbWidth / 2
                    drag.maximumX: rangeSliderItem.trackMargin + rangeSliderItem.trackWidth - rangeSliderItem.thumbWidth / 2
                    cursorShape: Qt.SizeHorCursor
                    onPressed: rangeSliderItem.dragThumb = 1
                    onReleased: rangeSliderItem.dragThumb = -1
                    onPositionChanged: {
                        if (!root.previewVm || root.totalLayers <= 0) return
                        var relX = maxThumb.x + rangeSliderItem.thumbWidth / 2 - rangeSliderItem.trackMargin
                        var newMax = Math.round((relX / rangeSliderItem.trackWidth) * root.totalLayers)
                        newMax = Math.min(root.totalLayers, Math.max(newMax, root.previewVm.currentLayerMin))
                        root.previewVm.setLayerRange(root.previewVm.currentLayerMin, newMax)
                    }
                }

                ToolTip.visible: maxThumbMA.containsMouse
                ToolTip.delay: 300
                ToolTip.text: qsTr("结束: %1 (Z: %2 mm)").arg(root.displayMax)
                            .arg(root.previewVm ? root.previewVm.layerZAt(Math.max(0, root.previewVm.currentLayerMax)).toFixed(2) : "0.00")
            }
        }

        // Range inputs and summary row.
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Label { text: qsTr("从"); color: Theme.textSecondary; font.pixelSize: 10 }
            CxTextField {
                id: minInput
                Layout.preferredWidth: 50
                Layout.preferredHeight: 24
                text: root.displayMin
                font.pixelSize: 11
                horizontalAlignment: TextInput.AlignHCenter
                validator: IntValidator { bottom: 1; top: root.maxDisplay }
                onEditingFinished: {
                    if (root.previewVm) {
                        var v = parseInt(text) || 1
                        root.previewVm.setLayerRange(v - 1, root.previewVm.currentLayerMax)
                    }
                }
                onActiveFocusChanged: if (activeFocus) selectAll()
            }

            Label { text: "-"; color: Theme.textDisabled; font.pixelSize: 12 }

            Label { text: qsTr("到"); color: Theme.textSecondary; font.pixelSize: 10 }
            CxTextField {
                id: maxInput
                Layout.preferredWidth: 50
                Layout.preferredHeight: 24
                text: root.displayMax
                font.pixelSize: 11
                horizontalAlignment: TextInput.AlignHCenter
                validator: IntValidator { bottom: 1; top: root.maxDisplay }
                onEditingFinished: {
                    if (root.previewVm) {
                        var v = parseInt(text) || 1
                        root.previewVm.setLayerRange(root.previewVm.currentLayerMin, v - 1)
                    }
                }
                onActiveFocusChanged: if (activeFocus) selectAll()
            }

            Item { Layout.fillWidth: true }
            Label {
                text: "/ " + root.maxDisplay
                color: Theme.textDisabled
                font.pixelSize: 11
            }
        }

        // Jump-to-layer control aligned with upstream IMSlider::render_go_to_layer_dialog.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            radius: 6
            color: jumpMA.containsMouse ? Theme.bgHover : Theme.bgPanel
            border.color: jumpMA.containsMouse ? Theme.accentDark : Theme.borderSubtle
            border.width: 1

            RowLayout {
                anchors.centerIn: parent
                spacing: 6
                Label {
                    text: qsTr("跳到层")
                    color: jumpMA.containsMouse ? Theme.accentLight : Theme.textSecondary
                    font.pixelSize: 11
                }
            }

            MouseArea {
                id: jumpMA
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    jumpDialog.open()
                    jumpInput.text = ""
                    jumpInput.forceActiveFocus()
                }
            }

            // Right-click context menu on jump button
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: jumpCtxMenu.popup()
            }
            CxMenu {
                id: jumpCtxMenu
                CxMenuItem {
                    text: qsTr("跳到层...")
                    onTriggered: {
                        jumpDialog.open()
                        jumpInput.text = ""
                        jumpInput.forceActiveFocus()
                    }
                }
            }
        }

        // Jump dialog.
        Dialog {
            id: jumpDialog
            anchors.centerIn: parent.parent
            modal: true
            title: qsTr("跳到层")
            padding: 16

            background: Rectangle {
                radius: 12
                color: Theme.bgElevated
                border.color: Theme.borderSubtle
                border.width: 1
            }

            header: Label {
                text: qsTr("跳到层")
                color: Theme.textPrimary
                font.bold: true
                font.pixelSize: 14
                padding: 8
            }

            ColumnLayout {
                spacing: 12

                Label {
                    text: qsTr("层 (1-%1):").arg(root.maxDisplay)
                    color: Theme.textSecondary
                    font.pixelSize: 12
                }

                CxTextField {
                    id: jumpInput
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 32
                    font.pixelSize: 13
                    horizontalAlignment: TextInput.AlignHCenter
                    validator: IntValidator { bottom: 1; top: root.maxDisplay }
                    Keys.onReturnPressed: doJump()
                    Keys.onEnterPressed: doJump()
                    Keys.onEscapePressed: jumpDialog.close()
                    Component.onCompleted: forceActiveFocus()
                }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: 8

                    CxButton {
                        text: qsTr("取消")
                        onClicked: jumpDialog.close()
                    }
                    CxButton {
                        text: qsTr("确定")
                        highlighted: true
                        onClicked: doJump()
                    }
                }
            }

            function doJump() {
                if (root.previewVm) {
                    var v = parseInt(jumpInput.text)
                    if (!isNaN(v) && v >= 1 && v <= root.maxDisplay) {
                        root.previewVm.jumpToLayer(v)
                        jumpDialog.close()
                    }
                }
            }
        }

        // Slider add menu aligned with upstream IMSlider::render_add_menu.
        // Shown on right-click on slider groove (empty area)
        CxMenu {
            id: sliderAddMenu

            CxMenuItem {
                text: qsTr("添加暂停")
                onTriggered: {
                    if (root.previewVm && root.addMenuTargetLayer >= 0)
                        root.previewVm.addPauseAtLayer(root.addMenuTargetLayer)
                }
            }
            CxMenuItem {
                text: qsTr("添加自定义 G-code...")
                onTriggered: {
                    customGcodeAddDialog.targetLayer = root.addMenuTargetLayer
                    customGcodeAddDialog.gcodeText = ""
                    customGcodeAddDialog.open()
                }
            }
            CxMenuItem {
                text: qsTr("跳到层...")
                onTriggered: {
                    jumpDialog.open()
                    jumpInput.text = ""
                    jumpInput.forceActiveFocus()
                }
            }
        }

        // Slider edit menu aligned with upstream IMSlider::render_edit_menu.
        // Shown on right-click on existing tick mark
        CxMenu {
            id: sliderEditMenu

            // PausePrint tick (type 0)
            CxMenuItem {
                text: qsTr("删除暂停")
                visible: root.editMenuTickType === 0
                onTriggered: {
                    if (root.previewVm && root.editMenuTickLayer >= 0)
                        root.previewVm.removeTickAtLayer(root.editMenuTickLayer)
                }
            }

            // Template tick (type 2)
            CxMenuItem {
                text: qsTr("删除自定义模板")
                visible: root.editMenuTickType === 2
                onTriggered: {
                    if (root.previewVm && root.editMenuTickLayer >= 0)
                        root.previewVm.removeTickAtLayer(root.editMenuTickLayer)
                }
            }

            // CustomGcode tick (type 1)
            CxMenuItem {
                text: qsTr("编辑自定义 G-code")
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
                text: qsTr("删除自定义 G-code")
                visible: root.editMenuTickType === 1
                onTriggered: {
                    if (root.previewVm && root.editMenuTickLayer >= 0)
                        root.previewVm.removeTickAtLayer(root.editMenuTickLayer)
                }
            }

            // ToolChange tick (type 3)
            CxMenuItem {
                text: qsTr("删除换料")
                visible: root.editMenuTickType === 3
                onTriggered: {
                    if (root.previewVm && root.editMenuTickLayer >= 0)
                        root.previewVm.removeTickAtLayer(root.editMenuTickLayer)
                }
            }

            // ColorChange tick (type 4)
            CxMenuItem {
                text: qsTr("删除换色")
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
            anchors.centerIn: parent.parent
        }

        // Custom G-code edit dialog.
        CustomGcodeDialog {
            id: customGcodeEditDialog
            previewVm: root.previewVm
            dialogTitle: qsTr("编辑自定义 G-code")
            isEditMode: true
            anchors.centerIn: parent.parent
        }

        Item { Layout.fillHeight: true }
    }
}
