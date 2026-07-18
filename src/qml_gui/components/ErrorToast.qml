import QtQuick
import QtQuick.Layouts

// Notification toast — slides up from bottom, supports queue, progress bar, confirm dialog
// Severity levels (aligns with upstream NotificationLevel):
//   0=info(green), 1=success(green), 2=warning(amber), 3=error(red),
//   4=seriousWarning(dark red), 5=hint(blue), 6/7=printInfo(purple), 8/9=progress(blue)
// Persistent mode: doesn't auto-dismiss, shows progress bar and/or confirm buttons
// Specialized types: hint navigation, slicing progress, export/preview buttons
Item {
    id: root

    readonly property int sev: backend.lastErrorSeverity
    readonly property bool shouldShow: sev >= 0 && backend.lastErrorMessage !== ""
    readonly property int pending: backend.pendingNotificationCount
    readonly property bool hasProgress: backend.currentNotificationHasProgress
    readonly property bool isPersistent: backend.currentNotificationPersistent
    readonly property int notiType: backend.currentNotificationType
    readonly property bool isHint: notiType === 10 // NotiTypeDidYouKnowHint
    readonly property bool isSlicingComplete: notiType === 2 && !root.hasProgress // NotiTypeSlicingProgress without progress = complete
    readonly property bool showExportBtn: backend.currentNotificationShowExport
    readonly property bool showPreviewBtn: backend.currentNotificationShowPreview

    // Dynamic width based on content
    readonly property bool hasExtraButtons: root.isHint || root.isSlicingComplete
    readonly property bool hasDocLink: root.isHint && backend.currentHintHasDocumentationLink
    visible: shouldShow || slideAnim.running
    anchors.bottom: parent.bottom
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottomMargin: 44
    width: Math.max(toastLabel.implicitWidth + 76, hasProgress ? 320 : (root.hasExtraButtons ? (hasDocLink ? 410 : 360) : 160))
    height: {
        var h = 40
        if (root.hasProgress) h = 60
        if (root.hasExtraButtons) h += 30
        return h
    }
    z: 200

    // Severity color mapping (expanded with slicing progress)
    readonly property color iconColor: sev === 3 ? Theme.statusError : sev === 4 ? Theme.statusError : sev === 2 ? Theme.statusError : sev === 1 ? Theme.accent : sev === 5 ? Theme.statusInfo : sev === 6 || sev === 7 ? Theme.textTertiary : sev === 8 || sev === 9 ? Theme.statusInfo : Theme.accent
    readonly property color bgColor: sev === 3 ? Theme.bgPanel : sev === 4 ? Theme.bgPanel : sev === 2 ? Theme.bgWarningSubtle : sev === 5 ? Theme.bgFloating : sev === 6 || sev === 7 ? Theme.bgFloating : sev === 8 || sev === 9 ? Theme.bgTooltip : Theme.bgFloating
    readonly property color textColor: sev === 3 ? Theme.chromeText : sev === 4 ? Theme.chromeText : sev === 2 ? Theme.chromeText : sev === 5 ? Theme.chromeText : sev === 6 || sev === 7 ? Theme.chromeText : sev === 8 || sev === 9 ? Theme.chromeText : Theme.chromeText
    readonly property string iconText: sev === 3 ? "✕" : sev === 4 ? "⚠" : sev === 2 ? "⚠" : sev === 1 ? "✓" : sev === 5 ? "?" : sev === 8 || sev === 9 ? "⟳" : "i"
    // Auto-dismiss uses user preference (in seconds)
    readonly property int autoDismissMs: backend.autoDismissSec * 1000

    onShouldShowChanged: {
        if (shouldShow) {
            root.opacity = 0
            root.y = 20
            slideAnim.restart()
            if (!root.isPersistent) {
                hideTimer.interval = root.autoDismissMs
                hideTimer.restart()
            } else {
                hideTimer.stop()
            }
        }
    }

    Timer {
        id: hideTimer
        onTriggered: backend.dismissNotification()
    }

    NumberAnimation on opacity { id: slideAnim; to: 1; from: 0; duration: 220; easing.type: Easing.OutCubic }

    Rectangle {
        anchors.fill: parent
        radius: 20
        color: bgColor
        border.color: iconColor
        border.width: 1

        // Hover pause: pause auto-dismiss while mouse is inside (aligns with upstream hover behavior)
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onEntered: { if (!root.isPersistent && hideTimer.running) hideTimer.stop() }
            onExited: { if (!root.isPersistent && root.shouldShow) { hideTimer.interval = 2000; hideTimer.restart() } }
        }

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 2
            width: parent.width - 16

            // Optional title
            Text {
                visible: backend.lastErrorTitle !== ""
                text: backend.lastErrorTitle
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
                    text: backend.lastErrorMessage
                    color: textColor
                    font.pixelSize: Theme.fontSizeMD
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                // Pending notification count badge
                Rectangle {
                    visible: root.pending > 0
                    width: 18
                    height: 18
                    radius: 9
                    color: iconColor

                    Text {
                        anchors.centerIn: parent
                        text: root.pending > 9 ? "9+" : root.pending.toString()
                        color: Theme.accentDark
                        font.pixelSize: Theme.fontSizeXS
                        font.bold: true
                    }
                }

                // Close button for persistent notifications
                Rectangle {
                    visible: root.isPersistent
                    width: 18; height: 18; radius: 9
                    color: closeMA.containsMouse ? Theme.bgPressed : "transparent"
                    Text { anchors.centerIn: parent; text: "✕"; color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    MouseArea {
                        id: closeMA
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: backend.dismissNotification()
                    }
                }
            }

            // Progress bar (aligns with upstream notification_manager progress notification)
            Rectangle {
                visible: root.hasProgress
                Layout.fillWidth: true
                height: 6
                radius: 3
                color: Theme.chromePressed

                Rectangle {
                    width: parent.width * (backend.currentNotificationProgress / 100.0)
                    height: parent.height
                    radius: 3
                    color: iconColor
                    Behavior on width { NumberAnimation { duration: 200 } }
                }

                Text {
                    anchors.left: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 6
                    text: backend.currentNotificationProgress + "%"
                    color: textColor
                    font.pixelSize: Theme.fontSizeXS
                    font.family: "Consolas, monospace"
                }
            }

            // Hint navigation buttons (aligns with upstream HintNotification next/prev arrows)
            RowLayout {
                visible: root.isHint
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
                            backend.dismissNotification()
                            backend.setHintsEnabled(false)
                        }
                    }
                }
            }

            // Slicing completion buttons (aligns with upstream SlicingProgressNotification export/preview)
            RowLayout {
                visible: root.isSlicingComplete
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 4
                spacing: 8

                // Preview button
                Rectangle {
                    visible: root.showPreviewBtn
                    width: 70; height: 24; radius: 4
                    color: previewMA.containsMouse ? Theme.scrollBarHoverColor : Theme.scrollBarHoverColor
                    Text { anchors.centerIn: parent; text: qsTr("预览"); color: Theme.accentDark; font.pixelSize: Theme.fontSizeSM; font.bold: true }
                    MouseArea {
                        id: previewMA
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            backend.dismissNotification()
                            backend.setCurrentPage(2)
                        }
                    }
                }

                // Export button
                Rectangle {
                    visible: root.showExportBtn
                    width: 70; height: 24; radius: 4
                    color: exportMA.containsMouse ? Theme.statusInfo : "#1d4ed8"
                    Text { anchors.centerIn: parent; text: qsTr("导出"); color: Theme.accentDark; font.pixelSize: Theme.fontSizeSM; font.bold: true }
                    MouseArea {
                        id: exportMA
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            backend.dismissNotification()
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
                        onClicked: backend.dismissNotification() }
                }
            }

            // Confirm/Cancel buttons (aligns with upstream notification_manager confirm dialog)
            RowLayout {
                visible: root.isPersistent && !root.hasProgress && !root.isHint && !root.isSlicingComplete
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
