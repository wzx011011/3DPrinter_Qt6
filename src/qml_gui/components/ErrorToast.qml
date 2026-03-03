import QtQuick
import QtQuick.Layouts

// Info-level (severity=0) toast — slides up from bottom, auto-dismisses in 3 s
Item {
    id: root

    // Only show for Info-level messages
    readonly property bool shouldShow: backend.lastErrorSeverity === 0
                                       && backend.lastErrorMessage !== ""

    visible: shouldShow || slideAnim.running
    anchors.bottom: parent.bottom
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottomMargin: 44
    width:  toastLabel.implicitWidth + 56
    height: 36
    z:      200

    onShouldShowChanged: {
        if (shouldShow) {
            root.opacity = 0
            root.y = 20
            slideAnim.restart()
            hideTimer.restart()
        }
    }

    Timer {
        id: hideTimer
        interval: 3000
        onTriggered: backend.clearError()
    }

    NumberAnimation on opacity { id: slideAnim; to: 1; from: 0; duration: 220; easing.type: Easing.OutCubic }

    Rectangle {
        anchors.fill: parent
        radius:        18
        color:         "#1a2e20"
        border.color:  "#18c75e"
        border.width:  1

        RowLayout {
            anchors.centerIn: parent
            spacing: 8

            Text { text: "✓"; color: "#18c75e"; font.pixelSize: 13; font.bold: true }
            Text {
                id: toastLabel
                text:            backend.lastErrorMessage
                color:           "#d4efe0"
                font.pixelSize:  12
            }
        }
    }
}
