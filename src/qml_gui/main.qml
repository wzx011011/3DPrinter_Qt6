import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window
import "controls"
import "pages"
import "components"
import "dialogs"

ApplicationWindow {
    id: root
    width: 1828
    height: 1000
    visible: true
    title: "OWzx Slicer"
    color: "#0d0f12"
    flags: Qt.Window | Qt.FramelessWindowHint
    minimumWidth: 1100
    minimumHeight: 700

    readonly property int resizeMargin: 6
    readonly property int frameMargin: (backend.visualCompareMode || root.visibility === Window.Maximized) ? 0 : 10
    readonly property int frameRadius: (backend.visualCompareMode || root.visibility === Window.Maximized) ? 0 : 18

    readonly property int pagePrepare: 1
    readonly property int pagePreview: 2
    readonly property int pageDevice: 7
    readonly property int pageOnline: 9
    readonly property color accentColor: "#18c75e"
    readonly property color topbarHover: "#1b2230"
    readonly property color topbarPressed: "#242c3a"
    readonly property color topbarText: "#cdd7e6"

    property var workflowTabs: buildWorkflowTabs()
    property var hiddenTabs: ({})
    property int pendingSwitchToken: -1
    property int pendingSwitchTargetPage: -1

    function toggleTabVisibility(pageIdx) {
        var copy = Object.assign({}, hiddenTabs)
        if (copy[pageIdx])
            delete copy[pageIdx]
        else
            copy[pageIdx] = true
        hiddenTabs = copy
        workflowTabs = buildWorkflowTabs()
    }

    function buildWorkflowTabs() {
        var allTabs = [
            { label: qsTr("在线模型"), page: root.pageOnline },
            { label: qsTr("准备"), page: root.pagePrepare },
            { label: qsTr("预览"), page: root.pagePreview },
            { label: qsTr("设备"), page: root.pageDevice }
        ]
        return allTabs.filter(function(t) { return !hiddenTabs[t.page] })
    }

    component TitleBarDivider: Rectangle {
        implicitWidth: 1
        implicitHeight: 18
        radius: 1
        color: "#303030"
    }

    Connections {
        target: backend
        function onLanguageChanged() { root.workflowTabs = root.buildWorkflowTabs() }
    }

    onFrameSwapped: {
        if (root.pendingSwitchToken >= 0 && backend.currentPage === root.pendingSwitchTargetPage) {
            backend.endLatency(root.pendingSwitchToken)
            root.pendingSwitchToken = -1
            root.pendingSwitchTargetPage = -1
        }
    }

    FileDialog {
        id: openModelDialog
        title: qsTr("打开模型文件")
        nameFilters: [
            qsTr("3MF 文件 (*.3mf)"),
            qsTr("STL 文件 (*.stl)"),
            qsTr("OBJ 文件 (*.obj)"),
            qsTr("STEP 文件 (*.step *.stp)"),
            qsTr("AMF 文件 (*.amf)"),
            qsTr("所有文件 (*)")
        ]
        onAccepted: {
            backend.topbarImportModel(selectedFile.toString())
        }
    }

    FileDialog {
        id: openProjectDialog
        title: qsTr("打开项目")
        nameFilters: [qsTr("项目文件 (*.3mf *.cxprj *.json)"), qsTr("所有文件 (*)")]
        onAccepted: {
            backend.topbarOpenProject(selectedFile.toString())
        }
    }

    FileDialog {
        id: saveProjectAsDialog
        title: qsTr("项目另存为")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("项目文件 (*.3mf *.cxprj)"), qsTr("项目元数据 (*.json)")]
        onAccepted: {
            backend.topbarSaveProjectAs(selectedFile.toString())
        }
    }

    CxMenu {
        id: fileMenu
        CxMenuItem {
            text: qsTr("新建项目")
            onTriggered: newProjectDialog.open()
        }
        CxMenuItem { text: qsTr("打开项目..."); onTriggered: openProjectDialog.open() }
        CxMenuItem { text: qsTr("导入模型..."); onTriggered: openModelDialog.open() }
        MenuSeparator {}
        CxMenuItem {
            text: qsTr("保存项目")
            onTriggered: {
                if (!backend.topbarSaveProject())
                    saveProjectAsDialog.open()
            }
        }
        CxMenuItem { text: qsTr("项目另存为..."); onTriggered: saveProjectAsDialog.open() }
        MenuSeparator {}
        CxMenu {
            id: fileRecentMenu
            title: qsTr("最近文件")
            Instantiator {
                model: backend.projectViewModel ? backend.projectViewModel.recentProjects : []
                delegate: CxMenuItem {
                    required property string modelData
                    text: modelData
                    onTriggered: {
                        backend.topbarOpenProject(modelData)
                    }
                }
                onObjectAdded: (index, object) => fileRecentMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => fileRecentMenu.removeItem(object)
            }
            MenuSeparator {}
            CxMenuItem {
                text: qsTr("清空最近文件")
                enabled: backend.projectViewModel && backend.projectViewModel.recentProjects.length > 0
                onTriggered: backend.projectViewModel && backend.projectViewModel.clearRecentProjects()
            }
        }
        MenuSeparator {}
        CxMenuItem { text: qsTr("退出"); onTriggered: Qt.quit() }
    }

    // Top dropdown menu (对齐上游 BBLTopbar dropdown → Edit/View/Preferences/Calibration/Help)
    CxMenu {
        id: topMenu

        CxMenu {
            title: qsTr("编辑")
            CxMenuItem {
                text: qsTr("撤销")
                enabled: backend.currentPage === root.pagePrepare
                onTriggered: preparePage.undoFromTopbar()
            }
            CxMenuItem {
                text: qsTr("重做")
                enabled: backend.currentPage === root.pagePrepare
                onTriggered: preparePage.redoFromTopbar()
            }
            MenuSeparator {}
            CxMenuItem {
                text: qsTr("剪切")
                enabled: backend.editorViewModel && backend.editorViewModel.hasSelection
                onTriggered: backend.editorViewModel.cutSelectedObjects()
            }
            CxMenuItem {
                text: qsTr("复制")
                enabled: backend.editorViewModel && backend.editorViewModel.hasSelection
                onTriggered: backend.editorViewModel.copySelectedObjects()
            }
            CxMenuItem {
                text: qsTr("粘贴")
                enabled: backend.editorViewModel && backend.editorViewModel.hasClipboardContent
                onTriggered: backend.editorViewModel.pasteObjects()
            }
            MenuSeparator {}
            CxMenuItem {
                text: qsTr("删除选中")
                enabled: backend.editorViewModel && backend.editorViewModel.hasSelection
                onTriggered: backend.editorViewModel.deleteSelectedObjects()
            }
            CxMenuItem {
                text: qsTr("清空全部")
                onTriggered: {
                    if (backend.editorViewModel) backend.editorViewModel.clearWorkspace()
                }
            }
            MenuSeparator {}
            CxMenuItem {
                text: qsTr("克隆选中")
                enabled: backend.editorViewModel && backend.editorViewModel.hasSelection
                onTriggered: backend.editorViewModel.duplicateSelectedObjects()
            }
            CxMenuItem { text: qsTr("全选"); onTriggered: {
                if (backend.editorViewModel) backend.editorViewModel.selectAllVisibleObjects()
            } }
            CxMenuItem { text: qsTr("取消选择"); onTriggered: {
                if (backend.editorViewModel) backend.editorViewModel.clearObjectSelection()
            } }
        }

        CxMenu {
            title: qsTr("视图")
            CxMenuItem { text: qsTr("默认视图"); onTriggered: preparePage.applyFitHintIfReady() }
            MenuSeparator {}
            CxMenu {
                title: qsTr("相机预设")
                CxMenuItem { text: qsTr("俯视图 (Top)"); onTriggered: preparePage.viewport3dRef.requestViewPreset(0) }
                CxMenuItem { text: qsTr("前视图 (Front)"); onTriggered: preparePage.viewport3dRef.requestViewPreset(1) }
                CxMenuItem { text: qsTr("右视图 (Right)"); onTriggered: preparePage.viewport3dRef.requestViewPreset(2) }
                CxMenuItem { text: qsTr("等轴视图 (Iso)"); onTriggered: preparePage.viewport3dRef.requestViewPreset(3) }
            }
            MenuSeparator {}
            CxMenuItem { text: preparePage.viewport3dRef.wireframeMode ? qsTr("关闭线框模式") : qsTr("线框模式"); onTriggered: preparePage.viewport3dRef.wireframeMode = !preparePage.viewport3dRef.wireframeMode }
            MenuSeparator {}
            CxMenu {
                title: qsTr("显示标签")
                CxMenuItem {
                    text: hiddenTabs[root.pageOnline] ? qsTr("✗ 在线模型") : qsTr("✓ 在线模型")
                    onTriggered: toggleTabVisibility(root.pageOnline)
                }
                CxMenuItem {
                    text: hiddenTabs[root.pagePrepare] ? qsTr("✗ 准备") : qsTr("✓ 准备")
                    onTriggered: toggleTabVisibility(root.pagePrepare)
                }
                CxMenuItem {
                    text: hiddenTabs[root.pagePreview] ? qsTr("✗ 预览") : qsTr("✓ 预览")
                    onTriggered: toggleTabVisibility(root.pagePreview)
                }
                CxMenuItem {
                    text: hiddenTabs[root.pageDevice] ? qsTr("✗ 设备") : qsTr("✓ 设备")
                    onTriggered: toggleTabVisibility(root.pageDevice)
                }
            }
        }

        MenuSeparator {}

        CxMenuItem {
            text: qsTr("偏好设置")
            onTriggered: backend.setCurrentPage(8)
        }

        MenuSeparator {}

        CxMenu {
            title: qsTr("校准")
            CxMenuItem { text: qsTr("温度校准"); onTriggered: backend.setCurrentPage(5) }
            CxMenuItem { text: qsTr("流量校准"); onTriggered: backend.setCurrentPage(5) }
            CxMenuItem { text: qsTr("压力推进校准"); onTriggered: backend.setCurrentPage(5) }
        }

        MenuSeparator {}

        CxMenu {
            title: qsTr("帮助")
            CxMenuItem {
                text: qsTr("快捷键...")
                onTriggered: shortcutDialog.open()
            }
            MenuSeparator {}
            CxMenuItem {
                text: qsTr("关于 OWzx")
                onTriggered: aboutDialog.open()
            }
        }
    }

    // Slice dropdown menu (title bar)
    CxMenu {
        id: sliceTopMenu
        CxMenuItem {
            text: qsTr("切片当前平板")
            onTriggered: {
                if (backend.editorViewModel)
                    backend.editorViewModel.requestSlice()
            }
        }
        CxMenuItem {
            text: qsTr("切片全部平板")
            onTriggered: {
                if (backend.editorViewModel)
                    backend.editorViewModel.requestSliceAll()
            }
        }
    }

    // Print dropdown menu (title bar)
    CxMenu {
        id: printTopMenu
        CxMenuItem {
            text: qsTr("发送打印")
            onTriggered: preparePage.openPrintDialog()
        }
        CxMenuItem {
            text: qsTr("导出 G-code")
            onTriggered: preparePage.openExportDialog()
        }
    }

    // Keyboard shortcuts dialog
    Dialog {
        id: shortcutDialog
        title: qsTr("快捷键一览")
        modal: true
        anchors.centerIn: parent
        width: 480
        height: 460
        padding: 20

        ScrollView {
            anchors.fill: parent
            clip: true
            contentWidth: availableWidth
            Column {
                width: parent.width
                spacing: 4
                Repeater {
                    model: [
                        { key: "Ctrl+Z", desc: qsTr("撤销") },
                        { key: "Ctrl+Y", desc: qsTr("重做") },
                        { key: "Ctrl+X", desc: qsTr("剪切选中") },
                        { key: "Ctrl+C", desc: qsTr("复制选中") },
                        { key: "Ctrl+V", desc: qsTr("粘贴") },
                        { key: "Ctrl+D", desc: qsTr("克隆选中") },
                        { key: "Ctrl+K", desc: qsTr("克隆选中 (备)") },
                        { key: "Delete", desc: qsTr("删除选中") },
                        { key: "Escape", desc: qsTr("取消选择") },
                        { key: "Ctrl+A", desc: qsTr("全选") },
                        { key: "F", desc: qsTr("适应视图") },
                        { key: "W", desc: qsTr("移动模式") },
                        { key: "E", desc: qsTr("旋转模式") },
                        { key: "R", desc: qsTr("缩放模式") },
                        { key: "Space", desc: qsTr("播放/暂停预览动画") },
                        { key: "Ctrl+U", desc: qsTr("测量工具") },
                        { key: "Ctrl+I", desc: qsTr("导入模型") },
                        { key: "Ctrl+O", desc: qsTr("打开项目") },
                        { key: "Ctrl+S", desc: qsTr("保存项目") },
                        { key: "Ctrl+P", desc: qsTr("偏好设置") },
                        { key: "←/→", desc: qsTr("预览步进 ±100") },
                        { key: "Home/End", desc: qsTr("预览跳到头/尾") },
                        { key: "PgUp/PgDn", desc: qsTr("层范围 ±1 层") },
                        { key: "Shift+PgUp/Dn", desc: qsTr("层范围 ±10 层") },
                        { key: "Ctrl+1/3/6", desc: qsTr("预设视角 顶/右/等轴") },
                        { key: "Ctrl+0", desc: qsTr("预设视角 前视") },
                    ]
                    delegate: RowLayout {
                        width: parent.width
                        spacing: 16
                        Rectangle {
                            width: 80; height: 22; radius: 4
                            color: "#1a2233"
                            border.color: Theme.borderSubtle
                            Text {
                                anchors.centerIn: parent
                                text: modelData.key; color: Theme.accent
                                font.pixelSize: 11; font.bold: true
                            }
                        }
                        Text {
                            text: modelData.desc; color: Theme.textSecondary
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }

        footer: Rectangle {
            width: parent.width; height: 40
            color: "transparent"
            Rectangle {
                anchors.centerIn: parent
                width: 60; height: 26; radius: 6; color: Theme.accent
                Text {
                    anchors.centerIn: parent
                    text: qsTr("关闭"); color: Theme.textOnAccent
                    font.pixelSize: 12
                }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: shortcutDialog.close()
                }
            }
        }
    }

    // About dialog
    Dialog {
        id: aboutDialog
        title: qsTr("关于 OWzx")
        modal: true
        anchors.centerIn: parent
        width: 360
        height: 200
        padding: 20

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Text {
                text: "OWzx Slicer"
                color: Theme.textPrimary
                font.pixelSize: 16
                font.bold: true
            }
            Text {
                text: qsTr("基于 OrcaSlicer 开源版本")
                color: Theme.textSecondary
                font.pixelSize: 12
            }
            Text {
                text: qsTr("Qt 6.10 + QML 重写迁移版")
                color: Theme.textSecondary
                font.pixelSize: 12
            }
            Text {
                text: qsTr("上游基线: 0d4ac73a6f3224a2bf753d7b9e67d7d515bc8557")
                color: Theme.textDisabled
                font.pixelSize: 10
            }
            Item { Layout.fillHeight: true }
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 60; height: 26; radius: 6; color: Theme.accent
                Text {
                    anchors.centerIn: parent
                    text: qsTr("确定"); color: Theme.textOnAccent
                    font.pixelSize: 12
                }
                MouseArea {
                    anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: aboutDialog.close()
                }
            }
        }
    }

    // New project confirmation dialog (对齐上游 MainFrame 清空确认)
    Dialog {
        id: newProjectDialog
        title: qsTr("新建项目")
        modal: true
        anchors.centerIn: parent
        width: 360
        height: 140
        padding: 20

        Column {
            anchors.fill: parent
            spacing: 16
            Label {
                text: qsTr("将创建新项目，当前未保存的更改将丢失。\n是否继续？")
                color: Theme.textPrimary
                font.pixelSize: 13
                wrapMode: Text.Wrap
                width: parent.width
            }
            Row {
                anchors.right: parent.right
                spacing: 8
                Rectangle {
                    width: 80; height: 28; radius: 6
                    color: "#1e2a38"
                    border.color: Theme.borderSubtle
                    Text { anchors.centerIn: parent; text: qsTr("取消"); color: Theme.textSecondary; font.pixelSize: 12 }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: newProjectDialog.close() }
                }
                Rectangle {
                    width: 80; height: 28; radius: 6
                    color: Theme.accent
                    Text { anchors.centerIn: parent; text: qsTr("确定"); color: Theme.textOnAccent; font.pixelSize: 12; font.bold: true }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { backend.topbarNewProject(); newProjectDialog.close() } }
                }
            }
        }
    }

    Shortcut {
        sequences: [StandardKey.Undo]
        enabled: backend.currentPage === root.pagePrepare
        onActivated: preparePage.undoFromTopbar()
    }
    Shortcut {
        sequences: [StandardKey.Save]
        onActivated: {
            if (!backend.topbarSaveProject())
                saveProjectAsDialog.open()
        }
    }
    Shortcut {
        sequences: [StandardKey.Redo]
        enabled: backend.currentPage === root.pagePrepare
        onActivated: preparePage.redoFromTopbar()
    }
    Shortcut {
        sequence: "Ctrl+Shift+Z"
        enabled: backend.currentPage === root.pagePrepare
        onActivated: preparePage.redoFromTopbar()
    }
    Shortcut {
        sequence: "Delete"
        enabled: backend.currentPage === root.pagePrepare
                 && backend.editorViewModel
                 && backend.editorViewModel.hasSelection
        onActivated: backend.editorViewModel.deleteSelection()
    }
    Shortcut {
        sequence: "Ctrl+I"
        onActivated: openModelDialog.open()
    }
    Shortcut {
        sequence: "Ctrl+O"
        onActivated: openProjectDialog.open()
    }
    Shortcut {
        sequence: "Ctrl+X"
        enabled: backend.editorViewModel && backend.editorViewModel.hasSelection
        onActivated: backend.editorViewModel.cutSelectedObjects()
    }
    Shortcut {
        sequence: "Ctrl+C"
        enabled: backend.editorViewModel && backend.editorViewModel.hasSelection
        onActivated: backend.editorViewModel.copySelectedObjects()
    }
    Shortcut {
        sequence: "Ctrl+V"
        enabled: backend.editorViewModel && backend.editorViewModel.hasClipboardContent
        onActivated: backend.editorViewModel.pasteObjects()
    }
    Shortcut {
        sequence: "Ctrl+K"
        enabled: backend.editorViewModel && backend.editorViewModel.hasSelection
        onActivated: backend.editorViewModel.duplicateSelectedObjects()
    }

    readonly property string compareReferenceSource: backend.currentPage === 1
        ? "qrc:/qml/assets/prepare_ref.png"
        : backend.currentPage === 2
            ? "qrc:/qml/assets/preview_ref.png"
            : backend.currentPage === 3
                ? "qrc:/qml/assets/monitor_ref.png"
                : ""

    Rectangle {
        id: shell
        anchors.fill: parent
        anchors.margins: root.frameMargin
        radius: root.frameRadius
        color: backend ? backend.bgColor : "#0d0f12"
        border.color: backend.borderColor
        border.width: root.visibility === Window.Maximized ? 0 : 1
        clip: true

        Image {
            anchors.fill: parent
            visible: backend.visualCompareMode && root.compareReferenceSource !== ""
            source: root.compareReferenceSource
            fillMode: Image.Stretch
        }

        ColumnLayout {
            visible: !backend.visualCompareMode || root.compareReferenceSource === ""
            anchors.fill: parent
            spacing: 0

            Rectangle {
                id: titleBar
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                color: "#181818"

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    propagateComposedEvents: true
                    z: -1
                    onPressed: (mouse) => {
                        if (mouse.button === Qt.LeftButton && root.visibility !== Window.Maximized)
                            root.startSystemMove()
                        mouse.accepted = false
                    }
                    onDoubleClicked: {
                        if (root.visibility === Window.Maximized) root.showNormal()
                        else root.showMaximized()
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 5
                    anchors.rightMargin: 0
                    spacing: 0

                    // Logo button (对齐上游 BBLTopbar ID_LOGO)
                    Rectangle {
                        Layout.preferredWidth: 40
                        Layout.preferredHeight: 40
                        color: logoHover.containsMouse ? "#2a2a2a" : "transparent"

                        Image {
                            anchors.centerIn: parent
                            width: 20; height: 20
                            source: "qrc:/qml/assets/icons/printer.svg"
                            fillMode: Image.PreserveAspectFit
                        }

                        HoverHandler { id: logoHover }
                        TapHandler { onTapped: backend.setCurrentPage(0) }
                    }

                    TitleBarDivider { Layout.leftMargin: 5; Layout.rightMargin: 10 }

                    // File menu button (对齐上游 BBLTopbar ID_TOP_FILE_MENU)
                    Rectangle {
                        id: fileBtn
                        Layout.preferredHeight: 30
                        Layout.preferredWidth: 60
                        radius: 3
                        color: fileBtnMouse.containsMouse ? "#2a2a2a" : "transparent"

                        Row {
                            anchors.centerIn: parent
                            spacing: 4
                            Text { text: qsTr("文件"); color: "#d0d0d0"; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                            Text { text: "▾"; color: "#888"; font.pixelSize: 10; anchors.verticalCenter: parent.verticalCenter }
                        }

                        MouseArea {
                            id: fileBtnMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: fileMenu.popup()
                        }
                    }

                    // Dropdown arrow (对齐上游 BBLTopbar ID_TOP_DROPDOWN_MENU)
                    Rectangle {
                        id: dropBtn
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 30
                        radius: 3
                        color: dropBtnMouse.containsMouse ? "#2a2a2a" : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: "▾"
                            color: "#888"
                            font.pixelSize: 10
                        }

                        MouseArea {
                            id: dropBtnMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: topMenu.popup()
                        }
                    }

                    TitleBarDivider { Layout.leftMargin: 5; Layout.rightMargin: 5 }

                    // Save button (对齐上游 BBLTopbar wxID_SAVE)
                    CxIconButton {
                        cxStyle: CxIconButton.Style.Chrome
                        buttonSize: 30
                        iconSize: 16
                        iconSource: "qrc:/qml/assets/icons/device-floppy.svg"
                        toolTipText: qsTr("保存项目")
                        onClicked: {
                            if (!backend.topbarSaveProject())
                                saveProjectAsDialog.open()
                        }
                    }

                    // Undo (对齐上游 BBLTopbar wxID_UNDO)
                    CxIconButton {
                        cxStyle: CxIconButton.Style.Chrome
                        buttonSize: 30
                        iconSize: 16
                        iconSource: "qrc:/qml/assets/icons/arrow-back-up.svg"
                        toolTipText: qsTr("撤销")
                        enabled: backend.currentPage === root.pagePrepare
                        onClicked: if (backend.currentPage === root.pagePrepare) preparePage.undoFromTopbar()
                    }

                    // Redo (对齐上游 BBLTopbar wxID_REDO)
                    CxIconButton {
                        cxStyle: CxIconButton.Style.Chrome
                        buttonSize: 30
                        iconSize: 16
                        iconSource: "qrc:/qml/assets/icons/arrow-forward-up.svg"
                        toolTipText: qsTr("重做")
                        enabled: backend.currentPage === root.pagePrepare
                        onClicked: if (backend.currentPage === root.pagePrepare) preparePage.redoFromTopbar()
                    }

                    // Stretch spacer (pushes tabs toward center)
                    Item { Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 20 }

                    // Tab navigation (对齐上游 BBLTopbar ButtonsCtrl)
                    RowLayout {
                        Layout.alignment: Qt.AlignVCenter
                        spacing: 2

                        Repeater {
                            model: root.workflowTabs
                            delegate: Rectangle {
                                required property var modelData
                                Layout.preferredHeight: 30
                                Layout.preferredWidth: Math.max(100, tabText.implicitWidth + 24)
                                radius: 3
                                color: {
                                    if (backend.currentPage === modelData.page) return "#15C963"
                                    if (tabHover.containsMouse) return "#4CD582"
                                    return "transparent"
                                }

                                Text {
                                    id: tabText
                                    anchors.centerIn: parent
                                    text: modelData.label
                                    color: backend.currentPage === modelData.page ? "#ffffff" : "#c0c0c0"
                                    font.pixelSize: 12
                                    font.bold: backend.currentPage === modelData.page
                                }

                                HoverHandler { id: tabHover }
                                TapHandler {
                                    onTapped: {
                                        if (backend.currentPage === modelData.page) return
                                        root.pendingSwitchToken = backend.beginLatency("tab-switch", modelData.label)
                                        root.pendingSwitchTargetPage = modelData.page
                                        backend.setCurrentPage(modelData.page)
                                    }
                                }
                            }
                        }
                    }

                    // Stretch spacer
                    Item { Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 20 }

                    // Title label (对齐上游 BBLTopbar ID_TITLE)
                    Text {
                        id: projectTitleLabel
                        Layout.alignment: Qt.AlignVCenter
                        Layout.maximumWidth: Math.max(160, (root.width / 2) - 500)
                        Layout.minimumWidth: 160
                        text: backend.displayProjectTitle
                        color: "#c0c0c0"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        ToolTip.visible: titleHover.containsMouse
                        ToolTip.text: text
                        HoverHandler { id: titleHover }
                    }

                    TitleBarDivider { Layout.leftMargin: 10; Layout.rightMargin: 10 }

                    // Bell icon (notification center)
                    Item {
                        Layout.alignment: Qt.AlignVCenter
                        width: 30; height: 30

                        CxIconButton {
                            id: bellButton
                            cxStyle: CxIconButton.Style.Chrome
                            buttonSize: 30
                            iconSize: 14
                            iconSource: "qrc:/qml/assets/icons/bell.svg"
                            toolTipText: qsTr("通知中心")
                            onClicked: notificationCenterPopup.open()
                        }

                        Rectangle {
                            visible: backend.unreadHistoryCount > 0
                            anchors.top: bellButton.top; anchors.topMargin: 2
                            anchors.left: bellButton.left; anchors.leftMargin: 18
                            width: 8; height: 8; radius: 4
                            color: "#f05545"
                            z: 1
                        }

                        Popup {
                            id: notificationCenterPopup
                            x: bellButton.x + bellButton.width - 320
                            y: bellButton.y + bellButton.height + 4
                            width: 320
                            height: 420
                            padding: 0
                            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                            background: Rectangle { color: "transparent"; border.width: 0 }

                            NotificationCenter {
                                anchors.fill: parent
                                onCloseRequested: notificationCenterPopup.close()
                            }
                        }
                    }

                    // Window controls
                    CxIconButton {
                        cxStyle: CxIconButton.Style.Chrome
                        buttonSize: 30; iconSize: 14
                        iconSource: "qrc:/qml/assets/icons/minus.svg"
                        toolTipText: qsTr("最小化")
                        onClicked: root.showMinimized()
                    }

                    CxIconButton {
                        cxStyle: CxIconButton.Style.Chrome
                        buttonSize: 30; iconSize: 14
                        iconSource: root.visibility === Window.Maximized
                            ? "qrc:/qml/assets/icons/restore.svg"
                            : "qrc:/qml/assets/icons/maximize.svg"
                        toolTipText: root.visibility === Window.Maximized ? qsTr("还原") : qsTr("最大化")
                        onClicked: root.visibility === Window.Maximized ? root.showNormal() : root.showMaximized()
                    }

                    CxIconButton {
                        cxStyle: CxIconButton.Style.ChromeDanger
                        buttonSize: 30; iconSize: 14
                        iconSource: "qrc:/qml/assets/icons/x.svg"
                        toolTipText: qsTr("关闭")
                        onClicked: Qt.quit()
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: "#303030"
                }
            }

            // Warning-level error banner (severity=1)
            ErrorBanner { }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: backend.currentPage

                // Page 0 — Home (Loader: lazy to avoid Qt6 QVariantList/V4 crash during static init)
                Loader {
                    active: backend.currentPage === 0
                    sourceComponent: Component {
                        HomePage { homeVm: backend.homeViewModel }
                    }
                }
                // Page 1 — Prepare (3D editor)
                PreparePage {
                    id: preparePage
                    editorVm: backend.editorViewModel
                    configVm: backend.configViewModel
                }
                // Page 2 — Preview (G-code)
                PreviewPage {
                    previewVm: backend.previewViewModel
                }
                // Page 3 — Monitor
                MonitorPage {
                    monitorVm: backend.monitorViewModel
                }
                // Page 4 — Project (Loader: fileTree is QVariantList)
                Loader {
                    active: backend.currentPage === 4
                    sourceComponent: Component {
                        ProjectPage { projectVm: backend.projectViewModel }
                    }
                }
                // Page 5 — Calibration (Loader: lazy)
                Loader {
                    active: backend.currentPage === 5
                    sourceComponent: Component {
                        CalibrationPage { calibrationVm: backend.calibrationViewModel }
                    }
                }
                // Page 6 — Auxiliary
                AuxiliaryPage {
                    projectVm: backend.projectViewModel
                }
                // Page 7 — Device List (Loader: lazy)
                Loader {
                    active: backend.currentPage === 7
                    sourceComponent: Component {
                        DeviceListPage { monitorVm: backend.monitorViewModel }
                    }
                }
                // Page 8 — Preferences (Loader: lazy)
                Loader {
                    active: backend.currentPage === 8
                    sourceComponent: Component {
                        PreferencesPage {
                            settingsVm: backend.settingsViewModel
                            backend: backend
                        }
                    }
                }
                // Page 9 — Model Mall (Loader: lazy)
                Loader {
                    active: backend.currentPage === 9
                    sourceComponent: Component {
                        ModelMallPage { modelMallVm: backend.modelMallViewModel }
                    }
                }
                // Page 10 — Multi-machine (Loader: lazy)
                Loader {
                    active: backend.currentPage === 10
                    sourceComponent: Component {
                        MultiMachinePage { multiMachineVm: backend.multiMachineViewModel }
                    }
                }
                // Page 11 — Settings (full-screen parameter editor)
                Loader {
                    active: backend.currentPage === 11
                    sourceComponent: Component {
                        SettingsPage { configVm: backend.configViewModel }
                    }
                }
            }

            // Status bar
            StatusBar {
                Layout.fillWidth: true
                statusText: "就绪  |  Qt 6.10  |  页面 " + (backend.currentPage + 1) + "  |  " + backend.latencyBrief
            }
        }

        // Floating Info toast (severity=0), z-stacked over shell content
        ErrorToast { }
    }

    // P8.1 — First-run ConfigWizard (auto-shows on first launch)
    ConfigWizardDialog {
        id: configWizardDialog
        onWizardFinished: {
            // Wizard completed successfully; selections saved to BackendContext
        }
    }

    // Auto-show config wizard on first launch
    Connections {
        target: backend
        function onShowConfigWizardRequested() { configWizardDialog.open() }
        function onShowBedShapeDialogRequested() { bedShapeDialog.open() }
        function onShowEditGCodeDialogRequested(key, value) {
            editGCodeDialog.dialogTitle = qsTr("编辑自定义 G-code (%1)").arg(key || "")
            editGCodeDialog.initialGCode = value || ""
            editGCodeDialog.open()
        }
        function onShowAMSSettingsDialogRequested() { amsSettingsDialog.open() }
        function onShowFirmwareDialogRequested() { firmwareDialog.open() }
        function onShowSpeedLimitDialogRequested() { speedLimitDialog.open() }
        function onShowWipeTowerDialogRequested() { wipeTowerDialog.open() }
        function onShowPrintHostDialogRequested() { printHostDialog.open() }
        function onShowPluginManagerDialogRequested() { pluginManagerDialog.open() }
        function onShowEnableLiteModeDialogRequested() { enableLiteModeDialog.open() }
    }

    // P8.2 — Bed shape dialog
    BedShapeDialog {
        id: bedShapeDialog
        editorVm: backend.editorViewModel
    }

    // P8.3 — G-code editor dialog
    EditGCodeDialog {
        id: editGCodeDialog
        onGcodeAccepted: function(gcode) {
            // Future: forward edited G-code to ConfigViewModel / PresetService
        }
    }

    // P8.4 — AMS settings dialog
    AMSSettingsDialog {
        id: amsSettingsDialog
    }

    // P8.5 — Firmware dialog
    FirmwareDialog {
        id: firmwareDialog
    }

    // P8.6a — Speed limit dialog
    SpeedLimitDialog {
        id: speedLimitDialog
    }

    // P8.6b — Wipe tower dialog
    WipeTowerDialog {
        id: wipeTowerDialog
    }

    // P8.6c — Print host dialog
    PrintHostDialog {
        id: printHostDialog
    }

    // P10.1 — Plugin manager dialog
    PluginManagerDialog {
        id: pluginManagerDialog
    }

    // P10.2 — Enable lite mode dialog
    EnableLiteModeDialog {
        id: enableLiteModeDialog
    }
    Component.onCompleted: {
        if (!backend.configWizardCompleted) {
            configWizardDialog.open()
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: 0
        z: 999
        visible: root.visibility !== Window.Maximized && !backend.visualCompareMode

        MouseArea {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: root.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.LeftEdge)
            }
        }

        MouseArea {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: root.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeHorCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.RightEdge)
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: root.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeVerCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.TopEdge)
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: root.resizeMargin
            hoverEnabled: true
            cursorShape: Qt.SizeVerCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.BottomEdge)
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.top: parent.top
            width: root.resizeMargin + 4
            height: root.resizeMargin + 4
            hoverEnabled: true
            cursorShape: Qt.SizeFDiagCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.TopEdge | Qt.LeftEdge)
            }
        }

        MouseArea {
            anchors.right: parent.right
            anchors.top: parent.top
            width: root.resizeMargin + 4
            height: root.resizeMargin + 4
            hoverEnabled: true
            cursorShape: Qt.SizeBDiagCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.TopEdge | Qt.RightEdge)
            }
        }

        MouseArea {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: root.resizeMargin + 4
            height: root.resizeMargin + 4
            hoverEnabled: true
            cursorShape: Qt.SizeBDiagCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.BottomEdge | Qt.LeftEdge)
            }
        }

        MouseArea {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: root.resizeMargin + 4
            height: root.resizeMargin + 4
            hoverEnabled: true
            cursorShape: Qt.SizeFDiagCursor
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.startSystemResize(Qt.BottomEdge | Qt.RightEdge)
            }
        }
    }

}
