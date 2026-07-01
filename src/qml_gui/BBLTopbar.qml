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
import "."

Item {
    id: root

    // ── 外部依赖 ──────────────────────────────────────────────────────────
    // `backend` 是 rootContext 的 context property（main_qml.cpp:134 注入），
    // 因此 BBLTopbar 内部直接通过 `backend` 引用，无需重复声明为 required property
    // （QML required property 在构造时绑定求值会先于 main.qml 的属性赋值，导致 undefined 误报）。
    property var preparePageRef: null        // PreparePage 引用（用于 undo/redo dispatch）
    property int windowVisibility: Window.Windowed  // 用于 Maximize/Restore 图标切换

    // ── BBLTopbar 对外发射的信号（main.qml 监听并 dispatch） ──────────────
    signal newProjectRequested()
    signal openProjectRequested()
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
    signal printRequested()
    signal bellClicked()
    signal windowMinimizeRequested()
    signal windowMaximizeRequested()
    signal windowCloseRequested()
    signal titleBarDragStarted()
    signal titleBarDoubleClicked()

    // ── 内部状态：当前 tab-switch latency token（main.qml 读取用于 onCurrentPageChanged 收尾）
    property int lastTabSwitchToken: -1

    implicitHeight: 40

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
                MenuItem { text: qsTr("打开项目..."); onTriggered: root.openProjectRequested() }
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
            anchors.fill: parent
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
            anchors.fill: parent
            anchors.leftMargin: 5
            anchors.rightMargin: 0
            spacing: 0

            // ── LEFT GROUP ───────────────────────────────────────────────
            // Logo (对齐上游 ID_LOGO)
            Rectangle {
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40
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
                    Text { text: qsTr("文件"); color: Theme.chromeText; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "▾"; color: Theme.chromeTextMuted; font.pixelSize: 10; anchors.verticalCenter: parent.verticalCenter }
                }

                MouseArea {
                    id: fileBtnMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: fileMenu.popup()
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
                currentIndex: backend.currentPage

                // 反馈环保护 (Pitfall 2 in 02-RESEARCH.md)：
                // 仅当 currentIndex 与 backend.currentPage 不同时才发请求
                onCurrentIndexChanged: {
                    if (currentIndex !== backend.currentPage
                            && currentIndex >= 0 && currentIndex <= backend.tpPlaceholder2) {
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
                        { label: qsTr("校准"),     pos: backend.tpCalibration },
                        { label: qsTr("辅助"),     pos: backend.tpPlaceholder1 }
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
                            font.pixelSize: 12
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

                // FilamentGroupPopup 占位 — Pitfall 5: 仅视觉，无 Q_INVOKABLE 调用，无状态
                // TODO(v2.1): implement FilamentGroupPopup
                CxIconButton {
                    cxStyle: CxIconButton.Style.Chrome
                    buttonSize: 30
                    iconSize: 16
                    iconSource: "qrc:/qml/assets/icons/box.svg"
                    toolTipText: ""
                    enabled: false
                    visible: false
                    opacity: 0.4
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
                font.pixelSize: 12
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
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.chromeBorder
        }
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
            onTriggered: root.openProjectRequested()
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
                    onTriggered: backend.topbarOpenProject(modelData)
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

        // Import 子菜单 — TOPBAR-02 完整覆盖
        CxMenu {
            title: qsTr("导入")
            CxMenuItem {
                text: qsTr("Import 3MF")
                onTriggered: root.importModelRequested(qsTr("3MF 文件 (*.3mf)"))
            }
            CxMenuItem {
                text: qsTr("Import STL")
                onTriggered: root.importModelRequested(qsTr("STL 文件 (*.stl)"))
            }
            CxMenuItem {
                text: qsTr("Import OBJ")
                onTriggered: root.importModelRequested(qsTr("OBJ 文件 (*.obj)"))
            }
            CxMenuItem {
                text: qsTr("Import STEP")
                onTriggered: root.importModelRequested(qsTr("STEP 文件 (*.step *.stp)"))
            }
            CxMenuItem {
                text: qsTr("Import AMF")
                onTriggered: root.importModelRequested(qsTr("AMF 文件 (*.amf)"))
            }
        }

        // Export 子菜单 — TOPBAR-02 完整覆盖
        CxMenu {
            title: qsTr("导出")
            CxMenuItem {
                text: qsTr("Export G-code")
                onTriggered: root.exportGcodeRequested()
            }
            CxMenuItem {
                text: qsTr("Export All Plate G-code")
                onTriggered: root.exportAllGcodeRequested()
            }
            CxMenuItem {
                text: qsTr("Export 3MF")
                onTriggered: root.exportProjectRequested()
            }
            CxMenuItem {
                text: qsTr("Export Model")
                onTriggered: root.exportModelRequested()
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

        // View 子菜单
        CxMenu {
            title: qsTr("视图")
            CxMenuItem {
                text: qsTr("显示/隐藏 Gizmo")
                enabled: false
                visible: false
            }
            MenuSeparator {}
            CxMenuItem {
                text: qsTr("重置视图")
                enabled: root.preparePageRef !== null
                onTriggered: if (root.preparePageRef) root.preparePageRef.applyFitHintIfReady()
            }
            MenuSeparator {}
            CxMenuItem {
                text: qsTr("显示层")
                enabled: false
                visible: false
            }
            CxMenuItem {
                text: qsTr("隐藏层")
                enabled: false
                visible: false
            }
        }

        MenuSeparator {}

        // Preferences — 仅发射信号（main.qml 打开 Preferences）
        CxMenuItem {
            text: qsTr("偏好设置")
            onTriggered: root.preferencesRequested()
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

        // Help 子菜单
        CxMenu {
            title: qsTr("帮助")
            CxMenuItem { text: qsTr("Documentation"); enabled: false }
            CxMenuItem { text: qsTr("Check for Updates"); enabled: false }
            MenuSeparator {}
            CxMenuItem { text: qsTr("About"); onTriggered: root.aboutRequested() }
            CxMenuItem { text: qsTr("Shortcut Overview"); onTriggered: root.shortcutOverviewRequested() }
        }
    }

    // 追加 BBLTopbar 缺失的信号（追加在末尾以保持代码可读）
    signal preferencesRequested()
    signal aboutRequested()
    signal shortcutOverviewRequested()
}
