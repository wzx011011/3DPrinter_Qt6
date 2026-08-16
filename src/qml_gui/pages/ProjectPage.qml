import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../controls"

Item {
    id: root
    required property var projectVm
    signal newProjectRequested()
    signal openProjectDialogRequested()
    /// Viewmodel used by project import/save actions. Passed through from main.qml.
    property var editorVm: null
    property var _fileTree: []

    Component.onCompleted: {
        root.reloadFileTree()
    }

    function reloadFileTree() {
        // Use Q_INVOKABLE accessors - never touch QVariantList to avoid Qt6 V4 VariantAssociationObject crash.
        // Phase 241 (PAGE-02): entries derive from the REAL loaded project
        // (project file + plates + object source modules) via
        // ProjectViewModel::refreshFileTree; empty until something loads.
        var arr = []
        var n = projectVm.fileTreeCount()
        for (var i = 0; i < n; ++i)
            arr.push({ name: projectVm.fileTreeName(i), isDir: projectVm.fileTreeIsDir(i), depth: projectVm.fileTreeDepth(i) })
        _fileTree = arr
    }

    // Phase 241 (PAGE-02): keep the tree live across project open/new/save
    // and object edits (ProjectViewModel emits projectChanged on refresh).
    Connections {
        target: root.projectVm
        function onProjectChanged() { root.reloadFileTree() }
    }

    Rectangle { anchors.fill: parent; color: Theme.bgBase }

    // PAGE-01: Project operation file dialogs.
    FileDialog {
        id: importModelDlg
        title: qsTr("导入模型")
        nameFilters: [qsTr("模型 (*.stl *.3mf *.obj *.amf *.step *.stp)"), qsTr("所有文件 (*)")]
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
            // v2.4 IO-03: Save through EditorViewModel and ProjectService.
            var path = currentFile.toString().replace("file:///", "")
            if (root.editorVm) {
                var ok = root.editorVm.saveProjectAs(path)
                console.log("[ProjectPage] save to: " + path + " result=" + ok)
            }
        }
    }

    // v2.4 IO-03: Model export dialog for STL/3MF/OBJ.
    FileDialog {
        id: exportModelDlg
        title: qsTr("导出模型")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("STL (*.stl)"), qsTr("3MF (*.3mf)"), qsTr("OBJ (*.obj)")]
        onAccepted: {
            var path = currentFile.toString().replace("file:///", "")
            // Infer the export format from the file extension.
            var ext = path.split('.').pop().toLowerCase()
            if (root.editorVm) {
                var ok = root.editorVm.exportModel(path, ext)
                console.log("[ProjectPage] export to: " + path + " format=" + ext + " result=" + ok)
            }
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

                // PAGE-01: Project action buttons mapped to upstream MainFrame project actions.
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
                                    root.newProjectRequested()
                                    break
                                case "open":
                                    root.openProjectDialogRequested()
                                    break
                                case "save":
                                case "saveAs":
                                    saveProjectDlg.open()
                                    break
                                case "import":
                                    importModelDlg.open()
                                    break
                                case "export":
                                    // v2.4 IO-03: Export the model with the selected format.
                                    exportModelDlg.open()
                                    break
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Text { text: root.projectVm.isDirty ? qsTr("● 未保存") : ""; color: Theme.statusWarning; font.pixelSize: Theme.fontSizeSM; font.bold: true }
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
                               text: qsTr("项目资源"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; font.bold: true }
                    }

                    // Phase 241 (PAGE-02): honest empty state — no fabricated
                    // tree entries before a project is opened or a model is
                    // imported.
                    Text {
                        visible: root._fileTree.length === 0
                        text: qsTr("未加载项目：打开项目或导入模型后在此显示资源结构")
                        color: Theme.textDisabled
                        font.pixelSize: Theme.fontSizeXS
                        wrapMode: Text.WordWrap
                        width: parent.width - 24
                        anchors.horizontalCenter: parent.horizontalCenter
                        topPadding: 16
                    }

                    ScrollView {
                        width: parent.width
                        height: parent.height - 40
                        clip: true
                        ScrollBar.vertical.policy: ScrollBar.AsNeeded

                        Column {
                            width: parent.width
                            spacing: 0

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
                                        Text { text: modelData.isDir ? "📁" : "📄"; font.pixelSize: Theme.fontSizeMD }
                                        Text { text: modelData.name; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; elide: Text.ElideRight; width: 160 }
                                    }
                                    HoverHandler { id: itemHov }
                                }
                            }
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
                    Text { text: qsTr("选择文件查看详情"); color: Theme.textSecondary; font.pixelSize: Theme.fontSize13; horizontalAlignment: Text.AlignHCenter; width: parent.width }
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
                               text: qsTr("属性"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD; font.bold: true }
                    }

                    Column {
                        anchors.left: parent.left; anchors.right: parent.right; anchors.margins: 12
                        spacing: 8
                        topPadding: 12

                        Repeater {
                            // Phase 130 (POLISH-05) + Phase 241 (PAGE-02):
                            // Path/Format derive from currentProjectPath;
                            // Size/Modified read real QFileInfo data through
                            // ProjectViewModel (no more permanent "-").
                            model: {
                              var p = root.projectVm.currentProjectPath
                              var fmt = p ? p.split('.').pop().toUpperCase() : "—"
                              return [
                                [qsTr("Path"), p || qsTr("No project")],
                                [qsTr("Format"), p ? fmt : "—"],
                                [qsTr("Size"), p ? (root.projectVm.projectFileSizeText() || "—") : "—"],
                                [qsTr("Modified"), p ? (root.projectVm.projectLastModifiedText() || "—") : "—"],
                              ]
                            }
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
                                    Text { text: modelData[0]; color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS }
                                    Text { text: modelData[1]; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM }
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
                   color: Theme.textDisabled; font.pixelSize: Theme.fontSizeXS }
        }
    }
}
