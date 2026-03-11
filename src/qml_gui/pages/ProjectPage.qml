import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

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

    Rectangle { anchors.fill: parent; color: Theme.bgBase }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingLG
        spacing: Theme.spacingMD

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: Theme.bgPanel
            radius: 16
            border.width: 1
            border.color: Theme.borderSubtle

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: Theme.spacingSM

                Repeater {
                    model: [qsTr("新建项目"),qsTr("打开"),qsTr("保存"),qsTr("另存为"),qsTr("导入模型"),qsTr("导出")]
                    delegate: CxButton {
                        text: modelData
                        compact: true
                        cxStyle: CxButton.Style.Secondary
                    }
                }

                Item { Layout.fillWidth: true }

                Text { text: root.projectVm.isDirty ? qsTr("● 未保存") : ""; color: Theme.statusWarning; font.pixelSize: 11; font.bold: true }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingMD

            Rectangle {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                color: Theme.bgPanel
                radius: 16
                border.width: 1
                border.color: Theme.borderSubtle

                Column {
                    anchors.fill: parent
                    anchors.margins: 0
                    spacing: 0

                    Rectangle {
                        width: parent.width; height: 40; color: Theme.bgSurface; radius: 16
                        Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12
                               text: qsTr("项目资源"); color: Theme.textSecondary; font.pixelSize: 12; font.bold: true }
                    }

                    Repeater {
                        model: root._fileTree
                        delegate: Rectangle {
                            required property var modelData
                            width: parent.width; height: 32
                            radius: 8
                            color: itemHov.containsMouse ? Theme.bgHover : "transparent"
                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: parent.left
                                anchors.leftMargin: 12 + (modelData.depth || 0) * 16
                                spacing: 6
                                Text { text: modelData.isDir ? "📁" : "📄"; font.pixelSize: 12 }
                                Text { text: modelData.name; color: Theme.textPrimary; font.pixelSize: 11 }
                            }
                            HoverHandler { id: itemHov }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.bgSurface
                radius: 18
                border.width: 1
                border.color: Theme.borderSubtle

                Column {
                    anchors.centerIn: parent
                    spacing: 12
                    visible: root.projectVm.selectedFile === ""
                    Text { text: "📋"; font.pixelSize: 48; color: Theme.textDisabled; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                    Text { text: qsTr("选择文件查看详情"); color: Theme.textSecondary; font.pixelSize: 13; horizontalAlignment: Text.AlignHCenter; width: parent.width }
                }
            }

            Rectangle {
                Layout.preferredWidth: 260
                Layout.fillHeight: true
                color: Theme.bgPanel
                radius: 16
                border.width: 1
                border.color: Theme.borderSubtle

                Column {
                    anchors.fill: parent; anchors.margins: 0
                    Rectangle {
                        width: parent.width; height: 40; color: Theme.bgSurface; radius: 16
                        Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12
                               text: qsTr("属性"); color: Theme.textSecondary; font.pixelSize: 12; font.bold: true }
                    }

                    Column {
                        anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 12
                        spacing: 8
                        topPadding: 12

                        Repeater {
                            model: [[qsTr("路径"),"—"],[qsTr("格式"),"—"],[qsTr("大小"),"—"],[qsTr("修改时间"),"—"]]
                            delegate: Rectangle {
                                required property var modelData
                                width: parent.width
                                height: 52
                                radius: 10
                                color: Theme.bgElevated
                                border.width: 1
                                border.color: Theme.borderSubtle

                                Column {
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    spacing: 4
                                    Text { text: modelData[0]; color: Theme.textDisabled; font.pixelSize: 10 }
                                    Text { text: modelData[1]; color: Theme.textPrimary; font.pixelSize: 11 }
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true; height: 28; color: Theme.bgPanel; radius: 12; border.width: 1; border.color: Theme.borderSubtle
            Text { anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12
                   text: root.projectVm.currentProjectPath !== "" ? root.projectVm.currentProjectPath : qsTr("无项目")
                   color: Theme.textDisabled; font.pixelSize: 10 }
        }
    }
}
