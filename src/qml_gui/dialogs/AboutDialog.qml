import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../controls"

// D4 -- AboutDialog: version + Qt version + license + confirm close
// Usage: AboutDialog { id: aboutDlg }  ->  aboutDlg.open()
// PreferencesPage "About" category triggers
CxDialog {
    id: root

    dialogTitle: qsTr("关于 OWzx")
    titleIcon: "❓"
    showCloseButton: true

    anchors.centerIn: parent
    width:  420
    height: contentCol.implicitHeight + 80

    contentItem: ColumnLayout {
        id: contentCol
        width: root.width - 32
        spacing: 16

        // Logo + product name
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 6

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 64; height: 64; radius: 12
                color: Theme.chromeSurface
                border.color: Theme.accent; border.width: 2
                Text { anchors.centerIn: parent; text: "🖨"; font.pixelSize: 30 }
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "OWzx Slicer"
                color: Theme.textPrimary; font.pixelSize: 18; font.bold: true
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("版本 2.4.0-dev  (Qt6 QML)")
                color: Theme.accent; font.pixelSize: Theme.fontSizeSM
            }
        }

        // Version info table
        Rectangle {
            Layout.fillWidth: true; radius: 6; color: Theme.bgSurface; border.color: Theme.bgCard; height: infoCols.implicitHeight + 16

            ColumnLayout {
                id: infoCols
                anchors.fill: parent; anchors.margins: 10; spacing: 6

                component InfoRow: RowLayout {
                    required property string label
                    required property string value
                    Layout.fillWidth: true; spacing: 0
                    Text { text: parent.label; color: Theme.textDisabled; font.pixelSize: Theme.fontSizeSM; Layout.preferredWidth: 120 }
                    Text { text: parent.value; color: Theme.chromeText; font.pixelSize: Theme.fontSizeSM }
                }

                InfoRow { label: qsTr("Qt 版本");     value: "6.10.0" }
                InfoRow { label: qsTr("QML 引擎");   value: "V4 / JavaScript" }
                InfoRow { label: qsTr("构建类型");   value: "Debug" }
                InfoRow { label: qsTr("目标平台");   value: "Windows x64 (MSVC)" }
                InfoRow { label: qsTr("构建日期");   value: "2026-03-03" }
                InfoRow { label: qsTr("开源协议");   value: "GNU LGPL v3" }
                InfoRow { label: qsTr("官方网站");   value: "www.orcaslicer.org" }
            }
        }

        // License note
        Rectangle {
            Layout.fillWidth: true; radius: 5; color: Theme.bgInset; height: licText.implicitHeight + 16
            Text {
                id: licText
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top; anchors.margins: 10
                text: qsTr("本软件基于 Qt 6 框架构建，遵循 GNU LGPL v3 协议。使用本软件即代表您同意相关使用条款。")
                color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.WordWrap
                lineHeight: 1.5
            }
        }

        // Divider
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.bgCard }

        // Confirm button
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 100; height: 30; radius: 4
            color: okHov.containsMouse ? Theme.accentDark : Theme.accentSubtle
            Text { anchors.centerIn: parent; text: qsTr("确认"); color: "white"; font.pixelSize: Theme.fontSizeMD; font.bold: true }
            MouseArea { id: okHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.close() }
        }
    }
}
