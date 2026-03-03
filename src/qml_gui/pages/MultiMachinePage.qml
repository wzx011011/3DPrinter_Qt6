import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var multiMachineVm
    property var _machines: []

    Component.onCompleted: {
        // Use Q_INVOKABLE accessors - never touch QVariantList to avoid Qt6 V4 VariantAssociationObject crash
        var arr = []
        var n = multiMachineVm.machineCount()
        for (var i = 0; i < n; ++i)
            arr.push({ name: multiMachineVm.machineName(i), model: multiMachineVm.machineModel(i),
                       status: multiMachineVm.machineStatus(i), online: multiMachineVm.machineOnline(i),
                       selected: multiMachineVm.machineSelected(i), progress: multiMachineVm.machineProgress(i),
                       remaining: multiMachineVm.machineRemaining(i), ip: multiMachineVm.machineIp(i) })
        _machines = arr
    }

    Rectangle { anchors.fill: parent; color: "#0d0f12" }

    ColumnLayout {
        anchors.fill: parent; spacing: 0

        // Header
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 48; color: "#131720"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 12
                Text { text: qsTr("多机打印"); color: "#e8edf6"; font.pixelSize: 15; font.bold: true }
                Item { Layout.fillWidth: true }
                Rectangle {
                    height: 28; width: 90; radius: 4; color: "#1c6e42"
                    Text { anchors.centerIn: parent; text: qsTr("发送到全部"); color: "white"; font.pixelSize: 11 }
                    TapHandler { onTapped: root.multiMachineVm.sendToAll() }
                }
                Rectangle {
                    height: 28; width: 80; radius: 4; color: "#252b38"; border.color: "#363d4e"; border.width: 1
                    Text { anchors.centerIn: parent; text: qsTr("任务队列"); color: "#c8d4e0"; font.pixelSize: 11 }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0

            // Machine grid - Repeater avoids QQmlDelegateModel incubation crash
            ScrollView {
                Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                Flow {
                    width: parent.width; spacing: 12; topPadding: 12; leftPadding: 12
                    Repeater {
                        model: root._machines
                        delegate: Rectangle {
                            width: 248; height: 190; radius: 8
                            color: "#131720"; border.color: modelData.selected ? "#18c75e" : "#1e2430"; border.width: modelData.selected ? 2 : 1
                            Column {
                                anchors.fill: parent; anchors.margins: 12; spacing: 8
                                RowLayout {
                                    width: parent.width
                                    Row { spacing: 6
                                        Rectangle { width: 8; height: 8; radius: 4; y: 5; color: modelData.online ? "#18c75e" : "#566070" }
                                        Text { text: modelData.name || (qsTr("打印机 ") + (index + 1)); color: "#d0dae8"; font.pixelSize: 12; font.bold: true }
                                    }
                                    Item { Layout.fillWidth: true }
                                    Text { text: modelData.model || "K1C"; color: "#566070"; font.pixelSize: 10 }
                                }
                                Rectangle {
                                    width: parent.width; height: 80; radius: 6; color: "#1a1e28"
                                    Column {
                                        anchors.centerIn: parent; spacing: 6
                                        Text { text: modelData.status || qsTr("空闲"); color: "#a0abbe"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; width: 220 }
                                        Rectangle {
                                            width: 200; height: 6; radius: 3; color: "#252b38"
                                            Rectangle { width: parent.width * ((modelData.progress || 0) / 100.0); height: parent.height; radius: 3; color: "#18c75e" }
                                        }
                                        Text { text: (modelData.progress || 0) + "%  ·  " + (modelData.remaining || "—"); color: "#566070"; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter; width: 220 }
                                    }
                                }
                                RowLayout {
                                    width: parent.width; spacing: 6
                                    Repeater {
                                        id: btnRep
                                        model: [qsTr("发送"),qsTr("暂停"),qsTr("停止")]
                                        delegate: Rectangle {
                                            Layout.fillWidth: true; height: 24; radius: 4
                                            readonly property int btnIdx: index
                                            color: btnIdx === 0 ? "#1c5c38" : "#252b38"
                                            border.color: btnIdx === 0 ? "#18c75e" : "#363d4e"; border.width: 1
                                            Text { anchors.centerIn: parent; text: modelData; color: btnIdx === 0 ? "#18c75e" : "#a0abbe"; font.pixelSize: 10 }
                                        }
                                    }
                                }
                            }
                            TapHandler { onTapped: root.multiMachineVm.selectMachine(index) }
                        }
                    }
                }
            }

            // Right panel - task queue
            Rectangle {
                Layout.preferredWidth: 260; Layout.fillHeight: true; color: "#0f1218"
                Column {
                    anchors.fill: parent;
                    Rectangle {
                        width: parent.width; height: 36; color: "#131720"
                        Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12
                               text: qsTr("任务队列"); color: "#a0abbe"; font.pixelSize: 12; font.bold: true }
                    }
                    Column {
                        anchors.horizontalCenter: parent.horizontalCenter
                        topPadding: 40; spacing: 8
                        visible: root._machines.length === 0 && multiMachineVm.taskQueueCount() === 0
                        Text { text: "📭"; font.pixelSize: 36; color: "#2e3444"; horizontalAlignment: Text.AlignHCenter; width: 200 }
                        Text { text: qsTr("暂无任务"); color: "#566070"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; width: 200 }
                    }
                }
            }
        }
    }
}
