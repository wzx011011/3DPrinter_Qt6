import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

Item {
    id: root
    required property var previewVm

    // 上游 IMSlider 使用 1-indexed 层号显示
    property int displayMin: root.previewVm ? root.previewVm.currentLayerMin + 1 : 1
    property int displayMax: root.previewVm ? root.previewVm.currentLayerMax + 1 : 1
    property int maxDisplay: root.previewVm ? Math.max(1, root.previewVm.layerCount) : 1
    property int totalLayers: root.previewVm ? Math.max(0, root.previewVm.layerCount - 1) : 0

    // 鼠标滚轮改变层范围（对齐上游 IMSlider::on_mouse_wheel）
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

        Label { text: qsTr("Layer Range"); color: Theme.textPrimary; font.bold: true; font.pixelSize: Theme.fontSizeLG }

        // ── Dual-thumb range slider (对齐上游 IMSlider 双拇指层范围拖拽) ──
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
                color: "#252b38"
            }

            // Active range track (between two thumbs)
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
                color: rangeSliderItem.dragThumb === 0 ? Theme.accent : "#c0d0e0"
                border.width: 1
                border.color: rangeSliderItem.dragThumb === 0 ? Theme.accent : "#8090a0"
                Behavior on x { NumberAnimation { duration: 50 } }
                Behavior on color { ColorAnimation { duration: 100 } }

                Text {
                    anchors.centerIn: parent
                    text: "|"
                    color: rangeSliderItem.dragThumb === 0 ? "#fff" : "#506070"
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
                ToolTip.text: qsTr("Start: %1 (Z: %2 mm)").arg(root.displayMin)
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
                color: rangeSliderItem.dragThumb === 1 ? Theme.accent : "#c0d0e0"
                border.width: 1
                border.color: rangeSliderItem.dragThumb === 1 ? Theme.accent : "#8090a0"
                Behavior on x { NumberAnimation { duration: 50 } }
                Behavior on color { ColorAnimation { duration: 100 } }

                Text {
                    anchors.centerIn: parent
                    text: "|"
                    color: rangeSliderItem.dragThumb === 1 ? "#fff" : "#506070"
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
                ToolTip.text: qsTr("End: %1 (Z: %2 mm)").arg(root.displayMax)
                            .arg(root.previewVm ? root.previewVm.layerZAt(Math.max(0, root.previewVm.currentLayerMax)).toFixed(2) : "0.00")
            }
        }

        // ── Range inputs + summary row ──
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Label { text: qsTr("From"); color: Theme.textSecondary; font.pixelSize: 10 }
            TextField {
                id: minInput
                Layout.preferredWidth: 50
                Layout.preferredHeight: 24
                text: root.displayMin
                color: Theme.textPrimary
                font.pixelSize: 11
                horizontalAlignment: TextInput.AlignHCenter
                validator: IntValidator { bottom: 1; top: root.maxDisplay }
                background: Rectangle {
                    radius: 4
                    color: "#1e2229"
                    border.color: minInput.activeFocus ? Theme.accent : "#2e3540"
                    border.width: 1
                }
                onEditingFinished: {
                    if (root.previewVm) {
                        var v = parseInt(text) || 1
                        root.previewVm.setLayerRange(v - 1, root.previewVm.currentLayerMax)
                    }
                }
                onActiveFocusChanged: if (activeFocus) selectAll()
            }

            Label { text: "-"; color: Theme.textDisabled; font.pixelSize: 12 }

            Label { text: qsTr("To"); color: Theme.textSecondary; font.pixelSize: 10 }
            TextField {
                id: maxInput
                Layout.preferredWidth: 50
                Layout.preferredHeight: 24
                text: root.displayMax
                color: Theme.textPrimary
                font.pixelSize: 11
                horizontalAlignment: TextInput.AlignHCenter
                validator: IntValidator { bottom: 1; top: root.maxDisplay }
                background: Rectangle {
                    radius: 4
                    color: "#1e2229"
                    border.color: maxInput.activeFocus ? Theme.accent : "#2e3540"
                    border.width: 1
                }
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

        // ── Jump to Layer (对齐上游 IMSlider::render_go_to_layer_dialog) ──
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            radius: 6
            color: jumpMA.containsMouse ? "#2a3545" : "#1e2229"
            border.color: jumpMA.containsMouse ? Theme.accentDark : "#2e3540"
            border.width: 1

            RowLayout {
                anchors.centerIn: parent
                spacing: 6
                Label {
                    text: qsTr("Jump to Layer")
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
            Menu {
                id: jumpCtxMenu
                MenuItem {
                    text: qsTr("Jump to Layer...")
                    onTriggered: {
                        jumpDialog.open()
                        jumpInput.text = ""
                        jumpInput.forceActiveFocus()
                    }
                }
            }
        }

        // ── Jump Dialog ──
        Dialog {
            id: jumpDialog
            anchors.centerIn: parent.parent
            modal: true
            title: qsTr("Jump to Layer")
            padding: 16

            background: Rectangle {
                radius: 12
                color: Theme.bgElevated
                border.color: Theme.borderSubtle
                border.width: 1
            }

            header: Label {
                text: qsTr("Jump to Layer")
                color: Theme.textPrimary
                font.bold: true
                font.pixelSize: 14
                padding: 8
            }

            ColumnLayout {
                spacing: 12

                Label {
                    text: qsTr("Layer (1-%1):").arg(root.maxDisplay)
                    color: Theme.textSecondary
                    font.pixelSize: 12
                }

                TextField {
                    id: jumpInput
                    Layout.preferredWidth: 120
                    Layout.preferredHeight: 32
                    color: Theme.textPrimary
                    font.pixelSize: 13
                    horizontalAlignment: TextInput.AlignHCenter
                    validator: IntValidator { bottom: 1; top: root.maxDisplay }
                    background: Rectangle {
                        radius: 6
                        color: "#1e2229"
                        border.color: jumpInput.activeFocus ? Theme.accent : "#2e3540"
                        border.width: 1
                    }
                    Keys.onReturnPressed: doJump()
                    Keys.onEnterPressed: doJump()
                    Keys.onEscapePressed: jumpDialog.close()
                    Component.onCompleted: forceActiveFocus()
                }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: 8

                    CxButton {
                        text: qsTr("Cancel")
                        onClicked: jumpDialog.close()
                    }
                    CxButton {
                        text: qsTr("OK")
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

        Item { Layout.fillHeight: true }
    }
}
