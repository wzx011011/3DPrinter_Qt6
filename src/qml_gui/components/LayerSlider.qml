import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var previewVm

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label { text: qsTr("层范围"); color: "#dfe6ef" }

        Slider {
            id: minSlider
            Layout.fillWidth: true
            from: 0
            to: Math.max(0, root.previewVm.layerCount - 1)
            stepSize: 1
            value: root.previewVm.currentLayerMin
            onMoved: root.previewVm.setLayerRange(Math.round(value), root.previewVm.currentLayerMax)
        }

        Slider {
            id: maxSlider
            Layout.fillWidth: true
            from: 0
            to: Math.max(0, root.previewVm.layerCount - 1)
            stepSize: 1
            value: root.previewVm.currentLayerMax
            onMoved: root.previewVm.setLayerRange(root.previewVm.currentLayerMin, Math.round(value))
        }

        Label {
            text: root.previewVm.currentLayerMin + " - " + root.previewVm.currentLayerMax
            color: "#9aa4b3"
            font.pixelSize: 12
        }
    }
}
