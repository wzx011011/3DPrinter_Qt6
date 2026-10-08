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

    // U08: separator color. The Basic MenuSeparator paints its 1px line with
    // palette.mid, which resolves to a light gray on this dark theme (light
    // line landing on the dark menu). Pin the mid role to the global
    // separator token (1px, Theme.separator from U01) so every separator
    // inside a CxMenu inherits the dark upstream look; item highlight/press
    // colors use other palette roles and are unaffected.
    palette.mid: Theme.separator

    background: Rectangle {
        color: Theme.menuBackground
        border.color: Theme.borderDefault
        border.width: 1
        radius: Theme.radiusSM
    }
}
