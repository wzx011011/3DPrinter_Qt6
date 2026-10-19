// main.qml — OWzx Slicer slim ApplicationWindow shell
//
// 顶层框架对齐：third_party/OrcaSlicer/src/slic3r/GUI/MainFrame.cpp + BBLTopbar.cpp
// - BBLTopbar.qml 承载完整标题栏（Plan 02-02 Task 1）
// - 9-page StackLayout 与 Backend.TabPosition 枚举 0..8 一一对应（ARCH-01）
// - 所有页面索引通过 Backend.TabPosition.tpX 引用，禁止硬编码整数（Pitfall 1）

import QtQuick
pragma ComponentBehavior: Bound
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import QtQuick.Window
import OWzx

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
    // STARTUP-DEFER (2026-10-19): Plater (Prepare+Preview+Assemble, ~2.7s of
    // the measured engine->load) builds only after the first paint (timer
    // below) — or immediately when the user actually enters the prepare/
    // preview pages, whichever comes first.
    property bool deferredUiReady: false

    // Null-safe access to the (possibly not-yet-loaded) Plater's prepare
    // page; every plater.preparePageRef call site routes through this.
    function preparePage()
    {
        return platerLoader.item ? platerLoader.item.preparePageRef : null
    }

    Timer {
        id: deferredUiTimer
        interval: 100
        repeat: false
        running: false
        onTriggered: root.deferredUiReady = true
    }
    // Frame margin is 0 by default: the shell fills the whole window so the
    // app content goes edge-to-edge (matching the OrcaSlicer screenshot).
    readonly property int frameMargin: 0
    // topbar-4 (upstream MainFrame.cpp:524): a standard square-corner system
    // frame -- upstream draws no rounded shell, so the radius is a constant 0
    // (was 18 outside visualCompareMode, which rounded the topbar arc).
    readonly property int frameRadius: 0
    readonly property int prepareChromeHeight: 70

    // R-P0.6: force-close is set ONLY after the dirty-project guard is
    // satisfied (confirm dialog accepted, or the project is clean).
    // Qt.quit() never delivers a QCloseEvent, so every quit entry point
    // (frameless X button, File>退出) must go through requestQuit() --
    // a direct Qt.quit() silently bypasses the dirty-project confirm.
    property bool forceClose: false
    property string pendingGuardAction: ""
    property string pendingOpenPath: ""
    property bool openDialogGuardApproved: false
    onClosing: function(close) {
        if (root.forceClose) {
            close.accepted = true
            return
        }
        if (Backend.projectViewModel && Backend.projectViewModel.isDirty
                && !Backend.dirtyProjectDiscardRemembered()) {
            close.accepted = false
            root.pendingGuardAction = "quit"
            root.pendingOpenPath = ""
            newProjectDialog.open()
        }
    }

    // 当前 tab-switch latency token (BBLTopbar 写入, Connections onCurrentPageChanged 收尾)
    // 替代旧的 pendingSwitchToken / pendingSwitchTargetPage（Plan 02-02 Pitfall 3 迁移）
    property int activeTabSwitchToken: -1

    // R-P0.6 (upstream MainFrame.cpp:443 close_with_confirm): the shared
    // quit entry point. Dirty projects get the confirm dialog; clean projects
    // (or an approved forceClose) quit directly.
    function requestQuit() {
        if (Backend.projectViewModel && Backend.projectViewModel.isDirty
                && !Backend.dirtyProjectDiscardRemembered()) {
            root.pendingGuardAction = "quit"
            root.pendingOpenPath = ""
            newProjectDialog.open()
            return
        }
        root.forceClose = true
        Qt.quit()
    }

    function requestNewProject() {
        if (Backend.projectViewModel && Backend.projectViewModel.isDirty
                && !Backend.dirtyProjectDiscardRemembered()) {
            root.pendingGuardAction = "new"
            root.pendingOpenPath = ""
            newProjectDialog.open()
            return
        }
        Backend.topbarNewProject()
    }

    function requestOpenProject(filePath) {
        var path = filePath || ""
        if (path !== "" && root.openDialogGuardApproved) {
            root.openDialogGuardApproved = false
            Backend.topbarOpenProject(path)
            return
        }
        if (Backend.projectViewModel && Backend.projectViewModel.isDirty
                && !Backend.dirtyProjectDiscardRemembered()) {
            root.pendingGuardAction = "open"
            root.pendingOpenPath = path
            newProjectDialog.open()
            return
        }
        if (path !== "")
            Backend.topbarOpenProject(path)
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
            qsTr("G-code 文件 (*.gcode)"),
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
            // GAP-6: an externally sliced .gcode opens as a new project in
            // the preview-only context (upstream Plater::load_gcode,
            // Plater.cpp:10037-10058).
            if (modelPath.toLowerCase().substring(modelPath.length - 6) === ".gcode") {
                Backend.openExternalGcode(modelPath)
                return
            }
            Backend.topbarImportModel(selectedFile.toString())
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
            Backend.topbarSaveProjectAs(selectedFile.toString())
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
            if (Backend.editorViewModel)
                Backend.editorViewModel.exportModel(path, ext)
        }
    }

    // Phase 237 (VIEW-02): Import Configs file dialog (upstream MainFrame::
    // load_config_file, MainFrame.cpp:3209-3211). Only *.json has a real Qt6
    // importer (PresetService::importBundle).
    FileDialog {
        id: importConfigsDialog
        title: qsTr("导入配置")
        nameFilters: [qsTr("配置文件 (*.json)"), qsTr("所有文件 (*)")]
        onAccepted: Backend.topbarImportConfigs(selectedFile.toString())
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
            if (Backend.editorViewModel)
                Backend.editorViewModel.requestExportGcode3mf(selectedFile.toString())
        }
    }

    // Latency 跟踪迁移：endLatency 在 currentPage 改变后触发
    // (替代旧 onFrameSwapped + pendingSwitchTargetPage 逻辑 — Pitfall 3)
    FolderDialog {
        id: exportAllGcodeDialog
        title: qsTr("导出全部平板 G-code")
        onAccepted: {
            if (Backend.editorViewModel)
                Backend.editorViewModel.requestExportAllGCode(selectedFolder.toString())
        }
    }

    Connections {
        target: Backend
        function onCurrentPageChanged() {
            if (root.activeTabSwitchToken >= 0) {
                Backend.endLatency(root.activeTabSwitchToken)
                root.activeTabSwitchToken = -1
            }
        }
        function onExportGCodeRequested() {
            const p = root.preparePage()
            if (p) p.openExportDialog()
        }
    }

    // Upstream MultiMachineManagerPage::EVT_MULTI_DEVICE_VIEW routes the
    // selected device to Monitor::jump_to_monitor(dev_id). Keep identity
    // resolution in MonitorViewModel and page navigation in the shell.
    Connections {
        target: Backend.multiMachineViewModel
        function onDeviceViewRequested(ip, name, model) {
            if (!Backend.monitorViewModel
                || !Backend.monitorViewModel.selectDeviceByIdentity(ip, name)) {
                Backend.postNotification(qsTr("Device is no longer available in Monitor."),
                                         qsTr("Device"), 2)
                return
            }
            Backend.requestSelectTab(Backend.tpDevice)
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
        onAccepted: if (Backend.editorViewModel) Backend.editorViewModel.clearWorkspace()
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
            if (Backend.editorViewModel && targetObjectIndex >= 0) {
                // meters -> CONV_FROM_METER (3); imperial -> CONV_FROM_INCH (1)
                Backend.editorViewModel.applyUnitConversion(
                            targetObjectIndex, targetUnitHint === 1 ? 3 : 1)
            }
        }
    }

    // Phase 237 (VIEW-04): editor prompt relayed by BackendContext.
    Connections {
        target: Backend
        function onUnitConversionPromptRequested(objectIndex, unitHint, objectName) {
            unitConvertConfirm.targetObjectIndex = objectIndex
            unitConvertConfirm.targetUnitHint = unitHint
            unitConvertConfirm.targetObjectName = objectName || ""
            unitConvertConfirm.open()
        }
    }

    // Phase 237 (VIEW-03): window-level file drop (upstream
    // PlaterDropTarget::OnDropFiles, Plater.cpp:2738-2767). Accepted model
    // extensions match the upstream load_files drop patterns
    // (Plater.cpp:17318-17319); a dropped .gcode joins them (U6) and routes
    // through the same GAP-6 preview-only entry as the Ctrl+I dialog. A
    // single .svg routes into the existing SVG emboss flow
    // (EditorViewModel::importSVG, upstream emboss_svg, Plater.cpp:2158-2177);
    // multiple files import into the current plate. Files outside the
    // whitelist surface a toast through the shared notification channel
    // (upstream drops them silently; U6 makes the outcome visible).
    DropArea {
        anchors.fill: parent
        onDropped: function(drop) {
            var modelSuffixes = ["3mf", "stl", "obj", "step", "stp", "amf", "svg", "zip", "gcode"]
            var paths = []
            var gcodePaths = []
            var ignoredCount = 0
            for (var i = 0; i < drop.urls.length; ++i) {
                var path = drop.urls[i].toString()
                // Local file URLs only (upstream OnDropFiles receives paths);
                // anything else counts as an unsupported drop.
                if (path.indexOf("file:///") !== 0) {
                    ++ignoredCount
                    continue
                }
                path = decodeURIComponent(path.substring(8))
                var dot = path.lastIndexOf(".")
                var suffix = dot >= 0 ? path.substring(dot + 1).toLowerCase() : ""
                if (suffix === "gcode")
                    gcodePaths.push(path)
                else if (modelSuffixes.indexOf(suffix) >= 0)
                    paths.push(path)
                else
                    ++ignoredCount
            }
            // U6: unsupported drops used to vanish silently; report them via
            // Backend.postNotification (severity 2 = NotiWarning, the same
            // channel ErrorToast renders for every other surface).
            if (ignoredCount > 0)
                Backend.postNotification(
                            qsTr("已忽略 %1 个不支持导入的拖入文件").arg(ignoredCount),
                            qsTr("拖放导入"), 2)
            // Upstream load_files partitions the drop set: G-code and models
            // cannot load together (Plater.cpp:17352-17355) and only one
            // G-code file opens at a time (Plater.cpp:17346-17350); both
            // rejections surface as "G-code loading" messages.
            if (gcodePaths.length > 0) {
                if (paths.length > 0) {
                    Backend.postNotification(qsTr("G-code 文件与模型不能同时加载"),
                                             qsTr("G-code 加载"), 2)
                    return
                }
                if (gcodePaths.length > 1) {
                    Backend.postNotification(qsTr("一次只能打开一个 G-code 文件"),
                                             qsTr("G-code 加载"), 2)
                    return
                }
                root.requestActivate()
                // Same GAP-6 routing as the Ctrl+I openModelDialog onAccepted:
                // a new preview-only project; openExternalGcode switches to
                // the Preview tab itself (BackendContext::openExternalGcode).
                Backend.openExternalGcode(gcodePaths[0])
                return
            }
            if (paths.length === 0)
                return
            // Upstream raises the window and switches to the Prepare tab
            // before importing (Plater.cpp:2745-2748).
            root.requestActivate()
            if (Backend.currentPage !== Backend.tp3DEditor)
                Backend.requestSelectTab(Backend.tp3DEditor)
            // Single .svg: SVG emboss flow (upstream emboss_svg). OWzx's
            // importer adds the SVG volume to the selected object; without a
            // selection the path is parked on editorVm.svgFilePath for the
            // SVG gizmo panel (PreparePage binds the field).
            if (paths.length === 1 && paths[0].toLowerCase().endsWith(".svg")) {
                if (Backend.editorViewModel) {
                    Backend.editorViewModel.svgFilePath = paths[0]
                    if (Backend.editorViewModel.hasSelection)
                        Backend.editorViewModel.importSVG()
                }
                return
            }
            // Zip archives open the Phase 236 FileArchiveDialog tree instead
            // of a direct import (same routing as the import dialog).
            if (paths.length === 1 && paths[0].toLowerCase().endsWith(".zip")) {
                fileArchiveDialog.openFor(paths[0])
                return
            }
            if (Backend.editorViewModel)
                Backend.editorViewModel.addFilesToCurrentPlate(paths)
        }
    }

    // Keyboard shortcuts dialog (extracted to dialogs/KBShortcutsDialog.qml,
    // Phase 195 UI-02; 5-group layout aligned with upstream KBShortcutsDialog.cpp).
    KBShortcutsDialog {
        id: shortcutDialog
    }

    // Preferences dialog (2026-10-19): upstream hosts Preferences as a modal
    // dialog opened from the menu, not as a workflow tab. Opened via the
    // topbar preferencesRequested signal, Ctrl+P, the command palette, and
    // Backend.showPreferencesDialog() (--open-dialog preferences).
    PreferencesDialog {
        id: preferencesDialog
        settingsVm: Backend.settingsViewModel
    }

    // U7: the "?" global entry the dialog always advertised is now real
    // (upstream KBShortcutsDialog.cpp:206 "?" Show keyboard shortcuts list).
    // Global scope like upstream -- the shortcuts table is useful from any
    // page; text editors keep "?" for typing (the IsTextFocused gate) and
    // an open dialog eats the accelerator (popupHoldsFocus, upstream modal
    // semantics); the Help menu still opens it from anywhere.
    Shortcut {
        sequence: "?"
        enabled: !root.textEditHoldFocus && !root.popupHoldsFocus
        onActivated: shortcutDialog.open()
    }

    // About dialog (Phase 236 DLG-04: replaced the inline duplicate with the
    // shared AboutDialog component so the Help menu surfaces the corrected
    // AGPL-3.0 license + open-source components list from one place; the
    // Preferences "About" category opens the same component).
    AboutDialog {
        id: aboutDialog
    }

    // New project confirmation dialog. GAP-7: carries the upstream
    // "remember my choice" checkbox (Plater.cpp:11125-11140 persists
    // save_project_choise); a remembered discard makes every dirty guard
    // proceed without prompting.
    Dialog {
        id: newProjectDialog
        title: qsTr("新建项目")
        modal: true
        anchors.centerIn: parent
        width: 360
        height: 180
        padding: Theme.spacingXL

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
            CxCheckBox {
                id: rememberDiscardCheck
                text: qsTr("记住我的选择（不再提醒）")
            }
            Row {
                anchors.right: parent.right
                spacing: 8
                Rectangle {
                    width: 80; height: 28; radius: Theme.radiusMD
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
                    width: 80; height: 28; radius: Theme.radiusMD
                    color: Theme.accent
                    Text { anchors.centerIn: parent; text: qsTr("确定"); color: Theme.textOnAccent; font.pixelSize: Theme.fontSizeMD; font.bold: true }
                    MouseArea {
                        anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (rememberDiscardCheck.checked)
                                Backend.setDirtyProjectDiscardRemembered(true)
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
                                    Backend.topbarOpenProject(path)
                                else {
                                    root.openDialogGuardApproved = true
                                    openProjectDialog.open()
                                }
                            } else if (action === "new") {
                                Backend.topbarNewProject()
                            }
                        }
                    }
                }
            }
        }
    }

    // U10: the text-editing chords stand down while a text item holds active
    // focus (textEditHoldFocus, same gate as Tab/Shift+Tab below). Upstream
    // routes undo/redo/cut/copy/paste/select-all through the focused canvas
    // (GLCanvas3D wxEVT_CHAR), so a focused text control always kept them;
    // editable QML text items keep them via ShortcutOverride, while read-only
    // TextInputs (NetworkTestDialog / PresetDiffDialog logs and diffs) ignore
    // it -- the gate is the uniform protection.
    Shortcut {
        sequences: [StandardKey.Undo]
        enabled: Backend.currentPage === Backend.tp3DEditor
                 && !root.textEditHoldFocus
        onActivated: { const p = root.preparePage(); if (p) p.undoFromTopbar() }
    }
    Shortcut {
        sequences: [StandardKey.Save]
        onActivated: {
            // R-P1.I: topbarSaveProject() already gates on canSave() (visible
            // error posted). The save-as fallback must not fire for a gated
            // save -- only for the empty-path case.
            if (Backend.canSave && !Backend.topbarSaveProject())
                saveProjectAsDialog.open()
        }
    }
    Shortcut {
        sequences: [StandardKey.Redo]
        enabled: Backend.currentPage === Backend.tp3DEditor
                 && !root.textEditHoldFocus
        onActivated: { const p = root.preparePage(); if (p) p.redoFromTopbar() }
    }
    Shortcut {
        sequence: "Ctrl+Shift+Z"
        enabled: Backend.currentPage === Backend.tp3DEditor
                 && !root.textEditHoldFocus
        onActivated: { const p = root.preparePage(); if (p) p.redoFromTopbar() }
    }
    // Ctrl+Tab (upstream KBShortcutsDialog global "Switch table page",
    // MainFrame tab order): cycle the top-level pages through the guarded
    // requestSelectTab. Skips the unmapped debug slot; pages that map to no
    // route keep their current upstream-equivalent no-op.
    Shortcut {
        sequences: ["Ctrl+Tab"]
        onActivated: {
            var order = [Backend.tpHome, Backend.tp3DEditor, Backend.tpPreview,
                         Backend.tpDevice, Backend.tpMultiDevice, Backend.tpProject,
                         Backend.tpCalibration]
            var cur = order.indexOf(Backend.currentPage)
            var next = order[(cur + 1 + order.length) % order.length]
            Backend.requestSelectTab(next)
        }
    }
    Shortcut {
        sequence: "Delete"
        enabled: Backend.currentPage === Backend.tp3DEditor
                 && Backend.editorViewModel
                 && Backend.editorViewModel.hasSelection
        onActivated: Backend.editorViewModel.deleteSelection()
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
        enabled: Backend.editorViewModel && Backend.editorViewModel.hasSelection
                 && !root.textEditHoldFocus
        onActivated: Backend.editorViewModel.cutSelectedObjects()
    }
    Shortcut {
        sequence: "Ctrl+C"
        enabled: Backend.editorViewModel && Backend.editorViewModel.hasSelection
                 && !root.textEditHoldFocus
        onActivated: Backend.editorViewModel.copySelectedObjects()
    }
    Shortcut {
        sequence: "Ctrl+V"
        enabled: Backend.editorViewModel && Backend.editorViewModel.hasClipboardContent
                 && !root.textEditHoldFocus
        onActivated: Backend.editorViewModel.pasteObjects()
    }
    Shortcut {
        sequence: "Ctrl+K"
        enabled: Backend.editorViewModel && Backend.editorViewModel.hasSelection
        onActivated: Backend.editorViewModel.duplicateSelectedObjects()
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
        enabled: Backend.currentPage === Backend.tp3DEditor && Backend.editorViewModel
                 && !root.textEditHoldFocus
        onActivated: Backend.editorViewModel.selectAllVisibleObjects()
    }
    Shortcut {
        // Deselect all (upstream WXK_ESCAPE -> deselect_all,
        // GLCanvas3D.cpp:3234; KBShortcutsDialog.cpp:245)
        sequence: "Esc"
        enabled: Backend.currentPage === Backend.tp3DEditor && Backend.editorViewModel
        onActivated: Backend.editorViewModel.clearObjectSelection()
    }
    Shortcut {
        // Delete all (upstream Ctrl+D "Delete all", GLCanvas3D.cpp:3210
        // -3213 -> EVT_GLTOOLBAR_DELETE_ALL; KBShortcutsDialog.cpp:256).
        // Destructive: confirmed via the shared ConfirmDialog first.
        sequence: "Ctrl+D"
        enabled: Backend.currentPage === Backend.tp3DEditor
                 && Backend.editorViewModel
                 && Backend.editorViewModel.modelCount > 0
        onActivated: deleteAllConfirm.open()
    }
    Shortcut {
        // Preferences (upstream Ctrl+P, KBShortcutsDialog.cpp:197 /
        // MainFrame.cpp:2697)
        sequence: "Ctrl+P"
        onActivated: preferencesDialog.open()
    }
    Shortcut {
        // Slice plate (upstream Ctrl+R "Slice plate", KBShortcutsDialog.cpp:184)
        sequence: "Ctrl+R"
        enabled: Backend.editorViewModel && Backend.editorViewModel.canRequestSlice
        onActivated: Backend.editorViewModel.requestSlice()
    }
    Shortcut {
        // Export plate sliced file as .gcode.3mf (upstream Ctrl+G "Export
        // plate sliced file", KBShortcutsDialog.cpp:182 -> Plater::
        // export_gcode_3mf, Plater.cpp:7298)
        sequence: "Ctrl+G"
        enabled: Backend.editorViewModel && Backend.editorViewModel.canExportGCode
        onActivated: {
            exportGcode3mfDialog.currentFile = defaultSlicedFileUrl()
            exportGcode3mfDialog.open()
        }
    }
    Shortcut {
        // Print plate (upstream Ctrl+Shift+G "Print plate",
        // KBShortcutsDialog.cpp:189)
        sequence: "Ctrl+Shift+G"
        enabled: Backend.currentPage === Backend.tp3DEditor
        onActivated: { const p = root.preparePage(); if (p) p.openPrintDialog() }
    }
    // ── Phase 237 (VIEW-01): camera view presets (upstream Ctrl+0..6,
    // GLCanvas3D.cpp:3192-3201 / KBShortcutsDialog.cpp:247-253). The
    // active-viewport routing lives on BBLTopbar (same helper the View menu
    // uses; upstream routes to the current canvas3D, MainFrame.cpp:3455).
    Shortcut {
        sequence: "Ctrl+0"
        enabled: Backend.currentPage === Backend.tp3DEditor || Backend.currentPage === Backend.tpPreview
        onActivated: bblTopbar.selectViewOnActiveViewport("plate")
    }
    Shortcut {
        sequence: "Ctrl+1"
        enabled: Backend.currentPage === Backend.tp3DEditor || Backend.currentPage === Backend.tpPreview
        onActivated: bblTopbar.selectViewOnActiveViewport("top")
    }
    Shortcut {
        sequence: "Ctrl+2"
        enabled: Backend.currentPage === Backend.tp3DEditor || Backend.currentPage === Backend.tpPreview
        onActivated: bblTopbar.selectViewOnActiveViewport("bottom")
    }
    Shortcut {
        sequence: "Ctrl+3"
        enabled: Backend.currentPage === Backend.tp3DEditor || Backend.currentPage === Backend.tpPreview
        onActivated: bblTopbar.selectViewOnActiveViewport("front")
    }
    Shortcut {
        sequence: "Ctrl+4"
        enabled: Backend.currentPage === Backend.tp3DEditor || Backend.currentPage === Backend.tpPreview
        onActivated: bblTopbar.selectViewOnActiveViewport("rear")
    }
    Shortcut {
        sequence: "Ctrl+5"
        enabled: Backend.currentPage === Backend.tp3DEditor || Backend.currentPage === Backend.tpPreview
        onActivated: bblTopbar.selectViewOnActiveViewport("left")
    }
    Shortcut {
        sequence: "Ctrl+6"
        enabled: Backend.currentPage === Backend.tp3DEditor || Backend.currentPage === Backend.tpPreview
        onActivated: bblTopbar.selectViewOnActiveViewport("right")
    }

    // True while a text-input item holds active focus (TextInput / TextEdit
    // and the controls wrapping them). The window-level shortcuts for Tab /
    // Shift+Tab and for the text-editing chords (undo/redo, cut/copy/paste,
    // select-all -- U10) must stand down so the editor keeps them, matching
    // upstream where those keys are handled by the focused canvas
    // (GLCanvas3D wxEVT_CHAR) and a focused text control always wins.
    // Editable TextInput/TextEdit also accept ShortcutOverride for the edit
    // chords themselves, but a read-only TextInput ignores the override, so
    // the explicit gate is what keeps the chord semantics uniform.
    readonly property bool textEditHoldFocus: {
        var it = activeFocusItem
        return !!it && it.cursorPosition !== undefined
    }

    // True while a Popup/Dialog owns keyboard focus: an open popup parents
    // its focused contentItem to the window Overlay, so the active-focus
    // chain reaches Overlay exactly in that state. Window-level key
    // Shortcuts must stand down then, matching upstream where a modal
    // dialog eats the frame accelerators -- otherwise e.g. Space would open
    // the command palette BENEATH the overlay and its (invisible) search
    // field would swallow every following keystroke.
    readonly property bool popupHoldsFocus: {
        var overlay = shell.Overlay.overlay
        if (!overlay)
            return false
        var it = activeFocusItem
        while (it) {
            if (it === overlay)
                return true
            it = it.parent
        }
        return false
    }

    // ── CANVAS-FOCUS hotkey model ───────────────────────────────────────────
    // Upstream plater letter keys are canvas events: GLCanvas3D receives
    // wxEVT_CHAR only while the canvas is the focused child (clicking a
    // sidebar control moves focus and the keys go dead there too). The QML
    // equivalent is the active page root holding active focus -- the pages
    // claim it via forceActiveFocus when they become visible (see
    // PreparePage/PreviewPage). Window-level Keys forwarding is NOT used:
    // an attached Keys handler on the ApplicationWindow never receives
    // unhandled events in Qt 6.10 (verified empirically), and forwarding
    // every unconsumed key would also act while a sidebar control holds
    // focus, which upstream does not do.
    //
    // Tab / Shift+Tab stay window Shortcuts: focus navigation consumes Tab
    // before item Keys can see it, so a per-item binding is unreliable.
    Shortcut {
        // Upstream Tab: switch Prepare <-> Preview (KBShortcutsDialog.cpp
        // global list).
        sequence: "Tab"
        enabled: (Backend.currentPage === Backend.tp3DEditor
                  || Backend.currentPage === Backend.tpPreview)
                 && !root.textEditHoldFocus
        onActivated: Backend.requestSelectTab(
            Backend.currentPage === Backend.tpPreview
                ? Backend.tp3DEditor : Backend.tpPreview)
    }
    Shortcut {
        // Upstream Shift+Tab: collapse/expand the sidebar.
        sequence: "Shift+Tab"
        enabled: (Backend.currentPage === Backend.tp3DEditor
                  || Backend.currentPage === Backend.tpPreview)
                 && !root.textEditHoldFocus
        onActivated: Backend.requestToggleSidebar()
    }

    // Phase 237 (VIEW-06): default .gcode.3mf suggestion derived from the
    // gcode export name (upstream derives it from the output template,
    // Plater.cpp:11508-11529).
    function defaultSlicedFileUrl() {
        if (!Backend.editorViewModel)
            return ""
        var name = Backend.editorViewModel.defaultExportGCodeFileName()
        if (name.endsWith(".gcode"))
            name = name.substring(0, name.length - 6)
        return "file:///" + name + ".gcode.3mf"
    }

    // Visual compare reference PNG 映射 — 使用 TabPosition 枚举替代整数（Pitfall 1）
    readonly property string compareReferenceSource: Backend.currentPage === Backend.tp3DEditor
        ? "qrc:/qml/assets/prepare_ref.png"
        : Backend.currentPage === Backend.tpPreview
            ? "qrc:/qml/assets/preview_ref.png"
            : Backend.currentPage === Backend.tpDevice
                ? "qrc:/qml/assets/monitor_ref.png"
                : ""

    Rectangle {
        id: shell
        anchors.fill: parent
        anchors.margins: root.frameMargin
        radius: root.frameRadius
        color: Theme.bgBase
        border.color: Theme.borderDefault
        border.width: root.visibility === Window.Maximized ? 0 : 1
        clip: true

        Image {
            anchors.fill: parent
            visible: Backend.visualCompareMode && root.compareReferenceSource !== ""
            source: root.compareReferenceSource
            fillMode: Image.Stretch
        }

        ColumnLayout {
            visible: !Backend.visualCompareMode || root.compareReferenceSource === ""
            anchors.fill: parent
            spacing: 0

            // ── BBLTopbar 完整标题栏（替代旧的 titleBar Rectangle）──────────
            // `Backend` 由 rootContext 注入（main_qml.cpp:134），BBLTopbar 内部直接读取，
            // 无需显式 property 传递（避免 required property 在构造期求值产生 undefined 误报）
            BBLTopbar {
                id: bblTopbar
                // Phase 3: preparePage 现在是 Plater 内部子组件，通过 root.preparePage() 访问
                // （Plater 延迟构造期间为 null，BBLTopbar 全部消费点已判空）
                preparePageRef: platerLoader.item ? platerLoader.item.preparePageRef : null
                // Phase 237 (VIEW-01): preview canvas for the View menu / Ctrl+0..6
                // active-viewport routing (upstream MainFrame::select_view targets
                // the current canvas3D, MainFrame.cpp:3455).
                previewPageRef: platerLoader.item ? platerLoader.item.previewPageRef : null
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
                onExportGcodeRequested: { const p = root.preparePage(); if (p) p.openExportDialog() }
                onExportAllGcodeRequested: exportAllGcodeDialog.open()
                onExportProjectRequested: saveProjectAsDialog.open()
                onExportModelRequested: exportModelDialog.open()
                onUndoRequested: if (Backend.currentPage === Backend.tp3DEditor) { const p = root.preparePage(); if (p) p.undoFromTopbar() }
                onRedoRequested: if (Backend.currentPage === Backend.tp3DEditor) { const p = root.preparePage(); if (p) p.redoFromTopbar() }
                // MENU-EDIT-EXPAND (upstream MainFrame.cpp:2400-2467): the
                // Edit-menu clipboard/delete/clone set routes through the same
                // ViewModel actions as the shortcuts; delete-selected keeps the
                // Phase 169 confirm, duplicate-plate clones the current plate.
                onEditCutRequested: if (Backend.editorViewModel) Backend.editorViewModel.cutSelectedObjects()
                onEditCopyRequested: if (Backend.editorViewModel) Backend.editorViewModel.copySelectedObjects()
                onEditPasteRequested: if (Backend.editorViewModel) Backend.editorViewModel.pasteObjects()
                onEditDeleteRequested: if (Backend.currentPage === Backend.tp3DEditor) { const p = root.preparePage(); if (p) p.confirmDeleteSelection() }
                onEditCloneRequested: if (Backend.editorViewModel) Backend.editorViewModel.duplicateSelectedObjects()
                onEditDuplicatePlateRequested: if (Backend.editorViewModel) Backend.editorViewModel.clonePlate(Backend.editorViewModel.currentPlateIndex)
                onCalibrationRequested: Backend.requestSelectTab(Backend.tpCalibration)
                onPreferencesRequested: preferencesDialog.open()
                onAboutRequested: aboutDialog.open()
                onShortcutOverviewRequested: shortcutDialog.open()
                // Dead-control elimination: upstream Help menu entries
                // (MainFrame.cpp:2136-2173) — daily tip popup + network test.
                onDailyTipRequested: dailyTipDialog.open()
                onNetworkTestRequested: topbarNetworkTestDialog.open()
                onSliceRequested: sliceTopMenuExternal.popup()
                onSliceSinglePlateRequested: if (Backend.editorViewModel) Backend.editorViewModel.requestSlice()
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
                onWindowCloseRequested: root.requestQuit()
                onQuitRequested: root.requestQuit()
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
                    onTriggered: if (Backend.editorViewModel) Backend.editorViewModel.requestSlice()
                }
                CxMenuItem {
                    text: qsTr("切片全部平板")
                    onTriggered: if (Backend.editorViewModel) Backend.editorViewModel.requestSliceAll()
                }
            }

            // Print 下拉菜单
            CxMenu {
                id: printTopMenuExternal
                CxMenuItem {
                    text: qsTr("发送打印")
                    onTriggered: { const p = root.preparePage(); if (p) p.openPrintDialog() }
                }
                CxMenuItem {
                    text: qsTr("导出 G-code")
                    onTriggered: { const p = root.preparePage(); if (p) p.openExportDialog() }
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
                currentIndex: (Backend.currentPage === Backend.tp3DEditor
                               || Backend.currentPage === Backend.tpPreview)
                              ? Backend.tp3DEditor   // 两 tab 都落到 slot 1 (Plater)
                              : Backend.currentPage

                // Page 0 (tpHome) — Home
                Loader {
                    active: Backend.currentPage === Backend.tpHome
                    sourceComponent: Component {
                        HomePage {
                            homeVm: Backend.homeViewModel
                            onNewProjectRequested: root.requestNewProject()
                            onOpenProjectRequested: function(filePath) {
                                root.requestOpenProject(filePath)
                            }
                        }
                    }
                }
                // Page 1 (tp3DEditor) + Page 2 (tpPreview) — Plater 单实例共享
                // PreparePage 和 PreviewPage 作为 Plater 内部常驻子组件，由 viewMode 切可见性（ARCH-05/06/07）。
                // STARTUP-DEFER (2026-10-19): Plater 构造占 engine->load 的
                // ~57%（对照量测 4772ms→2041ms），延迟到首帧后 100ms 或用户
                // 进入 prepare/preview 时立即加载，二取先。
                Loader {
                    id: platerLoader
                    active: root.deferredUiReady
                             || Backend.currentPage === Backend.tp3DEditor
                             || Backend.currentPage === Backend.tpPreview
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    sourceComponent: Plater {
                        editorVm: Backend.editorViewModel
                        previewVm: Backend.previewViewModel
                        configVm: Backend.configViewModel
                        viewMode: Backend.currentViewMode
                        // Phase 4: sidebar dockable 三态绑定 Backend (统一管理 + 持久化)
                        sidebarCollapsed: Backend.sidebarCollapsed
                        sidebarWidth: Backend.sidebarWidth
                        sidebarMinWidth: Backend.sidebarMinWidth
                        sidebarMaxWidth: Backend.sidebarMaxWidth
                        sidebarDockArea: Backend.sidebarDockArea
                        sidebarToggleRequested: function() { Backend.requestToggleSidebar() }
                        sidebarWidthChanged: function(w) { Backend.requestSetSidebarWidth(w) }
                        // Phase 4 (Sidebar Dockable) 会在此接 BBLTopbar 折叠按钮；当前默认展开
                        leftPanelVisible: true
                    }
                }
                // Page 2 slot 占位 —— 实际内容由 slot 1 的 Plater 承载（viewMode=Preview 时显示）
                // 此 Item 仅占位以满足 StackLayout 9-slot 结构，不可见（visible 绑定 false）。
                Item {
                    visible: false
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                // Page 3 (tpDevice) — Monitor. Loader-gated like every other
                // page so the page (and its refresh Timer) only exists while
                // the Monitor tab is active (Q7: the previously always-on
                // instance polled monitorVm every second from app start).
                Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: Backend.currentPage === Backend.tpDevice
                    sourceComponent: Component {
                        MonitorPage {
                            monitorVm: Backend.monitorViewModel
                        }
                    }
                }
                // Page 4 (tpMultiDevice) — Multi-machine
                Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: Backend.currentPage === Backend.tpMultiDevice
                    sourceComponent: Component {
                        MultiMachinePage { multiMachineVm: Backend.multiMachineViewModel }
                    }
                }
                // Page 5 (tpProject) — Project
                Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: Backend.currentPage === Backend.tpProject
                    sourceComponent: Component {
                        ProjectPage {
                            projectVm: Backend.projectViewModel
                            editorVm: Backend.editorViewModel
                            onNewProjectRequested: root.requestNewProject()
                            onOpenProjectDialogRequested: root.requestOpenProject("")
                        }
                    }
                }
                // Page 6 (tpCalibration) — Calibration
                Loader {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    active: Backend.currentPage === Backend.tpCalibration
                    sourceComponent: Component {
                        CalibrationPage { calibrationVm: Backend.calibrationViewModel }
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
                // Page 8 (tpPreferences) — structural placeholder (same
                // treatment as Page 7). Preferences moved to a menu-opened
                // modal dialog (dialogs/PreferencesDialog.qml, upstream
                // behavior) on 2026-10-19; the slot keeps the StackLayout
                // count and the TabPosition enum range stable.
                Item {
                    visible: false
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            // layout-1: no permanent status bar. Upstream MainFrame's sizer
            // holds only the topbar + the tab panel (MainFrame.cpp:524,1025;
            // no CreateStatusBar), and the reference screenshot shows the
            // viewport running to the window's bottom edge. The old 24px
            // StatusBar (with its latencyBrief debug text) is removed; that
            // debug data is env-gated instead (debugOverlayVisible below,
            // driven by QML_DEBUG_LOG / OWZX_DEBUG_OVERLAY in main_qml.cpp).
        }

        // Floating Info toast (severity=0), z-stacked over shell content
        ErrorToast { }

        // Debug-only latency overlay (layout-1): replaces the latency text the
        // removed StatusBar used to carry. Visible only when the process was
        // started with QML_DEBUG_LOG or OWZX_DEBUG_OVERLAY (context property
        // set in main_qml.cpp), never in normal user runs.
        // BUILDGATE restore (2026-09-24): also carries the removed StatusBar's
        // semantic page index (current page / reserved-slot count ending at
        // tpPreferences) that WAVE-4 still locks, without resurrecting the
        // permanent bar upstream MainFrame never had.
        Text {
            visible: Backend.debugOverlayVisible
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 6
            z: 250
            text: (Backend.currentPage + 1) + " / " + (Backend.tpPreferences + 1)
                  + (Backend.latencyBrief !== "" ? "  |  " + Backend.latencyBrief : "")
            color: Theme.textTertiary
            font.pixelSize: Theme.fontSizeXS
            font.family: Theme.fontMono
        }

        // ── AI 助手聊天侧栏（OWzx-only，docs/ai-control.md）──────────────────
        // 右缘常驻窄条（仅 AI 启用时可见）+ 360px 覆盖式面板；状态与动作全部
        // 走 Backend.aiViewModel（AiViewModel），此处纯呈现。
        Rectangle {
            id: aiChatEdgeTab
            visible: Backend && Backend.aiControlActive && !root.aiChatOpen
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: 22
            height: 96
            radius: Theme.radiusLG
            color: aiEdgeMa.containsMouse ? Theme.accent : Theme.bgPanel
            border.width: 1
            border.color: Theme.borderSubtle

            Text {
                anchors.centerIn: parent
                text: qsTr("AI")
                color: aiEdgeMa.containsMouse ? Theme.textOnAccent : Theme.textPrimary
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
        // source (not inline sourceComponent): an inline component reference
        // forces main.qml to resolve the ChatSidebar type — and therefore
        // `import QtWebEngine` — at parse time, so a missing WebEngine plugin
        // aborts the WHOLE application load. With a source URL the import
        // failure only puts this Loader into the Error state; the rest of the
        // app keeps running (the degradation this comment block always
        // promised). main_qml.cpp skips QtWebEngineQuick::initialize() under
        // OWZX_DISABLE_WEBENGINE=1, which must be paired with this.
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

            source: Qt.resolvedUrl("panels/ChatSidebar.qml")
            onLoaded: item.closed.connect(function () { root.aiChatOpen = false })
        }
    }

    // P8.1 — First-run ConfigWizard
    ConfigWizardDialog {
        id: configWizardDialog
        onWizardFinished: {
            // R-P1.J (upstream ConfigWizard::apply_config): completing the
            // wizard must APPLY the chosen printer/filament presets, not just
            // persist the vendor/model — otherwise the wizard's choices never
            // took effect and the app kept the previous presets.
            // G-02: upstream load_presets also chains the print (process)
            // preset; Qt6 selects the first available print preset, mirroring
            // the chain's default pick.
            var dlg = configWizardDialog
            if (Backend.configViewModel) {
                if (dlg.selectedPrinter && dlg.selectedPrinter.length > 0)
                    Backend.configViewModel.requestCurrentPrinterPreset(dlg.selectedPrinter)
                if (dlg.selectedFilament && dlg.selectedFilament.length > 0)
                    Backend.configViewModel.requestCurrentFilamentPreset(dlg.selectedFilament)
                var printNames = Backend.configViewModel.printPresetNames
                if (printNames && printNames.length > 0)
                    Backend.configViewModel.requestCurrentPrintPreset(printNames[0])
            }
        }
    }

    // Dialog request signals
    Connections {
        target: Backend
        function onShowConfigWizardRequested() { configWizardDialog.open() }
        function onShowBedShapeDialogRequested() { bedShapeDialog.open() }
        function onShowEditGCodeDialogRequested(key, value) {
            editGCodeDialog.dialogTitle = qsTr("编辑自定义 G-code (%1)").arg(key || "")
            editGCodeDialog.optionKey = key || ""
            editGCodeDialog.initialGCode = value || ""
            editGCodeDialog.open()
        }
        function onShowAMSSettingsDialogRequested() { amsSettingsDialog.open() }
        function onShowPreferencesDialogRequested() { preferencesDialog.open() }
        function onShowFirmwareDialogRequested() { firmwareDialog.open() }
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
        editorVm: Backend.editorViewModel
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
            if (editGCodeDialog.optionKey !== "" && Backend.configViewModel) {
                if (!Backend.configViewModel.setValue(editGCodeDialog.optionKey, gcode))
                    console.warn("[EditGCode] no owning option for key:",
                                 editGCodeDialog.optionKey)
            }
        }
    }

    // P8.4 — AMS settings dialog
    // Phase 201 (v5.6): bound to AmsMaterialsViewModel (mock data + QSettings persistence).
    AMSSettingsDialog {
        id: amsSettingsDialog
        amsVm: Backend.amsMaterialsViewModel
    }

    // P8.5 — Firmware dialog
    FirmwareDialog { id: firmwareDialog }

    // P8.6b — Wipe tower dialog
    WipeTowerDialog { id: wipeTowerDialog }

    // P8.6c — Print host dialog
    PrintHostDialog { id: printHostDialog }

    // P10.1 — Plugin manager dialog (Phase 202: PluginService real Backend)
    PluginManagerDialog {
        id: pluginManagerDialog
        pluginService: Backend.pluginService
    }

    // P10.2 — Enable lite mode dialog
    EnableLiteModeDialog { id: enableLiteModeDialog }

    // Phase 236 (DLG-01) — Export Preset Bundle (File > 导出预设包...).
    ExportPresetBundleDialog {
        id: exportPresetBundleDialog
        configVm: Backend.configViewModel
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
                text: Backend.currentHintText()
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeMD
                wrapMode: Text.WordWrap
            }

            Text {
                Layout.fillWidth: true
                visible: Backend.currentHintFollowText() !== ""
                text: Backend.currentHintFollowText()
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
                    onClicked: Backend.prevHint()
                }
                CxButton {
                    text: qsTr("下一条")
                    compact: true
                    onClicked: Backend.nextHint()
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
        networkVm: Backend.monitorViewModel
    }

    // Phase 236 (DLG-03) — out-of-bed objects prompt. Opened via
    // Backend.recenterPromptRequested after a load; "全部居中" routes through
    // EditorViewModel::recenterObjectsOutsideBed.
    RecenterDialog {
        id: recenterDialog
        editorVm: Backend.editorViewModel
        onRecenterRequested: {
            if (Backend.editorViewModel)
                Backend.editorViewModel.recenterObjectsOutsideBed()
        }
    }

    // Phase 236 (DLG-03) — zip archive import tree. openFor() lists the
    // model entries; confirm extracts + loads the checked entries.
    FileArchiveDialog {
        id: fileArchiveDialog
        editorVm: Backend.editorViewModel
        onImportRequested: function(archivePath, selectedEntries) {
            if (Backend.editorViewModel && selectedEntries.length > 0)
                Backend.editorViewModel.importArchiveEntries(archivePath, selectedEntries)
        }
    }

    // Phase 236 (DLG-03) — OBJ mtl color → extruder mapping. Opened via
    // editorVm.objColorMappingRequested after a multi-color .obj import.
    ObjColorDialog {
        id: objColorDialog
        editorVm: Backend.editorViewModel
        onApplyRequested: function(extruderId) {
            if (Backend.editorViewModel)
                Backend.editorViewModel.applyPendingObjColors(extruderId)
        }
    }

    // Phase 236 (DLG-03): editor-triggered dialog opens (obj color prompt)
    // and the outside-bed recenter prompt.
    Connections {
        target: Backend.editorViewModel
        function onObjColorMappingRequested(objectName) {
            objColorDialog.targetObjectName = objectName || ""
            objColorDialog.open()
        }
    }
    Connections {
        target: Backend
        function onRecenterPromptRequested() { recenterDialog.open() }
    }

    // Phase 56 — three independent non-modal settings dialogs (one per
    // PresetCollection scope). Opened from the Prepare sidebar via the
    // onSettingsRequested handler above. Each is an ApplicationWindow that
    // stays focusable alongside the main window (CONTEXT.md decision).
    SettingsDialog {
        id: printerSettingsDialog
        configVm: Backend.configViewModel
        presetTier: "printer"
        optionModel: Backend.configViewModel ? Backend.configViewModel.machineOptions : null
    }
    SettingsDialog {
        id: materialSettingsDialog
        configVm: Backend.configViewModel
        presetTier: "filament"
        optionModel: Backend.configViewModel ? Backend.configViewModel.filamentOptions : null
    }
    SettingsDialog {
        id: processSettingsDialog
        configVm: Backend.configViewModel
        presetTier: "print"
        optionModel: Backend.configViewModel ? Backend.configViewModel.printOptions : null
    }

    // Phase 173 (P3): command palette (Ctrl+Shift+P). Cross-industry
    // baseline: one overlay to reach every page and appearance action. The
    // palette owns presentation; main.qml owns the command table and side
    // effects (Backend page enums, Theme, AppSettings persistence).
    CommandPalette {
        id: commandPalette
        commands: [
            { id: "page-home",        label: qsTr("转到：首页"),       hint: "" },
            { id: "page-prepare",     label: qsTr("转到：准备"),       hint: "" },
            { id: "page-preview",     label: qsTr("转到：预览"),       hint: "" },
            { id: "page-device",      label: qsTr("转到：设备"),       hint: "" },
            { id: "page-multi",       label: qsTr("转到：多设备"),     hint: "" },
            { id: "page-project",     label: qsTr("转到：项目"),       hint: "" },
            { id: "page-calibration", label: qsTr("转到：校准"),       hint: "" },
            { id: "page-preferences", label: qsTr("打开：偏好设置"),   hint: "" },
            { id: "theme-toggle",     label: Theme.isDark ? qsTr("切换到浅色主题") : qsTr("切换到深色主题"), hint: "" },
            { id: "density-toggle",   label: Theme.compactMode ? qsTr("密度：舒适") : qsTr("密度：紧凑"), hint: "" },
            { id: "scale-100",        label: qsTr("界面缩放：100%（重启后生效）"), hint: "" },
            { id: "scale-125",        label: qsTr("界面缩放：125%（重启后生效）"), hint: "" },
            { id: "scale-150",        label: qsTr("界面缩放：150%（重启后生效）"), hint: "" }
        ]
        onCommandTriggered: (id) => {
            const pageMap = {}
            pageMap["page-home"] = Backend.tpHome
            // tpPrepare does not exist in TabPosition (BackendContext.h:
            // tp3DEditor = 1 is the Prepare canvas page); the old
            // Backend.tpPrepare read was always undefined and the command
            // dead-ended after closing the palette.
            pageMap["page-prepare"] = Backend.tp3DEditor
            pageMap["page-preview"] = Backend.tpPreview
            pageMap["page-device"] = Backend.tpDevice
            pageMap["page-multi"] = Backend.tpMultiDevice
            pageMap["page-project"] = Backend.tpProject
            pageMap["page-calibration"] = Backend.tpCalibration
            if (id === "page-preferences") {
                // Preferences is a menu-opened dialog now (upstream hosts it
                // as a modal, not a workflow tab).
                preferencesDialog.open()
                return
            }
            if (pageMap[id] !== undefined) {
                Backend.requestSelectTab(pageMap[id])
                return
            }
            if (id === "theme-toggle") {
                const nextTheme = Theme.isDark ? "light" : "dark"
                // S6: Theme.setTheme forwards into the merged single chain
                // (Backend.requestTheme persists the "themeIndex" key and
                // re-dispatches); there is no second QML-side store anymore.
                Theme.setTheme(nextTheme)
                return
            }
            if (id === "density-toggle") {
                const nextDensity = Theme.compactMode ? "comfortable" : "compact"
                Theme.setDensity(nextDensity)
                AppSettings.setValue("ui/density", nextDensity)
                return
            }
            if (id.indexOf("scale-") === 0) {
                AppSettings.setValue("ui/scaleFactor", Number(id.slice(6)) / 100)
                Backend.postNotification(qsTr("界面缩放已保存，重启应用后生效"))
                return
            }
        }
        Shortcut {
            sequence: "Ctrl+Shift+P"
            onActivated: commandPalette.open()
        }
        // U7: Space is the upstream plater's canvas "actions speed dial"
        // (KBShortcutsDialog.cpp:271); OWzx surfaces that action list as this
        // palette, so the advertised key gets a real binding. The gates mirror
        // the CANVAS-FOCUS model -- the shortcut only fires while no control
        // is operating the key:
        // - Prepare page only: the Preview page owns Space (play/pause,
        //   PreviewPage.handlePreviewKey) and other pages have their own
        //   Space consumers.
        // - !textEditHoldFocus: a focused editor must receive the space.
        // - activeFocusControl === null: a focused QQC2 Control (button,
        //   checkbox, combo, slider, topbar workflow tab) keeps Space for its
        //   own press-activation; page root / RhiViewport focus yields null,
        //   matching how every other canvas key reaches the page key table.
        // - !popupHoldsFocus: an open dialog with the keyboard focus must
        //   keep the key -- otherwise the palette opens beneath the Overlay
        //   and its invisible search field swallows every keystroke.
        Shortcut {
            sequence: "Space"
            enabled: Backend.currentPage === Backend.tp3DEditor
                     && !root.textEditHoldFocus
                     && root.activeFocusControl === null
                     && !root.popupHoldsFocus
            onActivated: commandPalette.open()
        }
    }

    Component.onCompleted: {
        // STARTUP-DEFER: let the window paint Home first; the deferred-UI
        // timer (100ms one-shot) then flips deferredUiReady to build Plater
        // off the critical path.
        deferredUiTimer.start()
        // S6: the persisted theme is applied by BackendContext's constructor
        // (initial applyTheme sync before QML loads) — Theme.qml projects
        // Backend.qmlTheme, so there is no QML-side restore step anymore.
        // Phase 173 (P3): persisted density mode.
        Theme.setDensity(AppSettings.value("ui/density", "comfortable").toString())
        if (!Backend.startupSkipFirstRun && !Backend.configWizardCompleted) {
            configWizardDialog.open()
        }
    }

    // Resize borders (8 个边界 MouseArea，对齐 frameless window 体验)
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: 0
        z: 999
        visible: root.visibility !== Window.Maximized && !Backend.visualCompareMode

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
