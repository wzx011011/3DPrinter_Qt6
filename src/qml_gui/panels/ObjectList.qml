import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
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
                    text: qsTr("在参数表中编辑")
                    enabled: !!root.editorVm && row.isSelected && root.editorVm.canOpenSelectionSettings && !root.editorVm.hasSelectedVolume
                    onTriggered: {
                        if (root.editorVm)
                            root.editorVm.requestSelectionSettings()
                    }
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

                    Text {
                        Layout.fillWidth: true
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
}
