import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

/// 工具位置提示框（对齐上游 GCodeViewer::Marker::render 的 ExtruderPosition 窗口）
/// 在预览页面播放/手动拖拽移动时，显示当前工具位置和移动属性
Rectangle {
    id: root
    required property var previewVm

    visible: root.previewVm && root.previewVm.hasToolPosition
    width: tooltipLayout.implicitWidth + 20
    height: tooltipLayout.implicitHeight + 16
    radius: 8
    color: "#0d1117"
    border.color: "#30363d"
    border.width: 1
    opacity: visible ? 0.92 : 0

    Behavior on opacity { NumberAnimation { duration: 150 } }

    ColumnLayout {
        id: tooltipLayout
        anchors.fill: parent
        anchors.margins: 8
        spacing: 3

        // 标题行
        RowLayout {
            spacing: 6
            Rectangle {
                width: 8; height: 8; radius: 4
                color: root.previewVm && root.previewVm.toolIsExtrusion ? "#18c75e" : "#f0883e"
            }
            Label {
                text: root.previewVm && root.previewVm.toolIsExtrusion
                      ? qsTr("挤出移动") : qsTr("空驶移动")
                color: Theme.textPrimary
                font.bold: true
                font.pixelSize: 11
            }
            Item { Layout.fillWidth: true }
            Label {
                text: root.previewVm ? ("#" + root.previewVm.toolMoveIndex) : ""
                color: Theme.textTertiary
                font.pixelSize: 10
            }
        }

        // 位置
        Row {
            spacing: 10
            Label { text: "X: " + (root.previewVm ? root.previewVm.toolX.toFixed(2) : "0.00"); color: "#8b949e"; font.pixelSize: 11; font.family: "Consolas, monospace" }
            Label { text: "Y: " + (root.previewVm ? root.previewVm.toolY.toFixed(2) : "0.00"); color: "#8b949e"; font.pixelSize: 11; font.family: "Consolas, monospace" }
            Label { text: "Z: " + (root.previewVm ? root.previewVm.toolZ.toFixed(2) : "0.00"); color: "#8b949e"; font.pixelSize: 11; font.family: "Consolas, monospace" }
        }

        // 速度
        Row {
            spacing: 10
            Label { text: qsTr("速度: ") + (root.previewVm ? root.previewVm.toolFeedrate.toFixed(0) : "0") + " mm/s"; color: "#8b949e"; font.pixelSize: 11; font.family: "Consolas, monospace" }
            Label { text: qsTr("层: ") + (root.previewVm ? (root.previewVm.toolLayer + 1) : 0); color: "#8b949e"; font.pixelSize: 11; font.family: "Consolas, monospace" }
        }

        // 视图模式特定字段（对齐上游 Marker::render 的条件显示）
        Row {
            spacing: 10
            visible: root.previewVm && root.previewVm.toolFanSpeed > 0
            Label { text: qsTr("风扇: ") + (root.previewVm ? root.previewVm.toolFanSpeed.toFixed(0) : "0") + "%"; color: "#8b949e"; font.pixelSize: 11; font.family: "Consolas, monospace" }
        }

        Row {
            spacing: 10
            visible: root.previewVm && root.previewVm.toolTemperature > 0
            Label { text: qsTr("温度: ") + (root.previewVm ? root.previewVm.toolTemperature.toFixed(0) : "0") + "°C"; color: "#8b949e"; font.pixelSize: 11; font.family: "Consolas, monospace" }
        }

        Row {
            spacing: 10
            visible: root.previewVm && root.previewVm.toolWidth > 0
            Label { text: qsTr("线宽: ") + (root.previewVm ? root.previewVm.toolWidth.toFixed(2) : "0.00") + "mm"; color: "#8b949e"; font.pixelSize: 11; font.family: "Consolas, monospace" }
        }

        Row {
            spacing: 10
            visible: root.previewVm && root.previewVm.toolAcceleration > 0
            Label { text: qsTr("加速度: ") + (root.previewVm ? root.previewVm.toolAcceleration.toFixed(0) : "0"); color: "#8b949e"; font.pixelSize: 11; font.family: "Consolas, monospace" }
        }

        Row {
            spacing: 10
            visible: root.previewVm && root.previewVm.toolExtruderId > 0
            Label { text: qsTr("挤出机: ") + (root.previewVm ? (root.previewVm.toolExtruderId + 1) : 1); color: "#8b949e"; font.pixelSize: 11; font.family: "Consolas, monospace" }
        }
    }
}
