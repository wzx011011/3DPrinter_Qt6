import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

Item {
    id: root
    required property var previewVm

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingSM

        Label { text: qsTr("层范围"); color: Theme.textPrimary; font.bold: true; font.pixelSize: Theme.fontSizeLG }

        CxSlider {
            id: minSlider
            Layout.fillWidth: true
            from: 0
            to: Math.max(0, root.previewVm.layerCount - 1)
            stepSize: 1
            value: root.previewVm.currentLayerMin
            onMoved: root.previewVm.setLayerRange(Math.round(value), root.previewVm.currentLayerMax)
        }

        CxSlider {
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
            color: Theme.textSecondary
            font.pixelSize: 12
        }
    }
}
