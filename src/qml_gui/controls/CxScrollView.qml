import QtQuick
import QtQuick.Controls
import ".."

ScrollView {
    id: root

    // R13 (restoration-map.md:43): slot #171717 (scrollBarTrackColor) /
    // thumb #959595 (scrollBarColor). The active thumb renders at full
    // opacity so the on-screen value matches the sampled token exactly
    // (0.8 over the #171717 slot blended to ~#7b7b7b).
    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AsNeeded
        background: Rectangle {
            implicitWidth: 8
            radius: 4
            color: Theme.scrollBarTrackColor
        }
        contentItem: Rectangle {
            implicitWidth: 8
            implicitHeight: 100
            radius: 4
            opacity: parent.active ? 1.0 : 0.5
            color: Theme.scrollBarColor
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }
    }

    ScrollBar.horizontal: ScrollBar {
        policy: ScrollBar.AsNeeded
        background: Rectangle {
            implicitHeight: 8
            radius: 4
            color: Theme.scrollBarTrackColor
        }
        contentItem: Rectangle {
            implicitWidth: 100
            implicitHeight: 8
            radius: 4
            opacity: parent.active ? 1.0 : 0.5
            color: Theme.scrollBarColor
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }
    }
}
