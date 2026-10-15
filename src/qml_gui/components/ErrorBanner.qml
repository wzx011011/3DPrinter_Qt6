import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Layouts
import OWzx

// Warning-level (severity=1) banner - spans full width below title bar.
// Geometry and element structure mirror the upstream bbl-theme warning
// PopNotification (third_party/OrcaSlicer/src/slic3r/GUI/
// NotificationManager.cpp): window rounding + border (:221,:244,:247),
// content-driven height (set_next_window_size :612-618), wrapped text with
// the "More" expand link (render_text :682-732), the collapse button
// (:988-1018), the borderless close glyph + full-height hit strip
// (render_close_button :830-875) and the rounded left level sign
// (bbl_render_left_sign :956-970). No per-level icon and no pending-count
// badge: the upstream warning level renders text only (render_left_sign
// :972-987 is fully commented out) and unclosed notifications stack instead
// of overflowing into a counter (render_notifications :3130-3155).
//
// X2 (audit 2026-10-10 (4)): the collapse machinery is the unified
// notification-window contract shared with ErrorToast/NotificationCenter --
// a hidden wrap measurer drives the full line count, the collapsed view is
// a maximumLineCount:2 + elide Text with the trailing "More" expander and
// the minimize glyph collapses it again (ErrorToast's technique). The former
// banner-private binary-search wrapped-line extraction
// (positionToRectangle/endOfWrappedLine) is retired: one implementation of
// the upstream折叠提取 semantics instead of three divergent ones.
Rectangle {
    id: root

    Layout.fillWidth: true
    visible: Backend.lastErrorSeverity === 1 && Backend.lastErrorMessage !== ""
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
    // ImGui::CalcTextSize("A").y, count_spaces :481).
    property real lineHeight: bodyMetrics.height

    // Icon button edge = glyph size x 1.25 (render_close_button :853-854).
    readonly property real buttonSize: 1.25 * lineHeight

    // Inner text geometry (count_spaces :483,:493): the text starts one line
    // height in (left_indentation) and stops three line heights before the
    // right edge (window_width_offset).
    readonly property real contentX: lineHeight
    readonly property real contentWidth: width - contentX - 3 * lineHeight

    readonly property string message: Backend.lastErrorMessage
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

    readonly property real bodyImplicitHeight: isExpanded ? expandedBody.implicitHeight
                                                          : collapsedBody.height
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

    // Hidden wrap reference: the full wrapped line count of the message.
    // Drives isExpanded/hasMoreLink without feeding truncated state back
    // into the visible text (no binding loop) -- the ErrorToast measurer
    // pattern, now the single collapse technique across the notification
    // surfaces.
    Text {
        id: measureText
        visible: false
        width: root.contentWidth
        text: root.message
        wrapMode: Text.Wrap
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMD
    }

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

    // Collapsed view: exactly two rendered lines, the second trimmed with an
    // ellipsis and the width reserved for the "More" expander (upstream trims
    // line 2 until "..More" fits, render_text :699-711 + :722-725). Same
    // contract as the ErrorToast collapsed entry.
    Item {
        id: collapsedBody
        visible: !root.isExpanded
        x: root.contentX
        width: root.contentWidth
        height: 2 * root.lineHeight
        anchors.verticalCenter: parent.verticalCenter

        Text {
            id: collapsedText
            anchors.left: parent.left
            anchors.top: parent.top
            width: root.hasMoreLink ? parent.width - moreLink.width - 4
                                    : parent.width
            text: root.message
            wrapMode: Text.Wrap
            maximumLineCount: 2
            elide: Text.ElideRight
            color: Theme.statusWarning
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMD
        }

        // "More" expander (render_hypertext more=true, :722-725/:743-749):
        // click re-expands to the full wrapped view. Brand accent, bold --
        // the ErrorToast link form, replacing the banner-private underline
        // Rectangle variant.
        Text {
            id: moreLink
            visible: root.hasMoreLink
            anchors.left: collapsedText.right
            anchors.leftMargin: 4
            anchors.bottom: parent.bottom
            height: root.lineHeight
            verticalAlignment: Text.AlignVCenter
            text: qsTr("More")
            color: moreArea.containsMouse ? Theme.accentDark : Theme.accent
            font.pixelSize: Theme.fontSizeMD
            font.bold: true

            MouseArea {
                id: moreArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.overrideMessage = root.message
                    root.expandedOverride = 1
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
        TapHandler { onTapped: Backend.dismissNotification() }
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
        TapHandler { onTapped: Backend.dismissNotification() }
    }
}
