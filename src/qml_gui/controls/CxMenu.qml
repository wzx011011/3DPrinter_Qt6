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

    // Upstream dropdown metrics (BBL Topbar file menu): compact 28px-ish rows,
    // thin visible separators, tight vertical padding.
    topPadding: 4
    bottomPadding: 4
    // Separator line: palette.mid pinned to Theme.separator is near-invisible
    // on the dark menu panel; use the subtle border tone so group separators
    // read like upstream's thin gray lines.
    palette.mid: Theme.borderSubtle

    // Qt 6.10 Menu does not derive popup width from custom-delegate items on
    // this shell (observed: menu opens with width 0 — invisible, eats the
    // next click). Size it from the widest enabled item instead; upstream
    // menus size the same way (widest entry rules the row width).
    width: {
        let maxW = 0
        for (let i = 0; i < count; ++i) {
            const it = itemAt(i)
            if (it && it.visible && it.implicitWidth > maxW)
                maxW = it.implicitWidth
        }
        return maxW + leftPadding + rightPadding
    }


    // U08: separator color. The Basic MenuSeparator paints its 1px line with
    // palette.mid; pinned to the subtle border tone (was Theme.separator, a
    // near-invisible near-black on the dark menu panel) so group separators
    // read like upstream's thin gray lines.
    background: Rectangle {
        color: Theme.menuBackground
        border.color: Theme.borderDefault
        border.width: 1
        radius: Theme.radiusSM
    }
}
