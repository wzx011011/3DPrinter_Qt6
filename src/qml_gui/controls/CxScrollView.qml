import QtQuick
import QtQuick.Controls
import OWzx

ScrollView {
    id: root

    // R13 (restoration-map.md:43): slot #171717 (scrollBarTrackColor) /
    // thumb #959595 (scrollBarColor). The active thumb renders at full
    // opacity so the on-screen value matches the sampled token exactly
    // (0.8 over the #171717 slot blended to ~#7b7b7b).
    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AsNeeded
        // Explicit anchors: ScrollView only auto-positions its own default
        // bars; custom attached bars landed at the content's top-left in the
        // wizard (2026-10-09), painting a thumb over the page title.
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        background: Rectangle {
            implicitWidth: 8
            radius: Theme.radiusSM
            color: Theme.scrollBarTrackColor
        }
        contentItem: Rectangle {
            implicitWidth: 8
            implicitHeight: 100
            radius: Theme.radiusSM
            opacity: parent.active ? 1.0 : 0.5
            color: Theme.scrollBarColor
            Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
        }
    }

    ScrollBar.horizontal: ScrollBar {
        policy: ScrollBar.AsNeeded
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        background: Rectangle {
            implicitHeight: 8
            radius: Theme.radiusSM
            color: Theme.scrollBarTrackColor
        }
        contentItem: Rectangle {
            implicitWidth: 100
            implicitHeight: 8
            radius: Theme.radiusSM
            opacity: parent.active ? 1.0 : 0.5
            color: Theme.scrollBarColor
            Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }
        }
    }
}
