pragma Singleton
import QtQuick

QtObject {
    // ── Background palette
    readonly property color bgBase:      "#0d0f12"
    readonly property color bgSurface:   "#131720"
    readonly property color bgPanel:     "#1a1e28"
    readonly property color bgCard:      "#21263200"
    readonly property color bgElevated:  "#252b38"
    readonly property color bgInset:     "#10141c"
    readonly property color bgFloating:  "#1a202bd9"
    readonly property color bgHover:     "#2e3444"
    readonly property color bgPressed:   "#3a4258"

    // ── Accent / Brand
    readonly property color accent:          "#18c75e"
    readonly property color accentLight:     "#1ed36b"
    readonly property color accentDark:      "#14a34e"
    readonly property color accentSubtle:    "#0e6636"

    // ── Text
    readonly property color textPrimary:     "#e8edf6"
    readonly property color textSecondary:   "#a0abbe"
    readonly property color textTertiary:    "#7f90a6"
    readonly property color textDisabled:    "#566070"
    readonly property color textOnAccent:    "#ffffff"

    // ── Border
    readonly property color borderDefault:   "#363d4e"
    readonly property color borderSubtle:    "#2a3040"
    readonly property color borderStrong:    "#454d5e"
    readonly property color borderFocus:     "#18c75e"

    // ── Chrome / Title bar
    readonly property color chromeSurface:       "#10161e"
    readonly property color chromeSurfaceAlt:    "#0f151d"
    readonly property color chromeHover:         "#1b2230"
    readonly property color chromePressed:       "#242c3a"
    readonly property color chromeBorder:        "#253043"
    readonly property color chromeText:          "#cdd7e6"
    readonly property color chromeTextMuted:     "#9fb0c7"
    readonly property color chromeDangerHover:   "#d33241"
    readonly property color chromeDangerPressed: "#aa1f2d"

    // ── Status
    readonly property color statusSuccess:   "#18c75e"
    readonly property color statusWarning:   "#f5a623"
    readonly property color statusError:     "#e04040"
    readonly property color statusInfo:      "#3b9eff"

    // ── Typography
    readonly property int fontSizeXS:   10
    readonly property int fontSizeSM:   11
    readonly property int fontSizeMD:   12
    readonly property int fontSizeLG:   14
    readonly property int fontSizeXL:   16
    readonly property int fontSizeXXL:  20

    // ── Spacing
    readonly property int spacingXS:  4
    readonly property int spacingSM:  6
    readonly property int spacingMD:  8
    readonly property int spacingLG:  12
    readonly property int spacingXL:  16
    readonly property int spacingXXL: 24

    // ── Radii
    readonly property int radiusSM:   3
    readonly property int radiusMD:   5
    readonly property int radiusLG:   8
    readonly property int radiusXL:   12

    // ── Control sizing
    readonly property int controlHeightSM:  28
    readonly property int controlHeightMD:  34
    readonly property int controlHeightLG:  40
    readonly property int iconButtonSizeSM: 32
    readonly property int iconButtonSizeMD: 34
    readonly property int iconButtonSizeLG: 38
    readonly property int pillHeight:       34
    readonly property int panelPadding:     12

    // ── Sidebar
    readonly property int sidebarWidth:    240
    readonly property int rightPanelWidth: 300

    // ── Title bar
    readonly property int titleBarHeight:  40
    readonly property int tabBarHeight:    36
    readonly property int statusBarHeight: 24
}
