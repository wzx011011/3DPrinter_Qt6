import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Notification center panel — shows notification history list
// Opens from bell icon in topbar, aligns with upstream notification_manager scrollable area
Item {
    id: root

    signal closeRequested()

    // Phase 167 (Cmp-01): collapsed the private 9-level severity→color and
    // severity→icon tables into lookups against the canonical Theme.severityColors
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

    Rectangle {
        anchors.fill: parent
        radius: 12
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
                    visible: backend.unreadHistoryCount > 0
                    width: unreadBadge.implicitWidth + 12
                    height: 20
                    radius: 10
                    color: Theme.statusError

                    Text {
                        id: unreadBadge
                        anchors.centerIn: parent
                        text: backend.unreadHistoryCount > 99 ? "99+" : backend.unreadHistoryCount.toString()
                        color: Theme.accentDark
                        font.pixelSize: Theme.fontSizeXS
                        font.bold: true
                    }
                }

                // Mark read button
                Rectangle {
                    visible: backend.unreadHistoryCount > 0
                    width: 24; height: 24; radius: 4
                    color: "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "✓"
                        color: Theme.statusInfo
                        font.pixelSize: Theme.fontSize13
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: backend.markHistoryRead()
                    }
                }

                // Clear all button
                Rectangle {
                    visible: backend.historyCount > 0
                    width: 24; height: 24; radius: 4
                    color: "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "🗑"
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSizeMD
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: backend.clearHistory()
                    }
                }

                // Close button
                Rectangle {
                    width: 24; height: 24; radius: 4
                    color: "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSize13
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: root.closeRequested()
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.bgCard }

            // Notification list
            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: backend.historyCount
                spacing: 4

                delegate: Rectangle {
                    width: ListView.view.width
                    height: delegateColumn.implicitHeight + 12
                    radius: 8
                    color: Theme.bgPanel
                    border.color: Theme.scrollBarTrackColor
                    border.width: 1

                    RowLayout {
                        id: delegateColumn
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.margins: 8
                        spacing: 10

                        // Severity icon
                        Text {
                            text: root.severityIcon(backend.historySeverity(index))
                            color: root.severityColor(backend.historySeverity(index))
                            font.pixelSize: Theme.fontSizeLG
                            font.bold: true
                        }

                        Column {
                            Layout.fillWidth: true
                            spacing: 2

                            // Title
                            Text {
                                visible: backend.historyTitle(index) !== ""
                                text: backend.historyTitle(index)
                                color: root.severityColor(backend.historySeverity(index))
                                font.pixelSize: Theme.fontSizeSM
                                font.bold: true
                                elide: Text.ElideRight
                                width: parent.width
                            }

                            // Message
                            Text {
                                text: backend.historyMessage(index)
                                color: Theme.chromeTextMuted
                                font.pixelSize: Theme.fontSizeMD
                                elide: Text.ElideRight
                                width: parent.width
                                wrapMode: Text.Wrap
                                maximumLineCount: 3
                            }

                            // Time
                            Text {
                                text: backend.historyTime(index)
                                color: Theme.borderStrong
                                font.pixelSize: Theme.fontSizeXS
                                font.family: "Consolas, monospace"
                            }
                        }
                    }
                }

                // Scrollbar
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    width: 4
                    contentItem: Rectangle {
                        radius: 2
                        color: Theme.borderDefault
                    }
                }
            }

            // Empty state
            Text {
                visible: backend.historyCount === 0
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
