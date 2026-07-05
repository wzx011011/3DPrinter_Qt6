import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"
import "../components"

// LeftSidebar -- three-column layout left panel
// Contains: Printer, Filament, Objects, Transform, Slice Progress
// Extracted from earlier Prepare sidebar code (Phase 52).
Rectangle {
    id: root
    required property var editorVm
    required property var configVm
    property string processCategory: ""
    signal exportRequested()
    property string paramsCurrentTab: "Quality"
    property string paramsSearchText: ""
    readonly property var paramsOptionModel: {
        if (!root.configVm) return null
        return root.configVm.printOptions
    }
    readonly property string paramsTier: "print"
    property var paramsFilteredIndices: []
    // G3: Printer 折叠状态（由 CollapsibleSection 管理）
    property bool printerExpanded: true

    color: "transparent"
    radius: 0
    border.width: 0

    function rebuildParamsFilter() {
        if (!root.configVm || !root.paramsOptionModel) {
            root.paramsFilteredIndices = []
            return
        }
        var indices = root.configVm.filterOptionIndices(
                    root.paramsTier, root.paramsSearchText, true)
        if (root.paramsCurrentTab !== "")
            indices = root.paramsOptionModel.filterIndicesByPage(indices, root.paramsCurrentTab)
        root.paramsFilteredIndices = indices
    }

    CxScrollView {
        anchors.fill: parent
        anchors.margins: Theme.spacingXS

        ColumnLayout {
            width: parent.width
            spacing: Theme.spacingXS

            // -- Section 1: Printer panel (对齐上游 SiderBar SidebarPrinter) --
            // G3: 用 CollapsibleSection 统一卡片风格（与 Filament/Objects 一致）
            CollapsibleSection {
                id: printerSection
                Layout.fillWidth: true
                compact: true
                title: qsTr("打印机")
                iconText: "🖨"
                expanded: printerExpanded
                rightActions: [
                    // Settings gear → ConfigWizard (对齐上游 m_printer_setting)
                    Rectangle {
                        width: 22; height: 22; radius: 4
                        color: printerGearBtn.containsMouse ? Theme.bgHover : "transparent"
                        Image {
                            anchors.centerIn: parent
                            width: 14; height: 14
                            source: "qrc:/qml/assets/icons/settings.svg"
                            fillMode: Image.PreserveAspectFit
                        }
                        MouseArea {
                            id: printerGearBtn
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: backend.showConfigWizard()
                        }
                    }
                ]

                // 展开/折叠状态由 CollapsibleSection 管理（点击标题栏切换）
                onExpandedChanged: printerExpanded = expanded

                content: ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    // Row 1: Printer preset combo + edit + connection (对齐上游 hsizer_printer)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 4

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
                                    root.configVm.requestCurrentPrinterPreset(model[i])
                            }
                        }

                        // Phase 52 PREPSB-03: dirty dot -- configVm.isPresetDirty
                        // is true when the active preset has unsaved option edits.
                        Rectangle {
                            visible: !!root.configVm && root.configVm.isPresetDirty
                            width: 8; height: 8; radius: 4
                            color: Theme.accent
                            Layout.leftMargin: 2
                            ToolTip.text: qsTr("预设已修改（未保存）")
                            ToolTip.visible: dirtyTipMA.containsMouse
                            MouseArea { id: dirtyTipMA; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                        }

                        // Edit preset (对齐上游 edit_btn → open SettingsDialog)
                        Rectangle {
                            // Phase 52 PREPSB-03: builtin/read-only presets are
                            // honestly gated via presetActionBlocker (mirrors
                            // PresetServiceMock::isBuiltinPreset gating).
                            opacity: (root.configVm && root.configVm.presetActionBlocker(2, root.configVm.currentPrinterPreset, "rename") !== "") ? 0.4 : 1.0
                            width: 26; height: 26; radius: 5
                            color: editPresetBtn.containsMouse ? Theme.bgHover : "transparent"
                            border.width: 1; border.color: Theme.borderSubtle
                            Text {
                                anchors.centerIn: parent
                                text: "✎"
                                color: Theme.textMuted
                                font.pixelSize: 12
                            }
                            MouseArea {
                                id: editPresetBtn
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                enabled: !(root.configVm && root.configVm.presetActionBlocker(2, root.configVm.currentPrinterPreset, "rename") !== "")
                                onClicked: backend.forwardSettingsRequest("printer")
                            }
                        }

                        // Connection (对齐上游 connection_btn)
                        Rectangle {
                            width: 26; height: 26; radius: 5
                            color: connBtn.containsMouse ? Theme.bgHover : "transparent"
                            border.width: 1; border.color: Theme.borderSubtle
                            Text {
                                anchors.centerIn: parent
                                text: "⇄"
                                color: Theme.textMuted
                                font.pixelSize: 11
                            }
                            MouseArea {
                                id: connBtn
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: backend.showPrintHostDialog()
                            }
                        }
                    }

                    // Row 2: Bed type (对齐上游 m_bed_type_list)
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

            // -- Section 2: Filament (Prepare sidebar section, Phase 52) --
            CollapsibleSection {
                id: filamentSection
                Layout.fillWidth: true
                compact: true
                title: qsTr("耗材")
                iconText: "F"
                expanded: true  // G1: 默认展开（对齐上游，耗材列表可见）

                rightActions: [
                    // 编辑耗材预设（对齐上游 m_edit_filament_btn，镜像打印机区 ✎ 编辑按钮）
                    // 依赖 CollapsibleSection 折叠点击区置底修复——否则本按钮点击被吞。
                    Rectangle {
                        opacity: (root.configVm && root.configVm.presetActionBlocker(1, root.configVm.currentFilamentPreset, "rename") !== "") ? 0.4 : 1.0
                        width: 22; height: 22; radius: 4
                        color: editFilaBtn.containsMouse ? Theme.bgHover : "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: "✎"
                            color: Theme.textMuted
                            font.pixelSize: 12
                        }
                        MouseArea {
                            id: editFilaBtn
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            enabled: !(root.configVm && root.configVm.presetActionBlocker(1, root.configVm.currentFilamentPreset, "rename") !== "")
                            onClicked: backend.forwardSettingsRequest("filament")
                        }
                    },
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        visible: !!root.configVm && !root.configVm.isCurrentFilamentCompatible()
                        color: "#f05545"
                        ToolTip.text: qsTr("当前耗材可能与打印机不兼容")
                        ToolTip.visible: compMA2.containsMouse
                        ToolTip.delay: 500
                        MouseArea { id: compMA2; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    },
                    Rectangle {
                        width: 22; height: 22; radius: 4
                        color: autoMatchBtn.containsMouse ? "#1c2a3e" : "transparent"
                        Text { anchors.centerIn: parent; text: "↻"; color: Theme.textMuted; font.pixelSize: 13; font.bold: true }
                        MouseArea { id: autoMatchBtn; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: if (root.configVm) root.configVm.autoMatchFilament()
                        }
                    }
                ]

                content: ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 6

                    // Filament slot grid — interactive (aligned with upstream Sidebar filament slots)
                    GridLayout {
                        columns: 1
                        rowSpacing: 4
                        columnSpacing: 4
                        Layout.fillWidth: true
                        Layout.preferredHeight: childrenRect.height

                        Repeater {
                            // Phase 52 PREPSB-01: dynamic slot count (upstream
                            // Plater shows one filament slot per extruder).
                            // editorVm.extruderCount is the extruder-count surface;
                            // guard with Math.max(1, ...) so a single-extruder (the
                            // common mock default) always shows at least one slot.
                            // Full multi-extruder count arrives when Phase 56 wires
                            // real PresetBundle nozzle_diameter data.
                            model: Math.max(1, root.editorVm ? root.editorVm.extruderCount : 1)
                            delegate: FilamentSlot {
                                required property int index
                                Layout.fillWidth: true
                                slotIndex: index
                                configVm: root.configVm
                            }
                        }
                    }
                }
            }

            // -- Section 3: Process 顶部条 (SIDEBAR-06/07/08 骨架, 对齐上游 ParamsPanel.m_top_panel) --
            // 上游: Process icon + SwitchButton(Global/Objects) + ModeIcon + ModeSwitchButton(Simple/Advanced) + Compare + Setting
            Rectangle {
                id: processTopbar
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                color: Theme.bgPanel
                radius: Theme.radiusSM
                border.width: 1
                border.color: Theme.borderSubtle

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 4

                    // Process icon + label (对齐上游 m_process_title)
                    Text {
                        text: "⚙"
                        color: Theme.accent
                        font.pixelSize: 14
                    }
                    Text {
                        text: qsTr("工艺")
                        color: Theme.textPrimary
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        Layout.leftMargin: 2
                    }
                    // Phase 52 PREPSB-03: process preset dirty dot (same
                    // isPresetDirty source as the printer row dirty dot).
                    Rectangle {
                        visible: !!root.configVm && root.configVm.isPresetDirty
                        width: 8; height: 8; radius: 4
                        color: Theme.accent
                        Layout.leftMargin: 2
                        ToolTip.text: qsTr("预设已修改（未保存）")
                        ToolTip.visible: procDirtyTipMA.containsMouse
                        MouseArea { id: procDirtyTipMA; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    }
                    Item { Layout.fillWidth: true }

                    // SwitchButton(Global/Objects) → 绑 settingsScope (SIDEBAR-07 做实)
                    // 对齐上游 m_scope_switch (ParamsPanel.cpp)
                    CxButton {
                        implicitWidth: 44
                        implicitHeight: 24
                        compact: true
                        cxStyle: root.configVm && root.configVm.settingsScope === "global"
                                 ? CxButton.Style.Primary : CxButton.Style.Ghost
                        text: qsTr("全局")
                        onClicked: if (root.configVm) root.configVm.requestGlobalScope()
                    }
                    CxButton {
                        implicitWidth: 44
                        implicitHeight: 24
                        compact: true
                        cxStyle: root.configVm && root.configVm.settingsScope !== "global"
                                 ? CxButton.Style.Primary : CxButton.Style.Ghost
                        text: qsTr("对象")
                        onClicked: {
                            // SIDEBAR-07: 切到对象作用域（用当前选中对象，无选中则提示）
                            if (root.editorVm && root.editorVm.selectedObjectIndex >= 0 && root.configVm) {
                                root.configVm.requestObjectScope("object", "",
                                    root.editorVm.selectedObjectIndex, -1)
                            }
                        }
                    }

                    // Phase 52 PREPSB-04: Plate scope (completes the
                    // Global/Object/Plate triad; mirrors upstream ParamsPanel
                    // scope switch). Uses the current plate index.
                    CxButton {
                        implicitWidth: 44
                        implicitHeight: 24
                        compact: true
                        cxStyle: root.configVm && root.configVm.settingsScope === "plate"
                                 ? CxButton.Style.Primary : CxButton.Style.Ghost
                        text: qsTr("盘")
                        enabled: root.editorVm && root.editorVm.currentPlateIndex >= 0
                        onClicked: {
                            if (root.editorVm && root.editorVm.currentPlateIndex >= 0 && root.configVm)
                                root.configVm.requestPlateScope(root.editorVm.currentPlateIndex)
                        }
                    }

                    // Phase 56: Advanced (Simple/Advanced) toggle -- settings-dialog
                    // feature, hidden until Phase 56.
                    CxButton {
                        implicitWidth: 44
                        implicitHeight: 24
                        compact: true
                        cxStyle: CxButton.Style.Ghost
                        text: qsTr("高级")  // 占位: Simple/Advanced 切换
                        enabled: false
                        visible: false
                    }

                    // Phase 56: Compare (DiffPresetDialog) -- settings-dialog feature,
                    // hidden until Phase 56.
                    CxButton {
                        implicitWidth: 28
                        implicitHeight: 24
                        compact: true
                        cxStyle: CxButton.Style.Ghost
                        text: "⇄"
                        enabled: false
                        visible: false
                    }

                    // Opens the independent process settings dialog.
                    CxButton {
                        implicitWidth: 28
                        implicitHeight: 24
                        compact: true
                        cxStyle: CxButton.Style.Ghost
                        text: "☰"
                        enabled: true
                        visible: true
                        onClicked: backend.forwardSettingsRequest("process")
                    }
                }
            }

            RowLayout {
                id: processPresetRow
                Layout.fillWidth: true
                spacing: 6

                CxComboBox {
                    Layout.fillWidth: true
                    model: root.configVm ? root.configVm.printPresetNames : []
                    currentIndex: {
                        if (!root.configVm) return -1
                        return root.configVm.printPresetNames.indexOf(root.configVm.currentPrintPreset)
                    }
                    onActivated: (i) => {
                        if (root.configVm && i >= 0)
                            root.configVm.requestCurrentPrintPreset(model[i])
                    }
                }

                Rectangle {
                    width: 26
                    height: 26
                    radius: 5
                    color: processEditMA.containsMouse ? Theme.bgHover : "transparent"
                    border.width: 1
                    border.color: Theme.borderSubtle

                    Text {
                        anchors.centerIn: parent
                        text: "✎"
                        color: Theme.textMuted
                        font.pixelSize: 12
                    }

                    MouseArea {
                        id: processEditMA
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: backend.forwardSettingsRequest("process")
                    }
                }
            }

            // -- Section 4: Process search and inline option list --
            Rectangle {
                id: searchBox
                Layout.fillWidth: true
                Layout.preferredHeight: 30
                color: Theme.bgInset
                radius: Theme.radiusSM
                border.width: 1
                border.color: searchField.activeFocus ? Theme.accent : Theme.borderSubtle

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 4

                    Text {
                        text: "🔍"
                        color: Theme.textTertiary
                        font.pixelSize: 12
                    }
                    TextField {
                        id: searchField
                        Layout.fillWidth: true
                        placeholderText: qsTr("搜索设置...")
                        color: Theme.textPrimary
                        font.pixelSize: 11
                        background: Item {}  // 透明背景（外框由 searchBox 提供）
                        selectByMouse: true
                        onAccepted: {
                            root.rebuildParamsFilter()
                        }
                        onTextChanged: {
                            root.paramsSearchText = text.trim()
                            root.rebuildParamsFilter()
                        }
                    }
                }
            }

            Rectangle {
                id: paramsInlinePanel
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(500, Math.max(260, paramsList.contentHeight + 44))
                color: Theme.bgPanel
                radius: Theme.radiusSM
                border.width: 1
                border.color: Theme.borderSubtle

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 3

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Repeater {
                            model: [
                                { key: "Quality", label: qsTr("质量") },
                                { key: "Strength", label: qsTr("强度") },
                                { key: "Support", label: qsTr("支撑") },
                                { key: "Temperature", label: qsTr("材料") },
                                { key: "Other", label: qsTr("其他") }
                            ]
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 24
                                color: modelData.key === root.paramsCurrentTab
                                       ? Theme.accent : "transparent"
                                radius: 3

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.label
                                    color: modelData.key === root.paramsCurrentTab
                                           ? Theme.textOnAccent : Theme.textSecondary
                                    font.pixelSize: Theme.fontSizeXS
                                    font.bold: modelData.key === root.paramsCurrentTab
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        root.paramsCurrentTab = modelData.key
                                        root.rebuildParamsFilter()
                                    }
                                }
                            }
                        }
                    }

                    ListView {
                        id: paramsList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: root.paramsFilteredIndices
                        spacing: 0

                        ScrollBar.vertical: ScrollBar {
                            visible: paramsList.contentHeight > paramsList.height
                        }

                        delegate: Item {
                            id: paramsDelegate
                            required property int index
                            required property var modelData

                            readonly property int optIdx: modelData
                            readonly property string optGroup: root.paramsOptionModel
                                ? root.paramsOptionModel.optGroup(optIdx) : ""
                            readonly property bool showGroupHeader: {
                                if (paramsDelegate.index === 0) return optGroup !== ""
                                var prevGroup = root.paramsOptionModel
                                    ? root.paramsOptionModel.optGroup(root.paramsFilteredIndices[paramsDelegate.index - 1]) : ""
                                return optGroup !== "" && optGroup !== prevGroup
                            }

                            width: paramsList.width
                            height: optRow.totalHeight

                            OptionRow {
                                id: optRow
                                anchors.left: parent.left
                                anchors.right: parent.right
                                optionModel: root.paramsOptionModel
                                optIdx: paramsDelegate.optIdx
                                rowIndex: paramsDelegate.index
                                searchText: root.paramsSearchText
                                showGroupHeader: paramsDelegate.showGroupHeader
                                oGroup: paramsDelegate.optGroup
                                compact: true
                                compactLabelWidth: 118
                                compactFieldWidth: 66
                                compactEnumWidth: 104
                                valueSource: {
                                    if (!root.configVm || !root.paramsOptionModel) return ""
                                    var key = root.paramsOptionModel.optKey(paramsDelegate.optIdx)
                                    return root.configVm.valueSourceForKey(key)
                                }
                            }
                        }
                    }
                }

                Component.onCompleted: root.rebuildParamsFilter()
            }

            // -- Section 5: Objects (Prepare sidebar section, Phase 52) --
            CollapsibleSection {
                id: objectSection
                Layout.fillWidth: true
                compact: true
                title: qsTr("对象")
                iconText: "◇"
                expanded: true

                content: ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 6

                    // All/Objects filter (from PreparePage leftPanel lines 3026-3075)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: Theme.spacingSM

                        Rectangle {
                            Layout.fillWidth: true
                            height: 28
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
                            height: 28
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

                    // Object list header
                    Text {
                        Layout.fillWidth: true
                        text: root.editorVm
                            ? (root.editorVm.objectCount + qsTr(" 个对象"))
                            : qsTr("0 个对象")
                        color: Theme.textSecondary
                        font.pixelSize: Theme.fontSizeXS
                    }

                    // Object list
                    ObjectList {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 200
                        editorVm: root.editorVm
                        showToolbar: false
                        showImportButton: false
                    }
                }
            }

            // -- Section 6: ObjectSettings (SIDEBAR-13, 选中对象的快速设置, 无选中隐藏) --
            // 对齐上游 ObjectSettings: 选中对象时显示层高/填充/速度等快速设置
            CollapsibleSection {
                id: sliceProgressSection
                Layout.fillWidth: true
                compact: true
                title: qsTr("切片")
                expanded: true

                content: ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 0

                    SliceProgress {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 620
                        editorVm: root.editorVm
                        onExportRequested: root.exportRequested()
                    }
                }
            }

            CollapsibleSection {
                id: objectSettingsSection
                Layout.fillWidth: true
                compact: true
                title: qsTr("对象设置")
                iconText: "☰"
                expanded: true
                // 无选中对象时隐藏整个区块
                visible: root.editorVm && root.editorVm.selectedObjectIndex >= 0

                content: ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 6

                    Text {
                        Layout.fillWidth: true
                        text: root.editorVm && root.editorVm.selectedObjectIndex >= 0
                              ? qsTr("正在编辑对象 #") + (root.editorVm.selectedObjectIndex + 1)
                              : qsTr("未选择对象")
                        color: Theme.textSecondary
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }

                    // 快速设置项（绑定 configVm，对齐上游 ObjectSettings 常用参数）
                    // 层高
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        visible: !!root.configVm
                        Text { text: qsTr("层高"); color: Theme.textTertiary; font.pixelSize: 10; Layout.preferredWidth: 90 }
                        TextField {
                            Layout.fillWidth: true
                            text: root.configVm ? root.configVm.layerHeight : ""
                            color: Theme.textPrimary
                            font.pixelSize: 10
                            selectByMouse: true
                            background: Item {}
                            readOnly: true
                        }
                    }
                    // 填充密度
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        visible: !!root.configVm
                        Text { text: qsTr("填充密度"); color: Theme.textTertiary; font.pixelSize: 10; Layout.preferredWidth: 90 }
                        TextField {
                            Layout.fillWidth: true
                            text: root.configVm ? (root.configVm.infillDensity + "%") : ""
                            color: Theme.textPrimary
                            font.pixelSize: 10
                            selectByMouse: true
                            background: Item {}
                            readOnly: true
                        }
                    }
                    // 打印速度
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        visible: !!root.configVm
                        Text { text: qsTr("打印速度"); color: Theme.textTertiary; font.pixelSize: 10; Layout.preferredWidth: 90 }
                        TextField {
                            Layout.fillWidth: true
                            text: root.configVm ? (root.configVm.printSpeed + " mm/s") : ""
                            color: Theme.textPrimary
                            font.pixelSize: 10
                            selectByMouse: true
                            background: Item {}
                            readOnly: true
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 20 }
        }
    }

    // -- Rename printer preset dialog --
    CxDialog {
        id: renamePrinterDialog
        dialogTitle: qsTr("重命名预设")
        width: 320
        ColumnLayout {
            spacing: 8
            CxTextField {
                id: renamePrinterField
                Layout.fillWidth: true
                text: root.configVm ? root.configVm.currentPrinterPreset : ""
                selectByMouse: true
                onAccepted: renamePrinterDialog.accept()
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 8
                CxButton { text: qsTr("取消"); cxStyle: CxButton.Style.Ghost; onClicked: renamePrinterDialog.reject() }
                CxButton { text: qsTr("确认"); onClicked: renamePrinterDialog.accept() }
            }
        }
        onAccepted: {
            if (root.configVm && renamePrinterField.text.trim().length > 0)
                root.configVm.renamePreset(2, root.configVm.currentPrinterPreset, renamePrinterField.text.trim())
        }
        onOpened: renamePrinterField.selectAll()
    }
}
