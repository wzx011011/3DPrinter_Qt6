import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Notification center panel — shows notification history list
// Opens from bell icon in topbar, aligns with upstream notification_manager scrollable area
Item {
    id: root

    signal closeRequested()

    // Severity color mapping (expanded 9-level, aligns with upstream NotificationLevel)
    function severityColor(sev) {
        switch (sev) {
        case 3: return "#f05545";   // Error - red
        case 4: return "#e04848";   // SeriousWarning - dark red
        case 2: return "#c87840";   // Warning - amber
        case 1: return "#18c75e";   // Success - green
        case 5: return "#58a6ff";   // Hint - blue
        case 6: return "#7c6aef";   // PrintInfo - purple
        case 7: return "#7c6aef";   // PrintInfoShort - purple
        case 8: return "#3b9cf0";   // Progress - light blue
        default: return "#18c75e";  // Info - green
        }
    }

    function severityIcon(sev) {
        switch (sev) {
        case 3: return "✕";
        case 4: return "⚠";
        case 2: return "⚠";
        case 1: return "✓";
        case 5: return "?";
        case 8: return "⟳";
        default: return "i";
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: "#0f1217"
        border.color: "#242a33"
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
                    color: "#e0e6ed"
                    font.pixelSize: 14
                    font.bold: true
                    Layout.fillWidth: true
                }

                // Unread badge
                Rectangle {
                    visible: backend.unreadHistoryCount > 0
                    width: unreadBadge.implicitWidth + 12
                    height: 20
                    radius: 10
                    color: "#f05545"

                    Text {
                        id: unreadBadge
                        anchors.centerIn: parent
                        text: backend.unreadHistoryCount > 99 ? "99+" : backend.unreadHistoryCount.toString()
                        color: "#fff"
                        font.pixelSize: 10
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
                        color: "#58a6ff"
                        font.pixelSize: 13
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
                        color: "#808890"
                        font.pixelSize: 12
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
                        color: "#808890"
                        font.pixelSize: 13
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: root.closeRequested()
                    }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: "#242a33" }

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
                    color: "#151a22"
                    border.color: "#1e2430"
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
                            font.pixelSize: 14
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
                                font.pixelSize: 11
                                font.bold: true
                                elide: Text.ElideRight
                                width: parent.width
                            }

                            // Message
                            Text {
                                text: backend.historyMessage(index)
                                color: "#b0b8c4"
                                font.pixelSize: 12
                                elide: Text.ElideRight
                                width: parent.width
                                wrapMode: Text.Wrap
                                maximumLineCount: 3
                            }

                            // Time
                            Text {
                                text: backend.historyTime(index)
                                color: "#505860"
                                font.pixelSize: 9
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
                        color: "#363d4e"
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
                color: "#505860"
                font.pixelSize: 13
            }
        }
    }
}
