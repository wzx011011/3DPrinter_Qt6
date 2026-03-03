import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    required property var projectVm
    property var _fileTree: []

    Component.onCompleted: {
        // Use Q_INVOKABLE accessors - never touch QVariantList to avoid Qt6 V4 VariantAssociationObject crash
        var arr = []
        var n = projectVm.fileTreeCount()
        for (var i = 0; i < n; ++i)
            arr.push({ name: projectVm.fileTreeName(i), isDir: projectVm.fileTreeIsDir(i), depth: projectVm.fileTreeDepth(i) })
        _fileTree = arr
    }

    Rectangle { anchors.fill: parent; color: "#0d0f12" }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Toolbar row
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            color: "#131720"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                Repeater {
                    model: [qsTr("新建项目"),qsTr("打开"),qsTr("保存"),qsTr("另存为"),qsTr("导入模型"),qsTr("导出")]
                    delegate: Rectangle {
                        height: 26; width: implicitWidth + 18; radius: 4
                        color: btnHov.containsMouse ? "#2e3444" : "#1a1e28"
                        border.color: "#363d4e"; border.width: 1
                        Text { anchors.centerIn: parent; text: modelData; color: "#c8d4e0"; font.pixelSize: 11 }
                        HoverHandler { id: btnHov }
                    }
                }

                Item { Layout.fillWidth: true }

                Text { text: root.projectVm.isDirty ? qsTr("● 未保存") : ""; color: "#f5a623"; font.pixelSize: 11 }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // File tree
            Rectangle {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                color: "#0f1218"

                Column {
                    anchors.fill: parent
                    anchors.margins: 0
                    spacing: 0

                    Rectangle {
                        width: parent.width; height: 32; color: "#131720"
                        Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12
                               text: qsTr("项目资源"); color: "#a0abbe"; font.pixelSize: 12; font.bold: true }
                    }

                    Repeater {
                        model: root._fileTree
                        delegate: Rectangle {
                            required property var modelData
                            width: parent.width; height: 28
                            color: itemHov.containsMouse ? "#1a1e28" : "transparent"
                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 12 + (modelData.depth || 0) * 16
                                spacing: 6
                                Text { text: modelData.isDir ? "📁" : "📄"; font.pixelSize: 12 }
                                Text { text: modelData.name; color: "#c0cad8"; font.pixelSize: 11 }
                            }
                            HoverHandler { id: itemHov }
                        }
                    }
                }
            }

            // Detail panel
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#0d0f12"

                Column {
                    anchors.centerIn: parent
                    spacing: 12
                    visible: root.projectVm.selectedFile === ""
                    Text { text: "📋"; font.pixelSize: 48; color: "#3a4250"; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                    Text { text: qsTr("选择文件查看详情"); color: "#566070"; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                }
            }

            // Properties panel
            Rectangle {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                color: "#0f1218"

                Column {
                    anchors.fill: parent; anchors.margins: 0
                    Rectangle {
                        width: parent.width; height: 32; color: "#131720"
                        Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12
                               text: qsTr("属性"); color: "#a0abbe"; font.pixelSize: 12; font.bold: true }
                    }

                    Column {
                        anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 12
                        spacing: 8
                        topPadding: 12

                        Repeater {
                            model: [[qsTr("路径"),"—"],[qsTr("格式"),"—"],[qsTr("大小"),"—"],[qsTr("修改时间"),"—"]]
                            delegate: Column {
                                required property var modelData
                                spacing: 2
                                Text { text: modelData[0]; color: "#566070"; font.pixelSize: 10 }
                                Text { text: modelData[1]; color: "#c0cad8"; font.pixelSize: 11 }
                            }
                        }
                    }
                }
            }
        }

        // Status bar
        Rectangle {
            Layout.fillWidth: true; height: 22; color: "#090b0e"
            Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12
                   text: root.projectVm.currentProjectPath !== "" ? root.projectVm.currentProjectPath : qsTr("无项目")
                   color: "#3a4250"; font.pixelSize: 10 }
        }
    }
}
