import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."
import "../controls"
import "../panels"
import "../components"
import "../dialogs"
import CrealityGL 1.0

Item {
    id: root
    required property var editorVm
    required property var configVm
    property alias viewport3dRef: viewport3d
    property string processCategory: ""
    property bool leftPanelVisible: true
    focus: true

    component ToolStripDivider: Rectangle {
        width: 1
        height: 22
        radius: 1
        color: Theme.borderSubtle
        opacity: 0.9
    }

    function applyFitHintIfReady() {
        if (!root.editorVm)
            return
        var h = root.editorVm.fitHint
        if (h && h.w > 0)
            viewport3d.requestFitView(h.x, h.y, h.z, h.w)
    }

    function undoFromTopbar() {
        viewport3d.undo()
    }
    function redoFromTopbar() {
        viewport3d.redo()
    }

    // Keyboard shortcuts matching upstream CrealityPrint Plater
    Keys.onPressed: (event) => {
        if (!root.editorVm)
            return
        var key = event.key
        var mod = event.modifiers
        switch (key) {
        case Qt.Key_W:
            viewport3d.gizmoMode = GLViewport.GizmoMove
            event.accepted = true
            break
        case Qt.Key_E:
            viewport3d.gizmoMode = GLViewport.GizmoRotate
            event.accepted = true
            break
        case Qt.Key_R:
            if (!(mod & Qt.ControlModifier)) {
                viewport3d.gizmoMode = GLViewport.GizmoScale
                event.accepted = true
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
            root.editorVm.deleteSelectedObjects()
            event.accepted = true
            break
        case Qt.Key_Escape:
            root.editorVm.clearObjectSelection()
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
        case Qt.Key_F:
            root.applyFitHintIfReady()
            event.accepted = true
            break
        case Qt.Key_U:
            if (mod & Qt.ControlModifier) {
                viewport3d.gizmoMode = GLViewport.GizmoMeasure
                event.accepted = true
            }
            break
        case Qt.Key_G:
            viewport3d.gizmoMode = GLViewport.GizmoFlatten
            event.accepted = true
            break
        case Qt.Key_X:
            if ((mod & Qt.ControlModifier) && (mod & Qt.ShiftModifier)) {
                viewport3d.gizmoMode = GLViewport.GizmoCut
                event.accepted = true
            }
            break
        }
    }

    // Object context menu (right-click, aligns with upstream create_object_menu/create_extra_object_menu)
    Menu {
        id: objectContextMenu

        MenuItem {
            text: qsTr("复制选中")
            onTriggered: if (root.editorVm) root.editorVm.duplicateSelectedObjects()
        }
        MenuItem {
            text: qsTr("删除选中")
            onTriggered: if (root.editorVm) root.editorVm.deleteSelectedObjects()
        }
        MenuSeparator { }
        MenuItem {
            text: qsTr("全选")
            onTriggered: if (root.editorVm) root.editorVm.selectAllVisibleObjects()
        }
        MenuItem {
            text: qsTr("取消选择")
            onTriggered: if (root.editorVm) root.editorVm.clearObjectSelection()
        }
        MenuSeparator { }
        // 对齐上游 create_extra_object_menu — Rename
        MenuItem {
            text: qsTr("重命名")
            enabled: root.editorVm && root.editorVm.selectedObjectCount === 1
            onTriggered: {
                renameDialog.currentObjIndex = root.editorVm.selectedObjectIndex
                renameDialog.currentName = root.editorVm.objectName(root.editorVm.selectedObjectIndex)
                renameDialog.open()
            }
        }
        // 对齐上游 create_extra_object_menu — Center
        MenuItem {
            text: qsTr("居中到热床")
            onTriggered: if (root.editorVm) root.editorVm.centerSelectedObjects()
        }
        // 对齐上游 create_extra_object_menu — Fill bed with copies
        MenuItem {
            text: qsTr("铺满热床")
            onTriggered: if (root.editorVm) root.editorVm.fillBedWithCopies()
        }
        // 对齐上游 create_extra_object_menu — Export as STL
        MenuItem {
            text: qsTr("导出为 STL")
            enabled: root.editorVm && root.editorVm.selectedObjectCount === 1
            onTriggered: if (root.editorVm) root.editorVm.exportSelectedAsStl()
        }
        MenuSeparator { }
        MenuItem {
            text: qsTr("移动模式")
            onTriggered: viewport3d.gizmoMode = GLViewport.GizmoMove
        }
        MenuItem {
            text: qsTr("旋转模式")
            onTriggered: viewport3d.gizmoMode = GLViewport.GizmoRotate
        }
        MenuItem {
            text: qsTr("缩放模式")
            onTriggered: viewport3d.gizmoMode = GLViewport.GizmoScale
        }
        MenuSeparator { }
        MenuItem {
            text: qsTr("自动朝向")
            onTriggered: if (root.editorVm) root.editorVm.autoOrientSelected()
        }
        MenuItem {
            text: qsTr("拆分对象")
            enabled: root.editorVm && root.editorVm.selectedObjectCount === 1
            onTriggered: if (root.editorVm) root.editorVm.splitSelectedObject()
        }
        MenuSeparator { }
        Menu {
            title: qsTr("镜像")
            MenuItem {
                text: qsTr("沿 X 轴镜像")
                onTriggered: viewport3d.mirrorSelection(0)
            }
            MenuItem {
                text: qsTr("沿 Y 轴镜像")
                onTriggered: viewport3d.mirrorSelection(1)
            }
            MenuItem {
                text: qsTr("沿 Z 轴镜像")
                onTriggered: viewport3d.mirrorSelection(2)
            }
        }
        MenuSeparator { }
        // 对齐上游 set_printable
        MenuItem {
            text: root.editorVm && root.editorVm.selectedObjectCount > 0
                   && root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex)
                   ? qsTr("设为不参与打印") : qsTr("设为可打印")
            onTriggered: {
                if (root.editorVm) {
                    if (root.editorVm.selectedObjectCount > 1)
                        root.editorVm.setSelectedObjectsPrintable(!root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex))
                    else
                        root.editorVm.setObjectPrintable(root.editorVm.selectedObjectIndex, !root.editorVm.objectPrintable(root.editorVm.selectedObjectIndex))
                }
            }
        }
        MenuItem {
            text: qsTr("显示/隐藏")
            onTriggered: if (root.editorVm) root.editorVm.toggleSelectedObjectsVisibility()
        }
        MenuItem {
            text: qsTr("适应视图")
            onTriggered: root.applyFitHintIfReady()
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

            Label { text: qsTr("输入新名称:"); color: Theme.textPrimary; font.pixelSize: 12 }

            TextField {
                id: renameInput
                Layout.fillWidth: true
                text: renameDialog.currentName
                color: Theme.textPrimary
                placeholderText: qsTr("对象名称")
                background: Rectangle { color: Theme.bgPanel; border.color: Theme.borderDefault; border.width: 1; radius: 4 }
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
                    Label { anchors.centerIn: parent; text: qsTr("取消"); color: Theme.textSecondary; font.pixelSize: 11 }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: renameDialog.close() }
                }
                Rectangle {
                    Layout.fillWidth: true; height: 28; radius: 4
                    color: Theme.accent
                    Label { anchors.centerIn: parent; text: qsTr("确认"); color: "#fff"; font.pixelSize: 11 }
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

    Menu {
        id: plateContextMenu

        MenuItem {
            text: qsTr("选择全部对象")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
                     && root.editorVm.plateObjectCount(root.contextPlateIndex) > 0
            onTriggered: if (root.editorVm) root.editorVm.selectAllOnPlate(root.contextPlateIndex)
        }
        MenuItem {
            text: qsTr("清空平板")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
                     && root.editorVm.plateObjectCount(root.contextPlateIndex) > 0
            onTriggered: if (root.editorVm) root.editorVm.removeAllOnPlate(root.contextPlateIndex)
        }
        MenuSeparator { }
        MenuItem {
            text: qsTr("排列对象")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
                     && root.editorVm.plateObjectCount(root.contextPlateIndex) > 0
            onTriggered: {
                if (root.editorVm) root.editorVm.selectAllOnPlate(root.contextPlateIndex)
                viewport3d.arrangeSelected()
            }
        }
        MenuItem {
            text: qsTr("自动朝向")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
                     && root.editorVm.plateObjectCount(root.contextPlateIndex) > 0
            onTriggered: {
                if (root.editorVm) root.editorVm.selectAllOnPlate(root.contextPlateIndex)
                root.editorVm.autoOrientSelected()
            }
        }
        MenuSeparator { }
        MenuItem {
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
        MenuItem {
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
        MenuItem {
            text: root.contextPlateIndex >= 0 && root.editorVm
                  && root.editorVm.isPlateLocked(root.contextPlateIndex)
                  ? qsTr("解锁平板") : qsTr("锁定平板")
            enabled: root.contextPlateIndex >= 0 && root.editorVm
            onTriggered: if (root.editorVm) root.editorVm.togglePlateLocked(root.contextPlateIndex)
        }
        MenuSeparator { }
        MenuItem {
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

                TextField {
                    id: nameField
                    Layout.fillWidth: true
                    text: dlg.currentName
                    font.pixelSize: 13
                    color: "#e2e8f1"
                    background: Rectangle {
                        radius: 6
                        color: "#0f1318"
                        border.color: nameField.activeFocus ? Theme.accent : Theme.borderSubtle
                    }
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
                        color: cancelHov.containsMouse ? "#2e3540" : "#1e2535"
                        Text { id: cancelText; anchors.centerIn: parent; text: qsTr("取消"); color: Theme.textSecondary; font.pixelSize: 12 }
                        HoverHandler { id: cancelHov }
                        TapHandler { onTapped: dlg.close() }
                    }
                    Rectangle {
                        height: 28
                        width: okText.implicitWidth + 20
                        radius: 6
                        color: Theme.accent
                        Text { id: okText; anchors.centerIn: parent; text: qsTr("确定"); color: Theme.textOnAccent; font.pixelSize: 12 }
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
            title: qsTr("平板设置") + " — " + settingsDlg.plateName
            modal: true
            anchors.centerIn: parent
            width: 360

            ColumnLayout {
                anchors.fill: parent
                spacing: 14

                // 平板名称
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        text: qsTr("平板名称")
                        color: Theme.textSecondary
                        font.pixelSize: 12
                        Layout.preferredWidth: 80
                    }
                    TextField {
                        id: psNameField
                        Layout.fillWidth: true
                        text: settingsDlg.plateName
                        maximumLength: 20
                        font.pixelSize: 12
                        color: Theme.textPrimary
                        background: Rectangle {
                            radius: 6
                            color: Theme.bgElevated
                            border.color: psNameField.activeFocus ? Theme.accent : Theme.borderSubtle
                        }
                    }
                }

                // 热床类型（对齐上游 PlateSettingsDialog bed type）
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        text: qsTr("热床类型")
                        color: Theme.textSecondary
                        font.pixelSize: 12
                        Layout.preferredWidth: 80
                    }
                    ComboBox {
                        id: bedTypeCombo
                        Layout.fillWidth: true
                        model: [
                            qsTr("默认 (PEI)"),
                            qsTr("EP 热床"),
                            qsTr("PC 热床"),
                            qsTr("纹理 PEI"),
                            qsTr("自定义")
                        ]
                        currentIndex: root.editorVm ? root.editorVm.plateBedType(settingsDlg.plateIndex) : 0
                        onActivated: function(index) {
                            if (root.editorVm) root.editorVm.setPlateBedType(settingsDlg.plateIndex, index)
                        }
                        contentItem: Text {
                            text: bedTypeCombo.displayText
                            color: Theme.textPrimary
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 8
                        }
                        background: Rectangle {
                            radius: 6
                            color: Theme.bgElevated
                            border.color: bedTypeCombo.activeFocus ? Theme.accent : Theme.borderSubtle
                        }
                        popup: Popup {
                            y: bedTypeCombo.height
                            width: bedTypeCombo.width
                            padding: 4
                            background: Rectangle { radius: 6; color: Theme.bgSurface; border.color: Theme.borderSubtle; border.width: 1 }
                            contentItem: ListView {
                                implicitHeight: contentHeight
                                model: bedTypeCombo.model
                                delegate: ItemDelegate {
                                    width: bedTypeCombo.width
                                    height: 30
                                    contentItem: Text {
                                        text: modelData
                                        color: bedTypeCombo.highlightedIndex === index ? Theme.textOnAccent : Theme.textPrimary
                                        font.pixelSize: 12
                                    }
                                    highlighted: bedTypeCombo.highlightedIndex === index
                                    background: Rectangle { color: highlighted ? Theme.accent : "transparent"; radius: 4 }
                                }
                            }
                        }
                    }
                }

                // 打印顺序（对齐上游 PlateSettingsDialog print sequence）
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        text: qsTr("打印顺序")
                        color: Theme.textSecondary
                        font.pixelSize: 12
                        Layout.preferredWidth: 80
                    }
                    ComboBox {
                        id: printSeqCombo
                        Layout.fillWidth: true
                        model: [qsTr("按层打印"), qsTr("按对象打印")]
                        currentIndex: root.editorVm ? root.editorVm.platePrintSequence(settingsDlg.plateIndex) : 0
                        onActivated: function(index) {
                            if (root.editorVm) root.editorVm.setPlatePrintSequence(settingsDlg.plateIndex, index)
                        }
                        contentItem: Text {
                            text: printSeqCombo.displayText
                            color: Theme.textPrimary
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 8
                        }
                        background: Rectangle {
                            radius: 6
                            color: Theme.bgElevated
                            border.color: printSeqCombo.activeFocus ? Theme.accent : Theme.borderSubtle
                        }
                        popup: Popup {
                            y: printSeqCombo.height
                            width: printSeqCombo.width
                            padding: 4
                            background: Rectangle { radius: 6; color: Theme.bgSurface; border.color: Theme.borderSubtle; border.width: 1 }
                            contentItem: ListView {
                                implicitHeight: contentHeight
                                model: printSeqCombo.model
                                delegate: ItemDelegate {
                                    width: printSeqCombo.width
                                    height: 30
                                    contentItem: Text {
                                        text: modelData
                                        color: printSeqCombo.highlightedIndex === index ? Theme.textOnAccent : Theme.textPrimary
                                        font.pixelSize: 12
                                    }
                                    highlighted: printSeqCombo.highlightedIndex === index
                                    background: Rectangle { color: highlighted ? Theme.accent : "transparent"; radius: 4 }
                                }
                            }
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
                        font.pixelSize: 12
                        Layout.preferredWidth: 80
                    }
                    ComboBox {
                        id: spiralCombo
                        Layout.fillWidth: true
                        model: [qsTr("跟随全局"), qsTr("开启"), qsTr("关闭")]
                        currentIndex: root.editorVm ? root.editorVm.plateSpiralMode(settingsDlg.plateIndex) : 0
                        onActivated: function(index) {
                            if (root.editorVm) root.editorVm.setPlateSpiralMode(settingsDlg.plateIndex, index)
                        }
                        contentItem: Text {
                            text: spiralCombo.displayText
                            color: Theme.textPrimary
                            font.pixelSize: 12
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 8
                        }
                        background: Rectangle {
                            radius: 6
                            color: Theme.bgElevated
                            border.color: spiralCombo.activeFocus ? Theme.accent : Theme.borderSubtle
                        }
                        popup: Popup {
                            y: spiralCombo.height
                            width: spiralCombo.width
                            padding: 4
                            background: Rectangle { radius: 6; color: Theme.bgSurface; border.color: Theme.borderSubtle; border.width: 1 }
                            contentItem: ListView {
                                implicitHeight: contentHeight
                                model: spiralCombo.model
                                delegate: ItemDelegate {
                                    width: spiralCombo.width
                                    height: 30
                                    contentItem: Text {
                                        text: modelData
                                        color: spiralCombo.highlightedIndex === index ? Theme.textOnAccent : Theme.textPrimary
                                        font.pixelSize: 12
                                    }
                                    highlighted: spiralCombo.highlightedIndex === index
                                    background: Rectangle { color: highlighted ? Theme.accent : "transparent"; radius: 4 }
                                }
                            }
                        }
                    }
                }

                // 提示信息
                Text {
                    Layout.fillWidth: true
                    text: qsTr("注：首层/其他层耗材顺序高级配置需在真实切片引擎下可用")
                    color: Theme.textDisabled
                    font.pixelSize: 10
                    wrapMode: Text.Wrap
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
                        Text { id: psOkText; anchors.centerIn: parent; text: qsTr("确定"); color: Theme.textOnAccent; font.pixelSize: 12; font.bold: true }
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

    FileDialog {
        id: openFileDlg
        title: qsTr("打开模型文件")
        nameFilters: [
            qsTr("3MF 文件 (*.3mf)"),
            qsTr("STL 文件 (*.stl)"),
            qsTr("OBJ 文件 (*.obj)"),
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
        nameFilters: [qsTr("G-code 文件 (*.gcode)")]
        onAccepted: {
            if (root.editorVm)
                root.editorVm.requestExportGCode(selectedFile.toString())
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.bgBase

        GLViewport {
            id: viewport3d
            anchors.fill: parent
            canvasType: GLViewport.CanvasView3D
            meshData: root.editorVm ? root.editorVm.meshData : null

            // Right-click context menu
            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                propagateComposedEvents: true
                onClicked: (mouse) => {
                    objectContextMenu.popup(mouse.x, mouse.y)
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

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#1a213018" }
                GradientStop { position: 0.4; color: "transparent" }
                GradientStop { position: 1.0; color: "#09101840" }
            }
        }

        CxPanel {
            id: topTools
            cxSurface: CxPanel.Surface.Floating
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: 14
            width: toolsContent.implicitWidth + 22
            height: 50
            radius: 16
            color: "#161c27de"
            border.color: "#2d3443"

            Item {
                id: toolsContent
                anchors.centerIn: parent
                implicitWidth: toolbarRow.implicitWidth
                implicitHeight: toolbarRow.implicitHeight

                Row {
                    id: toolbarRow
                    spacing: 10

                    Row {
                        spacing: 6

                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            selected: true
                            iconSource: "qrc:/qml/assets/icons/folder-plus.svg"
                            toolTipText: qsTr("导入模型")
                            onClicked: openFileDlg.open()
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/folder-open.svg"
                            toolTipText: qsTr("打开模型")
                            onClicked: openFileDlg.open()
                        }
                    }

                    ToolStripDivider { }

                    Row {
                        spacing: 6

                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            selected: viewport3d.gizmoMode === GLViewport.GizmoMove
                            iconSource: "qrc:/qml/assets/icons/arrow-forward-up.svg"
                            toolTipText: qsTr("移动 (W)")
                            onClicked: viewport3d.gizmoMode = GLViewport.GizmoMove
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            selected: viewport3d.gizmoMode === GLViewport.GizmoRotate
                            iconSource: "qrc:/qml/assets/icons/rotate-2.svg"
                            toolTipText: qsTr("旋转 (E)")
                            onClicked: viewport3d.gizmoMode = GLViewport.GizmoRotate
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            selected: viewport3d.gizmoMode === GLViewport.GizmoScale
                            iconSource: "qrc:/qml/assets/icons/maximize.svg"
                            toolTipText: qsTr("缩放 (R)")
                            onClicked: viewport3d.gizmoMode = GLViewport.GizmoScale
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            selected: viewport3d.gizmoMode === GLViewport.GizmoMeasure
                            iconSource: "qrc:/qml/assets/icons/box.svg"
                            toolTipText: qsTr("测量 (Ctrl+U)")
                            onClicked: viewport3d.gizmoMode = GLViewport.GizmoMeasure
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            selected: viewport3d.gizmoMode === GLViewport.GizmoFlatten
                            iconSource: "qrc:/qml/assets/icons/layers-subtract.svg"
                            toolTipText: qsTr("平放 (F)")
                            onClicked: viewport3d.gizmoMode = GLViewport.GizmoFlatten
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            selected: viewport3d.gizmoMode === GLViewport.GizmoCut
                            iconSource: "qrc:/qml/assets/icons/scissors.svg"
                            toolTipText: qsTr("切割 (Ctrl+Shift+X)")
                            onClicked: viewport3d.gizmoMode = GLViewport.GizmoCut
                        }
                    }

                    ToolStripDivider { }

                    Row {
                        spacing: 6

                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/layout-grid.svg"
                            toolTipText: qsTr("自动排列")
                            onClicked: viewport3d.arrangeSelected()
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/arrow-back-up.svg"
                            toolTipText: qsTr("自动朝向")
                            onClicked: root.editorVm.autoOrientSelected()
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/layers.svg"
                            toolTipText: qsTr("拆分对象")
                            onClicked: root.editorVm.splitSelectedObject()
                            enabled: root.editorVm && root.editorVm.hasSelection
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/mirror.svg"
                            toolTipText: qsTr("镜像 (沿 X 轴)")
                            onClicked: viewport3d.mirrorSelection(0)
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/restore.svg"
                            toolTipText: qsTr("重置视角")
                            onClicked: root.applyFitHintIfReady()
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            selected: !root.leftPanelVisible
                            iconSource: "qrc:/qml/assets/icons/list-details.svg"
                            toolTipText: root.leftPanelVisible ? qsTr("隐藏对象列表") : qsTr("显示对象列表")
                            onClicked: root.leftPanelVisible = !root.leftPanelVisible
                        }
                    }

                    ToolStripDivider { }

                    Row {
                        spacing: 6

                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            selected: !sidebar.collapsed
                            iconSource: "qrc:/qml/assets/icons/layout-sidebar-right.svg"
                            toolTipText: sidebar.collapsed ? qsTr("展开侧栏") : qsTr("收起侧栏")
                            onClicked: sidebar.collapsed = !sidebar.collapsed
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/settings.svg"
                            toolTipText: qsTr("准备页设置")
                            onClicked: backend.openSettings()
                        }
                    }
                }
            }
        }

        // ProcessBar 对齐上游 ProcessBar::GLToolbar — 分类标签切换右侧 PrintSettings
        CxPanel {
            id: processBar
            cxSurface: CxPanel.Surface.Floating
            anchors.top: topTools.bottom
            anchors.topMargin: 4
            anchors.horizontalCenter: parent.horizontalCenter
            width: procRow.implicitWidth + 20
            height: 28
            radius: 10
            color: "#161c27de"
            border.color: "#2d3443"

            Row {
                id: procRow
                anchors.centerIn: parent
                spacing: 2

                Repeater {
                    model: [
                        { label: qsTr("全部"), cat: "" },
                        { label: qsTr("质量"), cat: qsTr("质量") },
                        { label: qsTr("速度"), cat: qsTr("速度") },
                        { label: qsTr("支撑"), cat: qsTr("支撑") },
                        { label: qsTr("温度"), cat: qsTr("温度") },
                        { label: qsTr("填充"), cat: qsTr("填充") },
                        { label: qsTr("底座"), cat: qsTr("底座") },
                        { label: qsTr("其他"), cat: qsTr("其他") }
                    ]
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        width: procLbl.implicitWidth + 12
                        height: 20
                        radius: 5
                        color: root.processCategory === modelData.cat
                               ? Theme.accent : (procMA.containsMouse ? "#1e2535" : "transparent")
                        border.width: 1
                        border.color: root.processCategory === modelData.cat ? Theme.accent : Theme.borderSubtle

                        Text {
                            id: procLbl
                            anchors.centerIn: parent
                            text: modelData.label
                            color: root.processCategory === modelData.cat ? Theme.textOnAccent : Theme.textSecondary
                            font.pixelSize: 10
                            font.bold: true
                        }

                        MouseArea {
                            id: procMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.processCategory = modelData.cat
                                sidebar.switchToPrintTab()
                            }
                        }
                    }
                }
            }
        }

        // 测量信息面板（对齐上游 GLGizmoMeasure::on_render_input_window）
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 104
            anchors.horizontalCenter: parent.horizontalCenter
            width: measureContent.implicitWidth + 24
            height: measureContent.implicitHeight + 16
            radius: 12
            color: "#161c27e0"
            border.color: "#2d3443"
            visible: viewport3d.gizmoMode === GLViewport.GizmoMeasure && root.editorVm

            ColumnLayout {
                id: measureContent
                anchors.centerIn: parent
                spacing: 4

                Row {
                    spacing: 16
                    Label { text: qsTr("X:"); color: "#e066a0"; font.pixelSize: 11; font.bold: true; font.family: "Consolas, monospace" }
                    Label { text: root.editorVm ? root.editorVm.measureDimensions.x.toFixed(1) : "0.0"; color: "#c8d4e0"; font.pixelSize: 11; font.family: "Consolas, monospace" }
                    Label { text: qsTr("Y:"); color: "#4ec9b0"; font.pixelSize: 11; font.bold: true; font.family: "Consolas, monospace" }
                    Label { text: root.editorVm ? root.editorVm.measureDimensions.y.toFixed(1) : "0.0"; color: "#c8d4e0"; font.pixelSize: 11; font.family: "Consolas, monospace" }
                    Label { text: qsTr("Z:"); color: "#569cd6"; font.pixelSize: 11; font.bold: true; font.family: "Consolas, monospace" }
                    Label { text: root.editorVm ? root.editorVm.measureDimensions.z.toFixed(1) : "0.0"; color: "#c8d4e0"; font.pixelSize: 11; font.family: "Consolas, monospace" }
                }
                Label {
                    text: root.editorVm
                        ? (qsTr("体积: ") + root.editorVm.measureDimensions.w.toFixed(0) + qsTr(" mm³"))
                        : ""
                    color: "#8b949e"
                    font.pixelSize: 10
                    font.family: "Consolas, monospace"
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // Flatten 信息面板（对齐上游 GLGizmoFlatten::on_render_input_window）
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 104
            anchors.horizontalCenter: parent.horizontalCenter
            width: flattenContent.implicitWidth + 32
            height: flattenContent.implicitHeight + 20
            radius: 12
            color: "#161c27e0"
            border.color: "#2d3443"
            visible: viewport3d.gizmoMode === GLViewport.GizmoFlatten && root.editorVm

            ColumnLayout {
                id: flattenContent
                anchors.centerIn: parent
                spacing: 8

                Text {
                    text: qsTr("平放至面")
                    color: "#e8edf6"
                    font.pixelSize: 12
                    font.bold: true
                    Layout.alignment: Qt.AlignHCenter
                }
                Text {
                    text: root.editorVm ? (qsTr("候选面: ") + root.editorVm.flattenFaceCount) : ""
                    color: "#8b949e"
                    font.pixelSize: 11
                    Layout.alignment: Qt.AlignHCenter
                }
                Row {
                    spacing: 8
                    Layout.alignment: Qt.AlignHCenter
                    Rectangle {
                        width: 80; height: 28; radius: 4
                        color: "#18c75e"
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("平放")
                            color: "white"
                            font.pixelSize: 11
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.editorVm.flattenSelected()
                        }
                    }
                    Text {
                        text: qsTr("(G)")
                        color: "#6b7d94"
                        font.pixelSize: 10
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Text {
                    text: qsTr("将选中对象最大面朝下平放")
                    color: "#6b7d94"
                    font.pixelSize: 9
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // Cut 切割控制面板（对齐上游 GLGizmoCut::on_render_input_window）
        Rectangle {
            anchors.top: parent.top
            anchors.topMargin: 104
            anchors.horizontalCenter: parent.horizontalCenter
            width: cutContent.implicitWidth + 32
            height: cutContent.implicitHeight + 20
            radius: 12
            color: "#161c27e0"
            border.color: "#2d3443"
            visible: viewport3d.gizmoMode === GLViewport.GizmoCut && root.editorVm

            ColumnLayout {
                id: cutContent
                anchors.centerIn: parent
                spacing: 8

                Text {
                    text: qsTr("切割对象")
                    color: "#e8edf6"
                    font.pixelSize: 12
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
                            color: root.editorVm && root.editorVm.cutAxis === index ? "#1c2a3e" : "#1a1e28"
                            border.color: root.editorVm && root.editorVm.cutAxis === index ? "#18c75e" : "#2e3444"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.cutAxis === index ? "#18c75e" : "#8a96a8"
                                font.pixelSize: 10
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (root.editorVm) root.editorVm.cutAxis = index
                            }
                        }
                    }
                }

                // 切割位置滑块
                RowLayout {
                    spacing: 8
                    Layout.alignment: Qt.AlignHCenter
                    Text { text: qsTr("位置:"); color: "#8b949e"; font.pixelSize: 10 }
                    Slider {
                        from: -50; to: 50; stepSize: 0.5
                        value: root.editorVm ? root.editorVm.cutPosition : 0
                        implicitWidth: 120
                        onMoved: if (root.editorVm) root.editorVm.cutPosition = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.cutPosition.toFixed(1) + " mm" : "0.0 mm"
                        color: "#c8d4e0"
                        font.pixelSize: 10
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
                            color: root.editorVm && root.editorVm.cutKeepMode === index ? "#1c2a3e" : "#1a1e28"
                            border.color: root.editorVm && root.editorVm.cutKeepMode === index ? "#18c75e" : "#2e3444"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.cutKeepMode === index ? "#18c75e" : "#8a96a8"
                                font.pixelSize: 10
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
                        color: "#252b38"
                        border.color: "#363d4e"; border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("翻转")
                            color: "#c8d4e0"
                            font.pixelSize: 10
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.editorVm) root.editorVm.flipCutPlane()
                        }
                    }
                    Rectangle {
                        width: 70; height: 28; radius: 4
                        color: "#252b38"
                        border.color: "#363d4e"; border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("居中")
                            color: "#c8d4e0"
                            font.pixelSize: 10
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.editorVm) root.editorVm.centerCutPlane()
                        }
                    }
                    Rectangle {
                        width: 80; height: 28; radius: 4
                        color: "#18c75e"
                        Text {
                            anchors.centerIn: parent
                            text: qsTr("执行切割")
                            color: "white"
                            font.pixelSize: 10
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
                    color: "#6b7d94"
                    font.pixelSize: 9
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        Rectangle {
            id: leftPanel
            visible: root.leftPanelVisible
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 14
            anchors.topMargin: 66
            anchors.bottomMargin: 16
            width: 296
            radius: 18
            color: "#1a202bd9"
            border.width: 1
            border.color: Theme.borderSubtle

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingLG
                spacing: Theme.spacingMD

                CxSectionHeader {
                    Layout.fillWidth: true
                    title: qsTr("打印机")
                    subtitle: qsTr("对象与平台管理")
                }

                CxComboBox {
                    Layout.fillWidth: true
                    model: ["Creality K1C 0.4 nozzle", "K1 Max 0.4 nozzle"]
                }

                CxComboBox {
                    Layout.fillWidth: true
                    model: [qsTr("光面PEI板/涂层板"), qsTr("普通PEI板")]
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingSM

                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        radius: 10
                        color: root.editorVm && root.editorVm.showAllObjects ? Theme.accent : Theme.bgElevated
                        border.width: 1
                        border.color: root.editorVm && root.editorVm.showAllObjects ? Theme.accentDark : Theme.borderSubtle

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("全部")
                            color: root.editorVm && root.editorVm.showAllObjects ? Theme.textOnAccent : Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.editorVm) root.editorVm.setShowAllObjects(true)
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        radius: 10
                        color: root.editorVm && !root.editorVm.showAllObjects ? Theme.accent : Theme.bgElevated
                        border.width: 1
                        border.color: root.editorVm && !root.editorVm.showAllObjects ? Theme.accentDark : Theme.borderSubtle

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("对象")
                            color: root.editorVm && !root.editorVm.showAllObjects ? Theme.textOnAccent : Theme.textPrimary
                            font.pixelSize: Theme.fontSizeMD
                            font.bold: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.editorVm) root.editorVm.setShowAllObjects(false)
                        }
                    }
                }

                CxSectionHeader {
                    Layout.fillWidth: true
                    title: qsTr("平板")
                    subtitle: root.editorVm
                        ? (root.editorVm.plateCount + qsTr(" 个工作平板"))
                        : qsTr("0 个工作平板")
                }

                Repeater {
                    model: root.editorVm ? root.editorVm.plateCount : 0

                    delegate: Rectangle {
                        required property int index
                        property bool dragHover: false
                        Layout.fillWidth: true
                        Layout.preferredHeight: 52
                        radius: 12
                        readonly property bool isCurrent: root.editorVm && !root.editorVm.showAllObjects && root.editorVm.currentPlateIndex === index
                        color: dragHover ? Theme.accentSubtle
                            : isCurrent ? Theme.accentSubtle
                            : Theme.bgElevated
                        border.width: dragHover ? 2 : 1
                        border.color: dragHover ? Theme.accent
                            : isCurrent ? Theme.accent
                            : Theme.borderSubtle

                        // 拖拽悬浮高亮叠加（对齐上游 GUI_ObjectList 拖拽放置指示）
                        Rectangle {
                            anchors.fill: parent
                            radius: 12
                            color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.05)
                            visible: parent.dragHover
                        }

                        // 拖拽指示图标（对齐上游拖拽放置反馈）
                        Rectangle {
                            anchors.centerIn: parent
                            width: 44
                            height: 28
                            radius: 6
                            color: Theme.accent
                            opacity: parent.dragHover ? 0.9 : 0.0
                            visible: parent.dragHover
                            Behavior on opacity { NumberAnimation { duration: 150 } }

                            Text {
                                anchors.centerIn: parent
                                text: qsTr("+")
                                color: "#fff"
                                font.pixelSize: 16
                                font.bold: true
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 12
                            spacing: 10

                            // 平板缩略图（对齐上游 PartPlate thumbnail_data）
                            Rectangle {
                                width: 32
                                height: 32
                                radius: 6
                                color: root.editorVm ? root.editorVm.plateThumbnailColor(index) : "#152033"
                                border.width: 1
                                border.color: Theme.borderSubtle

                                // 对象数量标记
                                Text {
                                    anchors.centerIn: parent
                                    text: root.editorVm ? root.editorVm.plateObjectCount(index).toString() : "0"
                                    color: "#c8d4e0"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1

                                Text {
                                    Layout.fillWidth: true
                                    text: root.editorVm ? root.editorVm.plateName(index) : ""
                                    color: Theme.textPrimary
                                    font.pixelSize: 12
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: root.editorVm
                                        ? qsTr("%1 个对象").arg(root.editorVm.plateObjectCount(index))
                                        : qsTr("0 个对象")
                                    color: Theme.textSecondary
                                    font.pixelSize: 10
                                    elide: Text.ElideRight
                                }
                            }

                            Text {
                                text: root.editorVm && !root.editorVm.showAllObjects && root.editorVm.currentPlateIndex === index
                                    ? qsTr("当前")
                                    : qsTr("查看")
                                color: root.editorVm && !root.editorVm.showAllObjects && root.editorVm.currentPlateIndex === index
                                    ? Theme.accent
                                    : Theme.textDisabled
                                font.pixelSize: 10
                                font.bold: true
                            }

                            Image {
                                visible: root.editorVm && root.editorVm.isPlateLocked(index)
                                source: "qrc:/qml/assets/icons/lock.svg"
                                sourceSize.width: 12
                                sourceSize.height: 12
                                opacity: 0.6
                            }

                            Rectangle {
                                visible: root.editorVm && root.editorVm.isPlateSliced(index)
                                width: 8
                                height: 8
                                radius: 4
                                color: Theme.accent
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

                        // 对象拖拽目标（对齐上游跨平板拖拽）
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
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        radius: 8
                        color: Theme.bgElevated
                        border.width: 1
                        border.color: Theme.borderSubtle

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("+ 添加平板")
                            color: Theme.textSecondary
                            font.pixelSize: 11
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.editorVm) root.editorVm.addPlate()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 30
                        radius: 8
                        color: root.editorVm && root.editorVm.plateCount > 1 ? Theme.bgElevated : "transparent"
                        border.width: root.editorVm && root.editorVm.plateCount > 1 ? 1 : 0
                        border.color: Theme.borderSubtle
                        opacity: root.editorVm && root.editorVm.plateCount > 1 ? 1.0 : 0.3

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("- 删除平板")
                            color: Theme.textSecondary
                            font.pixelSize: 11
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: root.editorVm && root.editorVm.plateCount > 1 ? Qt.PointingHandCursor : Qt.ArrowCursor
                            enabled: root.editorVm && root.editorVm.plateCount > 1
                            onClicked: {
                                if (root.editorVm && root.editorVm.plateCount > 1)
                                    root.editorVm.deletePlate(root.editorVm.currentPlateIndex)
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: Theme.borderSubtle
                }

                CxSectionHeader {
                    Layout.fillWidth: true
                    title: qsTr("对象列表")
                    subtitle: root.editorVm
                        ? (root.editorVm.objectCount + qsTr(" 个对象"))
                        : qsTr("0 个对象")
                }

                ObjectList {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    editorVm: root.editorVm
                    showToolbar: false
                    showImportButton: false
                }
            }
        }

        Sidebar {
            id: sidebar
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.topMargin: 14
            anchors.rightMargin: 14
            anchors.bottomMargin: 14
            width: implicitWidth
            editorVm: root.editorVm
            configVm: root.configVm
            processCategory: root.processCategory
        }

        // 转发预览请求到页面导航（对齐上游 Plater::priv::on_preview）
        Connections {
            target: root.editorVm
            function onPreviewRequested() { backend.setCurrentPage(2) }
        }

        // 视角预设按钮（对齐上游 GLCanvas3D 视角预设）
        CxPanel {
            id: viewPresets
            cxSurface: CxPanel.Surface.Floating
            anchors.top: parent.top
            anchors.topMargin: 68
            anchors.right: sidebar.left
            anchors.rightMargin: 14
            width: 46
            height: 186
            radius: 14

            Column {
                anchors.centerIn: parent
                spacing: 0

                Repeater {
                    model: [
                        { label: qsTr("俯"), preset: 0, tip: qsTr("俯视图") },
                        { label: qsTr("前"), preset: 1, tip: qsTr("前视图") },
                        { label: qsTr("右"), preset: 2, tip: qsTr("右视图") },
                        { label: qsTr("轴"), preset: 3, tip: qsTr("等轴视图") },
                        { label: qsTr("适"), preset: -1, tip: qsTr("适应视图") }
                    ]
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        width: 34
                        height: 32
                        radius: 8
                        color: vpMA.containsMouse ? "#1e2535" : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: modelData.label
                            color: vpMA.containsMouse ? Theme.accent : Theme.textPrimary
                            font.pixelSize: 11
                            font.bold: true
                        }

                        MouseArea {
                            id: vpMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (modelData.preset >= 0)
                                    viewport3d.requestViewPreset(modelData.preset)
                                else
                                    root.applyFitHintIfReady()
                            }
                        }
                    }
                }
            }
        }

        Row {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 18
            anchors.left: leftPanel.right
            anchors.leftMargin: 14
            spacing: 8

            // 状态指示器（对齐上游 PartPlateList 底部状态）
            Row {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.editorVm
                        ? (root.editorVm.hasSelection
                           ? qsTr("已选 %1 个对象").arg(root.editorVm.selectedObjectCount)
                           : qsTr("%1 个对象 · %2").arg(root.editorVm.objectCount).arg(root.editorVm.statusText))
                        : ""
                    color: root.editorVm && root.editorVm.hasSelection ? Theme.accent : Theme.textSecondary
                    font.pixelSize: 11
                }

                // 当前平板切片状态指示（对齐上游 PartPlate::m_slice_result_valid）
                Rectangle {
                    visible: root.editorVm && root.editorVm.isPlateSliced(root.editorVm.currentPlateIndex)
                    anchors.verticalCenter: parent.verticalCenter
                    width: slicedText.implicitWidth + 12
                    height: 18
                    radius: 9
                    color: "#0e3325"

                    Label {
                        id: slicedText
                        anchors.centerIn: parent
                        text: qsTr("✓ 已切片")
                        color: "#18c75e"
                        font.pixelSize: 10
                    }
                }

                // 切片中进度
                Rectangle {
                    visible: root.editorVm && root.editorVm.isSlicing()
                    anchors.verticalCenter: parent.verticalCenter
                    width: slicingText.implicitWidth + 12
                    height: 18
                    radius: 9
                    color: "#2e2510"

                    Label {
                        id: slicingText
                        anchors.centerIn: parent
                        text: root.editorVm ? qsTr("切片中 %1%").arg(root.editorVm.sliceProgress()) : ""
                        color: "#e8a838"
                        font.pixelSize: 10
                    }
                }
            }
        }

        Row {
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 18
            anchors.right: sidebar.left
            anchors.rightMargin: 18
            spacing: 8

            CxPillAction {
                iconSource: "qrc:/qml/assets/icons/settings.svg"
                text: qsTr("打印配置")
                onClicked: sidebar.switchToPrintTab()
            }

            CxPillAction {
                iconSource: "qrc:/qml/assets/icons/download.svg"
                text: qsTr("导出 G-code")
                onClicked: exportGCodeDlg.open()
            }

            CxPillAction {
                iconSource: "qrc:/qml/assets/icons/layers.svg"
                text: qsTr("切片全部平板")
                visible: root.editorVm ? root.editorVm.plateCount > 1 : false
                onClicked: {
                    if (root.editorVm)
                        root.editorVm.requestSliceAll()
                }
            }

            CxPillAction {
                iconSource: "qrc:/qml/assets/icons/send-2.svg"
                text: qsTr("发送打印")
                primary: true
                onClicked: printDlg.open()
            }
        }
    }
}
