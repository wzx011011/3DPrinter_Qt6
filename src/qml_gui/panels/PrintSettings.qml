import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"
import "../components"

// 打印设置面板 — 三段式可折叠布局（对齐上游 Sidebar 结构）
// Section 1: 打印机（打印机预设 + 热床类型）
// Section 2: 耗材（耗材槽位 + 兼容性指示 + 自动匹配）
// Section 3: 打印设置（作用域选择 + 预设选择 + 选项列表）
Item {
    id: root
    required property var configVm
    required property var editorVm

    property bool advancedEnabled: false
    property string searchText: ""
    property string processCategory: ""
    property var filteredIndices: []
    readonly property var visibleCategories: [qsTr("质量"), qsTr("接缝"), qsTr("精度"), qsTr("墙壁和表面"), qsTr("填充"), qsTr("支撑"), qsTr("底座"), qsTr("速度"), qsTr("温度"), qsTr("其他")]
    readonly property var activeCategories: root.processCategory !== ""
        ? [root.processCategory]
        : root.visibleCategories
    readonly property string selectedScope: root.configVm ? root.configVm.settingsScope : "global"
    readonly property string settingsTargetType: root.configVm ? root.configVm.settingsTargetType : ""
    readonly property string settingsTargetName: root.configVm ? root.configVm.settingsTargetName : ""
    readonly property int settingsTargetObjectIndex: root.configVm ? root.configVm.settingsTargetObjectIndex : -1
    readonly property int settingsTargetVolumeIndex: root.configVm ? root.configVm.settingsTargetVolumeIndex : -1

    function scopeTitle() {
        switch (root.selectedScope) {
        case "volume": return qsTr("部件打印参数")
        case "object": return qsTr("对象打印参数")
        case "plate":  return qsTr("平板打印参数")
        default:       return qsTr("全局打印参数")
        }
    }

    function scopeSubtitle() {
        if (root.selectedScope === "global")
            return qsTr("当前预设作用于整个工程")
        if (root.settingsTargetName.length === 0)
            return qsTr("未选择目标")
        if (root.selectedScope === "volume")
            return qsTr("当前目标：部件 . %1").arg(root.settingsTargetName)
        if (root.selectedScope === "plate")
            return qsTr("当前目标：平板 . %1").arg(root.settingsTargetName)
        return qsTr("当前目标：对象 . %1").arg(root.settingsTargetName)
    }

    function rebuildFilter() {
        if (!root.configVm) { filteredIndices = []; return }
        var indices = root.configVm.filterOptionIndices("", root.searchText, root.advancedEnabled)
        // 排序以便拖拽排序（对齐上游 DragCanvas 交互）
        indices.sort(function(a, b) { return a - b; })
        filteredIndices = indices
    }

    function groupedIndices(category) {
        if (!root.configVm) return []
        return root.configVm.filterIndicesByCategory(root.filteredIndices, category)
    }

    function pageFilteredIndices(category, page) {
        if (!root.configVm) return []
        var catIndices = root.configVm.filterIndicesByCategory(root.filteredIndices, category)
        return root.configVm.filterIndicesByPage(catIndices, page)
    }

    Component.onCompleted: rebuildFilter()
    onSearchTextChanged: rebuildFilter()
    onAdvancedEnabledChanged: rebuildFilter()

    Connections {
        target: root.configVm ? root.configVm.printOptions : null
        function onDataVersionChanged() { root.rebuildFilter() }
    }

    // 搜索对话框（对齐上游 SearchDialog / OptionsSearcher）
    SearchDialog {
        id: searchDialog
        configVm: root.configVm
        parent: root
        onJumpToOption: (idx) => {
            // 跳转到指定选项：高亮显示并滚动到可见
            root.highlightedOptionIndex = idx
            highlightTimer.start()
        }
    }

    // 值来源层级弹窗（对齐上游 Tab 值来源可视化 + reset_to_level）
    Popup {
        id: valueChainPopup
        property string currentKey: ""
        property string chainJson: "{}"
        property string currentLevel: "default"
        property var chainData: ({})

        function show(key, json, level) {
            currentKey = key
            chainJson = json
            currentLevel = level
            try { chainData = JSON.parse(json) } catch(e) { chainData = {} }
            open()
        }

        function resetToLevel(level) {
            if (root.configVm)
                root.configVm.resetOptionToLevel(currentKey, level)
            close()
        }

        anchors.centerIn: parent ? parent : undefined
        width: 260
        height: 220
        modal: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        padding: 0

        background: Rectangle {
            radius: 8
            color: "#0f1520"
            border.width: 1
            border.color: Theme.borderSubtle

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                // 标题行
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("值来源") + " — " + valueChainPopup.currentKey
                        color: Theme.textPrimary
                        font.pixelSize: 12
                        font.bold: true
                        elide: Text.ElideRight
                    }
                    Rectangle {
                        width: 20; height: 20; radius: 4
                        color: closeMA.containsMouse ? "#2a2030" : "#1a1520"
                        Text { anchors.centerIn: parent; text: "\u2715"; color: "#808090"; font.pixelSize: 10 }
                        MouseArea { id: closeMA; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: valueChainPopup.close()
                        }
                    }
                }

                Rectangle { width: parent.width - 24; height: 1; color: Theme.borderSubtle }

                // 继承链层级列表
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    id: lvlContent

                    Column {
                        anchors.fill: parent
                        spacing: 0
                        // Level 0: Default
                        Rectangle {
                            width: parent.width; height: 28; color: "#0c1018"
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 8; spacing: 8
                                Rectangle { width: 8; height: 8; radius: 4; color: "#506070" }
                                Text { text: qsTr("默认值"); color: "#a0a8b0"; font.pixelSize: 11 }
                                Item { Layout.fillWidth: true }
                                Text { text: valueChainPopup.chainData.default || "-"; color: "#d0d8e0"; font.pixelSize: 11; font.family: "Consolas, monospace"
                                    Layout.alignment: Qt.AlignRight; Layout.rightMargin: 8
                                }
                                Rectangle {
                                    visible: valueChainPopup.currentLevel === "default"
                                    width: 20; height: 20; radius: 4; color: "#1a3a5c"
                                    Text { anchors.centerIn: parent; text: "\u2713"; color: "#58a6ff"; font.pixelSize: 11 }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: valueChainPopup.resetToLevel(0)
                                    }
                                }
                            }
                        }
                        // Level 1: Print
                        Rectangle {
                            width: parent.width; height: 28; color: "#0c1018"
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 8; spacing: 8
                                Rectangle { width: 8; height: 8; radius: 4; color: "#6ed4a0" }
                                Text { text: qsTr("打印预设"); color: "#a0c8a0"; font.pixelSize: 11 }
                                Item { Layout.fillWidth: true }
                                Text { text: valueChainPopup.chainData.print || "-"; color: "#d0d8e0"; font.pixelSize: 11; font.family: "Consolas, monospace"
                                    Layout.alignment: Qt.AlignRight; Layout.rightMargin: 8
                                }
                                Rectangle {
                                    visible: valueChainPopup.currentLevel === "print"
                                    width: 20; height: 20; radius: 4; color: "#1a3a2a"
                                    Text { anchors.centerIn: parent; text: "\u2713"; color: "#6ed4a0"; font.pixelSize: 11 }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: valueChainPopup.resetToLevel(1)
                                    }
                                }
                            }
                        }
                        // Level 2: Filament
                        Rectangle {
                            width: parent.width; height: 28; color: "#0c1018"
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 8; spacing: 8
                                Rectangle { width: 8; height: 8; radius: 4; color: "#d4a06e" }
                                Text { text: qsTr("耗材预设"); color: "#c8b0a0"; font.pixelSize: 11 }
                                Item { Layout.fillWidth: true }
                                Text { text: valueChainPopup.chainData.filament || "-"; color: "#d0d8e0"; font.pixelSize: 11; font.family: "Consolas, monospace"
                                    Layout.alignment: Qt.AlignRight; Layout.rightMargin: 8
                                }
                                Rectangle {
                                    visible: valueChainPopup.currentLevel === "filament"
                                    width: 20; height: 20; radius: 4; color: "#3a2a1a"
                                    Text { anchors.centerIn: parent; text: "\u2713"; color: "#d4a06e"; font.pixelSize: 11 }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: valueChainPopup.resetToLevel(2)
                                    }
                                }
                            }
                        }
                        // Level 3: Printer
                        Rectangle {
                            width: parent.width; height: 28; color: "#0c1018"
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 8; spacing: 8
                                Rectangle { width: 8; height: 8; radius: 4; color: "#6ea8d4" }
                                Text { text: qsTr("打印机预设"); color: "#a0b8c8"; font.pixelSize: 11 }
                                Item { Layout.fillWidth: true }
                                Text { text: valueChainPopup.chainData.printer || "-"; color: "#d0d8e0"; font.pixelSize: 11; font.family: "Consolas, monospace"
                                    Layout.alignment: Qt.AlignRight; Layout.rightMargin: 8
                                }
                                Rectangle {
                                    visible: valueChainPopup.currentLevel === "printer"
                                    width: 20; height: 20; radius: 4; color: "#1a2a3a"
                                    Text { anchors.centerIn: parent; text: "\u2713"; color: "#6ea8d4"; font.pixelSize: 11 }
                                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: valueChainPopup.resetToLevel(3)
                                    }
                                }
                            }
                        }
                        // Current value
                        Rectangle { width: parent.width - 24; height: 1; color: "#304050" }
                        Rectangle {
                            width: parent.width; height: 28; color: "#142030"
                            RowLayout {
                                anchors.fill: parent; anchors.leftMargin: 8; spacing: 8
                                Text { text: qsTr("当前值"); color: Theme.textPrimary; font.pixelSize: 11; font.bold: true }
                                Item { Layout.fillWidth: true }
                                Text { text: valueChainPopup.chainData.current || "-"; color: "#58a6ff"; font.pixelSize: 11; font.bold: true; font.family: "Consolas, monospace"
                                    Layout.alignment: Qt.AlignRight; Layout.rightMargin: 8
                                }
                            }
                        }
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("点击 \u2713 重置到该层级值")
                    color: "#506070"
                    font.pixelSize: 9
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

    // Ctrl+F 快捷键打开搜索对话框
    Shortcut {
        sequence: "Ctrl+F"
        onActivated: searchDialog.openWithQuery(root.searchText)
    }

    // 高亮跳转目标选项
    property int highlightedOptionIndex: -1
    Timer {
        id: highlightTimer
        interval: 3000
        onTriggered: root.highlightedOptionIndex = -1
    }

    // 重命名打印机预设对话框（放在根级避免作用域问题）
    Dialog {
        id: renamePrinterDialog
        title: qsTr("重命名预设")
        modal: true
        anchors.centerIn: parent
        width: 320
        ColumnLayout {
            spacing: 8
            TextField {
                id: renamePrinterField
                Layout.fillWidth: true
                text: root.configVm ? root.configVm.currentPrinterPreset : ""
                color: Theme.textPrimary
                font.pixelSize: 12
                selectByMouse: true
                onAccepted: renamePrinterDialog.accept()
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 8
                Button { text: qsTr("取消"); onClicked: renamePrinterDialog.reject() }
                Button { text: qsTr("确认"); highlighted: true; onClicked: renamePrinterDialog.accept() }
            }
        }
        onAccepted: {
            if (root.configVm && renamePrinterField.text.trim().length > 0)
                root.configVm.renamePreset(2, root.configVm.currentPrinterPreset, renamePrinterField.text.trim())
        }
        onOpened: renamePrinterField.selectAll()
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        // ═══════════════════════════════════════════════════
        // Modified Options summary bar（对齐上游 Tab::modified_options）
        // ═══════════════════════════════════════════════════
        Rectangle {
            id: modifiedBar
            Layout.fillWidth: true
            Layout.leftMargin: 0
            Layout.rightMargin: 0
            Layout.topMargin: 2
            visible: root.configVm ? root.configVm.globalModifiedCount > 0 : false
            implicitHeight: modifiedContentCol.implicitHeight + 16
            radius: 16
            color: Theme.bgPanel
            border.width: 1
            border.color: Theme.borderSubtle

            property bool expanded: false

            // Summary row (always visible when count > 0)
            Rectangle {
                id: modifiedSummaryRow
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.topMargin: 6
                height: 28
                color: "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 10
                    spacing: 8

                    // Warning dot
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: "#f59e0b"
                    }

                    Text {
                        text: qsTr("%1 项已修改").arg(root.configVm ? root.configVm.globalModifiedCount : 0)
                        color: Theme.textPrimary
                        font.pixelSize: 12
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    // Expand / Collapse toggle
                    Text {
                        text: modifiedBar.expanded ? qsTr("收起") : qsTr("展开")
                        color: Theme.accent
                        font.pixelSize: 11
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: modifiedBar.expanded = !modifiedBar.expanded
                        }
                    }

                    Text {
                        text: modifiedBar.expanded ? "\u25B2" : "\u25BC"
                        color: Theme.textDisabled
                        font.pixelSize: 8
                    }
                }
            }

            // Expanded content: flat list of modified options
            Column {
                id: modifiedContentCol
                anchors.top: modifiedSummaryRow.bottom
                anchors.topMargin: modifiedBar.expanded ? 6 : 0
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                anchors.bottomMargin: modifiedBar.expanded ? 10 : 0
                spacing: 0
                clip: true
                height: modifiedBar.expanded ? implicitHeight : 0
                visible: !!modifiedBar.expanded || !!expandAnim.running

                Behavior on height {
                    id: expandAnim
                    NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
                }
                Behavior on opacity {
                    NumberAnimation { duration: 150 }
                }
                opacity: modifiedBar.expanded ? 1.0 : 0.0

                // Separator line
                Rectangle {
                    width: parent.parent.width - 20
                    height: 1
                    color: Theme.borderSubtle
                }

                Repeater {
                    model: root.configVm ? root.configVm.globalModifiedCount : 0

                    Rectangle {
                        width: modifiedContentCol.width
                        height: 30
                        color: index % 2 === 0 ? "transparent" : "#0e1520"

                        readonly property string optKey: root.configVm ? root.configVm.globalModifiedKey(index) : ""
                        readonly property string curVal: root.configVm ? root.configVm.globalModifiedCurrentValue(optKey) : ""
                        readonly property string defVal: root.configVm ? root.configVm.globalModifiedDefaultValue(optKey) : ""

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 6

                            Text {
                                text: parent.parent.optKey
                                color: "#e8864a"
                                font.pixelSize: 11
                                font.family: "Consolas, monospace"
                                Layout.preferredWidth: 160
                                elide: Text.ElideRight
                            }

                            // Arrow indicator
                            Text {
                                text: "\u2192"
                                color: Theme.textDisabled
                                font.pixelSize: 10
                            }

                            Text {
                                text: parent.parent.defVal
                                color: Theme.textDisabled
                                font.pixelSize: 11
                                font.family: "Consolas, monospace"
                                Layout.preferredWidth: 60
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignRight
                            }

                            Text {
                                text: "\u2192"
                                color: Theme.textDisabled
                                font.pixelSize: 10
                            }

                            Text {
                                text: parent.parent.curVal
                                color: Theme.textPrimary
                                font.pixelSize: 11
                                font.family: "Consolas, monospace"
                                font.bold: true
                                Layout.preferredWidth: 60
                                elide: Text.ElideRight
                                horizontalAlignment: Text.AlignRight
                            }

                            Item { Layout.fillWidth: true }

                            CxButton {
                                id: resetOptBtn
                                text: qsTr("重置")
                                compact: true
                                cxStyle: CxButton.Style.Ghost
                                onClicked: {
                                    if (root.configVm)
                                        root.configVm.resetGlobalOption(resetOptBtn.parent.parent.optKey)
                                }
                            }
                        }
                    }
                }

                // Reset All button
                Rectangle {
                    width: modifiedContentCol.width
                    height: 34
                    color: "transparent"

                    RowLayout {
                        anchors.fill: parent
                        spacing: 8

                        Item { Layout.fillWidth: true }

                        CxButton {
                            text: qsTr("全部重置")
                            compact: true
                            cxStyle: CxButton.Style.Secondary
                            onClicked: {
                                if (root.configVm)
                                    root.configVm.resetAllGlobalOptions()
                            }
                        }
                    }
                }
            }
        }

        // ═══════════════════════════════════════════════════
        // Section 1: 打印机（对齐上游 Sidebar Printer Section）
        // ═══════════════════════════════════════════════════
        CollapsibleSection {
            id: printerSection
            title: qsTr("打印机")
            iconText: "P"
            Layout.fillWidth: true

            content: ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 6

                // 打印机预设选择行
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: qsTr("预设")
                        color: Theme.textSecondary
                        font.pixelSize: 10
                        font.bold: true
                        Layout.preferredWidth: 30
                    }
                    Rectangle {
                        width: 6; height: 6; radius: 3
                        visible: !!root.configVm && root.configVm.isPresetDirty
                        color: "#f59e0b"
                        Layout.alignment: Qt.AlignVCenter
                    }
                    CxComboBox {
                        id: printerPresetCombo
                        Layout.fillWidth: true
                        model: root.configVm ? root.configVm.printerPresetNames : []
                        currentIndex: {
                            if (!root.configVm) return -1
                            return root.configVm.printerPresetNames.indexOf(root.configVm.currentPrinterPreset)
                        }
                        onActivated: (i) => {
                            if (root.configVm && i >= 0)
                                root.configVm.setCurrentPrinterPreset(model[i])
                        }
                    }
                    Rectangle {
                        width: 24; height: 24; radius: 4
                        color: savePrinterBtn.containsMouse ? "#1c2a3e" : "transparent"
                        Text { anchors.centerIn: parent; text: "+"; color: "#8a96a8"; font.pixelSize: 14; font.bold: true }
                        MouseArea { id: savePrinterBtn; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.configVm) root.configVm.createCustomPreset(2, root.configVm.currentPrinterPreset + " (Custom)")
                        }
                    }
                    Rectangle {
                        width: 24; height: 24; radius: 4
                        visible: root.configVm ? root.configVm.canDeletePreset(root.configVm.currentPrinterPreset) : false
                        color: renamePrinterBtn2.containsMouse ? "#1c2a3e" : "transparent"
                        Text { anchors.centerIn: parent; text: "\u270E"; color: "#8a96a8"; font.pixelSize: 12 }
                        MouseArea { id: renamePrinterBtn2; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: renamePrinterDialog.open()
                        }
                    }
                    Rectangle {
                        width: 24; height: 24; radius: 4
                        visible: root.configVm ? root.configVm.canDeletePreset(root.configVm.currentPrinterPreset) : false
                        color: delPrinterBtn.containsMouse ? "#2e1a1a" : "transparent"
                        Text { anchors.centerIn: parent; text: "\u2715"; color: "#e06666"; font.pixelSize: 11 }
                        MouseArea { id: delPrinterBtn; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.configVm) root.configVm.deletePreset(2, root.configVm.currentPrinterPreset)
                        }
                    }
                }

                // 热床类型选择行（对齐上游 Sidebar Printer Section bed_type combo）
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        text: qsTr("热床")
                        color: Theme.textSecondary
                        font.pixelSize: 10
                        font.bold: true
                        Layout.preferredWidth: 30
                    }
                    CxComboBox {
                        Layout.fillWidth: true
                        model: [qsTr("PEI"), qsTr("EP"), qsTr("PC"), qsTr("Texture PEI"), qsTr("Custom")]
                        currentIndex: root.editorVm ? root.editorVm.plateBedType(root.editorVm.currentPlateIndex) : 0
                        onActivated: (i) => {
                            if (root.editorVm)
                                root.editorVm.setPlateBedType(root.editorVm.currentPlateIndex, i)
                        }
                    }
                }
            }
        }

        // ═══════════════════════════════════════════════════
        // Section 2: 耗材（对齐上游 Sidebar Material/Filament Section）
        // ═══════════════════════════════════════════════════
        CollapsibleSection {
            id: filamentSection
            title: qsTr("耗材")
            iconText: "F"
            Layout.fillWidth: true

            rightActions: [
                // 兼容性警告指示器
                Rectangle {
                    width: 8; height: 8; radius: 4
                    visible: !!root.configVm && !root.configVm.isCurrentFilamentCompatible()
                    color: "#f05545"
                    ToolTip.text: qsTr("Current filament may be incompatible with printer")
                    ToolTip.visible: compMA2.containsMouse
                    ToolTip.delay: 500
                    MouseArea { id: compMA2; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                },
                // 自动匹配按钮（对齐上游 Sidebar m_auto_mapping_btn）
                Rectangle {
                    width: 22; height: 22; radius: 4
                    color: autoMatchBtn.containsMouse ? "#1c2a3e" : "transparent"
                    Text { anchors.centerIn: parent; text: "\u21BB"; color: "#8a96a8"; font.pixelSize: 13; font.bold: true }
                    MouseArea { id: autoMatchBtn; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                        onClicked: if (root.configVm) root.configVm.autoMatchFilament()
                    }
                }
            ]

            content: ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 6

                // 耗材槽位网格（对齐上游 FilamentPanel FilamentItem 列表）
                GridLayout {
                    columns: 3
                    rowSpacing: 8
                    columnSpacing: 8
                    Layout.fillWidth: true
                    Layout.preferredHeight: childrenRect.height

                    Repeater {
                        model: 5
                        delegate: Rectangle {
                            required property int index
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            radius: 8
                            color: index === 0 ? "#b97914" : index === 3 ? "#214bc2" : index === 4 ? "#d63a21" : "#b9b9b9"
                            border.width: 1
                            border.color: index === 0 || index === 3 || index === 4 ? "transparent" : "#a8a8a8"

                            Column {
                                anchors.centerIn: parent
                                spacing: 1

                                Text {
                                    text: (index + 1).toString()
                                    color: "#101114"
                                    font.pixelSize: 12
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    width: 70
                                }
                                Text {
                                    text: root.configVm ? root.configVm.materialPresetName(index) : ""
                                    color: "#101114"
                                    font.pixelSize: 11
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    width: 70
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }
        }

        // ═══════════════════════════════════════════════════
        // Section 3: 打印设置（对齐上游 Sidebar ParamsPanel Section）
        // ═══════════════════════════════════════════════════
        CollapsibleSection {
            id: settingsSection
            title: qsTr("打印设置")
            iconText: "S"
            Layout.fillWidth: true
            Layout.fillHeight: true
            expanded: true

            content: ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: 6

                // 作用域选择器（对齐上游 ParamsPanel scope tabs）
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: root.scopeTitle()
                            color: Theme.textPrimary
                            font.pixelSize: 12
                            font.bold: true
                        }

                        Rectangle {
                            width: 38; height: 22; radius: 11
                            color: root.selectedScope === "global" ? Theme.accent : "transparent"
                            border.width: 1
                            border.color: root.selectedScope === "global" ? "transparent" : Theme.borderDefault

                            Text {
                                anchors.centerIn: parent
                                text: qsTr("Global")
                                color: root.selectedScope === "global" ? Theme.textOnAccent : Theme.textSecondary
                                font.pixelSize: 10
                                font.bold: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { if (root.configVm) root.configVm.activateGlobalScope() }
                            }
                        }

                        Rectangle {
                            width: 38; height: 22; radius: 11
                            enabled: root.settingsTargetName.length > 0
                            property bool isActive: root.selectedScope === "object" || root.selectedScope === "volume"
                            color: isActive ? Theme.bgElevated : "transparent"
                            border.width: 1
                            border.color: isActive ? Theme.borderDefault : "transparent"
                            opacity: enabled ? 1.0 : 0.45

                            Text {
                                anchors.centerIn: parent
                                text: root.selectedScope === "volume" ? qsTr("Part") : qsTr("Object")
                                color: parent.isActive ? Theme.textPrimary : Theme.textSecondary
                                font.pixelSize: 10
                                font.bold: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: parent.enabled
                                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: {
                                    if (root.configVm && root.settingsTargetName.length > 0)
                                        root.configVm.activateObjectScope(root.settingsTargetType,
                                                                          root.settingsTargetName,
                                                                          root.settingsTargetObjectIndex,
                                                                          root.settingsTargetVolumeIndex)
                                }
                            }
                        }

                        Rectangle {
                            width: 38; height: 22; radius: 11
                            visible: root.selectedScope === "plate" || root.editorVm
                            color: root.selectedScope === "plate" ? Theme.bgElevated : "transparent"
                            border.width: 1
                            border.color: root.selectedScope === "plate" ? Theme.borderDefault : "transparent"
                            opacity: root.selectedScope === "plate" ? 1.0 : 0.45

                            Text {
                                anchors.centerIn: parent
                                text: qsTr("Plate")
                                color: root.selectedScope === "plate" ? Theme.textPrimary : Theme.textSecondary
                                font.pixelSize: 10
                                font.bold: true
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (root.configVm && root.editorVm)
                                        root.configVm.activatePlateScope(root.editorVm.currentPlateIndex)
                                }
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            text: qsTr("Advanced")
                            color: Theme.textPrimary
                            font.pixelSize: 11
                            font.bold: true
                        }
                        Switch {
                            checked: root.advancedEnabled
                            onToggled: root.advancedEnabled = checked
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.scopeSubtitle()
                        color: Theme.textDisabled
                        font.pixelSize: 10
                        elide: Text.ElideRight
                    }
                }

                // 打印/耗材预设选择行（对齐上游 PresetBundle 2-tier in ParamsPanel）
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    // 打印预设行
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: qsTr("Print")
                            color: Theme.textSecondary
                            font.pixelSize: 10
                            font.bold: true
                            Layout.preferredWidth: 30
                        }
                        Rectangle {
                            width: 6; height: 6; radius: 3
                            visible: !!root.configVm && root.configVm.isPresetDirty
                            color: "#f59e0b"
                            Layout.alignment: Qt.AlignVCenter
                        }
                        CxComboBox {
                            id: printPresetCombo
                            Layout.fillWidth: true
                            model: root.configVm ? root.configVm.printPresetNames : []
                            currentIndex: {
                                if (!root.configVm) return -1
                                return root.configVm.printPresetNames.indexOf(root.configVm.currentPrintPreset)
                            }
                            onActivated: (i) => {
                                if (root.configVm && i >= 0)
                                    root.configVm.setCurrentPrintPreset(model[i])
                            }
                        }
                        Rectangle {
                            width: 24; height: 24; radius: 4
                            color: savePrintBtn.containsMouse ? "#1c2a3e" : "transparent"
                            Text { anchors.centerIn: parent; text: "+"; color: "#8a96a8"; font.pixelSize: 14; font.bold: true }
                            MouseArea { id: savePrintBtn; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: if (root.configVm) root.configVm.createCustomPreset(0, root.configVm.currentPrintPreset + " (Custom)")
                            }
                        }
                        Rectangle {
                            width: 24; height: 24; radius: 4
                            visible: root.configVm ? root.configVm.canDeletePreset(root.configVm.currentPrintPreset) : false
                            color: renamePrintBtn.containsMouse ? "#1c2a3e" : "transparent"
                            Text { anchors.centerIn: parent; text: "\u270E"; color: "#8a96a8"; font.pixelSize: 12 }
                            MouseArea { id: renamePrintBtn; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: renamePrintDialog.open()
                            }
                        }
                        Rectangle {
                            width: 24; height: 24; radius: 4
                            visible: root.configVm ? root.configVm.canDeletePreset(root.configVm.currentPrintPreset) : false
                            color: delPrintBtn.containsMouse ? "#2e1a1a" : "transparent"
                            Text { anchors.centerIn: parent; text: "\u2715"; color: "#e06666"; font.pixelSize: 11 }
                            MouseArea { id: delPrintBtn; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: if (root.configVm) root.configVm.deletePreset(0, root.configVm.currentPrintPreset)
                            }
                        }
                        // 重命名打印预设对话框
                        Dialog {
                            id: renamePrintDialog
                            title: qsTr("Rename Preset")
                            modal: true
                            anchors.centerIn: parent
                            width: 320
                            ColumnLayout {
                                spacing: 8
                                TextField {
                                    id: renamePrintField
                                    Layout.fillWidth: true
                                    text: root.configVm ? root.configVm.currentPrintPreset : ""
                                    color: Theme.textPrimary
                                    font.pixelSize: 12
                                    selectByMouse: true
                                    onAccepted: renamePrintDialog.accept()
                                }
                                RowLayout {
                                    Layout.alignment: Qt.AlignRight
                                    spacing: 8
                                    Button { text: qsTr("Cancel"); onClicked: renamePrintDialog.reject() }
                                    Button { text: qsTr("OK"); highlighted: true; onClicked: renamePrintDialog.accept() }
                                }
                            }
                            onAccepted: {
                                if (root.configVm && renamePrintField.text.trim().length > 0)
                                    root.configVm.renamePreset(0, root.configVm.currentPrintPreset, renamePrintField.text.trim())
                            }
                            onOpened: renamePrintField.selectAll()
                        }
                    }

                    // 耗材预设行
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Text {
                            text: qsTr("Filament")
                            color: Theme.textSecondary
                            font.pixelSize: 10
                            font.bold: true
                            Layout.preferredWidth: 30
                        }
                        Rectangle {
                            width: 8; height: 8; radius: 4
                            visible: !!root.configVm && !root.configVm.isCurrentFilamentCompatible()
                            color: "#f05545"
                            Layout.alignment: Qt.AlignVCenter
                            ToolTip.text: qsTr("Current filament may be incompatible with printer")
                            ToolTip.visible: compMA3.containsMouse
                            ToolTip.delay: 500
                        }
                        MouseArea {
                            id: compMA3
                            visible: !!root.configVm && !root.configVm.isCurrentFilamentCompatible()
                            Layout.fillWidth: true
                            Layout.preferredHeight: parent.height
                            hoverEnabled: true
                            acceptedButtons: Qt.NoButton
                        }
                        Rectangle {
                            width: 6; height: 6; radius: 3
                            visible: !!root.configVm && root.configVm.isPresetDirty
                            color: "#f59e0b"
                            Layout.alignment: Qt.AlignVCenter
                        }
                        ComboBox {
                            id: filamentPresetCombo
                            Layout.fillWidth: true
                            implicitHeight: 28
                            font.pixelSize: 12
                            model: root.configVm ? root.configVm.filamentPresetNames : []
                            currentIndex: {
                                if (!root.configVm) return -1
                                return root.configVm.filamentPresetNames.indexOf(root.configVm.currentFilamentPreset)
                            }
                            onActivated: (i) => {
                                if (root.configVm && i >= 0)
                                    root.configVm.setCurrentFilamentPreset(model[i])
                            }
                            // 对齐上游 Tab: 不兼容耗材灰化显示
                            delegate: ItemDelegate {
                                required property int index
                                required property string modelData
                                width: filamentPresetCombo.width
                                height: 26
                                highlighted: filamentPresetCombo.highlightedIndex === index
                                readonly property bool compat: root.configVm ? root.configVm.isFilamentCompatible(modelData) : true
                                background: Rectangle {
                                    color: highlighted ? "#1c6e42" : "transparent"
                                }
                                contentItem: RowLayout {
                                    spacing: 6
                                    Text {
                                        Layout.fillWidth: true
                                        leftPadding: 10
                                        text: modelData
                                        color: compat ? "#d8e0ec" : "#5a6070"
                                        font.pixelSize: 12
                                        font.strikeout: !compat
                                        elide: Text.ElideRight
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                    Text {
                                        visible: !compat
                                        text: "!"
                                        color: "#f05545"
                                        font.pixelSize: 10
                                        font.bold: true
                                    }
                                }
                            }
                            contentItem: Text {
                                leftPadding: 8
                                rightPadding: filamentPresetCombo.indicator.width + 4
                                text: filamentPresetCombo.displayText
                                color: "#d8e0ec"
                                font: filamentPresetCombo.font
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }
                            indicator: Text {
                                x: filamentPresetCombo.width - width - 8
                                y: (filamentPresetCombo.height - height) / 2
                                text: "\u25BE"
                                color: "#8a96a8"
                                font.pixelSize: 10
                            }
                            background: Rectangle {
                                radius: 4
                                color: filamentPresetCombo.pressed ? "#3a4050" : filamentPresetCombo.hovered ? "#353c4a" : "#2d3340"
                                border.color: filamentPresetCombo.activeFocus ? "#18c75e" : "#454d5e"
                                border.width: 1
                            }
                            popup: Popup {
                                y: filamentPresetCombo.height + 2
                                width: filamentPresetCombo.width
                                implicitHeight: contentItem.implicitHeight
                                padding: 0
                                background: Rectangle {
                                    color: "#252a34"
                                    border.color: "#454d5e"
                                    border.width: 1
                                    radius: 4
                                }
                                contentItem: ListView {
                                    clip: true
                                    implicitHeight: Math.min(contentHeight, 240)
                                    model: filamentPresetCombo.visible ? filamentPresetCombo.delegateModel : null
                                    ScrollIndicator.vertical: ScrollIndicator {}
                                }
                            }
                        }
                        Rectangle {
                            width: 24; height: 24; radius: 4
                            color: saveFilaBtn.containsMouse ? "#1c2a3e" : "transparent"
                            Text { anchors.centerIn: parent; text: "+"; color: "#8a96a8"; font.pixelSize: 14; font.bold: true }
                            MouseArea { id: saveFilaBtn; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: if (root.configVm) root.configVm.createCustomPreset(1, root.configVm.currentFilamentPreset + " (Custom)")
                            }
                        }
                        Rectangle {
                            width: 24; height: 24; radius: 4
                            visible: root.configVm ? root.configVm.canDeletePreset(root.configVm.currentFilamentPreset) : false
                            color: renameFilaBtn.containsMouse ? "#1c2a3e" : "transparent"
                            Text { anchors.centerIn: parent; text: "\u270E"; color: "#8a96a8"; font.pixelSize: 12 }
                            MouseArea { id: renameFilaBtn; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: renameFilaDialog.open()
                            }
                        }
                        Rectangle {
                            width: 24; height: 24; radius: 4
                            visible: root.configVm ? root.configVm.canDeletePreset(root.configVm.currentFilamentPreset) : false
                            color: delFilaBtn.containsMouse ? "#2e1a1a" : "transparent"
                            Text { anchors.centerIn: parent; text: "\u2715"; color: "#e06666"; font.pixelSize: 11 }
                            MouseArea { id: delFilaBtn; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                onClicked: if (root.configVm) root.configVm.deletePreset(1, root.configVm.currentFilamentPreset)
                            }
                        }
                        // 重命名耗材预设对话框
                        Dialog {
                            id: renameFilaDialog
                            title: qsTr("Rename Preset")
                            modal: true
                            anchors.centerIn: parent
                            width: 320
                            ColumnLayout {
                                spacing: 8
                                TextField {
                                    id: renameFilaField
                                    Layout.fillWidth: true
                                    text: root.configVm ? root.configVm.currentFilamentPreset : ""
                                    color: Theme.textPrimary
                                    font.pixelSize: 12
                                    selectByMouse: true
                                    onAccepted: renameFilaDialog.accept()
                                }
                                RowLayout {
                                    Layout.alignment: Qt.AlignRight
                                    spacing: 8
                                    Button { text: qsTr("Cancel"); onClicked: renameFilaDialog.reject() }
                                    Button { text: qsTr("OK"); highlighted: true; onClicked: renameFilaDialog.accept() }
                                }
                            }
                            onAccepted: {
                                if (root.configVm && renameFilaField.text.trim().length > 0)
                                    root.configVm.renamePreset(1, root.configVm.currentFilamentPreset, renameFilaField.text.trim())
                            }
                            onOpened: renameFilaField.selectAll()
                        }
                    }
                }

                // 分类选项列表（对齐上游 ParamsPanel m_page_view）
                // 双层级: Page > Category > Options（对齐上游 Tab::Page > Group > Option）
                ScrollView {
                    id: optionsScrollView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    // 滚动到高亮选项（对齐上游 Tab::activate_option 滚动行为）
                    function scrollToHighlight(itemY, itemHeight) {
                        if (!itemY || !contentItem) return
                        const visibleHeight = height
                        const targetY = itemY - visibleHeight / 2 + itemHeight / 2
                        const maxScroll = contentItem.contentHeight - visibleHeight
                        const newContentY = Math.max(0, Math.min(maxScroll, targetY))
                        scrollAnimation.from = contentItem.contentY
                        scrollAnimation.to = newContentY
                        scrollAnimation.start()
                    }

                    NumberAnimation {
                        id: scrollAnimation
                        target: optionsScrollView.contentItem
                        property: "contentY"
                        duration: 300
                        easing.type: Easing.OutCubic
                    }

                    Column {
                        id: optionsColumn
                        width: parent.width
                        spacing: 6

                        // Page-level Repeater (对齐上游 Tab::Page)
                        Repeater {
                            id: pageRepeater
                            model: root.configVm && root.configVm.printOptions
                                ? root.configVm.printOptions.pageNames() : []

                            delegate: Column {
                                width: parent.width
                                spacing: 6

                                // Page header (对齐上游 Page title bar)
                                Rectangle {
                                    width: parent.width
                                    height: 28
                                    radius: 6
                                    color: "#1a2030"
                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 10
                                        anchors.rightMargin: 8
                                        Text {
                                            text: modelData
                                            color: Theme.accent
                                            font.pixelSize: 11
                                            font.bold: true
                                            Layout.fillWidth: true
                                            elide: Text.ElideRight
                                        }
                                    }
                                }

                                // Category-level Repeater (对齐上游 ConfigOptionsGroup)
                                Repeater {
                                    model: root.activeCategories

                                    delegate: Column {
                                        width: parent.width
                                        spacing: 0

                                        readonly property var sectionIndices: root.pageFilteredIndices(modelData, pageRepeater.modelData)
                                        visible: sectionIndices.length > 0

                                        // Group header
                                        Rectangle {
                                            width: parent.width
                                            height: 28
                                            radius: 4
                                            color: "#161c28"
                                            border.width: 1
                                            border.color: Theme.borderSubtle

                                            RowLayout {
                                                anchors.fill: parent
                                                anchors.leftMargin: 10
                                                anchors.rightMargin: 8

                                                Text {
                                                    text: modelData
                                                    color: Theme.textPrimary
                                                    font.pixelSize: 11
                                                    font.bold: true
                                                }
                                                Item { Layout.fillWidth: true }
                                            }
                                        }

                                        // Option items
                                        Repeater {
                                            model: parent.parent.sectionIndices

                                            delegate: Rectangle {
                                                required property int modelData
                                                readonly property int optIdx: modelData
                                                readonly property var opts: root.configVm ? root.configVm.printOptions : null
                                                readonly property string oType: opts ? opts.optType(optIdx) : ""
                                                readonly property string oLabel: opts ? opts.optLabel(optIdx) : ""
                                                readonly property string oKey: opts ? opts.optKey(optIdx) : ""
                                                readonly property var oVal: opts ? opts.optValue(optIdx) : 0
                                                readonly property double oMin: opts ? opts.optMin(optIdx) : 0
                                                readonly property double oMax: opts ? opts.optMax(optIdx) : 1
                                                readonly property double oStep: opts ? opts.optStep(optIdx) : 1
                                                readonly property bool oRO: opts ? opts.optReadonly(optIdx) : false
                                                readonly property string oUnit: opts ? opts.optUnit(optIdx) : ""
                                                // 值来源层级指示（对齐上游 Tab 预设继承链）
                                                readonly property string valueSrc: root.configVm ? root.configVm.valueSourceForKey(oKey) : "default"
                                                readonly property string valueChain: root.configVm ? root.configVm.valueChainForKey(oKey) : "{}"
                                                readonly property bool isHighlighted: root.highlightedOptionIndex === optIdx
                                                // 修改状态指示（对齐上游 Tab::Field m_is_modified_value / OptStatus）
                                                readonly property bool isModified: opts ? opts.optIsDirty(optIdx) : false

                                                // 滚动到高亮选项（对齐上游 Tab::activate_option ensure_visible）
                                                onIsHighlightedChanged: {
                                                    if (isHighlighted && optionsScrollView.contentItem) {
                                                        // 延迟执行以确保布局完成
                                                        Qt.callLater(() => {
                                                            const pos = mapToItem(optionsScrollView.contentItem, 0, 0)
                                                            optionsScrollView.scrollToHighlight(pos.y, height)
                                                        })
                                                    }
                                                }

                                                width: parent.width
                                                height: (oType === "double" || oType === "int") ? 44 : 38
                                                color: isHighlighted ? "#1c6e42" : "transparent"

                                                Behavior on color { ColorAnimation { duration: 300 } }

                                                RowLayout {
                                                    anchors.fill: parent
                                                    anchors.leftMargin: 10
                                                    anchors.rightMargin: isModified ? 8 : 10
                                                    spacing: 6

                                                    // 值来源色点指示器（对齐上游 Tab value source）— 点击弹出继承链弹窗
                                                    Rectangle {
                                                        width: 5; height: 5; radius: 2.5
                                                        visible: valueSrc !== "default"
                                                        color: {
                                                            switch (valueSrc) {
                                                            case "printer": return "#6ea8d4"
                                                            case "filament": return "#d4a06e"
                                                            case "print": return "#6ed4a0"
                                                            default: return "transparent"
                                                            }
                                                        }
                                                        Layout.alignment: Qt.AlignVCenter
                                                        MouseArea {
                                                            anchors.fill: parent
                                                            anchors.margins: -4
                                                            hoverEnabled: true
                                                            cursorShape: Qt.PointingHandCursor
                                                            onClicked: valueChainPopup.show(oKey, valueChain, valueSrc)
                                                        }
                                                    }

                                                    Text {
                                                        Layout.fillWidth: true
                                                        text: oLabel
                                                        color: isHighlighted ? "#fff" : (isModified ? "#e8864a" : (oRO ? Theme.textDisabled : Theme.textSecondary))
                                                        font.pixelSize: 12
                                                        elide: Text.ElideRight
                                                        font.bold: isHighlighted || isModified
                                                    }

                                                    // 还原到默认值按钮（对齐上游 Tab::Field m_bmp_value_revert）
                                                    Rectangle {
                                                        visible: isModified && !oRO
                                                        width: 16; height: 16; radius: 3
                                                        color: revertMA.containsMouse ? "#2e3a1a" : "transparent"
                                                        border.width: 1
                                                        border.color: "#5a3a2a"
                                                        Layout.alignment: Qt.AlignVCenter
                                                        Text {
                                                            anchors.centerIn: parent
                                                            text: "↺"
                                                            color: "#e8864a"
                                                            font.pixelSize: 10
                                                            font.bold: true
                                                        }
                                                        MouseArea {
                                                            id: revertMA
                                                            anchors.fill: parent
                                                            anchors.margins: -3
                                                            hoverEnabled: true
                                                            cursorShape: Qt.PointingHandCursor
                                                            onClicked: {
                                                                if (opts) opts.resetOption(optIdx)
                                                            }
                                                        }
                                                    }

                                                    CxSlider {
                                                        visible: oType === "double" || oType === "int"
                                                        Layout.preferredWidth: 100
                                                        from: oMin
                                                        to: oMax
                                                        stepSize: oStep
                                                        value: typeof oVal === "number" ? oVal : oMin
                                                        enabled: !oRO
                                                        onMoved: {
                                                            if (opts)
                                                                opts.setValue(optIdx, value)
                                                        }
                                                    }

                                                    Rectangle {
                                                        visible: oType === "double" || oType === "int"
                                                        width: 58; height: 28; radius: 8
                                                        color: Theme.bgElevated
                                                        border.width: 1
                                                        border.color: Theme.borderSubtle

                                                        Text {
                                                            anchors.centerIn: parent
                                                            text: typeof oVal === "number" ? Number(oVal).toFixed(oType === "double" ? 1 : 0) : "-"
                                                            color: Theme.textPrimary
                                                            font.pixelSize: 11
                                                            font.bold: true
                                                        }
                                                    }

                                                    Text {
                                                        visible: oType === "double" || oType === "int"
                                                        text: oUnit
                                                        color: Theme.textDisabled
                                                        font.pixelSize: 10
                                                    }

                                                    CxCheckBox {
                                                        visible: oType === "bool"
                                                        checked: oVal === true || oVal === "true"
                                                        enabled: !oRO
                                                        text: ""
                                                        onToggled: {
                                                            if (opts)
                                                                opts.setValue(optIdx, checked)
                                                        }
                                                    }

                                                    CxComboBox {
                                                        visible: oType === "enum"
                                                        Layout.preferredWidth: 110
                                                        currentIndex: typeof oVal === "number" ? oVal : 0
                                                        model: opts ? opts.optEnumLabelsList(optIdx) : []
                                                        onActivated: {
                                                            if (opts)
                                                                opts.setValue(optIdx, currentIndex)
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Layer range section (对齐上游 ModelObject::layer_config_ranges)
                // 仅在 object/volume 作用域下显示
                Column {
                    Layout.fillWidth: true
                    spacing: 4
                    visible: root.configVm && root.configVm.layerRangeCount() > 0
                        && (root.selectedScope === "object" || root.selectedScope === "volume")

                    // Section title
                    Rectangle {
                        width: parent.width
                        height: 24
                        radius: 4
                        color: "#1a2535"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 8
                            spacing: 6

                            Text {
                                text: qsTr("Layer Ranges")
                                color: Theme.accent
                                font.pixelSize: 10
                                font.bold: true
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: qsTr("%1 ranges").arg(root.configVm ? root.configVm.layerRangeCount() : 0)
                                color: Theme.textDisabled
                                font.pixelSize: 9
                            }
                        }
                    }

                    // Layer range list
                    Repeater {
                        model: root.configVm ? root.configVm.layerRangeCount() : 0

                        delegate: Rectangle {
                            property int delegateIndex: index
                            width: parent.width
                            height: 32
                            radius: 4
                            color: "#141a24"
                            border.width: 1
                            border.color: Theme.borderSubtle

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 8
                                anchors.rightMargin: 8
                                spacing: 6

                                Text {
                                    text: "#" + (delegateIndex + 1)
                                    color: Theme.textDisabled
                                    font.pixelSize: 10
                                    font.bold: true
                                    Layout.preferredWidth: 20
                                }

                                Text {
                                    text: qsTr("Z %1 – %2 mm").arg(
                                        root.configVm ? root.configVm.layerRangeMinZ(delegateIndex).toFixed(1) : "0.0",
                                        root.configVm ? root.configVm.layerRangeMaxZ(delegateIndex).toFixed(1) : "0.0"
                                    )
                                    color: Theme.textPrimary
                                    font.pixelSize: 10
                                    font.family: "Consolas, monospace"
                                    Layout.fillWidth: true
                                }

                                Rectangle {
                                    width: 20; height: 20; radius: 3
                                    color: delRangeBtn.containsMouse ? "#2e1a1a" : "transparent"
                                    Text { anchors.centerIn: parent; text: "\u2715"; color: "#e06666"; font.pixelSize: 9 }
                                    MouseArea { id: delRangeBtn; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                                        onClicked: if (root.configVm) root.configVm.removeLayerRange(delegateIndex)
                                    }
                                }
                            }
                        }
                    }

                    // Add range button
                    Rectangle {
                        width: parent.width
                        height: 24
                        radius: 4
                        color: addRangeBtn.containsMouse ? "#1c2a3e" : Theme.bgElevated
                        border.width: 1
                        border.color: Theme.borderSubtle
                        visible: root.configVm && root.configVm.layerRangeCount() < 10

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 8
                            spacing: 6

                            Text {
                                text: "+ " + qsTr("Add Layer Range")
                                color: addRangeBtn.containsMouse ? Theme.accent : Theme.textSecondary
                                font.pixelSize: 10
                            }
                        }

                        MouseArea {
                            id: addRangeBtn
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (root.configVm) {
                                    var count = root.configVm.layerRangeCount()
                                    // Default: extend from last range or start at 0
                                    var minZ = count > 0 ? root.configVm.layerRangeMaxZ(count - 1) : 0.0
                                    var maxZ = minZ + 5.0
                                    root.configVm.addLayerRange(minZ, maxZ)
                                }
                            }
                        }
                    }
                }

                // 搜索框 + 保存按钮
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Rectangle {
                        Layout.fillWidth: true
                        height: 28
                        radius: 6
                        color: Theme.bgElevated
                        border.width: 1
                        border.color: root.searchText.length > 0 ? Theme.accent : Theme.borderSubtle

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 6

                            Text {
                                text: "\uD83D\uDD0D"
                                color: Theme.textDisabled
                                font.pixelSize: 12
                            }

                            TextField {
                                Layout.fillWidth: true
                                placeholderText: qsTr("搜索参数...")
                                color: Theme.textPrimary
                                font.pixelSize: 11
                                background: null
                                text: root.searchText
                                onTextChanged: root.searchText = text
                            }

                            Rectangle {
                                visible: root.searchText.length > 0
                                width: 16; height: 16; radius: 8
                                color: clearSearchMA.containsMouse ? "#2e1a1a" : "#1e2535"
                                Text { anchors.centerIn: parent; text: "\u2715"; color: "#8a96a8"; font.pixelSize: 8 }
                                MouseArea {
                                    id: clearSearchMA
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.searchText = ""
                                }
                            }

                            // 搜索对话框按钮（对齐上游 SearchDialog）
                            Rectangle {
                                width: 18; height: 18; radius: 3
                                color: openSearchBtn.containsMouse ? "#1c6e42" : "transparent"
                                Text {
                                    anchors.centerIn: parent
                                    text: "\uD83D\uDD0E"
                                    color: openSearchBtn.containsMouse ? "#fff" : Theme.textDisabled
                                    font.pixelSize: 10
                                }
                                MouseArea {
                                    id: openSearchBtn
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: searchDialog.openWithQuery(root.searchText)
                                }
                            }
                        }
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 6
                        color: saveBtn.containsMouse ? "#1c2a3e" : Theme.bgElevated
                        border.width: 1
                        border.color: Theme.borderSubtle
                        Text { anchors.centerIn: parent; text: "\uD83D\uDCBE"; font.pixelSize: 12 }
                        MouseArea { id: saveBtn; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.configVm) root.configVm.saveCurrentPreset()
                        }
                    }

                    Rectangle {
                        width: 28; height: 28; radius: 6
                        color: resetBtn.containsMouse ? "#2e1a1a" : Theme.bgElevated
                        border.width: 1
                        border.color: Theme.borderSubtle
                        Text { anchors.centerIn: parent; text: "\u21BA"; color: "#e06666"; font.pixelSize: 12 }
                        MouseArea { id: resetBtn; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.configVm && root.configVm.printOptions) root.configVm.printOptions.resetToDefaults()
                        }
                    }
                }

                // 脏计数 + 匹配计数
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Text {
                        visible: root.configVm && root.configVm.printOptions && root.configVm.printOptions.dirtyCount() > 0
                        text: qsTr("%1 项已修改").arg(root.configVm ? root.configVm.printOptions.dirtyCount() : 0)
                        color: "#f59e0b"
                        font.pixelSize: 10
                    }

                    Text {
                        visible: root.searchText.length > 0
                        text: qsTr("%1 项匹配").arg(root.filteredIndices.length)
                        color: Theme.textDisabled
                        font.pixelSize: 10
                    }

                    Item { Layout.fillWidth: true }
                }
            }
        }

        // ── Scope Difference Panel (对齐上游 Tab::is_modified_value per-scope diff) ──
        Rectangle {
            Layout.fillWidth: true
            visible: root.configVm && root.configVm.settingsScope !== "global" && root.configVm.scopeOverrideCount() > 0
            Layout.preferredHeight: scopeDiffContent.implicitHeight + 16
            radius: 10
            color: Theme.bgElevated
            border.width: 1
            border.color: Theme.borderSubtle

            ColumnLayout {
                id: scopeDiffContent
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text {
                        text: qsTr("作用域差异 (%1)").arg(root.configVm ? root.configVm.scopeOverrideCount() : 0)
                        color: Theme.textSecondary
                        font.pixelSize: 10
                        font.bold: true
                    }
                    Item { Layout.fillWidth: true }
                    // Reset all overrides button
                    Text {
                        text: qsTr("全部重置")
                        color: Theme.textTertiary
                        font.pixelSize: 10
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            hoverEnabled: true
                            onEntered: parent.color = Theme.accent
                            onExited: parent.color = Theme.textTertiary
                            onClicked: {
                                if (root.configVm) root.configVm.resetAllScopeOverrides()
                            }
                        }
                    }
                }

                // List overridden keys
                Repeater {
                    model: root.configVm ? root.configVm.scopeOverrideCount() : 0
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        required property int index

                        Rectangle {
                            width: 6; height: 6; radius: 3
                            color: "#f59e0b"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: root.configVm ? root.configVm.scopeOverriddenKey(index) : ""
                            color: Theme.textPrimary
                            font.pixelSize: 10
                            elide: Text.ElideMiddle
                            Layout.fillWidth: true
                        }
                        // Reset single override
                        Rectangle {
                            width: 22; height: 18; radius: 4
                            color: scopeResetMA.containsMouse ? "#2e1a1a" : Theme.bgPanel
                            border.width: 1
                            border.color: Theme.borderSubtle
                            Text { anchors.centerIn: parent; text: "\u21BA"; color: "#e06666"; font.pixelSize: 10 }
                            MouseArea {
                                id: scopeResetMA
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                hoverEnabled: true
                                onClicked: {
                                    if (root.configVm) {
                                        var key = root.configVm.scopeOverriddenKey(index)
                                        root.configVm.resetScopeOverride(key)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
