import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"
import "../components"

// LeftSidebar -- three-column layout left panel
// Contains: Printer, Filament, Objects, Transform, Slice Progress
// Extracted from PreparePage leftPanel + Sidebar.qml content
Rectangle {
    id: root
    required property var editorVm
    required property var configVm
    property string processCategory: ""
    // G3: Printer 折叠状态（由 CollapsibleSection 管理）
    property bool printerExpanded: true

    color: "transparent"
    radius: 0
    border.width: 0

    CxScrollView {
        anchors.fill: parent
        anchors.margins: Theme.spacingSM

        ColumnLayout {
            width: parent.width
            spacing: 10  // 区块间距增大（对齐上游 OrcaSlicer 卡片层次）

            // -- Section 1: Printer panel (对齐上游 SiderBar SidebarPrinter) --
            // G3: 用 CollapsibleSection 统一卡片风格（与 Filament/Objects 一致）
            CollapsibleSection {
                id: printerSection
                Layout.fillWidth: true
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

                        // Edit preset (对齐上游 edit_btn → open SettingsPage)
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
                                onClicked: backend.openSettings()
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

            // -- Section 2: Filament (from PrintSettings.qml lines 626-704) --
            CollapsibleSection {
                id: filamentSection
                Layout.fillWidth: true
                title: qsTr("耗材")
                iconText: "F"
                expanded: true  // G1: 默认展开（对齐上游，耗材列表可见）

                rightActions: [
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
                        columns: 3
                        rowSpacing: 8
                        columnSpacing: 8
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
                Layout.preferredHeight: 40
                color: Theme.bgPanel
                radius: 6
                border.width: 1
                border.color: Theme.borderSubtle

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

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

                    // Phase 52 PREPSB-02: "Setting" entry point -- visible and
                    // enabled. Emits settingsRequested("process"); BackendContext
                    // forwards it (interim no-op log; Phase 56 wires the real
                    // independent settings dialog). Honest deferred entry point.
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

            // -- Section 4: Search bar (SIDEBAR-11, 对齐上游 OG::SearchCtrl) --
            Rectangle {
                id: searchBox
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                color: Theme.bgInset
                radius: 6
                border.width: 1
                border.color: searchField.activeFocus ? Theme.accent : Theme.borderSubtle

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 6

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
                            // Phase 52 PREPSB-04: wire search to
                            // configVm.filterOptionIndices so the search is live.
                            // The matched option indices drive the config option
                            // model; full ParamsPanel option rendering is Phase 56.
                            if (root.configVm && text.trim().length > 0) {
                                root.configVm.filterOptionIndices("", text.trim(), false)
                            }
                        }
                        onTextChanged: {
                            // Phase 52 PREPSB-04: live filter as the user types.
                            if (root.configVm) {
                                root.configVm.filterOptionIndices("", text.trim(), false)
                            }
                        }
                    }
                }
            }

            // -- Section 5: Objects (from PreparePage leftPanel + Sidebar.qml) --
            CollapsibleSection {
                id: objectSection
                Layout.fillWidth: true
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
                id: objectSettingsSection
                Layout.fillWidth: true
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

            // -- Section 7: ObjectLayers (SIDEBAR-14, 变量层高编辑器, 仅打印对象显示) --
            // 对齐上游 ObjectLayers: 变量层高编辑器（v2.0 占位，完整编辑器延后）
            CollapsibleSection {
                id: objectLayersSection
                Layout.fillWidth: true
                title: qsTr("可变层高")
                iconText: "≣"
                expanded: false  // 默认折叠（占位）
                // 仅打印对象时显示
                visible: root.editorVm && root.editorVm.selectedObjectIndex >= 0

                content: ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 6

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("可变层高编辑器暂不可用")
                        color: Theme.textTertiary
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }
                }
            }

            // -- Section 8: ParamsPanel page_view (SIDEBAR-15, 参数列表 + 7 子 Tab) --
            // 对齐上游 ParamsPanel page_view: 左侧 7 子 Tab (Print/PrintPlate/PrintObject/
            // PrintPart/PrintLayer/Filament/Printer) + 右侧参数列表
            // v2.0 骨架: 7 Tab 按钮 + 参数列表占位（完整参数需 ConfigViewModel pageView 扩展）
            CollapsibleSection {
                id: paramsPanelSection
                Layout.fillWidth: true
                title: qsTr("参数")
                iconText: "▽"
                expanded: true

                content: ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 4

                    // 7 子 Tab 按钮（对齐上游 ParamsPanel 左侧 TabList）
                    // 完整参数列表需 ConfigViewModel pageView 分组扩展
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Repeater {
                            // 7 子 Tab: Print/PrintPlate/PrintObject/PrintPart/PrintLayer/Filament/Printer
                            model: [
                                { key: "print", label: qsTr("打印") },
                                { key: "printPlate", label: qsTr("盘") },
                                { key: "printObject", label: qsTr("对象") },
                                { key: "printPart", label: qsTr("部件") },
                                { key: "printLayer", label: qsTr("层") },
                                { key: "filament", label: qsTr("耗材") },
                                { key: "printer", label: qsTr("打印机") }
                            ]
                            delegate: Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 22
                                color: modelData.key === paramsTabBar.currentTab
                                       ? Theme.accent : Theme.bgPanel
                                radius: 3
                                border.width: 1
                                border.color: Theme.borderSubtle

                                Text {
                                    anchors.centerIn: parent
                                    text: modelData.label
                                    color: modelData.key === paramsTabBar.currentTab
                                           ? Theme.textOnAccent : Theme.textSecondary
                                    font.pixelSize: Theme.fontSizeXS
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: paramsTabBar.currentTab = modelData.key
                                }
                            }
                        }
                    }

                    // 参数列表占位
                    Text {
                        Layout.fillWidth: true
                        Layout.leftMargin: 4
                        text: qsTr("参数列表暂不可用")
                        color: Theme.textTertiary
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }
                }

                // 7 子 Tab 当前选中状态（内联，避免污染外层）
                QtObject {
                    id: paramsTabBar
                    property string currentTab: "print"
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
