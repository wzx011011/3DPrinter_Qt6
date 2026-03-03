import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var monitorVm

    Rectangle {
        anchors.fill: parent
        color: "#101319"
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
        opacity: 0.92
    }

    ColumnLayout {
        visible: !backend.visualCompareMode
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: "#171a20"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                Rectangle { width: 62; height: 18; radius: 2; color: "#3b424c"; Text { anchors.centerIn: parent; text: "管理分组"; color: "#dce3ed"; font.pixelSize: 10 } }
                Rectangle { width: 62; height: 18; radius: 2; color: "#3b424c"; Text { anchors.centerIn: parent; text: "查看任务"; color: "#dce3ed"; font.pixelSize: 10 } }
                Item { Layout.fillWidth: true }
                Rectangle { width: 20; height: 20; radius: 2; color: "#2e343d"; Text { anchors.centerIn: parent; text: "◫"; color: "#aab4c1" } }
                Rectangle { width: 20; height: 20; radius: 2; color: "#2e343d"; Text { anchors.centerIn: parent; text: "☰"; color: "#aab4c1" } }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            color: "#1a1d23"

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                Label { text: "New Group1 ✎"; color: "#dce3ed" }
                Item { Layout.fillWidth: true }
                Rectangle { width: 68; height: 20; radius: 2; color: "#3b424c"; Text { anchors.centerIn: parent; text: "+扫描添加"; color: "#dce3ed"; font.pixelSize: 10 } }
                Rectangle { width: 68; height: 20; radius: 2; color: "#3b424c"; Text { anchors.centerIn: parent; text: "+手动添加"; color: "#dce3ed"; font.pixelSize: 10 } }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 28
            color: "#1d2229"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                spacing: 24
                Label { text: "设备名称/卡片"; color: "#8f9aa9"; font.pixelSize: 11 }
                Label { text: "设备状态"; color: "#8f9aa9"; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#1a1d23"
            border.color: "#2a3039"
            border.width: 1
            anchors.margins: 8

            Column {
                anchors.centerIn: parent
                spacing: 8
                Text { text: "📦"; color: "#cbd3df"; font.pixelSize: 56; horizontalAlignment: Text.AlignHCenter }
                Label { text: "No Data"; color: "#9eabba"; anchors.horizontalCenter: parent.horizontalCenter }
            }
        }
    }
}
