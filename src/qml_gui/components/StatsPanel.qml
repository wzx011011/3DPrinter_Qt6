import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: root
    required property var previewVm

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.spacingSM

        Label { text: qsTr("统计"); color: Theme.textPrimary; font.bold: true; font.pixelSize: Theme.fontSizeLG }

        Rectangle {
            Layout.fillWidth: true
            radius: 12
            color: Theme.bgElevated
            border.width: 1
            border.color: Theme.borderSubtle

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 8
                Label { text: qsTr("总时间: ") + root.previewVm.totalTime; color: Theme.textPrimary; font.pixelSize: 12 }
                Label { text: qsTr("耗材: ") + root.previewVm.filamentUsed; color: Theme.textPrimary; font.pixelSize: 12 }
                Label { text: qsTr("层数: ") + root.previewVm.layerCount; color: Theme.textPrimary; font.pixelSize: 12 }
            }
        }
    }
}
