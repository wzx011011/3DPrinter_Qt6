import QtQuick
import QtQuick.Layouts
import ".."

Item {
    id: root

    property string title: ""
    property string subtitle: ""
    default property alias actions: trailingRow.data

    implicitWidth: headerRow.implicitWidth
    implicitHeight: headerRow.implicitHeight

    RowLayout {
        id: headerRow
        anchors.fill: parent
        spacing: Theme.spacingMD

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Text {
                text: root.title
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
            }

            Text {
                visible: root.subtitle.length > 0
                text: root.subtitle
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
            }
        }

        RowLayout {
            id: trailingRow
            spacing: Theme.spacingSM
        }
    }
}