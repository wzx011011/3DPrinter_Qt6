import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../controls"

Item {
    id: root
    required property var projectVm
    /// editorVm 注入（用于 loadFile 导入模型；由 main.qml 透传）
    property var editorVm: null
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

    // PAGE-01: 项目操作 FileDialog（打开/导入用）
    FileDialog {
        id: openProjectDlg
        title: qsTr("打开项目")
        nameFilters: [qsTr("3MF 项目 (*.3mf)"), qsTr("所有文件 (*)")]
        onAccepted: {
            // 打开 .3mf 项目 = 加载到 editorVm（ProjectService.loadFile）
            if (root.editorVm)
                root.editorVm.loadFile(currentFile.toString().replace("file:///", ""))
        }
    }

    FileDialog {
        id: importModelDlg
        title: qsTr("导入模型")
        nameFilters: [qsTr("模型 (*.stl *.3mf *.obj *.step *.stp)"), qsTr("所有文件 (*)")]
        onAccepted: {
            if (root.editorVm)
                root.editorVm.loadFile(currentFile.toString().replace("file:///", ""))
        }
    }

    FileDialog {
        id: saveProjectDlg
        title: qsTr("保存项目")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("3MF 项目 (*.3mf)")]
        defaultSuffix: "3mf"
        onAccepted: {
            // TODO PAGE-01: ProjectService 无 saveProject API, 当前占位（需 service 扩展）
            // 真实实现需 ProjectServiceMock.saveProjectAs(path)
            console.log("[ProjectPage] save to: " + currentFile)
        }
    }

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

                // PAGE-01: 6 按钮接线（对齐上游 MainFrame 项目操作）
                Repeater {
                    model: [
                        { label: qsTr("新建项目"), action: "new" },
                        { label: qsTr("打开"), action: "open" },
                        { label: qsTr("保存"), action: "save" },
                        { label: qsTr("另存为"), action: "saveAs" },
                        { label: qsTr("导入模型"), action: "import" },
                        { label: qsTr("导出"), action: "export" }
                    ]
                    delegate: CxButton {
                        text: modelData.label
                        compact: true
                        cxStyle: CxButton.Style.Secondary
                        onClicked: {
                            switch (modelData.action) {
                                case "new":
                                    // 新建 = 清空当前（TODO: ProjectService.clearProject）
                                    console.log("[ProjectPage] new project")
                                    break
                                case "open":
                                    openProjectDlg.open()
                                    break
                                case "save":
                                case "saveAs":
                                    saveProjectDlg.open()
                                    break
                                case "import":
                                    importModelDlg.open()
                                    break
                                case "export":
                                    // 导出 = 同 saveAs（TODO: 区分 G-code/3mf 导出）
                                    saveProjectDlg.open()
                                    break
                            }
                        }
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
