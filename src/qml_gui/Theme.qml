pragma Singleton
import QtQuick

// =============================================================================
// OWzx Theme — single source of truth for all design tokens.
//
// Phase 160 (DS-01, v5.2 UI Excellence): canonical token list. Every visible
// value in QML should reference one of these tokens; new code MUST NOT
// introduce hardcoded hex literals, font.pixelSize integers, or arbitrary
// spacing values. Later phases (161-169) migrate existing hardcodes here.
//
// Token groups:
//   Background palette  — bg* (page/panel/card/elevated/inset/floating/hover/pressed/tooltip)
//   Accent / Brand      — accent* (the OWzx green)
//   Text                — text* (primary/secondary/tertiary/disabled/muted/onAccent)
//   Border              — border* (default/subtle/strong/focus/input/active)
//   Chrome / Title bar  — chrome* (surface/hover/pressed/border/text/danger)
//   Status              — status* + severityColors list (notification palette)
//   Typography          — fontSize* (6-step scale + 13) + fontMono
//   Spacing             — spacing* (6-step scale)
//   Radii               — radius* (5-step scale)
//   Control             — switch*/progress*/overlay/menu/selection + sizing
//   Layout              — sidebar*/rightPanel*/titleBar/tabBar/statusBar
// =============================================================================

QtObject {
    // ── Background palette
    readonly property color bgBase:      "#0d0f12"
    readonly property color bgSurface:   "#131720"
    readonly property color bgPanel:     "#161a23"
    readonly property color bgCard:      "#21263200"
    readonly property color bgElevated:  "#2a3140"
    readonly property color bgInset:     "#10141c"
    readonly property color bgFloating:  "#1a202bd9"
    readonly property color bgHover:     "#2e3444"
    readonly property color bgPressed:   "#3a4258"
    readonly property color bgTooltip:   "#1a2332"

    // ── Accent / Brand
    readonly property color accent:           "#18c75e"
    readonly property color accentLight:      "#1ed36b"
    readonly property color accentDark:       "#14a34e"
    readonly property color accentSubtle:     "#0e6636"
    // Phase 160 (DS-01): pressed-state accent for use in CxIconButton/CxButton
    // (replaces Qt.darker(accentSubtle, 1.2) at CxIconButton.qml:48).
    readonly property color accentSubtlePressed: "#0a4d28"

    // ── Text
    readonly property color textPrimary:     "#e8edf6"
    readonly property color textSecondary:   "#a0abbe"
    readonly property color textTertiary:    "#7f90a6"
    readonly property color textDisabled:    "#566070"
    readonly property color textMuted:       "#8b949e"
    readonly property color textOnAccent:    "#ffffff"

    // ── Border
    readonly property color borderDefault:   "#363d4e"
    readonly property color borderSubtle:    "#333b4e"
    readonly property color borderStrong:    "#454d5e"
    readonly property color borderFocus:     "#18c75e"
    readonly property color borderInput:     "#2e3848"
    // Phase 160 (DS-01): borderActive was referenced in QML but undefined
    // (silent undefined runtime). Sourced from the active-border usage in
    // PreparePage focus indicators — slightly brighter than borderStrong.
    readonly property color borderActive:    "#5a6478"

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
    readonly property color bgErrorSubtle:   "#4a1c1c"
    readonly property color bgWarningSubtle: "#3a3420"
    // Phase 160 (DS-01): error pressed/dark for CxButton danger variant
    // (replaces Qt.darker(statusError, 1.2) at CxButton.qml:31-32).
    readonly property color statusErrorDark:    "#b03333"
    readonly property color statusErrorPressed: "#8a2828"

    // ── Scrollbar (Phase 160 DS-01: was hardcoded across CxScrollView)
    readonly property color scrollBarColor:       "#3a4258"
    readonly property color scrollBarHoverColor:  "#4a5470"
    readonly property color scrollBarTrackColor:  "#1c2230"

    // ── Typography
    readonly property int fontSizeXS:   10
    readonly property int fontSizeSM:   11
    readonly property int fontSizeMD:   12
    // Phase 160 (DS-01): fontSize13 used 17x in pages but missing from scale.
    readonly property int fontSize13:   13
    readonly property int fontSizeLG:   14
    readonly property int fontSizeXL:   16
    readonly property int fontSizeXXL:  20
    // Phase 160 (DS-01): monospace font token — replaces 26
    // `font.family: "Consolas"` hardcodes across 8 component files.
    readonly property string fontMono:      "Consolas"
    readonly property string fontMonoAlt:   "Cascadia Mono"   // fallback if Consolas missing

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
    readonly property int radiusXXL:  16

    // ── Control tokens (aliases where colors match existing tokens)
    readonly property color switchTrackOff:   "#2a3040"
    readonly property color switchTrackOn:    accent
    readonly property color switchKnob:       textPrimary
    readonly property color progressTrack:    borderSubtle
    readonly property color progressFill:     accent
    readonly property color overlayDim:       "#80000000"    // black at 50%
    readonly property color menuBackground:   "#1a202b"
    readonly property color selectionColor:   accent
    readonly property color selectionText:    bgBase

    // ── Control sizing
    readonly property int controlHeightSM:  28
    readonly property int controlHeightMD:  34
    readonly property int controlHeightLG:  40
    // Phase 160 (DS-01): extend scale for taller CTAs (e.g. wizard buttons).
    readonly property int controlHeightXL:  46
    readonly property int iconButtonSizeSM: 32
    readonly property int iconButtonSizeMD: 34
    readonly property int iconButtonSizeLG: 38
    readonly property int pillHeight:       34
    readonly property int panelPadding:     12
    // Phase 160 (DS-01): smaller padding for scroll gutters / dense rows.
    readonly property int panelPaddingSM:   8

    // ── Component sizing tokens (Phase 160 DS-01: replace hand-rolled values
    //     in CxSlider/CxSwitch/CxDialog). Phase 161 will migrate consumers.
    readonly property int sliderTrackHeight:    4
    readonly property int sliderHandleSize:     14
    readonly property int switchTrackWidth:     42
    readonly property int switchTrackHeight:    22
    readonly property int dialogHeaderHeight:   44
    readonly property int dialogFooterHeight:   52

    // ── Sidebar
    // Phase 160 (DS-01): old sidebarWidth=240 was never read (dead). Real
    // width comes from min/max/default below — Phase 164 unbreaks the
    // 7-layer 392px lock and routes everything through these tokens.
    readonly property int sidebarWidth:         320   // legacy alias (= default)
    readonly property int sidebarWidthMin:      240
    readonly property int sidebarWidthMax:      520
    readonly property int sidebarWidthDefault:  320
    readonly property int rightPanelWidth:      300
    readonly property int rightPanelWidthMin:   240
    readonly property int rightPanelWidthMax:   480

    // ── Title bar
    readonly property int titleBarHeight:  40
    readonly property int tabBarHeight:    36
    readonly property int statusBarHeight: 24

    // ── Notification severity palette (Phase 160 DS-01, consumed by Phase 167).
    // One source of truth — collapses the 3 private 10-level tables in
    // ErrorBanner/ErrorToast/NotificationCenter (~50 duplicated hex literals).
    // Indices follow BackendContext severity convention:
    //   0=Info 1=Success 2=Warning 3=Error 4=SeriousWarning
    //   5=Hint 6=PrintInfo 7=PrintInfoShort 8=Progress 9=Other
    readonly property var severityColors: [
        "#18c75e",  // 0 Info (green)
        "#18c75e",  // 1 Success (green)
        "#c87840",  // 2 Warning (amber)
        "#f05545",  // 3 Error (red)
        "#e04848",  // 4 SeriousWarning (dark red)
        "#58a6ff",  // 5 Hint (blue)
        "#7c6aef",  // 6 PrintInfo (purple)
        "#7c6aef",  // 7 PrintInfoShort (purple)
        "#3b9cf0",  // 8 Progress (light blue)
        "#18c75e"   // 9 Other (green)
    ]
    readonly property var severityIcons: [
        "i",  // 0 Info
        "✓",  // 1 Success
        "⚠",  // 2 Warning
        "✕",  // 3 Error
        "⚠",  // 4 SeriousWarning
        "?",  // 5 Hint
        "ℹ",  // 6 PrintInfo
        "ℹ",  // 7 PrintInfoShort
        "⟳",  // 8 Progress
        "•"   // 9 Other
    ]
}
