import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Rectangle {
    id: root
    required property var previewVm

    visible: root.previewVm && root.previewVm.hasToolPosition
    width: tooltipLayout.implicitWidth + 20
    height: tooltipLayout.implicitHeight + 16
    radius: 6
    color: "#11151dcc"
    border.color: Theme.borderSubtle
    border.width: 1
    opacity: visible ? 0.94 : 0

    Behavior on opacity { NumberAnimation { duration: 120 } }

    ColumnLayout {
        id: tooltipLayout
        anchors.fill: parent
        anchors.margins: 8
        spacing: 4

        RowLayout {
            spacing: 6
            Rectangle {
                width: 8
                height: 8
                radius: 4
                color: root.previewVm && root.previewVm.toolIsExtrusion ? Theme.accent : Theme.statusWarning
            }
            Label {
                text: root.previewVm && root.previewVm.toolIsExtrusion ? qsTr("挤出移动") : qsTr("空驶移动")
                color: Theme.textPrimary
                font.bold: true
                font.pixelSize: Theme.fontSizeSM
            }
            Item { Layout.fillWidth: true }
            Label {
                text: root.previewVm ? "#" + root.previewVm.toolMoveIndex : ""
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeXS
            }
        }

        Row {
            spacing: 10
            Label { text: "X " + (root.previewVm ? root.previewVm.toolX.toFixed(2) : "0.00"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; font.family: Theme.fontMono }
            Label { text: "Y " + (root.previewVm ? root.previewVm.toolY.toFixed(2) : "0.00"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; font.family: Theme.fontMono }
            Label { text: "Z " + (root.previewVm ? root.previewVm.toolZ.toFixed(2) : "0.00"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; font.family: Theme.fontMono }
        }

        Row {
            spacing: 10
            Label { text: qsTr("速度 ") + (root.previewVm ? root.previewVm.toolFeedrate.toFixed(0) : "0") + " mm/s"; color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; font.family: Theme.fontMono }
            Label { text: qsTr("层 ") + (root.previewVm ? root.previewVm.toolLayer + 1 : 0); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; font.family: Theme.fontMono }
        }

        Row {
            spacing: 10
            visible: root.previewVm && root.previewVm.toolFanSpeed > 0
            Label { text: qsTr("风扇 ") + (root.previewVm ? root.previewVm.toolFanSpeed.toFixed(0) : "0") + "%"; color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; font.family: Theme.fontMono }
        }

        Row {
            spacing: 10
            visible: root.previewVm && root.previewVm.toolTemperature > 0
            Label { text: qsTr("温度 ") + (root.previewVm ? root.previewVm.toolTemperature.toFixed(0) : "0") + " C"; color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; font.family: Theme.fontMono }
        }

        Row {
            spacing: 10
            visible: root.previewVm && root.previewVm.toolWidth > 0
            Label { text: qsTr("线宽 ") + (root.previewVm ? root.previewVm.toolWidth.toFixed(2) : "0.00") + " mm"; color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; font.family: Theme.fontMono }
        }

        Row {
            spacing: 10
            visible: root.previewVm && root.previewVm.toolAcceleration > 0
            Label { text: qsTr("加速度 ") + (root.previewVm ? root.previewVm.toolAcceleration.toFixed(0) : "0"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; font.family: Theme.fontMono }
        }

        Row {
            spacing: 10
            visible: root.previewVm && root.previewVm.toolLayerTime > 0
            Label { text: qsTr("层耗时 ") + (root.previewVm ? root.previewVm.toolLayerTime.toFixed(1) : "0.0") + "s"; color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; font.family: Theme.fontMono }
        }

        Row {
            spacing: 10
            visible: root.previewVm && root.previewVm.toolExtruderId > 0
            Label { text: qsTr("挤出机 ") + (root.previewVm ? root.previewVm.toolExtruderId + 1 : 1); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; font.family: Theme.fontMono }
        }
    }
}
