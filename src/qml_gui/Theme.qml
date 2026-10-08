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
//   Layout              — sidebar*/rightPanel*/titleBar/tabBar
// =============================================================================

QtObject {
    // ── Background palette
    // ctl-1 (direction=ref): neutral-gray rebase. The reference screenshot is
    // a flat neutral gray scale -- sidebar/inputs #4B4B4D, viewport #363638,
    // left card #2F3034, topbar #010101 -- with no blue cast, so every bg*
    // token maps to an un-tinted gray at a comparable lift over its old role.
    readonly property color bgBase:      "#2f3034"
    readonly property color bgSurface:   "#3a3a3c"
    readonly property color bgPanel:     "#4b4b4d"
    // U01 fix: was "#3a3a3c00" -- 9-digit hex parses as #AARRGGBB, i.e.
    // alpha 0x3a (~22.7%) over an olive RGB (58,60,0). That tint is what
    // pixel-blended to the measured #47483b separator/border values in
    // config-wizard (separator y=496), notification-center (#47483b stroke /
    // #2a2b1f separator), cali-history and about-dialog.
    readonly property color bgCard:      "#3a3a3c"

    // ── Overlay / popup surfaces (U01: §2 & upstream-anchored literals).
    // These values are also the R1 global surface-convergence targets: the
    // deferred bgElevated/bgSurface stack must fold onto them without drift.
    readonly property color surfaceDeep:   "#2d2d31"  // §2 body base = upstream notification window dark bg (NotificationManager.cpp:239)
    readonly property color bannerSurface: "#36363b"  // §2 banner band token
    readonly property color overlayBorder: "#3e3e45"  // upstream notification window dark border (NotificationManager.cpp:247)
    readonly property color separator:     "#000000"  // §2 dialog 1px separator line
    readonly property color bgElevated:  "#4b4b4d"
    readonly property color bgInset:     "#262628"
    readonly property color bgFloating:  "#4f4f51d9"
    readonly property color bgHover:     "#555557"
    readonly property color bgPressed:   "#5f5f61"
    readonly property color bgTooltip:   "#343436"

    // ── Accent / Brand
    readonly property color accent:           "#18c75e"
    readonly property color accentLight:      "#1ed36b"
    readonly property color accentDark:       "#14a34e"
    readonly property color accentSubtle:     "#0e6636"
    // Phase 160 (DS-01): pressed-state accent for use in CxIconButton/CxButton
    // (replaces Qt.darker(accentSubtle, 1.2) at CxIconButton.qml:48).
    readonly property color accentSubtlePressed: "#0a4d28"

    // ── Text
    // ctl-1: near-white neutral grays (no blue cast).
    readonly property color textPrimary:     "#f5f5f5"
    readonly property color textSecondary:   "#c9c9c9"
    readonly property color textTertiary:    "#a3a3a3"
    readonly property color textDisabled:    "#6f6f6f"
    readonly property color textMuted:       "#ababab"
    readonly property color textOnAccent:    "#ffffff"

    // ── Border
    // ctl-1: neutral #5a5a5c family; borderFocus keeps the brand green.
    readonly property color borderDefault:   "#5a5a5c"
    readonly property color borderSubtle:    "#525254"
    readonly property color borderStrong:    "#666668"
    readonly property color borderFocus:     "#18c75e"
    readonly property color borderInput:     "#565658"
    // Phase 160 (DS-01): borderActive was referenced in QML but undefined
    // (silent undefined runtime). Sourced from the active-border usage in
    // PreparePage focus indicators — slightly brighter than borderStrong.
    readonly property color borderActive:    "#6e6e70"

    // ── Chrome / Title bar
    // topbar-1/ctl-2 (direction=ref): the reference topbar measures #010101
    // and its second toolbar band #27292C with white text; upstream's dark
    // map (BBLTopbar.cpp:106 rgb(38,46,48)) reads the same neutral way.
    readonly property color chromeSurface:       "#010101"
    readonly property color chromeSurfaceAlt:    "#0f0f10"
    readonly property color chromeHover:         "#1f2124"
    readonly property color chromePressed:       "#27292c"
    readonly property color chromeBorder:        "#262628"
    readonly property color chromeText:          "#fefefe"
    readonly property color chromeTextMuted:     "#b4b4b4"
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
    // R13 (restoration-map.md:43): slot #171717 / thumb #959595.
    readonly property color scrollBarColor:       "#959595"
    readonly property color scrollBarHoverColor:  "#7e7e80"
    readonly property color scrollBarTrackColor:  "#171717"

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
    // ctl-4: default UI font family. Upstream privately installs
    // "HarmonyOS Sans SC" (Label.cpp:22; AddPrivateFont at Label.cpp:99-100)
    // and builds every Head_/Body_ font on it. main_qml.cpp loads the bundled
    // TTFs via QFontDatabase::addApplicationFont and installs the same family
    // as the QGuiApplication font -- keep the two names in sync.
    readonly property string fontFamily:    "HarmonyOS Sans SC"

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
    readonly property color switchTrackOff:   "#3f3f41"
    readonly property color switchTrackOn:    accent
    readonly property color switchKnob:       textPrimary
    readonly property color progressTrack:    borderSubtle
    readonly property color progressFill:     accent
    readonly property color overlayDim:       "#80000000"    // black at 50%
    // U01 (prepare-context-menus gap12): popup-layer surface anchored to the
    // §2 body base = upstream notification window dark bg
    // (NotificationManager.cpp:239). Popup-layer only -- not part of the
    // deferred bgElevated/bgSurface global stack.
    readonly property color menuBackground:   "#2d2d31"
    readonly property color selectionColor:   accent
    readonly property color selectionText:    bgBase

    // R14 (restoration-map.md:44) disabled control capsule, measured from the
    // prepare-page export capsule at (1200,58). U01 applies it to CxButton's
    // Primary disabled state, replacing the old accentSubtle + 0.45 opacity
    // overlay.
    readonly property color controlDisabledBg:   "#8e8e83"
    readonly property color controlDisabledText: "#56564f"

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
    // width comes from min/max/default below. R8 (2026-09-24): default
    // restored to 392 -- the upstream-measured sidebar width incl. its 18px
    // scrollbar gutter (docs/ui-reference/restoration-map.md).
    readonly property int sidebarWidth:         392   // legacy alias (= default)
    readonly property int sidebarWidthMin:      300
    readonly property int sidebarWidthMax:      520
    readonly property int sidebarWidthDefault:  392
    readonly property int rightPanelWidth:      300
    readonly property int rightPanelWidthMin:   240
    readonly property int rightPanelWidthMax:   480

    // ── Title bar
    // layout-1: no statusBarHeight token -- upstream MainFrame has no status
    // bar (MainFrame.cpp:524,1025) and the OWzx StatusBar was removed.
    readonly property int titleBarHeight:  40
    readonly property int tabBarHeight:    36

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
