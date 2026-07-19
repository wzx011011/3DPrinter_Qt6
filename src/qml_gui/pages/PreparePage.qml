import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../controls"
import "../panels"
import "../components"
import "../dialogs"
import OWzxGL 1.0

Item {
    id: root
    required property var editorVm
    required property var configVm
    property alias viewport3dRef: viewport3d
    property string processCategory: ""
    property bool leftPanelVisible: true
    property int activeGizmoDragMode: GLViewport.GizmoMove
    readonly property int gizmoPanelTopOffset: 74
    readonly property bool visualCompareActive: typeof backend !== "undefined" && backend.visualCompareMode
    readonly property int targetViewportTopInset: 58
    readonly property int targetViewportBottomInset: 50
    readonly property int preparePlateBarHeight: 44
    readonly property int prepareBottomViewControlsBottomMargin: root.targetViewportBottomInset
        + ((root.editorVm && root.editorVm.plateCount > 0) ? root.preparePlateBarHeight + 8 : 0)
    // Phase 4: sidebar dockable 三态透传 (backend → Plater → PreparePage → DockableSidebar)
    property bool sidebarCollapsed: false
    // Phase 164 (SW-01): sidebar is now resizable within [300, 520] — was
    // min==max==392 making the drag handle a no-op. Bound to backend.* which
    // sources from BackendContext's kSidebar{Min,Max,Default}Width constants.
    property int sidebarWidth: backend ? backend.sidebarWidth : 392
    property int sidebarMinWidth: backend ? backend.sidebarMinWidth : 300
    property int sidebarMaxWidth: backend ? backend.sidebarMaxWidth : 520
    property int sidebarDockArea: 0   // 0=Left, 1=Right
    // sidebar 操作回调 (转发到 backend, 由 main.qml 注入)
    property var sidebarToggleRequested: null
    property var sidebarWidthChanged: null
    focus: true

    // Phase 169 (XD-01): shared destructive-action confirm dialog. Used by
    // deleteSelection (Delete/Backspace key) and other destructive triggers
    // to confirm before firing. See dialogs/ConfirmDialog.qml.
    ConfirmDialog {
        id: deleteConfirm
        dialogTitle: qsTr("删除选中")
        message: qsTr("确定要删除选中的对象吗？可通过撤销（Ctrl+Z）恢复。")
        confirmText: qsTr("删除")
        cancelText: qsTr("取消")
        destructive: true
    }

    // Phase 174 (FEAT-01): per-object settings override dialog. Opens when
    // selectionSettingsRequested fires (right-click Settings, GLToolbars
    // settings button, ObjectList menu). Reads/writes via the scopedOption*
    // VM proxies; backend lives on ProjectServiceMock.
    SelectionSettingsDialog {
        id: selectionSettingsDialog
        editorVm: root.editorVm
        objectIndex: root.editorVm ? root.editorVm.selectedObjectIndex : -1
        volumeIndex: root.editorVm ? root.editorVm.selectedVolumeIndex : -1
    }

    // Phase 175 (FEAT-02): per-object layer-range editor dialog. Reachable
    // via a new "层高范围" menu item in the object context menu.
    ObjectLayersDialog {
        id: objectLayersDialog
        editorVm: root.editorVm
        objectIndex: root.editorVm ? root.editorVm.selectedObjectIndex : -1
    }

    Connections {
        target: root.editorVm
        // Phase 174 (FEAT-01): opens the per-object settings override dialog.
        function onSelectionSettingsRequested() {
            selectionSettingsDialog.open()
        }
    }

    function applyFitHintIfReady() {
        if (!root.editorVm)
            return
        var h = root.editorVm.fitHint
        if (h && h.w > 0)
            viewport3d.requestFitView(h.x, h.y, h.z, h.w)
    }

    // Phase 121 (PAINT-03): per-gizmo brush-param selectors. The three paint
    // gizmos (Support=6, Seam=7, MMU=10) each have their own radius/tool/
    // cursor-type property set on the ViewModel; these helpers pick the active
    // set based on the current gizmoMode so the RhiViewport brushRadius /
    // brushCursorType / paintState Q_PROPERTYs bind to the right control. They
    // are pure value selectors -- no picking/geometry logic (the
    // rhiViewportSelectionPickingBridgeStaysCppOwned audit forbids that in QML).
    function _activeBrushRadius() {
        if (!root.editorVm)
            return 2
        if (viewport3d.gizmoMode === GLViewport.GizmoSeamPaint)
            return root.editorVm.seamPaintCursorRadius
        if (viewport3d.gizmoMode === GLViewport.GizmoMmuSegmentation)
            return root.editorVm.supportPaintCursorRadius
        // Default + GizmoSupportPaint: the support-paint cursor radius.
        return root.editorVm.supportPaintCursorRadius
    }
    function _activeBrushCursorType() {
        if (!root.editorVm)
            return 1
        // All three gizmos reuse supportPaintCursorType (0=Circle, 1=Sphere).
        return root.editorVm.supportPaintCursorType
    }
    function _activePaintState() {
        if (!root.editorVm)
            return 1
        // Support/Seam: tool 1=Enforcer(1), 2=Blocker(2). MMU: the selected
        // extruder (1-based) maps to EnforcerBlockerType ExtruderN (value N).
        if (viewport3d.gizmoMode === GLViewport.GizmoMmuSegmentation)
            return root.editorVm.mmuSelectedExtruder + 1
        if (viewport3d.gizmoMode === GLViewport.GizmoSeamPaint)
            return root.editorVm.seamPaintTool
        // GizmoSupportPaint: supportPaintTool (1=Enforcer, 2=Blocker).
        return root.editorVm.supportPaintTool
    }

    function undoFromTopbar() {
        viewport3d.undo()
    }
    function redoFromTopbar() {
        viewport3d.redo()
    }
    function openPrintDialog() {
        printDlg.open()
    }
    function openExportDialog() {
        if (!root.editorVm || !root.editorVm.canExportGCode) {
            if (root.editorVm)
                root.editorVm.requestExportGCode("")
            return
        }
        exportGCodeDlg.currentFile = root.editorVm.defaultExportGCodeFileName()
        exportGCodeDlg.open()
    }

    // GL FBO 缩略图捕获（对齐上游 PartPlate::thumbnail_data）
    function requestGLThumbnail() {
        viewport3d.requestThumbnailCapture(root.editorVm ? root.editorVm.currentPlateIndex : 0, 128)
    }

    function thumbnailSource(data) {
        if (!data || data.length === 0)
            return ""
        return data.indexOf("data:image/") === 0 ? data : "data:image/png;base64," + data
    }

    function setGizmoIfAvailable(mode) {
        if (!root.editorVm || ((root.editorVm.availableGizmoMask & (1 << mode)) === 0))
            return false
        viewport3d.gizmoMode = mode
        return true
    }

    // Keyboard shortcuts matching upstream CrealityPrint Plater
    Keys.onPressed: (event) => {
        if (!root.editorVm)
            return
        var key = event.key
        var mod = event.modifiers
        switch (key) {
        case Qt.Key_W:
            event.accepted = root.setGizmoIfAvailable(GLViewport.GizmoMove)
            break
        case Qt.Key_E:
            event.accepted = root.setGizmoIfAvailable(GLViewport.GizmoRotate)
            break
        case Qt.Key_R:
            if (!(mod & Qt.ControlModifier)) {
                event.accepted = root.setGizmoIfAvailable(GLViewport.GizmoScale)
            }
            break
        case Qt.Key_Z:
            // Undo handled by Shortcut below
            if (mod & Qt.ControlModifier)
                event.accepted = true
            break
        case Qt.Key_Y:
            // Redo handled by Shortcut below
            if (mod & Qt.ControlModifier)
                event.accepted = true
            break
        case Qt.Key_Delete:
        case Qt.Key_Backspace:
            // Phase 169 (XD-01): confirm before deleting selection (was
            // firing immediately — Delete is too easy to hit accidentally).
            deleteConfirm.openWithAction(function() {
                if (root.editorVm) root.editorVm.deleteSelection()
            })
            event.accepted = true
            break
        case Qt.Key_Escape:
            root.editorVm.clearObjectSelection()
            // Phase 115 REVIEW M2: also reset the two-click measure flow
            // so Escape cancels a pending A->B measurement cleanly.
            if (viewport3d.gizmoMode === GLViewport.GizmoMeasure)
                root.editorVm.clearMeasureReadout()
            event.accepted = true
            break
        case Qt.Key_A:
            if (mod & Qt.ControlModifier) {
                root.editorVm.selectAllVisibleObjects()
                event.accepted = true
            }
            break
        case Qt.Key_D:
            if (mod & Qt.ControlModifier) {
                root.editorVm.duplicateSelectedObjects()
                event.accepted = true
            }
            break
        case Qt.Key_C:
            if (mod & Qt.ControlModifier) {
                root.editorVm.copySelectedObjects()
                event.accepted = true
            }
            break
        case Qt.Key_V:
            if (mod & Qt.ControlModifier) {
                root.editorVm.pasteObjects()
                event.accepted = true
            }
            break
        case Qt.Key_X:
            if ((mod & Qt.ControlModifier) && !(mod & Qt.ShiftModifier)) {
                root.editorVm.cutSelectedObjects()
                event.accepted = true
            } else if ((mod & Qt.ControlModifier) && (mod & Qt.ShiftModifier)) {
                event.accepted = root.setGizmoIfAvailable(GLViewport.GizmoCut)
            }
            break
        case Qt.Key_F:
            root.applyFitHintIfReady()
            event.accepted = true
            break
        case Qt.Key_U:
            if (mod & Qt.ControlModifier) {
                event.accepted = root.setGizmoIfAvailable(GLViewport.GizmoMeasure)
            }
            break
        case Qt.Key_G:
            event.accepted = root.setGizmoIfAvailable(GLViewport.GizmoFlatten)
            break
        case Qt.Key_P:
            if (!(mod & Qt.ControlModifier)) {
                event.accepted = root.setGizmoIfAvailable(GLViewport.GizmoSupportPaint)
            }
            break
        }
    }

    // Default canvas context menu (right-click on empty space, no selection)
    // 对齐 upstream Plater::priv::on_right_click → menus.default_menu()
    CxMenu {
        id: defaultContextMenu

        CxMenuItem {
            text: qsTr("添加模型...")
            onTriggered: openFileDlg.open()
        }
        CxMenu {
            title: qsTr("添加图元")
            CxMenuItem {
                text: qsTr("立方体")
                onTriggered: if (root.editorVm) root.editorVm.addPrimitiveToPlate(0)
            }
            CxMenuItem {
                text: qsTr("球体")
                onTriggered: if (root.editorVm) root.editorVm.addPrimitiveToPlate(1)
            }
            CxMenuItem {
                text: qsTr("圆柱体")
                onTriggered: if (root.editorVm) root.editorVm.addPrimitiveToPlate(2)
            }
            CxMenuItem {
                text: qsTr("圆锥体")
                onTriggered: if (root.editorVm) root.editorVm.addPrimitiveToPlate(3)
            }
            CxMenuItem {
                text: qsTr("截锥体")
                onTriggered: if (root.editorVm) root.editorVm.addPrimitiveToPlate(4)
            }
            CxMenuItem {
                text: qsTr("圆环体")
                onTriggered: if (root.editorVm) root.editorVm.addPrimitiveToPlate(5)
            }
            CxMenuItem {
                text: qsTr("圆盘")
                onTriggered: if (root.editorVm) root.editorVm.addPrimitiveToPlate(6)
            }
            CxMenuItem {
                text: qsTr("文字")
                onTriggered: if (root.editorVm) root.editorVm.addTextObject()
            }
            CxMenuItem {
                text: qsTr("SVG")
                onTriggered: if (root.editorVm) root.editorVm.importSVG()
            }
        }
        MenuSeparator { }
        CxMenuItem {
            text: editorVm && editorVm.showLabels ? qsTr("隐藏标签") : qsTr("显示标签")
            onTriggered: if (root.editorVm) root.editorVm.showLabels = !root.editorVm.showLabels
        }
    }

    // Object context menu (right-click on selected object, aligns with upstream create_object_menu/create_extra_object_menu)
    CxMenu {
        id: objectContextMenu

        CxMenuItem {
            text: qsTr("复制选中")
            enabled: root.editorVm && root.editorVm.canDuplicateSelectedObjects
            onTriggered: if (root.editorVm) root.editorVm.duplicateSelectedObjects()
        }
        CxMenuItem {
            text: qsTr("删除选中")
            enabled: root.editorVm && root.editorVm.canDeleteSelection
            onTriggered: deleteConfirm.openWithAction(function() {
                if (root.editorVm) root.editorVm.deleteSelection()
            })
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("全选")
            onTriggered: if (root.editorVm) root.editorVm.selectAllVisibleObjects()
        }
        CxMenuItem {
            text: qsTr("取消选择")
            onTriggered: if (root.editorVm) root.editorVm.clearObjectSelection()
        }
        MenuSeparator { }
        // 对齐上游 create_extra_object_menu — Rename
        CxMenuItem {
            text: qsTr("重命名")
            enabled: root.editorVm && root.editorVm.canRenameSelectedObject
            onTriggered: {
                renameDialog.currentObjIndex = root.editorVm.selectedObjectIndex
                renameDialog.currentName = root.editorVm.objectName(root.editorVm.selectedObjectIndex)
                renameDialog.open()
            }
        }
        // 对齐上游 create_extra_object_menu — Center
        CxMenuItem {
            text: qsTr("居中到热床")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: if (root.editorVm) root.editorVm.centerSelectedObjects()
        }
        // 对齐上游 create_extra_object_menu — Fill bed with copies
        CxMenuItem {
            text: qsTr("铺满热床")
            enabled: root.editorVm && root.editorVm.canRenameSelectedObject
            onTriggered: if (root.editorVm) root.editorVm.fillBedWithCopies()
        }
        // 对齐上游 create_extra_object_menu — Export as STL
        CxMenuItem {
            text: qsTr("导出为 STL")
            enabled: root.editorVm && root.editorVm.canRenameSelectedObject
            onTriggered: if (root.editorVm) root.editorVm.exportSelectedAsStl()
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("移动模式")
            enabled: root.editorVm && ((root.editorVm.availableGizmoMask & (1 << GLViewport.GizmoMove)) !== 0)
            onTriggered: root.setGizmoIfAvailable(GLViewport.GizmoMove)
        }
        CxMenuItem {
            text: qsTr("旋转模式")
            enabled: root.editorVm && ((root.editorVm.availableGizmoMask & (1 << GLViewport.GizmoRotate)) !== 0)
            onTriggered: root.setGizmoIfAvailable(GLViewport.GizmoRotate)
        }
        CxMenuItem {
            text: qsTr("缩放模式")
            enabled: root.editorVm && ((root.editorVm.availableGizmoMask & (1 << GLViewport.GizmoScale)) !== 0)
            onTriggered: root.setGizmoIfAvailable(GLViewport.GizmoScale)
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("自动朝向")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: if (root.editorVm) root.editorVm.autoOrientSelected()
        }
        CxMenuItem {
            text: qsTr("拆分对象")
            enabled: root.editorVm && root.editorVm.canRenameSelectedObject
            onTriggered: if (root.editorVm) root.editorVm.splitSelectedObject()
        }
        MenuSeparator { }
        CxMenu {
            title: qsTr("镜像")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            CxMenuItem {
                text: qsTr("沿 X 轴镜像")
                onTriggered: if (root.editorVm) root.editorVm.mirrorSelectedObjects(0)
            }
            CxMenuItem {
                text: qsTr("沿 Y 轴镜像")
                onTriggered: if (root.editorVm) root.editorVm.mirrorSelectedObjects(1)
            }
            CxMenuItem {
                text: qsTr("沿 Z 轴镜像")
                onTriggered: if (root.editorVm) root.editorVm.mirrorSelectedObjects(2)
            }
        }
        MenuSeparator { }
        // 对齐上游 set_printable
        CxMenuItem {
            text: root.editorVm && root.editorVm.selectedObjectCount > 0
                   && root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex)
                   ? qsTr("设为不参与打印") : qsTr("设为可打印")
            enabled: root.editorVm && root.editorVm.canSetSelectionPrintable
            onTriggered: {
                if (root.editorVm) {
                    if (root.editorVm.selectedObjectCount > 1)
                        root.editorVm.setSelectedObjectsPrintable(!root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex))
                    else
                        root.editorVm.setObjectPrintable(root.editorVm.selectedObjectIndex, !root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex))
                }
            }
        }
        CxMenuItem {
            text: qsTr("显示/隐藏")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: if (root.editorVm) root.editorVm.toggleSelectedObjectsVisibility()
        }
        CxMenuItem {
            text: qsTr("适应视图")
            onTriggered: root.applyFitHintIfReady()
        }
        MenuSeparator { }
        // 对齐上游 create_extra_object_menu — Fix Model
        CxMenuItem {
            text: qsTr("修复模型")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: if (root.editorVm) root.editorVm.fixMeshSelected()
        }
        // 对齐上游 create_extra_object_menu — Simplify
        CxMenuItem {
            text: qsTr("简化模型")
            enabled: root.editorVm && ((root.editorVm.availableGizmoMask & (1 << GLViewport.GizmoSimplify)) !== 0)
            onTriggered: if (root.editorVm) root.editorVm.simplifyMeshSelected()
        }
        // Phase 141 / DEBT-02: removed orphaned "网格布尔运算" CxMenuItem — the
        // stub it called in EditorViewModel was a no-op (returned false, qWarning
        // "not yet implemented"). The working boolean path is the boolean dialog
        // (CxComboBox with 3 ops + booleanExecute button below), which routes
        // through EditorViewModel booleanExecute → ProjectServiceMock meshBoolean.
        // The dead menu + stub were removed together per the No-Deprecated-UI rule.
        MenuSeparator { }
        // 对齐上游 append_menu_item_per_object_settings
        CxMenuItem {
            text: qsTr("编辑参数表")
            enabled: root.editorVm && root.editorVm.canOpenSelectionSettings
            onTriggered: if (root.editorVm) root.editorVm.requestSelectionSettings()
        }
        CxMenuItem {
            text: qsTr("编辑工艺设置")
            onTriggered: backend.forwardSettingsRequest("process")
        }
        MenuSeparator { }
        // 对齐上游 append_menu_item_reload_from_disk
        CxMenuItem {
            text: qsTr("从磁盘重新加载")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: if (root.editorVm) root.editorVm.reloadSelectedFromDisk()
        }
        // 对齐上游 append_menu_item_replace_with_stl
        CxMenuItem {
            text: qsTr("替换为 STL...")
            enabled: root.editorVm && root.editorVm.hasSelectedVolume
            onTriggered: replaceWithStlDlg.open()
        }
        // Phase 175 (FEAT-02): layer-range editor entry (对齐上游 GUI_ObjectLayers).
        CxMenuItem {
            text: qsTr("层高范围...")
            enabled: root.editorVm && root.editorVm.selectedObjectIndex >= 0
            onTriggered: objectLayersDialog.open()
        }
        // 对齐上游 append_menu_item_change_filament — Change Filament submenu
        CxMenu {
            title: qsTr("更换耗材")
            enabled: root.editorVm && root.editorVm.canRenameSelectedObject
            CxMenuItem {
                text: qsTr("默认")
                onTriggered: if (root.editorVm) root.editorVm.setVolumeExtruderId(
                                 root.editorVm.selectedObjectIndex, 0, -1)
            }
            Repeater {
                model: root.configVm ? root.configVm.filamentPresetNames : []
                delegate: CxMenuItem {
                    text: qsTr("T%1 — %2").arg(index + 1).arg(modelData)
                    onTriggered: if (root.editorVm) root.editorVm.setVolumeExtruderId(
                                     root.editorVm.selectedObjectIndex, 0, index + 1)
                }
            }
        }
    }

    // Multi-selection context menu (对齐上游 multi_selection_menu)
    CxMenu {
        id: multiContextMenu

        // 对齐上游 append_menu_item_merge_to_multipart_object
        CxMenuItem {
            text: qsTr("组合")
            enabled: root.editorVm && root.editorVm.canDuplicateSelectedObjects
            onTriggered: if (root.editorVm) root.editorVm.assembleSelectedObjects()
        }
        CxMenuItem {
            text: qsTr("克隆")
            enabled: root.editorVm && root.editorVm.canDuplicateSelectedObjects
            onTriggered: if (root.editorVm) root.editorVm.duplicateSelectedObjects()
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("居中到热床")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: if (root.editorVm) root.editorVm.centerSelectedObjects()
        }
        CxMenuItem {
            text: qsTr("修复模型")
            enabled: root.editorVm && root.editorVm.canTransformSelection
            onTriggered: if (root.editorVm) root.editorVm.fixMeshSelected()
        }
        CxMenuItem {
            text: qsTr("删除")
            enabled: root.editorVm && root.editorVm.canDeleteSelection
            onTriggered: deleteConfirm.openWithAction(function() {
                if (root.editorVm) root.editorVm.deleteSelection()
            })
        }
        CxMenuItem {
            text: qsTr("复制")
            enabled: root.editorVm && root.editorVm.canDuplicateSelectedObjects
            onTriggered: if (root.editorVm) root.editorVm.copySelectedObjects()
        }
        CxMenuItem {
            text: qsTr("粘贴")
            enabled: root.editorVm && root.editorVm.hasClipboardContent
            onTriggered: if (root.editorVm) root.editorVm.pasteObjects()
        }
        MenuSeparator { }
        // 对齐上游 append_menu_item_set_printable
        CxMenuItem {
            text: qsTr("设为可打印")
            enabled: root.editorVm && root.editorVm.canSetSelectionPrintable
            onTriggered: if (root.editorVm) root.editorVm.setSelectedObjectsPrintable(true)
        }
        CxMenuItem {
            text: qsTr("编辑工艺设置")
            onTriggered: backend.forwardSettingsRequest("process")
        }
        MenuSeparator { }
        // 对齐上游 append_menu_item_change_filament — Change Filament submenu
        CxMenu {
            title: qsTr("更换耗材")
            enabled: root.editorVm && root.editorVm.canDuplicateSelectedObjects
            CxMenuItem {
                text: qsTr("默认")
                onTriggered: if (root.editorVm) root.editorVm.setVolumeExtruderId(
                                 root.editorVm.selectedObjectIndex, 0, -1)
            }
            Repeater {
                model: root.configVm ? root.configVm.filamentPresetNames : []
                delegate: CxMenuItem {
                    text: qsTr("T%1 — %2").arg(index + 1).arg(modelData)
                    onTriggered: if (root.editorVm) root.editorVm.setVolumeExtruderId(
                                     root.editorVm.selectedObjectIndex, 0, index + 1)
                }
            }
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("导出为 STL")
            enabled: root.editorVm && root.editorVm.canRenameSelectedObject
            onTriggered: if (root.editorVm) root.editorVm.exportSelectedAsStl()
        }
    }

    // Rename dialog (对齐上游 Plater::rename_object)
    Dialog {
        id: renameDialog
        property int currentObjIndex: -1
        property string currentName: ""
        title: qsTr("重命名对象")
        modal: true
        anchors.centerIn: parent
        width: 300
        padding: 16

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            Label { text: qsTr("输入新名称:"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeMD }

            CxTextField {
                id: renameInput
                Layout.fillWidth: true
                text: renameDialog.currentName
                placeholderText: qsTr("对象名称")
                onAccepted: {
                    if (root.editorVm && renameDialog.currentObjIndex >= 0) {
                        root.editorVm.renameObject(renameDialog.currentObjIndex, renameInput.text)
                        renameDialog.close()
                    }
                }
                Component.onCompleted: renameInput.forceActiveFocus()
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Rectangle {
                    Layout.fillWidth: true; height: 28; radius: 4
                    color: Theme.bgPressed
                    Label { anchors.centerIn: parent; text: qsTr("取消"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeSM }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: renameDialog.close() }
                }
                Rectangle {
                    Layout.fillWidth: true; height: 28; radius: 4
                    color: Theme.accent
                    Label { anchors.centerIn: parent; text: qsTr("确认"); color: Theme.accentDark; font.pixelSize: Theme.fontSizeSM }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (root.editorVm && renameDialog.currentObjIndex >= 0) {
                                root.editorVm.renameObject(renameDialog.currentObjIndex, renameInput.text)
                                renameDialog.close()
                            }
                        } }
                }
            }
        }
    }

    // Plate context menu (right-click on plate card)
    property int contextPlateIndex: -1

    CxMenu {
        id: plateContextMenu

        CxMenuItem {
            text: qsTr("选择全部对象")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
                     && root.editorVm.plateObjectCount(root.contextPlateIndex) > 0
            onTriggered: if (root.editorVm) root.editorVm.selectAllOnPlate(root.contextPlateIndex)
        }
        CxMenuItem {
            text: qsTr("清空平板")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
                     && root.editorVm.plateObjectCount(root.contextPlateIndex) > 0
            onTriggered: if (root.editorVm) root.editorVm.removeAllOnPlate(root.contextPlateIndex)
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("排列对象")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
                     && root.editorVm.plateObjectCount(root.contextPlateIndex) > 0
            onTriggered: {
                if (root.editorVm) root.editorVm.selectAllOnPlate(root.contextPlateIndex)
                if (root.editorVm) root.editorVm.arrangeAllObjects()
            }
        }
        CxMenuItem {
            text: qsTr("自动朝向")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
                     && root.editorVm.plateObjectCount(root.contextPlateIndex) > 0
            onTriggered: {
                if (root.editorVm) root.editorVm.selectAllOnPlate(root.contextPlateIndex)
                root.editorVm.autoOrientSelected()
            }
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("重命名")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
            onTriggered: {
                if (!root.editorVm || root.contextPlateIndex < 0) return
                var dialog = plateRenameDialog.createObject(root)
                dialog.plateIndex = root.contextPlateIndex
                dialog.currentName = root.editorVm.plateName(root.contextPlateIndex)
                dialog.open()
            }
        }
        CxMenuItem {
            text: qsTr("平板设置")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
            onTriggered: {
                if (!root.editorVm || root.contextPlateIndex < 0) return
                var dialog = plateSettingsDialogComp.createObject(root)
                dialog.plateIndex = root.contextPlateIndex
                dialog.plateName = root.editorVm.plateName(root.contextPlateIndex)
                dialog.open()
            }
        }
        CxMenuItem {
            text: root.contextPlateIndex >= 0 && root.editorVm
                  && root.editorVm.isPlateLocked(root.contextPlateIndex)
                  ? qsTr("解锁平板") : qsTr("锁定平板")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
            onTriggered: if (root.editorVm) root.editorVm.togglePlateLocked(root.contextPlateIndex)
        }
        // ── v3.0 Phase 17: plate lifecycle completion (PLATE-03/04/05) ──
        // 对齐上游 create_plate_menu — 切换 per-plate printable（D-08）
        CxMenuItem {
            text: root.contextPlateIndex >= 0 && root.editorVm
                  && root.editorVm.isPlatePrintable(root.contextPlateIndex)
                  ? qsTr("设为不打印") : qsTr("设为可打印")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
            onTriggered: if (root.editorVm) {
                var cur = root.editorVm.isPlatePrintable(root.contextPlateIndex)
                if (!root.editorVm.setPlatePrintable(root.contextPlateIndex, !cur))
                    backend.postNotification(qsTr("设置打印状态失败"), qsTr("操作失败"), 1)
            }
        }
        // 对齐上游 duplicate_plate — 克隆平板（D-06，深拷贝含对象）
        CxMenuItem {
            text: qsTr("克隆平板")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
                     && root.editorVm.canAddPlate
            onTriggered: if (root.editorVm) {
                if (!root.editorVm.clonePlate(root.contextPlateIndex))
                    backend.postNotification(qsTr("克隆平板失败：可能已达到最大平板数（36）"), qsTr("克隆失败"), 1)
            }
        }
        // 对齐上游 move_plate_to_index — 左移/右移重排（D-07）
        CxMenuItem {
            text: qsTr("左移平板")
            enabled: root.contextPlateIndex > 0 && root.editorVm
            onTriggered: if (root.editorVm) {
                if (!root.editorVm.movePlate(root.contextPlateIndex, root.contextPlateIndex - 1))
                    backend.postNotification(qsTr("移动平板失败"), qsTr("操作失败"), 1)
            }
        }
        CxMenuItem {
            text: qsTr("右移平板")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
                     && root.contextPlateIndex < root.editorVm.plateCount - 1
            onTriggered: if (root.editorVm) {
                if (!root.editorVm.movePlate(root.contextPlateIndex, root.contextPlateIndex + 1))
                    backend.postNotification(qsTr("移动平板失败"), qsTr("操作失败"), 1)
            }
        }
        MenuSeparator { }
        // 对齐上游 create_plate_menu — Reload All
        CxMenuItem {
            text: qsTr("全部重新加载")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
                     && root.editorVm.plateObjectCount(root.contextPlateIndex) > 0
            onTriggered: if (root.editorVm) root.editorVm.reloadAllOnPlate()
        }
        // 对齐上游 create_plate_menu — Paste
        CxMenuItem {
            text: qsTr("粘贴")
            onTriggered: if (root.editorVm) root.editorVm.pasteObjects()
        }
        MenuSeparator { }
        // 对齐上游 create_plate_menu — Add Models
        CxMenuItem {
            text: qsTr("添加模型...")
            onTriggered: openFileDlg.open()
        }
        // 对齐上游 create_plate_menu — Add Primitive submenu
        CxMenu {
            title: qsTr("添加图元")
            CxMenuItem {
                text: qsTr("立方体")
                onTriggered: if (root.editorVm) root.editorVm.addPrimitiveToPlate(0)
            }
            CxMenuItem {
                text: qsTr("球体")
                onTriggered: if (root.editorVm) root.editorVm.addPrimitiveToPlate(1)
            }
            CxMenuItem {
                text: qsTr("圆柱体")
                onTriggered: if (root.editorVm) root.editorVm.addPrimitiveToPlate(2)
            }
            CxMenuItem {
                text: qsTr("圆锥体")
                onTriggered: if (root.editorVm) root.editorVm.addPrimitiveToPlate(3)
            }
            CxMenuItem {
                text: qsTr("截锥体")
                onTriggered: if (root.editorVm) root.editorVm.addPrimitiveToPlate(4)
            }
            CxMenuItem {
                text: qsTr("圆环体")
                onTriggered: if (root.editorVm) root.editorVm.addPrimitiveToPlate(5)
            }
            CxMenuItem {
                text: qsTr("圆盘")
                onTriggered: if (root.editorVm) root.editorVm.addPrimitiveToPlate(6)
            }
            CxMenuItem {
                text: qsTr("文字")
                onTriggered: if (root.editorVm) root.editorVm.addTextObject()
            }
            CxMenuItem {
                text: qsTr("SVG")
                onTriggered: if (root.editorVm) root.editorVm.importSVG()
            }
        }
        MenuSeparator { }
        CxMenuItem {
            text: qsTr("删除平板")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
                     && root.editorVm.plateCount > 1
            onTriggered: if (root.editorVm) root.editorVm.deletePlate(root.contextPlateIndex)
        }
    }

    Component {
        id: plateRenameDialog
        Dialog {
            id: dlg
            required property int plateIndex
            required property string currentName
            title: qsTr("重命名平板")
            modal: true
            anchors.centerIn: parent
            width: 320

            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                CxTextField {
                    id: nameField
                    Layout.fillWidth: true
                    text: dlg.currentName
                    font.pixelSize: Theme.fontSize13
                    onAccepted: dlg.accept()
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Item { Layout.fillWidth: true }

                    Rectangle {
                        height: 28
                        width: cancelText.implicitWidth + 20
                        radius: 6
                        color: cancelHov.containsMouse ? Theme.borderSubtle : Theme.bgHover
                        Text { id: cancelText; anchors.centerIn: parent; text: qsTr("取消"); color: Theme.textSecondary; font.pixelSize: Theme.fontSizeMD }
                        HoverHandler { id: cancelHov }
                        TapHandler { onTapped: dlg.close() }
                    }
                    Rectangle {
                        height: 28
                        width: okText.implicitWidth + 20
                        radius: 6
                        color: Theme.accent
                        Text { id: okText; anchors.centerIn: parent; text: qsTr("确定"); color: Theme.textOnAccent; font.pixelSize: Theme.fontSizeMD }
                        TapHandler {
                            onTapped: {
                                if (root.editorVm && nameField.text.length > 0)
                                    root.editorVm.renamePlate(dlg.plateIndex, nameField.text)
                                dlg.close()
                            }
                        }
                    }
                }
            }

            onOpened: nameField.forceActiveFocus()
            onClosed: destroy()
        }
    }

    // 平板设置对话框（对齐上游 PlateSettingsDialog）
    Component {
        id: plateSettingsDialogComp
        Dialog {
            id: settingsDlg
            required property int plateIndex
            property string plateName: ""
            property int extruderCount: root.editorVm ? root.editorVm.plateExtruderCount(settingsDlg.plateIndex) : 1
            title: qsTr("平板设置") + " — " + settingsDlg.plateName
            modal: true
            anchors.centerIn: parent
            width: 420

            // 挤出机颜色数组（对齐上游 DragCanvas extruder colors）
            property var extruderColors: [Theme.statusError, Theme.accentDark, "#4444FF", Theme.statusWarning]

            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                // 平板名称
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        text: qsTr("平板名称")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                        Layout.preferredWidth: 100
                    }
                    CxTextField {
                        id: psNameField
                        Layout.fillWidth: true
                        text: settingsDlg.plateName
                        maximumLength: 20
                        font.pixelSize: Theme.fontSizeMD
                    }
                }

                // 热床类型（对齐上游 BedType: btDefault/btPC/btEP/btPEI/btPTE/btDEF/btER）
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        text: qsTr("热床类型")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                        Layout.preferredWidth: 100
                    }
                    CxComboBox {
                        id: bedTypeCombo
                        Layout.fillWidth: true
                        model: [
                            qsTr("跟随全局"),
                            qsTr("光滑 PEI"),
                            qsTr("高温 PEI"),
                            qsTr("纹理 PEI"),
                            qsTr("PC 热床"),
                            qsTr("EP 热床"),
                            qsTr("环氧树脂板"),
                            qsTr("自定义")
                        ]
                        currentIndex: root.editorVm ? root.editorVm.plateBedType(settingsDlg.plateIndex) : 0
                        onActivated: function(index) {
                            if (root.editorVm) root.editorVm.setPlateBedType(settingsDlg.plateIndex, index)
                        }
                    }
                }

                // 打印顺序（对齐上游 PlateSettingsDialog print sequence: ByDefault/ByLayer/ByObject）
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        text: qsTr("打印顺序")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                        Layout.preferredWidth: 100
                    }
                    CxComboBox {
                        id: printSeqCombo
                        Layout.fillWidth: true
                        model: [qsTr("跟随全局"), qsTr("按层打印"), qsTr("按对象打印")]
                        currentIndex: root.editorVm ? root.editorVm.platePrintSequence(settingsDlg.plateIndex) : 0
                        onActivated: function(index) {
                            if (root.editorVm) root.editorVm.setPlatePrintSequence(settingsDlg.plateIndex, index)
                        }
                    }
                }

                // 螺旋花瓶模式（对齐上游 PlateSettingsDialog spiral mode）
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        text: qsTr("螺旋花瓶")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeMD
                        Layout.preferredWidth: 100
                    }
                    CxComboBox {
                        id: spiralCombo
                        Layout.fillWidth: true
                        model: [qsTr("跟随全局"), qsTr("开启"), qsTr("关闭")]
                        currentIndex: root.editorVm ? root.editorVm.plateSpiralMode(settingsDlg.plateIndex) : 0
                        onActivated: function(index) {
                            if (root.editorVm) root.editorVm.setPlateSpiralMode(settingsDlg.plateIndex, index)
                        }
                    }
                }

                // ── 首层耗材顺序（对齐 upstream first_layer_print_sequence）──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: firstLayerCol.implicitHeight + 16
                    radius: 8
                    color: Theme.bgSurface
                    border.color: Theme.borderSubtle
                    border.width: 1

                    ColumnLayout {
                        id: firstLayerCol
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        Text {
                            text: qsTr("首层耗材顺序")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                            font.bold: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Text {
                                text: qsTr("模式")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeSM
                                Layout.preferredWidth: 60
                            }
                            CxComboBox {
                                id: firstLayerSeqChoiceCombo
                                Layout.fillWidth: true
                                model: [qsTr("自动"), qsTr("自定义")]
                                onActivated: function(index) {
                                    if (root.editorVm) root.editorVm.setPlateFirstLayerSeqChoice(settingsDlg.plateIndex, index)
                                }
                            }
                        }

                        // 自定义顺序：拖拽排序的挤出机列表（对齐上游 DragCanvas）
                        Text {
                            text: qsTr("挤出机顺序（拖拽调整）")
                            color: Theme.textSecondary
                            font.pixelSize: Theme.fontSizeSM
                            visible: firstLayerSeqChoiceCombo.currentIndex === 1
                        }

                        // DragCanvas-style 拖拽排序挤出机列表
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: firstLayerCanvas.height + 8
                            radius: 6
                            color: Theme.bgElevated
                            visible: firstLayerSeqChoiceCombo.currentIndex === 1
                            property var dragData: null

                            // 初始化顺序（首次显示时从 ViewModel 加载）
                            property var seqOrder: {
                                if (!visible || !root.editorVm) return []
                                var order = root.editorVm.plateFirstLayerSeqOrder(settingsDlg.plateIndex)
                                // order 为空时生成默认顺序 1..N
                                if (!order || order.length === 0) {
                                    order = []
                                    for (var i = 1; i <= settingsDlg.extruderCount; i++) order.push(i)
                                }
                                return order
                            }

                            onSeqOrderChanged: {
                                // 持久化到 ViewModel
                                if (visible && root.editorVm && seqOrder.length > 0)
                                    root.editorVm.setPlateFirstLayerSeqOrder(settingsDlg.plateIndex, seqOrder)
                            }

                            // 拖拽完成后交换顺序
                            function swapItems(fromIdx, toIdx) {
                                var arr = seqOrder.slice()
                                var tmp = arr[fromIdx]
                                arr[fromIdx] = arr[toIdx]
                                arr[toIdx] = tmp
                                seqOrder = arr
                            }

                            Flow {
                                id: firstLayerCanvas
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.margins: 4
                                spacing: 6

                                Repeater {
                                    id: firstLayerSeqRepeater
                                    model: firstLayerCanvas.parent.seqOrder

                                    delegate: Item {
                                        width: pillRow.implicitWidth + 12
                                        height: 28
                                        property int seqIndex: index

                                        // 拖拽源
                                        Drag.active: pillDragMA.drag.active
                                        Drag.hotSpot.x: width / 2
                                        Drag.hotSpot.y: height / 2
                                        Drag.keys: ["first-layer-seq"]

                                        RowLayout {
                                            id: pillRow
                                            anchors.centerIn: parent
                                            spacing: 4

                                            // 挤出机颜色圆
                                            Rectangle {
                                                width: 18; height: 18; radius: 9
                                                color: settingsDlg.extruderColors[(modelData - 1) % 4]
                                                border.width: 1
                                                border.color: parent.parent.pillDropArea.containsDrag ? "white" : "transparent"
                                            }
                                            Text {
                                                text: qsTr("%1").arg(modelData)
                                                color: "white"
                                                font.pixelSize: Theme.fontSizeXS
                                                font.bold: true
                                                anchors.centerIn: parent
                                            }
                                            // 箭头（非最后一个时显示）
                                            Text {
                                                text: "›"
                                                color: Theme.textDisabled
                                                font.pixelSize: Theme.fontSizeLG
                                                visible: seqIndex < firstLayerSeqRepeater.count - 1
                                            }
                                        }

                                        // 拖拽视觉反馈
                                        Rectangle {
                                            anchors.fill: parent
                                            radius: 6
                                            color: "transparent"
                                            border.width: 2
                                            border.color: pillDropArea.containsDrag ? Theme.accent : (pillDragMA.containsMouse ? Theme.borderActive : "transparent")
                                            opacity: pillDragMA.drag.active ? 0.3 : (pillDropArea.containsDrag ? 0.15 : 0)
                                        }

                                        // 拖拽目标区域
                                        DropArea {
                                            id: pillDropArea
                                            anchors.fill: parent
                                            keys: ["first-layer-seq"]
                                            onDropped: function(drop) {
                                                if (drop.source !== pillRow.parent && drop.source.seqIndex !== undefined) {
                                                    firstLayerCanvas.parent.swapItems(drop.source.seqIndex, seqIndex)
                                                }
                                                drop.accepted = true
                                            }
                                        }

                                        // 拖拽手势
                                        MouseArea {
                                            id: pillDragMA
                                            anchors.fill: parent
                                            cursorShape: Qt.OpenHandCursor
                                            drag.target: parent
                                            drag.axis: Drag.XAndYAxis
                                            hoverEnabled: true
                                            onReleased: function() {
                                                // 释放时复位位置
                                                parent.x = 0
                                                parent.y = 0
                                                parent.Drag.drop()
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // ── 其他层耗材顺序（对齐 upstream other_layers_print_sequence + OtherLayersSeqPanel）──
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: otherLayerCol.implicitHeight + 16
                    radius: 8
                    color: Theme.bgSurface
                    border.color: Theme.borderSubtle
                    border.width: 1

                    ColumnLayout {
                        id: otherLayerCol
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Text {
                                text: qsTr("其他层耗材顺序")
                                color: Theme.textPrimary
                                font.pixelSize: Theme.fontSizeMD
                                font.bold: true
                                Layout.fillWidth: true
                            }
                            CxComboBox {
                                id: otherLayersSeqChoiceCombo
                                Layout.preferredWidth: 100
                                model: [qsTr("自动"), qsTr("自定义")]
                                onActivated: function(index) {
                                    if (root.editorVm) root.editorVm.setPlateOtherLayersSeqChoice(settingsDlg.plateIndex, index)
                                }
                            }
                        }

                        // 自定义面板（对齐上游 OtherLayersSeqPanel）
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            visible: otherLayersSeqChoiceCombo.currentIndex === 1

                            // 层范围序列条目列表
                            Text {
                                text: qsTr("层范围序列（从第 2 层起，自动排序）")
                                color: Theme.textSecondary
                                font.pixelSize: Theme.fontSizeXS
                                Layout.fillWidth: true
                                wrapMode: Text.Wrap
                            }

                            Repeater {
                                id: otherLayersSeqRepeater
                                model: otherLayersSeqChoiceCombo.currentIndex === 1
                                        ? (root.editorVm ? root.editorVm.plateOtherLayersSeqCount(settingsDlg.plateIndex) : 0) : 0
                                delegate: Rectangle {
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: seqEntryCol.implicitHeight + 8
                                    radius: 6
                                    color: Theme.bgElevated
                                    border.color: Theme.borderSubtle
                                    property int entryIndex: index

                                    ColumnLayout {
                                        id: seqEntryCol
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        spacing: 4

                                        RowLayout {
                                            Layout.fillWidth: true
                                            spacing: 4
                                            Text {
                                                text: qsTr("起始层")
                                                color: Theme.textSecondary
                                                font.pixelSize: Theme.fontSizeXS
                                            }
                                            CxTextField {
                                                id: beginField
                                                Layout.preferredWidth: 50
                                                text: root.editorVm ? root.editorVm.plateOtherLayersSeqBegin(settingsDlg.plateIndex, delegateModel.entryIndex) : 2
                                                font.pixelSize: Theme.fontSizeXS
                                                horizontalAlignment: TextInput.AlignHCenter
                                                onEditingFinished: {
                                                    var v = parseInt(text, 10)
                                                    if (isNaN(v) || v < 2) v = 2
                                                    if (root.editorVm)
                                                        root.editorVm.setPlateOtherLayersSeqRange(settingsDlg.plateIndex, delegateModel.entryIndex, v,
                                                            parseInt(endField.text, 10) || v)
                                                }
                                            }
                                            Text {
                                                text: "–"
                                                color: Theme.textDisabled
                                                font.pixelSize: Theme.fontSizeXS
                                            }
                                            Text {
                                                text: qsTr("结束层")
                                                color: Theme.textSecondary
                                                font.pixelSize: Theme.fontSizeXS
                                            }
                                            CxTextField {
                                                id: endField
                                                Layout.preferredWidth: 50
                                                text: root.editorVm ? root.editorVm.plateOtherLayersSeqEnd(settingsDlg.plateIndex, delegateModel.entryIndex) : 100
                                                font.pixelSize: Theme.fontSizeXS
                                                horizontalAlignment: TextInput.AlignHCenter
                                                onEditingFinished: {
                                                    var v = parseInt(text, 10)
                                                    var bv = parseInt(beginField.text, 10) || 2
                                                    if (isNaN(v) || v < bv) v = bv
                                                    if (root.editorVm)
                                                        root.editorVm.setPlateOtherLayersSeqRange(settingsDlg.plateIndex, delegateModel.entryIndex, bv, v)
                                                }
                                            }

                                            Item { Layout.fillWidth: true }

                                            // 删除按钮
                                            Rectangle {
                                                width: 20; height: 20; radius: 4
                                                color: removeBtnMA.containsMouse ? Theme.statusError : Theme.bgSurface
                                                border.color: Theme.borderSubtle
                                                Text { anchors.centerIn: parent; text: "✕"; color: removeBtnMA.containsMouse ? "white" : Theme.textDisabled; font.pixelSize: Theme.fontSizeXS }
                                                TapHandler { id: removeBtnMA; onTapped: {
                                                    if (root.editorVm) root.editorVm.removePlateOtherLayersSeqEntry(settingsDlg.plateIndex, delegateModel.entryIndex)
                                                }}
                                            }
                                        }

                                        // DragCanvas-style 拖拽排序挤出机顺序
                                        Rectangle {
                                            Layout.fillWidth: true
                                            Layout.preferredHeight: otherLayerCanvas.height + 6
                                            radius: 4
                                            color: Theme.bgSurface
                                            property int entryIdx: delegateModel.entryIndex

                                            // 从 ViewModel 加载此 entry 的挤出机顺序
                                            property var seqOrder: {
                                                if (!root.editorVm) return []
                                                var order = root.editorVm.plateOtherLayersSeqOrder(settingsDlg.plateIndex, entryIdx)
                                                if (!order || order.length === 0) {
                                                    order = []
                                                    for (var i = 1; i <= settingsDlg.extruderCount; i++) order.push(i)
                                                }
                                                return order
                                            }

                                            onSeqOrderChanged: {
                                                if (root.editorVm && seqOrder.length > 0)
                                                    root.editorVm.setPlateOtherLayersSeqOrder(settingsDlg.plateIndex, entryIdx, seqOrder)
                                            }

                                            function swapItems(fromIdx, toIdx) {
                                                var arr = seqOrder.slice()
                                                var tmp = arr[fromIdx]
                                                arr[fromIdx] = arr[toIdx]
                                                arr[toIdx] = tmp
                                                seqOrder = arr
                                            }

                                            Flow {
                                                id: otherLayerCanvas
                                                anchors.left: parent.left
                                                anchors.right: parent.right
                                                anchors.top: parent.top
                                                anchors.margins: 3
                                                spacing: 4

                                                Repeater {
                                                    model: otherLayerCanvas.parent.seqOrder

                                                    delegate: Item {
                                                        width: pillOLRow.implicitWidth + 10
                                                        height: 24
                                                        property int seqIndex: index

                                                        Drag.active: pillOLDragMA.drag.active
                                                        Drag.hotSpot.x: width / 2
                                                        Drag.hotSpot.y: height / 2
                                                        Drag.keys: ["other-layer-seq-" + otherLayerCanvas.parent.entryIdx]

                                                        RowLayout {
                                                            id: pillOLRow
                                                            anchors.centerIn: parent
                                                            spacing: 3

                                                            Rectangle {
                                                                width: 16; height: 16; radius: 8
                                                                color: settingsDlg.extruderColors[(modelData - 1) % 4]
                                                                border.width: 1
                                                                border.color: parent.parent.pillOLDrop.containsDrag ? "white" : "transparent"
                                                            }
                                                            Text {
                                                                text: qsTr("%1").arg(modelData)
                                                                color: "white"
                                                                font.pixelSize: 8
                                                                font.bold: true
                                                                anchors.centerIn: parent
                                                            }
                                                            Text {
                                                                text: "›"
                                                                color: Theme.textDisabled
                                                                font.pixelSize: Theme.fontSizeMD
                                                                visible: seqIndex < (otherLayerCanvas.parent.seqOrder.length - 1)
                                                            }
                                                        }

                                                        Rectangle {
                                                            anchors.fill: parent
                                                            radius: 5
                                                            color: "transparent"
                                                            border.width: 1
                                                            border.color: pillOLDrop.containsDrag ? Theme.accent : (pillOLDragMA.containsMouse ? Theme.borderActive : "transparent")
                                                            opacity: pillOLDragMA.drag.active ? 0.3 : (pillOLDrop.containsDrag ? 0.15 : 0)
                                                        }

                                                        DropArea {
                                                            id: pillOLDrop
                                                            anchors.fill: parent
                                                            keys: ["other-layer-seq-" + otherLayerCanvas.parent.entryIdx]
                                                            onDropped: function(drop) {
                                                                if (drop.source !== pillOLRow.parent && drop.source.seqIndex !== undefined) {
                                                                    otherLayerCanvas.parent.swapItems(drop.source.seqIndex, seqIndex)
                                                                }
                                                                drop.accepted = true
                                                            }
                                                        }

                                                        MouseArea {
                                                            id: pillOLDragMA
                                                            anchors.fill: parent
                                                            cursorShape: Qt.OpenHandCursor
                                                            drag.target: parent
                                                            drag.axis: Drag.XAndYAxis
                                                            hoverEnabled: true
                                                            onReleased: function() {
                                                                parent.x = 0
                                                                parent.y = 0
                                                                parent.Drag.drop()
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // 添加层范围条目按钮
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    height: 24
                                    width: addSeqBtnText.implicitWidth + 16
                                    radius: 5
                                    color: addSeqBtnMA.containsMouse ? Theme.accent : Theme.bgElevated
                                    border.color: Theme.accent
                                    Text { id: addSeqBtnText; anchors.centerIn: parent; text: qsTr("+ 添加层范围"); color: addSeqBtnMA.containsMouse ? Theme.textOnAccent : Theme.accent; font.pixelSize: Theme.fontSizeXS }
                                    TapHandler { id: addSeqBtnMA; onTapped: {
                                        var count = root.editorVm ? root.editorVm.plateOtherLayersSeqCount(settingsDlg.plateIndex) : 0
                                        var beginL = 2
                                        var endL = 100
                                        // 推断下一个范围的起始层
                                        if (count > 0) {
                                            beginL = (root.editorVm.plateOtherLayersSeqEnd(settingsDlg.plateIndex, count - 1) || 99) + 1
                                            endL = beginL + 100
                                        }
                                        if (root.editorVm) root.editorVm.addPlateOtherLayersSeqEntry(settingsDlg.plateIndex, beginL, endL)
                                    }}
                                }
                            }
                        }
                    }
                }

                // 确认按钮
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        height: 30
                        width: psOkText.implicitWidth + 24
                        radius: 6
                        color: Theme.accent
                        Text { id: psOkText; anchors.centerIn: parent; text: qsTr("确定"); color: Theme.textOnAccent; font.pixelSize: Theme.fontSizeMD; font.bold: true }
                        TapHandler {
                            onTapped: {
                                if (root.editorVm) {
                                    if (psNameField.text.length > 0)
                                        root.editorVm.renamePlate(settingsDlg.plateIndex, psNameField.text)
                                }
                                settingsDlg.close()
                            }
                        }
                    }
                }
            }

            onOpened: {
                if (root.editorVm) {
                    bedTypeCombo.currentIndex = root.editorVm.plateBedType(settingsDlg.plateIndex)
                    printSeqCombo.currentIndex = root.editorVm.platePrintSequence(settingsDlg.plateIndex)
                    spiralCombo.currentIndex = root.editorVm.plateSpiralMode(settingsDlg.plateIndex)
                    firstLayerSeqChoiceCombo.currentIndex = root.editorVm.plateFirstLayerSeqChoice(settingsDlg.plateIndex)
                    otherLayersSeqChoiceCombo.currentIndex = root.editorVm.plateOtherLayersSeqChoice(settingsDlg.plateIndex)
                }
                psNameField.forceActiveFocus()
            }
            onClosed: destroy()
        }
    }

    PrintDialog {
        id: printDlg
        editorVm: root.editorVm
    }

    // 排列设置弹出面板（对齐上游 ArrangeSettings popup）
    CxPopup {
        id: arrangeSettingsPopup
        anchors.centerIn: parent
        width: 320
        padding: 20
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        ColumnLayout {
            anchors.fill: parent
            spacing: 14

            // 标题行
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: qsTr("排列设置")
                    font.pixelSize: Theme.fontSizeLG
                    font.bold: true
                    color: Theme.textPrimary
                    Layout.fillWidth: true
                }
                CxIconButton {
                    buttonSize: 26
                    iconSize: 14
                    iconSource: "qrc:/qml/assets/icons/x.svg"
                    onClicked: arrangeSettingsPopup.close()
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

            // 对象间距
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Text {
                    text: qsTr("对象间距")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                    Layout.preferredWidth: 90
                }
                CxTextField {
                    id: arrangeDistField
                    Layout.fillWidth: true
                    text: root.editorVm ? (root.editorVm.arrangeDistance > 0 ? root.editorVm.arrangeDistance.toFixed(1) : "0") : "0"
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: Theme.fontSizeMD
                    selectByMouse: true
                    validator: DoubleValidator { bottom: 0; top: 100 }
                    onAccepted: {
                        if (root.editorVm) root.editorVm.arrangeDistance = parseFloat(text) || 0
                    }
                    onEditingFinished: {
                        if (root.editorVm) root.editorVm.arrangeDistance = parseFloat(text) || 0
                    }
                }
                Text {
                    text: qsTr("mm (0=自动)")
                    color: Theme.textDisabled
                    font.pixelSize: Theme.fontSizeSM
                }
            }

            // 自动旋转
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Text {
                    text: qsTr("自动旋转")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                    Layout.fillWidth: true
                }
                CxSwitch {
                    checked: root.editorVm ? root.editorVm.arrangeRotation : false
                    onToggled: { if (root.editorVm) root.editorVm.arrangeRotation = checked }
                }
            }

            // 对齐 Y 轴
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Text {
                    text: qsTr("对齐 Y 轴")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                    Layout.fillWidth: true
                }
                CxSwitch {
                    checked: root.editorVm ? root.editorVm.arrangeAlignY : false
                    enabled: root.editorVm ? !root.editorVm.arrangeRotation : true
                    onToggled: { if (root.editorVm) root.editorVm.arrangeAlignY = checked }
                }
            }

            // 允许多耗材同板
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Text {
                    text: qsTr("允许多耗材")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                    Layout.fillWidth: true
                }
                CxSwitch {
                    checked: root.editorVm ? root.editorVm.arrangeMultiMaterial : true
                    onToggled: { if (root.editorVm) root.editorVm.arrangeMultiMaterial = checked }
                }
            }

            // 避免校准区域
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Text {
                    text: qsTr("避免校准区域")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeMD
                    Layout.fillWidth: true
                }
                CxSwitch {
                    checked: root.editorVm ? root.editorVm.arrangeAvoidCalibration : true
                    onToggled: { if (root.editorVm) root.editorVm.arrangeAvoidCalibration = checked }
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

            // 按钮行
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("重置默认")
                    onClicked: { if (root.editorVm) { root.editorVm.resetArrangeSettings(); arrangeDistField.text = "0"; } }
                }
                CxButton {
                    text: qsTr("排列")
                    cxStyle: CxButton.Style.Primary
                    onClicked: {
                        if (root.editorVm) root.editorVm.arrangeDistance = parseFloat(arrangeDistField.text) || 0
                        if (root.editorVm) root.editorVm.arrangeAllObjects()
                        arrangeSettingsPopup.close()
                    }
                }
            }
        }
    }

    FileDialog {
        id: openFileDlg
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
            if (root.editorVm)
                root.editorVm.loadFile(selectedFile.toString())
        }
    }

    FileDialog {
        id: exportGCodeDlg
        title: qsTr("导出 G-code")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("G-code 文件 (*.gcode)")]
        defaultSuffix: "gcode"
        currentFile: root.editorVm ? root.editorVm.defaultExportGCodeFileName() : "output.gcode"
        onAccepted: {
            if (root.editorVm)
                root.editorVm.requestExportGCode(selectedFile.toString())
        }
    }

    // Replace selected object with STL (对齐上游 append_menu_item_replace_with_stl)
    FileDialog {
        id: replaceWithStlDlg
        title: qsTr("替换为 STL")
        nameFilters: [
            qsTr("STL 文件 (*.stl)"),
            qsTr("OBJ 文件 (*.obj)"),
            qsTr("所有文件 (*)")
        ]
        onAccepted: {
            if (root.editorVm)
                root.editorVm.replaceWithStl(selectedFile.toString())
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.bgBase

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // ═══ Left sidebar (280px) ═══
            // Phase 4: DockableSidebar 替代固定 LeftSidebar
            // 折叠/宽度/dockArea 由 backend 统一管理 + 持久化 (ARCH-08/09/10)
            // dockArea=Right 时通过 Layout 顺序镜像 (见下方 RowLayout 逻辑)
            DockableSidebar {
                id: leftSidebar
                Layout.fillHeight: true
                Layout.preferredWidth: root.sidebarCollapsed ? 0 : root.sidebarWidth
                Layout.topMargin: 14
                Layout.bottomMargin: 14
                // 折叠时完全收起 (宽度 0 + 隐藏), viewportArea 独占
                collapsed: root.sidebarCollapsed
                sidebarWidth: root.sidebarWidth
                minWidth: root.sidebarMinWidth
                maxWidth: root.sidebarMaxWidth
                editorVm: root.editorVm
                configVm: root.configVm
                processCategory: root.processCategory
                _isRightDocked: root.sidebarDockArea === 1   // sdaRight
                toggleRequested: root.sidebarToggleRequested
                widthChanged: root.sidebarWidthChanged
                onExportRequested: root.openExportDialog()
                visible: root.leftPanelVisible
            }

            // ═══ Center: 3D Viewport + overlays ═══
            Item {
                id: viewportArea
                Layout.fillWidth: true
                Layout.fillHeight: true

                // ── Phase G4: 3D 视口工具栏 overlay ──
                GLToolbars {
                    id: glToolbars
                    anchors.fill: parent
                    z: 100
                    editorVm: root.editorVm
                    viewport3d: viewport3d
                    viewControlsBottomMargin: root.prepareBottomViewControlsBottomMargin
                    onAddModelRequested: openFileDlg.open()
                    onFitViewRequested: root.applyFitHintIfReady()
                    onSliceRequested: if (root.editorVm) root.editorVm.requestSlice()
                }
                GLViewport {
                    id: viewport3d
                    anchors.fill: parent
                    z: 0
                    canvasType: GLViewport.CanvasView3D
                    meshData: root.editorVm ? root.editorVm.meshData : null
                    bedWidth: root.editorVm ? root.editorVm.bedWidth : 220
                    bedDepth: root.editorVm ? root.editorVm.bedDepth : 220
                    bedOriginX: root.editorVm ? root.editorVm.bedOriginX : 0
                    bedOriginY: root.editorVm ? root.editorVm.bedOriginY : 0
                    bedShapeType: root.editorVm ? root.editorVm.bedShapeType : 0
                    bedDiameter: root.editorVm ? root.editorVm.bedDiameter : 220
                    // Phase 100 (WTREAD-01): wire the real post-slice wipe-tower
                    // geometry (from Print::wipe_tower_data() via SliceService ->
                    // EditorViewModel) into the GLViewport Q_PROPERTYs so the real
                    // dims reach the renderer's m_wipeTowerDirty rebuild path
                    // (RhiViewportRenderer::uploadWipeTowerBuffer, .cpp:1064-1095).
                    // Closes the WT-VIEWPORT-DEFAULTS unbound gap (99-GAP-MATRIX).
                    // showWipeTower is the WTREAD-02 gate (has_wipe_tower()):
                    // false on single-material slices so no placeholder box
                    // leaks. Defaults match RhiViewport.h:304-309 (10/10/50/100/25)
                    // for the null-editorVm / pre-slice path.
                    showWipeTower: root.editorVm ? root.editorVm.showWipeTower : false
                    wipeTowerWidth: root.editorVm ? root.editorVm.wipeTowerWidth : 10
                    wipeTowerDepth: root.editorVm ? root.editorVm.wipeTowerDepth : 10
                    wipeTowerHeight: root.editorVm ? root.editorVm.wipeTowerHeight : 50
                    wipeTowerX: root.editorVm ? root.editorVm.wipeTowerX : 100
                    wipeTowerZ: root.editorVm ? root.editorVm.wipeTowerZ : 25
                    // Phase 109 (WTMESH-01/02): Option B real-mesh bindings.
                    // hasRealMesh gates the renderer branch (true -> real
                    // convex-hull mesh from the slice engine; false -> Option A
                    // dimensioned box, Phase 99 Frozen Decision 2 baseline).
                    // meshVertices carries the flattened XYZ triples (libslic3r
                    // world frame). Default hasRealMesh=false so the pre-slice
                    // and single-material paths take Option A.
                    wipeTowerHasRealMesh: root.editorVm ? root.editorVm.wipeTowerHasRealMesh : false
                    wipeTowerMeshVertices: root.editorVm ? root.editorVm.wipeTowerMeshVertices : []
                    // Phase 121 (PAINT-02/PAINT-03): paint overlay reverse-channel
                    // + brush params. paintOverlayData carries the flattened
                    // painted-facet byte stream from EditorViewModel (bound for
                    // both the RHI + Software paths). The brush params (radius,
                    // cursor type, paint state) are sourced per-gizmoMode from the
                    // matching editorVm brush property set. brushMouseScreenX/Y +
                    // brushButtonState are driven by the C++ mouse handlers inside
                    // RhiViewport (updateBrushCursorState) -- NOT bound from QML.
                    paintOverlayData: root.editorVm ? root.editorVm.paintOverlayData : null
                    extrudersColors: root.editorVm ? root.editorVm.extrudersColors : []
                    brushRadius: root.editorVm ? _activeBrushRadius() : 2
                    brushCursorType: root.editorVm ? _activeBrushCursorType() : 1
                    paintState: root.editorVm ? _activePaintState() : 1
                    currentPlateIndex: root.editorVm ? root.editorVm.currentPlateIndex : 0
                    plateCount: root.editorVm ? root.editorVm.plateCount : 0
                    activePlateObjectIndices: root.editorVm ? root.editorVm.activePlateObjectIndices : []
                    meshBatchSourceObjectIndices: root.editorVm ? root.editorVm.meshBatchSourceObjectIndices : []
                    selectedSourceObjectIndex: root.editorVm ? root.editorVm.selectedSourceObjectIndex : -1
                    onObjectPickedSource: function(sourceIndex) {
                        if (root.editorVm)
                            root.editorVm.selectSourceObject(sourceIndex)
                    }
                    onGizmoDragBegin: {
                        if (root.editorVm) {
                            root.activeGizmoDragMode = viewport3d.gizmoMode
                            if (root.activeGizmoDragMode === GLViewport.GizmoRotate)
                                root.editorVm.beginGizmoRotateDrag()
                            else if (root.activeGizmoDragMode === GLViewport.GizmoScale)
                                root.editorVm.beginGizmoScaleDrag()
                            else
                                root.editorVm.beginGizmoMoveDrag()
                        }
                    }
                    onGizmoMoveRequested: function(worldDelta) {
                        if (root.editorVm)
                            root.editorVm.applyGizmoMoveDelta(worldDelta.x, worldDelta.y, worldDelta.z)
                    }
                    onGizmoRotateRequested: function(axis, radians) {
                        if (root.editorVm)
                            root.editorVm.applyGizmoRotateDelta(axis, radians)
                    }
                    onGizmoScaleRequested: function(axis, factor) {
                        if (root.editorVm)
                            root.editorVm.applyGizmoScaleFactor(axis, factor)
                    }
                    onGizmoDragEnd: {
                        if (root.editorVm) {
                            if (root.activeGizmoDragMode === GLViewport.GizmoRotate)
                                root.editorVm.endGizmoRotateDrag()
                            else if (root.activeGizmoDragMode === GLViewport.GizmoScale)
                                root.editorVm.endGizmoScaleDrag()
                            else
                                root.editorVm.endGizmoMoveDrag()
                        }
                    }
                    // Phase 115 (MEASURE-04): wire the snap UX (mouse-move ->
                    // stage-2 pick -> getFeature -> Shift toggle -> visual
                    // feedback + two-click measurement). The RhiViewport emits
                    // these only while gizmoMode === GizmoMeasure; the ViewModel
                    // runs the two-stage pick (Phase 113 stage-2) +
                    // MeasureEngine::getFeature (Phase 114) and updates the
                    // measure* readouts live. Shift forces PointSelection;
                    // default is FeatureSelection (mirrors upstream
                    // GLGizmoMeasure.cpp:409-442).
                    //
                    // The handler forwards the world-space pick inputs opaquely
                    // to the ViewModel -- no picking or geometry-hit logic lives
                    // in QML (the rhiViewportSelectionPickingBridgeStaysCppOwned
                    // audit enforces this). Parameter names mirror the C++ signal.
                    onMeasurePickRequested: function(worldOrigin, worldDirection, pickedSourceIndex, shiftHeld) {
                        if (root.editorVm)
                            root.editorVm.pickMeasureFeatureAt(worldOrigin, worldDirection, pickedSourceIndex, shiftHeld)
                    }
                    onMeasureHoverLeft: {
                        if (root.editorVm)
                            root.editorVm.clearMeasureReadout()
                    }
                    // Phase 120 (PAINT-01): forward the paint-gizmo pick (brush
                    // down/drag) to the ViewModel. Same opaque-forward contract
                    // as onMeasurePickRequested -- no picking or geometry-hit
                    // logic in QML. Parameter names mirror the C++ signal.
                    onPaintPickRequested: function(worldOrigin, worldDirection, pickedSourceIndex, brushRadius, cursorType, paintState) {
                        if (root.editorVm)
                            root.editorVm.paintAtFacet(-1, -1, -1, 0.0, 0.0, 0.0, paintState, brushRadius, cursorType, pickedSourceIndex, worldOrigin, worldDirection)
                    }
                    cutAxis: root.editorVm ? root.editorVm.cutAxis : 2
                    cutPosition: root.editorVm ? root.editorVm.cutPosition : 0.0

                    // Right-click context menu dispatch
                    // 对齐 upstream Plater::priv::on_right_click (Plater.cpp:9512-9566)
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.RightButton
                        propagateComposedEvents: true
                        onClicked: (mouse) => {
                            if (root.editorVm && root.editorVm.selectedObjectIndex >= 0) {
                                // Single object selected → object menu
                                objectContextMenu.popup(mouse.x, mouse.y)
                            } else if (root.editorVm && root.editorVm.selectedObjectCount > 1) {
                                // Multiple objects selected → multi-selection menu (对齐上游 multi_selection_menu)
                                multiContextMenu.popup(mouse.x, mouse.y)
                            } else {
                                // Nothing selected (empty canvas) → default menu
                                defaultContextMenu.popup(mouse.x, mouse.y)
                            }
                            mouse.accepted = false
                        }
                        onPressed: (mouse) => mouse.accepted = false
                        onReleased: (mouse) => mouse.accepted = false
                    }

                    // Undo/Redo shortcuts (QML Shortcuts work better than Keys for Ctrl combos)
                    Shortcut {
                        sequences: ["Ctrl+Z"]
                        onActivated: viewport3d.undo()
                    }
                    Shortcut {
                        sequences: ["Ctrl+Shift+Z", "Ctrl+Y"]
                        onActivated: viewport3d.redo()
                    }

                    Connections {
                        target: root.editorVm
                        function onStateChanged() {
                            root.applyFitHintIfReady()
                        }
                    }

                    onVisibleChanged: {
                        if (visible)
                            Qt.callLater(root.applyFitHintIfReady)
                    }

                    Component.onCompleted: Qt.callLater(root.applyFitHintIfReady)

                    DropArea {
                        anchors.fill: parent
                        keys: ["text/uri-list"]
                        onDropped: (drop) => {
                            if (drop.hasUrls && drop.urls.length > 0 && root.editorVm)
                                root.editorVm.loadFile(drop.urls[0].toString())
                        }

                        Rectangle {
                            anchors.fill: parent
                            color: "#4a0b1018"
                            visible: parent.containsDrag

                            Rectangle {
                                anchors.centerIn: parent
                                width: 260
                                height: 64
                                radius: Theme.radiusXL
                                color: Theme.bgPanel
                                border.width: 1
                                border.color: Theme.accent

                                Text {
                                    anchors.centerIn: parent
                                    text: qsTr("松开以导入模型")
                                    color: Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeXL
                                    font.bold: true
                                }
                            }
                        }
                    }
                }

        // ── 视口告警覆盖层（对齐上游 EWarning / Plater::_set_warning_notification）──
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 10
            width: warningContent.implicitWidth + 28
            height: warningContent.implicitHeight + 16
            radius: 8
            color: root.editorVm && root.editorVm.viewportWarning === 2 ? Theme.bgErrorSubtle : Theme.bgWarningSubtle
            border.width: 1
            border.color: root.editorVm && root.editorVm.viewportWarning === 2 ? Theme.statusError : Theme.statusWarning
            visible: root.editorVm ? root.editorVm.hasViewportWarning : false
            opacity: visible ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 200 } }

            RowLayout {
                id: warningContent
                anchors.centerIn: parent
                spacing: 8

                Text {
                    text: root.editorVm && root.editorVm.viewportWarning === 2 ? "⚠" : "⚡"
                    font.pixelSize: Theme.fontSizeLG
                    color: root.editorVm && root.editorVm.viewportWarning === 2 ? Theme.statusError : Theme.statusWarning
                }

                Text {
                    text: root.editorVm ? root.editorVm.viewportWarningMessage : ""
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                }
            }
        }

        // Gradient overlay removed — was blocking GL FBO rendering

        // 支撑/缝线绘制信息面板（对齐上游 GLGizmoFdmSupports info panel）
        Rectangle {
            id: transformMiniPanel
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: transformMiniContent.implicitWidth + 24
            height: transformMiniContent.implicitHeight + 14
            radius: 6
            color: Theme.bgFloating
            border.width: 1
            border.color: Theme.borderSubtle
            visible: root.editorVm
                     && (viewport3d.gizmoMode === GLViewport.GizmoMove
                         || viewport3d.gizmoMode === GLViewport.GizmoRotate
                         || viewport3d.gizmoMode === GLViewport.GizmoScale)

            RowLayout {
                id: transformMiniContent
                anchors.centerIn: parent
                spacing: 12

                Text {
                    text: viewport3d.gizmoMode === GLViewport.GizmoMove
                          ? qsTr("Move")
                          : viewport3d.gizmoMode === GLViewport.GizmoRotate
                            ? qsTr("Rotate")
                            : qsTr("Scale")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                }

                TransformMetric {
                    axisName: "X"
                    accentColor: "#e066a0"
                    valueText: viewport3d.gizmoMode === GLViewport.GizmoMove
                               ? root.editorVm.objectPosX.toFixed(1)
                               : viewport3d.gizmoMode === GLViewport.GizmoRotate
                                 ? root.editorVm.objectRotX.toFixed(1)
                                 : root.editorVm.objectScaleX.toFixed(2)
                }
                TransformMetric {
                    axisName: "Y"
                    accentColor: Theme.textTertiary
                    valueText: viewport3d.gizmoMode === GLViewport.GizmoMove
                               ? root.editorVm.objectPosY.toFixed(1)
                               : viewport3d.gizmoMode === GLViewport.GizmoRotate
                                 ? root.editorVm.objectRotY.toFixed(1)
                                 : root.editorVm.objectScaleY.toFixed(2)
                }
                TransformMetric {
                    axisName: "Z"
                    accentColor: Theme.statusInfo
                    valueText: viewport3d.gizmoMode === GLViewport.GizmoMove
                               ? root.editorVm.objectPosZ.toFixed(1)
                               : viewport3d.gizmoMode === GLViewport.GizmoRotate
                                 ? root.editorVm.objectRotZ.toFixed(1)
                                 : root.editorVm.objectScaleZ.toFixed(2)
                }
            }
        }

        // Phase 121 (PAINT-03/OV-07): Support paint panel with brush controls.
        // Modeled on the seam panel below: tool selector (Enforcer/Blocker),
        // radius CxSlider, cursor-type combo, plus the count readouts + clear
        // button. Visible only on GizmoSupportPaint (seam has its own panel).
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: supportPaintContent.implicitWidth + 24
            height: supportPaintContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoSupportPaint && root.editorVm

            ColumnLayout {
                id: supportPaintContent
                anchors.centerIn: parent
                spacing: 6

                Text {
                    text: qsTr("支撑绘制")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                // Count readouts (enforced / blocked).
                RowLayout {
                    spacing: 12
                    Layout.alignment: Qt.AlignHCenter
                    Text {
                        text: qsTr("已强制: %1").arg(root.editorVm.enforcedSupportCount)
                        color: Theme.accent
                        font.pixelSize: Theme.fontSizeSM
                    }
                    Text {
                        text: qsTr("已阻止: %1").arg(root.editorVm.blockedSupportCount)
                        color: Theme.statusWarning
                        font.pixelSize: Theme.fontSizeSM
                    }
                }

                // Tool selector (对齐上游 GLGizmoFdmSupports m_current_tool):
                // 1=Enforcer (green), 2=Blocker (red). Bound to supportPaintTool.
                Row {
                    spacing: 4
                    Layout.alignment: Qt.AlignHCenter
                    Repeater {
                        model: [qsTr("强制"), qsTr("阻止")]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: 72; height: 24; radius: 4
                            color: root.editorVm && root.editorVm.supportPaintTool === (index + 1) ? Theme.chromePressed : Theme.bgPanel
                            border.color: root.editorVm && root.editorVm.supportPaintTool === (index + 1) ? Theme.statusInfo : Theme.bgHover
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.supportPaintTool === (index + 1) ? Theme.statusInfo : Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (root.editorVm) root.editorVm.supportPaintTool = index + 1
                            }
                        }
                    }
                }

                // Cursor radius (对齐上游 brush radius slider).
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("半径:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: 0.05; to: 20; stepSize: 0.1
                        value: root.editorVm ? root.editorVm.supportPaintCursorRadius : 2
                        implicitWidth: 100
                        onMoved: if (root.editorVm) root.editorVm.supportPaintCursorRadius = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.supportPaintCursorRadius.toFixed(1) : "2.0"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 30
                    }
                }

                // Cursor type combo (Sphere=1 / Circle=0, 对齐 PaintCursorType).
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("光标:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    Repeater {
                        model: [{label: qsTr("球体"), val: 1}, {label: qsTr("圆形"), val: 0}]
                        delegate: Rectangle {
                            required property var modelData
                            width: 56; height: 22; radius: 4
                            color: root.editorVm && root.editorVm.supportPaintCursorType === modelData.val ? Theme.chromePressed : Theme.bgPanel
                            border.color: root.editorVm && root.editorVm.supportPaintCursorType === modelData.val ? Theme.statusInfo : Theme.bgHover
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData.label
                                color: root.editorVm && root.editorVm.supportPaintCursorType === modelData.val ? Theme.statusInfo : Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (root.editorVm) root.editorVm.supportPaintCursorType = modelData.val
                            }
                        }
                    }
                }

                // Clear button.
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 80; height: 24; radius: 4
                    color: Theme.bgElevated
                    border.color: Theme.borderDefault; border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("清除全部")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (root.editorVm) root.editorVm.clearAllPaintData()
                    }
                }
            }
        }

        // 测量信息面板（对齐上游 GLGizmoMeasure::on_render_input_window）
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: measureContent.implicitWidth + 24
            height: measureContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoMeasure && root.editorVm

            ColumnLayout {
                id: measureContent
                anchors.centerIn: parent
                spacing: 4

                // 测量拾取模式（对齐上游 GLGizmoMeasure selection mode）
                Row {
                    spacing: 4
                    Layout.alignment: Qt.AlignHCenter
                    Repeater {
                        model: [qsTr("点测量"), qsTr("特征测量")]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: 60; height: 22; radius: 4
                            color: root.editorVm && root.editorVm.measureSelectionMode === index ? Theme.chromePressed : Theme.bgPanel
                            border.color: root.editorVm && root.editorVm.measureSelectionMode === index ? Theme.statusInfo : Theme.bgHover
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.measureSelectionMode === index ? Theme.statusInfo : Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (root.editorVm) root.editorVm.measureSelectionMode = index
                            }
                        }
                    }
                }

                Row {
                    spacing: 16
                    Label { text: qsTr("X:"); color: "#e066a0"; font.pixelSize: Theme.fontSizeSM; font.bold: true; font.family: "Consolas, monospace" }
                    Label { text: root.editorVm ? root.editorVm.measureDimensions.x.toFixed(1) : "0.0"; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; font.family: "Consolas, monospace" }
                    Label { text: qsTr("Y:"); color: Theme.textTertiary; font.pixelSize: Theme.fontSizeSM; font.bold: true; font.family: "Consolas, monospace" }
                    Label { text: root.editorVm ? root.editorVm.measureDimensions.y.toFixed(1) : "0.0"; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; font.family: "Consolas, monospace" }
                    Label { text: qsTr("Z:"); color: Theme.statusInfo; font.pixelSize: Theme.fontSizeSM; font.bold: true; font.family: "Consolas, monospace" }
                    Label { text: root.editorVm ? root.editorVm.measureDimensions.z.toFixed(1) : "0.0"; color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; font.family: "Consolas, monospace" }
                }
                Label {
                    text: root.editorVm
                        ? (qsTr("体积: ") + root.editorVm.measureDimensions.w.toFixed(0) + qsTr(" mm³"))
                        : ""
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSizeXS
                    font.family: "Consolas, monospace"
                    Layout.alignment: Qt.AlignHCenter
                }
                // Phase 115 (MEASURE-04): the picked-feature readouts. Surfaces
                // the MeasureEngine angle / perpendicular distance / direct
                // distance live as the cursor moves (MS-03 visual feedback).
                // Mirrors the upstream GLGizmoMeasure.cpp:1990-2048 readout
                // rows. Empty until the second click completes the measure.
                Label {
                    visible: root.editorVm && root.editorVm.measureReadoutValid
                    text: root.editorVm && root.editorVm.measureAngleText
                          ? (qsTr("角度: ") + root.editorVm.measureAngleText)
                          : ""
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.family: "Consolas, monospace"
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    visible: root.editorVm && root.editorVm.measureReadoutValid
                    text: root.editorVm && root.editorVm.measurePerpendicularDistanceText
                          ? (qsTr("垂直距离: ") + root.editorVm.measurePerpendicularDistanceText)
                          : ""
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.family: "Consolas, monospace"
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    visible: root.editorVm && root.editorVm.measureReadoutValid
                    text: root.editorVm && root.editorVm.measureDirectDistanceText
                          ? (qsTr("直线距离: ") + root.editorVm.measureDirectDistanceText)
                          : ""
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.family: "Consolas, monospace"
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: root.editorVm && root.editorVm.measureSelectionMode === 1
                        ? qsTr("点击网格面拾取特征 (点/边/圆/平面)")
                        : qsTr("点测量模式 — 显示选中对象尺寸")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeXS
                    Layout.alignment: Qt.AlignHCenter
                }
                // Phase 115 (MEASURE-04): the live hover-feature indicator.
                // Surfaces the FeatureKind of the feature currently under the
                // cursor (MS-03 visual feedback) + the Shift-toggle hint.
                Label {
                    visible: viewport3d.gizmoMode === GLViewport.GizmoMeasure && root.editorVm
                    text: {
                        if (!root.editorVm || root.editorVm.measureHoverFeatureKind === 0)
                            return qsTr("悬停特征: 无")
                        // OWzx::FeatureKind: 1=Point, 2=Edge, 4=Circle, 8=Plane
                        switch (root.editorVm.measureHoverFeatureKind) {
                            case 1: return qsTr("悬停特征: 点")
                            case 2: return qsTr("悬停特征: 边")
                            case 4: return qsTr("悬停特征: 圆")
                            case 8: return qsTr("悬停特征: 平面")
                            default: return qsTr("悬停特征: 未知")
                        }
                    }
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeXS
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    visible: viewport3d.gizmoMode === GLViewport.GizmoMeasure
                    text: qsTr("Shift = 点测量 (对齐上游 GLGizmoMeasure)")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeXS
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // Flatten 信息面板（对齐上游 GLGizmoFlatten::on_render_input_window）
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: flattenContent.implicitWidth + 32
            height: flattenContent.implicitHeight + 20
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoFlatten && root.editorVm

            ColumnLayout {
                id: flattenContent
                anchors.centerIn: parent
                spacing: 8

                Text {
                    text: qsTr("平放至面")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: root.editorVm ? (qsTr("候选面: ") + root.editorVm.flattenFaceCount) : ""
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSizeSM
                    Layout.alignment: Qt.AlignHCenter
                }
                Row {
                    spacing: 8
                    Layout.alignment: Qt.AlignHCenter
                    Rectangle {
                        width: 80; height: 28; radius: 4
                        color: Theme.accent
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("平放")
                            color: "white"
                            font.pixelSize: Theme.fontSizeSM
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.editorVm.flattenSelected()
                        }
                    }
                    Text {
                        text: qsTr("(G)")
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Text {
                    text: qsTr("将选中对象最大面朝下平放")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeXS
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // Cut 切割控制面板（对齐上游 GLGizmoCut::on_render_input_window）
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: cutContent.implicitWidth + 32
            height: cutContent.implicitHeight + 20
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoCut && root.editorVm

            ColumnLayout {
                id: cutContent
                anchors.centerIn: parent
                spacing: 8

                Text {
                    text: qsTr("切割对象")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                // 切割轴选择
                Row {
                    spacing: 4
                    Layout.alignment: Qt.AlignHCenter
                    Repeater {
                        model: [qsTr("X 轴"), qsTr("Y 轴"), qsTr("Z 轴")]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: 58; height: 26; radius: 4
                            color: root.editorVm && root.editorVm.cutAxis === index ? Theme.chromePressed : Theme.bgPanel
                            border.color: root.editorVm && root.editorVm.cutAxis === index ? Theme.accent : Theme.bgHover
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.cutAxis === index ? Theme.accent : Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (root.editorVm) root.editorVm.cutAxis = index
                            }
                        }
                    }
                }

                // 切割模式（对齐上游 GLGizmoCut Planar/TongueAndGroove）
                Row {
                    spacing: 4
                    Layout.alignment: Qt.AlignHCenter
                    Repeater {
                        model: [qsTr("平面切割"), qsTr("舌槽模式")]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: 70; height: 24; radius: 4
                            color: root.editorVm && root.editorVm.cutMode === index ? Theme.chromePressed : Theme.bgPanel
                            border.color: root.editorVm && root.editorVm.cutMode === index ? Theme.statusInfo : Theme.bgHover
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.cutMode === index ? Theme.statusInfo : Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (root.editorVm) root.editorVm.cutMode = index
                            }
                        }
                    }
                }

                // 连接器类型（对齐上游 GLGizmoCut connector type: Plug/Dowel/Snap）
                Row {
                    spacing: 4
                    Layout.alignment: Qt.AlignHCenter
                    visible: root.editorVm ? root.editorVm.cutMode === 1 : false
                    Text { text: qsTr("类型:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS; anchors.verticalCenter: parent.verticalCenter }
                    Repeater {
                        model: [qsTr("Plug"), qsTr("Dowel"), qsTr("Snap")]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: 52; height: 22; radius: 4
                            color: root.editorVm && root.editorVm.connectorType === index ? Theme.chromePressed : Theme.bgPanel
                            border.color: root.editorVm && root.editorVm.connectorType === index ? Theme.statusWarning : Theme.bgHover
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.connectorType === index ? Theme.statusWarning : Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (root.editorVm) root.editorVm.connectorType = index
                            }
                        }
                    }
                }

                // 连接器样式（对齐上游 GLGizmoCut connector style: Prism/Frustum）
                Row {
                    spacing: 4
                    Layout.alignment: Qt.AlignHCenter
                    visible: root.editorVm ? root.editorVm.cutMode === 1 : false
                    enabled: root.editorVm ? root.editorVm.connectorType === 0 : false
                    opacity: enabled ? 1.0 : 0.45
                    Text { text: qsTr("样式:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS; anchors.verticalCenter: parent.verticalCenter }
                    Repeater {
                        model: [qsTr("Prism"), qsTr("Frustum")]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: 56; height: 22; radius: 4
                            color: root.editorVm && root.editorVm.connectorStyle === index ? Theme.chromePressed : Theme.bgPanel
                            border.color: root.editorVm && root.editorVm.connectorStyle === index ? Theme.statusWarning : Theme.bgHover
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.connectorStyle === index ? Theme.statusWarning : Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                            MouseArea {
                                anchors.fill: parent
                                enabled: root.editorVm ? root.editorVm.connectorType === 0 : false
                                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: if (root.editorVm) root.editorVm.connectorStyle = index
                            }
                        }
                    }
                }

                // 连接器形状（对齐上游 GLGizmoCut connector shape: Triangle/Square/Hexagon/Circle）
                Row {
                    spacing: 3
                    Layout.alignment: Qt.AlignHCenter
                    visible: root.editorVm ? root.editorVm.cutMode === 1 : false
                    enabled: root.editorVm ? root.editorVm.connectorType !== 2 : false
                    opacity: enabled ? 1.0 : 0.45
                    Text { text: qsTr("形状:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS; anchors.verticalCenter: parent.verticalCenter }
                    Repeater {
                        model: ["△", "□", "⬡", "○"]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: 28; height: 22; radius: 4
                            color: root.editorVm && root.editorVm.connectorShape === index ? Theme.chromePressed : Theme.bgPanel
                            border.color: root.editorVm && root.editorVm.connectorShape === index ? Theme.statusWarning : Theme.bgHover
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.connectorShape === index ? Theme.statusWarning : Theme.textTertiary
                                font.pixelSize: Theme.fontSizeSM
                            }
                            MouseArea {
                                anchors.fill: parent
                                enabled: root.editorVm ? root.editorVm.connectorType !== 2 : false
                                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: if (root.editorVm) root.editorVm.connectorShape = index
                            }
                        }
                    }
                }

                // 连接器尺寸/深度（对齐上游 connector size/depth_ratio）
                RowLayout {
                    spacing: 8
                    Layout.alignment: Qt.AlignHCenter
                    visible: root.editorVm ? root.editorVm.cutMode === 1 : false
                    Text { text: qsTr("尺寸:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: 2; to: 20; stepSize: 0.5
                        value: root.editorVm ? root.editorVm.connectorSize : 5
                        implicitWidth: 80
                        onMoved: if (root.editorVm) root.editorVm.connectorSize = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.connectorSize.toFixed(1) : "5.0"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 30
                    }
                    Text { text: qsTr("深度:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: 0.1; to: 1.0; stepSize: 0.05
                        value: root.editorVm ? root.editorVm.connectorDepth : 0.5
                        implicitWidth: 60
                        onMoved: if (root.editorVm) root.editorVm.connectorDepth = value
                    }
                    Text {
                        text: root.editorVm ? (root.editorVm.connectorDepth * 100).toFixed(0) + "%" : "50%"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 30
                    }
                }

                // 切割位置滑块
                RowLayout {
                    spacing: 8
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("位置:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: -50; to: 50; stepSize: 0.5
                        value: root.editorVm ? root.editorVm.cutPosition : 0
                        implicitWidth: 120
                        onMoved: if (root.editorVm) root.editorVm.cutPosition = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.cutPosition.toFixed(1) + " mm" : "0.0 mm"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 50
                    }
                }

                // 保留模式
                Row {
                    spacing: 4
                    Layout.alignment: Qt.AlignHCenter
                    Repeater {
                        model: [qsTr("全部保留"), qsTr("保留上半"), qsTr("保留下半")]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: 70; height: 26; radius: 4
                            color: root.editorVm && root.editorVm.cutKeepMode === index ? Theme.chromePressed : Theme.bgPanel
                            border.color: root.editorVm && root.editorVm.cutKeepMode === index ? Theme.accent : Theme.bgHover
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.cutKeepMode === index ? Theme.accent : Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (root.editorVm) root.editorVm.cutKeepMode = index
                            }
                        }
                    }
                }

                // 操作按钮
                Row {
                    spacing: 8
                    Layout.alignment: Qt.AlignHCenter
                    Rectangle {
                        width: 60; height: 28; radius: 4
                        color: Theme.bgElevated
                        border.color: Theme.borderDefault; border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("翻转")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXS
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.editorVm) root.editorVm.flipCutPlane()
                        }
                    }
                    Rectangle {
                        width: 70; height: 28; radius: 4
                        color: Theme.bgElevated
                        border.color: Theme.borderDefault; border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("居中")
                            color: Theme.textPrimary
                            font.pixelSize: Theme.fontSizeXS
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.editorVm) root.editorVm.centerCutPlane()
                        }
                    }
                    Rectangle {
                        width: 80; height: 28; radius: 4
                        color: Theme.accent
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("执行切割")
                            color: "white"
                            font.pixelSize: Theme.fontSizeXS
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (root.editorVm)
                                    root.editorVm.cutSelected(root.editorVm.cutAxis, root.editorVm.cutPosition)
                            }
                        }
                    }
                }
                Text {
                    text: qsTr("(Ctrl+Shift+X)")
                    color: Theme.textTertiary
                    font.pixelSize: Theme.fontSizeXS
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // Seam 缝线绘制控制面板（对齐上游 GLGizmoSeam::on_render_input_window）
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: seamContent.implicitWidth + 24
            height: seamContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoSeamPaint && root.editorVm

            ColumnLayout {
                id: seamContent
                anchors.centerIn: parent
                spacing: 6

                Text {
                    text: qsTr("缝线绘制")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                // 工具选择（对齐上游 GLGizmoSeam m_current_tool）
                Row {
                    spacing: 4
                    Layout.alignment: Qt.AlignHCenter
                    Repeater {
                        model: [qsTr("强制缝线"), qsTr("阻止缝线")]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: 72; height: 24; radius: 4
                            color: root.editorVm && root.editorVm.seamPaintTool === (index + 1) ? Theme.chromePressed : Theme.bgPanel
                            border.color: root.editorVm && root.editorVm.seamPaintTool === (index + 1) ? Theme.statusInfo : Theme.bgHover
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.seamPaintTool === (index + 1) ? Theme.statusInfo : Theme.textTertiary
                                font.pixelSize: Theme.fontSizeXS
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (root.editorVm) root.editorVm.seamPaintTool = index + 1
                            }
                        }
                    }
                }

                // 光标半径
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("半径:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: 0.05; to: 20; stepSize: 0.1
                        value: root.editorVm ? root.editorVm.seamPaintCursorRadius : 2
                        implicitWidth: 100
                        onMoved: if (root.editorVm) root.editorVm.seamPaintCursorRadius = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.seamPaintCursorRadius.toFixed(1) : "2.0"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 30
                    }
                }

                // 清除按钮
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 80; height: 24; radius: 4
                    color: Theme.bgElevated
                    border.color: Theme.borderDefault; border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("清除全部")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (root.editorVm) root.editorVm.clearSeamPaintOnSelection()
                    }
                }
            }
        }

        // Hollow SLA 空洞标记控制面板（对齐上游 GLGizmoHollow::on_render_input_window）
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: hollowContent.implicitWidth + 24
            height: hollowContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoHollow && root.editorVm

            ColumnLayout {
                id: hollowContent
                anchors.centerIn: parent
                spacing: 6

                Text {
                    text: qsTr("SLA 空洞标记")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                // 启用/禁用空洞（对齐上游 GLGizmoHollow m_enable_hollowing）
                RowLayout {
                    spacing: 8
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("启用空洞化:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    Rectangle {
                        width: 36; height: 18; radius: 9
                        color: root.editorVm && root.editorVm.hollowEnabled ? Theme.accent : Theme.borderDefault
                        Rectangle {
                            width: 14; height: 14; radius: 7
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: root.editorVm && root.editorVm.hollowEnabled ? undefined : parent.left
                            anchors.right: root.editorVm && root.editorVm.hollowEnabled ? parent.right : undefined
                            anchors.leftMargin: root.editorVm && root.editorVm.hollowEnabled ? 0 : 2
                            anchors.rightMargin: root.editorVm && root.editorVm.hollowEnabled ? 2 : 0
                            color: "white"
                            Behavior on anchors.left { PropertyAnimation { duration: 150 } }
                            Behavior on anchors.right { PropertyAnimation { duration: 150 } }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.editorVm) root.editorVm.hollowEnabled = !root.editorVm.hollowEnabled
                        }
                    }
                }

                // 钻孔半径（对齐上游 m_new_hole_radius）
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("钻孔半径:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: 0.5; to: 10; stepSize: 0.1
                        value: root.editorVm ? root.editorVm.hollowHoleRadius : 2
                        implicitWidth: 100
                        onMoved: if (root.editorVm) root.editorVm.hollowHoleRadius = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.hollowHoleRadius.toFixed(1) : "2.0"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 30
                    }
                }

                // 钻孔高度（对齐上游 m_new_hole_height）
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("钻孔高度:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: 1; to: 20; stepSize: 0.5
                        value: root.editorVm ? root.editorVm.hollowHoleHeight : 6
                        implicitWidth: 100
                        onMoved: if (root.editorVm) root.editorVm.hollowHoleHeight = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.hollowHoleHeight.toFixed(1) : "6.0"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 30
                    }
                }

                // 空洞偏移（对齐上游 m_offset_stash）
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("偏移:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: 0.5; to: 10; stepSize: 0.1
                        value: root.editorVm ? root.editorVm.hollowOffset : 3
                        implicitWidth: 100
                        onMoved: if (root.editorVm) root.editorVm.hollowOffset = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.hollowOffset.toFixed(1) : "3.0"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 30
                    }
                }

                // 删除选中钻孔
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 100; height: 24; radius: 4
                    color: Theme.bgElevated
                    border.color: Theme.borderDefault; border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("删除选中 (%1)").arg(root.editorVm ? root.editorVm.hollowSelectedHoleCount : 0)
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (root.editorVm) root.editorVm.deleteSelectedHollowPoints()
                    }
                }
            }
        }

        // 简化模型控制面板（对齐上游 GLGizmoSimplify）
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: simplifyContent.implicitWidth + 24
            height: simplifyContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoSimplify && root.editorVm

            ColumnLayout {
                id: simplifyContent
                anchors.centerIn: parent
                spacing: 6

                Text {
                    text: qsTr("模型简化")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                // 当前三角面数（对齐上游 GLGizmoSimplify m_triangles_count）
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("当前面数:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    Text {
                        text: root.editorVm ? root.editorVm.selectedObjectTriangleCount.toLocaleString() : "0"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                    }
                }

                // 目标面数（对齐上游 GLGizmoSimplify wanted_count）
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("目标面数:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: 100; to: 500000; stepSize: 100
                        value: root.editorVm ? root.editorVm.simplifyWantedCount : 0
                        implicitWidth: 100
                        onMoved: if (root.editorVm) root.editorVm.simplifyWantedCount = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.simplifyWantedCount.toLocaleString() : "0"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 50
                    }
                }

                // 最大误差（对齐上游 GLGizmoSimplify max_error）
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("最大误差:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: 0.001; to: 1.0; stepSize: 0.001
                        value: root.editorVm ? root.editorVm.simplifyMaxError : 0
                        implicitWidth: 100
                        onMoved: if (root.editorVm) root.editorVm.simplifyMaxError = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.simplifyMaxError.toFixed(3) : "0.000"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 40
                    }
                }

                // 执行简化按钮（对齐上游 GLGizmoSimplify apply）
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 100; height: 24; radius: 4
                    color: Theme.accent
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("执行简化")
                        color: "white"
                        font.pixelSize: Theme.fontSizeXS
                        font.bold: true
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (root.editorVm) root.editorVm.simplifySelected()
                    }
                }
            }
        }

        // Phase 143 (VDB-05): Hollow gizmo control panel (对齐上游 GLGizmoHollow).
        // Surfaces the existing Q_PROPERTYs declared at EditorViewModel.h:326-340
        // (hollowEnabled, hollowOffset, hollowQuality, hollowClosingDistance,
        // hollowHoleRadius, hollowHoleHeight). The full SLA slice path (VDB-06)
        // is a v5.1+ follow-up — this panel wires the parameter surface so when
        // SLAPrint lands the values flow through.
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: hollowContent.implicitWidth + 24
            height: hollowContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoHollow && root.editorVm

            ColumnLayout {
                id: hollowContent
                anchors.centerIn: parent
                spacing: 6

                Text {
                    text: qsTr("镂空设置")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                // 启用镂空（对齐上游 hollowing_enable）
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("启用镂空:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxCheckBox {
                        checked: root.editorVm ? root.editorVm.hollowEnabled : false
                        onCheckedChanged: if (root.editorVm) root.editorVm.hollowEnabled = checked
                    }
                }

                // 镂空厚度（对齐上游 hollowing_min_thickness — offset）
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("厚度:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: 1.0; to: 10.0; stepSize: 0.1
                        value: root.editorVm ? root.editorVm.hollowOffset : 3.0
                        implicitWidth: 100
                        onMoved: if (root.editorVm) root.editorVm.hollowOffset = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.hollowOffset.toFixed(1) + " mm" : "3.0 mm"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 50
                    }
                }

                // 镂空质量（对齐上游 hollowing_quality）
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("质量:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: 0.0; to: 1.0; stepSize: 0.05
                        value: root.editorVm ? root.editorVm.hollowQuality : 0.5
                        implicitWidth: 100
                        onMoved: if (root.editorVm) root.editorVm.hollowQuality = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.hollowQuality.toFixed(2) : "0.50"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 40
                    }
                }

                // 闭合距离（对齐上游 hollowing_closing_distance）
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("闭合距离:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: 0.0; to: 10.0; stepSize: 0.1
                        value: root.editorVm ? root.editorVm.hollowClosingDistance : 2.0
                        implicitWidth: 100
                        onMoved: if (root.editorVm) root.editorVm.hollowClosingDistance = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.hollowClosingDistance.toFixed(1) + " mm" : "2.0 mm"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 50
                    }
                }

                // 排水孔直径（对齐上游 hole_diameter — hollowHoleRadius*2）
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("排水孔半径:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: 1.0; to: 10.0; stepSize: 0.1
                        value: root.editorVm ? root.editorVm.hollowHoleRadius : 3.0
                        implicitWidth: 100
                        onMoved: if (root.editorVm) root.editorVm.hollowHoleRadius = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.hollowHoleRadius.toFixed(1) + " mm" : "3.0 mm"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 50
                    }
                }

                // 排水孔深度（对齐上游 hole_depth — hollowHoleHeight）
                RowLayout {
                    spacing: 6
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("排水孔深度:"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                    CxSlider {
                        from: 1.0; to: 10.0; stepSize: 0.1
                        value: root.editorVm ? root.editorVm.hollowHoleHeight : 3.0
                        implicitWidth: 100
                        onMoved: if (root.editorVm) root.editorVm.hollowHoleHeight = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.hollowHoleHeight.toFixed(1) + " mm" : "3.0 mm"
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 50
                    }
                }

                // SLA 切片提示 — VDB-06 is a v5.1+ follow-up
                Text {
                    text: qsTr("注：完整 SLA 切片待 v5.1+")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSizeXS
                    font.italic: true
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // MMU 多耗材分段控制面板（对齐上游 GLGizmoMmuSegmentation）
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: mmuContent.implicitWidth + 24
            height: mmuContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoMmuSegmentation && root.editorVm

            ColumnLayout {
                id: mmuContent
                anchors.centerIn: parent
                spacing: 6

                Text {
                    text: qsTr("MMU 分段")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }

                // 耗材选择器（对齐上游 m_extruders_colors 色块选择）
                Row {
                    spacing: 4
                    Layout.alignment: Qt.AlignHCenter
                    Repeater {
                        model: root.editorVm ? root.editorVm.mmuExtruderCount : 4
                        Rectangle {
                            width: 28; height: 28; radius: 4
                            color: {
                                var colors = [Theme.statusInfo, Theme.statusError, Theme.accent, Theme.statusWarning,
                                             "#8B5CF6", "#EC4899", Theme.statusInfo, Theme.statusWarning,
                                             Theme.statusInfo, Theme.accentLight, Theme.statusError, "#84CC16", "#D946EF",
                                             Theme.statusInfo, "#A855F7", Theme.statusWarning, Theme.accentLight];
                                var c = index < colors.length ? colors[index] : Theme.textMuted;
                                root.editorVm && root.editorVm.mmuSelectedExtruder === index ? c : c + "66"
                            }
                            border.width: root.editorVm && root.editorVm.mmuSelectedExtruder === index ? 2 : 0
                            border.color: "white"
                            Text {
                                anchors.centerIn: parent
                                text: index + 1
                                color: root.editorVm && root.editorVm.mmuSelectedExtruder === index ? "white" : Theme.textPrimary
                                font.pixelSize: Theme.fontSizeSM
                                font.bold: true
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (root.editorVm) root.editorVm.mmuSelectedExtruder = index
                            }
                        }
                    }
                }

                // 当前选中耗材提示（对齐上游 m_selected_extruder_idx）
                Text {
                    text: qsTr("当前耗材: %1").arg(
                        root.editorVm ? root.editorVm.mmuSelectedExtruder + 1 : 1)
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSizeXS
                    Layout.alignment: Qt.AlignHCenter
                }

                // 清除分段按钮（对齐上游 reset triangle painting）
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 80; height: 24; radius: 4
                    color: Theme.borderDefault
                    border.color: Theme.borderStrong; border.width: 1
                    Text {
                        anchors.centerIn: parent
                        text: qsTr("清除分段")
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeXS
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (root.editorVm) root.editorVm.clearMmuSegmentation()
                    }
                }
            }
        }

        // Drill control panel (对齐上游 GLGizmoDrill)
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: drillContent.implicitWidth + 24
            height: drillContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoDrill && root.editorVm

            ColumnLayout {
                id: drillContent
                anchors.centerIn: parent
                spacing: 6

                Text { text: qsTr("钻孔"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; font.bold: true; Layout.alignment: Qt.AlignHCenter }

                Text { text: qsTr("半径"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxSpinBox { Layout.preferredWidth: 80; value: root.editorVm ? root.editorVm.drillRadius : 5; from: 1; to: 50; onValueModified: if (root.editorVm) root.editorVm.drillRadius = value }

                Text { text: qsTr("深度"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxSpinBox { Layout.preferredWidth: 80; value: root.editorVm ? root.editorVm.drillDepth : 50; from: 1; to: 200; onValueModified: if (root.editorVm) root.editorVm.drillDepth = value }

                Text { text: qsTr("形状"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxComboBox { Layout.preferredWidth: 80; model: [qsTr("圆形"), qsTr("三角形"), qsTr("方形")]; currentIndex: root.editorVm ? root.editorVm.drillShape : 0; onActivated: if (root.editorVm) root.editorVm.drillShape = currentIndex }

                Text { text: qsTr("方向"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxComboBox { Layout.preferredWidth: 80; model: [qsTr("法线"), qsTr("平行平台"), qsTr("垂直屏幕")]; currentIndex: root.editorVm ? root.editorVm.drillDirection : 0; onActivated: if (root.editorVm) root.editorVm.drillDirection = currentIndex }

                // Phase 173 (CL-03): migrated from Rectangle+Text+MouseArea
                // pseudo-button to CxButton (compact Secondary).
                CxButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("执行钻孔")
                    compact: true
                    cxStyle: CxButton.Style.Secondary
                    onClicked: if (root.editorVm) root.editorVm.drillSelected()
                }
            }
        }

        // Emboss control panel (对齐上游 GLGizmoEmboss)
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: embossContent.implicitWidth + 24
            height: embossContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoEmboss && root.editorVm

            ColumnLayout {
                id: embossContent
                anchors.centerIn: parent
                spacing: 6

                Text { text: qsTr("文字浮雕"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; font.bold: true; Layout.alignment: Qt.AlignHCenter }

                Text { text: qsTr("文本"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxTextField { Layout.preferredWidth: 120; implicitHeight: 22; font.pixelSize: Theme.fontSizeXS; text: root.editorVm ? root.editorVm.embossText : ""; onEditingFinished: if (root.editorVm) root.editorVm.embossText = text }

                // Phase 145 (EMB-04): font selector. Populated from the system
                // font list via embossFontList(); user selection stores the path
                // into embossFontPath which the pipeline reads (Phase 144 / EMB-01).
                Text { text: qsTr("字体"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxComboBox {
                    Layout.preferredWidth: 160
                    // Lazy-populate on first show (the list can be ~200 entries).
                    model: {
                        if (!root.editorVm) return []
                        const list = root.editorVm.embossFontList()
                        return list.length > 0 ? list : [{ family: qsTr("(默认)"), path: "" }]
                    }
                    textRole: "family"
                    displayText: root.editorVm && root.editorVm.embossFontPath.length > 0 ? currentText : qsTr("(默认 arial)")
                    onActivated: if (root.editorVm && count > 0) {
                        const entry = model[index]
                        root.editorVm.embossFontPath = entry ? entry.path : ""
                    }
                }

                Text { text: qsTr("高度"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxSpinBox { Layout.preferredWidth: 80; value: root.editorVm ? root.editorVm.embossHeight : 2; from: 1; to: 20; onValueModified: if (root.editorVm) root.editorVm.embossHeight = value }

                Text { text: qsTr("深度"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxSpinBox { Layout.preferredWidth: 80; value: root.editorVm ? root.editorVm.embossDepth : 1; from: 1; to: 20; onValueModified: if (root.editorVm) root.editorVm.embossDepth = value }

                // Phase 158 (EMBO-F01): style controls — boldness slider + italic
                // checkbox + use-surface / curve-projection toggles. Boldness +
                // italic map to upstream FontProp fields and reach text2shapes.
                // use-surface + curve-projection are projection concepts the
                // upstream emboss library has no primitive for; they are
                // persisted into TextConfiguration for round-trip fidelity but
                // their geometry deformation is documented-deferred.
                Text { text: qsTr("粗细"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                // Phase 168 (VS-02): migrated from raw QtQuick Slider to CxSlider
                // (consistency with peer gizmo panels — Simplify, Support paint).
                CxSlider {
                    Layout.preferredWidth: 80
                    from: 0.0; to: 2.0; stepSize: 0.1
                    value: root.editorVm ? root.editorVm.embossBoldness : 0.0
                    onMoved: if (root.editorVm) root.editorVm.embossBoldness = value
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    CxCheckBox {
                        text: qsTr("斜体")
                        checked: root.editorVm ? root.editorVm.embossItalic : false
                        onToggled: if (root.editorVm) root.editorVm.embossItalic = checked
                    }
                    CxCheckBox {
                        text: qsTr("贴附表面")
                        checked: root.editorVm ? root.editorVm.embossUseSurface : false
                        onToggled: if (root.editorVm) root.editorVm.embossUseSurface = checked
                        ToolTip.visible: hovered
                        // Phase 165 (CW-02): was developer jargon leaking into
                        // user copy — replaced with user-appropriate text.
                        ToolTip.text: qsTr("将文字贴附到模型表面（实验性，完整效果将在后续版本提供）")
                    }
                    CxCheckBox {
                        text: qsTr("曲线投影")
                        checked: root.editorVm ? root.editorVm.embossCurveProjection : false
                        onToggled: if (root.editorVm) root.editorVm.embossCurveProjection = checked
                        ToolTip.visible: hovered
                        ToolTip.text: qsTr("沿曲面进行投影（实验性，完整效果将在后续版本提供）")
                    }
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: Theme.spacingSM
                    // Phase 173 (CL-03): 同步执行 migrated from pseudo-button to CxButton.
                    CxButton {
                        Layout.preferredWidth: 70
                        text: qsTr("执行")
                        compact: true
                        cxStyle: CxButton.Style.Secondary
                        onClicked: if (root.editorVm) root.editorVm.embossSelected()
                    }
                    // Phase 145 (EMB-03): async 执行 — 长文本不阻塞 UI.
                    // Phase 173 (CL-03): migrated from pseudo-button to CxButton (Primary).
                    CxButton {
                        Layout.preferredWidth: 90
                        text: qsTr("异步执行")
                        compact: true
                        cxStyle: CxButton.Style.Primary
                        onClicked: if (root.editorVm) root.editorVm.embossSelectedAsync()
                    }
                }

                // Phase 145 (EMB-03): status feedback for async emboss.
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("(异步执行后状态显示于此)")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSizeXS
                    font.italic: true
                    visible: root.editorVm && root.editorVm.embossText.length > 0
                }
                Connections {
                    target: root.editorVm
                    // Phase 145 (EMB-03): the viewmodel handles object/mesh refresh
                    // internally on embossVolumeAdded; QML just shows user feedback.
                    function onEmbossVolumeAdded(objectIndex, volumeName) {
                        if (backend) backend.postNotification(qsTr("浮雕完成：") + volumeName)
                    }
                    function onEmbossVolumeFailed(reason) {
                        if (backend) backend.postError(qsTr("浮雕失败：") + reason, 2)
                    }
                }
            }
        }

        // MeshBoolean control panel (对齐上游 GLGizmoMeshBoolean)
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: boolContent.implicitWidth + 24
            height: boolContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoMeshBoolean && root.editorVm

            ColumnLayout {
                id: boolContent
                anchors.centerIn: parent
                spacing: 6

                Text { text: qsTr("布尔运算"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; font.bold: true; Layout.alignment: Qt.AlignHCenter }

                Text { text: qsTr("运算类型"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxComboBox { Layout.preferredWidth: 100; model: [qsTr("并集 (Union)"), qsTr("差集 (Difference)"), qsTr("交集 (Intersection)")]; currentIndex: root.editorVm ? root.editorVm.booleanOperation : 1; onActivated: if (root.editorVm) root.editorVm.booleanOperation = currentIndex }

                Text { text: qsTr("需选中 2 个以上对象"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS; Layout.alignment: Qt.AlignHCenter }

                // Phase 173 (CL-03): migrated from Rectangle+Text+MouseArea
                // pseudo-button to CxButton (compact Secondary).
                CxButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("执行运算")
                    compact: true
                    cxStyle: CxButton.Style.Secondary
                    onClicked: if (root.editorVm) root.editorVm.booleanExecute()
                }
            }
        }

        // AdvancedCut control panel (对齐上游 GLGizmoAdvancedCut)
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: cutContent.implicitWidth + 24
            height: cutContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoAdvancedCut && root.editorVm

            ColumnLayout {
                id: advCutContent
                anchors.centerIn: parent
                spacing: 6

                Text { text: qsTr("高级切割"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; font.bold: true; Layout.alignment: Qt.AlignHCenter }

                Text { text: qsTr("切割轴"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxComboBox { Layout.preferredWidth: 80; model: ["X", "Y", "Z"]; currentIndex: root.editorVm ? root.editorVm.cutAxis : 2; onActivated: if (root.editorVm) root.editorVm.cutAxis = currentIndex }

                Text { text: qsTr("位置"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxSpinBox { Layout.preferredWidth: 80; value: root.editorVm ? root.editorVm.cutPosition : 0; from: -500; to: 500; onValueModified: if (root.editorVm) root.editorVm.cutPosition = value }

                CxCheckBox { text: qsTr("保留两侧"); checked: root.editorVm ? root.editorVm.cutKeepMode === 1 : true; onCheckedChanged: if (root.editorVm) root.editorVm.cutKeepMode = checked ? 1 : 0 }
                CxCheckBox { text: qsTr("仅上半部"); checked: root.editorVm ? root.editorVm.cutKeepMode === 2 : false; onCheckedChanged: if (root.editorVm) root.editorVm.cutKeepMode = checked ? 2 : 1 }

                // Phase 173 (CL-03): migrated from Rectangle+Text+MouseArea
                // pseudo-button to CxButton (compact Secondary).
                CxButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("执行切割")
                    compact: true
                    cxStyle: CxButton.Style.Secondary
                    onClicked: if (root.editorVm) root.editorVm.cutSelected()
                }
            }
        }

        // FaceDetector control panel (对齐上游 GLGizmoFaceDetector)
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: fdContent.implicitWidth + 24
            height: fdContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoFaceDetector && root.editorVm

            ColumnLayout {
                id: fdContent
                anchors.centerIn: parent
                spacing: 6

                Text { text: qsTr("面检测"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; font.bold: true; Layout.alignment: Qt.AlignHCenter }

                Text { text: qsTr("角度阈值"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxSpinBox { Layout.preferredWidth: 80; value: root.editorVm ? root.editorVm.faceDetectorAngle : 5; from: 1; to: 90; onValueModified: if (root.editorVm) root.editorVm.faceDetectorAngle = value }

                Text { text: qsTr("检测与 Z 轴平行的平面"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS; Layout.alignment: Qt.AlignHCenter }

                // Phase 173 (CL-03): migrated from Rectangle+Text+MouseArea
                // pseudo-button to CxButton (compact Secondary).
                CxButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("执行检测")
                    compact: true
                    cxStyle: CxButton.Style.Secondary
                    onClicked: if (root.editorVm) root.editorVm.detectFlatFaces()
                }
            }
        }

        // Text tool control panel (对齐上游 GLGizmoText)
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: txtContent.implicitWidth + 24
            height: txtContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoText && root.editorVm

            ColumnLayout {
                id: txtContent
                anchors.centerIn: parent
                spacing: 6

                Text { text: qsTr("文字工具"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; font.bold: true; Layout.alignment: Qt.AlignHCenter }

                Text { text: qsTr("文本内容"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxTextField { Layout.preferredWidth: 120; implicitHeight: 22; font.pixelSize: Theme.fontSizeXS; text: root.editorVm ? root.editorVm.textContent : ""; onEditingFinished: if (root.editorVm) root.editorVm.textContent = text }

                Text { text: qsTr("字号"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxSpinBox { Layout.preferredWidth: 80; value: root.editorVm ? root.editorVm.textSize : 20; from: 1; to: 200; onValueModified: if (root.editorVm) root.editorVm.textSize = value }

                // Phase 173 (CL-03): migrated from Rectangle+Text+MouseArea
                // pseudo-button to CxButton (compact Secondary).
                CxButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("添加文字")
                    compact: true
                    cxStyle: CxButton.Style.Secondary
                    onClicked: if (root.editorVm) root.editorVm.addTextObject()
                }
            }
        }

        // SVG import control panel (对齐上游 GLGizmoSVG)
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: svgContent.implicitWidth + 24
            height: svgContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoSVG && root.editorVm

            ColumnLayout {
                id: svgContent
                anchors.centerIn: parent
                spacing: 6

                Text { text: qsTr("SVG 导入"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; font.bold: true; Layout.alignment: Qt.AlignHCenter }

                Text { text: qsTr("文件路径"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxTextField { Layout.preferredWidth: 140; implicitHeight: 22; font.pixelSize: Theme.fontSizeXS; text: root.editorVm ? root.editorVm.svgFilePath : ""; placeholderText: qsTr("选择 SVG 文件..."); enabled: false }

                Text { text: qsTr("缩放"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS }
                CxSpinBox { Layout.preferredWidth: 80; value: root.editorVm ? root.editorVm.svgScale : 1; from: 1; to: 100; onValueModified: if (root.editorVm) root.editorVm.svgScale = value }

                // Phase 173 (CL-03): migrated from Rectangle+Text+MouseArea
                // pseudo-button to CxButton (compact Secondary).
                CxButton {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("导入 SVG")
                    compact: true
                    cxStyle: CxButton.Style.Secondary
                    onClicked: if (root.editorVm) root.editorVm.importSVG()
                }
            }
        }

        // SLA Supports control panel (对齐上游 GLGizmoSlaSupports)
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: root.gizmoPanelTopOffset
            anchors.horizontalCenter: parent.horizontalCenter
            width: slaContent.implicitWidth + 24
            height: slaContent.implicitHeight + 16
            radius: 6
            color: Theme.bgFloating
            border.color: Theme.borderSubtle
            visible: viewport3d.gizmoMode === GLViewport.GizmoSlaSupports && root.editorVm

            ColumnLayout {
                id: slaContent
                anchors.centerIn: parent
                spacing: 6

                Text { text: qsTr("SLA 支撑"); color: Theme.textPrimary; font.pixelSize: Theme.fontSizeSM; font.bold: true; Layout.alignment: Qt.AlignHCenter }

                Text { text: qsTr("点击模型表面添加支撑点"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS; Layout.alignment: Qt.AlignHCenter; wrapMode: Text.Wrap; Layout.maximumWidth: 120 }
                Text { text: qsTr("右键删除单个支撑"); color: Theme.textMuted; font.pixelSize: Theme.fontSizeXS; Layout.alignment: Qt.AlignHCenter }
                Text { text: qsTr("（需 SLA 切片配置）"); color: Theme.statusWarning; font.pixelSize: Theme.fontSizeXS; Layout.alignment: Qt.AlignHCenter }
            }
        }

            } // end viewportArea

        } // end RowLayout

        // 转发预览请求到页面导航（对齐上游 Plater::priv::on_preview）
        Connections {
            target: root.editorVm
            function onPreviewRequested() { backend.setCurrentPage(2) }
            // 平板切换或对象变更时请求 GL 缩略图更新（对齐上游 Plater::update_plate_thumbnails）
            function onCurrentPlateIndexChanged() { root.requestGLThumbnail() }
            // Phase 156 (CLOS-03): on state change (covers project content
            // changes — arrange/add/move all emit stateChanged), schedule a
            // session-capture sweep for any plate missing a thumbnail. The
            // 250ms debounce coalesces bursts (arrange/add/move emit several
            // stateChanged signals in quick succession).
            function onStateChanged() { sessionThumbScheduler.restart() }
        }

        // Refresh the plate list after the GL FBO thumbnail capture completes.
        Connections {
            target: viewport3d
            function onThumbnailCaptured() { /* plate cards auto-rebind via lastThumbnailData property */ }
            // Phase 156 (CLOS-03): per-plate capture delivery. Routes the
            // captured bytes back into PartPlate::setThumbnail via the
            // ProjectServiceMock write path so non-current plates retain real
            // thumbnails within the session (the gap that forced Phase 151 to
            // ship persisted-only).
            function onThumbnailCapturedForPlate(plateIndex, data) {
                if (!root.editorVm || !data || data.length === 0) return
                root.editorVm.setPlateThumbnailFromBase64(plateIndex, data)
            }
        }

        // Phase 156 (CLOS-03): session-capture scheduler. Iterates plates and
        // requests a thumbnail capture for each plate that lacks one. Aligned
        // with upstream Plater::update_plate_thumbnails (fires on content
        // change + before save). The renderer's queued delivery serializes
        // captures naturally; we just enqueue one per plate.
        Timer {
            id: sessionThumbScheduler
            interval: 250
            repeat: false
            onTriggered: root.captureMissingPlateThumbnails()
        }

        function captureMissingPlateThumbnails() {
            if (!root.editorVm || !viewport3d) return
            const count = root.editorVm.plateCount
            if (count <= 0) return
            // Capture any plate that has no thumbnail yet (including non-current
            // plates — the gap Phase 151 left). The per-plate signal handler
            // above persists each capture into PartPlate::setThumbnail.
            for (let i = 0; i < count; i++) {
                if (root.editorVm.plateThumbnailBase64(i).length === 0) {
                    viewport3d.requestThumbnailCapture(i, 128)
                }
            }
        }

        // Bottom plate bar (aligned with upstream GLCanvas3D plate thumbnails at viewport bottom)
        Item {
            id: plateBar
            parent: viewportArea
            anchors.bottom: parent.bottom
            anchors.bottomMargin: root.targetViewportBottomInset
            anchors.left: parent.left
            anchors.leftMargin: 14
            anchors.right: parent.right
            anchors.rightMargin: 14
            height: 44
            visible: root.editorVm && root.editorVm.plateCount > 0

            Rectangle {
                id: prepareVisualStatusPill
                anchors.fill: parent
                color: "transparent"

            RowLayout {
                anchors.fill: parent
                spacing: 6

                ListView {
                    id: plateListView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: ListView.Horizontal
                    spacing: 4
                    clip: true
                    model: root.editorVm ? root.editorVm.plateCount : 0

                    delegate: Rectangle {
                        required property int index
                        property bool dragHover: false
                        // Phase 151 (PLATE-02): stash the source plate index during
                        // a plate-drag so the drop target knows which plate to move.
                        property int draggedPlateIndex: -1
                        readonly property int sliceResultStatus: root.editorVm ? root.editorVm.plateSliceResultStatus(index) : 0
                        width: 86
                        height: plateListView.height
                        radius: 4
                        readonly property bool isCurrent: root.editorVm && !root.editorVm.showAllObjects && root.editorVm.currentPlateIndex === index
                        color: dragHover ? Theme.accentSubtle
                            : isCurrent ? Theme.accentSubtle
                            : Theme.bgElevated
                        border.width: dragHover ? 2 : 1
                        border.color: dragHover ? Theme.accent
                            : isCurrent ? Theme.accent
                            : Theme.borderSubtle

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 4
                            anchors.rightMargin: 4
                            spacing: 4

                            // Thumbnail
                            Rectangle {
                                width: 28
                                height: 28
                                radius: 3
                                color: Theme.bgElevated
                                border.width: 1
                                border.color: Theme.borderSubtle
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    anchors.margins: 1
                                    fillMode: Image.PreserveAspectFit
                                    smooth: true
                                    source: {
                                        if (!root.editorVm) return ""
                                        var glThumb = viewport3d.lastThumbnailData
                                        if (glThumb.length > 0 && index === root.editorVm.currentPlateIndex)
                                            return root.thumbnailSource(glThumb)
                                        return root.thumbnailSource(root.editorVm.plateThumbnailBase64(index))
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 0

                                Text {
                                    Layout.fillWidth: true
                                    text: root.editorVm ? root.editorVm.plateName(index) : ""
                                    color: isCurrent ? Theme.accent : Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeXS
                                    font.bold: isCurrent
                                    elide: Text.ElideRight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: root.editorVm
                                        ? qsTr("%1 对象").arg(root.editorVm.plateObjectCount(index))
                                        : ""
                                    color: Theme.textTertiary
                                    font.pixelSize: 8
                                    elide: Text.ElideRight
                                }
                                Text {
                                    id: plateStateText
                                    Layout.fillWidth: true
                                    text: sliceResultStatus === 1 ? qsTr("Sliced")
                                        : sliceResultStatus === 2 ? qsTr("Stale")
                                        : qsTr("Ready")
                                    color: sliceResultStatus === 1 ? Theme.accent
                                        : sliceResultStatus === 2 ? Theme.statusWarning
                                        : Theme.textDisabled
                                    font.pixelSize: 8
                                    elide: Text.ElideRight
                                }
                            }

                            Rectangle {
                                visible: sliceResultStatus !== 0
                                width: 5; height: 5; radius: 3
                                color: sliceResultStatus === 1 ? Theme.accent : Theme.statusWarning
                                opacity: 0.8
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            onClicked: function(mouse) {
                                if (!root.editorVm) return
                                if (mouse.button === Qt.RightButton) {
                                    root.contextPlateIndex = index
                                    plateContextMenu.popup()
                                    mouse.accepted = true
                                    return
                                }
                                root.editorVm.setCurrentPlateIndex(index)
                                root.editorVm.setShowAllObjects(false)
                            }
                        }

                        DropArea {
                            anchors.fill: parent
                            keys: ["object-drag"]
                            onEntered: function(drag) { parent.dragHover = true }
                            onExited: function(drag) { parent.dragHover = false }
                            onDropped: function(drop) {
                                parent.dragHover = false
                                if (!root.editorVm) return
                                root.editorVm.moveSelectedObjectToPlate(index)
                                drop.acceptProposedAction()
                            }
                        }

                        // Phase 151 (PLATE-02): drag-to-reorder. DropArea accepts
                        // a "plate-drag" key (emitted by the DragHandler below on
                        // sibling cards); on drop calls editorVm.movePlate.
                        // Mirrors upstream PartPlate plate-bar drag-reorder.
                        DropArea {
                            anchors.fill: parent
                            keys: ["plate-drag"]
                            onEntered: function(drag) { parent.dragHover = true }
                            onExited: function(drag) { parent.dragHover = false }
                            onDropped: function(drop) {
                                parent.dragHover = false
                                if (!root.editorVm) return
                                // The source plate's index is stashed on the
                                // dragged card via draggedPlateIndex (set on drag start).
                                const srcIndex = drop.source && drop.source.draggedPlateIndex !== undefined
                                    ? drop.source.draggedPlateIndex : -1
                                if (srcIndex < 0 || srcIndex === index) return
                                root.editorVm.movePlate(srcIndex, index)
                                drop.acceptProposedAction()
                            }
                        }

                        // Phase 151 (PLATE-02): drag handler on the card header
                        // area (the label strip — avoids hijacking the click-to-
                        // select interaction on the rest of the card). On drag
                        // start, attaches the plate index so the drop target
                        // knows which plate to move.
                        DragHandler {
                            target: parent
                            xAxis.enabled: true
                            yAxis.enabled: false
                            dragThreshold: 6
                            // Only the top label strip initiates a plate drag —
                            // clicking the thumbnail/body still selects the plate.
                            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                            onActiveChanged: {
                                if (active && parent && root.editorVm) {
                                    parent.Drag.startDrag("plate-drag")
                                    parent.draggedPlateIndex = parent.index
                                } else if (parent) {
                                    parent.Drag.drop()
                                }
                            }
                        }
                    }
                }

                // Add plate button
                Rectangle {
                    visible: root.editorVm && root.editorVm.canAddPlate
                    width: 26
                    height: 26
                    radius: 4
                    color: addPlateMA.containsMouse ? Theme.bgHover : Theme.bgElevated
                    border.width: 1
                    border.color: Theme.borderSubtle

                    Text {
                        anchors.centerIn: parent
                        text: "+"
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeLG
                        font.bold: true
                    }

                    MouseArea {
                        id: addPlateMA
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (root.editorVm) root.editorVm.addPlate()
                    }
                }
            }
        }
        }

        // 对象信息栏（对齐上游 Plater::show_object_info）
        Rectangle {
            id: objectInfoBar
            visible: root.editorVm && root.editorVm.selectedObjectCount === 1
                     && root.editorVm.selectedObjectInfoText !== ""
            parent: viewportArea
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 52
            anchors.left: parent.left
            anchors.leftMargin: 14
            height: infoRow.implicitHeight + 10
            width: infoRow.implicitWidth + 16
            radius: 4
            color: Theme.bgInset

            property bool hasErrors: root.editorVm
                && (root.editorVm.selectedObjectOpenEdges > 0
                    || root.editorVm.selectedObjectRepairedErrors > 0)

            Row {
                id: infoRow
                anchors.centerIn: parent
                spacing: 6

                // 流形状态图标
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 14
                    height: 14
                    radius: 7
                    color: objectInfoBar.hasErrors
                           ? (root.editorVm.selectedObjectOpenEdges > 0 ? Theme.statusErrorDark : Theme.statusWarning)
                           : Theme.accentDark

                    Label {
                        anchors.centerIn: parent
                        text: objectInfoBar.hasErrors
                              ? (root.editorVm.selectedObjectOpenEdges > 0 ? "!" : "~")
                              : "OK"
                        color: Theme.textOnAccent
                        font.pixelSize: 7
                        font.bold: true
                    }
                }

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.editorVm ? root.editorVm.selectedObjectInfoText : ""
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                }

                // 非流形边警告（对齐上游 get_mesh_errors_info remaining errors）
                Label {
                    visible: root.editorVm && root.editorVm.selectedObjectOpenEdges > 0
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr(" | %1 non-manifold edges").arg(root.editorVm.selectedObjectOpenEdges)
                    color: Theme.statusErrorDark
                    font.pixelSize: Theme.fontSizeSM
                }

                // 已修复错误计数（对齐 upstream auto-repaired info）
                Label {
                    visible: root.editorVm && root.editorVm.selectedObjectRepairedErrors > 0
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr(" | %1 errors repaired").arg(root.editorVm.selectedObjectRepairedErrors)
                    color: Theme.statusWarning
                    font.pixelSize: Theme.fontSizeSM
                }
            }
        }

    }

    component TransformMetric: Row {
        property string axisName: ""
        property string valueText: ""
        property color accentColor: Theme.textSecondary

        spacing: 4

        Text {
            text: axisName
            color: accentColor
            font.pixelSize: Theme.fontSizeXS
            font.bold: true
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: valueText
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeXS
            font.family: "Consolas, monospace"
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
