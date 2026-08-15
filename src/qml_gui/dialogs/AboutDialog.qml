import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// D4 -- AboutDialog: version + Qt version + license + confirm close
// Usage: AboutDialog { id: aboutDlg }  ->  aboutDlg.open()
// PreferencesPage "About" category triggers
//
// Phase 236 (DLG-04): license corrected to AGPL-3.0. Upstream AboutDialog.cpp
// :148-156 states: "Orca Slicer is licensed under GNU Affero General Public
// License, version 3" + "Orca Slicer is based on PrusaSlicer and BambuStudio"
// + an open-source components list (m_entries). OWzx mirrors the same
// structure — the previous Lesser-GPL license text was factually wrong for
// an OrcaSlicer-derived work.
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
        spacing: Theme.spacingXL
        // Logo + product name
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.spacingSM
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
                anchors.fill: parent; anchors.margins: 10; spacing: Theme.spacingSM
                component InfoRow: RowLayout {
                    required property string label
                    required property string value
                    Layout.fillWidth: true; spacing: Theme.spacingXS
                    Text { text: parent.label; color: Theme.textDisabled; font.pixelSize: Theme.fontSizeSM; Layout.preferredWidth: 120 }
                    Text { text: parent.value; color: Theme.chromeText; font.pixelSize: Theme.fontSizeSM }
                }

                InfoRow { label: qsTr("Qt 版本");     value: "6.10.0" }
                InfoRow { label: qsTr("QML 引擎");   value: "V4 / JavaScript" }
                InfoRow { label: qsTr("构建类型");   value: "Debug" }
                InfoRow { label: qsTr("目标平台");   value: "Windows x64 (MSVC)" }
                InfoRow { label: qsTr("构建日期");   value: "2026-03-03" }
                InfoRow { label: qsTr("开源协议");   value: qsTr("GNU Affero General Public License v3 (AGPL-3.0)") }
                InfoRow { label: qsTr("官方网站");   value: "www.orcaslicer.org" }
            }
        }

        // License note — Phase 236 (DLG-04): AGPL-3.0 + attribution, aligned
        // with upstream AboutDialog.cpp:148-156.
        Rectangle {
            Layout.fillWidth: true; radius: 5; color: Theme.bgInset; height: licText.implicitHeight + 16
            Text {
                id: licText
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top; anchors.margins: Theme.spacingMD
                text: qsTr("OWzx Slicer 基于 OrcaSlicer，遵循 GNU Affero 通用公共许可证第 3 版（AGPL-3.0）发布。OrcaSlicer 基于 PrusaSlicer 与 BambuStudio。使用本软件即代表您同意相关使用条款。")
                color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS
                wrapMode: Text.WordWrap
                lineHeight: 1.5
            }
        }

        // Open-source components (upstream AboutDialog m_entries list)
        Rectangle {
            Layout.fillWidth: true; radius: 5; color: Theme.bgInset; height: libsCol.implicitHeight + 16

            ColumnLayout {
                id: libsCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top; anchors.margins: Theme.spacingMD
                spacing: Theme.spacingXS

                Text {
                    text: qsTr("开源组件")
                    color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM; font.bold: true
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("本软件使用的开源组件的版权及其他专有权利归其各自所有者：Qt (GPL/LGPL)、Boost、CGAL、TBB、OpenCV、libigl、Eigen、clipper/clipper2、assimp、cereal、nlohmann/json、miniz、nanosvg、admesh、qhull、libnest2d。")
                    color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS
                    wrapMode: Text.WordWrap
                    lineHeight: 1.4
                }
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
