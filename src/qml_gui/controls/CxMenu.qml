import QtQuick
import QtQuick.Controls
import ".."

// popupType=Item: render the menu inside the scene graph instead of a
// native OS popup window. Since Qt 6.8 Windows defaults Menus to
// Popup.Native, which mispositions/hides the popup on the frameless
// self-drawn shell window (observed: menu opened at an off-screen rect
// and never became visible).
//
// Sizing mirrors the qtquickcontrols2 Basic Menu template: its background
// carries implicitWidth 200 and the control binds implicitWidth to
// max(background, content). Our custom background Rectangle has no
// intrinsic size, so without restoring that floor the popup collapses to
// width 0 once opened (observed: File menu opened with w=0, h=294 and
// stayed invisible).
Menu {
    id: root

    popupType: Popup.Item

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset,
                            implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset,
                             implicitContentHeight + topPadding + bottomPadding)

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 40
        color: Theme.menuBackground
        border.color: Theme.borderDefault
        border.width: 1
        radius: Theme.radiusSM
    }
}
