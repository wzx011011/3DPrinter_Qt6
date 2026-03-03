import QtQuick
import QtQuick.Controls
import QtQuick.Controls.impl

Button {
    id: root

    property int cxStyle: CxButton.Style.Primary
    property bool compact: false

    enum Style { Primary, Secondary, Danger, Ghost }

    implicitHeight: compact ? 24 : 30
    implicitWidth: Math.max(64, contentItem.implicitWidth + leftPadding + rightPadding)
    leftPadding: 12
    rightPadding: 12

    background: Rectangle {
        radius: 4
        color: {
            const d = !root.enabled
            const h = root.hovered
            const p = root.pressed
            if (root.cxStyle === CxButton.Style.Primary) {
                if (d) return "#264c38"
                if (p) return "#16a354"
                if (h) return "#1ed36b"
                return "#18c75e"
            }
            if (root.cxStyle === CxButton.Style.Danger) {
                if (d) return "#4a2828"
                if (p) return "#c93030"
                return "#e03535"
            }
            if (root.cxStyle === CxButton.Style.Ghost) {
                if (p) return "#3a4050"
                if (h) return "#2e3442"
                return "transparent"
            }
            // Secondary (default)
            if (d) return "#2a2f38"
            if (p) return "#4a5060"
            if (h) return "#424957"
            return "#363c4a"
        }
        border.color: {
            if (root.cxStyle === CxButton.Style.Primary) return "transparent"
            if (root.cxStyle === CxButton.Style.Danger) return "transparent"
            if (root.cxStyle === CxButton.Style.Ghost) return root.hovered ? "#566070" : "transparent"
            return "#4e5568"
        }
        border.width: 1
    }

    contentItem: Text {
        text: root.text
        color: {
            if (!root.enabled) return "#5a6270"
            if (root.cxStyle === CxButton.Style.Primary) return "#ffffff"
            if (root.cxStyle === CxButton.Style.Danger) return "#ffffff"
            return "#d8e0ec"
        }
        font.pixelSize: root.compact ? 11 : 12
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
