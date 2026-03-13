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
            if (mod & Qt.ControlModifier) {
                if (mod & Qt.ShiftModifier)
                    viewport3d.redo()
                else
                    viewport3d.undo()
                event.accepted = true
            }
            break
        case Qt.Key_Y:
            if (mod & Qt.ControlModifier) {
                viewport3d.redo()
                event.accepted = true
            }
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
        case Qt.Key_F:
            root.applyFitHintIfReady()
            event.accepted = true
            break
        }
    }

    // Object context menu (right-click)
    Menu {
        id: objectContextMenu

        MenuItem {
            text: qsTr("删除选中")
            onTriggered: if (root.editorVm) root.editorVm.deleteSelectedObjects()
        }
        MenuSeparator { }
        MenuItem {
            text: qsTr("全选")
            shortcut: "Ctrl+A"
            onTriggered: if (root.editorVm) root.editorVm.selectAllVisibleObjects()
        }
        MenuItem {
            text: qsTr("取消选择")
            shortcut: "Esc"
            onTriggered: if (root.editorVm) root.editorVm.clearObjectSelection()
        }
        MenuSeparator { }
        MenuItem {
            text: qsTr("移动模式")
            shortcut: "W"
            onTriggered: viewport3d.gizmoMode = GLViewport.GizmoMove
        }
        MenuItem {
            text: qsTr("旋转模式")
            shortcut: "E"
            onTriggered: viewport3d.gizmoMode = GLViewport.GizmoRotate
        }
        MenuItem {
            text: qsTr("缩放模式")
            shortcut: "R"
            onTriggered: viewport3d.gizmoMode = GLViewport.GizmoScale
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
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/box.svg"
                            toolTipText: qsTr("对象视图")
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
                    }

                    ToolStripDivider { }

                    Row {
                        spacing: 6

                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/layout-grid.svg"
                            toolTipText: qsTr("布局工具")
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/rotate-2.svg"
                            toolTipText: qsTr("重置视角")
                            onClicked: root.applyFitHintIfReady()
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/list-details.svg"
                            toolTipText: qsTr("对象列表")
                        }
                        CxIconButton {
                            buttonSize: 34
                            iconSize: 16
                            iconSource: "qrc:/qml/assets/icons/lock.svg"
                            toolTipText: qsTr("锁定视图")
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
                            iconSource: "qrc:/qml/assets/icons/printer.svg"
                            toolTipText: qsTr("打印机视图")
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

        Rectangle {
            id: leftPanel
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
                        Layout.fillWidth: true
                        Layout.preferredHeight: 42
                        radius: 12
                        color: root.editorVm && !root.editorVm.showAllObjects && root.editorVm.currentPlateIndex === index
                            ? Theme.accentSubtle
                            : Theme.bgElevated
                        border.width: 1
                        border.color: root.editorVm && !root.editorVm.showAllObjects && root.editorVm.currentPlateIndex === index
                            ? Theme.accent
                            : Theme.borderSubtle

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 10

                            Rectangle {
                                width: 22
                                height: 22
                                radius: 7
                                color: "#152033"
                                border.width: 1
                                border.color: Theme.borderSubtle

                                Text {
                                    anchors.centerIn: parent
                                    text: (index + 1).toString()
                                    color: Theme.textPrimary
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
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (!root.editorVm)
                                    return
                                root.editorVm.setCurrentPlateIndex(index)
                                root.editorVm.setShowAllObjects(false)
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
        }

        Column {
            anchors.right: sidebar.left
            anchors.rightMargin: 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 10

            Repeater {
                model: [
                    { icon: "qrc:/qml/assets/icons/folder-plus.svg", tip: qsTr("导入模型") },
                    { icon: "qrc:/qml/assets/icons/list-details.svg", tip: qsTr("对象列表") },
                    { icon: "qrc:/qml/assets/icons/rotate-2.svg", tip: qsTr("视图重置") },
                    { icon: "qrc:/qml/assets/icons/lock.svg", tip: qsTr("锁定视图") },
                    { icon: "qrc:/qml/assets/icons/settings.svg", tip: qsTr("准备页设置") }
                ]
                delegate: CxIconButton {
                    buttonSize: Theme.iconButtonSizeLG
                    iconSize: 16
                    iconSource: modelData.icon
                    toolTipText: modelData.tip
                    onClicked: {
                        if (index === 0)
                            openFileDlg.open()
                        else if (index === 2)
                            root.applyFitHintIfReady()
                        else if (index === 4)
                            backend.openSettings()
                    }
                }
            }
        }

        CxPanel {
            cxSurface: CxPanel.Surface.Floating
            anchors.top: parent.top
            anchors.topMargin: 68
            anchors.right: sidebar.left
            anchors.rightMargin: 14
            width: 46
            height: 62
            radius: 14

            Text {
                anchors.centerIn: parent
                text: qsTr("上\n前")
                color: Theme.textPrimary
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeMD
                font.bold: true
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
