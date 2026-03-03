import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var homeVm
    // Pure-JS copy - avoids Qt6 V4 VariantAssociationObject lifetime crash
    // Initialized to [] so Repeater componentComplete() sees empty model
    property var _recentProjects: []

    Component.onCompleted: {
        // Use Q_INVOKABLE accessors - never touch QVariantList to avoid Qt6 V4 VariantAssociationObject crash
        var arr = []
        var n = homeVm.recentProjectCount()
        for (var i = 0; i < n; ++i)
            arr.push({ name: homeVm.recentProjectName(i), date: homeVm.recentProjectDate(i), path: homeVm.recentProjectPath(i) })
        _recentProjects = arr
    }

    Rectangle { anchors.fill: parent; color: "#0d0f12" }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 20

        // Header
        RowLayout {
            Layout.fillWidth: true
            spacing: 16

            Column {
                spacing: 4
                Text { text: "Creality Print 7.0"; color: "#e8edf6"; font.pixelSize: 22; font.bold: true }
                Text { text: qsTr("专业级 3D 打印切片软件"); color: "#7a8898"; font.pixelSize: 13 }
            }

            Item { Layout.fillWidth: true }

            // Login state
            Rectangle {
                width: 100; height: 30; radius: 15
                color: "#1c6e42"
                Text { anchors.centerIn: parent; text: qsTr("登录账号"); color: "white"; font.pixelSize: 12 }
            }
        }

        // Recent Projects Section
        Text { text: qsTr("最近项目"); color: "#a0abbe"; font.pixelSize: 13; font.bold: true }

        ScrollView {
            Layout.fillWidth: true; Layout.preferredHeight: 160; clip: true
            Flow {
                width: parent.width; spacing: 10
                Repeater {
                    model: root._recentProjects
                    delegate: Rectangle {
                        width: 170; height: 140; radius: 6; color: "#1a1e28"; border.color: "#2e3444"; border.width: 1
                        Column {
                            anchors.fill: parent; anchors.margins: 8; spacing: 6
                            Rectangle { width: parent.width; height: 90; radius: 4; color: "#252b38"
                                Text { anchors.centerIn: parent; text: "🖨"; font.pixelSize: 32; color: "#566070" }
                            }
                            Text { text: modelData.name || (qsTr("项目 ") + (index + 1)); color: "#c8d4e0"; font.pixelSize: 11; elide: Text.ElideRight; width: parent.width }
                            Text { text: modelData.date || "—"; color: "#566070"; font.pixelSize: 10 }
                        }
                        HoverHandler { id: recentHover }
                        Rectangle { anchors.fill: parent; radius: parent.radius; color: recentHover.hovered ? "#0a18c75e" : "transparent" }
                    }
                }
            }
        }

        // Quick actions
        Text { text: qsTr("快速入口"); color: "#a0abbe"; font.pixelSize: 13; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Repeater {
                model: [
                    { icon: "📂", title: qsTr("打开项目"),   sub: qsTr("打开已有 3MF/STL 文件") },
                    { icon: "➕", title: qsTr("新建项目"),   sub: qsTr("从空白开始创建") },
                    { icon: "🔧", title: qsTr("校准"),       sub: qsTr("打印机校准向导") },
                    { icon: "🌐", title: qsTr("模型商城"),   sub: qsTr("在线下载模型") }
                ]
                delegate: Rectangle {
                    required property var modelData
                    Layout.fillWidth: true
                    height: 80
                    radius: 6
                    color: qaHover.hovered ? "#1c2130" : "#131720"
                    border.color: "#2e3444"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 4
                        Text { text: parent.parent.modelData.icon; font.pixelSize: 22; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                        Text { text: parent.parent.modelData.title; color: "#d0dae8"; font.pixelSize: 12; font.bold: true; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                        Text { text: parent.parent.modelData.sub; color: "#566070"; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                    }

                    HoverHandler { id: qaHover }
                }
            }
        }

        Item { Layout.fillHeight: true }

        // Version info
        Text { text: qsTr("版本 7.0.0  |  Qt 6.10  |  ©2026 Creality"); color: "#3a4250"; font.pixelSize: 10 }
    }
}
