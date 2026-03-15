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

    // GL FBO 缩略图捕获（对齐上游 PartPlate::thumbnail_data）
    function requestGLThumbnail() {
        viewport3d.requestThumbnailCapture(root.editorVm ? root.editorVm.currentPlateIndex : 0, 128)
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
                viewport3d.gizmoMode = GLViewport.GizmoCut
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
        case Qt.Key_P:
            if (!(mod & Qt.ControlModifier)) {
                viewport3d.gizmoMode = GLViewport.GizmoSupportPaint
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
                viewport3d.arrangeSelected(
                    root.editorVm ? root.editorVm.arrangeDistance : 0,
                    root.editorVm ? root.editorVm.arrangeRotation : false,
                    root.editorVm ? root.editorVm.arrangeAlignY : false
                )
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
            property int extruderCount: root.editorVm ? root.editorVm.plateExtruderCount(settingsDlg.plateIndex) : 1
            title: qsTr("平板设置") + " — " + settingsDlg.plateName
            modal: true
            anchors.centerIn: parent
            width: 420

            // 挤出机颜色数组（对齐上游 DragCanvas extruder colors）
            property var extruderColors: ["#FF4444", "#44AA44", "#4444FF", "#FF8800"]

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
                        font.pixelSize: 12
                        Layout.preferredWidth: 100
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

                // 热床类型（对齐上游 BedType: btDefault/btPC/btEP/btPEI/btPTE/btDEF/btER）
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        text: qsTr("热床类型")
                        color: Theme.textSecondary
                        font.pixelSize: 12
                        Layout.preferredWidth: 100
                    }
                    ComboBox {
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

                // 打印顺序（对齐上游 PlateSettingsDialog print sequence: ByDefault/ByLayer/ByObject）
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10
                    Text {
                        text: qsTr("打印顺序")
                        color: Theme.textSecondary
                        font.pixelSize: 12
                        Layout.preferredWidth: 100
                    }
                    ComboBox {
                        id: printSeqCombo
                        Layout.fillWidth: true
                        model: [qsTr("跟随全局"), qsTr("按层打印"), qsTr("按对象打印")]
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
                        Layout.preferredWidth: 100
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
                            font.pixelSize: 12
                            font.bold: true
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Text {
                                text: qsTr("模式")
                                color: Theme.textSecondary
                                font.pixelSize: 11
                                Layout.preferredWidth: 60
                            }
                            ComboBox {
                                id: firstLayerSeqChoiceCombo
                                Layout.fillWidth: true
                                model: [qsTr("自动"), qsTr("自定义")]
                                onActivated: function(index) {
                                    if (root.editorVm) root.editorVm.setPlateFirstLayerSeqChoice(settingsDlg.plateIndex, index)
                                }
                                contentItem: Text {
                                    text: firstLayerSeqChoiceCombo.displayText
                                    color: Theme.textPrimary
                                    font.pixelSize: 11
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 8
                                }
                                background: Rectangle { radius: 5; color: Theme.bgElevated; border.color: Theme.borderSubtle }
                                popup: Popup {
                                    y: firstLayerSeqChoiceCombo.height
                                    width: firstLayerSeqChoiceCombo.width
                                    padding: 4
                                    background: Rectangle { radius: 6; color: Theme.bgSurface; border.color: Theme.borderSubtle; border.width: 1 }
                                    contentItem: ListView {
                                        implicitHeight: contentHeight
                                        model: firstLayerSeqChoiceCombo.model
                                        delegate: ItemDelegate {
                                            width: firstLayerSeqChoiceCombo.width
                                            height: 28
                                            contentItem: Text { text: modelData; color: firstLayerSeqChoiceCombo.highlightedIndex === index ? Theme.textOnAccent : Theme.textPrimary; font.pixelSize: 11 }
                                            highlighted: firstLayerSeqChoiceCombo.highlightedIndex === index
                                            background: Rectangle { color: highlighted ? Theme.accent : "transparent"; radius: 4 }
                                        }
                                    }
                                }
                            }
                        }

                        // 自定义顺序：拖拽排序的挤出机列表（对齐上游 DragCanvas）
                        Text {
                            text: qsTr("挤出机顺序（拖拽调整）")
                            color: Theme.textSecondary
                            font.pixelSize: 11
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
                                                font.pixelSize: 9
                                                font.bold: true
                                                anchors.centerIn: parent
                                            }
                                            // 箭头（非最后一个时显示）
                                            Text {
                                                text: "›"
                                                color: Theme.textDisabled
                                                font.pixelSize: 14
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
                                font.pixelSize: 12
                                font.bold: true
                                Layout.fillWidth: true
                            }
                            ComboBox {
                                id: otherLayersSeqChoiceCombo
                                Layout.preferredWidth: 100
                                model: [qsTr("自动"), qsTr("自定义")]
                                onActivated: function(index) {
                                    if (root.editorVm) root.editorVm.setPlateOtherLayersSeqChoice(settingsDlg.plateIndex, index)
                                }
                                contentItem: Text {
                                    text: otherLayersSeqChoiceCombo.displayText
                                    color: Theme.textPrimary
                                    font.pixelSize: 11
                                    verticalAlignment: Text.AlignVCenter
                                    leftPadding: 8
                                }
                                background: Rectangle { radius: 5; color: Theme.bgElevated; border.color: Theme.borderSubtle }
                                popup: Popup {
                                    y: otherLayersSeqChoiceCombo.height
                                    width: otherLayersSeqChoiceCombo.width
                                    padding: 4
                                    background: Rectangle { radius: 6; color: Theme.bgSurface; border.color: Theme.borderSubtle; border.width: 1 }
                                    contentItem: ListView {
                                        implicitHeight: contentHeight
                                        model: otherLayersSeqChoiceCombo.model
                                        delegate: ItemDelegate {
                                            width: otherLayersSeqChoiceCombo.width
                                            height: 28
                                            contentItem: Text { text: modelData; color: otherLayersSeqChoiceCombo.highlightedIndex === index ? Theme.textOnAccent : Theme.textPrimary; font.pixelSize: 11 }
                                            highlighted: otherLayersSeqChoiceCombo.highlightedIndex === index
                                            background: Rectangle { color: highlighted ? Theme.accent : "transparent"; radius: 4 }
                                        }
                                    }
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
                                font.pixelSize: 10
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
                                                font.pixelSize: 10
                                            }
                                            TextField {
                                                id: beginField
                                                Layout.preferredWidth: 50
                                                text: root.editorVm ? root.editorVm.plateOtherLayersSeqBegin(settingsDlg.plateIndex, delegateModel.entryIndex) : 2
                                                font.pixelSize: 10
                                                color: Theme.textPrimary
                                                horizontalAlignment: TextInput.AlignHCenter
                                                background: Rectangle { radius: 4; color: Theme.bgSurface; border.color: Theme.borderSubtle }
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
                                                font.pixelSize: 10
                                            }
                                            Text {
                                                text: qsTr("结束层")
                                                color: Theme.textSecondary
                                                font.pixelSize: 10
                                            }
                                            TextField {
                                                id: endField
                                                Layout.preferredWidth: 50
                                                text: root.editorVm ? root.editorVm.plateOtherLayersSeqEnd(settingsDlg.plateIndex, delegateModel.entryIndex) : 100
                                                font.pixelSize: 10
                                                color: Theme.textPrimary
                                                horizontalAlignment: TextInput.AlignHCenter
                                                background: Rectangle { radius: 4; color: Theme.bgSurface; border.color: Theme.borderSubtle }
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
                                                color: removeBtnMA.containsMouse ? "#FF4444" : Theme.bgSurface
                                                border.color: Theme.borderSubtle
                                                Text { anchors.centerIn: parent; text: "✕"; color: removeBtnMA.containsMouse ? "white" : Theme.textDisabled; font.pixelSize: 10 }
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
                                                                font.pixelSize: 12
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
                                    Text { id: addSeqBtnText; anchors.centerIn: parent; text: qsTr("+ 添加层范围"); color: addSeqBtnMA.containsMouse ? Theme.textOnAccent : Theme.accent; font.pixelSize: 10 }
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
    Popup {
        id: arrangeSettingsPopup
        anchors.centerIn: parent
        width: 320
        padding: 20
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            radius: 10
            color: Theme.bgElevated
            border.width: 1
            border.color: Theme.borderSubtle
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 14

            // 标题行
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: qsTr("排列设置")
                    font.pixelSize: 14
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
                    font.pixelSize: 12
                    Layout.preferredWidth: 90
                }
                TextField {
                    id: arrangeDistField
                    Layout.fillWidth: true
                    implicitHeight: 28
                    text: root.editorVm ? (root.editorVm.arrangeDistance > 0 ? root.editorVm.arrangeDistance.toFixed(1) : "0") : "0"
                    horizontalAlignment: Text.AlignHCenter
                    color: Theme.textPrimary
                    font.pixelSize: 12
                    selectByMouse: true
                    validator: DoubleValidator { bottom: 0; top: 100 }
                    background: Rectangle {
                        radius: 4
                        color: Theme.bgBase
                        border.width: 1
                        border.color: arrangeDistField.activeFocus ? Theme.accent : Theme.borderSubtle
                    }
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
                    font.pixelSize: 11
                }
            }

            // 自动旋转
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                Text {
                    text: qsTr("自动旋转")
                    color: Theme.textSecondary
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
                Switch {
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
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
                Switch {
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
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
                Switch {
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
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
                Switch {
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
                        viewport3d.arrangeSelected(
                            root.editorVm ? root.editorVm.arrangeDistance : 0,
                            root.editorVm ? root.editorVm.arrangeRotation : false,
                            root.editorVm ? root.editorVm.arrangeAlignY : false
                        )
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

        // ── 视口告警覆盖层（对齐上游 EWarning / Plater::_set_warning_notification）──
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 10
            width: warningContent.implicitWidth + 28
            height: warningContent.implicitHeight + 16
            radius: 8
            color: root.editorVm && root.editorVm.viewportWarning === 2 ? "#4a1c1c" : "#3a3420"
            border.width: 1
            border.color: root.editorVm && root.editorVm.viewportWarning === 2 ? "#ef4444" : "#f59e0b"
            visible: root.editorVm ? root.editorVm.hasViewportWarning : false
            opacity: visible ? 1.0 : 0.0
            Behavior on opacity { NumberAnimation { duration: 200 } }

            RowLayout {
                id: warningContent
                anchors.centerIn: parent
                spacing: 8

                Text {
                    text: root.editorVm && root.editorVm.viewportWarning === 2 ? "⚠" : "⚡"
                    font.pixelSize: 14
                    color: root.editorVm && root.editorVm.viewportWarning === 2 ? "#ef4444" : "#f59e0b"
                }

                Text {
                    text: root.editorVm ? root.editorVm.viewportWarningMessage : ""
                    color: "#e2e8f1"
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
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
                            iconSource: "qrc:/qml/assets/icons/layout-grid-plus.svg"
                            toolTipText: qsTr("添加平板")
                            enabled: root.editorVm && root.editorVm.plateCount < 10
                            onClicked: root.editorVm.addPlate()
                        }
                    }

                    ToolStripDivider { }

                    Row {
                        spacing: 6

                        // 删除选中 (对齐上游 GLToolbar delete)
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/trash.svg"
                            toolTipText: qsTr("删除选中 (Delete)")
                            enabled: root.editorVm && root.editorVm.hasSelection
                            onClicked: root.editorVm.deleteSelectedObjects()
                        }
                        // 清空全部 (对齐上游 GLToolbar delete_all)
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/x.svg"
                            toolTipText: qsTr("清空全部")
                            enabled: root.editorVm && root.editorVm.objectCount > 0
                            onClicked: root.editorVm.clearWorkspace()
                        }
                        // 复制 (对齐上游 GLToolbar copy)
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/copy.svg"
                            toolTipText: qsTr("复制 (Ctrl+C)")
                            enabled: root.editorVm && root.editorVm.hasSelection
                            onClicked: root.editorVm.copySelectedObjects()
                        }
                        // 粘贴 (对齐上游 GLToolbar paste)
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/clipboard.svg"
                            toolTipText: qsTr("粘贴 (Ctrl+V)")
                            enabled: root.editorVm && root.editorVm.hasClipboardContent
                            onClicked: root.editorVm.pasteObjects()
                        }
                        // 克隆 (对齐上游 GLToolbar clone)
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/clone.svg"
                            toolTipText: qsTr("克隆 (Ctrl+D)")
                            enabled: root.editorVm && root.editorVm.hasSelection
                            onClicked: root.editorVm.duplicateSelectedObjects()
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
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            selected: viewport3d.gizmoMode === GLViewport.GizmoSupportPaint
                            iconSource: "qrc:/qml/assets/icons/layers.svg"
                            toolTipText: qsTr("支撑绘制 (P)")
                            onClicked: viewport3d.gizmoMode = GLViewport.GizmoSupportPaint
                        }
                    }

                    ToolStripDivider { }

                    Row {
                        spacing: 6

                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/layout-grid.svg"
                            toolTipText: qsTr("排列设置")
                            onClicked: arrangeSettingsPopup.open()
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
                            color: root.editorVm && root.editorVm.measureSelectionMode === index ? "#1c2a3e" : "#1a1e28"
                            border.color: root.editorVm && root.editorVm.measureSelectionMode === index ? "#569cd6" : "#2e3444"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.measureSelectionMode === index ? "#569cd6" : "#8a96a8"
                                font.pixelSize: 10
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
                Label {
                    text: root.editorVm && root.editorVm.measureSelectionMode === 1
                        ? qsTr("点击网格面拾取特征 (点/边/圆/平面)")
                        : qsTr("点测量模式 — 显示选中对象尺寸")
                    color: "#6b7d94"
                    font.pixelSize: 9
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
                            color: root.editorVm && root.editorVm.cutMode === index ? "#1c2a3e" : "#1a1e28"
                            border.color: root.editorVm && root.editorVm.cutMode === index ? "#5b8def" : "#2e3444"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.cutMode === index ? "#5b8def" : "#8a96a8"
                                font.pixelSize: 10
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
                    Text { text: qsTr("类型:"); color: "#8b949e"; font.pixelSize: 10; anchors.verticalCenter: parent.verticalCenter }
                    Repeater {
                        model: [qsTr("Plug"), qsTr("Dowel"), qsTr("Snap")]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: 52; height: 22; radius: 4
                            color: root.editorVm && root.editorVm.connectorType === index ? "#1c2a3e" : "#1a1e28"
                            border.color: root.editorVm && root.editorVm.connectorType === index ? "#e8a838" : "#2e3444"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.connectorType === index ? "#e8a838" : "#8a96a8"
                                font.pixelSize: 9
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
                    Text { text: qsTr("样式:"); color: "#8b949e"; font.pixelSize: 10; anchors.verticalCenter: parent.verticalCenter }
                    Repeater {
                        model: [qsTr("Prism"), qsTr("Frustum")]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: 56; height: 22; radius: 4
                            color: root.editorVm && root.editorVm.connectorStyle === index ? "#1c2a3e" : "#1a1e28"
                            border.color: root.editorVm && root.editorVm.connectorStyle === index ? "#e8a838" : "#2e3444"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.connectorStyle === index ? "#e8a838" : "#8a96a8"
                                font.pixelSize: 9
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
                    Text { text: qsTr("形状:"); color: "#8b949e"; font.pixelSize: 10; anchors.verticalCenter: parent.verticalCenter }
                    Repeater {
                        model: ["△", "□", "⬡", "○"]
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            width: 28; height: 22; radius: 4
                            color: root.editorVm && root.editorVm.connectorShape === index ? "#1c2a3e" : "#1a1e28"
                            border.color: root.editorVm && root.editorVm.connectorShape === index ? "#e8a838" : "#2e3444"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                color: root.editorVm && root.editorVm.connectorShape === index ? "#e8a838" : "#8a96a8"
                                font.pixelSize: 11
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
                    Text { text: qsTr("尺寸:"); color: "#8b949e"; font.pixelSize: 10 }
                    Slider {
                        from: 2; to: 20; stepSize: 0.5
                        value: root.editorVm ? root.editorVm.connectorSize : 5
                        implicitWidth: 80
                        onMoved: if (root.editorVm) root.editorVm.connectorSize = value
                    }
                    Text {
                        text: root.editorVm ? root.editorVm.connectorSize.toFixed(1) : "5.0"
                        color: "#c8d4e0"
                        font.pixelSize: 10
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 30
                    }
                    Text { text: qsTr("深度:"); color: "#8b949e"; font.pixelSize: 10 }
                    Slider {
                        from: 0.1; to: 1.0; stepSize: 0.05
                        value: root.editorVm ? root.editorVm.connectorDepth : 0.5
                        implicitWidth: 60
                        onMoved: if (root.editorVm) root.editorVm.connectorDepth = value
                    }
                    Text {
                        text: root.editorVm ? (root.editorVm.connectorDepth * 100).toFixed(0) + "%" : "50%"
                        color: "#c8d4e0"
                        font.pixelSize: 10
                        font.family: "Consolas, monospace"
                        Layout.preferredWidth: 30
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

                            // 平板缩略图（对齐上游 PartPlate::thumbnail_data，优先使用 GL FBO 捕获）
                            Rectangle {
                                width: 48
                                height: 48
                                radius: 6
                                color: Theme.bgElevated
                                border.width: 1
                                border.color: Theme.borderSubtle
                                clip: true

                                Image {
                                    anchors.fill: parent
                                    anchors.margins: 2
                                    fillMode: Image.PreserveAspectFit
                                    smooth: true
                                    // 优先使用 GL FBO 缩略图，回退到 QPainter 合成
                                    source: {
                                        if (!root.editorVm) return ""
                                        var glThumb = viewport3d.lastThumbnailData
                                        if (glThumb.length > 0 && index === root.editorVm.currentPlateIndex)
                                            return "data:image/png;base64," + glThumb
                                        return "data:image/png;base64," + root.editorVm.generatePlateThumbnail(index, 96)
                                    }
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
            // 平板切换或对象变更时请求 GL 缩略图更新（对齐上游 Plater::update_plate_thumbnails）
            function onCurrentPlateIndexChanged() { root.requestGLThumbnail() }
        }

        // GL FBO 缩略图捕获完成后刷新平板列表（对齐上游 PartPlate::thumbnail_data 回写）
        Connections {
            target: viewport3d
            function onThumbnailCaptured() { /* plate cards auto-rebind via lastThumbnailData property */ }
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
