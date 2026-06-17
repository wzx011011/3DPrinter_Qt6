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

    color: Theme.bgPanel
    radius: 18
    border.width: 1
    border.color: Theme.borderSubtle

    CxScrollView {
        anchors.fill: parent
        anchors.margins: Theme.spacingSM

        ColumnLayout {
            width: parent.width
            spacing: 4

            // -- Section 1: Printer panel (对齐上游 SiderBar SidebarPrinter) --
            ColumnLayout {
                id: printerSection
                Layout.fillWidth: true
                spacing: 0

                // Title bar: printer icon + label + gear (对齐上游 m_panel_printer_title)
                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    radius: 6
                    color: Theme.bgElevated

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 6
                        spacing: 6

                        Image {
                            width: 16; height: 16
                            source: "qrc:/qml/assets/icons/printer.svg"
                            fillMode: Image.PreserveAspectFit
                        }

                        Text {
                            text: qsTr("打印机")
                            color: Theme.textPrimary
                            font.pixelSize: 12
                            font.bold: true
                            Layout.fillWidth: true
                        }

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

                        // Expand/collapse arrow
                        Text {
                            text: printerExpanded ? "▾" : "▸"
                            color: Theme.textDisabled
                            font.pixelSize: 10
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: printerExpanded = !printerExpanded
                    }

                    property bool printerExpanded: true
                }

                // Separator (对齐上游 StaticLine #CECECE)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 4; Layout.rightMargin: 4
                    height: 1
                    color: Theme.borderSubtle
                    visible: printerExpanded
                }

                // Content panel (对齐上游 m_panel_printer_content)
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.leftMargin: 4
                    Layout.rightMargin: 4
                    spacing: 8
                    visible: printerExpanded

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
                                    root.configVm.setCurrentPrinterPreset(model[i])
                            }
                        }

                        // Edit preset (对齐上游 edit_btn → open SettingsPage)
                        Rectangle {
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

                // Bottom separator (对齐上游 #A6A9AA)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: 4
                    Layout.bottomMargin: 2
                    height: 1
                    color: Theme.borderSubtle
                }

                property bool printerExpanded: true
            }

            // -- Section 2: Filament (from PrintSettings.qml lines 626-704) --
            CollapsibleSection {
                id: filamentSection
                Layout.fillWidth: true
                title: qsTr("耗材")
                iconText: "F"
                expanded: false

                rightActions: [
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        visible: !!root.configVm && !root.configVm.isCurrentFilamentCompatible()
                        color: "#f05545"
                        ToolTip.text: qsTr("Current filament may be incompatible with printer")
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
                            model: 5
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
                        text: qsTr("Process")
                        color: Theme.textPrimary
                        font.pixelSize: 12
                        font.weight: Font.Medium
                        Layout.leftMargin: 2
                    }
                    Item { Layout.fillWidth: true }

                    // SwitchButton(Global/Objects) → 绑 settingsScope (SIDEBAR-07 做实)
                    // 对齐上游 m_scope_switch (ParamsPanel.cpp)
                    CxButton {
                        implicitWidth: 52
                        implicitHeight: 24
                        compact: true
                        cxStyle: root.configVm && root.configVm.settingsScope === "global"
                                 ? CxButton.Style.Primary : CxButton.Style.Ghost
                        text: qsTr("Global")
                        onClicked: if (root.configVm) root.configVm.activateGlobalScope()
                    }
                    CxButton {
                        implicitWidth: 56
                        implicitHeight: 24
                        compact: true
                        cxStyle: root.configVm && root.configVm.settingsScope !== "global"
                                 ? CxButton.Style.Primary : CxButton.Style.Ghost
                        text: qsTr("Objects")
                        onClicked: {
                            // SIDEBAR-07: 切到对象作用域（用当前选中对象，无选中则提示）
                            if (root.editorVm && root.editorVm.selectedObjectIndex >= 0 && root.configVm) {
                                root.configVm.activateObjectScope("object", "",
                                    root.editorVm.selectedObjectIndex, -1)
                            }
                        }
                    }

                    // ModeSwitchButton(Simple/Advanced) — SIDEBAR-08 骨架 (configMode 待 VM 扩展)
                    CxButton {
                        implicitWidth: 60
                        implicitHeight: 24
                        compact: true
                        cxStyle: CxButton.Style.Ghost
                        text: qsTr("Advanced")  // 占位: Simple/Advanced 切换
                        onClicked: {
                            // TODO SIDEBAR-08: 需 ConfigViewModel configMode 属性 + 参数过滤
                        }
                    }

                    // Compare 按钮 (SIDEBAR-09 占位, v2.2 DiffPresetDialog)
                    CxButton {
                        implicitWidth: 28
                        implicitHeight: 24
                        compact: true
                        cxStyle: CxButton.Style.Ghost
                        text: "⇄"
                        onClicked: {
                            // TODO SIDEBAR-09: 打开 DiffPresetDialog (v2.2)
                        }
                    }

                    // Setting 按钮 (SIDEBAR-10 占位, ObjectTableDialog)
                    CxButton {
                        implicitWidth: 28
                        implicitHeight: 24
                        compact: true
                        cxStyle: CxButton.Style.Ghost
                        text: "☰"
                        onClicked: {
                            // TODO SIDEBAR-10: 打开 ObjectTableDialog
                        }
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
                        placeholderText: qsTr("Search settings...")
                        color: Theme.textPrimary
                        font.pixelSize: 11
                        background: Item {}  // 透明背景（外框由 searchBox 提供）
                        selectByMouse: true
                        onAccepted: {
                            // SIDEBAR-11: 跳转到 SearchDialog（复用现有搜索）
                            if (text.trim().length > 0 && root.configVm) {
                                // 触发搜索：调用 configVm 的 preset 搜索或打开 SearchDialog
                                // 当前简化：清空并提示（完整跳转需 SearchDialog 集成）
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
                title: qsTr("Object Settings")
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
                              ? qsTr("Editing object #") + (root.editorVm.selectedObjectIndex + 1)
                              : qsTr("(no selection)")
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
                        Text { text: qsTr("Layer Height"); color: Theme.textTertiary; font.pixelSize: 10; Layout.preferredWidth: 90 }
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
                        Text { text: qsTr("Infill Density"); color: Theme.textTertiary; font.pixelSize: 10; Layout.preferredWidth: 90 }
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
                        Text { text: qsTr("Print Speed"); color: Theme.textTertiary; font.pixelSize: 10; Layout.preferredWidth: 90 }
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
                title: qsTr("Variable Layer Height")
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
                        text: qsTr("Variable layer height editor (reserved)")
                        color: Theme.textTertiary
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }
                    // TODO SIDEBAR-14: 完整变量层高编辑器（层高曲线 + 拖拽编辑）
                }
            }

            // -- Section 8: ParamsPanel page_view (SIDEBAR-15, 参数列表 + 7 子 Tab) --
            // 对齐上游 ParamsPanel page_view: 左侧 7 子 Tab (Print/PrintPlate/PrintObject/
            // PrintPart/PrintLayer/Filament/Printer) + 右侧参数列表
            // v2.0 骨架: 7 Tab 按钮 + 参数列表占位（完整参数需 ConfigViewModel pageView 扩展）
            CollapsibleSection {
                id: paramsPanelSection
                Layout.fillWidth: true
                title: qsTr("Parameters")
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
                                { key: "print", label: qsTr("Print") },
                                { key: "printPlate", label: qsTr("Plate") },
                                { key: "printObject", label: qsTr("Object") },
                                { key: "printPart", label: qsTr("Part") },
                                { key: "printLayer", label: qsTr("Layer") },
                                { key: "filament", label: qsTr("Filam.") },
                                { key: "printer", label: qsTr("Printer") }
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
                                    font.pixelSize: 8
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: paramsTabBar.currentTab = modelData.key
                                }
                            }
                        }

                        // currentTab 状态（7 子 Tab 切换）
                        // TODO SIDEBAR-15: 切换时按 pageView 分组加载参数列表（需 ConfigViewModel 扩展）
                    }

                    // 参数列表占位
                    Text {
                        Layout.fillWidth: true
                        Layout.leftMargin: 4
                        text: qsTr("(parameter list — reserved)")
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

            // ── v1.x 残留区块（Phase 6 迁移到 Gizmo 浮层时清理）──
            // -- Section 4: Transform (from Sidebar.qml lines 108-362) --
            CollapsibleSection {
                Layout.fillWidth: true
                title: qsTr("变换")
                iconText: "✥"
                expanded: true
                visible: root.editorVm && root.editorVm.hasObjectManipSelection

                content: ColumnLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    spacing: 6

                    // Title row with uniform scale + reset buttons
                    RowLayout {
                        Layout.fillWidth: true
                        Item { Layout.fillWidth: true }
                        Rectangle {
                            width: uniformMA.containsMouse ? 56 : 50
                            height: 20
                            radius: 5
                            color: root.editorVm && root.editorVm.uniformScale ? Theme.accentSubtle : Theme.bgElevated
                            border.width: 1
                            border.color: root.editorVm && root.editorVm.uniformScale ? Theme.accent : Theme.borderSubtle
                            Text { anchors.centerIn: parent; text: qsTr("统一"); color: root.editorVm && root.editorVm.uniformScale ? Theme.accent : Theme.textDisabled; font.pixelSize: 9 }
                            MouseArea {
                                id: uniformMA
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { if (root.editorVm) root.editorVm.uniformScale = !root.editorVm.uniformScale }
                            }
                        }
                        Rectangle {
                            width: resetMA.containsMouse ? 48 : 42
                            height: 20
                            radius: 5
                            color: Theme.bgElevated
                            border.width: 1
                            border.color: Theme.borderSubtle
                            Text { anchors.centerIn: parent; text: qsTr("重置"); color: Theme.textSecondary; font.pixelSize: 9 }
                            MouseArea {
                                id: resetMA
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: { if (root.editorVm) root.editorVm.resetObjectTransform() }
                            }
                        }
                    }

                    // Position
                    Repeater {
                        model: [
                            {label: "X", color: "#ef4444", px: "objectPosX"},
                            {label: "Y", color: "#22c55e", px: "objectPosY"},
                            {label: "Z", color: "#3b82f6", px: "objectPosZ"}
                        ]
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            required property var modelData

                            Text {
                                text: modelData.label
                                color: modelData.color
                                font.pixelSize: 10
                                font.bold: true
                                Layout.preferredWidth: 14
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                height: 22
                                radius: 4
                                color: Theme.bgInset
                                border.width: 1
                                border.color: spinMA.containsMouse ? modelData.color : Theme.borderInput
                                TextInput {
                                    anchors.fill: parent
                                    anchors.leftMargin: 4
                                    anchors.rightMargin: 4
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignRight
                                    color: Theme.textPrimary
                                    font.pixelSize: 10
                                    font.family: "monospace"
                                    selectByMouse: true
                                    validator: DoubleValidator { decimals: 2; notation: DoubleValidator.StandardNotation }
                                    text: {
                                        if (!root.editorVm) return "0"
                                        return Number(root.editorVm[modelData.px]).toFixed(2)
                                    }
                                    onEditingFinished: {
                                        if (!root.editorVm) return
                                        const val = parseFloat(text)
                                        if (isNaN(val)) return
                                        root.editorVm[modelData.px] = val
                                    }
                                }
                                MouseArea {
                                    id: spinMA
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.NoButton
                                }
                            }
                            Text { text: "mm"; color: Theme.textDisabled; font.pixelSize: 9; Layout.preferredWidth: 18 }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

                    // Rotation
                    Text { text: qsTr("旋转 (°)"); color: Theme.textTertiary; font.pixelSize: 9 }
                    Repeater {
                        model: [
                            {label: "X", color: "#ef4444", px: "objectRotX"},
                            {label: "Y", color: "#22c55e", px: "objectRotY"},
                            {label: "Z", color: "#3b82f6", px: "objectRotZ"}
                        ]
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            required property var modelData

                            Text {
                                text: modelData.label
                                color: modelData.color
                                font.pixelSize: 10
                                font.bold: true
                                Layout.preferredWidth: 14
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                height: 22
                                radius: 4
                                color: Theme.bgInset
                                border.width: 1
                                border.color: rotMA.containsMouse ? modelData.color : Theme.borderInput
                                TextInput {
                                    anchors.fill: parent
                                    anchors.leftMargin: 4
                                    anchors.rightMargin: 4
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignRight
                                    color: Theme.textPrimary
                                    font.pixelSize: 10
                                    font.family: "monospace"
                                    selectByMouse: true
                                    validator: DoubleValidator { decimals: 1; bottom: -360; top: 360 }
                                    text: {
                                        if (!root.editorVm) return "0"
                                        return Number(root.editorVm[modelData.px]).toFixed(1)
                                    }
                                    onEditingFinished: {
                                        if (!root.editorVm) return
                                        const val = parseFloat(text)
                                        if (isNaN(val)) return
                                        root.editorVm[modelData.px] = val
                                    }
                                }
                                MouseArea {
                                    id: rotMA
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.NoButton
                                }
                            }
                            Text { text: "°"; color: Theme.textDisabled; font.pixelSize: 9; Layout.preferredWidth: 18 }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.borderSubtle }

                    // Scale
                    Text { text: qsTr("缩放 (%)"); color: Theme.textTertiary; font.pixelSize: 9 }
                    Repeater {
                        model: [
                            {label: "X", color: "#ef4444", px: "objectScaleX"},
                            {label: "Y", color: "#22c55e", px: "objectScaleY"},
                            {label: "Z", color: "#3b82f6", px: "objectScaleZ"}
                        ]
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            required property var modelData

                            Text {
                                text: modelData.label
                                color: modelData.color
                                font.pixelSize: 10
                                font.bold: true
                                Layout.preferredWidth: 14
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                height: 22
                                radius: 4
                                color: Theme.bgInset
                                border.width: 1
                                border.color: sclMA.containsMouse ? modelData.color : Theme.borderInput
                                TextInput {
                                    anchors.fill: parent
                                    anchors.leftMargin: 4
                                    anchors.rightMargin: 4
                                    verticalAlignment: Text.AlignVCenter
                                    horizontalAlignment: Text.AlignRight
                                    color: Theme.textPrimary
                                    font.pixelSize: 10
                                    font.family: "monospace"
                                    selectByMouse: true
                                    validator: DoubleValidator { decimals: 1; bottom: 0.01 }
                                    text: {
                                        if (!root.editorVm) return "100"
                                        return (Number(root.editorVm[modelData.px]) * 100).toFixed(1)
                                    }
                                    onEditingFinished: {
                                        if (!root.editorVm) return
                                        const val = parseFloat(text)
                                        if (isNaN(val) || val <= 0) return
                                        root.editorVm[modelData.px] = val / 100.0
                                    }
                                }
                                MouseArea {
                                    id: sclMA
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.NoButton
                                }
                            }
                            Text { text: "%"; color: Theme.textDisabled; font.pixelSize: 9; Layout.preferredWidth: 18 }
                        }
                    }

                    // Dimensions info (read-only)
                    Rectangle {
                        Layout.fillWidth: true
                        height: dimRow.implicitHeight + 10
                        radius: 4
                        color: Theme.bgInset
                        visible: root.editorVm && root.editorVm.measureDimensions.x > 0

                        RowLayout {
                            id: dimRow
                            anchors.fill: parent
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8
                            spacing: 6
                            Text { text: "W:"; color: "#ef4444"; font.pixelSize: 9; font.bold: true }
                            Text { text: root.editorVm ? root.editorVm.measureDimensions.x.toFixed(1) : "--"; color: Theme.textSecondary; font.pixelSize: 9; font.family: "monospace" }
                            Text { text: "D:"; color: "#22c55e"; font.pixelSize: 9; font.bold: true }
                            Text { text: root.editorVm ? root.editorVm.measureDimensions.y.toFixed(1) : "--"; color: Theme.textSecondary; font.pixelSize: 9; font.family: "monospace" }
                            Text { text: "H:"; color: "#3b82f6"; font.pixelSize: 9; font.bold: true }
                            Text { text: root.editorVm ? root.editorVm.measureDimensions.z.toFixed(1) : "--"; color: Theme.textSecondary; font.pixelSize: 9; font.family: "monospace" }
                        }
                    }
                }
            }

            // -- Section 5: Slice Progress (from Sidebar.qml lines 398-411) --
            CollapsibleSection {
                id: sliceSection
                Layout.fillWidth: true
                title: qsTr("切片")
                iconText: "☰"
                expanded: true
                visible: root.editorVm ? root.editorVm.isSlicing() : false

                content: SliceProgress {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 120
                    editorVm: root.editorVm
                }
            }

            Item { Layout.preferredHeight: 20 }
        }
    }

    // -- Rename printer preset dialog --
    CxDialog {
        id: renamePrinterDialog
        dialogTitle: qsTr("Rename Preset")
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
