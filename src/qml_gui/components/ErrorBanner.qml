import QtQuick
import QtQuick.Layouts
import ".."

// Warning-level (severity=1) banner - spans full width below title bar.
// Geometry and element structure mirror the upstream bbl-theme warning
// PopNotification (third_party/OrcaSlicer/src/slic3r/GUI/
// NotificationManager.cpp): window rounding + border (:221,:244,:247),
// content-driven height (set_next_window_size :612-618), wrapped text with
// the "..More" expand link (render_text :682-732), the collapse button
// (:988-1018), the borderless close glyph + full-height hit strip
// (render_close_button :830-875) and the rounded left level sign
// (bbl_render_left_sign :956-970). No per-level icon and no pending-count
// badge: the upstream warning level renders text only (render_left_sign
// :972-987 is fully commented out) and unclosed notifications stack instead
// of overflowing into a counter (render_notifications :3130-3155).
Rectangle {
    id: root

    Layout.fillWidth: true
    visible: backend.lastErrorSeverity === 1 && backend.lastErrorMessage !== ""
    clip:    true
    color:   Theme.bgBase

    // Window rounding and border size (upstream use_bbl_theme: WindowRounding
    // = 4x scale at :221, WindowBorderSize = radius / 4 at :247). The dark
    // theme draws the fixed gray border (NotificationManager.cpp:244).
    readonly property int windowRadius: 4
    radius: windowRadius
    border.width: windowRadius / 4
    border.color: Theme.overlayBorder

    // Warning level color for the left sign (upstream warning level uses
    // m_WarnColor, NotificationManager.cpp:225-226; brand warning token).
    readonly property color levelColor: Theme.statusWarning

    // Line height of the body font (upstream m_line_height =
    // ImGui::CalcTextSize("A").y, count_spaces :481). Starts from the font
    // metrics; refreshCollapsedLines() refines it to the natural height of
    // one laid-out text line.
    property real lineHeight: bodyMetrics.height

    // Icon button edge = glyph size x 1.25 (render_close_button :853-854).
    readonly property real buttonSize: 1.25 * lineHeight

    // Inner text geometry (count_spaces :483,:493): the text starts one line
    // height in (left_indentation) and stops three line heights before the
    // right edge (window_width_offset).
    readonly property real contentX: lineHeight
    readonly property real contentWidth: width - contentX - 3 * lineHeight

    readonly property string message: backend.lastErrorMessage
    readonly property int lineCount: Math.max(measureText.lineCount, 1)

    // Expansion state (upstream m_multiline): fresh notifications with <= 6
    // lines start expanded, longer ones start collapsed (init() :603-606);
    // "More" expands (:745-749), the collapse button collapses (:1013).
    property int expandedOverride: 0 // 0 = default, 1 = expanded, 2 = collapsed
    property string overrideMessage: ""
    readonly property bool isExpanded: {
        if (overrideMessage === message && expandedOverride !== 0)
            return expandedOverride === 1
        return lineCount <= 6
    }
    // "More" renders when the collapsed view hides lines (:722-725).
    readonly property bool hasMoreLink: !isExpanded && measureText.lineCount > 2

    // Collapsed-mode line substrings, extracted from the wrapped layout.
    property string collapsedLine1: ""
    property string collapsedLine2: ""

    readonly property real bodyImplicitHeight: isExpanded ? expandedBody.implicitHeight
                                                          : collapsedBody.implicitHeight
    // Height = (max(lines, 2) + 1) x line_height: one line height of combined
    // vertical padding on top of the content, minimum two content lines
    // (set_next_window_size :612-618); the collapsed view is fixed at
    // 2 x line_height + padding (:614-617).
    height: visible ? Math.max(3 * lineHeight, bodyImplicitHeight + lineHeight) : 0

    // ---- Measurement helpers ------------------------------------------------

    TextMetrics {
        id: bodyMetrics
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMD
    }

    TextMetrics {
        id: moreMetrics
        text: ".." + qsTr("More")
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMD
    }

    TextMetrics {
        id: spaceMetrics
        text: " "
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMD
    }

    // Hidden wrap reference: counts the wrapped lines and provides the
    // character position -> line mapping used by the collapsed two-line view.
    Text {
        id: measureText
        visible: false
        width: root.contentWidth
        text: root.message
        wrapMode: Text.Wrap
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMD
        onLineCountChanged: root.refreshCollapsedLinesLater()
    }

    function refreshCollapsedLinesLater() {
        // Coalesced deferred refresh: runs once after the pending binding and
        // layout changes settle.
        Qt.callLater(root.refreshCollapsedLines)
    }

    // Advance width of s with the body font.
    function textAdvance(s) {
        bodyMetrics.text = s
        return bodyMetrics.advanceWidth
    }

    // Character position just past the last character of wrapped line lineNo
    // (1-based); 0 for line 0, message length beyond the last line.
    function endOfWrappedLine(lineNo) {
        const limit = root.message.length
        if (lineNo < 1)
            return 0
        if (measureText.lineCount < lineNo)
            return limit
        const bottom = lineNo * root.lineHeight
        let lo = 0
        let hi = limit
        while (lo < hi) {
            const mid = (lo + hi) >> 1
            if (measureText.positionToRectangle(mid).y < bottom)
                lo = mid + 1
            else
                hi = mid
        }
        return lo
    }

    // Upstream skips one space/newline right after every wrapped break
    // (NotificationManager.cpp:636,647-648,698).
    function skipSeparator(pos) {
        if (pos < root.message.length && (root.message[pos] === " " || root.message[pos] === "\n"))
            return pos + 1
        return pos
    }

    function wrappedLineBounds(lineNo) {
        if (lineNo < 1 || measureText.lineCount < lineNo)
            return [root.message.length, root.message.length]
        const start = lineNo > 1 ? skipSeparator(endOfWrappedLine(lineNo - 1)) : 0
        const end = endOfWrappedLine(lineNo)
        return [start, end]
    }

    // Rebuilds the collapsed substrings and the refined line height.
    function refreshCollapsedLines() {
        measureText.forceLayout()
        if (measureText.lineCount > 0)
            root.lineHeight = measureText.implicitHeight / measureText.lineCount
        // The refined line height feeds contentWidth; force the relayout so
        // the extraction below sees the final geometry.
        measureText.forceLayout()
        const bounds1 = wrappedLineBounds(1)
        root.collapsedLine1 = root.message.substring(bounds1[0], bounds1[1])
        const bounds2 = wrappedLineBounds(2)
        let line2 = root.message.substring(bounds2[0], bounds2[1])
        if (root.hasMoreLink) {
            // Chop the second line until ".." plus "More" still fit on it
            // (render_text :699-703).
            const maxWidth = root.contentWidth - moreMetrics.advanceWidth
            while (line2.length > 0 && root.textAdvance(line2) > maxWidth)
                line2 = line2.substring(0, line2.length - 1)
        }
        root.collapsedLine2 = line2
    }

    Component.onCompleted: refreshCollapsedLinesLater()
    onMessageChanged: refreshCollapsedLinesLater()
    onWidthChanged: refreshCollapsedLinesLater()
    onIsExpandedChanged: refreshCollapsedLinesLater()

    // ---- Left level sign ----------------------------------------------------

    // Bar of width 2xWindowRadius, full height, with the outer (left) corners
    // rounded at WindowRadius (bbl_render_left_sign :956-970).
    Item {
        x: root.border.width
        y: root.border.width
        width: 2 * root.windowRadius
        height: root.height - 2 * root.border.width

        Rectangle {
            anchors.fill: parent
            radius: root.windowRadius
            color: root.levelColor
        }
        // Square-corner patch over the right half of the bar (:963-964,969).
        Rectangle {
            x: root.windowRadius
            width: root.windowRadius
            height: parent.height
            color: root.levelColor
        }
    }

    // ---- Text -----------------------------------------------------------------

    // Expanded view: the whole message wraps (render_text multiline branch,
    // :690). A single-line message is centered; longer content starts half a
    // line height below the top edge (starting_y :686).
    Text {
        id: expandedBody
        visible: root.isExpanded
        x: root.contentX
        y: root.lineCount === 1 ? (root.height - implicitHeight) / 2 : root.lineHeight / 2
        width: root.contentWidth
        text: root.message
        wrapMode: Text.Wrap
        color: Theme.statusWarning
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMD
    }

    // Collapsed view: at most two lines; when more lines exist the second one
    // is chopped and followed by the "More" hyperlink (render_text :690-725).
    Column {
        id: collapsedBody
        visible: !root.isExpanded
        x: root.contentX
        width: root.contentWidth
        spacing: 0
        anchors.verticalCenter: parent.verticalCenter

        Text {
            width: parent.width
            visible: root.collapsedLine1 !== ""
            text: root.collapsedLine1
            color: Theme.statusWarning
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMD
        }

        Row {
            visible: measureText.lineCount > 1
            spacing: spaceMetrics.advanceWidth

            Text {
                text: root.collapsedLine2 + (root.hasMoreLink ? ".." : "")
                color: Theme.statusWarning
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMD
            }

            // "More" hyperlink with a 1px underline 2px above the glyph
            // bottom (render_hypertext :734-792). Brand accent green replaces
            // the upstream teal hyperlink color.
            Text {
                id: moreLink
                visible: root.hasMoreLink
                text: qsTr("More")
                color: moreArea.containsMouse ? Theme.accentDark : Theme.accent
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMD

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 2
                    height: 1
                    color: parent.color
                }

                // Invisible hit area padding around the label
                // (render_hypertext :738-743).
                MouseArea {
                    id: moreArea
                    x: -4
                    y: -5
                    width: parent.width + 6
                    height: parent.height + 10
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.overrideMessage = root.message
                        root.expandedOverride = 1
                    }
                }
            }
        }
    }

    // ---- Collapse button --------------------------------------------------------

    // Rendered only when expanded with more than 3 lines (render() :336-338):
    // bottom-right glyph icon, one glyph x1.25 in size, its bottom edge 5px
    // above the window bottom (render_minimize_button :988-1018).
    Image {
        id: minimizeButton
        visible: root.isExpanded && root.lineCount > 3
        x: root.width - 1.8 * root.lineHeight
        y: root.height - height - 5
        width: root.buttonSize
        height: root.buttonSize
        source: minimizeZone.hovered ? "qrc:/qml/assets/icons/notification_minimalize_hover_dark.svg"
                                     : "qrc:/qml/assets/icons/notification_minimalize_dark.svg"
        TapHandler {
            onTapped: {
                root.overrideMessage = root.message
                root.expandedOverride = 2
            }
        }
    }

    // Hover zone of the collapse glyph: the right tenth of the banner, bottom
    // two line heights (render_minimize_button :1001-1006).
    Item {
        x: root.width - root.width / 10
        y: root.height - 2 * root.lineHeight + 1
        width: root.width / 10
        height: 2 * root.lineHeight - 1
        HoverHandler { id: minimizeZone }
    }

    // ---- Close button -------------------------------------------------------------

    // Borderless transparent glyph button at width - 2.75 x line_height,
    // flush with the top while the collapse button is visible and otherwise
    // at half the height minus the button size (render_close_button
    // :853-860). Hovering the right-tenth/top zone swaps in the hover glyph
    // (:849-852).
    Image {
        id: closeButton
        x: root.width - 2.75 * root.lineHeight
        y: minimizeButton.visible ? 0 : root.height / 2 - root.buttonSize
        width: root.buttonSize
        height: root.buttonSize
        source: closeZone.hovered ? "qrc:/qml/assets/icons/notification_close_hover_dark.svg"
                                  : "qrc:/qml/assets/icons/notification_close_dark.svg"
        TapHandler { onTapped: backend.dismissNotification() }
    }

    Item {
        x: root.width - root.width / 10
        y: 0
        width: root.width / 10
        height: 2 * root.lineHeight + 10
        HoverHandler { id: closeZone }
    }

    // Invisible full-height hit strip on the right edge; it stops two line
    // heights short of the bottom while the collapse button is visible
    // (render_close_button :866-872).
    Item {
        x: root.width - 2.35 * root.lineHeight
        y: 0
        width: 2.125 * root.lineHeight
        height: minimizeButton.visible ? root.height - 2 * root.lineHeight : root.height
        TapHandler { onTapped: backend.dismissNotification() }
    }
}
