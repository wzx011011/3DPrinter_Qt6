import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var modelMallVm
    property var _models: []

    Component.onCompleted: {
        // Use Q_INVOKABLE accessors - never touch QVariantList to avoid Qt6 V4 VariantAssociationObject crash
        var arr = []
        var n = modelMallVm.modelCount()
        for (var i = 0; i < n; ++i)
            arr.push({ name: modelMallVm.modelName(i), author: modelMallVm.modelAuthor(i),
                       likes: modelMallVm.modelLikes(i), free: modelMallVm.modelFree(i),
                       price: modelMallVm.modelPrice(i) })
        _models = arr
    }

    Rectangle { anchors.fill: parent; color: "#0d0f12" }

    ColumnLayout {
        anchors.fill: parent; spacing: 0

        // Search header
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 56; color: "#131720"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 12

                Text { text: qsTr("模型商城"); color: "#e8edf6"; font.pixelSize: 15; font.bold: true }

                Rectangle {
                    Layout.fillWidth: true; height: 32; radius: 16
                    color: "#1a1e28"; border.color: "#363d4e"; border.width: 1
                    Row {
                        anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12; spacing: 8
                        Text { text: "🔍"; font.pixelSize: 13; color: "#566070" }
                        TextField {
                            text: ""; placeholderText: qsTr("搜索模型…"); color: "#d0dae8"; font.pixelSize: 12
                            width: 300; background: Rectangle { color: "transparent" }
                        }
                    }
                }

                Repeater {
                    model: [qsTr("推荐"),qsTr("热门"),qsTr("最新"),qsTr("免费")]
                    delegate: Rectangle {
                        height: 28; width: 52; radius: 14
                        color: index === 0 ? "#1c5c38" : "#1a1e28"
                        border.color: index === 0 ? "#18c75e" : "#363d4e"; border.width: 1
                        Text { anchors.centerIn: parent; text: modelData; color: index === 0 ? "#18c75e" : "#a0abbe"; font.pixelSize: 11 }
                    }
                }
            }
        }

        // Category tabs
        Rectangle {
            Layout.fillWidth: true; height: 38; color: "#0f1218"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; spacing: 4
                Repeater {
                    model: [qsTr("全部"),qsTr("家居"),qsTr("玩具"),qsTr("工具"),qsTr("艺术"),qsTr("教育"),qsTr("珠宝"),qsTr("运动")]
                    delegate: Rectangle {
                        height: 26; width: implicitText.width + 20; radius: 13
                        color: index === root.modelMallVm.categoryIndex ? "#1c6e42" : "transparent"
                        Text { id: implicitText; anchors.centerIn: parent; text: modelData; color: index === root.modelMallVm.categoryIndex ? "#18c75e" : "#8a96a8"; font.pixelSize: 11 }
                        TapHandler { onTapped: root.modelMallVm.setCategoryIndex(index) }
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }

        // Model grid - Repeater avoids QQmlDelegateModel incubation crash
        ScrollView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            Flow {
                width: parent.width; spacing: 12; topPadding: 12; leftPadding: 12
                Repeater {
                    model: root._models
                    delegate: Rectangle {
                        width: 178; height: 210; radius: 8
                        color: "#131720"; border.color: "#1e2430"; border.width: 1
                        Column {
                            anchors.fill: parent; spacing: 0
                            Rectangle {
                                width: parent.width; height: 130; radius: 8; color: "#1a1e28"
                                Text { anchors.centerIn: parent; text: ["🏠","🎮","🔧","🎨","📚"][index % 5]; font.pixelSize: 40; color: "#3a4250" }
                                Rectangle {
                                    anchors.right: parent.right; anchors.bottom: parent.bottom; anchors.margins: 6
                                    width: 40; height: 18; radius: 9; color: "#0a0d12"
                                    Text { anchors.centerIn: parent; text: "♥ " + (modelData.likes || (100 + index * 37)); color: "#e04040"; font.pixelSize: 9 }
                                }
                            }
                            Column {
                                anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 8; spacing: 4; topPadding: 8
                                Text { text: modelData.name || (qsTr("3D 模型 #") + (index + 1)); color: "#d0dae8"; font.pixelSize: 11; font.bold: true; elide: Text.ElideRight; width: parent.width }
                                RowLayout {
                                    Text { text: modelData.author || "Creality"; color: "#566070"; font.pixelSize: 10 }
                                    Item { Layout.fillWidth: true }
                                    Text { text: modelData.free ? qsTr("免费") : ("¥" + (modelData.price || (9.9 + index))); color: modelData.free ? "#18c75e" : "#f5a623"; font.pixelSize: 11; font.bold: true }
                                }
                            }
                        }
                        HoverHandler { id: mallHov }
                        Rectangle { anchors.fill: parent; radius: parent.radius; color: mallHov.hovered ? "#08ffffff" : "transparent" }
                    }
                }
            }
        }
    }
}
