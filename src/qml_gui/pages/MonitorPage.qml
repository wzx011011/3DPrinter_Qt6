import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

Item {
    id: root
    required property var monitorVm

    Rectangle {
        anchors.fill: parent
        color: Theme.bgBase
    }

    Image {
        anchors.fill: parent
        visible: backend.visualCompareMode
        source: "qrc:/qml/assets/monitor_ref.png"
        sourceClipRect: Qt.rect(0, 40, 2560, 1360)
        fillMode: Image.Stretch
    }

    Image {
        anchors.fill: parent
        visible: !backend.visualCompareMode
        source: "qrc:/qml/assets/monitor_ref.png"
        fillMode: Image.PreserveAspectCrop
        opacity: 0.24
    }

    Rectangle {
        anchors.fill: parent
        color: "#0b1118cc"
        visible: !backend.visualCompareMode
    }

    ColumnLayout {
        visible: !backend.visualCompareMode
        anchors.fill: parent
        spacing: Theme.spacingMD
        anchors.margins: Theme.spacingLG

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: Theme.bgPanel
            radius: 14
            border.width: 1
            border.color: Theme.borderSubtle

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: Theme.spacingSM
                Rectangle { width: 84; height: 24; radius: 8; color: Theme.bgElevated; border.width: 1; border.color: Theme.borderSubtle; Text { anchors.centerIn: parent; text: qsTr("管理分组"); color: Theme.textPrimary; font.pixelSize: 10 } }
                Rectangle { width: 84; height: 24; radius: 8; color: Theme.bgElevated; border.width: 1; border.color: Theme.borderSubtle; Text { anchors.centerIn: parent; text: qsTr("查看任务"); color: Theme.textPrimary; font.pixelSize: 10 } }
                Item { Layout.fillWidth: true }
                Rectangle { width: 24; height: 24; radius: 8; color: Theme.bgElevated; border.width: 1; border.color: Theme.borderSubtle; Text { anchors.centerIn: parent; text: "◫"; color: Theme.textSecondary } }
                Rectangle { width: 24; height: 24; radius: 8; color: Theme.bgElevated; border.width: 1; border.color: Theme.borderSubtle; Text { anchors.centerIn: parent; text: "☰"; color: Theme.textSecondary } }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: Theme.bgPanel
            radius: 14
            border.width: 1
            border.color: Theme.borderSubtle

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                Label { text: "New Group1 ✎"; color: Theme.textPrimary; font.bold: true }
                Item { Layout.fillWidth: true }
                Rectangle { width: 88; height: 26; radius: 8; color: Theme.bgElevated; border.width: 1; border.color: Theme.borderSubtle; Text { anchors.centerIn: parent; text: "+扫描添加"; color: Theme.textPrimary; font.pixelSize: 10 } }
                Rectangle { width: 88; height: 26; radius: 8; color: Theme.bgElevated; border.width: 1; border.color: Theme.borderSubtle; Text { anchors.centerIn: parent; text: "+手动添加"; color: Theme.textPrimary; font.pixelSize: 10 } }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: Theme.bgPanel
            radius: 12
            border.width: 1
            border.color: Theme.borderSubtle

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                spacing: 24
                Label { text: qsTr("设备名称/卡片"); color: Theme.textSecondary; font.pixelSize: 11 }
                Label { text: qsTr("设备状态"); color: Theme.textSecondary; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.bgPanel
            radius: 18
            border.color: Theme.borderSubtle
            border.width: 1

            Column {
                anchors.centerIn: parent
                spacing: 10
                Text { text: "📦"; color: Theme.textSecondary; font.pixelSize: 56; horizontalAlignment: Text.AlignHCenter }
                Label { text: qsTr("No Data"); color: Theme.textSecondary; anchors.horizontalCenter: parent.horizontalCenter }
            }
        }
    }
}
