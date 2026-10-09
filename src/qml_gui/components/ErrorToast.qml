import QtQuick
import ".."

// Stacked notification toasts (Phase 240 NOTI-01, upstream NotificationManager
// render_notifications -- every live notification is visible simultaneously,
// importance ordered: errors at the top, progress at the bottom).
//
// Geometry follows the upstream ImGui windows 1:1 at scale 1
// (NotificationManager.cpp):
//   - docked at the canvas bottom-right corner, right/bottom margin 10px
//     (render :297-299 win_pos = cnv - right_gap / - m_top_y, pivot 1.0;
//     GLCanvas3D.cpp:2227-2228 SLIDER_DEFAULT_RIGHT_MARGIN/BOTTOM_MARGIN = 10)
//   - window width fixed at 25x line height = 450px (count_spaces :498
//     m_window_width = m_line_height * 25; ImGuiWrapper.hpp:59 font 18)
//   - window height derived from the wrapped line count:
//     (multiline ? max(lines,2) : 2) * line_height + 1 * line_height
//     (set_next_window_size :612-618); default multiline collapses to 2 lines
//     behind a "More" link only when lines > 6 (init :603-605)
//   - window radius 4 (ensure_ui_inited :185), border radius/4 = 1 (:247)
//   - 8px full-height rounded color bar on the left edge, no icon glyph
//     (bbl_render_left_sign :956-970; render_left_sign :972-987 is commented
//     out upstream); text is indented by 1 line height (:483)
//   - close button on every notification (render :339 calls
//     render_close_button unconditionally), X at width - 2.75*line_height
//     plus a full-height invisible click strip 2.125*line_height wide
//     (render_close_button :830-875)
//   - long text wraps at window_width - window_width_offset (:501-592);
//     collapsed entries render 2 lines + clickable "More" (render_text
//     :723-724), expanded entries with > 3 lines get a minimize button
//     (render :336-338, render_minimize_button :988-1018)
//   - error (severity 3) / serious warning (severity 4) use the block
//     notification form bbl_render_block_notification :359-447: solid
//     m_ErrorColor / m_WarnColor fill, forced white text, BlockNotifErrorIcon
//     at line_height/3 (:943-954), dedicated 2x-glyph close button at
//     width - 2*line_height (:877-933); no left bar, no minimize
//   - entries stack bottom-up with GAP_WIDTH = 10 (:33, render_notifications
//     :3145/:3152)
//
// Severity levels (aligns with upstream NotificationLevel):
//   0=info(green), 1=success(green), 2=warning(amber), 3=error(red block),
//   4=seriousWarning(amber block), 5=hint(blue), 6/7=printInfo(purple),
//   8/9=progress(blue)
// Persistent mode: doesn't auto-dismiss, shows progress bar and/or confirm buttons
// Specialized types: hint navigation, slicing progress, export/preview buttons
// Duplicate compression: repeatCount >= 2 renders an "xN" escalation badge
// (upstream UpdatedItemsInfoNotification counter).
// Hypertext: entries may carry a trailing clickable link (upstream
// NotificationData.hypertext rendered by render_hypertext :734-753).
Item {
    id: root

    // The visible stack, most important first (index 0 renders at the top,
    // matching upstream ErrorNotificationLevel at the "Top most position").
    readonly property var stack: backend.notificationStack
    readonly property bool shouldShow: stack.length > 0
    visible: shouldShow
    // Docked bottom-right (upstream canvas bottom-right corner), 10px
    // right/bottom margins (SLIDER_DEFAULT_RIGHT_MARGIN / BOTTOM_MARGIN).
    anchors.bottom: parent.bottom
    anchors.right: parent.right
    anchors.rightMargin: 10
    anchors.bottomMargin: 10
    width: childrenRect.width
    height: childrenRect.height
    z: 200

    // One delegate per stacked notification. Each entry owns its own
    // auto-dismiss timer and dismisses ONLY itself via
    // backend.dismissNotificationById(id) (upstream PopNotification::close()).
    component ToastEntry: Item {
        id: entry

        required property var modelData
        required property int index

        readonly property int sev: modelData.severity !== undefined ? modelData.severity : 0
        readonly property int entryId: modelData.id !== undefined ? modelData.id : 0
        readonly property int notiType: modelData.type !== undefined ? modelData.type : 0
        readonly property bool isPersistent: modelData.persistent === true
        readonly property bool hasProgress: modelData.hasProgress === true
        readonly property int progressValue: modelData.progressValue !== undefined ? modelData.progressValue : 0
        readonly property int repeatCount: modelData.repeatCount !== undefined ? modelData.repeatCount : 1
        readonly property bool isHint: entry.notiType === 10 // NotiTypeDidYouKnowHint
        readonly property bool isSlicingComplete: entry.notiType === 2 && !entry.hasProgress
        readonly property bool requiresConfirm: modelData.requiresConfirm === true
        readonly property bool showExportBtn: modelData.showExportButton === true
        readonly property bool showPreviewBtn: modelData.showPreviewButton === true
        readonly property string hypertext: modelData.hypertext !== undefined ? modelData.hypertext : ""
        // Block notification form: upstream routes Error + SeriousWarning
        // levels to bbl_render_block_notification (render_notifications
        // :3142-3143).
        readonly property bool isBlock: entry.sev === 3 || entry.sev === 4

        // -- Upstream window geometry (scale 1) ----------------------------
        // Line height = ImGui font size (ImGuiWrapper.hpp:59 m_font_size 18).
        readonly property int lineHeight: 18
        // m_window_width = m_line_height * 25 (count_spaces :498).
        readonly property int windowWidth: entry.lineHeight * 25
        // m_left_indentation (:483 regular, :489-490 block = 32 + line height).
        readonly property int leftIndentation: entry.isBlock ? 32 + entry.lineHeight : entry.lineHeight
        // m_window_width_offset (:495-497 regular, :492-493 block = 90).
        readonly property int windowWidthOffset: entry.isBlock ? 90 : entry.lineHeight * 4
        readonly property int textAreaWidth: entry.windowWidth - entry.windowWidthOffset
        // Expanded (multiline) state; init() only collapses behind "More" when
        // the text wraps to more than 6 lines (:603-605).
        property bool multiline: true
        readonly property int fullLineCount: measureLabel.lineCount
        // set_next_window_size (:612-618): multiline uses max(lines, 2) rows.
        readonly property int lineRows: entry.multiline ? Math.max(entry.fullLineCount, 2) : 2
        // Minimize button: multiline AND more than 3 lines (render :336-338).
        // Block notifications never get one (commented out upstream :431-434).
        readonly property bool minimizeVisible: !entry.isBlock && entry.multiline && entry.fullLineCount > 3

        readonly property bool hasExtraButtons: entry.isHint || entry.isSlicingComplete || entry.requiresConfirm
        width: entry.windowWidth
        // Height = (multiline ? max(lines, 2) : 2) * line_height + 1 * line_height
        // (set_next_window_size :612-618). Progress bar / action button rows
        // are Qt6 feature rows on top of the text window (slice-6 sizing).
        height: (entry.lineRows + 1) * entry.lineHeight
                + (entry.hasProgress ? 36 : 0)
                + (entry.hasExtraButtons ? 30 : 0)

        // -- Colors --------------------------------------------------------
        // m_CurrentColor (use_bbl_theme :224-234): error -> m_ErrorColor,
        // warning -> m_WarnColor, otherwise m_NormalColor (info green, kept
        // as the OWzx brand accent). Surfaces stay on the current neutral
        // gray tokens; the block form fills with the semantic color itself.
        readonly property color currentColor: sev === 3 ? Theme.statusError
                                             : sev === 4 || sev === 2 ? Theme.statusWarning
                                             : sev === 5 || sev === 8 || sev === 9 ? Theme.statusInfo
                                             : sev === 6 || sev === 7 ? Theme.textTertiary
                                             : Theme.accent
        readonly property color bgColor: entry.isBlock ? entry.currentColor
                                        : sev === 2 ? Theme.bgWarningSubtle
                                        : sev === 8 || sev === 9 ? Theme.bgTooltip
                                        : Theme.bgFloating
        // Block notifications force white text (push Text {1,1,1,1} :420).
        readonly property color textColor: entry.isBlock ? Theme.textOnAccent : Theme.chromeText
        // Auto-dismiss uses user preference (in seconds)
        readonly property int autoDismissMs: backend.autoDismissSec * 1000

        Component.onCompleted: {
            slideAnim.restart()
            if (!entry.isPersistent)
                hideTimer.restart()
            // init() :603-605: default state collapses texts wrapping to more
            // than 6 lines behind the "More" link.
            if (entry.fullLineCount > 6)
                entry.multiline = false
        }

        // In-place updates (e.g. slicing progress -> slicing complete) flip
        // the persistent flag on a LIVE delegate: stop the auto-dismiss
        // timer so persistent entries stay until closed (upstream
        // SlicingProgressNotification has no fade-out).
        onIsPersistentChanged: {
            if (entry.isPersistent)
                hideTimer.stop()
            else
                hideTimer.restart()
        }

        Timer {
            id: hideTimer
            interval: entry.autoDismissMs
            onTriggered: backend.dismissNotificationById(entry.entryId)
        }

        NumberAnimation on opacity { id: slideAnim; to: 1; from: 0; duration: Theme.motionNormal; easing.type: Theme.easingStandard }

        // Offscreen measuring label: full wrapped line count at the upstream
        // text width (count_lines :501-592). The visible label truncates when
        // collapsed, so it cannot report the full count itself.
        Text {
            id: measureLabel
            visible: false
            width: entry.textAreaWidth
            text: modelData.message !== undefined ? modelData.message : ""
            font.pixelSize: Theme.fontSizeMD
            lineHeight: entry.lineHeight
            lineHeightMode: Text.FixedHeight
            wrapMode: Text.Wrap
        }

        // Window body: radius 4 (:185), 1px border = radius/4 (:247) in the
        // current color (light-mode bbl_theme push :237); the block form
        // borders with its own fill color (invisible, as upstream :408-418).
        Rectangle {
            id: body
            anchors.fill: parent
            radius: Theme.radiusSM
            color: entry.bgColor
            border.color: entry.isBlock ? entry.bgColor : entry.currentColor
            border.width: 1

            // Hover pause: pause auto-dismiss while mouse is inside (aligns
            // with upstream hover behavior pausing all countdowns)
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: { if (!entry.isPersistent && hideTimer.running) hideTimer.stop() }
                onExited: { if (!entry.isPersistent) { hideTimer.interval = 2000; hideTimer.restart() } }
            }

            // -- Left edge color bar (regular notifications only) ---------
            // bbl_render_left_sign :956-970: full-height rounded bar, width
            // 2 * m_WindowRadius = 8px, drawn at left edge + border size, in
            // m_CurrentColor; second square rect covers the right half of the
            // bar (upstream draws rounded + square AddRectFilled pairs).
            // No icon glyph: upstream render_left_sign is fully commented out.
            Rectangle {
                visible: !entry.isBlock
                x: 1
                y: 1
                width: 8
                height: parent.height - 2
                radius: Theme.radiusSM
                color: entry.currentColor
            }
            Rectangle {
                visible: !entry.isBlock
                x: 5
                y: 1
                width: 4
                height: parent.height - 2
                color: entry.currentColor
            }

            // -- Content column (text starts at m_left_indentation, wraps at
            // window_width - m_window_width_offset) -------------------------
            Column {
                x: entry.leftIndentation
                width: entry.textAreaWidth
                anchors.verticalCenter: parent.verticalCenter
                spacing: Theme.spacingXXS

                // Optional title
                Text {
                    visible: modelData.title !== undefined && modelData.title !== ""
                    text: modelData.title !== undefined ? modelData.title : ""
                    color: entry.isBlock ? entry.textColor : entry.currentColor
                    font.pixelSize: Theme.fontSizeXS
                    font.bold: true
                    elide: Text.ElideRight
                    width: parent.width
                    horizontalAlignment: Text.AlignHCenter
                }

                // Message area: lineRows * line_height tall (+ one reserved
                // row when a hypertext link rides along, since QML Text
                // cannot place a link inline after a wrapped block the way
                // render_hypertext does).
                Item {
                    id: messageArea
                    width: parent.width
                    height: entry.lineRows * entry.lineHeight
                            + (entry.hypertext !== "" ? entry.lineHeight : 0)

                    Text {
                        id: messageLabel
                        text: modelData.message !== undefined ? modelData.message : ""
                        color: entry.textColor
                        font.pixelSize: Theme.fontSizeMD
                        lineHeight: entry.lineHeight
                        lineHeightMode: Text.FixedHeight
                        wrapMode: Text.Wrap
                        // Collapsed: exactly 2 rendered lines, second trimmed
                        // with an ellipsis and the width reserved for "More"
                        // (upstream trims line 2 until "..More" fits, render_
                        // text :708-711). Expanded: everything shows.
                        maximumLineCount: entry.multiline ? 1000 : 2
                        elide: Text.ElideRight
                        width: entry.multiline
                               ? parent.width
                               : parent.width - moreLink.width - 4
                    }

                    // "More" expander: collapsed AND more than 2 lines
                    // (render_text :723-724, render_hypertext more=true).
                    // Click re-expands to multiline and the window re-sizes.
                    Text {
                        id: moreLink
                        visible: !entry.multiline && entry.fullLineCount > 2
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: entry.lineHeight
                        verticalAlignment: Text.AlignVCenter
                        text: qsTr("More")
                        color: Theme.accent
                        font.pixelSize: Theme.fontSizeMD
                        font.bold: true
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: entry.multiline = true
                        }
                    }

                    // Hypertext link (upstream NotificationData.hypertext,
                    // render_hypertext :734-753; mutually exclusive with
                    // "More" there). Click routes through the backend, which
                    // acknowledges + closes the entry (PopNotification::
                    // on_text_click -> close()).
                    Text {
                        visible: !moreLink.visible && entry.hypertext !== ""
                        anchors.left: parent.left
                        anchors.bottom: parent.bottom
                        height: entry.lineHeight
                        verticalAlignment: Text.AlignVCenter
                        text: entry.hypertext
                        color: Theme.accent
                        font.pixelSize: Theme.fontSizeMD
                        elide: Text.ElideRight
                        width: parent.width
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.activateNotificationHypertext(entry.entryId)
                        }
                    }

                    // Duplicate compression / escalation counter (upstream
                    // UpdatedItemsInfoNotification xN badge), pinned to the
                    // first text line's band.
                    Rectangle {
                        visible: entry.repeatCount > 1
                        anchors.top: parent.top
                        anchors.right: parent.right
                        width: 24
                        height: entry.lineHeight
                        radius: Theme.radiusLG
                        color: entry.currentColor

                        Text {
                            anchors.centerIn: parent
                            text: "x" + entry.repeatCount
                            color: Theme.accentDark
                            font.pixelSize: Theme.fontSizeXS
                            font.bold: true
                        }
                    }
                }

                // Progress bar (slice-6, upstream SlicingProgressNotification.cpp:
                // 366-390 render_bar -- 4px bar, light-gray #d9d9d9 track,
                // teal-fill semantics carried by the brand green, percentage
                // text with 2 decimals below the bar's left edge).
                Rectangle {
                    visible: entry.hasProgress
                    width: parent.width
                    height: 4
                    radius: 0
                    color: Theme.textSecondary

                    Rectangle {
                        width: parent.width * (entry.progressValue / 100.0)
                        height: parent.height
                        radius: 0
                        color: Theme.accent
                        Behavior on width { NumberAnimation { duration: Theme.motionNormal } }
                    }

                    Text {
                        anchors.top: parent.bottom
                        anchors.topMargin: 3
                        anchors.left: parent.left
                        text: entry.progressValue.toFixed(2) + "%"
                        color: entry.textColor
                        font.pixelSize: Theme.fontSizeXS
                        font.family: Theme.fontMono
                    }
                }

                // Hint navigation buttons (aligns with upstream HintNotification next/prev arrows)
                Row {
                    visible: entry.isHint
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 6

                    // Prev hint
                    Rectangle {
                        width: 24; height: 22; radius: Theme.radiusSM
                        color: prevMA.containsMouse ? Theme.borderInput : Theme.chromePressed
                        Text { anchors.centerIn: parent; text: "<"; color: Theme.chromeTextMuted; font.pixelSize: Theme.fontSize13; font.bold: true }
                        MouseArea {
                            id: prevMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.prevHint()
                        }
                    }

                    // Hint index
                    Text {
                        id: hintCounter
                        anchors.verticalCenter: parent.verticalCenter
                        // G-07: re-read the counter on hint-cursor moves -
                        // currentHintIndex()/hintCount() are Q_INVOKABLEs whose
                        // one-shot binding kept the creation-time counter.
                        property int refreshTick: 0
                        Connections {
                            target: backend
                            function onDailyTipChanged() { ++hintCounter.refreshTick }
                        }
                        text: {
                            const tick = hintCounter.refreshTick  // re-evaluate per move
                            return backend.currentHintIndex() >= 0
                                  ? (backend.currentHintIndex() + 1) + "/" + backend.hintCount() : ""
                        }
                        color: entry.isBlock ? entry.textColor : Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                    }

                    // Next hint
                    Rectangle {
                        width: 24; height: 22; radius: Theme.radiusSM
                        color: nextMA.containsMouse ? Theme.borderInput : Theme.chromePressed
                        Text { anchors.centerIn: parent; text: ">"; color: Theme.chromeTextMuted; font.pixelSize: Theme.fontSize13; font.bold: true }
                        MouseArea {
                            id: nextMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.nextHint()
                        }
                    }

                    // Documentation link button (aligns with upstream HintNotification documentation button)
                    Rectangle {
                        visible: backend.currentHintHasDocumentationLink()
                        width: 40; height: 22; radius: Theme.radiusSM
                        color: docMA.containsMouse ? Theme.bgWarningSubtle : Theme.bgCard
                        Text { anchors.centerIn: parent; text: qsTr("文档"); color: entry.isBlock ? entry.textColor : Theme.textMuted; font.pixelSize: Theme.fontSizeXS; font.bold: true }
                        MouseArea {
                            id: docMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.openHintDocumentation()
                        }
                    }

                    // Don't show again
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        leftPadding: 8
                        text: qsTr("不再提示")
                        color: prefMA.containsMouse ? Theme.chromeTextMuted : Theme.borderActive
                        font.pixelSize: Theme.fontSizeXS
                        MouseArea {
                            id: prefMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                backend.dismissNotificationById(entry.entryId)
                                backend.setHintsEnabled(false)
                            }
                        }
                    }
                }

                // Slicing completion buttons (aligns with upstream SlicingProgressNotification export/preview)
                Row {
                    visible: entry.isSlicingComplete
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 8

                    // Preview button
                    Rectangle {
                        visible: entry.showPreviewBtn
                        width: 70; height: 24; radius: Theme.radiusSM
                        color: previewMA.containsMouse ? Theme.scrollBarHoverColor : Theme.scrollBarHoverColor
                        Text { anchors.centerIn: parent; text: qsTr("预览"); color: Theme.accentDark; font.pixelSize: Theme.fontSizeSM; font.bold: true }
                        MouseArea {
                            id: previewMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                backend.dismissNotificationById(entry.entryId)
                                backend.setCurrentPage(2)
                            }
                        }
                    }

                    // Export button
                    Rectangle {
                        visible: entry.showExportBtn
                        width: 70; height: 24; radius: Theme.radiusSM
                        color: Theme.statusInfo
                        Text { anchors.centerIn: parent; text: qsTr("导出"); color: Theme.accentDark; font.pixelSize: Theme.fontSizeSM; font.bold: true }
                        MouseArea {
                            id: exportMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                backend.dismissNotificationById(entry.entryId)
                                backend.exportGCodeRequested()
                            }
                        }
                    }

                    // Dismiss
                    Rectangle {
                        width: 50; height: 24; radius: Theme.radiusSM
                        color: Theme.chromePressed
                        border.color: Theme.borderDefault; border.width: 1
                        Text { anchors.centerIn: parent; text: qsTr("关闭"); color: Theme.chromeText; font.pixelSize: Theme.fontSizeXS }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: backend.dismissNotificationById(entry.entryId) }
                    }
                }

                // Confirm/Cancel buttons (aligns with upstream notification_manager confirm dialog)
                Row {
                    visible: entry.requiresConfirm
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 8

                    Rectangle {
                        width: 60; height: 24; radius: Theme.radiusSM
                        color: Theme.chromePressed
                        border.color: Theme.borderDefault; border.width: 1
                        Text { anchors.centerIn: parent; text: qsTr("取消"); color: Theme.chromeText; font.pixelSize: Theme.fontSizeSM }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: backend.cancelNotificationById(entry.entryId) }
                    }
                    Rectangle {
                        width: 60; height: 24; radius: Theme.radiusSM
                        color: entry.currentColor
                        Text { anchors.centerIn: parent; text: qsTr("确认"); color: Theme.accentDark; font.pixelSize: Theme.fontSizeSM }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: backend.confirmNotificationById(entry.entryId) }
                    }
                }
            }

            // -- Minimize button (regular form, multiline > 3 lines) -------
            // render_minimize_button :988-1018: glyph button at
            // width - 1.8 * line_height, height - button - 5; click collapses
            // back to 2 lines + "More".
            Image {
                visible: entry.minimizeVisible
                source: minimizeMA.containsMouse
                        ? "qrc:/qml/assets/icons/notification_minimalize_hover.svg"
                        : "qrc:/qml/assets/icons/notification_minimalize.svg"
                width: 22
                height: 22
                x: entry.windowWidth - entry.lineHeight * 1.8
                y: entry.height - height - 5

                MouseArea {
                    id: minimizeMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: entry.multiline = false
                }
            }

            // -- Close button, every notification (render :339) ------------
            // Block form: dedicated 2x-glyph button at width - 2*line_height,
            // vertically centered (bbl_render_block_notif_buttons :877-933).
            Image {
                visible: entry.isBlock
                source: blockCloseMA.containsMouse
                        ? "qrc:/qml/assets/icons/block_notification_close_hover.svg"
                        : "qrc:/qml/assets/icons/block_notification_close.svg"
                width: 36
                height: 36
                x: entry.windowWidth - entry.lineHeight * 2
                y: entry.height / 2 - height / 2

                MouseArea {
                    id: blockCloseMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: backend.dismissNotificationById(entry.entryId)
                }
            }

            // Regular form: X glyph at width - 2.75 * line_height, vertically
            // centered (top row when the minimize button occupies it,
            // render_close_button :856-863), hover swaps the glyph.
            Image {
                visible: !entry.isBlock
                source: (closeMA.containsMouse || closeStrip.containsMouse)
                        ? "qrc:/qml/assets/icons/notification_close_hover.svg"
                        : "qrc:/qml/assets/icons/notification_close.svg"
                width: 22
                height: 22
                x: entry.windowWidth - entry.lineHeight * 2.75
                y: entry.minimizeVisible ? 0 : entry.height / 2 - height

                MouseArea {
                    id: closeMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: backend.dismissNotificationById(entry.entryId)
                }
            }

            // Invisible full-height click strip right of the X, 2.125 *
            // line_height wide (render_close_button :868-874); shrinks by
            // 2 line heights when the minimize button is visible.
            MouseArea {
                id: closeStrip
                visible: !entry.isBlock
                x: entry.windowWidth - entry.lineHeight * 2.35
                y: 0
                width: entry.lineHeight * 2.125
                height: entry.minimizeVisible ? entry.height - entry.lineHeight * 2 : entry.height
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: backend.dismissNotificationById(entry.entryId)
            }
        }
    }

    Column {
        id: toastColumn
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        // GAP_WIDTH = 10 between stacked notifications
        // (NotificationManager.cpp:33, :3145, :3152).
        spacing: Theme.spacingMD

        Repeater {
            model: root.stack
            delegate: ToastEntry {}
        }
    }
}
