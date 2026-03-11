import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

Item {
    id: root
    required property var configVm

    Rectangle {
        anchors.fill: parent
        color: Theme.bgBase
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingLG
        spacing: Theme.spacingMD

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            radius: 16
            color: Theme.bgPanel
            border.width: 1
            border.color: Theme.borderSubtle

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                Label {
                    text: qsTr("参数配置")
                    color: Theme.textPrimary
                    font.bold: true
                    font.pixelSize: Theme.fontSizeLG
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingMD

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 170
                radius: 16
                color: Theme.bgPanel
                border.width: 1
                border.color: Theme.borderSubtle

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    Text {
                        text: qsTr("预设")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        font.bold: true
                    }

                    CxComboBox {
                        id: presetBox
                        Layout.fillWidth: true
                        model: root.configVm.presetNames
                        currentIndex: model.indexOf(root.configVm.currentPreset)
                        onActivated: (idx) => root.configVm.setCurrentPreset(model[idx])
                    }

                    Text {
                        text: qsTr("当前预设: ") + root.configVm.currentPreset
                        color: Theme.textPrimary
                    }

                    Text {
                        text: qsTr("层高: ") + Number(root.configVm.layerHeight).toFixed(2) + " mm"
                        color: Theme.textPrimary
                    }

                    Item { Layout.fillHeight: true }

                    CxButton {
                        text: qsTr("加载默认")
                        onClicked: root.configVm.loadDefault()
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 170
                radius: 16
                color: Theme.bgPanel
                border.width: 1
                border.color: Theme.borderSubtle

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8

                    Text {
                        text: qsTr("概览")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeSM
                        font.bold: true
                    }

                    Text { text: qsTr("打印速度: ") + root.configVm.printSpeed; color: Theme.textPrimary }
                    Text { text: qsTr("壁线圈数: ") + root.configVm.wallCount; color: Theme.textPrimary }
                    Text { text: qsTr("支撑: ") + (root.configVm.supportEnabled ? qsTr("开启") : qsTr("关闭")); color: Theme.textPrimary }
                    Text { text: qsTr("Brim: ") + (root.configVm.enableBrim ? qsTr("开启") : qsTr("关闭")); color: Theme.textPrimary }
                }
            }
        }

        Item { Layout.fillHeight: true }
    }
}
