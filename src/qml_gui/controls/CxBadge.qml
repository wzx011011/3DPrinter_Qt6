import QtQuick
import QtQuick.Layouts
import ".."

// CxBadge.qml - compact label badge for option rows, notification counts, and
// status chips. Extracted from the inline Badge in OptionRow.qml (Phase 194,
// UI-01) so every call site shares one theme-aware rendering.
//
// Presentation only: callers pass label + color/fill tokens; no business logic.
Rectangle {
    id: root

    property string label: ""
    property color colorToken: Theme.textTertiary
    property color fillToken: Theme.bgInset

    implicitWidth: Math.max(18, badgeText.implicitWidth + 8)
    implicitHeight: 16
    Layout.preferredWidth: Math.max(18, badgeText.implicitWidth + 8)
    Layout.preferredHeight: 16
    radius: 3
    color: root.fillToken
    border.width: 1
    border.color: Theme.borderSubtle

    Text {
        id: badgeText
        anchors.centerIn: parent
        text: root.label
        color: root.colorToken
        font.pixelSize: Theme.fontSizeXS
        font.bold: true
    }
}
