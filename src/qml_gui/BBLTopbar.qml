// BBLTopbar.qml — OWzx Slicer top strip component
//
// 上游对齐：third_party/OrcaSlicer/src/slic3r/GUI/BBLTopbar.cpp
//   - CUSTOM_ID enum (lines 29-41): ID_LOGO, ID_TOP_FILE_MENU, ID_TOP_DROPDOWN_MENU,
//     ID_TITLE, ID_MODEL_STORE, ID_PUBLISH, ID_CALIB
//   - CenteredTitle pattern (lines 43-94): project name centered horizontally with ellipsizing
//
// 本组件为 main.qml 顶层 ApplicationWindow 内的标题栏区域（不是独立 Window）。
// 它包含：Logo + [File ▾] + [▾] + Save/Undo/Redo/Calibration + 占位按钮 + 9-tab TabBar
//         + side_tools(Slice/Print/FilamentGroupPopup 占位) + CenteredTitle + Bell + 窗口控制
//
// 所有 Tab 切换通过 backend.requestSelectTab(backend.TabPosition.tpX)（Plan 02-01 提供）。
// 所有页面索引通过 backend.TabPosition 枚举引用——禁止硬编码整数（Pitfall 1）。

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "controls"
import "dialogs"
import "."

Item {
    id: root

    // ── 外部依赖 ──────────────────────────────────────────────────────────
    // `backend` 是 rootContext 的 context property（main_qml.cpp:134 注入），
    // 因此 BBLTopbar 内部直接通过 `backend` 引用，无需重复声明为 required property
    // （QML required property 在构造时绑定求值会先于 main.qml 的属性赋值，导致 undefined 误报）。
    property var preparePageRef: null        // PreparePage reference (undo/redo dispatch)
    property var previewPageRef: null        // PreviewPage reference (VIEW-01 preview-canvas viewport routing)
    property int windowVisibility: Window.Windowed  // 用于 Maximize/Restore 图标切换

    // ── BBLTopbar 对外发射的信号（main.qml 监听并 dispatch） ──────────────
    signal newProjectRequested()
    signal openProjectRequested(string filePath)
    signal saveAsRequested()
    signal importModelRequested(string nameFilter)
    signal exportGcodeRequested()
    signal exportAllGcodeRequested()
    signal exportProjectRequested()
    signal exportModelRequested()
    signal undoRequested()
    signal redoRequested()
    signal calibrationRequested()
    signal sliceRequested()
    signal sliceSinglePlateRequested()
    signal printRequested()
    // Phase 237 (VIEW-02): Edit-menu delete-all confirm (upstream Ctrl+D
    // "Delete all", GLCanvas3D.cpp:3210-3213 -> EVT_GLTOOLBAR_DELETE_ALL).
    signal deleteAllRequested()
    // Phase 237 (VIEW-02): File-menu entries (upstream MainFrame.cpp:2327
    // -2332 import_zip_archive / load_config_file, and the export-sliced-file
    // entry backing Ctrl+G "Export plate sliced file",
    // KBShortcutsDialog.cpp:182).
    signal importZipRequested()
    signal importConfigsRequested()
    signal exportSlicedFileRequested()
    signal bellClicked()
    signal windowMinimizeRequested()
    signal windowMaximizeRequested()
    signal windowCloseRequested()
    signal titleBarDragStarted()
    signal titleBarDoubleClicked()

    // ── 内部状态：当前 tab-switch latency token（main.qml 读取用于 onCurrentPageChanged 收尾）
    property int lastTabSwitchToken: -1

    readonly property var workflowTabs: [
        { label: qsTr("首页"), icon: "qrc:/qml/assets/icons/box.svg", pos: backend.tpHome },
        { label: qsTr("准备"), icon: "qrc:/qml/assets/icons/box.svg", pos: backend.tp3DEditor },
        { label: qsTr("预览"), icon: "qrc:/qml/assets/icons/layers.svg", pos: backend.tpPreview },
        { label: qsTr("设备"), icon: "qrc:/qml/assets/icons/printer.svg", pos: backend.tpDevice },
        { label: qsTr("多设备"), icon: "qrc:/qml/assets/icons/printer.svg", pos: backend.tpMultiDevice },
        { label: qsTr("项目"), icon: "qrc:/qml/assets/icons/device-floppy.svg", pos: backend.tpProject },
        { label: qsTr("校准"), icon: "qrc:/qml/assets/icons/settings.svg", pos: backend.tpCalibration },
        { label: qsTr("偏好设置"), icon: "qrc:/qml/assets/icons/settings.svg", pos: backend.tpPreferences }
    ]

    function selectWorkflowTab(tab) {
        if (!tab || backend.currentPage === tab.pos)
            return
        root.lastTabSwitchToken = backend.beginLatency("tab-switch", tab.label)
        backend.requestSelectTab(tab.pos)
    }

    // ── Phase 237 (VIEW-01): View-menu camera routing ────────────────────
    // Upstream routes select_view() to the CURRENT canvas
    // (MainFrame.cpp:3455 -> Plater::select_view -> current canvas3D). The
    // Preview tab hosts its own RhiViewport inside PreviewPage, so the
    // active-viewport resolution follows backend.currentPage.
    readonly property var activeViewport: {
        if (backend.currentPage === backend.tpPreview && root.previewPageRef)
            return root.previewPageRef.previewViewportRef
        if (root.preparePageRef)
            return root.preparePageRef.viewport3dRef
        return null
    }

    function selectViewOnActiveViewport(direction) {
        if (root.activeViewport)
            root.activeViewport.selectView(direction)
    }

    implicitHeight: 70

    // macOS 系统菜单栏（TOPBAR-07）：仅 macOS 激活，Windows/Linux 保持 inactive
    Loader {
        id: macOSMenuBarLoader
        active: Qt.platform.os === "osx"
        anchors.fill: parent
        sourceComponent: MenuBar {
            // TODO(cross-platform): full macOS MenuBar content validated when cross-platform build supported
            Menu {
                title: qsTr("文件")
                MenuItem { text: qsTr("新建项目"); onTriggered: root.newProjectRequested() }
                MenuItem { text: qsTr("打开项目..."); onTriggered: root.openProjectRequested("") }
                MenuItem { text: qsTr("保存项目"); onTriggered: if (!backend.topbarSaveProject()) root.saveAsRequested() }
                MenuItem { text: qsTr("退出"); onTriggered: Qt.quit() }
            }
            Menu {
                title: qsTr("编辑")
                MenuItem { text: qsTr("撤销"); onTriggered: root.undoRequested() }
                MenuItem { text: qsTr("重做"); onTriggered: root.redoRequested() }
            }
            Menu {
                title: qsTr("帮助")
                MenuItem { text: qsTr("关于 OWzx"); onTriggered: root.aboutRequested() }
            }
        }
    }

    Rectangle {
        id: topbarBackground
        anchors.fill: parent
        color: Theme.chromeSurface

        // Title bar drag for frameless window (与上游 BBLTopbar forwardMouseEvent 等价)
        MouseArea {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 36
            acceptedButtons: Qt.LeftButton
            propagateComposedEvents: true
            z: -1
            onPressed: (mouse) => {
                if (mouse.button === Qt.LeftButton)
                    root.titleBarDragStarted()
                mouse.accepted = false
            }
            onDoubleClicked: root.titleBarDoubleClicked()
        }

        RowLayout {
            id: titleToolRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 36
            anchors.leftMargin: 5
            anchors.rightMargin: 0
            spacing: 0

            // ── LEFT GROUP ───────────────────────────────────────────────
            // Logo (对齐上游 ID_LOGO)
            Rectangle {
                Layout.preferredWidth: 36
                Layout.preferredHeight: 32
                color: logoHover.containsMouse ? Theme.chromeHover : "transparent"

                Image {
                    anchors.centerIn: parent
                    width: 20; height: 20
                    source: "qrc:/qml/assets/icons/printer.svg"
                    fillMode: Image.PreserveAspectFit
                }
                HoverHandler { id: logoHover }
                TapHandler {
                    onTapped: backend.requestSelectTab(backend.tpHome)
                }
            }

            TitleBarDivider { Layout.leftMargin: 5; Layout.rightMargin: 10 }

            // [File ▾] 按钮 (对齐上游 ID_TOP_FILE_MENU)
            Rectangle {
                id: fileBtn
                Layout.preferredHeight: 30
                Layout.preferredWidth: 60
                radius: 3
                color: fileBtnMouse.containsMouse ? Theme.chromeHover : "transparent"

                Row {
                    anchors.centerIn: parent
                    spacing: 4
                    Text { text: qsTr("文件"); color: Theme.chromeText; font.pixelSize: Theme.fontSizeMD; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "▾"; color: Theme.chromeTextMuted; font.pixelSize: Theme.fontSizeXS; anchors.verticalCenter: parent.verticalCenter }
                }

                MouseArea {
                    id: fileBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: fileMenu.popup(fileBtn, 0, fileBtn.height)
                }
            }

            // [▾] 二级菜单 (对齐上游 ID_TOP_DROPDOWN_MENU)
            Rectangle {
                id: dropBtn
                Layout.preferredWidth: 24
                Layout.preferredHeight: 30
                radius: 3
                color: dropBtnMouse.containsMouse ? Theme.chromeHover : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "▾"
                    color: Theme.chromeTextMuted
                    font.pixelSize: Theme.fontSizeXS
                }

                MouseArea {
                    id: dropBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: topMenu.popup(dropBtn, 0, dropBtn.height)
                }
            }

            TitleBarDivider { Layout.leftMargin: 5; Layout.rightMargin: 5 }

            // Save 按钮 (对齐上游 wxID_SAVE) — Phase 51 SHELL-03: canSave gate (disabled while slicing)
            CxIconButton {
                cxStyle: CxIconButton.Style.Chrome
                buttonSize: 30
                iconSize: 16
                iconSource: "qrc:/qml/assets/icons/device-floppy.svg"
                toolTipText: qsTr("保存项目")
                enabled: backend.canSave
                onClicked: {
                    if (backend.canSave && !backend.topbarSaveProject())
                        root.saveAsRequested()
                }
            }

            // Undo (对齐上游 wxID_UNDO) — Phase 51 SHELL-03: gate on BOTH page AND canUndo (undo stack non-empty)
            CxIconButton {
                cxStyle: CxIconButton.Style.Chrome
                buttonSize: 30
                iconSize: 16
                iconSource: "qrc:/qml/assets/icons/arrow-back-up.svg"
                toolTipText: qsTr("撤销")
                enabled: backend.currentPage === backend.tp3DEditor && backend.canUndo
                onClicked: if (backend.currentPage === backend.tp3DEditor && backend.canUndo) root.undoRequested()
            }

            // Redo (对齐上游 wxID_REDO) — Phase 51 SHELL-03: gate on BOTH page AND canRedo (redo stack non-empty)
            CxIconButton {
                cxStyle: CxIconButton.Style.Chrome
                buttonSize: 30
                iconSize: 16
                iconSource: "qrc:/qml/assets/icons/arrow-forward-up.svg"
                toolTipText: qsTr("重做")
                enabled: backend.currentPage === backend.tp3DEditor && backend.canRedo
                onClicked: if (backend.currentPage === backend.tp3DEditor && backend.canRedo) root.redoRequested()
            }

            // Calibration 快捷按钮 (对齐上游 ID_CALIB)
            CxIconButton {
                cxStyle: CxIconButton.Style.Chrome
                buttonSize: 30
                iconSize: 16
                iconSource: "qrc:/qml/assets/icons/settings.svg"
                toolTipText: qsTr("校准")
                onClicked: root.calibrationRequested()
            }

            // Account 占位按钮 (对齐上游 条件按钮，v2.0 仅占位 — CONTEXT.md 决策)
            CxIconButton {
                cxStyle: CxIconButton.Style.Chrome
                buttonSize: 30
                iconSize: 16
                iconSource: "qrc:/qml/assets/icons/printer.svg"
                toolTipText: ""
                enabled: false
                visible: false
            }

            // ModelStore 占位按钮 (对齐上游 ID_MODEL_STORE)
            CxIconButton {
                cxStyle: CxIconButton.Style.Chrome
                buttonSize: 30
                iconSize: 16
                iconSource: "qrc:/qml/assets/icons/layout-grid.svg"
                toolTipText: ""
                enabled: false
                visible: false
            }

            // Publish 占位按钮 (对齐上游 ID_PUBLISH)
            CxIconButton {
                cxStyle: CxIconButton.Style.Chrome
                buttonSize: 30
                iconSize: 16
                iconSource: "qrc:/qml/assets/icons/send-2.svg"
                toolTipText: ""
                enabled: false
                visible: false
            }

            // Stretch spacer (pushes tabs toward center)
            Item { Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 20 }

            // ── CENTER GROUP: 9-page TabBar (对齐上游 Notebook 9 页) ─────
            TabBar {
                id: navTabBar
                Layout.alignment: Qt.AlignVCenter
                visible: false
                currentIndex: backend.currentPage

                // 反馈环保护 (Pitfall 2 in 02-RESEARCH.md)：
                // 仅当 currentIndex 与 backend.currentPage 不同时才发请求
                onCurrentIndexChanged: {
                    if (currentIndex !== backend.currentPage
                            && currentIndex >= 0 && currentIndex <= backend.tpPreferences) {
                        backend.requestSelectTab(currentIndex)
                    }
                }

                background: Rectangle { color: "transparent" }

                // 可见页签模型：标签 + TabPosition 枚举引用 + 启用状态 + 提示
                Repeater {
                    model: [
                        { label: qsTr("首页"),     pos: backend.tpHome },
                        { label: qsTr("准备"),     pos: backend.tp3DEditor },
                        { label: qsTr("预览"),     pos: backend.tpPreview },
                        { label: qsTr("设备"),     pos: backend.tpDevice },
                        { label: qsTr("多设备"),   pos: backend.tpMultiDevice },
                        { label: qsTr("项目"),     pos: backend.tpProject },
                        { label: qsTr("校准"),     pos: backend.tpCalibration }
                    ]

                    delegate: TabButton {
                        id: tabBtn
                        required property var modelData
                        text: modelData.label
                        enabled: !modelData.disabled
                        // 自定义样式以匹配现有 dark theme（Pitfall 7）
                        background: Rectangle {
                            implicitHeight: 30
                            implicitWidth: Math.max(96, tabBtnText.implicitWidth + 24)
                            radius: 3
                            color: {
                                if (!tabBtn.enabled) return "transparent"
                                if (tabBtn.checked) return Theme.accent
                                if (tabBtn.hovered) return Theme.accentLight
                                return "transparent"
                            }
                            Behavior on color { ColorAnimation { duration: 100 } }
                        }
                        contentItem: Text {
                            id: tabBtnText
                            text: tabBtn.text
                            color: !tabBtn.enabled ? Theme.textDisabled
                                  : tabBtn.checked ? Theme.textOnAccent
                                  : tabBtn.hovered ? Theme.textOnAccent
                                  : Theme.chromeText
                            font.pixelSize: Theme.fontSizeMD
                            font.bold: tabBtn.checked
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            Behavior on color { ColorAnimation { duration: 100 } }
                        }
                        ToolTip.visible: tabBtn.hovered && (tabBtn.modelData.tooltip || "").length > 0
                        ToolTip.text: tabBtn.modelData.tooltip || ""
                        ToolTip.delay: 400
                        onClicked: {
                            if (backend.currentPage === modelData.pos) return
                            // Latency 跟踪：beginLatency token 通过 lastTabSwitchToken 暴露给 main.qml
                            root.lastTabSwitchToken = backend.beginLatency("tab-switch", modelData.label)
                            // 单一派发源 (WR-01): 不在此处直接调用 requestSelectTab，避免与
                            // onCurrentIndexChanged 双发 tabSelectRequested。设置 currentIndex 触发
                            // 上方 onCurrentIndexChanged，由其统一调用 requestSelectTab。
                            navTabBar.currentIndex = modelData.pos
                        }
                    }
                }
            }

            // Stretch spacer
            Item { Layout.fillWidth: true; Layout.fillHeight: true; Layout.minimumWidth: 20 }

            // ── RIGHT GROUP: side_tools + CenteredTitle + Bell + 窗口控制 ──

            // side_tools 容器 (ARCH-04 — 对齐上游 Notebook.cpp:45-55)
            RowLayout {
                id: sideTools
                Layout.alignment: Qt.AlignVCenter
                visible: false
                spacing: 2

                // Slice 下拉 (复用 main.qml sliceTopMenu 语义) — Phase 51 SHELL-03: canSlice gate
                CxIconButton {
                    cxStyle: CxIconButton.Style.Chrome
                    buttonSize: 30
                    iconSize: 16
                    iconSource: "qrc:/qml/assets/icons/layers.svg"
                    toolTipText: qsTr("切片")
                    enabled: backend.canSlice
                    onClicked: root.sliceRequested()
                }

                // Print 下拉
                CxIconButton {
                    cxStyle: CxIconButton.Style.Chrome
                    buttonSize: 30
                    iconSize: 16
                    iconSource: "qrc:/qml/assets/icons/printer.svg"
                    toolTipText: qsTr("打印")
                    onClicked: root.printRequested()
                }

                // Phase 110 (FMAP-03): FilamentGroupPopup trigger. Opens the
                // CxPopup-based 3-mode selector (AutoForFlush / AutoForMatch /
                // Manual). fmmDefault is the per-plate inherit-sentinel and is
                // NOT surfaced as a 4th radio (anti-feature per FEATURES.md).
                CxIconButton {
                    cxStyle: CxIconButton.Style.Chrome
                    buttonSize: 30
                    iconSize: 16
                    iconSource: "qrc:/qml/assets/icons/box.svg"
                    toolTipText: qsTr("Filament Group")
                    enabled: backend.editorViewModel != null
                    onClicked: filamentGroupPopup.openForCurrentPlate()
                }
            }

            // CenteredTitle (对齐上游 BBLTopbar.cpp:43-94 CenteredTitle 类)
            Text {
                id: projectTitleLabel
                Layout.alignment: Qt.AlignVCenter
                Layout.maximumWidth: Math.max(160, (root.width / 2) - 500)
                Layout.minimumWidth: 160
                text: backend.displayProjectTitle
                color: Theme.chromeText
                font.pixelSize: Theme.fontSizeMD
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignHCenter
                HoverHandler { id: titleHover }
                ToolTip.visible: titleHover.hovered
                ToolTip.text: projectTitleLabel.text
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
                    onClicked: root.bellClicked()
                }

                Rectangle {
                    visible: backend.unreadHistoryCount > 0
                    anchors.top: bellButton.top; anchors.topMargin: 2
                    anchors.left: bellButton.left; anchors.leftMargin: 18
                    width: 8; height: 8; radius: 4
                    color: Theme.statusError
                    z: 1
                }
            }

            // ── 窗口控制 (TOPBAR-06) ─────────────────────────────────────
            CxIconButton {
                cxStyle: CxIconButton.Style.Chrome
                buttonSize: 30
                iconSize: 14
                iconSource: "qrc:/qml/assets/icons/minus.svg"
                toolTipText: qsTr("最小化")
                onClicked: root.windowMinimizeRequested()
            }

            CxIconButton {
                cxStyle: CxIconButton.Style.Chrome
                buttonSize: 30
                iconSize: 14
                iconSource: root.windowVisibility === Window.Maximized
                    ? "qrc:/qml/assets/icons/restore.svg"
                    : "qrc:/qml/assets/icons/maximize.svg"
                toolTipText: root.windowVisibility === Window.Maximized ? qsTr("还原") : qsTr("最大化")
                onClicked: root.windowMaximizeRequested()
            }

            CxIconButton {
                cxStyle: CxIconButton.Style.ChromeDanger
                buttonSize: 30
                iconSize: 14
                iconSource: "qrc:/qml/assets/icons/x.svg"
                toolTipText: qsTr("关闭")
                onClicked: root.windowCloseRequested()
            }
        }

        Rectangle {
            id: workflowBar
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.topMargin: 36
            height: 34
            color: Theme.chromeSurface

            RowLayout {
                anchors.fill: parent
                spacing: 0

                Repeater {
                    model: root.workflowTabs
                    delegate: Button {
                        id: workflowTab
                        required property var modelData
                        Layout.preferredWidth: Math.max(112, workflowLabel.implicitWidth + 48)
                        Layout.fillHeight: true
                        activeFocusOnTab: true
                        padding: 0
                        hoverEnabled: true
                        Accessible.name: workflowTab.modelData.label
                        onClicked: root.selectWorkflowTab(workflowTab.modelData)

                        Keys.onPressed: (event) => {
                            if (event.key === Qt.Key_Return
                                    || event.key === Qt.Key_Enter
                                    || event.key === Qt.Key_Space) {
                                root.selectWorkflowTab(workflowTab.modelData)
                                event.accepted = true
                            }
                        }

                        background: Rectangle {
                            color: backend.currentPage === workflowTab.modelData.pos ? Theme.accent : "transparent"
                        }

                        contentItem: Item {
                            Row {
                                anchors.centerIn: parent
                                spacing: 7

                                Image {
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 16
                                    height: 16
                                    source: workflowTab.modelData.icon
                                    opacity: backend.currentPage === workflowTab.modelData.pos ? 1.0 : 0.72
                                    fillMode: Image.PreserveAspectFit
                                }

                                Text {
                                    id: workflowLabel
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: workflowTab.modelData.label
                                    color: backend.currentPage === workflowTab.modelData.pos ? Theme.textOnAccent : Theme.chromeText
                                    font.pixelSize: Theme.fontSizeMD
                                    font.bold: backend.currentPage === workflowTab.modelData.pos
                                }
                            }
                        }
                    }
                }

                Item { Layout.fillWidth: true; Layout.fillHeight: true }

                Rectangle {
                    id: prepareSliceButton
                    property string toolTipText: backend.editorViewModel ? backend.editorViewModel.sliceActionHint : qsTr("Backend unavailable")
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 106
                    Layout.preferredHeight: 24
                    radius: 12
                    enabled: backend.editorViewModel && backend.editorViewModel.canRequestSlice
                    color: enabled ? Theme.borderActive : Theme.borderInput
                    opacity: backend.currentPage === backend.tp3DEditor ? 1.0 : 0.0
                    visible: backend.currentPage === backend.tp3DEditor
                    ToolTip.visible: sliceMouse.containsMouse && prepareSliceButton.toolTipText.length > 0
                    ToolTip.text: prepareSliceButton.toolTipText
                    ToolTip.delay: 400

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("切片单盘")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                        opacity: prepareSliceButton.enabled ? 1.0 : 0.55
                    }

                    MouseArea {
                        id: sliceMouse
                        anchors.fill: parent
                        enabled: parent.enabled
                        hoverEnabled: true
                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: root.sliceSinglePlateRequested()
                    }
                }

                Item { width: 8; height: 1 }

                Rectangle {
                    id: prepareExportGcodeButton
                    property string toolTipText: backend.editorViewModel ? backend.editorViewModel.exportActionHint : qsTr("Backend unavailable")
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 146
                    Layout.preferredHeight: 24
                    radius: 12
                    enabled: backend.editorViewModel && backend.editorViewModel.canExportGCode
                    color: enabled ? Theme.accent : Theme.borderDefault
                    visible: backend.currentPage === backend.tp3DEditor
                    ToolTip.visible: exportMouse.containsMouse && prepareExportGcodeButton.toolTipText.length > 0
                    ToolTip.text: prepareExportGcodeButton.toolTipText
                    ToolTip.delay: 400

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("导出G-code文件")
                        color: Theme.textOnAccent
                        font.pixelSize: Theme.fontSizeMD
                        font.bold: true
                        opacity: prepareExportGcodeButton.enabled ? 1.0 : 0.55
                    }

                    MouseArea {
                        id: exportMouse
                        anchors.fill: parent
                        enabled: parent.enabled
                        hoverEnabled: true
                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        onClicked: root.exportGcodeRequested()
                    }
                }

                // ── Phase 90 AssembleView view-mode toggle ───────────────────
                // (90-CONTEXT.md decision 5; mirrors upstream Plater::assemble_view
                //  Plater.cpp:4959 as a peer of view3D/preview.) Visible only on the
                //  Prepare/3D-editor tab — AssembleView is a sub-view of tp3DEditor,
                //  like Prepare<->Preview are peer Plater view-modes. Clicking
                //  requests vmAssembleView via the Q_INVOKABLE entry point that
                //  emits viewModeChangeRequested first.
                Rectangle {
                    id: assembleViewToggle
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredWidth: 92
                    Layout.preferredHeight: 24
                    radius: 12
                    color: assembleToggleMouse.containsMouse ? Theme.accentDark : Theme.accentSubtle
                    border.width: backend.currentViewMode === backend.vmAssembleView ? 1 : 0
                    border.color: Theme.accent
                    opacity: backend.currentPage === backend.tp3DEditor ? 1.0 : 0.0
                    visible: backend.currentPage === backend.tp3DEditor
                    ToolTip.visible: assembleToggleMouse.containsMouse
                    ToolTip.text: qsTr("切换到装配视图")
                    ToolTip.delay: 400

                    Text {
                        anchors.centerIn: parent
                        text: qsTr("装配视图")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeMD
                    }

                    MouseArea {
                        id: assembleToggleMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: backend.requestChangeViewMode(backend.vmAssembleView)
                    }
                }

                Item { width: 20; height: 1 }
            }
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.chromeBorder
        }
    }

    // Phase 110 (FMAP-03): FilamentGroupPopup instance. Bound to the
    // BackendContext EditorViewModel so the popup can read the Phase 108
    // auto-recommended map preview and write the selected mode back.
    FilamentGroupPopup {
        id: filamentGroupPopup
        parent: Overlay.overlay
        editorVm: backend.editorViewModel
    }

    // ── TitleBarDivider 复用组件 ────────────────────────────────────────
    component TitleBarDivider: Rectangle {
        implicitWidth: 1
        implicitHeight: 18
        radius: 1
        color: Theme.chromeBorder
    }

    // ── [File ▾] 菜单 — 完整覆盖 CONTEXT.md 锁定结构 ───────────────────
    // 顺序：New / Open / Recent / Save / Save As / Import(3MF/STL/OBJ/STEP/AMF) / Export(G-code/3MF/Model) / Quit
    CxMenu {
        id: fileMenu

        CxMenuItem {
            text: qsTr("新建项目")
            onTriggered: root.newProjectRequested()
        }
        CxMenuItem {
            text: qsTr("打开项目...")
            onTriggered: root.openProjectRequested("")
        }

        // Recent 子菜单 — Instantiator 动态绑定 recentProjects
        CxMenu {
            id: fileRecentMenu
            title: qsTr("最近文件")
            Instantiator {
                model: backend.projectViewModel ? backend.projectViewModel.recentProjects : []
                delegate: CxMenuItem {
                    required property string modelData
                    text: modelData
                    // WR-04: modelData is a local QString path (ProjectViewModel::recentProjects),
                    // not QUrl-encoded. topbarOpenProject tolerates both via QUrl::isLocalFile()
                    // fallback — see BackendContext::topbarOpenProject.
                    onTriggered: root.openProjectRequested(modelData)
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

        CxMenuItem {
            text: qsTr("保存项目")
            onTriggered: {
                if (!backend.topbarSaveProject())
                    root.saveAsRequested()
            }
        }
        CxMenuItem {
            text: qsTr("项目另存为...")
            onTriggered: root.saveAsRequested()
        }

        MenuSeparator {}

        // Import 子菜单 — TOPBAR-02 完整覆盖 — Phase 51 SHELL-03: canImport gate (!isBusy)
        CxMenu {
            title: qsTr("导入")
            CxMenuItem {
                enabled: backend.canImport
                text: qsTr("Import 3MF")
                onTriggered: root.importModelRequested(qsTr("3MF 文件 (*.3mf)"))
            }
            CxMenuItem {
                enabled: backend.canImport
                text: qsTr("Import STL")
                onTriggered: root.importModelRequested(qsTr("STL 文件 (*.stl)"))
            }
            CxMenuItem {
                enabled: backend.canImport
                text: qsTr("Import OBJ")
                onTriggered: root.importModelRequested(qsTr("OBJ 文件 (*.obj)"))
            }
            CxMenuItem {
                enabled: backend.canImport
                text: qsTr("Import STEP")
                onTriggered: root.importModelRequested(qsTr("STEP 文件 (*.step *.stp)"))
            }
            CxMenuItem {
                enabled: backend.canImport
                text: qsTr("Import AMF")
                onTriggered: root.importModelRequested(qsTr("AMF 文件 (*.amf)"))
            }
            // Phase 237 (VIEW-02): Import Zip Archive (upstream
            // Plater::import_zip_archive, MainFrame.cpp:2327-2329). The
            // existing openModelDialog zip branch opens the FileArchiveDialog
            // (Phase 236), so this reuses the same file dialog + routing.
            CxMenuItem {
                enabled: backend.canImport
                text: qsTr("Import Zip Archive")
                onTriggered: root.importZipRequested()
            }
            // Phase 237 (VIEW-02): Import Configs (upstream MainFrame::
            // load_config_file, MainFrame.cpp:2330-2332). The Qt6 consumer
            // surface is the .json preset bundle (PresetServiceMock::
            // importBundle); upstream's zip/orca_* formats have no importer
            // here so they stay out of the filter.
            CxMenuItem {
                text: qsTr("导入配置...")
                onTriggered: root.importConfigsRequested()
            }
        }

        // Export 子菜单 — TOPBAR-02 完整覆盖 — Phase 51 SHELL-03: canExport gate
        CxMenu {
            title: qsTr("导出")
            CxMenuItem {
                enabled: backend.canExport
                text: qsTr("Export G-code")
                onTriggered: root.exportGcodeRequested()
            }
            // Phase 237 (VIEW-06): Export plate sliced file as .gcode.3mf
            // (upstream Plater::export_gcode_3mf, Plater.cpp:11499; Ctrl+G
            // "Export plate sliced file", KBShortcutsDialog.cpp:182). Gated
            // on a valid current-plate slice result, matching the Prepare
            // export button's canExportGCode gate.
            CxMenuItem {
                enabled: backend.editorViewModel && backend.editorViewModel.canExportGCode
                text: qsTr("导出切片文件...") + "\tCtrl+G"
                onTriggered: root.exportSlicedFileRequested()
            }
            CxMenuItem {
                enabled: backend.canExport
                text: qsTr("Export All Plate G-code")
                onTriggered: root.exportAllGcodeRequested()
            }
            CxMenuItem {
                enabled: backend.canExport
                text: qsTr("Export 3MF")
                onTriggered: root.exportProjectRequested()
            }
            CxMenuItem {
                enabled: backend.canExport
                text: qsTr("Export Model")
                onTriggered: root.exportModelRequested()
            }
            MenuSeparator {}
            // Phase 236 (DLG-01): Export Preset Bundle entry (upstream
            // ExportPresetBundleDialog; preset export is always available,
            // not gated on a sliced result).
            CxMenuItem {
                text: qsTr("导出预设包...")
                onTriggered: backend.showExportPresetBundleDialog()
            }
        }

        MenuSeparator {}

        CxMenuItem {
            text: qsTr("退出")
            onTriggered: Qt.quit()
        }
    }

    // ── [▾] 二级菜单 — Edit / View / Preferences / Calibration(9+1 占位) / Help
    CxMenu {
        id: topMenu

        // Edit 子菜单
        CxMenu {
            title: qsTr("编辑")
            CxMenuItem {
                text: qsTr("撤销")
                enabled: backend.currentPage === backend.tp3DEditor && backend.canUndo
                onTriggered: if (backend.currentPage === backend.tp3DEditor && backend.canUndo) root.undoRequested()
            }
            CxMenuItem {
                text: qsTr("重做")
                enabled: backend.currentPage === backend.tp3DEditor && backend.canRedo
                onTriggered: if (backend.currentPage === backend.tp3DEditor && backend.canRedo) root.redoRequested()
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
            // Phase 237 (VIEW-02): Delete All (upstream Ctrl+D, KBShortcuts
            // .cpp:256 "Delete all"; GLCanvas3D.cpp:3210-3213 posts
            // EVT_GLTOOLBAR_DELETE_ALL -> Plater::delete_all_objects_from_
            // model, Plater.cpp:4939). main.qml confirms first.
            CxMenuItem {
                text: qsTr("全部删除") + "\tCtrl+D"
                enabled: backend.editorViewModel && backend.editorViewModel.modelCount > 0
                onTriggered: root.deleteAllRequested()
            }
            CxMenuItem {
                text: qsTr("全选")
                onTriggered: if (backend.editorViewModel) backend.editorViewModel.selectAllVisibleObjects()
            }
            CxMenuItem {
                text: qsTr("取消选择")
                onTriggered: if (backend.editorViewModel) backend.editorViewModel.clearObjectSelection()
            }
            MenuSeparator {}
            CxMenuItem {
                text: qsTr("反向选择")
                enabled: false
                visible: false
            }
        }

        // View submenu -- Phase 237 (VIEW-01). Mirrors the upstream View
        // menu camera presets (MainFrame.cpp:2213-2235 add_common_view_menu_
        // items) plus the projection radio pair, the G-code window check item
        // and the 3D navigator check item from MainFrame.cpp:2601-2638.
        // Wave 4 exposes only controls with a live Qt6 consumer. Labels and
        // overhang are intentionally omitted: labels lack the upstream
        // object-instance text render path, and overhang is only a paint-tool
        // filter rather than a global scene overlay.
        CxMenu {
            id: viewMenu

            readonly property bool viewEnabled: backend.currentPage === backend.tp3DEditor
                                                || backend.currentPage === backend.tpPreview

            CxMenuItem {
                text: qsTr("默认视图") + "\tCtrl+0"
                enabled: viewMenu.viewEnabled && root.activeViewport !== null
                onTriggered: root.selectViewOnActiveViewport("plate")
            }
            CxMenuItem {
                text: qsTr("顶部视图") + "\tCtrl+1"
                enabled: viewMenu.viewEnabled && root.activeViewport !== null
                onTriggered: root.selectViewOnActiveViewport("top")
            }
            CxMenuItem {
                text: qsTr("底部视图") + "\tCtrl+2"
                enabled: viewMenu.viewEnabled && root.activeViewport !== null
                onTriggered: root.selectViewOnActiveViewport("bottom")
            }
            CxMenuItem {
                text: qsTr("前部视图") + "\tCtrl+3"
                enabled: viewMenu.viewEnabled && root.activeViewport !== null
                onTriggered: root.selectViewOnActiveViewport("front")
            }
            CxMenuItem {
                text: qsTr("后部视图") + "\tCtrl+4"
                enabled: viewMenu.viewEnabled && root.activeViewport !== null
                onTriggered: root.selectViewOnActiveViewport("rear")
            }
            CxMenuItem {
                text: qsTr("左侧视图") + "\tCtrl+5"
                enabled: viewMenu.viewEnabled && root.activeViewport !== null
                onTriggered: root.selectViewOnActiveViewport("left")
            }
            CxMenuItem {
                text: qsTr("右侧视图") + "\tCtrl+6"
                enabled: viewMenu.viewEnabled && root.activeViewport !== null
                onTriggered: root.selectViewOnActiveViewport("right")
            }

            MenuSeparator {}

            CxMenuItem {
                text: qsTr("重置视图")
                enabled: root.preparePageRef !== null
                onTriggered: if (root.preparePageRef) root.preparePageRef.applyFitHintIfReady()
            }
            CxMenuItem {
                text: qsTr("重置窗口布局")
                enabled: viewMenu.viewEnabled
                onTriggered: backend.resetWindowLayout()
            }
            CxMenuItem {
                text: (root.activeViewport && root.activeViewport.showSelectedOutline ? "✓ " : "")
                      + qsTr("显示选中对象轮廓")
                enabled: viewMenu.viewEnabled && root.activeViewport !== null
                onTriggered: if (root.activeViewport)
                    root.activeViewport.showSelectedOutline = !root.activeViewport.showSelectedOutline
            }

            MenuSeparator {}

            // Projection toggle (upstream View-menu radio pair "Use
            // Perspective View" / "Use Orthogonal View", MainFrame.cpp:2604
            // -2620; persisted upstream as use_perspective_camera).
            CxMenuItem {
                text: (root.activeViewport && !root.activeViewport.orthographicCamera ? "● " : "○ ")
                      + qsTr("使用透视投影")
                enabled: viewMenu.viewEnabled && root.activeViewport !== null
                onTriggered: if (root.activeViewport) root.activeViewport.orthographicCamera = false
            }
            CxMenuItem {
                text: (root.activeViewport && root.activeViewport.orthographicCamera ? "● " : "○ ")
                      + qsTr("使用正交投影")
                enabled: viewMenu.viewEnabled && root.activeViewport !== null
                onTriggered: if (root.activeViewport) root.activeViewport.orthographicCamera = true
            }

            MenuSeparator {}

            // Show G-code Window (upstream MainFrame.cpp:2623-2629 -- enabled
            // only on the Preview tab, toggles show_gcode_window). The Qt6
            // PreviewPage right-panel G-code source view is the consumer.
            CxMenuItem {
                text: (backend.previewViewModel && backend.previewViewModel.showGcodeWindow ? "✓ " : "")
                      + qsTr("显示 G-code 窗口")
                enabled: backend.currentPage === backend.tpPreview
                          && backend.previewViewModel !== null
                          && backend.previewViewModel.previewReady
                onTriggered: if (backend.previewViewModel)
                    backend.previewViewModel.setShowGcodeWindow(!backend.previewViewModel.showGcodeWindow)
            }

            // v5.16 (NAVIGATOR): Show 3D Navigator (upstream MainFrame.cpp:
            // 2630-2638, app_config show_3d_navigator default on,
            // AppConfig.cpp:200 -- "Show 3D navigator in Prepare and Preview
            // scene"). The viewport navigatorEnabled Q_PROPERTY binds this.
            CxMenuItem {
                text: (backend.settingsViewModel && backend.settingsViewModel.show3DNavigator ? "✓ " : "")
                      + qsTr("显示 3D 导航器")
                enabled: viewMenu.viewEnabled && backend.settingsViewModel !== null
                onTriggered: if (backend.settingsViewModel)
                    backend.settingsViewModel.setShow3DNavigator(!backend.settingsViewModel.show3DNavigator)
            }
        }

        MenuSeparator {}

        // Preferences — 仅发射信号（main.qml 打开 Preferences）
        CxMenuItem {
            text: qsTr("偏好设置")
            onTriggered: root.preferencesRequested()
        }

        // Phase 236 (DLG-01): tool dialogs that upstream hosts in the
        // Preferences dialog. The OWzx PreferencesPage is not yet mounted in
        // the shell (pre-existing gap), so the shell menu carries the
        // reachable entry points; the same dialog ids open via the
        // BackendContext request signals.
        CxMenu {
            title: qsTr("工具")
            CxMenuItem {
                text: qsTr("AMS 设置...")
                onTriggered: backend.showAMSSettingsDialog()
            }
            CxMenuItem {
                text: qsTr("固件...")
                onTriggered: backend.showFirmwareDialog()
            }
            CxMenuItem {
                text: qsTr("速度限制...")
                onTriggered: backend.showSpeedLimitDialog()
            }
            CxMenuItem {
                text: qsTr("擦料塔...")
                onTriggered: backend.showWipeTowerDialog()
            }
            CxMenuItem {
                text: qsTr("插件管理...")
                onTriggered: backend.showPluginManagerDialog()
            }
            CxMenuItem {
                text: qsTr("精简模式...")
                onTriggered: backend.showEnableLiteModeDialog()
            }
        }

        MenuSeparator {}

        // Calibration submenu aligned with upstream BBLTopbar::GetCalibMenu.
        CxMenu {
            title: qsTr("Calibration")
            CxMenuItem {
                text: qsTr("Calibration Center")
                onTriggered: root.calibrationRequested()
            }
            MenuSeparator {}
            CxMenuItem {
                text: qsTr("Flow Dynamics")
                onTriggered: {
                    backend.calibrationViewModel.selectItemById("flow_dynamics")
                    root.calibrationRequested()
                }
            }
            CxMenuItem {
                text: qsTr("Flow Rate")
                onTriggered: {
                    backend.calibrationViewModel.selectItemById("flow_rate")
                    root.calibrationRequested()
                }
            }
            CxMenuItem {
                text: qsTr("Temp Tower")
                onTriggered: {
                    backend.calibrationViewModel.selectItemById("temp_tower")
                    root.calibrationRequested()
                }
            }
            MenuSeparator {}
            CxMenuItem { text: qsTr("Hardware calibration pending"); enabled: false }
        }

        MenuSeparator {}

        // Help 子菜单（对齐上游 generate_help_menu，MainFrame.cpp:2136-2173）
        CxMenu {
            title: qsTr("帮助")
            CxMenuItem {
                text: qsTr("快捷键概览")
                onTriggered: root.shortcutOverviewRequested()
            }
            CxMenuItem {
                text: qsTr("设置向导")
                onTriggered: backend.showConfigWizard()
            }
            MenuSeparator {}
            CxMenuItem {
                text: qsTr("显示配置文件夹")
                onTriggered: backend.openConfigFolder()
            }
            CxMenuItem {
                text: qsTr("每日提示")
                onTriggered: root.dailyTipRequested()
            }
            CxMenuItem {
                text: root.updateCheckRunning ? qsTr("正在检查更新…") : qsTr("检查更新")
                enabled: !root.updateCheckRunning
                onTriggered: backend.checkForUpdates()
            }
            CxMenuItem {
                text: qsTr("网络测试")
                onTriggered: root.networkTestRequested()
            }
            MenuSeparator {}
            // Phase 236 (DLG-03): system information dump entry (upstream
            // Help > System Information).
            CxMenuItem {
                text: qsTr("系统信息")
                onTriggered: backend.showSysInfoDialog()
            }
            CxMenuItem { text: qsTr("About"); onTriggered: root.aboutRequested() }
        }
    }

    // 追加 BBLTopbar 缺失的信号（追加在末尾以保持代码可读）
    signal preferencesRequested()
    signal aboutRequested()
    signal shortcutOverviewRequested()
    signal dailyTipRequested()
    signal networkTestRequested()

    // Drives the busy label on Help > 检查更新.
    property bool updateCheckRunning: backend ? backend.updateCheckRunning : false
}
