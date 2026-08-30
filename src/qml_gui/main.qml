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
import "panels"

ApplicationWindow {
    id: root
    width: 1828
    height: 1000
    visible: true
    title: "OWzx Slicer"
    color: Theme.bgBase
    // Frameless + maximized by default for screenshot parity with OrcaSlicer.
    flags: Qt.Window | Qt.FramelessWindowHint
    visibility: Window.Maximized
    minimumWidth: 1100
    minimumHeight: 700

    readonly property int resizeMargin: 6
    // AI 助手聊天侧栏开关（OWzx-only，docs/ai-control.md）
    property bool aiChatOpen: false
    // Frame margin is 0 by default: the shell fills the whole window so the
    // app content goes edge-to-edge (matching the OrcaSlicer screenshot).
    readonly property int frameMargin: 0
    readonly property int frameRadius: (backend.visualCompareMode) ? 0 : 18
    readonly property int prepareChromeHeight: 70

    // The force-close bypass remains set through the Qt.quit-generated close
    // event. Clearing it before Qt.quit would reopen the dirty-project guard.
    property bool forceClose: false
    property string pendingGuardAction: ""
    property string pendingOpenPath: ""
    property bool openDialogGuardApproved: false
    onClosing: function(close) {
        if (root.forceClose) {
            close.accepted = true
            return
        }
        if (backend.projectViewModel && backend.projectViewModel.isDirty) {
            close.accepted = false
            root.pendingGuardAction = "quit"
            root.pendingOpenPath = ""
            newProjectDialog.open()
        }
    }

    // 当前 tab-switch latency token (BBLTopbar 写入, Connections onCurrentPageChanged 收尾)
    // 替代旧的 pendingSwitchToken / pendingSwitchTargetPage（Plan 02-02 Pitfall 3 迁移）
    property int activeTabSwitchToken: -1

    function requestNewProject() {
        if (backend.projectViewModel && backend.projectViewModel.isDirty) {
            root.pendingGuardAction = "new"
            root.pendingOpenPath = ""
            newProjectDialog.open()
            return
        }
        backend.topbarNewProject()
    }

    function requestOpenProject(filePath) {
        var path = filePath || ""
        if (path !== "" && root.openDialogGuardApproved) {
            root.openDialogGuardApproved = false
            backend.topbarOpenProject(path)
            return
        }
        if (backend.projectViewModel && backend.projectViewModel.isDirty) {
            root.pendingGuardAction = "open"
            root.pendingOpenPath = path
            newProjectDialog.open()
            return
        }
        if (path !== "")
            backend.topbarOpenProject(path)
        else
            openProjectDialog.open()
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
            qsTr("压缩包 (*.zip)"),
            qsTr("所有文件 (*)")
        ]
        onAccepted: {
            // Phase 236 (DLG-03): zip archives open the FileArchiveDialog tree
            // (upstream FileArchiveDialog) instead of a direct import.
            var modelPath = selectedFile.toString()
            if (modelPath.toLowerCase().substring(modelPath.length - 4) === ".zip") {
                fileArchiveDialog.openFor(modelPath)
                return
            }
            backend.topbarImportModel(selectedFile.toString())
        }
    }

    FileDialog {
        id: openProjectDialog
        title: qsTr("打开项目")
        nameFilters: [qsTr("项目文件 (*.3mf *.cxprj *.json)"), qsTr("所有文件 (*)")]
        onAccepted: root.requestOpenProject(selectedFile.toString())
        onRejected: root.openDialogGuardApproved = false
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

    // Phase 237 (VIEW-02): Import Configs file dialog (upstream MainFrame::
    // load_config_file, MainFrame.cpp:3209-3211). Only *.json has a real Qt6
    // importer (PresetServiceMock::importBundle).
    FileDialog {
        id: importConfigsDialog
        title: qsTr("导入配置")
        nameFilters: [qsTr("配置文件 (*.json)"), qsTr("所有文件 (*)")]
        onAccepted: backend.topbarImportConfigs(selectedFile.toString())
    }

    // Phase 237 (VIEW-06): export the plate sliced file as .gcode.3mf
    // (upstream Plater::export_gcode_3mf "Save Sliced file as:",
    // Plater.cpp:11436-11448). No defaultSuffix: the target carries a
    // double extension (.gcode.3mf) and the ViewModel appends it when
    // missing (upstream appends .3mf the same way, Plater.cpp:11546-11547).
    FileDialog {
        id: exportGcode3mfDialog
        title: qsTr("保存切片文件")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("切片文件 (*.gcode.3mf)")]
        onAccepted: {
            if (backend.editorViewModel)
                backend.editorViewModel.requestExportGcode3mf(selectedFile.toString())
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

    // Phase 237 (VIEW-02): delete-all destructive confirm (upstream
    // "All objects will be removed, continue?" message, Plater.cpp:11107;
    // confirm runs EditorViewModel::clearWorkspace, the OWzx equivalent of
    // delete_all_objects_from_model, Plater.cpp:4939).
    ConfirmDialog {
        id: deleteAllConfirm
        dialogTitle: qsTr("全部删除")
        message: qsTr("将移除所有对象，是否继续？")
        confirmText: qsTr("删除")
        onAccepted: if (backend.editorViewModel) backend.editorViewModel.clearWorkspace()
    }

    // Phase 237 (VIEW-04): unit-conversion confirm (upstream "Object too
    // small" dialog, Plater.cpp:4237-4253 — offers scaling a meters/imperial
    // authored mesh to millimeters). The conversion type maps the hint to
    // the upstream ConversionType (1 = CONV_FROM_INCH x25.4,
    // 3 = CONV_FROM_METER x1000).
    ConfirmDialog {
        id: unitConvertConfirm
        property int targetObjectIndex: -1
        property int targetUnitHint: 0
        property string targetObjectName: ""
        dialogTitle: qsTr("对象过小")
        destructive: false
        message: qsTr("来自文件 %1 的对象过小，可能以米或英寸为单位。\n是否缩放为毫米？").arg(targetObjectName)
        confirmText: qsTr("缩放")
        onAccepted: {
            if (backend.editorViewModel && targetObjectIndex >= 0) {
                // meters -> CONV_FROM_METER (3); imperial -> CONV_FROM_INCH (1)
                backend.editorViewModel.applyUnitConversion(
                            targetObjectIndex, targetUnitHint === 1 ? 3 : 1)
            }
        }
    }

    // Phase 237 (VIEW-04): editor prompt relayed by BackendContext.
    Connections {
        target: backend
        function onUnitConversionPromptRequested(objectIndex, unitHint, objectName) {
            unitConvertConfirm.targetObjectIndex = objectIndex
            unitConvertConfirm.targetUnitHint = unitHint
            unitConvertConfirm.targetObjectName = objectName || ""
            unitConvertConfirm.open()
        }
    }

    // Phase 237 (VIEW-03): window-level file drop (upstream
    // PlaterDropTarget::OnDropFiles, Plater.cpp:2738-2767). Accepted model
    // extensions match the upstream load_files surface; non-model files are
    // filtered out silently. A single .svg routes into the existing SVG
    // emboss flow (EditorViewModel::importSVG, upstream emboss_svg,
    // Plater.cpp:2158-2177); multiple files import into the current plate.
    DropArea {
        anchors.fill: parent
        onDropped: function(drop) {
            var modelSuffixes = ["3mf", "stl", "obj", "step", "stp", "amf", "svg", "zip"]
            var paths = []
            for (var i = 0; i < drop.urls.length; ++i) {
                var path = drop.urls[i].toString()
                // Local file URLs only (upstream OnDropFiles receives paths).
                if (path.indexOf("file:///") !== 0)
                    continue
                path = decodeURIComponent(path.substring(8))
                var dot = path.lastIndexOf(".")
                var suffix = dot >= 0 ? path.substring(dot + 1).toLowerCase() : ""
                if (modelSuffixes.indexOf(suffix) >= 0)
                    paths.push(path)
            }
            if (paths.length === 0)
                return
            // Upstream raises the window and switches to the Prepare tab
            // before importing (Plater.cpp:2745-2748).
            root.requestActivate()
            if (backend.currentPage !== backend.tp3DEditor)
                backend.requestSelectTab(backend.tp3DEditor)
            // Single .svg: SVG emboss flow (upstream emboss_svg). OWzx's
            // importer adds the SVG volume to the selected object; without a
            // selection the path is parked on editorVm.svgFilePath for the
            // SVG gizmo panel (PreparePage binds the field).
            if (paths.length === 1 && paths[0].toLowerCase().endsWith(".svg")) {
                if (backend.editorViewModel) {
                    backend.editorViewModel.svgFilePath = paths[0]
                    if (backend.editorViewModel.hasSelection)
                        backend.editorViewModel.importSVG()
                }
                return
            }
            // Zip archives open the Phase 236 FileArchiveDialog tree instead
            // of a direct import (same routing as the import dialog).
            if (paths.length === 1 && paths[0].toLowerCase().endsWith(".zip")) {
                fileArchiveDialog.openFor(paths[0])
                return
            }
            if (backend.editorViewModel)
                backend.editorViewModel.addFilesToCurrentPlate(paths)
        }
    }

    // Keyboard shortcuts dialog (extracted to dialogs/KBShortcutsDialog.qml,
    // Phase 195 UI-02; 5-group layout aligned with upstream KBShortcutsDialog.cpp).
    KBShortcutsDialog {
        id: shortcutDialog
    }

    // About dialog (Phase 236 DLG-04: replaced the inline duplicate with the
    // shared AboutDialog component so the Help menu surfaces the corrected
    // AGPL-3.0 license + open-source components list from one place; the
    // Preferences "About" category opens the same component).
    AboutDialog {
        id: aboutDialog
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
                font.pixelSize: Theme.fontSize13
                wrapMode: Text.Wrap
                width: parent.width
            }
            Row {
                anchors.right: parent.right
                spacing: 8
                Rectangle {
                    width: 80; height: 28; radius: 6
                    color: Theme.chromePressed
                    border.color: Theme.borderSubtle
                    Text { anchors.centerIn: parent; text: qsTr("取消"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.pendingGuardAction = ""
                            root.pendingOpenPath = ""
                            newProjectDialog.close()
                        }
                    }
                }
                Rectangle {
                    width: 80; height: 28; radius: 6
                    color: Theme.accent
                    Text { anchors.centerIn: parent; text: qsTr("确定"); color: Theme.textOnAccent; font.pixelSize: Theme.fontSizeMD; font.bold: true }
                    MouseArea {
                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var action = root.pendingGuardAction
                            var path = root.pendingOpenPath
                            root.pendingGuardAction = ""
                            root.pendingOpenPath = ""
                            newProjectDialog.close()
                            if (action === "quit") {
                                root.forceClose = true
                                Qt.quit()
                            } else if (action === "open") {
                                if (path !== "")
                                    backend.topbarOpenProject(path)
                                else {
                                    root.openDialogGuardApproved = true
                                    openProjectDialog.open()
                                }
                            } else if (action === "new") {
                                backend.topbarNewProject()
                            }
                        }
                    }
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
        onActivated: root.requestOpenProject("")
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
    // ── Phase 237 (VIEW-02): upstream global shortcut bindings ────────────
    // Each binding calls a real existing API (upstream KBShortcutsDialog.cpp
    // global list, lines 173-215).
    Shortcut {
        // New Project (upstream Ctrl+N, KBShortcutsDialog.cpp:175)
        sequence: "Ctrl+N"
        onActivated: root.requestNewProject()
    }
    Shortcut {
        // Save Project as (upstream Ctrl+Shift+S, KBShortcutsDialog.cpp:178)
        sequence: "Ctrl+Shift+S"
        onActivated: saveProjectAsDialog.open()
    }
    Shortcut {
        // Select all objects (upstream plater Ctrl+A, KBShortcutsDialog.cpp:255)
        sequence: "Ctrl+A"
        enabled: backend.currentPage === backend.tp3DEditor && backend.editorViewModel
        onActivated: backend.editorViewModel.selectAllVisibleObjects()
    }
    Shortcut {
        // Deselect all (upstream WXK_ESCAPE -> deselect_all,
        // GLCanvas3D.cpp:3234; KBShortcutsDialog.cpp:245)
        sequence: "Esc"
        enabled: backend.currentPage === backend.tp3DEditor && backend.editorViewModel
        onActivated: backend.editorViewModel.clearObjectSelection()
    }
    Shortcut {
        // Delete all (upstream Ctrl+D "Delete all", GLCanvas3D.cpp:3210
        // -3213 -> EVT_GLTOOLBAR_DELETE_ALL; KBShortcutsDialog.cpp:256).
        // Destructive: confirmed via the shared ConfirmDialog first.
        sequence: "Ctrl+D"
        enabled: backend.currentPage === backend.tp3DEditor
                 && backend.editorViewModel
                 && backend.editorViewModel.modelCount > 0
        onActivated: deleteAllConfirm.open()
    }
    Shortcut {
        // Preferences (upstream Ctrl+P, KBShortcutsDialog.cpp:197 /
        // MainFrame.cpp:2697)
        sequence: "Ctrl+P"
        onActivated: backend.openSettings()
    }
    Shortcut {
        // Slice plate (upstream Ctrl+R "Slice plate", KBShortcutsDialog.cpp:184)
        sequence: "Ctrl+R"
        enabled: backend.editorViewModel && backend.editorViewModel.canRequestSlice
        onActivated: backend.editorViewModel.requestSlice()
    }
    Shortcut {
        // Export plate sliced file as .gcode.3mf (upstream Ctrl+G "Export
        // plate sliced file", KBShortcutsDialog.cpp:182 -> Plater::
        // export_gcode_3mf, Plater.cpp:7298)
        sequence: "Ctrl+G"
        enabled: backend.editorViewModel && backend.editorViewModel.canExportGCode
        onActivated: {
            exportGcode3mfDialog.currentFile = defaultSlicedFileUrl()
            exportGcode3mfDialog.open()
        }
    }
    Shortcut {
        // Print plate (upstream Ctrl+Shift+G "Print plate",
        // KBShortcutsDialog.cpp:189)
        sequence: "Ctrl+Shift+G"
        enabled: backend.currentPage === backend.tp3DEditor
        onActivated: plater.preparePageRef.openPrintDialog()
    }
    // ── Phase 237 (VIEW-01): camera view presets (upstream Ctrl+0..6,
    // GLCanvas3D.cpp:3192-3201 / KBShortcutsDialog.cpp:247-253). The
    // active-viewport routing lives on BBLTopbar (same helper the View menu
    // uses; upstream routes to the current canvas3D, MainFrame.cpp:3455).
    Shortcut {
        sequence: "Ctrl+0"
        enabled: backend.currentPage === backend.tp3DEditor || backend.currentPage === backend.tpPreview
        onActivated: bblTopbar.selectViewOnActiveViewport("plate")
    }
    Shortcut {
        sequence: "Ctrl+1"
        enabled: backend.currentPage === backend.tp3DEditor || backend.currentPage === backend.tpPreview
        onActivated: bblTopbar.selectViewOnActiveViewport("top")
    }
    Shortcut {
        sequence: "Ctrl+2"
        enabled: backend.currentPage === backend.tp3DEditor || backend.currentPage === backend.tpPreview
        onActivated: bblTopbar.selectViewOnActiveViewport("bottom")
    }
    Shortcut {
        sequence: "Ctrl+3"
        enabled: backend.currentPage === backend.tp3DEditor || backend.currentPage === backend.tpPreview
        onActivated: bblTopbar.selectViewOnActiveViewport("front")
    }
    Shortcut {
        sequence: "Ctrl+4"
        enabled: backend.currentPage === backend.tp3DEditor || backend.currentPage === backend.tpPreview
        onActivated: bblTopbar.selectViewOnActiveViewport("rear")
    }
    Shortcut {
        sequence: "Ctrl+5"
        enabled: backend.currentPage === backend.tp3DEditor || backend.currentPage === backend.tpPreview
        onActivated: bblTopbar.selectViewOnActiveViewport("left")
    }
    Shortcut {
        sequence: "Ctrl+6"
        enabled: backend.currentPage === backend.tp3DEditor || backend.currentPage === backend.tpPreview
        onActivated: bblTopbar.selectViewOnActiveViewport("right")
    }

    // Phase 237 (VIEW-06): default .gcode.3mf suggestion derived from the
    // gcode export name (upstream derives it from the output template,
    // Plater.cpp:11508-11529).
    function defaultSlicedFileUrl() {
        if (!backend.editorViewModel)
            return ""
        var name = backend.editorViewModel.defaultExportGCodeFileName()
        if (name.endsWith(".gcode"))
            name = name.substring(0, name.length - 6)
        return "file:///" + name + ".gcode.3mf"
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
        color: backend ? backend.bgColor : Theme.bgBase
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
                // Phase 237 (VIEW-01): preview canvas for the View menu / Ctrl+0..6
                // active-viewport routing (upstream MainFrame::select_view targets
                // the current canvas3D, MainFrame.cpp:3455).
                previewPageRef: plater.previewPageRef
                Layout.fillWidth: true
                Layout.preferredHeight: root.prepareChromeHeight
                windowVisibility: root.visibility

                // v5.16 (PLATE-05): confirm only when unsaved changes exist
                // (upstream close_with_confirm semantics); a clean project
                // starts immediately.
                onNewProjectRequested: root.requestNewProject()
                onOpenProjectRequested: function(filePath) {
                    root.requestOpenProject(filePath)
                }
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
                // Dead-control elimination: upstream Help menu entries
                // (MainFrame.cpp:2136-2173) — daily tip popup + network test.
                onDailyTipRequested: dailyTipDialog.open()
                onNetworkTestRequested: topbarNetworkTestDialog.open()
                onSliceRequested: sliceTopMenuExternal.popup()
                onSliceSinglePlateRequested: if (backend.editorViewModel) backend.editorViewModel.requestSlice()
                onPrintRequested: printTopMenuExternal.popup()
                // Phase 237 (VIEW-02): destructive delete-all confirm (upstream
                // message "All objects will be removed, continue?",
                // Plater.cpp:11107) routes through the shared ConfirmDialog.
                onDeleteAllRequested: deleteAllConfirm.open()
                // Phase 237 (VIEW-02): zip import reuses the Phase 236
                // FileArchiveDialog flow (openModelDialog's zip branch).
                onImportZipRequested: {
                    openModelDialog.nameFilters = [qsTr("压缩包 (*.zip)"), qsTr("所有文件 (*)")]
                    openModelDialog.open()
                }
                onImportConfigsRequested: importConfigsDialog.open()
                // Phase 237 (VIEW-06): export plate sliced file (.gcode.3mf).
                onExportSlicedFileRequested: {
                    exportGcode3mfDialog.currentFile = defaultSlicedFileUrl()
                    exportGcode3mfDialog.open()
                }
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
                        HomePage {
                            homeVm: backend.homeViewModel
                            onNewProjectRequested: root.requestNewProject()
                            onOpenProjectRequested: function(filePath) {
                                root.requestOpenProject(filePath)
                            }
                        }
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
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                // Page 3 (tpDevice) — Monitor
                MonitorPage {
                    monitorVm: backend.monitorViewModel
                }
                // Page 4 (tpMultiDevice) — Multi-machine
                Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: backend.currentPage === backend.tpMultiDevice
                    sourceComponent: Component {
                        MultiMachinePage { multiMachineVm: backend.multiMachineViewModel }
                    }
                }
                // Page 5 (tpProject) — Project
                Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: backend.currentPage === backend.tpProject
                    sourceComponent: Component {
                        ProjectPage {
                            projectVm: backend.projectViewModel
                            editorVm: backend.editorViewModel
                            onNewProjectRequested: root.requestNewProject()
                            onOpenProjectDialogRequested: root.requestOpenProject("")
                        }
                    }
                }
                // Page 6 (tpCalibration) — Calibration
                Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: backend.currentPage === backend.tpCalibration
                    sourceComponent: Component {
                        CalibrationPage { calibrationVm: backend.calibrationViewModel }
                    }
                }
                // Page 7 (tpPlaceholder1) — structural placeholder.
                // AuxiliaryPage was removed in v4.6 Phase 126 (dead code: the page had zero
                // function — its actions all jumped to Prepare/Preview or logged "not yet
                // implemented", and AuxiliaryService was a file-copy service unrelated to the
                // UI). The slot stays as a structural placeholder to keep the StackLayout slot
                // count stable; the "辅助" tab in BBLTopbar is removed.
                Item {
                    visible: false
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                // Page 8 (tpPreferences) - Preferences.
                Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: backend.currentPage === backend.tpPreferences
                    sourceComponent: Component {
                        PreferencesPage {
                            settingsVm: backend.settingsViewModel
                            backend: backend
                        }
                    }
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

        // ── AI 助手聊天侧栏（OWzx-only，docs/ai-control.md）──────────────────
        // 右缘常驻窄条（仅 AI 启用时可见）+ 360px 覆盖式面板；状态与动作全部
        // 走 backend.aiViewModel（AiViewModel），此处纯呈现。
        Rectangle {
            id: aiChatEdgeTab
            visible: backend && backend.aiControlActive && !root.aiChatOpen
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: 22
            height: 96
            radius: 8
            color: aiEdgeMa.containsMouse ? Theme.accent : Theme.bgPanel
            border.width: 1
            border.color: Theme.borderSubtle

            Text {
                anchors.centerIn: parent
                text: qsTr("AI")
                color: aiEdgeMa.containsMouse ? "#FFFFFF" : Theme.textPrimary
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
            }
            MouseArea {
                id: aiEdgeMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.aiChatOpen = true
            }
        }

        // WebEngine 聊天面板懒加载：Chromium 进程树只在首次打开侧栏时启动
        // （首开 +~1s），关闭仅隐藏不销毁，重开即恢复。
        Loader {
            id: aiChatPanelLoader
            active: root.aiChatOpen
            visible: root.aiChatOpen && status === Loader.Ready
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: root.prepareChromeHeight
            width: 360
            z: 50

            sourceComponent: ChatSidebar {
                onClosed: root.aiChatOpen = false
            }
        }
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
            editGCodeDialog.optionKey = key || ""
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
        // Phase 236 (DLG-01): Export Preset Bundle dialog (File menu).
        function onShowExportPresetBundleDialogRequested() { exportPresetBundleDialog.open() }
        // Phase 236 (DLG-03): system information dialog (Help menu).
        function onShowSysInfoDialogRequested() { sysInfoDialog.open() }
        // Phase 56 — independent settings dialogs (region SETPRINT/SETMAT/SETPROC-SHELL).
        // BackendContext::forwardSettingsRequest(category) already ran
        // setActivePresetTier(category) before emitting, so the dialog opens scoped
        // to the right tier.
        function onSettingsRequested(category) {
            if (category === "printer") printerSettingsDialog.show()
            else if (category === "filament") materialSettingsDialog.show()
            else if (category === "print" || category === "process") processSettingsDialog.show()
        }
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
            // Phase 236 (DLG-02): write the edited text back onto the source
            // option key. ConfigViewModel::setValue routes through the owning
            // ConfigOptionModel (tier mapping + dirty tracking), identical to
            // an inline OptionRow edit. Key-less opens (template browsing
            // without a source field) skip the write-back.
            if (editGCodeDialog.optionKey !== "" && backend.configViewModel) {
                if (!backend.configViewModel.setValue(editGCodeDialog.optionKey, gcode))
                    console.warn("[EditGCode] no owning option for key:",
                                 editGCodeDialog.optionKey)
            }
        }
    }

    // P8.4 — AMS settings dialog
    // Phase 201 (v5.6): bound to AmsMaterialsViewModel (mock data + QSettings persistence).
    AMSSettingsDialog {
        id: amsSettingsDialog
        amsVm: backend.amsMaterialsViewModel
    }

    // P8.5 — Firmware dialog
    FirmwareDialog { id: firmwareDialog }

    // P8.6a — Speed limit dialog
    SpeedLimitDialog { id: speedLimitDialog }

    // P8.6b — Wipe tower dialog
    WipeTowerDialog { id: wipeTowerDialog }

    // P8.6c — Print host dialog
    PrintHostDialog { id: printHostDialog }

    // P10.1 — Plugin manager dialog (Phase 202: PluginService real backend)
    PluginManagerDialog {
        id: pluginManagerDialog
        pluginService: backend.pluginService
    }

    // P10.2 — Enable lite mode dialog
    EnableLiteModeDialog { id: enableLiteModeDialog }

    // Phase 236 (DLG-01) — Export Preset Bundle (File > 导出预设包...).
    ExportPresetBundleDialog {
        id: exportPresetBundleDialog
        configVm: backend.configViewModel
    }

    // Phase 236 (DLG-03) — system information dump (Help > 系统信息).
    SysInfoDialog { id: sysInfoDialog }

    // Dead-control elimination — Help > 每日提示 (upstream Show Tip of the
    // Day, MainFrame.cpp:2149 dailytips open). Content comes from the same
    // hints.json database the HomePage Daily Tips card reads.
    CxDialog {
        id: dailyTipDialog
        modal: true
        dialogTitle: qsTr("每日提示")
        anchors.centerIn: parent
        width: 420
        padding: Theme.spacingLG

        contentItem: ColumnLayout {
            spacing: Theme.spacingMD

            Text {
                Layout.fillWidth: true
                text: backend.currentHintText()
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.WordWrap
            }

            Text {
                Layout.fillWidth: true
                visible: backend.currentHintFollowText() !== ""
                text: backend.currentHintFollowText()
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSM

                CxButton {
                    text: qsTr("上一条")
                    compact: true
                    onClicked: backend.prevHint()
                }
                CxButton {
                    text: qsTr("下一条")
                    compact: true
                    onClicked: backend.nextHint()
                }
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("关闭")
                    cxStyle: CxButton.Style.Primary
                    compact: true
                    onClicked: dailyTipDialog.close()
                }
            }
        }
    }

    // Dead-control elimination — Help > 网络测试 (upstream Open Network Test,
    // MainFrame.cpp:2167 NetworkTestDialog). Own instance so the entry works
    // from any page (Preferences hosts the other instance).
    NetworkTestDialog {
        id: topbarNetworkTestDialog
        parent: Overlay.overlay
        networkVm: backend.monitorViewModel
    }

    // Phase 236 (DLG-03) — out-of-bed objects prompt. Opened via
    // backend.recenterPromptRequested after a load; "全部居中" routes through
    // EditorViewModel::recenterObjectsOutsideBed.
    RecenterDialog {
        id: recenterDialog
        editorVm: backend.editorViewModel
        onRecenterRequested: {
            if (backend.editorViewModel)
                backend.editorViewModel.recenterObjectsOutsideBed()
        }
    }

    // Phase 236 (DLG-03) — zip archive import tree. openFor() lists the
    // model entries; confirm extracts + loads the checked entries.
    FileArchiveDialog {
        id: fileArchiveDialog
        editorVm: backend.editorViewModel
        onImportRequested: function(archivePath, selectedEntries) {
            if (backend.editorViewModel && selectedEntries.length > 0)
                backend.editorViewModel.importArchiveEntries(archivePath, selectedEntries)
        }
    }

    // Phase 236 (DLG-03) — OBJ mtl color → extruder mapping. Opened via
    // editorVm.objColorMappingRequested after a multi-color .obj import.
    ObjColorDialog {
        id: objColorDialog
        editorVm: backend.editorViewModel
        onApplyRequested: function(extruderId) {
            if (backend.editorViewModel)
                backend.editorViewModel.applyPendingObjColors(extruderId)
        }
    }

    // Phase 236 (DLG-03): editor-triggered dialog opens (obj color prompt)
    // and the outside-bed recenter prompt.
    Connections {
        target: backend.editorViewModel
        function onObjColorMappingRequested(objectName) {
            objColorDialog.targetObjectName = objectName || ""
            objColorDialog.open()
        }
    }
    Connections {
        target: backend
        function onRecenterPromptRequested() { recenterDialog.open() }
    }

    // Phase 56 — three independent non-modal settings dialogs (one per
    // PresetCollection scope). Opened from the Prepare sidebar via the
    // onSettingsRequested handler above. Each is an ApplicationWindow that
    // stays focusable alongside the main window (CONTEXT.md decision).
    SettingsDialog {
        id: printerSettingsDialog
        configVm: backend.configViewModel
        presetTier: "printer"
        optionModel: backend.configViewModel ? backend.configViewModel.machineOptions : null
    }
    SettingsDialog {
        id: materialSettingsDialog
        configVm: backend.configViewModel
        presetTier: "filament"
        optionModel: backend.configViewModel ? backend.configViewModel.filamentOptions : null
    }
    SettingsDialog {
        id: processSettingsDialog
        configVm: backend.configViewModel
        presetTier: "print"
        optionModel: backend.configViewModel ? backend.configViewModel.printOptions : null
    }

    Component.onCompleted: {
        if (!startupSkipFirstRun && !backend.configWizardCompleted) {
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
