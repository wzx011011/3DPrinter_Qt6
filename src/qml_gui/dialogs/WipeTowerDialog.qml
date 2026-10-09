import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.6b -- WipeTowerDialog (upstream truth: the wx host WipingDialog in
// WipeTowerDialog.cpp wraps resources/web/flush/WipingDialog.html -- a
// auto-calc tip panel, a Re-calculate button row, a scrollable N×N
// flush-volume matrix with sticky header/row badges, description/range
// texts and a multiplier input; WipingDialog.html:261-345).
//
// U05 alignment (WipingDialog.html / WipeTowerDialog.cpp):
//  - adaptive size, min width 450, matrix wrapped in CxScrollView with a
//    500px max viewport and sticky header row + row badges
//    (WipeTowerDialog.cpp:431-451, WipingDialog.html:84-96);
//  - badges render the real filament colours (ConfigViewModel::
//    wizardFilamentColours -> PresetServiceMock::activeFilamentColours) with
//    the badge text colour picked by luminance (WipingDialog.html:446-452,
//    532-534);
//  - flush multiplier persists through ConfigViewModel::setFlushMultiplier
//    (QSettings "flush/multiplier"; upstream writes flush_multiplier onto
//    the project config, WipeTowerDialog.cpp:245-246) and is read back on
//    open; valid range [0, 3] (WipeTowerDialog.cpp:199-200);
//  - cell edits clamp into [0, 20000] (limitDisplayVal clamps at
//    m_max_flush_volumes, which is the WipingDialog ctor default
//    g_max_flush_volume = 20000 -- WipeTowerDialog.hpp:50 +
//    FlushVolCalc.cpp:13; html:481-489); the [min, 900] suggestion label
//    keeps the html:456 TYPICAL_MAX_FLUSH_VOLUME; out-of-range cells
//    render red, deviations from the default matrix and a multiplier ≠ 1
//    render orange (html:626-657);
//  - the former 简单/高级 mode row was removed (it only toggled its own
//    colours, no upstream counterpart), the ramming block was removed (it
//    belongs to the separate upstream RammingDialog, WipeTowerDialog.cpp:
//    28-58 -- re-add with the RammingDialog feature batch) and the Reset
//    button was removed (no upstream counterpart; Re-calculate re-derives
//    the matrix from the filament colours);
//  - Esc closes (upstream Esc -> EndModal, WipeTowerDialog.cpp:554-563).
// Defer: multi-nozzle left/right selectors + per-nozzle matrices/multipliers
// (service-layer rework), title i18n alignment.
// Usage: WipeTowerDialog { id: dlg }  ->  dlg.open()

CxDialog {
    id: root

    closePolicy: Popup.CloseOnEscape

    dialogTitle: qsTr("擦料塔设置")

    anchors.centerIn: parent

    // 20px content gutter (upstream container padding: 20,
    // WipingDialog.html:31) = 4px dialog padding + 16px content margins.
    padding: Theme.spacingXS

    // Adaptive width with a 450px floor (upstream clamps the applied size to
    // a usable minimum, WipeTowerDialog.cpp:450-451).
    width: Math.max(450, bodyColumn.implicitWidth
                    + 2 * Theme.spacingXL + 2 * Theme.spacingXS)
    // Adaptive height: Popup sizes from contentItem + header + footer; the
    // matrix viewport itself is capped at 500px (WipeTowerDialog.cpp:431-433).
    implicitHeight: bodyColumn.implicitHeight + 2 * Theme.spacingXL
                    + 2 * Theme.spacingXS + Theme.dialogHeaderHeight
                    + Theme.dialogFooterHeight

    // ── Upstream constants ──
    // Flush multiplier valid range [0, 3] (WipeTowerDialog.cpp:199-200).
    readonly property real minFlushMultiplier: 0.0
    readonly property real maxFlushMultiplier: 3.0
    // Typical suggested max flushing volume (TYPICAL_MAX_FLUSH_VOLUME = 900,
    // WipingDialog.html:456); lower bound = minFlushVolume.
    readonly property int typicalMaxFlushVolume: 900
    // Hard cell limit: limitDisplayVal and the red check clamp at
    // m_max_flush_volumes, whose ctor default is g_max_flush_volume = 20000
    // (WipeTowerDialog.hpp:50 + FlushVolCalc.cpp:13), not the 900 label value.
    readonly property int maxFlushVolume: 20000

    // v5.12 gap-closure: flush volume matrix computed from filament colours
    // via PresetServiceMock::calculateFlushMatrix (FlushVolCalculator). Falls
    // back to a flat default when no preset service is available.
    readonly property var configVm: typeof backend !== "undefined" && backend
        ? backend.configViewModel : null
    property var flushMatrix: defaultMatrix(4)
    property int extruderCount: 4
    // Flush matrices are N×N over the ACTIVE machine's filament slots
    // (upstream caps the per-machine count well below this); anything larger
    // means the matrix was derived over the whole vendor colour catalogue
    // (Creality alone ships 500+) and must not reach the Grid as N² cells.
    readonly property int maxExtruderCount: 16
    // Real filament colours (ConfigViewModel::wizardFilamentColours); badges
    // fall back to a neutral token when the list is shorter than the matrix.
    property var extruderColors: []

    // Flat default (uniform) matrix for N extruders.
    function defaultMatrix(n) {
        var m = []
        for (var i = 0; i < n; ++i) {
            var row = []
            for (var j = 0; j < n; ++j)
                row.push(i === j ? 0 : 140)
            m.push(row)
        }
        return m
    }
    // The reference the orange "deviates from default" tint compares
    // against. Upstream's m_default_matrix is the colour-derived calculation
    // delivered at open (WipingDialog.html:499-501, compared at :644-650),
    // so snapshot the loaded matrix below instead of a synthetic uniform
    // grid (which flagged every loaded cell as a false deviation).
    property var referenceMatrix: defaultMatrix(4)

    function copyMatrix(matrix) {
        var copy = []
        for (var row = 0; row < matrix.length; ++row)
            copy.push(matrix[row].slice())
        return copy
    }

    // Convert a flat QVariantList (row-major N*N) to a 2D array.
    function flatToMatrix(flat, n) {
        if (!flat || flat.length === 0) return defaultMatrix(n)
        var m = []
        for (var i = 0; i < n; ++i) {
            var row = []
            for (var j = 0; j < n; ++j)
                row.push(flat[i * n + j])
            m.push(row)
        }
        return m
    }

    // Badge colour for extruder slot `slot` (real filament colour when
    // available, neutral token otherwise).
    function filamentColourAt(slot) {
        return (extruderColors !== null && slot < extruderColors.length)
            ? extruderColors[slot] : Theme.bgHover
    }

    // Badge text colour by luminance (WipingDialog.html:446-452, 532-534:
    // luminance > 0.5 renders black, otherwise white).
    function badgeTextColour(colourHex) {
        var hex = String(colourHex)
        if (hex.length < 7 || hex.charAt(0) !== "#")
            return Theme.textOnAccent
        var r = parseInt(hex.substring(1, 3), 16)
        var g = parseInt(hex.substring(3, 5), 16)
        var b = parseInt(hex.substring(5, 7), 16)
        var luminance = (0.299 * r + 0.587 * g + 0.114 * b) / 255
        return luminance > 0.5 ? "black" : Theme.textOnAccent
    }

    // True when any off-diagonal cell sits outside the suggested window
    // (turns the volume-range panel red, WipingDialog.html:659-666).
    readonly property bool hasOutOfRangeCell: {
        for (var i = 0; i < flushMatrix.length; ++i) {
            for (var j = 0; j < flushMatrix[i].length; ++j) {
                if (i === j)
                    continue
                var v = Number(flushMatrix[i][j])
                if (v < minFlushVolume || v > maxFlushVolume)
                    return true
            }
        }
        return false
    }

    Component.onCompleted: {
        if (configVm) {
            var flat = configVm.wizardFlushMatrix()
            if (flat && flat.length > 0) {
                var n = Math.sqrt(flat.length)
                var nR = Math.round(n)
                if (nR >= 2 && nR <= maxExtruderCount && Math.abs(n - nR) < 1e-9) {
                    extruderCount = nR
                    flushMatrix = flatToMatrix(flat, nR)
                }
            }
            // Real filament colours for the badges (upstream feeds
            // filament_colour into the table, WipingDialog.html:492).
            var colours = configVm.wizardFilamentColours()
            if (colours && colours.length > 0)
                extruderColors = colours.slice(0, maxExtruderCount)
            // Read the persisted flush multiplier back on open
            // (ConfigViewModel QSettings channel; upstream restores
            // flush_multiplier from the project config).
            flushMultiplier = configVm.flushMultiplier()
        }
        // Upstream's default matrix is fixed at open (buildTable delivers
        // default_matrixs once; Re-calculate does not rebuild it), so
        // snapshot whatever the service loaded as the deviation reference.
        referenceMatrix = copyMatrix(flushMatrix)
    }

    // Wiping settings
    property real flushMultiplier: 1.0
    // Lower bound of the suggested flushing-volume window. OWzx stand-in for
    // the upstream per-filament min_flush_volumes config (get_min_flush_volumes);
    // editable here so the [min, 900] suggestion stays truthful.
    property real minFlushVolume: 80.0

    contentItem: ColumnLayout {
        id: bodyColumn
        anchors.fill: parent
        anchors.margins: Theme.spacingXL
        spacing: Theme.spacingMD

        // ── Auto-calc tip panel (WipingDialog.html:263-267, copy
        //    WipeTowerDialog.cpp:380) ──
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: tipText.implicitHeight + 2 * Theme.spacingLG
            // Dark tip panel #4c4c55 (WipingDialog.html:190-193) maps to the
            // bgPanel surface token.
            color: Theme.bgPanel

            Text {
                id: tipText
                anchors.fill: parent
                anchors.margins: Theme.spacingLG
                text: qsTr("Orca would re-calculate your flushing volumes everytime the filaments color changed or filaments changed. You could disable the auto-calculate in Orca Slicer > Preferences")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.Wrap
            }
        }

        // ── Re-calculate row (button moved to the top per
        //    WipingDialog.html:269-287; the per-nozzle selector beside it is
        //    deferred with the multi-nozzle batch) ──
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD

            CxButton {
                text: qsTr("计算")
                cxStyle: CxButton.Style.Secondary
                compact: true
                // v5.12: recompute flush matrix from filament colours.
                enabled: configVm !== null
                onClicked: {
                    if (!configVm) return
                    var flat = configVm.wizardFlushMatrix()
                    if (flat && flat.length > 0) {
                        var n = Math.sqrt(flat.length)
                        var nR = Math.round(n)
                        if (nR >= 2 && nR <= maxExtruderCount && Math.abs(n - nR) < 1e-9) {
                            extruderCount = nR
                            flushMatrix = flatToMatrix(flat, nR)
                        }
                    }
                }
            }

            Item { Layout.fillWidth: true }
        }

        // ── Flush volume matrix (N×N; sticky header row + sticky row badges
        //    over a CxScrollView body capped at 500px,
        //    WipingDialog.html:289-299 + sticky css :84-113) ──
        Rectangle {
            id: matrixWrap
            Layout.fillWidth: true
            readonly property int n: root.extruderCount
            readonly property int cellWidth: 60
            readonly property int cellHeight: 25
            readonly property int cornerWidth: 56
            implicitWidth: cornerWidth + n * cellWidth + 2 * border.width
            implicitHeight: cellHeight
                            + Math.min(n * cellHeight, 500) + 2 * border.width
            color: Theme.bgInset
            border.color: Theme.borderInput
            border.width: 1

            // Sticky corner: the from/to label (WipingDialog.html:526-529).
            Rectangle {
                width: matrixWrap.cornerWidth
                height: matrixWrap.cellHeight
                color: Theme.bgPanel

                Text {
                    anchors.centerIn: parent
                    text: qsTr("from/to")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                }
            }

            // Sticky header strip: column badges follow the body's horizontal
            // scroll offset.
            Item {
                x: matrixWrap.cornerWidth
                width: parent.width - x - matrixWrap.border.width
                height: matrixWrap.cellHeight
                clip: true

                Row {
                    x: -bodyFlick.contentX
                    spacing: 0

                    Repeater {
                        model: matrixWrap.n

                        Rectangle {
                            required property int index
                            width: matrixWrap.cellWidth
                            height: matrixWrap.cellHeight
                            color: Theme.bgPanel

                            Rectangle {
                                anchors.centerIn: parent
                                width: 16
                                height: 16
                                radius: Theme.radiusMD
                                color: root.filamentColourAt(index)
                                border.color: Theme.borderDefault
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: index + 1
                                    color: root.badgeTextColour(
                                                root.filamentColourAt(index))
                                    font.pixelSize: Theme.fontSizeXS
                                    font.bold: true
                                }
                            }
                        }
                    }
                }
            }

            // Sticky row strip: row badges follow the body's vertical scroll
            // offset; each backing cell mirrors the body zebra tint.
            Item {
                y: matrixWrap.cellHeight
                width: matrixWrap.cornerWidth
                height: parent.height - y - matrixWrap.border.width
                clip: true

                Column {
                    y: -bodyFlick.contentY
                    spacing: 0

                    Repeater {
                        model: matrixWrap.n

                        Rectangle {
                            required property int index
                            width: matrixWrap.cornerWidth
                            height: matrixWrap.cellHeight
                            // Zebra: odd rows (1-based) carry the lighter
                            // tint (WipingDialog.html:98-104, dark :231-237).
                            color: index % 2 === 0 ? Theme.bgPanel
                                                   : "transparent"

                            Rectangle {
                                anchors.centerIn: parent
                                width: 16
                                height: 16
                                radius: Theme.radiusMD
                                color: root.filamentColourAt(index)
                                border.color: Theme.borderDefault
                                border.width: 1

                                Text {
                                    anchors.centerIn: parent
                                    text: index + 1
                                    color: root.badgeTextColour(
                                                root.filamentColourAt(index))
                                    font.pixelSize: Theme.fontSizeXS
                                    font.bold: true
                                }
                            }
                        }
                    }
                }
            }

            // Scrolling body (CxScrollView, max 500px implicit height).
            CxScrollView {
                id: bodyScroll
                x: matrixWrap.cornerWidth
                y: matrixWrap.cellHeight
                width: parent.width - x - matrixWrap.border.width
                height: parent.height - y - matrixWrap.border.width
                clip: true

                Flickable {
                    id: bodyFlick
                    clip: true
                    contentWidth: bodyGrid.implicitWidth
                    contentHeight: bodyGrid.implicitHeight
                    interactive: contentWidth > bodyScroll.availableWidth
                                 || contentHeight > bodyScroll.availableHeight

                    Grid {
                        id: bodyGrid
                        columns: matrixWrap.n
                        rows: matrixWrap.n
                        rowSpacing: 0
                        columnSpacing: 0

                        Repeater {
                            model: matrixWrap.n * matrixWrap.n

                            // Zebra backing cell (odd rows lighter) hosting
                            // the editable flush volume.
                            Rectangle {
                                id: cellRect
                                required property int index
                                readonly property int rowIndex: Math.floor(index / matrixWrap.n)
                                readonly property int columnIndex: index % matrixWrap.n
                                width: matrixWrap.cellWidth
                                height: matrixWrap.cellHeight
                                color: rowIndex % 2 === 0 ? Theme.bgPanel
                                                          : "transparent"

                                CxTextField {
                                    anchors.fill: parent
                                    anchors.margins: Theme.spacingXXS
                                    font.pixelSize: Theme.fontSizeSM
                                    horizontalAlignment: Text.AlignHCenter
                                    text: root.flushMatrix[cellRect.rowIndex] !== undefined
                                        ? root.flushMatrix[cellRect.rowIndex][cellRect.columnIndex] : "0"
                                    enabled: cellRect.rowIndex !== cellRect.columnIndex
                                    // Diagnostic tint (WipingDialog.html:
                                    // 626-657): out-of-range red, deviation
                                    // from the default matrix orange.
                                    color: {
                                        if (!enabled)
                                            return Theme.textTertiary
                                        var v = Number(text)
                                        if (v < root.minFlushVolume
                                                || v > root.maxFlushVolume)
                                            return Theme.statusError
                                        var reference = root.referenceMatrix[cellRect.rowIndex][cellRect.columnIndex]
                                        if (v !== reference)
                                            return Theme.statusWarning
                                        return Theme.textPrimary
                                    }
                                    onEditingFinished: {
                                        var value = Number(text)
                                        if (!isFinite(value))
                                            return
                                        // Clamp into the valid window before
                                        // writing back (limitDisplayVal,
                                        // WipingDialog.html:481-489: [0,
                                        // m_max_flush_volumes] = 20000).
                                        value = Math.max(0, Math.min(
                                                             value,
                                                             root.maxFlushVolume))
                                        var matrix = root.copyMatrix(root.flushMatrix)
                                        matrix[rowIndex][columnIndex] = value
                                        root.flushMatrix = matrix
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ── Description section (WipingDialog.html:301-334) ──
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingXS

            Text {
                Layout.fillWidth: true
                text: qsTr("Flushing volume (mm³) for each filament pair.")
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.Wrap
            }

            Text {
                Layout.fillWidth: true
                // [min, 900] window (updateVolumeRange, WipingDialog.html:
                // 454-459); the panel turns red while a cell is out of range
                // (html:659-666).
                text: qsTr("Suggestion: Flushing Volume in range [%1, %2]")
                    .arg(root.minFlushVolume.toFixed(0))
                    .arg(root.typicalMaxFlushVolume)
                color: root.hasOutOfRangeCell ? Theme.statusError
                                              : Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.Wrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD

                Text {
                    Layout.preferredWidth: 70
                    text: qsTr("擦洗倍率")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeMD
                }
                CxTextField {
                    Layout.preferredWidth: 60
                    implicitHeight: 25
                    text: root.flushMultiplier.toFixed(2)
                    // Multiplier ≠ 1 renders orange (WipingDialog.html:
                    // 626-633).
                    color: Math.abs(root.flushMultiplier - 1.0) > 0.001
                           ? Theme.statusWarning : Theme.textPrimary
                    onEditingFinished: {
                        var value = Number(text)
                        if (!isFinite(value)) {
                            text = root.flushMultiplier.toFixed(2)
                            return
                        }
                        // Valid range [0, 3] (WipeTowerDialog.cpp:199-200;
                        // input clamp WipingDialog.html:599-609).
                        value = Math.max(root.minFlushMultiplier,
                                         Math.min(value, root.maxFlushMultiplier))
                        root.flushMultiplier = value
                        text = value.toFixed(2)
                    }
                }

                Text {
                    Layout.preferredWidth: 70
                    text: qsTr("最小体积")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeMD
                }
                CxTextField {
                    Layout.preferredWidth: 60
                    implicitHeight: 25
                    text: root.minFlushVolume.toFixed(1)
                    onEditingFinished: {
                        var value = Number(text)
                        if (!isFinite(value)) {
                            text = root.minFlushVolume.toFixed(1)
                            return
                        }
                        value = Math.max(0, Math.min(value,
                                                     root.typicalMaxFlushVolume))
                        root.minFlushVolume = value
                        text = value.toFixed(1)
                    }
                }

                Item { Layout.fillWidth: true }
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("The multiplier should be in range [0.50, 3.00].")
                color: Theme.textTertiary
                font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.Wrap
            }
        }
    }

    footer: Rectangle {
        width: parent.width
        height: Theme.dialogFooterHeight
        color: Theme.bgSurface
        radius: Theme.radiusLG
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 12
            color: parent.color
        }

        // Centered button row (WipingDialog.html:171-177
        // .button-container justify-content: center).
        RowLayout {
            anchors.centerIn: parent
            spacing: Theme.spacingMD

            CxButton {
                text: qsTr("确定")
                cxStyle: CxButton.Style.Primary
                // Phase 236 (DLG-02): OK persists the edited flush matrix via
                // PresetServiceMock::saveFlushVolumes (upstream writes
                // flush_volumes_matrix on the project config at
                // Plater.cpp:2125). The saved matrix wins over the derived
                // one on the next open (calculateFlushMatrix reads it back).
                // U05: the flush multiplier persists alongside it through the
                // ConfigViewModel QSettings channel (upstream
                // flush_multiplier, WipeTowerDialog.cpp:245-246).
                onClicked: {
                    if (configVm && flushMatrix && flushMatrix.length > 0)
                        configVm.wizardSaveFlushVolumes(root.copyMatrix(flushMatrix))
                    if (configVm)
                        configVm.setFlushMultiplier(root.flushMultiplier)
                    root.accept()
                }
            }

            CxButton {
                text: qsTr("取消")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }
        }
    }
}
