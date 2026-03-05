import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var previewVm

    RowLayout {
        anchors.fill: parent
        spacing: 8

        Button {
            text: qsTr("播放")
            onClicked: root.previewVm.playAnimation()
        }

        Button {
            text: qsTr("暂停")
            onClicked: root.previewVm.pauseAnimation()
        }

        Slider {
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
            color: "#dfe6ef"
            font.pixelSize: 12
        }
    }
}
