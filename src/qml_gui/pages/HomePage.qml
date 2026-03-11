import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

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

    Rectangle { anchors.fill: parent; color: Theme.bgBase }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingXXL
        spacing: Theme.spacingXL

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 88
            radius: 20
            color: Theme.bgPanel
            border.width: 1
            border.color: Theme.borderSubtle

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                spacing: Theme.spacingXL

                Rectangle {
                    width: 48
                    height: 48
                    radius: 14
                    color: Theme.accentSubtle
                    border.width: 1
                    border.color: Theme.accentDark

                    Text {
                        anchors.centerIn: parent
                        text: "△"
                        color: Theme.accentLight
                        font.pixelSize: 22
                        font.bold: true
                    }
                }

                Column {
                    spacing: 4
                    Text { text: "Creality Print 7.0"; color: Theme.textPrimary; font.pixelSize: 24; font.bold: true }
                    Text { text: qsTr("专业级 3D 打印切片软件"); color: Theme.textSecondary; font.pixelSize: 13 }
                }

                Item { Layout.fillWidth: true }

                CxButton {
                    text: qsTr("登录账号")
                    cxStyle: CxButton.Style.Primary
                }
            }
        }

        Text { text: qsTr("最近项目"); color: Theme.textSecondary; font.pixelSize: 13; font.bold: true }

        ScrollView {
            Layout.fillWidth: true; Layout.preferredHeight: 188; clip: true
            Flow {
                width: parent.width; spacing: Theme.spacingMD
                Repeater {
                    model: root._recentProjects
                    delegate: Rectangle {
                        width: 196; height: 164; radius: 16; color: Theme.bgPanel; border.color: Theme.borderSubtle; border.width: 1
                        Column {
                            anchors.fill: parent; anchors.margins: 12; spacing: 8
                            Rectangle { width: parent.width; height: 102; radius: 12; color: Theme.bgElevated
                                Text { anchors.centerIn: parent; text: "🖨"; font.pixelSize: 34; color: Theme.textDisabled }
                            }
                            Text { text: modelData.name || (qsTr("项目 ") + (index + 1)); color: Theme.textPrimary; font.pixelSize: 12; font.bold: true; elide: Text.ElideRight; width: parent.width }
                            Text { text: modelData.date || "—"; color: Theme.textSecondary; font.pixelSize: 10 }
                            Text { text: modelData.path || ""; color: Theme.textDisabled; font.pixelSize: 10; elide: Text.ElideRight; width: parent.width }
                        }
                        HoverHandler { id: recentHover }
                        Rectangle { anchors.fill: parent; radius: parent.radius; color: recentHover.hovered ? "#1018c75e" : "transparent" }
                    }
                }
            }
        }

        Text { text: qsTr("快速入口"); color: Theme.textSecondary; font.pixelSize: 13; font.bold: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingLG

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
                    height: 112
                    radius: 18
                    color: qaHover.hovered ? Theme.bgHover : Theme.bgPanel
                    border.color: Theme.borderSubtle
                    border.width: 1

                    Column {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8
                        Text { text: parent.parent.modelData.icon; font.pixelSize: 24; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                        Text { text: parent.parent.modelData.title; color: Theme.textPrimary; font.pixelSize: 13; font.bold: true; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                        Text { text: parent.parent.modelData.sub; color: Theme.textSecondary; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter; width: parent.width; wrapMode: Text.WordWrap }
                    }

                    HoverHandler { id: qaHover }
                }
            }
        }

        Item { Layout.fillHeight: true }

        Text { text: qsTr("版本 7.0.0  |  Qt 6.10  |  ©2026 Creality"); color: Theme.textDisabled; font.pixelSize: 10 }
    }
}
