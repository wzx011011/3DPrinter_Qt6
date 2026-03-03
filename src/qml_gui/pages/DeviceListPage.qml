import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var monitorVm  // DeviceListPage reuses MonitorViewModel
    property var _devices: []

    Component.onCompleted: {
        // MonitorViewModel exposes deviceNames (QStringList) but no full device objects.
        // DeviceListPage shows empty state when no devices are available.
        _devices = []
    }

    Rectangle { anchors.fill: parent; color: "#0d0f12" }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Top bar
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 44; color: "#131720"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 12
                Text { text: qsTr("设备列表"); color: "#e8edf6"; font.pixelSize: 15; font.bold: true }
                Item { Layout.fillWidth: true }
                Rectangle { width: 26; height: 26; radius: 13; color: refreshHov.containsMouse ? "#2e3444" : "transparent"
                    Text { anchors.centerIn: parent; text: "↻"; color: "#a0abbe"; font.pixelSize: 16 }
                    HoverHandler { id: refreshHov }
                    TapHandler { onTapped: root.monitorVm.refreshDeviceList() }
                }
                Rectangle {
                    height: 28; width: 80; radius: 4; color: "#1c6e42"
                    Text { anchors.centerIn: parent; text: qsTr("+ 添加设备"); color: "white"; font.pixelSize: 11 }
                    TapHandler { onTapped: root.monitorVm.addDevice() }
                }
            }
        }

        // Table header
        Rectangle {
            Layout.fillWidth: true; height: 28; color: "#0f1218"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 0
                Text { text: qsTr("设备名称"); color: "#566070"; font.pixelSize: 11; Layout.preferredWidth: 220 }
                Text { text: qsTr("设备型号"); color: "#566070"; font.pixelSize: 11; Layout.preferredWidth: 160 }
                Text { text: qsTr("状态");     color: "#566070"; font.pixelSize: 11; Layout.preferredWidth: 120 }
                Text { text: "IP 地址";  color: "#566070"; font.pixelSize: 11; Layout.preferredWidth: 160 }
                Text { text: qsTr("最近打印"); color: "#566070"; font.pixelSize: 11; Layout.fillWidth: true }
            }
        }

        Item {
            Layout.fillWidth: true; Layout.fillHeight: true

            ScrollView {
                anchors.fill: parent; clip: true
                Column {
                    width: parent.width
                    Repeater {
                        model: root._devices
                        delegate: Rectangle {
                            width: parent.width; height: 52
                            color: index % 2 === 0 ? "#0a0d12" : "transparent"
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 0
                                Row {
                                    spacing: 10; Layout.preferredWidth: 220
                                    Rectangle { width: 8; height: 8; radius: 4; y: 22; color: modelData.online ? "#18c75e" : "#566070" }
                                    Column {
                                        spacing: 2; y: 10
                                        Text { text: modelData.name || (qsTr("设备 ") + (index+1)); color: "#d0dae8"; font.pixelSize: 12 }
                                        Text { text: modelData.sn || "SN: —"; color: "#566070"; font.pixelSize: 10 }
                                    }
                                }
                                Text { text: modelData.model || "K1C"; color: "#c0cad8"; font.pixelSize: 11; Layout.preferredWidth: 160 }
                                Rectangle {
                                    Layout.preferredWidth: 120
                                    Rectangle {
                                        width: 64; height: 18; radius: 9; y: 17
                                        color: modelData.online ? "#0e4a28" : "#2a2a2a"
                                        border.color: modelData.online ? "#18c75e" : "#566070"; border.width: 1
                                        Text { anchors.centerIn: parent; text: modelData.online ? qsTr("在线") : qsTr("离线"); color: modelData.online ? "#18c75e" : "#566070"; font.pixelSize: 10 }
                                    }
                                }
                                Text { text: modelData.ip || "—"; color: "#a0abbe"; font.pixelSize: 11; Layout.preferredWidth: 160 }
                                Text { text: modelData.lastPrint || "—"; color: "#566070"; font.pixelSize: 11; Layout.fillWidth: true; elide: Text.ElideRight }
                            }
                            Rectangle { width: parent.width; height: 1; y: parent.height - 1; color: "#151a22" }
                        }
                    }
                }
            }

            Column {
                anchors.centerIn: parent; spacing: 12
                visible: root._devices.length === 0
                Text { text: "📡"; font.pixelSize: 48; color: "#2e3444"; horizontalAlignment: Text.AlignHCenter; width: 200 }
                Text { text: qsTr("未发现设备\n请确认打印机与电脑在同一局域网"); color: "#566070"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; width: 200 }
            }
        }
    }
}
