import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

Item {
    id: root
    required property var previewVm

    RowLayout {
        anchors.fill: parent
        spacing: Theme.spacingMD

        CxButton {
            text: qsTr("播放")
            onClicked: root.previewVm.playAnimation()
        }

        CxButton {
            text: qsTr("暂停")
            onClicked: root.previewVm.pauseAnimation()
        }

        CxSlider {
            id: moveSlider
            Layout.fillWidth: true
            from: 0
            to: Math.max(0, root.previewVm.moveCount)
            stepSize: 1
            value: root.previewVm.currentMove
            onMoved: root.previewVm.setCurrentMove(Math.round(value))
        }

        Label {
            text: root.previewVm.currentMove + " / " + root.previewVm.moveCount
            color: Theme.textPrimary
            font.pixelSize: 12
        }
    }
}
