import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import ".."

// C3 — 对象列表面板
// 绑定到 EditorViewModel Q_INVOKABLE 访问器，避免 QVariantList 暴露到 QML
Item {
    id: root
    required property var editorVm
    property bool showToolbar: true
    property bool showImportButton: true

    // ── 顶部工具栏 ──────────────────────────────────────────────
    Rectangle {
        id: toolbar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 36
        radius: 12
        color: Theme.bgPanel
        border.width: 1
        border.color: Theme.borderSubtle
        visible: root.showToolbar

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 8
            spacing: 4

            Text {
                text: qsTr("对象列表")
                color: Theme.textSecondary
                font.pixelSize: Theme.fontSizeSM
                font.bold: true
            }
            Rectangle {
                width: 54; height: 22; radius: 7
                color: root.editorVm && !root.editorVm.showAllObjects ? Theme.accent : Theme.bgElevated
                Text { anchors.centerIn: parent; text: qsTr("当前盘"); color: root.editorVm && !root.editorVm.showAllObjects ? Theme.textOnAccent : Theme.textPrimary; font.pixelSize: 9 }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root.editorVm) root.editorVm.setShowAllObjects(false) }
                }
            }
            Rectangle {
                width: 42; height: 22; radius: 7
                color: root.editorVm && root.editorVm.showAllObjects ? Theme.accent : Theme.bgElevated
                Text { anchors.centerIn: parent; text: qsTr("全部"); color: root.editorVm && root.editorVm.showAllObjects ? Theme.textOnAccent : Theme.textPrimary; font.pixelSize: 9 }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root.editorVm) root.editorVm.setShowAllObjects(true) }
                }
            }
            Rectangle {
                width: 44; height: 22; radius: 7
                color: root.editorVm && root.editorVm.objectOrganizeMode === 0 ? Theme.accent : Theme.bgElevated
                Text { anchors.centerIn: parent; text: qsTr("按盘"); color: root.editorVm && root.editorVm.objectOrganizeMode === 0 ? Theme.textOnAccent : Theme.textPrimary; font.pixelSize: 9 }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root.editorVm) root.editorVm.setObjectOrganizeMode(0) }
                }
            }
            Rectangle {
                width: 50; height: 22; radius: 7
                color: root.editorVm && root.editorVm.objectOrganizeMode === 1 ? Theme.accent : Theme.bgElevated
                Text { anchors.centerIn: parent; text: qsTr("按模块"); color: root.editorVm && root.editorVm.objectOrganizeMode === 1 ? Theme.textOnAccent : Theme.textPrimary; font.pixelSize: 9 }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root.editorVm) root.editorVm.setObjectOrganizeMode(1) }
                }
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                visible: !!root.editorVm && root.editorVm.hasSelection
                radius: 7
                color: Theme.accentSubtle
                border.width: 1
                border.color: Theme.borderSubtle
                implicitWidth: selectedCountText.implicitWidth + 10
                implicitHeight: 22

                Text {
                    id: selectedCountText
                    anchors.centerIn: parent
                    text: !root.editorVm ? ""
                          : root.editorVm.hasSelectedVolume ? qsTr("已选 %1 个部件").arg(root.editorVm.selectedVolumeCount)
                          : qsTr("已选 %1 项").arg(root.editorVm.selectedObjectCount)
                    color: Theme.textPrimary
                    font.pixelSize: 10
                    font.bold: true
                }
            }
            Rectangle {
                visible: !!root.editorVm && !root.editorVm.hasSelectedVolume && root.editorVm.objectCount > 1
                width: 38
                height: 22
                radius: 7
                color: selectAllMA.containsMouse ? Theme.bgHover : Theme.bgElevated
                Text { anchors.centerIn: parent; text: qsTr("全选"); color: Theme.textSecondary; font.pixelSize: 9 }
                MouseArea {
                    id: selectAllMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root.editorVm) root.editorVm.selectAllVisibleObjects() }
                }
            }
            Rectangle {
                visible: !!root.editorVm && root.editorVm.hasSelection
                width: 38
                height: 22
                radius: 7
                color: clearSelMA.containsMouse ? Theme.bgHover : Theme.bgElevated
                Text { anchors.centerIn: parent; text: qsTr("清空"); color: Theme.textSecondary; font.pixelSize: 9 }
                MouseArea {
                    id: clearSelMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root.editorVm) root.editorVm.clearObjectSelection() }
                }
            }
            Rectangle {
                visible: !!root.editorVm && root.editorVm.canOpenSelectionSettings
                width: 38
                height: 22
                radius: 7
                color: settingsMA.containsMouse ? Theme.bgHover : Theme.bgElevated
                Text { anchors.centerIn: parent; text: qsTr("参数"); color: Theme.textSecondary; font.pixelSize: 9 }
                MouseArea {
                    id: settingsMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root.editorVm) root.editorVm.requestSelectionSettings() }
                }
            }
            Rectangle {
                visible: !!root.editorVm && !root.editorVm.hasSelectedVolume && root.editorVm.selectedObjectCount > 0
                width: 38
                height: 22
                radius: 7
                color: hideSelMA.containsMouse ? Theme.bgHover : Theme.bgElevated
                Text { anchors.centerIn: parent; text: qsTr("禁打"); color: Theme.textSecondary; font.pixelSize: 9 }
                MouseArea {
                    id: hideSelMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root.editorVm) root.editorVm.setSelectedObjectsPrintable(false) }
                }
            }
            Rectangle {
                visible: !!root.editorVm && !root.editorVm.hasSelectedVolume && root.editorVm.selectedObjectCount > 0
                width: 38
                height: 22
                radius: 7
                color: showSelMA.containsMouse ? Theme.bgHover : Theme.bgElevated
                Text { anchors.centerIn: parent; text: qsTr("打印"); color: Theme.textSecondary; font.pixelSize: 9 }
                MouseArea {
                    id: showSelMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root.editorVm) root.editorVm.setSelectedObjectsPrintable(true) }
                }
            }
            Rectangle {
                visible: !!root.editorVm && root.editorVm.hasSelection
                width: 38
                height: 22
                radius: 7
                color: deleteSelMA.containsMouse ? "#7d2020" : Theme.bgElevated
                Text { anchors.centerIn: parent; text: qsTr("删除"); color: deleteSelMA.containsMouse ? "#ffaaaa" : Theme.textSecondary; font.pixelSize: 9 }
                MouseArea {
                    id: deleteSelMA
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root.editorVm) root.editorVm.deleteSelection() }
                }
            }
            Text {
                text: root.editorVm ? root.editorVm.objectCount + qsTr(" 个") : "0" + qsTr(" 个")
                color: Theme.textDisabled
                font.pixelSize: 11
            }
        }
    }

    // ── 对象列表 ─────────────────────────────────────────────────
    ListView {
        id: listView
        anchors.top: root.showToolbar ? toolbar.bottom : parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: root.showImportButton ? importBtn.top : parent.bottom
        anchors.topMargin: root.showToolbar ? 0 : 4
        anchors.bottomMargin: root.showImportButton ? 6 : 0
        clip: true
        model: root.editorVm ? root.editorVm.objectCount : 0

        // Drag reorder state (对齐上游 GUI_ObjectList OnBeginDrag/OnDrop)
        property int dragSourceIndex: -1

        delegate: Item {
            id: row
            width: listView.width
            height: baseBlock.y + baseBlock.height + childColumn.height + 4
            required property int index

            readonly property bool isSelected: root.editorVm
                                               && root.editorVm.isObjectSelected(row.index)
            readonly property bool objPrintable: root.editorVm
                                                 && root.editorVm.objectPrintable(row.index)
            readonly property int plateIndex: root.editorVm ? root.editorVm.objectPlateIndex(row.index) : -1
            readonly property string plateLabel: root.editorVm ? root.editorVm.objectPlateName(row.index) : ""
            readonly property string moduleLabel: root.editorVm ? root.editorVm.objectModuleName(row.index) : ""
            readonly property string groupLabel: root.editorVm ? root.editorVm.objectGroupLabel(row.index) : ""
            readonly property bool groupExpanded: root.editorVm ? root.editorVm.objectGroupExpanded(row.index) : true
            readonly property int volumeCount: root.editorVm ? root.editorVm.objectVolumeCount(row.index) : 0
            readonly property bool objectExpanded: root.editorVm ? root.editorVm.objectExpanded(row.index) : true
            readonly property bool hasVolumeChildren: row.volumeCount > 0
            readonly property bool showGroupHeader: root.editorVm
                                                    && row.groupLabel.length > 0
                                                    && (row.index === 0
                                                        || root.editorVm.objectGroupLabel(row.index - 1) !== row.groupLabel)

            // Inline rename state (对齐上游 GUI_ObjectList rename_item via double-click)
            property bool renaming: false
            property string renameText: ""

            // Drag hover state (for insertion line)
            property bool dragHover: false

            // Drop target highlight during drag (对齐上游 GUI_ObjectList OnBeginDrag 视觉反馈)
            Rectangle {
                anchors.fill: parent
                anchors.margins: -1
                radius: 12
                color: row.dragHover ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.08) : "transparent"
                border.width: (listView.dragSourceIndex >= 0 && listView.dragSourceIndex !== row.index) ? 2 : 0
                border.color: row.dragHover ? Theme.accent : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.4)
                visible: listView.dragSourceIndex >= 0 && listView.dragSourceIndex !== row.index
                z: -1
            }

            // Insertion line indicator during drag (对齐上游拖拽放置位置指示)
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: groupHeader.visible ? groupHeader.top : parent.top
                height: 2
                radius: 1
                color: Theme.accent
                visible: listView.dragSourceIndex >= 0 && row.dragHover && listView.dragSourceIndex !== row.index
            }

            // Drag source row opacity reduction
            states: [
                State {
                    name: "dragging"
                    when: listView.dragSourceIndex === row.index
                    PropertyChanges { target: row; opacity: 0.4 }
                }
            ]

            Menu {
                id: rowMenu

                MenuItem {
                    text: qsTr("查看所在平板")
                    enabled: root.editorVm && row.plateIndex >= 0
                    onTriggered: {
                        if (!root.editorVm || row.plateIndex < 0)
                            return
                        root.editorVm.setCurrentPlateIndex(row.plateIndex)
                        root.editorVm.setShowAllObjects(false)
                    }
                }

                MenuItem {
                    text: row.objPrintable ? qsTr("设为不参与打印") : qsTr("设为可打印")
                    enabled: !!root.editorVm
                    onTriggered: {
                        if (root.editorVm)
                            root.editorVm.setObjectPrintable(row.index, !row.objPrintable)
                    }
                }

                MenuSeparator {
                    visible: !!root.editorVm && row.isSelected && root.editorVm.selectedObjectCount > 1
                }

                MenuItem {
                    visible: !!root.editorVm && row.isSelected && root.editorVm.selectedObjectCount > 1
                    text: qsTr("设为不参与打印")
                    onTriggered: {
                        if (root.editorVm)
                            root.editorVm.setSelectedObjectsPrintable(false)
                    }
                }

                MenuItem {
                    visible: !!root.editorVm && row.isSelected && root.editorVm.selectedObjectCount > 1
                    text: qsTr("设为可打印")
                    onTriggered: {
                        if (root.editorVm)
                            root.editorVm.setSelectedObjectsPrintable(true)
                    }
                }

                MenuSeparator { }

                MenuItem {
                    text: qsTr("重命名")
                    enabled: !!root.editorVm && row.isSelected && root.editorVm.selectedObjectCount === 1
                    onTriggered: {
                        if (root.editorVm) {
                            row.renaming = true
                            row.renameText = root.editorVm.objectName(row.index)
                        }
                    }
                }

                MenuSeparator { }

                // ── Add Volume 子菜单 (对齐上游 GUI_Factories::append_menu_items_add_volume) ──
                Menu {
                    title: qsTr("添加部件")
                    enabled: !!root.editorVm && row.isSelected && root.editorVm.selectedObjectCount === 1 && !root.editorVm.hasSelectedVolume

                    MenuItem {
                        text: qsTr("添加部件")
                        onTriggered: {
                            if (root.editorVm) root.editorVm.addVolumeToObject(0)
                        }
                    }
                    MenuItem {
                        text: qsTr("添加负体积")
                        onTriggered: {
                            if (root.editorVm) root.editorVm.addVolumeToObject(1)
                        }
                    }
                    MenuItem {
                        text: qsTr("添加修改器")
                        onTriggered: {
                            if (root.editorVm) root.editorVm.addVolumeToObject(2)
                        }
                    }
                    MenuItem {
                        text: qsTr("添加支撑屏蔽")
                        onTriggered: {
                            if (root.editorVm) root.editorVm.addVolumeToObject(3)
                        }
                    }
                    MenuItem {
                        text: qsTr("添加支撑增强")
                        onTriggered: {
                            if (root.editorVm) root.editorVm.addVolumeToObject(4)
                        }
                    }

                    MenuSeparator { }

                    // 文字浮雕（对齐上游 GLGizmoText）
                    MenuItem {
                        text: qsTr("文字浮雕")
                        onTriggered: {
                            if (!root.editorVm) return
                            addTextDialogComp.createObject(root, {
                                objectIndex: row.index,
                                editorVm: root.editorVm
                            }).open()
                        }
                    }

                    // SVG 浮雕（对齐上游 GLGizmoSVG）
                    MenuItem {
                        text: qsTr("导入 SVG 浮雕...")
                        onTriggered: {
                            if (!root.editorVm) return
                            svgFileDialogComp.createObject(root, {
                                objectIndex: row.index,
                                editorVm: root.editorVm
                            }).open()
                        }
                    }

                    MenuSeparator { }

                    // 从文件导入（对齐上游 GUI_ObjectList::load_generic_subobject 文件加载）
                    MenuItem {
                        text: qsTr("从文件导入部件...")
                        onTriggered: {
                            if (!root.editorVm) return
                            volumeFileDialogComp.createObject(root, {
                                objectIndex: row.index,
                                editorVm: root.editorVm
                            }).open()
                        }
                    }

                    MenuSeparator { }

                    // 原始体创建（对齐上游 create_mesh + add_volume）
                    Menu {
                        title: qsTr("添加原始体")

                        MenuItem {
                            text: qsTr("立方体")
                            onTriggered: {
                                if (root.editorVm) root.editorVm.addPrimitive(row.index, 0)
                            }
                        }
                        MenuItem {
                            text: qsTr("球体")
                            onTriggered: {
                                if (root.editorVm) root.editorVm.addPrimitive(row.index, 1)
                            }
                        }
                        MenuItem {
                            text: qsTr("圆柱体")
                            onTriggered: {
                                if (root.editorVm) root.editorVm.addPrimitive(row.index, 2)
                            }
                        }
                        MenuItem {
                            text: qsTr("圆环")
                            onTriggered: {
                                if (root.editorVm) root.editorVm.addPrimitive(row.index, 3)
                            }
                        }
                    }
                }

                // Change volume type submenu (对齐上游 GUI_ObjectList::load_generic_subobject type conversion)
                Menu {
                    title: qsTr("转换为")
                    visible: !!root.editorVm && root.editorVm.hasSelectedVolume
                    enabled: !!root.editorVm && root.editorVm.hasSelectedVolume

                    MenuItem {
                        text: qsTr("部件")
                        onTriggered: { if (root.editorVm) root.editorVm.changeVolumeType(0) }
                    }
                    MenuItem {
                        text: qsTr("负体积")
                        onTriggered: { if (root.editorVm) root.editorVm.changeVolumeType(1) }
                    }
                    MenuItem {
                        text: qsTr("修改器")
                        onTriggered: { if (root.editorVm) root.editorVm.changeVolumeType(2) }
                    }
                    MenuItem {
                        text: qsTr("支撑屏蔽")
                        onTriggered: { if (root.editorVm) root.editorVm.changeVolumeType(3) }
                    }
                    MenuItem {
                        text: qsTr("支撑增强")
                        onTriggered: { if (root.editorVm) root.editorVm.changeVolumeType(4) }
                    }
                }

                MenuSeparator { }

                MenuItem {
                    text: qsTr("在参数表中编辑")
                    enabled: !!root.editorVm && row.isSelected && root.editorVm.canOpenSelectionSettings && !root.editorVm.hasSelectedVolume
                    onTriggered: {
                        if (root.editorVm)
                            root.editorVm.requestSelectionSettings()
                    }
                }

                MenuSeparator { }

                // 复制/粘贴/克隆/剪切（对齐上游 create_extra_object_menu）
                MenuItem {
                    text: qsTr("复制")
                    enabled: !!root.editorVm && root.editorVm.hasSelection && !root.editorVm.hasSelectedVolume
                    onTriggered: { if (root.editorVm) root.editorVm.copySelectedObjects() }
                }

                MenuItem {
                    text: qsTr("剪切")
                    enabled: !!root.editorVm && root.editorVm.hasSelection && !root.editorVm.hasSelectedVolume
                    onTriggered: { if (root.editorVm) root.editorVm.cutSelectedObjects() }
                }

                MenuItem {
                    text: qsTr("克隆")
                    enabled: !!root.editorVm && row.isSelected && !root.editorVm.hasSelectedVolume
                    onTriggered: { if (root.editorVm) root.editorVm.duplicateSelectedObjects() }
                }

                MenuSeparator { }

                // 居中/镜像（对齐上游 create_extra_object_menu center/mirror）
                MenuItem {
                    text: qsTr("居中到热床")
                    enabled: !!root.editorVm && root.editorVm.hasSelection
                    onTriggered: { if (root.editorVm) root.editorVm.centerSelectedObjects() }
                }

                MenuSeparator { }

                MenuItem {
                    text: row.isSelected && root.editorVm && root.editorVm.selectedObjectCount > 1
                          ? qsTr("删除已选对象")
                          : qsTr("删除对象")
                    enabled: !!root.editorVm
                    onTriggered: {
                        if (!root.editorVm)
                            return
                        if (row.isSelected && root.editorVm.selectedObjectCount > 1)
                            root.editorVm.deleteSelectedObjects()
                        else
                            root.editorVm.deleteObject(row.index)
                    }
                }
            }

            Rectangle {
                id: groupHeader
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 20
                radius: 8
                visible: row.showGroupHeader
                color: "#111722"
                border.width: 1
                border.color: Theme.borderSubtle

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 6

                    Text {
                        text: row.groupExpanded ? "-" : "+"
                        color: Theme.textDisabled
                        font.pixelSize: 11
                        font.bold: true
                    }

                    Text {
                        text: row.groupLabel
                        color: Theme.textSecondary
                        font.pixelSize: 10
                        font.bold: true
                    }

                    Rectangle {
                        width: 4
                        height: 4
                        radius: 2
                        color: Theme.textDisabled
                    }

                    Text {
                        text: root.editorVm
                            ? qsTr("%1 个对象").arg(root.editorVm.objectGroupCount(row.index))
                            : qsTr("0 个对象")
                        color: Theme.textDisabled
                        font.pixelSize: 9
                    }

                    Item { Layout.fillWidth: true }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root.editorVm)
                            root.editorVm.toggleObjectGroupExpanded(row.index)
                    }
                }
            }

            // 背景
            Rectangle {
                visible: row.groupExpanded
                id: baseBlock
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: groupHeader.visible ? groupHeader.bottom : parent.top
                height: 54
                radius: 10
                color: row.isSelected ? Theme.accentSubtle
                     : selMA.containsMouse ? Theme.bgHover
                     : "transparent"
                Rectangle {
                    // 左侧选中指示条
                    width: 3; height: parent.height - 10
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 2
                    radius: 2
                    color: row.isSelected ? Theme.accent : "transparent"
                }
            }

            RowLayout {
                visible: row.groupExpanded
                    id: objectRow
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: groupHeader.visible ? groupHeader.bottom : parent.top
                anchors.leftMargin: 12
                anchors.rightMargin: 6
                spacing: 8

                    Rectangle {
                        visible: row.hasVolumeChildren
                        width: 16
                        height: 16
                        radius: 5
                        color: objectExpandMA.containsMouse ? Theme.bgHover : "transparent"

                        Text {
                            anchors.centerIn: parent
                            text: row.objectExpanded ? "-" : "+"
                            color: Theme.textDisabled
                            font.pixelSize: 11
                            font.bold: true
                        }

                        MouseArea {
                            id: objectExpandMA
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (root.editorVm)
                                    root.editorVm.toggleObjectExpanded(row.index)
                            }
                        }
                    }

                    Item {
                        visible: !row.hasVolumeChildren
                        width: visible ? 16 : 0
                        height: 16
                    }

                // 可见性切换小圆点
                Rectangle {
                    width: 9; height: 9; radius: 5
                    color: row.objPrintable ? Theme.accent : Theme.bgPressed
                    MouseArea {
                        anchors.fill: parent
                        z: 2
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (root.editorVm)
                            {
                                if (row.isSelected && root.editorVm.selectedObjectCount > 1)
                                    root.editorVm.setSelectedObjectsPrintable(!row.objPrintable)
                                else
                                    root.editorVm.setObjectPrintable(row.index, !row.objPrintable)
                            }
                        }
                    }
                }

                // 文件名与 plate 信息
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    // Name display / inline rename (对齐上游 GUI_ObjectList rename_item)
                    TextField {
                        id: renameField
                        Layout.fillWidth: true
                        visible: row.renaming
                        text: row.renameText
                        color: Theme.textPrimary
                        font.pixelSize: 12
                        background: Rectangle {
                            radius: 4
                            color: Theme.bgElevated
                            border.color: Theme.accent
                            border.width: 1
                        }
                        onAccepted: {
                            if (root.editorVm && row.renameText.trim().length > 0) {
                                root.editorVm.renameObject(row.index, row.renameText.trim())
                            }
                            row.renaming = false
                        }
                        onTextChanged: row.renameText = text
                        onActiveFocusChanged: {
                            if (!activeFocus && row.renaming) {
                                if (root.editorVm && row.renameText.trim().length > 0) {
                                    root.editorVm.renameObject(row.index, row.renameText.trim())
                                }
                                row.renaming = false
                            }
                        }
                        Component.onCompleted: selectAll()
                    }

                    Text {
                        Layout.fillWidth: true
                        visible: !row.renaming
                        text: root.editorVm ? root.editorVm.objectName(row.index) : ""
                        color: row.isSelected ? Theme.textPrimary : "#bbc7d4"
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        visible: row.plateLabel.length > 0 || !row.objPrintable
                                 || (root.editorVm && root.editorVm.objectOrganizeMode === 0 && row.moduleLabel.length > 0)

                        Rectangle {
                            visible: row.plateLabel.length > 0
                            radius: 6
                            color: "#243247"
                            border.width: 1
                            border.color: Theme.borderSubtle
                            implicitWidth: plateText.implicitWidth + 10
                            implicitHeight: 18

                            Text {
                                id: plateText
                                anchors.centerIn: parent
                                text: row.plateLabel
                                color: Theme.textSecondary
                                font.pixelSize: 9
                                font.bold: true
                            }
                        }

                        Rectangle {
                            visible: root.editorVm && root.editorVm.objectOrganizeMode === 0 && row.moduleLabel.length > 0
                            radius: 6
                            color: "#1f2937"
                            border.width: 1
                            border.color: Theme.borderSubtle
                            implicitWidth: moduleText.implicitWidth + 10
                            implicitHeight: 18

                            Text {
                                id: moduleText
                                anchors.centerIn: parent
                                text: row.moduleLabel
                                color: Theme.textDisabled
                                font.pixelSize: 9
                            }
                        }

                        Text {
                            visible: !row.objPrintable
                            text: qsTr("不参与打印")
                            color: Theme.textDisabled
                            font.pixelSize: 10
                        }
                    }
                }

                // 删除按钮
                Rectangle {
                    width: 22; height: 22; radius: 7
                    color: delMA.containsMouse ? "#7d2020" : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        color: delMA.containsMouse ? "#ffaaaa" : Theme.textDisabled
                        font.pixelSize: 10
                    }
                    MouseArea {
                        id: delMA
                        anchors.fill: parent
                        z: 2
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (!root.editorVm)
                                return
                            if (row.isSelected && root.editorVm.selectedObjectCount > 1)
                                root.editorVm.deleteSelectedObjects()
                            else
                                root.editorVm.deleteObject(row.index)
                        }
                    }
                }
            }

            Column {
                id: childColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: baseBlock.bottom
                anchors.topMargin: row.groupExpanded && row.objectExpanded && row.hasVolumeChildren ? 4 : 0
                leftPadding: 24
                rightPadding: 6
                spacing: 4
                visible: row.groupExpanded && row.objectExpanded && row.hasVolumeChildren

                Repeater {
                    model: childColumn.visible ? row.volumeCount : 0

                    delegate: Rectangle {
                        required property int index
                        readonly property bool isSelected: root.editorVm && root.editorVm.isVolumeSelected(row.index, index)
                        width: childColumn.width - childColumn.leftPadding - childColumn.rightPadding
                        height: 34
                        radius: 9
                        color: isSelected ? Theme.accentSubtle : Theme.bgElevated
                        border.width: 1
                        border.color: isSelected ? Theme.accent : Theme.borderSubtle

                        Menu {
                            id: volumeMenu

                            MenuItem {
                                text: qsTr("在参数表中编辑")
                                enabled: !!root.editorVm && root.editorVm.canOpenSelectionSettings
                                onTriggered: {
                                    if (root.editorVm)
                                        root.editorVm.requestSelectionSettings()
                                }
                            }

                            MenuSeparator { }

                            MenuItem {
                                text: root.editorVm && root.editorVm.selectedVolumeCount > 1
                                      ? qsTr("删除已选部件")
                                      : qsTr("删除部件")
                                enabled: !!root.editorVm
                                onTriggered: {
                                    if (root.editorVm)
                                        root.editorVm.deleteSelection()
                                }
                            }
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 10
                            spacing: 8

                            Rectangle {
                                width: 6
                                height: 6
                                radius: 3
                                color: isSelected ? Theme.accent : "#7b8794"
                            }

                            Text {
                                Layout.fillWidth: true
                                text: root.editorVm ? root.editorVm.objectVolumeName(row.index, index) : ""
                                color: isSelected ? Theme.textPrimary : Theme.textSecondary
                                font.pixelSize: 11
                                elide: Text.ElideRight
                            }

                            Rectangle {
                                visible: root.editorVm && root.editorVm.objectVolumeTypeLabel(row.index, index).length > 0
                                radius: 6
                                color: "#1d2735"
                                border.width: 1
                                border.color: Theme.borderSubtle
                                implicitWidth: volumeTypeText.implicitWidth + 10
                                implicitHeight: 18

                                Text {
                                    id: volumeTypeText
                                    anchors.centerIn: parent
                                    text: root.editorVm ? root.editorVm.objectVolumeTypeLabel(row.index, index) : ""
                                    color: Theme.textDisabled
                                    font.pixelSize: 9
                                }
                            }

                            Rectangle {
                                width: 20
                                height: 20
                                radius: 6
                                color: volumeDeleteMA.containsMouse ? "#7d2020" : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: "✕"
                                    color: volumeDeleteMA.containsMouse ? "#ffaaaa" : Theme.textDisabled
                                    font.pixelSize: 10
                                }

                                MouseArea {
                                    id: volumeDeleteMA
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (root.editorVm)
                                            root.editorVm.deleteSelection()
                                    }
                                }
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            cursorShape: Qt.PointingHandCursor
                            onClicked: function(mouse) {
                                if (!root.editorVm)
                                    return
                                if (mouse.button === Qt.LeftButton && (mouse.modifiers & Qt.ControlModifier))
                                    root.editorVm.toggleVolumeSelection(row.index, index)
                                else
                                    root.editorVm.selectVolume(row.index, index)
                                if (mouse.button === Qt.RightButton)
                                    volumeMenu.popup()
                            }
                        }
                    }
                }
            }

            // 选中点击层（最低 z，被按钮覆盖）
            MouseArea {
                id: selMA
                visible: row.groupExpanded
                anchors.left: baseBlock.left
                anchors.right: baseBlock.right
                anchors.top: baseBlock.top
                anchors.bottom: baseBlock.bottom
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                cursorShape: Qt.PointingHandCursor

                Drag.supportedActions: Qt.MoveAction
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2
                Drag.keys: ["object-drag", "object-reorder"]

                Drag.onDragStarted: {
                    if (!root.editorVm) return
                    if (!row.isSelected)
                        root.editorVm.selectObject(row.index)
                    listView.dragSourceIndex = row.index
                    Drag.mimeData = {
                        "object-index": row.index,
                        "object-name": root.editorVm.objectName(row.index),
                        "source-reorder": true
                    }
                }

                Drag.onDragFinished: {
                    listView.dragSourceIndex = -1
                    row.dragHover = false
                }

                // Drop target for list reorder (对齐上游 GUI_ObjectList OnDrop)
                DropArea {
                    anchors.fill: parent
                    keys: ["object-reorder"]
                    onEntered: function(drag) { row.dragHover = true }
                    onExited: function(drag) { row.dragHover = false }
                    onDropped: function(drop) {
                        row.dragHover = false
                        if (!root.editorVm) return
                        var fromIdx = drop.getDataAsString("object-index")
                        var fromNum = parseInt(fromIdx)
                        if (!isNaN(fromNum) && fromNum !== row.index && fromNum >= 0 && row.index >= 0) {
                            root.editorVm.moveObject(fromNum, row.index)
                        }
                    }
                }

                onDoubleClicked: function(mouse) {
                    // 对齐上游 GUI_ObjectList rename_item: double-click to inline rename
                    if (mouse.button === Qt.LeftButton && root.editorVm && root.editorVm.selectedObjectCount <= 1) {
                        root.editorVm.selectObject(row.index)
                        row.renaming = true
                        row.renameText = root.editorVm.objectName(row.index)
                    }
                }

                onClicked: function(mouse) {
                    if (!root.editorVm)
                        return
                    if (mouse.button === Qt.LeftButton && (mouse.modifiers & Qt.ControlModifier))
                        root.editorVm.toggleObjectSelection(row.index)
                    else if (mouse.button === Qt.RightButton) {
                        if (!row.isSelected)
                            root.editorVm.selectObject(row.index)
                    } else
                        root.editorVm.selectObject(row.index)

                    if (mouse.button === Qt.RightButton)
                        rowMenu.popup()
                }
            }
        }
    }

    // ── 空列表占位 ────────────────────────────────────────────────
    Text {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -20
        visible: !root.editorVm || root.editorVm.objectCount === 0
        text: qsTr("场景中无对象\n请从顶部菜单导入模型")
        color: Theme.textDisabled
        font.pixelSize: 12
        horizontalAlignment: Text.AlignHCenter
        lineHeight: 1.5
    }

    // ── 导入按钮 ─────────────────────────────────────────────────
    Rectangle {
        id: importBtn
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 8
        height: 34
        radius: 12
        color: importMA.containsMouse ? Theme.accentLight : Theme.accent
        visible: root.showImportButton

        Text {
            anchors.centerIn: parent
            text: qsTr("+ 导入模型")
            color: Theme.textOnAccent
            font.pixelSize: 12
            font.bold: true
        }

        MouseArea {
            id: importMA
            anchors.fill: parent
            hoverEnabled: true
            enabled: false
            cursorShape: Qt.ArrowCursor
        }
    }

    // ── 从文件导入 volume 对话框（对齐上游 GUI_ObjectList::load_generic_subobject 文件加载）──
    Component {
        id: volumeFileDialogComp
        FileDialog {
            required property int objectIndex
            required property var editorVm
            title: qsTr("选择模型文件导入为部件")
            nameFilters: ["模型文件 (*.stl *.obj *.3mf)", "STL 文件 (*.stl)", "OBJ 文件 (*.obj)", "所有文件 (*)"]
            onAccepted: {
                if (editorVm) {
                    editorVm.addVolumeFromFile(objectIndex, selectedFile.toString().replace("file:///", ""), 0)
                }
                destroy()
            }
            onRejected: destroy()
        }
    }

    // ── SVG 导入对话框（对齐上游 GLGizmoSVG）──
    Component {
        id: svgFileDialogComp
        FileDialog {
            required property int objectIndex
            required property var editorVm
            title: qsTr("选择 SVG 文件")
            nameFilters: ["SVG 文件 (*.svg)", "所有文件 (*)"]
            onAccepted: {
                if (editorVm) {
                    editorVm.addSvgVolume(objectIndex, selectedFile.toString().replace("file:///", ""))
                }
                destroy()
            }
            onRejected: destroy()
        }
    }

    // ── 文字浮雕输入对话框（对齐上游 GLGizmoText）──
    Component {
        id: addTextDialogComp
        Dialog {
            id: textDlg
            required property int objectIndex
            required property var editorVm
            title: qsTr("添加文字浮雕")
            modal: true
            anchors.centerIn: parent
            width: 320

            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                Text {
                    text: qsTr("输入浮雕文字")
                    color: Theme.textSecondary
                    font.pixelSize: 12
                }

                TextField {
                    id: textField
                    Layout.fillWidth: true
                    placeholderText: qsTr("请输入文字...")
                    font.pixelSize: 13
                    color: Theme.textPrimary
                    background: Rectangle {
                        radius: 6
                        color: Theme.bgElevated
                        border.color: textField.activeFocus ? Theme.accent : Theme.borderSubtle
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Item { Layout.fillWidth: true }

                    Rectangle {
                        height: 28
                        width: cancelBtnText.implicitWidth + 20
                        radius: 5
                        color: Theme.bgElevated
                        border.color: Theme.borderSubtle
                        Text { id: cancelBtnText; anchors.centerIn: parent; text: qsTr("取消"); color: Theme.textSecondary; font.pixelSize: 11 }
                        TapHandler { onTapped: textDlg.close() }
                    }

                    Rectangle {
                        height: 28
                        width: okBtnText.implicitWidth + 20
                        radius: 5
                        color: Theme.accent
                        Text { id: okBtnText; anchors.centerIn: parent; text: qsTr("确定"); color: Theme.textOnAccent; font.pixelSize: 11; font.bold: true }
                        TapHandler {
                            onTapped: {
                                if (editorVm && textField.text.length > 0) {
                                    editorVm.addTextVolume(textDlg.objectIndex, textField.text)
                                }
                                textDlg.close()
                            }
                        }
                    }
                }
            }

            onOpened: textField.forceActiveFocus()
            onClosed: destroy()
        }
    }
}
