import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Controls
import QtQuick.Layouts
import OWzx

// Notification center panel -- shows notification history list.
// Opens from the bell icon in the topbar; main.qml hosts the popup.
//
// Each row mirrors the upstream notification window form 1:1
// (NotificationManager.cpp):
//   - full-height rounded left sign bar in the severity color
//     (bbl_render_left_sign, cpp:956-970: width = 2 x WindowRadius = 8,
//     radius = WindowRadius = 4, plus a square-off fill on the bar's right
//     half, cpp:963-969);
//   - error / seriousWarning switch to the block-notification form instead:
//     block error glyph at line_height / 3 (bbl_render_block_notif_left_sign,
//     cpp:943-954) and text indented by 32 + line_height (cpp:494-496);
//   - per-row close glyph at 2.75 x line_height from the right edge, glyph
//     size 1.25 x line_height, plus an invisible 2.125 x line_height
//     full-height click strip (render_close_button, cpp:849-872); block rows
//     use the larger 2 x line_height glyph at 2.0 x line_height from the
//     right (bbl_render_block_notif_buttons, cpp:877-903);
//   - text collapses to 2 lines with a hypertext "More" link that expands
//     (render_text, cpp:690-703 + 722-725) and a minimize glyph once the
//     expanded text passes 3 lines (cpp:337-338 + render_minimize_button,
//     cpp:988-1017: x = width - 1.8 x line_height, y = height - button - 5).
// The upstream line height at scale 1 is 18 px (window width = 25 x 18 = 450,
// cpp:498; collapsed window = 3 x line_height, hpp:639-640).
//
// X2 (audit 2026-10-10 ④): the row chrome (close/minimize glyphs, "More"
// expander) is the unified ErrorToast notification-window contract — shared
// svg glyph assets and the maximumLineCount collapse — instead of a third
// divergent rendering of the upstream form; the header action chips are
// system CxButtons. Severity color/icon lookups stay on the canonical
// Theme.severityColors/severityIcons palettes (Phase 167 Cmp-01).
Item {
    id: root

    signal closeRequested()

    // Upstream notification line height at scale 1 (NotificationManager.cpp:498).
    readonly property int notifLineHeight: 18

    // Phase 167 (Cmp-01): collapsed the private 9-level severity->color and
    // severity->icon tables into lookups against the canonical Theme.severityColors
    // and Theme.severityIcons palettes (Phase 160 tokens). Single source of
    // truth for the notification system (was duplicated across ErrorBanner/
    // ErrorToast/NotificationCenter per Components-UI-REVIEW).
    function severityColor(sev) {
        if (sev >= 0 && sev < Theme.severityColors.length)
            return Theme.severityColors[sev]
        return Theme.accent  // fallback
    }

    function severityIcon(sev) {
        if (sev >= 0 && sev < Theme.severityIcons.length)
            return Theme.severityIcons[sev]
        return "i"  // fallback
    }

    // Error / SeriousWarning render the upstream block-notification form
    // (NotificationManager.cpp:494-496) instead of the left sign bar.
    function isBlockLevel(sev) {
        return sev === 3 || sev === 4  // NotiError / NotiSeriousWarning
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusXL
        color: Theme.bgInset
        border.color: Theme.bgCard
        border.width: 1

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            // Header
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: qsTr("通知中心")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeLG
                    font.bold: true
                    Layout.fillWidth: true
                }

                // Unread badge
                Rectangle {
                    visible: Backend.unreadHistoryCount > 0
                    width: unreadBadge.implicitWidth + 12
                    height: 20
                    radius: Theme.radiusXL
                    color: Theme.statusError

                    Text {
                        id: unreadBadge
                        anchors.centerIn: parent
                        text: Backend.unreadHistoryCount > 99 ? "99+" : Backend.unreadHistoryCount.toString()
                        color: Theme.accentDark
                        font.pixelSize: Theme.fontSizeXS
                        font.bold: true
                    }
                }

                // Header actions. X2: the three hand-drawn 24px glyph chips
                // are system CxButtons (Chip style); the history list rows
                // below keep the upstream window form.
                CxButton {
                    visible: Backend.unreadHistoryCount > 0
                    text: "✓"
                    compact: true
                    cxStyle: CxButton.Style.Chip
                    toolTipText: qsTr("全部标为已读")
                    onClicked: Backend.markHistoryRead()
                }

                CxButton {
                    visible: Backend.historyCount > 0
                    text: "🗑"
                    compact: true
                    cxStyle: CxButton.Style.Chip
                    toolTipText: qsTr("清空通知记录")
                    onClicked: Backend.clearHistory()
                }

                CxButton {
                    text: "✕"
                    compact: true
                    cxStyle: CxButton.Style.Chip
                    onClicked: root.closeRequested()
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.bgCard }

            // Notification list, most important first (importance stable sort
            // in BackendContext::archiveNotification, upstream sort_notifications
            // NotificationManager.cpp:3136 + 3232-3242).
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: Backend.notificationHistory
                spacing: 4

                delegate: Rectangle {
                    id: card
                    required property var modelData

                    readonly property int sev: modelData.severity !== undefined ? modelData.severity : 0
                    readonly property bool blockLevel: root.isBlockLevel(sev)
                    readonly property color sevColor: root.severityColor(sev)
                    // Expanded state of the collapsed-2-lines message
                    // (upstream m_multiline, NotificationManager.cpp:690).
                    property bool expanded: false

                    // Upstream text indentation (m_left_indentation,
                    // NotificationManager.cpp:483 normal = line_height,
                    // cpp:495 block = 32 + line_height).
                    readonly property int contentLeft: blockLevel ? 32 + root.notifLineHeight : root.notifLineHeight
                    // Upstream wrap width reserves the close-button zone
                    // (m_window_width_offset, cpp:493 = left_indentation +
                    // 3 x line_height; block rows use 90, cpp:496).
                    readonly property int contentRight: blockLevel ? 90 - contentLeft : 3 * root.notifLineHeight

                    width: ListView.view.width
                    height: cardColumn.implicitHeight + 2 * 9
                    // Upstream WindowRounding = 4 x scale (cpp:185, cpp:221).
                    radius: Theme.radiusSM
                    color: Theme.bgPanel
                    border.color: Theme.scrollBarTrackColor
                    // Upstream WindowBorderSize = WindowRadius / 4 (cpp:247).
                    border.width: 1

                    // Left sign: full-height severity bar, rounded like the
                    // window with a square-off fill on the right half
                    // (bbl_render_left_sign, NotificationManager.cpp:956-970).
                    // Block rows paint the whole window in the severity color
                    // upstream instead; the Qt6 surface stays on the neutral
                    // token, so they show the block glyph only.
                    Rectangle {
                        visible: !card.blockLevel
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 8  // 2 x WindowRadius (cpp:961)
                        radius: Theme.radiusSM // = WindowRadius (cpp:968)
                        color: card.sevColor
                    }
                    Rectangle {
                        // Square-off fill on the right half of the rounded bar
                        // (upstream second AddRectFilled, cpp:963-969).
                        visible: !card.blockLevel
                        anchors.left: parent.left
                        anchors.leftMargin: 4
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 4
                        color: card.sevColor
                    }

                    // Block error glyph (bbl_render_block_notif_left_sign,
                    // NotificationManager.cpp:943-954: x = line_height / 3,
                    // vertically centered).
                    Text {
                        visible: card.blockLevel
                        anchors.left: parent.left
                        anchors.leftMargin: root.notifLineHeight / 3
                        anchors.verticalCenter: parent.verticalCenter
                        text: root.severityIcon(card.sev)
                        color: card.sevColor
                        font.pixelSize: root.notifLineHeight
                        font.bold: true
                    }

                    Column {
                        id: cardColumn
                        anchors.left: parent.left
                        anchors.leftMargin: card.contentLeft
                        anchors.right: parent.right
                        anchors.rightMargin: card.contentRight
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.spacingXXS

                        // Title
                        Text {
                            visible: card.modelData.title !== undefined && card.modelData.title !== ""
                            width: parent.width
                            text: card.modelData.title !== undefined ? card.modelData.title : ""
                            color: card.sevColor
                            font.pixelSize: Theme.fontSizeSM
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        // Message: collapses to 2 lines (upstream renders
                        // min(endlines, 2), NotificationManager.cpp:690) with
                        // a "More" hypertext that expands (cpp:722-725 +
                        // render_hypertext cpp:743-749); the expanded text
                        // longer than 3 lines gets the minimize glyph
                        // (cpp:337-338).
                        Item {
                            id: messageArea
                            width: parent.width
                            height: msgText.height

                            // Hidden measurer: natural wrapped line count of
                            // the full message at full width. Drives the
                            // "More" link without feeding truncated state
                            // back into msgText's width (no binding loop).
                            Text {
                                id: msgMeasure
                                visible: false
                                width: messageArea.width
                                text: msgText.text
                                font: msgText.font
                                wrapMode: Text.Wrap
                            }

                            Text {
                                id: msgText
                                anchors.left: parent.left
                                anchors.top: parent.top
                                // Reserve the "More" link's inline space on the
                                // truncated second line (upstream trims the
                                // line to fit "..More", cpp:699-702).
                                width: moreLink.visible ? messageArea.width - moreLink.width - 4
                                                        : messageArea.width
                                text: card.modelData.message !== undefined ? card.modelData.message : ""
                                color: Theme.chromeTextMuted
                                font.pixelSize: Theme.fontSizeMD
                                elide: Text.ElideRight
                                wrapMode: Text.Wrap
                                maximumLineCount: card.expanded ? 9999 : 2
                            }

                            // "More" hypertext after the truncated second line
                            // (upstream render_hypertext with more=true,
                            // NotificationManager.cpp:722-725, 743-749). X2:
                            // the accent bold expander is the unified
                            // ErrorToast link form (was an underlined
                            // Text variant).
                            Text {
                                id: moreLink
                                visible: !card.expanded && msgMeasure.lineCount > 2
                                anchors.left: msgText.right
                                anchors.leftMargin: 4
                                anchors.bottom: parent.bottom
                                text: qsTr("更多")
                                color: moreMA.containsMouse ? Theme.accentDark : Theme.accent
                                font.pixelSize: Theme.fontSizeMD
                                font.bold: true
                                MouseArea {
                                    id: moreMA
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: card.expanded = true
                                }
                            }
                        }
                    }

                    // Minimize glyph (render_minimize_button,
                    // NotificationManager.cpp:988-1017: shown when multiline
                    // && lines > 3, left edge at width - 1.8 x line_height,
                    // y = height - button - 5, click collapses). X2: the
                    // shared notification_minimalize svg glyph (the ErrorToast
                    // contract) replaces the page-private ▴ Text variant.
                    Image {
                        visible: card.expanded && msgMeasure.lineCount > 3
                        x: parent.width - 1.8 * root.notifLineHeight
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: Theme.insetSM
                        width: 22; height: 22
                        source: minMA.containsMouse
                                ? "qrc:/qml/assets/icons/notification_minimalize_hover.svg"
                                : "qrc:/qml/assets/icons/notification_minimalize.svg"
                        MouseArea {
                            id: minMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: card.expanded = false
                        }
                    }

                    // Invisible full-height click strip widening the close
                    // hit area (render_close_button,
                    // NotificationManager.cpp:867-872: x = width -
                    // 2.35 x line_height, width 2.125 x line_height, full
                    // height). Normal rows only -- upstream block rows have
                    // no strip.
                    MouseArea {
                        visible: !card.blockLevel
                        x: parent.width - 2.35 * root.notifLineHeight
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 2.125 * root.notifLineHeight
                        cursorShape: Qt.PointingHandCursor
                        onClicked: Backend.removeHistoryById(card.modelData.id)
                    }

                    // Per-row close glyph (render_close_button,
                    // NotificationManager.cpp:853-860: 1.25 x line_height
                    // glyph at width - 2.75 x line_height, vertically
                    // centered; hover swaps the glyph). X2: the shared
                    // notification_close svg glyph (the ErrorToast contract)
                    // replaces the row-private ✕ Text variant.
                    Image {
                        visible: !card.blockLevel
                        x: parent.width - 2.75 * root.notifLineHeight
                        anchors.verticalCenter: parent.verticalCenter
                        width: 22; height: 22
                        source: closeMA.containsMouse
                                ? "qrc:/qml/assets/icons/notification_close_hover.svg"
                                : "qrc:/qml/assets/icons/notification_close.svg"
                        MouseArea {
                            id: closeMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Backend.removeHistoryById(card.modelData.id)
                        }
                    }

                    // Block-row close glyph, larger (bbl_render_block_notif_
                    // buttons, NotificationManager.cpp:877-903: 2 x line_height
                    // glyph, left edge at width - 2.0 x line_height, vertically
                    // centered). Same shared block_notification_close asset
                    // family the ErrorToast block form uses.
                    Image {
                        visible: card.blockLevel
                        x: parent.width - 2.0 * root.notifLineHeight
                        anchors.verticalCenter: parent.verticalCenter
                        width: 36; height: 36
                        source: blockCloseMA.containsMouse
                                ? "qrc:/qml/assets/icons/block_notification_close_hover.svg"
                                : "qrc:/qml/assets/icons/block_notification_close.svg"
                        MouseArea {
                            id: blockCloseMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Backend.removeHistoryById(card.modelData.id)
                        }
                    }
                }

                // Scrollbar
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    width: 4
                    contentItem: Rectangle {
                        radius: Theme.radiusXS
                        color: Theme.borderDefault
                    }
                }
            }

            // Empty state
            Text {
                visible: Backend.historyCount === 0
                Layout.fillWidth: true
                Layout.fillHeight: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: qsTr("暂无通知记录")
                color: Theme.borderStrong
                font.pixelSize: Theme.fontSize13
            }
        }
    }
}
