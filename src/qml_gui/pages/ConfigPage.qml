import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var configVm

    Rectangle {
        anchors.fill: parent
        color: "#1a1d23"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Label {
            text: "参数配置"
            color: "#ffffff"
            font.bold: true
        }

        ComboBox {
            id: presetBox
            Layout.fillWidth: true
            model: root.configVm.presetNames
            currentIndex: model.indexOf(root.configVm.currentPreset)
            onActivated: (idx) => root.configVm.setCurrentPreset(model[idx])
        }

        Label {
            text: "当前预设: " + root.configVm.currentPreset
            color: "#dfe6ef"
        }

        Label {
            text: "层高: " + Number(root.configVm.layerHeight).toFixed(2) + " mm"
            color: "#dfe6ef"
        }

        RowLayout {
            Button {
                text: "加载默认"
                onClicked: root.configVm.loadDefault()
            }
            Item { Layout.fillWidth: true }
        }

        Item { Layout.fillHeight: true }
    }
}
