import QtQuick
import ".."

// CxEmptyState.qml — shared empty-state block (Phase 172, P2).
//
// Replaces the per-page glyph+title Text columns (📋/⚙/…) with one themed
// component so every empty surface reads the same: display-tier glyph, one
// secondary-tone title, optional tertiary message, optional action button.
// Presentation only; the owner keeps controlling `visible`.
Item {
    id: root

    property string glyph: ""
    property string title: ""
    property string message: ""
    property string actionText: ""
    signal actionTriggered()

    implicitWidth: Math.max(contentColumn.implicitWidth, 220)
    implicitHeight: contentColumn.implicitHeight

    Column {
        id: contentColumn
        anchors.centerIn: parent
        spacing: Theme.spacingMD

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.glyph
            font.pixelSize: Theme.fontSizeDisplayXL
            color: Theme.textDisabled
            horizontalAlignment: Text.AlignHCenter
            visible: root.glyph.length > 0
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.title
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeLG
            font.bold: true
            horizontalAlignment: Text.AlignHCenter
            visible: root.title.length > 0
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.message
            color: Theme.textDisabled
            font.pixelSize: Theme.fontSizeSM
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            width: Math.min(implicitWidth, 300)
            visible: root.message.length > 0
        }
        CxButton {
            id: actionButton
            anchors.horizontalCenter: parent.horizontalCenter
            cxStyle: CxButton.Style.Secondary
            text: root.actionText
            visible: root.actionText.length > 0
            onClicked: root.actionTriggered()
        }
    }
}
