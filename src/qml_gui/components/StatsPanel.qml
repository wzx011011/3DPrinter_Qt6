import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var previewVm

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        Label { text: qsTr("统计"); color: "#dfe6ef" }
        Label { text: qsTr("总时间: ") + root.previewVm.totalTime; color: "#cdd5e2"; font.pixelSize: 12 }
        Label { text: qsTr("耗材: ") + root.previewVm.filamentUsed; color: "#cdd5e2"; font.pixelSize: 12 }
        Label { text: qsTr("层数: ") + root.previewVm.layerCount; color: "#cdd5e2"; font.pixelSize: 12 }
    }
}
