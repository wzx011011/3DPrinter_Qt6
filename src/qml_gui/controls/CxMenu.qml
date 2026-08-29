import QtQuick
import QtQuick.Controls
import ".."

// popupType=Item: render the menu inside the scene graph instead of a
// native OS popup window. Since Qt 6.8 Windows defaults Menus to
// Popup.Native, which mispositions/hides the popup on the frameless
// self-drawn shell window (observed: menu opened at an off-screen rect
// and never became visible).
Menu {
    id: root

    popupType: Popup.Item

    background: Rectangle {
        color: Theme.menuBackground
        border.color: Theme.borderDefault
        border.width: 1
        radius: Theme.radiusSM
    }
}
