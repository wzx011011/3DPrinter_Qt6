import QtQuick
import QtQuick.Controls
import ".."

Popup {
    id: root

    property int popupRadius: Theme.radiusLG

    background: Rectangle {
        color: Theme.bgElevated
        border.color: Theme.borderDefault
        border.width: 1
        radius: root.popupRadius
    }

    Overlay.modeless: Rectangle {
        color: Theme.overlayDim
    }
}
