import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: root
    required property var previewVm

    readonly property int totalLayers: root.previewVm ? root.previewVm.layerCount : 0
    readonly property int lastLayerIndex: Math.max(0, root.totalLayers - 1)

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

        RangeSlider {
            id: layerRangeSlider
            Layout.fillHeight: true
            Layout.preferredWidth: 30
            Layout.alignment: Qt.AlignHCenter
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
