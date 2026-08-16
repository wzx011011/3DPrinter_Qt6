import QtQuick
import QtQuick.Layouts
import ".."

// Stacked notification toasts (Phase 240 NOTI-01, upstream NotificationManager
// render_notifications -- every live notification is visible simultaneously,
// importance ordered: errors at the top, progress at the bottom).
// Severity levels (aligns with upstream NotificationLevel):
//   0=info(green), 1=success(green), 2=warning(amber), 3=error(red),
//   4=seriousWarning(dark red), 5=hint(blue), 6/7=printInfo(purple), 8/9=progress(blue)
// Persistent mode: doesn't auto-dismiss, shows progress bar and/or confirm buttons
// Specialized types: hint navigation, slicing progress, export/preview buttons
// Duplicate compression: repeatCount >= 2 renders an "xN" escalation badge
// (upstream UpdatedItemsInfoNotification counter).
Item {
    id: root

    // The visible stack, most important first (index 0 renders at the top,
    // matching upstream ErrorNotificationLevel at the "Top most position").
    readonly property var stack: backend.notificationStack
    readonly property bool shouldShow: stack.length > 0
    visible: shouldShow
    anchors.bottom: parent.bottom
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottomMargin: 44
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
        readonly property bool showExportBtn: modelData.showExportButton === true
        readonly property bool showPreviewBtn: modelData.showPreviewButton === true

        // Dynamic width based on content
        readonly property bool hasExtraButtons: entry.isHint || entry.isSlicingComplete
        readonly property bool hasDocLink: entry.isHint && backend.currentHintHasDocumentationLink
        width: Math.max(toastLabel.implicitWidth + (entry.repeatCount > 1 ? 96 : 76),
                        entry.hasProgress ? 320 : (entry.hasExtraButtons ? (entry.hasDocLink ? 410 : 360) : 160))
        height: {
            var h = 40
            if (entry.hasProgress) h = 60
            if (entry.hasExtraButtons) h += 30
            return h
        }

        // Severity color mapping (expanded with slicing progress)
        readonly property color iconColor: sev === 3 ? Theme.statusError : sev === 4 ? Theme.statusError : sev === 2 ? Theme.statusError : sev === 1 ? Theme.accent : sev === 5 ? Theme.statusInfo : sev === 6 || sev === 7 ? Theme.textTertiary : sev === 8 || sev === 9 ? Theme.statusInfo : Theme.accent
        readonly property color bgColor: sev === 3 ? Theme.bgPanel : sev === 4 ? Theme.bgPanel : sev === 2 ? Theme.bgWarningSubtle : sev === 5 ? Theme.bgFloating : sev === 6 || sev === 7 ? Theme.bgFloating : sev === 8 || sev === 9 ? Theme.bgTooltip : Theme.bgFloating
        readonly property color textColor: Theme.chromeText
        readonly property string iconText: sev === 3 ? "✕" : sev === 4 ? "⚠" : sev === 2 ? "⚠" : sev === 1 ? "✓" : sev === 5 ? "?" : sev === 8 || sev === 9 ? "⟳" : "i"
        // Auto-dismiss uses user preference (in seconds)
        readonly property int autoDismissMs: backend.autoDismissSec * 1000

        opacity: 0
        Component.onCompleted: {
            slideAnim.restart()
            if (!entry.isPersistent)
                hideTimer.restart()
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

        NumberAnimation on opacity { id: slideAnim; to: 1; from: 0; duration: 220; easing.type: Easing.OutCubic }

        Rectangle {
            anchors.fill: parent
            radius: 20
            color: bgColor
            border.color: iconColor
            border.width: 1

            // Hover pause: pause auto-dismiss while mouse is inside (aligns
            // with upstream hover behavior pausing all countdowns)
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: { if (!entry.isPersistent && hideTimer.running) hideTimer.stop() }
                onExited: { if (!entry.isPersistent) { hideTimer.interval = 2000; hideTimer.restart() } }
            }

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 2
                width: parent.width - 16

                // Optional title
                Text {
                    visible: modelData.title !== undefined && modelData.title !== ""
                    text: modelData.title !== undefined ? modelData.title : ""
                    color: iconColor
                    font.pixelSize: Theme.fontSizeXS
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                }

                RowLayout {
                    width: parent.width
                    spacing: 8

                    Text { text: iconText; color: iconColor; font.pixelSize: Theme.fontSize13; font.bold: true }
                    Text {
                        id: toastLabel
                        text: modelData.message !== undefined ? modelData.message : ""
                        color: textColor
                        font.pixelSize: Theme.fontSizeMD
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    // Duplicate compression / escalation counter (upstream
                    // UpdatedItemsInfoNotification xN badge)
                    Rectangle {
                        visible: entry.repeatCount > 1
                        width: 24
                        height: 18
                        radius: 9
                        color: iconColor

                        Text {
                            anchors.centerIn: parent
                            text: "x" + entry.repeatCount
                            color: Theme.accentDark
                            font.pixelSize: Theme.fontSizeXS
                            font.bold: true
                        }
                    }

                    // Close button for persistent notifications
                    Rectangle {
                        visible: entry.isPersistent
                        width: 18; height: 18; radius: 9
                        color: closeMA.containsMouse ? Theme.bgPressed : "transparent"
                        Text { anchors.centerIn: parent; text: "✕"; color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                        MouseArea {
                            id: closeMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.dismissNotificationById(entry.entryId)
                        }
                    }
                }

                // Progress bar (aligns with upstream notification_manager progress notification)
                Rectangle {
                    visible: entry.hasProgress
                    Layout.fillWidth: true
                    height: 6
                    radius: 3
                    color: Theme.chromePressed

                    Rectangle {
                        width: parent.width * (entry.progressValue / 100.0)
                        height: parent.height
                        radius: 3
                        color: iconColor
                        Behavior on width { NumberAnimation { duration: 200 } }
                    }

                    Text {
                        anchors.left: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 6
                        text: entry.progressValue + "%"
                        color: textColor
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                    }
                }

                // Hint navigation buttons (aligns with upstream HintNotification next/prev arrows)
                RowLayout {
                    visible: entry.isHint
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 4
                    spacing: 6

                    // Prev hint
                    Rectangle {
                        width: 24; height: 22; radius: 4
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
                        text: backend.currentHintIndex >= 0
                              ? (backend.currentHintIndex + 1) + "/" + backend.hintCount
                              : ""
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                    }

                    // Next hint
                    Rectangle {
                        width: 24; height: 22; radius: 4
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
                        visible: backend.currentHintHasDocumentationLink
                        width: 40; height: 22; radius: 4
                        color: docMA.containsMouse ? Theme.bgWarningSubtle : Theme.bgCard
                        Text { anchors.centerIn: parent; text: qsTr("文档"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS; font.bold: true }
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
                        Layout.leftMargin: 8
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
                RowLayout {
                    visible: entry.isSlicingComplete
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 4
                    spacing: 8

                    // Preview button
                    Rectangle {
                        visible: entry.showPreviewBtn
                        width: 70; height: 24; radius: 4
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
                        width: 70; height: 24; radius: 4
                        color: exportMA.containsMouse ? Theme.statusInfo : "#1d4ed8"
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
                        width: 50; height: 24; radius: 4
                        color: Theme.chromePressed
                        border.color: Theme.borderDefault; border.width: 1
                        Text { anchors.centerIn: parent; text: qsTr("关闭"); color: Theme.chromeText; font.pixelSize: Theme.fontSizeXS }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: backend.dismissNotificationById(entry.entryId) }
                    }
                }

                // Confirm/Cancel buttons (aligns with upstream notification_manager confirm dialog)
                RowLayout {
                    visible: entry.isPersistent && !entry.hasProgress && !entry.isHint && !entry.isSlicingComplete
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 4
                    spacing: 8

                    Rectangle {
                        width: 60; height: 24; radius: 4
                        color: Theme.chromePressed
                        border.color: Theme.borderDefault; border.width: 1
                        Text { anchors.centerIn: parent; text: qsTr("取消"); color: Theme.chromeText; font.pixelSize: Theme.fontSizeSM }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: backend.cancelCurrentNotification() }
                    }
                    Rectangle {
                        width: 60; height: 24; radius: 4
                        color: iconColor
                        Text { anchors.centerIn: parent; text: qsTr("确认"); color: Theme.accentDark; font.pixelSize: Theme.fontSizeSM }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: backend.confirmCurrentNotification() }
                    }
                }
            }
        }
    }

    Column {
        id: toastColumn
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 8

        Repeater {
            model: root.stack
            delegate: ToastEntry {}
        }
    }
}
