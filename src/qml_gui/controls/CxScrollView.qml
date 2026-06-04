import QtQuick
import QtQuick.Controls
import ".."

ScrollView {
    id: root

    ScrollBar.vertical: ScrollBar {
        policy: ScrollBar.AsNeeded
        contentItem: Rectangle {
            implicitWidth: 8
            implicitHeight: 100
            radius: 4
            opacity: parent.active ? 0.8 : 0.5
            color: Theme.bgPressed
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }
    }

    ScrollBar.horizontal: ScrollBar {
        policy: ScrollBar.AsNeeded
        contentItem: Rectangle {
            implicitWidth: 100
            implicitHeight: 8
            radius: 4
            opacity: parent.active ? 0.8 : 0.5
            color: Theme.bgPressed
            Behavior on opacity { NumberAnimation { duration: 150 } }
        }
    }
}
