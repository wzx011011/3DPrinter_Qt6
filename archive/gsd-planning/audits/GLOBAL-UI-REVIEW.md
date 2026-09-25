# Global UI Review - All Implemented UI Design

**Audited:** 2026-06-25  
**Scope:** all implemented Qt6/QML UI design under `src/qml_gui/`, with supporting viewmodel/service surface considered where it affects visible UI behavior.  
**Baseline:** abstract 6-pillar UI standards plus OWzx source-truth migration constraints. No approved global `UI-SPEC.md` exists.  
**Screenshots:** not captured. This is a native Qt application, no localhost web surface was available on ports 3000, 5173, or 8080, and no Playwright-MCP browser target was available for the app.  
**Registry audit:** skipped. `components.json` is absent; no shadcn/third-party UI registry is initialized.

---

## Pillar Scores

| Pillar | Score | Key Finding |
|--------|-------|-------------|
| 1. Copywriting | 2/4 | Most visible text is wrapped in `qsTr`, but the UI mixes Chinese, English, internal phase wording, and user-facing placeholders. |
| 2. Visuals | 2/4 | Major slicer surfaces exist, but placeholder tabs, disabled toolbar items, and dense overlay panels weaken hierarchy and product confidence. |
| 3. Color | 2/4 | `Theme.qml` exists, but 903 hardcoded hex color occurrences remain across QML, causing palette drift between pages. |
| 4. Typography | 2/4 | A token scale exists, but 24 raw pixel sizes are used directly, including 7/8/9 px text in dense panels. |
| 5. Spacing | 2/4 | Spacing tokens exist, but raw margins/spacing dominate and fixed desktop dimensions constrain responsive behavior. |
| 6. Experience Design | 2/4 | Core interactions are wired in many places, but top-level no-ops, disabled menus, and visible reserved workflows remain. |

**Overall: 12/24**

---

## Top 3 Priority Fixes

1. **Remove visible placeholder and no-op paths from top-level navigation and menus** - users see disabled or inert product promises in the first screen - either wire implemented behavior or hide/classify blocked actions behind a deferred-feature policy.
2. **Centralize all color, typography, and spacing through `Theme.qml` tokens** - current pages look like multiple design systems stitched together - migrate hardcoded values in `BBLTopbar.qml`, `MonitorPage.qml`, `PrintSettings.qml`, and dialog files first.
3. **Normalize copy language and user-facing terminology** - the UI currently mixes Chinese labels, English settings names, internal version notes, and reserved placeholders - define a single active locale contract and remove phase/version text from runtime UI.

---

## Audit Measurements

- QML files audited: 80.
- `Theme.` references: 2,589.
- `qsTr(...)` references: 1,661.
- Hardcoded hex color occurrences: 903.
- Bare `text: "..."` string occurrences: 175.
- `enabled: false` occurrences: 32.
- Empty `onClicked` / `onTriggered` handlers: 20.
- Placeholder/TODO/version markers in QML: 160.
- Raw `font.pixelSize` values found: 24 distinct sizes (`7` through `64`), outside the six-token scale in `Theme.qml`.

---

## Detailed Findings

### Pillar 1: Copywriting (2/4)

**WARNING: Mixed runtime language weakens product consistency.** `LeftSidebar.qml` mixes Chinese section labels with English labels such as `Process`, `Global`, `Objects`, `Advanced`, `Object Settings`, `Layer Height`, `Infill Density`, `Print Speed`, `Variable Layer Height`, `Filam.`, and `Rename Preset` (`src/qml_gui/panels/LeftSidebar.qml:217`, `:233`, `:242`, `:258`, `:420`, `:447`, `:463`, `:479`, `:498`, `:550`, `:606`). That makes the UI feel unfinished even when behavior exists.

**WARNING: Internal planning state leaks into user-facing copy.** Top-level UI exposes labels such as `占位`, `v2.1 实现`, and `AssembleView - reserved (v2.0 placeholder)` (`src/qml_gui/BBLTopbar.qml:284`, `:285`, `:230`, `:240`, `:250`; `src/qml_gui/pages/Plater.qml:108`; `src/qml_gui/main.qml:531`). Runtime UI should not mention implementation phases.

**WARNING: Some technical English is untranslated even inside `qsTr`.** Calibration menu entries and imported file actions use English technical labels (`Temperature`, `Max flowrate`, `Pressure advance`, `Import 3MF`, `Export Model`) while surrounding menus are Chinese (`src/qml_gui/BBLTopbar.qml:672`-`:682`). If English technical terms are intentional, the rule should be documented; otherwise they should be translated consistently.

### Pillar 2: Visuals (2/4)

**WARNING: First-viewport navigation still advertises unfinished pages.** `BBLTopbar.qml` keeps two notebook placeholders in the tab model, both disabled, and the second placeholder has a hidden page body (`src/qml_gui/BBLTopbar.qml:284`-`:285`; `src/qml_gui/main.qml:523`-`:531`). This creates a permanent unfinished-product signal in the most prominent navigation component.

**WARNING: The top bar uses one-off visual styling instead of the project control system.** The title bar background, hover colors, tab hover color, text colors, and dividers are hardcoded directly in `BBLTopbar.qml` (`:86`, `:113`, `:135`, `:159`, `:301`, `:312`, `:386`, `:458`, `:467`). This makes the top-level chrome visually diverge from the rest of the themed QML controls.

**WARNING: Dense GL tool panels risk occluding the primary 3D task.** `PreparePage.qml` defines many floating panels around `anchors.topMargin: 104` for support paint, seam, cutting, drilling, emboss, boolean, face detection, text, SVG, and SLA support workflows. They use 9-10 px text and compact fixed controls (`src/qml_gui/pages/PreparePage.qml:1760`-`:2858`). Without screenshots, this cannot be visually confirmed, but the code strongly suggests heavy overlay density on the main 3D canvas.

### Pillar 3: Color (2/4)

**WARNING: Hardcoded colors are still widespread despite `Theme.qml`.** The audit counted 903 hardcoded hex color occurrences across QML. The most repeated values duplicate or compete with tokens: `#18c75e` (44), `#566070` (41), `#a0abbe` (33), `#1c2a3e` (29), `#ef4444` (28), and `#22c55e` (27). These should be tokenized or documented as data colors.

**WARNING: Page-specific palettes drift from the global theme.** `MonitorPage.qml` uses Tailwind-like blue/green/red/gray values for device controls and camera state (`#2563eb`, `#3b82f6`, `#22c55e`, `#ef4444`, `#374151`) while `Theme.qml` defines `statusInfo`, `statusSuccess`, `statusError`, and dark-surface tokens (`src/qml_gui/pages/MonitorPage.qml:504`, `:606`, `:1011`, `:1085`, `:1210`, `:1230`, `:1310`). This creates a different visual language for the monitor page.

**WARNING: Hover and danger states are manually redefined in repeated surfaces.** `PrintSettings.qml` repeats custom hover blues and danger backgrounds (`#1c2a3e`, `#2e1a1a`) in many action buttons (`src/qml_gui/panels/PrintSettings.qml:572`, `:581`, `:646`, `:877`, `:886`, `:1011`, `:1428`, `:1531`). These belong in reusable button variants.

### Pillar 4: Typography (2/4)

**WARNING: Raw type sizes exceed the declared type scale.** `Theme.qml` defines six sizes from 10 to 20 (`src/qml_gui/Theme.qml:58`-`:63`), but QML uses 24 distinct raw `font.pixelSize` values: 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 40, 48, 56, and 64. Raw sizes make cross-page hierarchy unpredictable.

**WARNING: Important controls use very small text.** Dense controls use 7-9 px text in `PreparePage.qml`, `ObjectList.qml`, and `LeftSidebar.qml` (`src/qml_gui/pages/PreparePage.qml:3083`; `src/qml_gui/panels/ObjectList.qml:44`, `:54`, `:64`, `:74`, `:108`, `:123`, `:138`, `:153`, `:168`, `:183`; `src/qml_gui/panels/LeftSidebar.qml:567`). These are likely below comfortable readability for repeated operational use.

**WARNING: Large display sizes are used as decorative empty-state icons rather than a coherent hierarchy.** Examples include 48/64 px text glyphs in `ProjectPage.qml` and `MonitorPage.qml` (`src/qml_gui/pages/ProjectPage.qml:202`; `src/qml_gui/pages/MonitorPage.qml:479`, `:581`, `:798`, `:1786`). If icon fonts are retained, they should be replaced with asset/icons and size tokens.

### Pillar 5: Spacing (2/4)

**WARNING: Raw spacing dominates tokenized spacing.** The project has spacing tokens from 4 to 24 (`src/qml_gui/Theme.qml:67`-`:72`), but raw spacing and margins are common. Counted raw spacing values include `8` (167), `6` (115), `4` (98), `16` (68), `0` (68), `10` (58), `12` (55), and `2` (47). The values mostly match the intended scale, but bypassing `Theme.*` makes future density tuning expensive.

**WARNING: Desktop-first fixed window constraints limit layout resilience.** `main.qml` fixes the initial window at 1828x1000 and minimum at 1100x700 (`src/qml_gui/main.qml:20`-`:27`). Several panels rely on fixed widths/heights, such as object list height 200 and plate cards 120x62 (`src/qml_gui/panels/LeftSidebar.qml:407`; `src/qml_gui/pages/PreparePage.qml:2890`, `:2909`). This is acceptable for a desktop slicer baseline but should be tested at minimum size and high DPI.

**WARNING: Some UI rows are likely too compressed for localization.** The 7-subtab parameter row uses 22 px-high tabs and 8 px text (`src/qml_gui/panels/LeftSidebar.qml:555`, `:567`), while object-list filter buttons use 9 px labels (`src/qml_gui/panels/ObjectList.qml:44`-`:74`). Longer translated labels are likely to truncate or become unreadable.

### Pillar 6: Experience Design (2/4)

**BLOCKER: Top-level actions are visible but inert.** `main.qml` leaves export project/model and preferences requests as TODO/no-op handlers (`src/qml_gui/main.qml:371`-`:376`). `BBLTopbar.qml` exposes Help > Documentation as an empty handler and macOS About as an empty handler (`src/qml_gui/BBLTopbar.qml:690`, `:78`). These are first-level user actions and should either work or be hidden/disabled with a clear reason.

**WARNING: Many disabled controls do not explain actionable next steps.** Topbar account/model-store/publish, placeholder tabs, filament group popup, inverse selection, view-layer controls, calibration entries, network test, and layer editing are disabled or empty (`src/qml_gui/BBLTopbar.qml:224`-`:251`, `:366`-`:374`, `:625`-`:653`, `:672`-`:682`; `src/qml_gui/dialogs/NetworkTestDialog.qml:77`; `src/qml_gui/components/GLToolbars.qml:96`-`:100`). Disabled features should be separated into `Placeholder`, `Blocked`, or `Future` categories and surfaced consistently.

**WARNING: Search and settings controls suggest behavior that is not complete.** Left sidebar settings search accepts input but does not perform a visible search or open the search dialog (`src/qml_gui/panels/LeftSidebar.qml:313`-`:323`). Parameter subtabs change local state but still show a reserved parameter list (`src/qml_gui/panels/LeftSidebar.qml:541`-`:585`).

**PASS: Core operational flows are meaningfully wired in several areas.** Prepare context menus dispatch to `EditorViewModel`, undo/redo shortcuts exist, slice/export menus call real viewmodel paths, object list interactions bind to selection and plate state, and monitor/camera controls have concrete state bindings. The score remains 2 because high-visibility placeholders and no-ops still interrupt the main experience.

---

## Recommended Remediation Order

1. **Top-level action triage:** update `BBLTopbar.qml` and `main.qml` so every visible menu/tab/toolbar action is one of: wired, hidden, disabled with reason, or moved to a future/blocked registry.
2. **Theme migration pass:** migrate hardcoded `BBLTopbar.qml`, `MonitorPage.qml`, `PrintSettings.qml`, and common dialogs to `Theme.qml` colors, font tokens, and spacing/radius tokens.
3. **Copy contract:** choose active runtime language policy for zh_CN vs English technical terms, remove phase/version language from UI, and enforce `qsTr` for all user-visible strings.
4. **Density and minimum-size check:** run the app at 1100x700 and common high-DPI settings, then fix panels using 7-9 px text or 22 px-high touch/click targets.
5. **Placeholder registry:** create a planning-owned list of remaining placeholder/blocked UI surfaces so they stop appearing as accidental product design.

---

## Files Audited

Primary QML surface:

- `src/qml_gui/main.qml`
- `src/qml_gui/BBLTopbar.qml`
- `src/qml_gui/Theme.qml`
- `src/qml_gui/pages/*.qml`
- `src/qml_gui/panels/*.qml`
- `src/qml_gui/components/*.qml`
- `src/qml_gui/controls/*.qml`
- `src/qml_gui/dialogs/*.qml`

Supporting context:

- `src/core/viewmodels/*.h`
- `src/core/viewmodels/*.cpp`
- `src/core/services/*.h`
- `src/core/services/*.cpp`
- `.planning/PROJECT.md`
- `.planning/REQUIREMENTS.md`
- `.planning/audits/2026-06-24-plan-implementation-alignment.md`
