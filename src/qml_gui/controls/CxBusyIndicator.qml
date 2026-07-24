import QtQuick
import ".."

// CxBusyIndicator.qml - lightweight spinner for async operations (e.g. Emboss
// text generation in Phase 196). Added in Phase 194 (UI-01) as the shared
// control so no call site reinvents a spinner.
//
// Presentation only: bind `running` to a ViewModel Q_PROPERTY.
Item {
    id: root

    property bool running: false
    property color colorToken: Theme.accent
    property int size: 20
    property int lineCount: 8

    implicitWidth: root.size
    implicitHeight: root.size
    opacity: root.running ? 1.0 : 0.0
    visible: root.running

    RotationAnimation on rotation {
        running: root.running
        loops: Animation.Infinite
        duration: 900
        from: 0
        to: 360
    }

    // Spinning spokes: opaque near the leading edge, fading toward the tail.
    Repeater {
        model: root.lineCount
        delegate: Rectangle {
            x: root.width / 2 - width / 2
            y: 0
            width: 2
            height: root.size / 2
            radius: 1
            color: root.colorToken
            transformOrigin: Item.Bottom
            rotation: index * (360 / root.lineCount)
            opacity: 0.25 + 0.75 * ((index + 1) / root.lineCount)
        }
    }
}
