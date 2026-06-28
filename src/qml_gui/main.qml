// main.qml — OWzx Slicer slim ApplicationWindow shell
//
// 顶层框架对齐：third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.cpp + BBLTopbar.cpp
// - BBLTopbar.qml 承载完整标题栏（Plan 02-02 Task 1）
// - 9-page StackLayout 与 backend.TabPosition 枚举 0..8 一一对应（ARCH-01）
// - 所有页面索引通过 backend.TabPosition.tpX 引用，禁止硬编码整数（Pitfall 1）

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
    flags: owzxUseFramelessShell ? (Qt.Window | Qt.FramelessWindowHint) : Qt.Window
    minimumWidth: 1100
    minimumHeight: 700

    readonly property int resizeMargin: 6
    readonly property int frameMargin: (!owzxUseFramelessShell || backend.visualCompareMode || root.visibility === Window.Maximized) ? 0 : 10
    readonly property int frameRadius: (!owzxUseFramelessShell || backend.visualCompareMode || root.visibility === Window.Maximized) ? 0 : 18

    // 当前 tab-switch latency token (BBLTopbar 写入, Connections onCurrentPageChanged 收尾)
    // 替代旧的 pendingSwitchToken / pendingSwitchTargetPage（Plan 02-02 Pitfall 3 迁移）
    property int activeTabSwitchToken: -1

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

    FileDialog {
        id: exportModelDialog
        title: qsTr("导出模型")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("STL (*.stl)"), qsTr("3MF (*.3mf)"), qsTr("OBJ (*.obj)")]
        defaultSuffix: "stl"
        onAccepted: {
            var path = selectedFile.toString().replace("file:///", "")
            var ext = path.split(".").pop().toLowerCase()
            if (backend.editorViewModel)
                backend.editorViewModel.exportModel(path, ext)
        }
    }

    // Latency 跟踪迁移：endLatency 在 currentPage 改变后触发
    // (替代旧 onFrameSwapped + pendingSwitchTargetPage 逻辑 — Pitfall 3)
    FolderDialog {
        id: exportAllGcodeDialog
        title: qsTr("导出全部平板 G-code")
        onAccepted: {
            if (backend.editorViewModel)
                backend.editorViewModel.requestExportAllGCode(selectedFolder.toString())
        }
    }

    Connections {
        target: backend
        function onCurrentPageChanged() {
            if (root.activeTabSwitchToken >= 0) {
                backend.endLatency(root.activeTabSwitchToken)
                root.activeTabSwitchToken = -1
            }
        }
        function onExportGCodeRequested() {
            plater.preparePageRef.openExportDialog()
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

    // New project confirmation dialog
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
        enabled: backend.currentPage === backend.tp3DEditor
            onActivated: plater.preparePageRef.undoFromTopbar()
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
        enabled: backend.currentPage === backend.tp3DEditor
        onActivated: plater.preparePageRef.redoFromTopbar()
    }
    Shortcut {
        sequence: "Ctrl+Shift+Z"
        enabled: backend.currentPage === backend.tp3DEditor
        onActivated: plater.preparePageRef.redoFromTopbar()
    }
    Shortcut {
        sequence: "Delete"
        enabled: backend.currentPage === backend.tp3DEditor
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

    // Visual compare reference PNG 映射 — 使用 TabPosition 枚举替代整数（Pitfall 1）
    readonly property string compareReferenceSource: backend.currentPage === backend.tp3DEditor
        ? "qrc:/qml/assets/prepare_ref.png"
        : backend.currentPage === backend.tpPreview
            ? "qrc:/qml/assets/preview_ref.png"
            : backend.currentPage === backend.tpDevice
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

            // ── BBLTopbar 完整标题栏（替代旧的 titleBar Rectangle）──────────
            // `backend` 由 rootContext 注入（main_qml.cpp:134），BBLTopbar 内部直接读取，
            // 无需显式 property 传递（避免 required property 在构造期求值产生 undefined 误报）
            BBLTopbar {
                id: bblTopbar
                // Phase 3: preparePage 现在是 Plater 内部子组件，通过 plater.preparePageRef 访问
                preparePageRef: plater.preparePageRef
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                windowVisibility: root.visibility

                onNewProjectRequested: newProjectDialog.open()
                onOpenProjectRequested: openProjectDialog.open()
                onSaveAsRequested: saveProjectAsDialog.open()
                onImportModelRequested: function(nameFilter) {
                    openModelDialog.nameFilters = [nameFilter, qsTr("所有文件 (*)")]
                    openModelDialog.open()
                }
                onExportGcodeRequested: plater.preparePageRef.openExportDialog()
                onExportAllGcodeRequested: exportAllGcodeDialog.open()
                onExportProjectRequested: saveProjectAsDialog.open()
                onExportModelRequested: exportModelDialog.open()
                onUndoRequested: if (backend.currentPage === backend.tp3DEditor) plater.preparePageRef.undoFromTopbar()
                onRedoRequested: if (backend.currentPage === backend.tp3DEditor) plater.preparePageRef.redoFromTopbar()
                onCalibrationRequested: backend.requestSelectTab(backend.tpCalibration)
                onPreferencesRequested: backend.openSettings()
                onAboutRequested: aboutDialog.open()
                onShortcutOverviewRequested: shortcutDialog.open()
                onSliceRequested: sliceTopMenuExternal.popup()
                onPrintRequested: printTopMenuExternal.popup()
                onBellClicked: notificationCenterPopup.open()
                onWindowMinimizeRequested: root.showMinimized()
                onWindowMaximizeRequested: root.visibility === Window.Maximized ? root.showNormal() : root.showMaximized()
                onWindowCloseRequested: Qt.quit()
                onTitleBarDragStarted: if (root.visibility !== Window.Maximized) root.startSystemMove()
                onTitleBarDoubleClicked: root.visibility === Window.Maximized ? root.showNormal() : root.showMaximized()

                // BBLTopbar 内部点击 tab 时通过 beginLatency 写入 lastTabSwitchToken，
                // 这里同步到 root.activeTabSwitchToken 以便 Connections onCurrentPageChanged 收尾
                onLastTabSwitchTokenChanged: {
                    if (bblTopbar.lastTabSwitchToken >= 0) {
                        root.activeTabSwitchToken = bblTopbar.lastTabSwitchToken
                    }
                }
            }

            // Slice 下拉菜单（外部 owned by main.qml, BBLTopbar 触发 popup）
            CxMenu {
                id: sliceTopMenuExternal
                CxMenuItem {
                    text: qsTr("切片当前平板")
                    onTriggered: if (backend.editorViewModel) backend.editorViewModel.requestSlice()
                }
                CxMenuItem {
                    text: qsTr("切片全部平板")
                    onTriggered: if (backend.editorViewModel) backend.editorViewModel.requestSliceAll()
                }
            }

            // Print 下拉菜单
            CxMenu {
                id: printTopMenuExternal
                CxMenuItem {
                    text: qsTr("发送打印")
                    onTriggered: plater.preparePageRef.openPrintDialog()
                }
                CxMenuItem {
                    text: qsTr("导出 G-code")
                    onTriggered: plater.preparePageRef.openExportDialog()
                }
            }

            // Notification center popup（生命周期归 main.qml 管理 — BBLTopbar 仅 emit bellClicked）
            Popup {
                id: notificationCenterPopup
                x: root.width - 340
                y: 44
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

            // Warning-level error banner (severity=1)
            ErrorBanner { }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                // Phase 3: Plater 单实例共享 —— tp3DEditor 和 tpPreview 都映射到 slot 1（Plater）。
                // viewMode 由 BackendContext::requestSelectTab 联动（tpPreview→Preview, tp3DEditor→View3D），
                // Plater 内部通过 viewMode 切 PreparePage/PreviewPage 可见性。
                // 上游契约：third_party/OrcaSlicer/src/slic3r/GUI/Plater.cpp 单一 wxPanel 实例
                // 被 Prepare/Preview tab 共享（ARCH-05）。
                currentIndex: (backend.currentPage === backend.tp3DEditor
                               || backend.currentPage === backend.tpPreview)
                              ? backend.tp3DEditor   // 两 tab 都落到 slot 1 (Plater)
                              : backend.currentPage

                // Page 0 (tpHome) — Home
                Loader {
                    active: backend.currentPage === backend.tpHome
                    sourceComponent: Component {
                        HomePage { homeVm: backend.homeViewModel }
                    }
                }
                // Page 1 (tp3DEditor) + Page 2 (tpPreview) — Plater 单实例共享
                // PreparePage 和 PreviewPage 作为 Plater 内部常驻子组件，由 viewMode 切可见性（ARCH-05/06/07）。
                Plater {
                    id: plater
                    editorVm: backend.editorViewModel
                    previewVm: backend.previewViewModel
                    configVm: backend.configViewModel
                    viewMode: backend.currentViewMode
                    // Phase 4: sidebar dockable 三态绑定 backend (统一管理 + 持久化)
                    sidebarCollapsed: backend.sidebarCollapsed
                    sidebarWidth: backend.sidebarWidth
                    sidebarMinWidth: backend.sidebarMinWidth
                    sidebarMaxWidth: backend.sidebarMaxWidth
                    sidebarDockArea: backend.sidebarDockArea
                    sidebarToggleRequested: function() { backend.requestToggleSidebar() }
                    sidebarWidthChanged: function(w) { backend.requestSetSidebarWidth(w) }
                    // Phase 4 (Sidebar Dockable) 会在此接 BBLTopbar 折叠按钮；当前默认展开
                    leftPanelVisible: true
                }
                // Page 2 slot 占位 —— 实际内容由 slot 1 的 Plater 承载（viewMode=Preview 时显示）
                // 此 Item 仅占位以满足 StackLayout 9-slot 结构，不可见（visible 绑定 false）。
                Item {
                    visible: false
                    anchors.fill: parent
                }
                // Page 3 (tpDevice) — Monitor
                MonitorPage {
                    monitorVm: backend.monitorViewModel
                }
                // Page 4 (tpMultiDevice) — Multi-machine
                Loader {
                    active: backend.currentPage === backend.tpMultiDevice
                    sourceComponent: Component {
                        MultiMachinePage { multiMachineVm: backend.multiMachineViewModel }
                    }
                }
                // Page 5 (tpProject) — Project
                Loader {
                    active: backend.currentPage === backend.tpProject
                    sourceComponent: Component {
                        ProjectPage { projectVm: backend.projectViewModel; editorVm: backend.editorViewModel }
                    }
                }
                // Page 6 (tpCalibration) — Calibration
                Loader {
                    active: backend.currentPage === backend.tpCalibration
                    sourceComponent: Component {
                        CalibrationPage { calibrationVm: backend.calibrationViewModel }
                    }
                }
                // Page 7 (tpPlaceholder1) — AuxiliaryPage（v2.3 UI-02 挂载，原占位替换）
                Loader {
                    active: backend.currentPage === backend.tpPlaceholder1
                    sourceComponent: Component {
                        AuxiliaryPage {
                            projectVm: backend.projectViewModel
                            backend: backend
                        }
                    }
                }
                // Page 8 (tpPlaceholder2) — reserved by upstream debug tooling; not exposed in navigation.
                Item {
                    visible: false
                }
            }

            // Status bar — 显示 "9" 页数（之前是 12）
            StatusBar {
                Layout.fillWidth: true
                statusText: "就绪  |  Qt 6.10  |  页面 " + (backend.currentPage + 1) + " / 9  |  " + backend.latencyBrief
            }
        }

        // Floating Info toast (severity=0), z-stacked over shell content
        ErrorToast { }
    }

    // P8.1 — First-run ConfigWizard
    ConfigWizardDialog {
        id: configWizardDialog
        onWizardFinished: {
            // Wizard completed successfully; selections saved to BackendContext
        }
    }

    // Dialog request signals
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
    AMSSettingsDialog { id: amsSettingsDialog }

    // P8.5 — Firmware dialog
    FirmwareDialog { id: firmwareDialog }

    // P8.6a — Speed limit dialog
    SpeedLimitDialog { id: speedLimitDialog }

    // P8.6b — Wipe tower dialog
    WipeTowerDialog { id: wipeTowerDialog }

    // P8.6c — Print host dialog
    PrintHostDialog { id: printHostDialog }

    // P10.1 — Plugin manager dialog
    PluginManagerDialog { id: pluginManagerDialog }

    // P10.2 — Enable lite mode dialog
    EnableLiteModeDialog { id: enableLiteModeDialog }

    Component.onCompleted: {
        if (!backend.configWizardCompleted) {
            configWizardDialog.open()
        }
    }

    // Resize borders (8 个边界 MouseArea，对齐 frameless window 体验)
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
