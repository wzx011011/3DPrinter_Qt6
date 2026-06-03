import QtQuick
import QtQuick.Controls
import ".."

ProgressBar {
    id: root

    property int barHeight: 6
    property color fillColor: Theme.progressFill
    property color trackColor: Theme.progressTrack
    property bool animateWidth: true

    background: Rectangle {
        width: root.availableWidth
        height: root.barHeight
        radius: root.barHeight / 2
        color: root.trackColor
        opacity: root.enabled ? 1.0 : 0.45
    }

    contentItem: Rectangle {
        width: root.visualPosition * root.availableWidth
        height: root.barHeight
        radius: root.barHeight / 2
        color: root.fillColor

        Behavior on width {
            enabled: root.animateWidth
            NumberAnimation { duration: 150; easing.type: Easing.OutCubic }
        }
    }
}
