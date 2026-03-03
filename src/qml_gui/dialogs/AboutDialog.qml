import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// D4 — AboutDialog：版本号 + Qt 版本 + 协议 + 确认关闭
// 用法：AboutDialog { id: aboutDlg }  →  aboutDlg.open()
// PreferencesPage「关于」分类触发
Dialog {
    id: root

    modal: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    anchors.centerIn: parent
    width:  420
    height: contentCol.implicitHeight + 80

    background: Rectangle {
        color: "#1a1f28"; radius: 8
        border.color: "#2e3848"; border.width: 1
    }

    header: Rectangle {
        width: parent.width; height: 44; color: "#141920"; radius: 8
        Rectangle { anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right; height: 12; color: parent.color }

        RowLayout {
            anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 12; spacing: 10
            Text { text: qsTr("❓  关于 Creality Print"); color: "#e2e8f5"; font.pixelSize: 14; font.bold: true }
            Item { Layout.fillWidth: true }
            Rectangle {
                width: 24; height: 24; radius: 4
                color: closeAboutHov.containsMouse ? "#3d2020" : "transparent"
                Text { anchors.centerIn: parent; text: "✕"; color: "#7a8fa3"; font.pixelSize: 12 }
                MouseArea { id: closeAboutHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.close() }
            }
        }
    }

    contentItem: ColumnLayout {
        id: contentCol
        width: root.width - 32
        spacing: 16

        // Logo + 产品名
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 6

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 64; height: 64; radius: 12
                color: "#111e1a"
                border.color: "#22c564"; border.width: 2
                Text { anchors.centerIn: parent; text: "🖨"; font.pixelSize: 30 }
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: "Creality Print"
                color: "#e2e8f5"; font.pixelSize: 18; font.bold: true
            }

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("版本 7.0.0.0  (Qt6 QML 重写)")
                color: "#22c564"; font.pixelSize: 11
            }
        }

        // 版本信息表格
        Rectangle {
            Layout.fillWidth: true; radius: 6; color: "#111620"; border.color: "#1e2535"; height: infoCols.implicitHeight + 16

            ColumnLayout {
                id: infoCols
                anchors.fill: parent; anchors.margins: 10; spacing: 6

                component InfoRow: RowLayout {
                    required property string label
                    required property string value
                    Layout.fillWidth: true; spacing: 0
                    Text { text: parent.label; color: "#566070"; font.pixelSize: 11; Layout.preferredWidth: 120 }
                    Text { text: parent.value; color: "#c8d4e0"; font.pixelSize: 11 }
                }

                InfoRow { label: qsTr("Qt 版本");     value: "6.10.0" }
                InfoRow { label: qsTr("QML 引擎");   value: "V4 / JavaScript" }
                InfoRow { label: qsTr("构建类型");   value: "Debug" }
                InfoRow { label: qsTr("目标平台");   value: "Windows x64 (MSVC)" }
                InfoRow { label: qsTr("构建日期");   value: "2026-03-03" }
                InfoRow { label: qsTr("开源协议");   value: "GNU LGPL v3" }
                InfoRow { label: qsTr("官方网站");   value: "www.creality.com" }
            }
        }

        // 协议说明
        Rectangle {
            Layout.fillWidth: true; radius: 5; color: "#0f1318"; height: licText.implicitHeight + 16
            Text {
                id: licText
                anchors.left: parent.left; anchors.right: parent.right
                anchors.top: parent.top; anchors.margins: 10
                text: qsTr("本软件基于 Qt 6 框架构建，遵循 GNU LGPL v3 协议。使用本软件即代表您同意相关使用条款。")
                color: "#566070"; font.pixelSize: 10
                wrapMode: Text.WordWrap
                lineHeight: 1.5
            }
        }

        // 分割线
        Rectangle { Layout.fillWidth: true; height: 1; color: "#1e2535" }

        // 确认按钮
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 100; height: 30; radius: 4
            color: okHov.containsMouse ? "#19a84e" : "#157a39"
            Text { anchors.centerIn: parent; text: qsTr("确认"); color: "white"; font.pixelSize: 12; font.bold: true }
            MouseArea { id: okHov; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.close() }
        }
    }
}
