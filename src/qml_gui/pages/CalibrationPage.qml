import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../dialogs"

Item {
    id: root
    required property var calibrationVm
    property var _calibItems: []

    CalibrationDialog {
        id: calibDlg
        calibrationVm: root.calibrationVm
    }

    Component.onCompleted: {
        // Use Q_INVOKABLE accessors - never touch QVariantList to avoid Qt6 V4 VariantAssociationObject crash
        var arr = []
        var n = calibrationVm.calibItemCount()
        for (var i = 0; i < n; ++i)
            arr.push({ icon: calibrationVm.calibItemIcon(i), name: calibrationVm.calibItemName(i), desc: calibrationVm.calibItemDesc(i) })
        _calibItems = arr
    }

    Rectangle { anchors.fill: parent; color: "#0d0f12" }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 44
            color: "#131720"
            Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 16
                   text: qsTr("校准中心"); color: "#e8edf6"; font.pixelSize: 15; font.bold: true }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Calibration item list
            Rectangle {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                color: "#0f1218"

                ScrollView {
                    anchors.fill: parent; anchors.topMargin: 8; clip: true
                    Column {
                        width: parent.width; spacing: 2
                        Repeater {
                            model: root._calibItems
                            delegate: Rectangle {
                                width: parent.width - 12; x: 6; height: 48; radius: 5
                                color: root.calibrationVm.selectedIndex === index ? "#1c2a3e" : (itemHov.containsMouse ? "#161d28" : "transparent")
                                border.color: root.calibrationVm.selectedIndex === index ? "#18c75e" : "transparent"; border.width: 1
                                Row {
                                    anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12; spacing: 10
                                    Text { text: modelData.icon || "⚙"; font.pixelSize: 20; color: "#a0abbe" }
                                    Column {
                                        spacing: 2
                                        Text { text: modelData.name; color: "#d0dae8"; font.pixelSize: 12 }
                                        Text { text: modelData.desc || ""; color: "#566070"; font.pixelSize: 10 }
                                    }
                                }
                                HoverHandler { id: itemHov }
                                TapHandler { onTapped: root.calibrationVm.selectItem(index) }
                            }
                        }
                    }
                }
            }

            // Detail content
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#0d0f12"

                Column {
                    anchors.centerIn: parent
                    spacing: 20
                    visible: root.calibrationVm.selectedIndex < 0

                    Text { text: "⚙"; font.pixelSize: 64; color: "#2e3444"; horizontalAlignment: Text.AlignHCenter; width: 300 }
                    Text { text: qsTr("选择左侧校准项目开始"); color: "#566070"; font.pixelSize: 14; horizontalAlignment: Text.AlignHCenter; width: 300 }
                }

                // When item selected
                ColumnLayout {
                    visible: root.calibrationVm.selectedIndex >= 0
                    anchors.fill: parent; anchors.margins: 32
                    spacing: 16

                    Text {
                        text: root.calibrationVm.selectedTitle
                        color: "#e8edf6"; font.pixelSize: 18; font.bold: true
                    }

                    Text {
                        text: root.calibrationVm.selectedDescription
                        color: "#a0abbe"; font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        Layout.fillWidth: true; Layout.preferredHeight: 240
                        color: "#131720"; radius: 8; border.color: "#2e3444"; border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: root.calibrationVm.selectedPreviewLabel
                            color: "#566070"; font.pixelSize: 13
                        }
                    }

                    RowLayout {
                        spacing: 12
                        Button {
                            text: qsTr("开始校准")
                            background: Rectangle { radius: 4; color: parent.pressed ? "#1a9e4f" : "#18c75e" }
                            contentItem: Text { text: parent.text; color: "white"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            onClicked: calibDlg.open()
                        }
                        Button {
                            text: qsTr("重置参数")
                            background: Rectangle { radius: 4; color: parent.pressed ? "#3a4258" : "#252b38"; border.color: "#363d4e"; border.width: 1 }
                            contentItem: Text { text: parent.text; color: "#c8d4e0"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                            onClicked: root.calibrationVm.resetParameters()
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }
        }
    }
}
