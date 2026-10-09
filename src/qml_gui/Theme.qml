pragma Singleton
import QtQuick

// =============================================================================
// OWzx Theme — single source of truth for all design tokens.
//
// Phase 160 (DS-01, v5.2 UI Excellence): canonical token list. Every visible
// value in QML should reference one of these tokens; new code MUST NOT
// introduce hardcoded hex literals, font.pixelSize integers, or arbitrary
// spacing values. Later phases (161-169) migrate existing hardcodes here.
// Phase 170 (DS-03): reality-aligned radius/spacing scales, motion tier,
// display type rungs, data/mode palette; controls/ fully tokenized.
// Phase 171 (P1): dual-theme. Every chrome color token is a conditional
// binding on `theme` ("dark" default | "light"); token NAMES are stable so
// the ~3900 existing Theme.* references need no change. Brand green fills
// are theme-independent; text/border/status tokens pick contrast-safe light
// variants. Upstream-measured literals (e.g. topbar-13 AddSpacer(10)) live
// at their call sites and are exempt.
//
// Token groups:
//   Theme mode          — theme/isDark/setTheme (Phase 171)
//   Background palette  — bg* (page/panel/card/elevated/inset/floating/hover/pressed/tooltip)
//   Accent / Brand      — accent* (the OWzx green)
//   Text                — text* (primary/secondary/tertiary/disabled/muted/onAccent)
//   Border              — border* (default/subtle/strong/focus/input/active)
//   Chrome / Title bar  — chrome* (surface/hover/pressed/border/text/danger)
//   Status              — status* + severityColors list (notification palette)
//   Typography          — fontSize* (8-step scale + 13 + display tier) + fontMono
//   Motion              — motionFast/Normal/Slow + easingStandard (Phase 170)
//   Spacing             — spacing* (7-step scale incl. XXS)
//   Radii               — radius* (7-step scale incl. XS/Hero)
//   Control             — switch*/progress*/overlay/menu/selection + sizing
//   Layout              — sidebar*/rightPanel*/titleBar/tabBar
//   Data & mode         — asm*/cali*/tipGlass*/extruderPalette/visibilityTypeColors
// =============================================================================

QtObject {
    // ── Theme mode (Phase 171, P1)
    // Persisted via the `appSettings` context property (QSettings "ui/theme");
    // main.qml bridges the stored value in on startup, the topbar toggle
    // writes it back. Dark remains the default and the design-referenceTruth.
    property string theme: "dark"
    readonly property bool isDark: theme !== "light"
    function setTheme(name) {
        if (name === "dark" || name === "light")
            theme = name
    }

    // ── Density (Phase 173, P3)
    // "comfortable" preserves the measured/upstream control metrics; "compact"
    // steps the control-height family down 4-6px for dense workflows. Spacing
    // and layout-truth tokens are density-independent by design.
    property string density: "comfortable"
    readonly property bool compactMode: density === "compact"
    function setDensity(name) {
        if (name === "comfortable" || name === "compact")
            density = name
    }

    // ── Background palette
    // ctl-1 (direction=ref): neutral-gray rebase. The reference screenshot is
    // a flat neutral gray scale -- sidebar/inputs #4B4B4D, viewport #363638,
    // left card #2F3034, topbar #010101 -- with no blue cast, so every bg*
    // token maps to an un-tinted gray at a comparable lift over its old role.
    readonly property color bgBase:      isDark ? "#2f3034" : "#eef0f3"
    readonly property color bgSurface:   isDark ? "#3a3a3c" : "#ffffff"
    readonly property color bgPanel:     isDark ? "#4b4b4d" : "#f2f4f7"
    // U01 fix: was "#3a3a3c00" -- 9-digit hex parses as #AARRGGBB, i.e.
    // alpha 0x3a (~22.7%) over an olive RGB (58,60,0). That tint is what
    // pixel-blended to the measured #47483b separator/border values in
    // config-wizard (separator y=496), notification-center (#47483b stroke /
    // #2a2b1f separator), cali-history and about-dialog.
    readonly property color bgCard:      isDark ? "#3a3a3c" : "#ffffff"

    // ── Overlay / popup surfaces (U01: §2 & upstream-anchored literals).
    // These values are also the R1 global surface-convergence targets: the
    // deferred bgElevated/bgSurface stack must fold onto them without drift.
    readonly property color surfaceDeep:   isDark ? "#2d2d31" : "#ffffff"  // §2 body base = upstream notification window bg (NotificationManager.cpp:239)
    readonly property color bannerSurface: isDark ? "#36363b" : "#eef1f4"  // §2 banner band token
    readonly property color overlayBorder: isDark ? "#3e3e45" : "#d8dbe0"  // upstream notification window border (NotificationManager.cpp:247)
    readonly property color separator:     isDark ? "#000000" : "#d5d8dd"  // §2 dialog 1px separator line
    readonly property color bgElevated:  isDark ? "#4b4b4d" : "#ffffff"
    readonly property color bgInset:     isDark ? "#262628" : "#e3e6ea"
    readonly property color bgFloating:  isDark ? "#4f4f51d9" : "#f5f7fad9"
    readonly property color bgHover:     isDark ? "#555557" : "#e7eaee"
    readonly property color bgPressed:   isDark ? "#5f5f61" : "#dde1e6"
    readonly property color bgTooltip:   isDark ? "#343436" : "#fbfcfd"

    // ── Accent / Brand
    // Brand fills are theme-independent; only the subtle *tint* surface tier
    // flips (dark needs a deep green glass, light a pale mint wash).
    readonly property color accent:           "#18c75e"
    readonly property color accentLight:      "#1ed36b"
    readonly property color accentDark:       "#14a34e"
    readonly property color accentSubtle:     isDark ? "#0e6636" : "#dcf5e7"
    // Phase 160 (DS-01): pressed-state accent for use in CxIconButton/CxButton
    // (replaces Qt.darker(accentSubtle, 1.2) at CxIconButton.qml:48).
    readonly property color accentSubtlePressed: isDark ? "#0a4d28" : "#c3ecd5"

    // ── Text
    // ctl-1: near-white neutral grays (no blue cast).
    readonly property color textPrimary:     isDark ? "#f5f5f5" : "#1b1d21"
    readonly property color textSecondary:   isDark ? "#c9c9c9" : "#4d5157"
    readonly property color textTertiary:    isDark ? "#a3a3a3" : "#797f87"
    readonly property color textDisabled:    isDark ? "#6f6f6f" : "#a9adb4"
    readonly property color textMuted:       isDark ? "#ababab" : "#8d939b"
    readonly property color textOnAccent:    "#ffffff"

    // ── Border
    // ctl-1: neutral #5a5a5c family; borderFocus keeps the brand green.
    readonly property color borderDefault:   isDark ? "#5a5a5c" : "#c5c9d0"
    readonly property color borderSubtle:    isDark ? "#525254" : "#d3d6db"
    readonly property color borderStrong:    isDark ? "#666668" : "#b3b8c0"
    readonly property color borderFocus:     "#18c75e"
    readonly property color borderInput:     isDark ? "#565658" : "#c8ccd2"
    // Phase 160 (DS-01): borderActive was referenced in QML but undefined
    // (silent undefined runtime). Sourced from the active-border usage in
    // PreparePage focus indicators — slightly brighter than borderStrong.
    readonly property color borderActive:    isDark ? "#6e6e70" : "#a3a9b1"

    // ── Chrome / Title bar
    // topbar-1/ctl-2 (direction=ref): the reference topbar measures #010101
    // and its second toolbar band #27292C with white text; upstream's dark
    // map (BBLTopbar.cpp:106 rgb(38,46,48)) reads the same neutral way.
    // Light mode lifts the chrome bands (upstream light chrome is light too).
    readonly property color chromeSurface:       isDark ? "#010101" : "#e9ebee"
    readonly property color chromeSurfaceAlt:    isDark ? "#0f0f10" : "#f3f4f6"
    readonly property color chromeHover:         isDark ? "#1f2124" : "#dcdfe3"
    readonly property color chromePressed:       isDark ? "#27292c" : "#d3d6db"
    readonly property color chromeBorder:        isDark ? "#262628" : "#d0d3d8"
    readonly property color chromeText:          isDark ? "#fefefe" : "#1a1c1f"
    readonly property color chromeTextMuted:     isDark ? "#b4b4b4" : "#6d7278"
    readonly property color chromeDangerHover:   "#d33241"
    readonly property color chromeDangerPressed: "#aa1f2d"

    // ── Status
    // Text-facing status tones darken in light mode for contrast on white;
    // fill-only tones (error dark/pressed) stay fixed.
    readonly property color statusSuccess:   isDark ? "#18c75e" : "#0f9e48"
    readonly property color statusWarning:   isDark ? "#f5a623" : "#b9790a"
    readonly property color statusError:     isDark ? "#e04040" : "#d13a3a"
    readonly property color statusInfo:      isDark ? "#3b9eff" : "#1e78d6"
    readonly property color bgErrorSubtle:   isDark ? "#4a1c1c" : "#fbe9e9"
    readonly property color bgWarningSubtle: isDark ? "#3a3420" : "#faf0da"
    // Phase 160 (DS-01): error pressed/dark for CxButton danger variant
    // (replaces Qt.darker(statusError, 1.2) at CxButton.qml:31-32).
    readonly property color statusErrorDark:    "#b03333"
    readonly property color statusErrorPressed: "#8a2828"

    // ── Scrollbar (Phase 160 DS-01: was hardcoded across CxScrollView)
    // R13 (restoration-map.md:43): slot #171717 / thumb #959595.
    readonly property color scrollBarColor:       isDark ? "#959595" : "#a6abb3"
    readonly property color scrollBarHoverColor:  isDark ? "#7e7e80" : "#8b9099"
    readonly property color scrollBarTrackColor:  isDark ? "#171717" : "#ebecee"

    // ── Typography
    readonly property int fontSizeXS:   10
    readonly property int fontSizeSM:   11
    readonly property int fontSizeMD:   12
    // Phase 160 (DS-01): fontSize13 used 17x in pages but missing from scale.
    readonly property int fontSize13:   13
    readonly property int fontSizeLG:   14
    readonly property int fontSizeXL:   16
    readonly property int fontSizeXXL:  20
    // Phase 170 (DS-03): display tier for page/hero headers and empty-state
    // art, previously satisfied by scattered 22/24/28/29/30/34/48/56/64
    // literals.
    readonly property int fontSizeDisplay:    24
    readonly property int fontSizeDisplayXL:  32
    // Phase 160 (DS-01): monospace font token — replaces 26
    // `font.family: "Consolas"` hardcodes across 8 component files.
    readonly property string fontMono:      "Consolas, monospace"
    readonly property string fontMonoAlt:   "Cascadia Mono"   // fallback if Consolas missing
    // ctl-4: default UI font family. Upstream privately installs
    // "HarmonyOS Sans SC" (Label.cpp:22; AddPrivateFont at Label.cpp:99-100)
    // and builds every Head_/Body_ font on it. main_qml.cpp loads the bundled
    // TTFs via QFontDatabase::addApplicationFont and installs the same family
    // as the QGuiApplication font -- keep the two names in sync.
    readonly property string fontFamily:    "HarmonyOS Sans SC"

    // ── Motion (Phase 170, DS-03)
    // Three transition rungs + one standard easing. Fact-of-code baseline:
    // 120ms was the dominant micro-transition, 150-220ms the mid tier, with
    // 30/30 sites already on Easing.OutCubic. Loop animations whose duration
    // is the cycle time (busy spinner 900ms, progress pulses 1000/1400ms) are
    // exempt -- they are periods, not transition speeds.
    readonly property int motionFast:     120
    readonly property int motionNormal:   200
    readonly property int motionSlow:     300
    readonly property int easingStandard: Easing.OutCubic

    // ── Spacing
    // Phase 170 (DS-03): added the spacingXXS=2 hairline rung — 2px gaps are
    // the dominant micro-spacing in dense lists/chips and were previously
    // unsatisfiable from the scale (forcing 1/2/3 literals).
    readonly property int spacingXXS: 2
    readonly property int spacingXS:  4
    readonly property int spacingSM:  6
    readonly property int spacingMD:  8
    readonly property int spacingLG:  12
    readonly property int spacingXL:  16
    readonly property int spacingXXL: 24

    // ── Radii
    // Phase 170 (DS-03): scale redefined to the codebase-reality system
    // 2/4/6/8/12/16/24 (previously 3/5/8/12/16, which ~250 hardcoded sites
    // ignored). Upstream corner tiers corroborate: Button.cpp:191-221 uses
    // 4/8/12 (Choice/Compact/Window). radiusSM 3→4 and radiusMD 5→6 shift
    // existing token consumers by 1px -- adjudicated as the reality-aligned
    // unification (2026-10-09 design-quality round).
    readonly property int radiusXS:   2
    readonly property int radiusSM:   4
    readonly property int radiusMD:   6
    readonly property int radiusLG:   8
    readonly property int radiusXL:   12
    readonly property int radiusXXL:  16
    readonly property int radiusHero: 24

    // ── Control tokens (aliases where colors match existing tokens)
    readonly property color switchTrackOff:   isDark ? "#3f3f41" : "#c2c6cd"
    // Phase 171 (P1): explicit knob white in both themes (was aliased to
    // textPrimary, which would render a dark knob in light mode).
    readonly property color switchKnob:       isDark ? "#f5f5f5" : "#ffffff"
    readonly property color progressTrack:    borderSubtle
    readonly property color progressFill:     accent
    readonly property color overlayDim:       "#80000000"    // black at 50%
    // U01 (prepare-context-menus gap12): popup-layer surface anchored to the
    // §2 body base = upstream notification window dark bg
    // (NotificationManager.cpp:239). Popup-layer only -- not part of the
    // deferred bgElevated/bgSurface global stack.
    readonly property color menuBackground:   isDark ? "#2d2d31" : "#ffffff"
    readonly property color selectionColor:   accent
    readonly property color selectionText:    isDark ? bgBase : "#ffffff"

    // R14 (restoration-map.md:44) disabled control capsule, measured from the
    // prepare-page export capsule at (1200,58). U01 applies it to CxButton's
    // Primary disabled state, replacing the old accentSubtle + 0.45 opacity
    // overlay. Light mode lifts the pair to a matching neutral.
    readonly property color controlDisabledBg:   isDark ? "#8e8e83" : "#c6c8c0"
    readonly property color controlDisabledText: isDark ? "#56564f" : "#6e7068"

    // ── Control sizing (height family is density-aware, Phase 173 P3)
    readonly property int controlHeightSM:  compactMode ? 24 : 28
    readonly property int controlHeightMD:  compactMode ? 28 : 34
    readonly property int controlHeightLG:  compactMode ? 34 : 40
    // Phase 160 (DS-01): extend scale for taller CTAs (e.g. wizard buttons).
    readonly property int controlHeightXL:  compactMode ? 40 : 46
    readonly property int iconButtonSizeSM: compactMode ? 28 : 32
    readonly property int iconButtonSizeMD: compactMode ? 30 : 34
    readonly property int iconButtonSizeLG: compactMode ? 34 : 38
    readonly property int pillHeight:       compactMode ? 30 : 34
    readonly property int panelPadding:     compactMode ? 10 : 12
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

    // ── Data & mode palette (Phase 170, DS-03)
    // Functional/data colors that live OUTSIDE the chrome palette: assembly
    // mode indicators, extruder swatches, visibility type colors, tooltip
    // glass. They are named here (not inline hex) so pages stay token-clean;
    // values are preserved 1:1 from the pages they migrated out of. They are
    // theme-independent: they color viewport/canvas content, not chrome.
    readonly property color asmSelectTeal:    "#009688"   // AssemblePage select-first mode
    readonly property color asmRevertOrange:  "#FF6F00"   // AssemblePage revert mode
    readonly property color asmPurple:        "#A437A4"   // AssemblePage auxiliary mode
    // AssemblePage measurement card (light surface independent of app theme)
    readonly property color asmCardBg:        "#FAFAFA"
    readonly property color asmCardBgAlt:     "#FAFAFC"
    readonly property color asmCardBorder:    "#E7E7E7"
    readonly property color asmCardTextDim:   "#C8C8C8"
    readonly property color asmCardText:      "#545859"
    readonly property color asmMeasureGreen:  "#57DF3D"
    readonly property color asmMeasureDark:   "#004E00"
    // Calibration light history card
    readonly property color caliCardBg:       "#262e30"
    readonly property color caliCardTextDim:  "#cecece"
    readonly property color caliCardText:     "#eeeeee"
    // Viewport tooltip glass (ToolPositionTooltip / GLToolbars family)
    readonly property color tipGlassBg:       "#11151dcc"
    readonly property color tipGlassBorder:   "#ffffff99"
    readonly property color tipGlassDim:      "#80808080"
    readonly property color tipGlassShadow:   "#40333333"
    // Extruder / filament swatch data palette (PreparePage squeeze colors)
    readonly property var extruderPalette: [
        "#4444FF", "#e066a0", "#00AE42", "#8B5CF6",
        "#EC4899", "#84CC16", "#D946EF", "#A855F7"
    ]
    // VisibilityFilter per-type marker colors
    readonly property var visibilityTypeColors: [
        "#FFFF00", "#CD22D6", "#49ADCE", "#E6E6E6", "#C1BE63"
    ]

    // ── Notification severity palette (Phase 160 DS-01, consumed by Phase 167).
    // One source of truth — collapses the 3 private 10-level tables in
    // ErrorBanner/ErrorToast/NotificationCenter (~50 duplicated hex literals).
    // Indices follow BackendContext severity convention:
    //   0=Info 1=Success 2=Warning 3=Error 4=SeriousWarning
    //   5=Hint 6=PrintInfo 7=PrintInfoShort 8=Progress 9=Other
    // Phase 171 (P1): light mode darkens the text-facing tones for contrast.
    readonly property var severityColors: isDark ? [
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
    ] : [
        "#0f9e48",  // 0 Info (green)
        "#0f9e48",  // 1 Success (green)
        "#a05f28",  // 2 Warning (amber)
        "#d63c2c",  // 3 Error (red)
        "#c23a3a",  // 4 SeriousWarning (dark red)
        "#2a7fd4",  // 5 Hint (blue)
        "#6355d6",  // 6 PrintInfo (purple)
        "#6355d6",  // 7 PrintInfoShort (purple)
        "#2379cc",  // 8 Progress (light blue)
        "#0f9e48"   // 9 Other (green)
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
