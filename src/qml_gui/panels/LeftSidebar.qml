pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OWzx.Models 1.0
import ".."
import "../controls"
import "../components"

Rectangle {
    id: root
    required property var editorVm
    required property var configVm
    // G-07 (v5.17): filament compatibility now comes from the NOTIFY-ful
    // configVm.filamentSlotCompatibility list property (item i =
    // isFilamentCompatible(filamentPresetForSlot(i))), so the row
    // decorations below re-evaluate through normal binding dependencies
    // instead of a manual stateChanged tick. slotIncompatible() at the root
    // maps the list, falling back to list[0] for out-of-range rows — the
    // same currentFilamentPreset_ fallback filamentPresetForSlot applies,
    // so the retired per-slot compatibility Q_INVOKABLE's semantics are
    // preserved exactly.
    property string processCategory: ""
    signal exportRequested()

    // Sidebar filament slot rows (upstream add_filament/delete_filament,
    // Plater.cpp:5380,5393): seeded once from the editor extruder count,
    // then driven by the filament title-bar buttons. Every change mirrors
    // into configVm.setExtruderCount (upstream
    // update_multi_material_filament_presets slot-vector resize) so the
    // per-slot preset vector keeps matching the visible rows.
    property int filamentRowCount: 1
    Component.onCompleted: {
        filamentRowCount = Math.max(1, root.editorVm ? root.editorVm.extruderCount : 1)
    }

    readonly property int targetSidebarWidth: 392
    readonly property color panelSurface: Theme.bgElevated
    readonly property color sectionSurface: Theme.bgHover
    readonly property color controlSurface: Theme.borderDefault
    readonly property color fieldSurface: Theme.chromePressed
    readonly property color dividerColor: Theme.bgPressed
    readonly property color mutedText: Theme.textSecondary

    property string paramsCurrentTab: "Quality"
    property string paramsSearchText: ""
    readonly property var paramsOptionModel: {
        if (!root.configVm) return null
        return root.configVm.printOptions
    }
    readonly property string paramsTier: "print"
    // Upstream six process pages incl. Speed
    // (Tab.cpp:2637,2772,2842,2914,2981,3036). Shared by the tab strip and
    // the one-persistent-page-per-tab host below.
    readonly property var paramsTabs: [
        { key: "Quality", label: qsTr("质量") },
        { key: "Strength", label: qsTr("强度") },
        { key: "Speed", label: qsTr("速度") },
        { key: "Support", label: qsTr("支撑") },
        { key: "Multimaterial", label: qsTr("材料") },
        { key: "Others", label: qsTr("其他") }
    ]

    color: panelSurface
    radius: 0
    border.width: 0

    // Sidebar layout contract (upstream Plater.cpp:2425-2429,3245,3257): the
    // sidebar itself does not scroll as one column. Printer/filament/process
    // sections stay fixed; the params area fills the remaining height and
    // scrolls on its own.
    ColumnLayout {
        id: sidebarContent
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        spacing: 8

        PixelHeader {
            id: printerHeader
            Layout.fillWidth: true
            title: qsTr("打印机")
            iconSource: "qrc:/qml/assets/icons/printer.svg"
            // Upstream printer title row carries connect/sync/settings
            // (Plater.cpp:2475-2495). The upstream monitor_signal_strong /
            // printer_sync_not bitmaps are absent from the project icon set,
            // so the registered send (same dialog as the row-level connect
            // button) and rotate (sync) glyphs stand in.
            extraActions: [
                { icon: "qrc:/qml/assets/icons/send-2.svg", tooltip: qsTr("打印机连接") },
                { icon: "qrc:/qml/assets/icons/rotate-2.svg", tooltip: qsTr("同步打印机信息") }
            ]
            onExtraActionTriggered: (index) => {
                if (index === 0)
                    backend.showPrintHostDialog()
                else if (index === 1 && backend.monitorViewModel)
                    backend.monitorViewModel.refresh()
            }
            actionIcon: "qrc:/qml/assets/icons/settings.svg"
            actionToolTip: qsTr("打印机设置")
            onActionTriggered: backend.forwardSettingsRequest("printer")
            // Whole-bar fold toggle (upstream Plater.cpp:2515-2525); while
            // folded the title carries the preset name (Plater.cpp:2518-2521).
            collapsible: true
            collapsedDetail: root.configVm ? root.configVm.currentPrinterPreset : ""
        }

        // v5.14: compact preset row replaces the 76px hero card (screenshot
        // truth shows preset selectors as dense single rows, matching the
        // filament/process rows below). Hidden while the printer title bar
        // is folded (upstream m_panel_printer_content toggle,
        // Plater.cpp:2515-2525).
        Rectangle {
            id: printerPresetRow
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            visible: printerHeader.expanded
            radius: Theme.radiusSM
            color: root.sectionSurface
            border.width: 1
            border.color: root.dividerColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                anchors.topMargin: 4
                anchors.bottomMargin: 4
                spacing: 6

                Image {
                    Layout.preferredWidth: 18
                    Layout.preferredHeight: 18
                    source: "qrc:/qml/assets/icons/printer.svg"
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.8
                }

                CxComboBox {
                    Layout.fillWidth: true
                    // Upstream preset combos are 30px tall
                    // (PresetComboBoxes.cpp:828, Plater.cpp:2595).
                    Layout.preferredHeight: 30
                    font.pixelSize: Theme.fontSizeSM
                    // v5.16 (PSET2-05): decorated list — section
                    // separators + incompatibility gray-out.
                    model: root.configVm ? root.configVm.decoratedPrinterPresetNames : []
                    currentIndex: {
                        if (!root.configVm) return -1
                        return root.configVm.decoratedPrinterPresetNames.indexOf(root.configVm.currentPrinterPreset)
                    }
                    onActivated: (i) => {
                        if (!root.configVm) return
                        if (i >= 0 && i < model.length)
                            root.configVm.requestCurrentPrinterPreset(
                                root.configVm.plainPresetName(model[i]))
                    }
                }

                Rectangle {
                    visible: !!root.configVm && root.configVm.isPresetDirty
                    Layout.preferredWidth: 8
                    Layout.preferredHeight: 8
                    radius: Theme.radiusSM
                    color: Theme.accent
                    ToolTip.text: qsTr("预设已修改（未保存）")
                    ToolTip.visible: printerDirtyMA.containsMouse
                    MouseArea { id: printerDirtyMA; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                }

                // v5.16 (PSET2-07): per-row preset edit affordance
                // (rename/delete, upstream preset combo right-click menu).
                // Row actions reveal on hover (upstream hides the printer
                // edit button until hover, Plater.cpp:2568-2570,2636-2646).
                PixelIconButton {
                    hoverShow: true
                    iconSource: "qrc:/qml/assets/icons/dots.svg"
                    toolTipText: qsTr("预设操作")
                    onClicked: root.editPreset(2, root.configVm ? root.configVm.currentPrinterPreset : "")
                }

                PixelIconButton {
                    hoverShow: true
                    iconSource: "qrc:/qml/assets/icons/settings.svg"
                    toolTipText: qsTr("编辑打印机预设")
                    onClicked: backend.forwardSettingsRequest("printer")
                }

                PixelIconButton {
                    hoverShow: true
                    iconSource: "qrc:/qml/assets/icons/send-2.svg"
                    toolTipText: qsTr("打印机连接")
                    onClicked: backend.showPrintHostDialog()
                }
            }
        }

        // Bed-type dropdown row under the printer preset row (reference
        // image). Upstream semantics: the printer section carries a
        // bed-surface card whose combo drives curr_bed_type
        // (Plater.cpp:2716-2786 reset_bed_type_combox_choices; the selection
        // handler at :12109-12140 writes the project config and invalidates
        // slice results). Q11-④: the combo is real — bound to the current
        // plate's curr_bed_type through the same EditorViewModel accessors
        // the plate-settings dialog uses: setPlateBedType syncs the plate
        // DynamicPrintConfig the slicer consumes
        // (ProjectServiceMock::setPlateBedType) and invalidates the plate's
        // slice result. Indices are the libslic3r BedType enum values
        // (PrintConfig.hpp:404-412; 0 = btDefault = follow the project/
        // preset default); the labels come from editorVm.bedTypeLabels —
        // the single source shared with PreparePage's plate-settings combo.
        // No local label copy: the previous hand tables were misaligned with
        // the enum order and carried a fabricated 8th "自定义" value.
        Rectangle {
            id: printerBedTypeRow
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            visible: printerHeader.expanded
            radius: Theme.radiusSM
            color: root.sectionSurface
            border.width: 1
            border.color: root.dividerColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                anchors.topMargin: 4
                anchors.bottomMargin: 4
                spacing: 6

                Image {
                    Layout.preferredWidth: 18
                    Layout.preferredHeight: 18
                    // Upstream bed card thumbnail is "printer_placeholder"
                    // (Plater.cpp:2727), absent from the project icon set;
                    // the registered bed param glyph stands in.
                    source: "qrc:/qml/assets/icons/param_bed_temp.svg"
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.8
                }

                Text {
                    text: qsTr("热床类型")
                    color: root.mutedText
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                }

                CxComboBox {
                    id: bedTypeCombo
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    font.pixelSize: Theme.fontSizeSM
                    // Q11-④: single label source indexed by the libslic3r
                    // BedType enum (contract on the EditorViewModel property).
                    model: root.editorVm ? root.editorVm.bedTypeLabels : []
                    enabled: root.editorVm && root.editorVm.currentPlateIndex >= 0
                    currentIndex: root.editorVm ? root.editorVm.currentPlateBedType : 0
                    onActivated: function(index) {
                        if (root.editorVm && root.editorVm.currentPlateIndex >= 0)
                            root.editorVm.setPlateBedType(root.editorVm.currentPlateIndex, index)
                    }
                    // Honest scope disclosure: this writes the current
                    // plate's curr_bed_type (the slice-effective value), not
                    // an app-wide override.
                    ToolTip.visible: bedTypeComboHover.hovered
                    ToolTip.text: qsTr("作用于当前盘的 curr_bed_type（切片实际消费）；0 = 跟随全局默认")
                    ToolTip.delay: 400
                    HoverHandler { id: bedTypeComboHover }
                }
            }
        }

        PixelHeader {
            id: filamentHeader
            Layout.fillWidth: true
            title: qsTr("耗材丝")
            // Upstream filament titlebar leads with the spool glyph
            // ("filament" ScalableButton, Plater.cpp:2913); spool.svg is
            // absent from the project set, so the registered spool-family
            // param icon stands in.
            iconSource: "qrc:/qml/assets/icons/param_filament_for_features.svg"
            // Upstream add/delete filament title-bar buttons, delete first
            // (Plater.cpp:2989-2990); delete hides while a single filament
            // remains (Plater.cpp:5395,2992).
            extraActions: [
                { icon: "qrc:/qml/assets/icons/minus.svg", tooltip: qsTr("删除最后一根耗材丝"),
                  visible: root.filamentRowCount > 1 },
                { icon: "qrc:/qml/assets/icons/plus.svg", tooltip: qsTr("添加一根耗材丝") }
            ]
            onExtraActionTriggered: (index) => {
                if (index === 0) {
                    if (root.filamentRowCount > 1) {
                        root.filamentRowCount -= 1
                        if (root.configVm)
                            root.configVm.setExtruderCount(root.filamentRowCount)
                    }
                } else if (index === 1) {
                    if (root.filamentRowCount < 64) { // MAXIMUM_EXTRUDER_NUMBER (libslic3r.h:65)
                        root.filamentRowCount += 1
                        if (root.configVm)
                            root.configVm.setExtruderCount(root.filamentRowCount)
                    }
                }
            }
            actionIcon: "qrc:/qml/assets/icons/settings.svg"
            actionToolTip: qsTr("耗材设置")
            onActionTriggered: backend.forwardSettingsRequest("filament")
            // Whole-bar fold toggle (upstream Plater.cpp:2892-2911).
            collapsible: true
        }

        Repeater {
            model: Math.max(1, root.filamentRowCount)

            delegate: Rectangle {
                id: filamentPixelRow
                required property int index
                Layout.fillWidth: true
                Layout.preferredHeight: 38
                // Folded with the filament section (upstream
                // m_filament_area_wrapper toggle, Plater.cpp:2906-2908).
                visible: filamentHeader.expanded
                radius: Theme.radiusSM
                color: root.sectionSurface
                border.width: 1
                border.color: root.slotIncompatible(filamentPixelRow.index)
                    ? Theme.statusError : root.dividerColor

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    anchors.topMargin: 4
                    anchors.bottomMargin: 4
                    spacing: 6

                    // Ref: the color chip carries the extruder number and sits
                    // flush against the preset combo (upstream labels the
                    // color picker with the slot number, Plater.cpp:3340-3342).
                    Rectangle {
                        Layout.preferredWidth: 22
                        Layout.preferredHeight: 22
                        radius: Theme.radiusSM
                        color: root.filamentColor(filamentPixelRow.index)
                        border.width: 1
                        border.color: Qt.lighter(root.filamentColor(filamentPixelRow.index), 1.25)

                        Text {
                            anchors.centerIn: parent
                            text: String(filamentPixelRow.index + 1)
                            color: Theme.textOnAccent
                            font.pixelSize: Theme.fontSizeSM
                            font.bold: true
                        }
                    }

                    CxComboBox {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        font.pixelSize: Theme.fontSizeSM
                        // v5.16 (PSET2-05): decorated list — section
                        // separators + incompatibility gray-out.
                        model: root.configVm ? root.configVm.decoratedFilamentPresetNames : []
                        currentIndex: {
                            if (!root.configVm) return -1
                            // v5.16 (CIRC-04): each slot shows ITS OWN preset
                            // (was: the category list's Nth entry for all slots).
                            // PSET2-05: compare against plain names since the
                            // list entries may carry the display suffix.
                            var slotPreset = root.configVm.filamentPresetForSlot(filamentPixelRow.index)
                            var names = root.configVm.decoratedFilamentPresetNames
                            for (var i = 0; i < names.length; i++) {
                                if (root.configVm.plainPresetName(names[i]) === slotPreset) return i
                            }
                            return -1
                        }
                        onActivated: (i) => {
                            if (!root.configVm) return
                            var names = root.configVm.decoratedFilamentPresetNames
                            if (i >= 0 && i < names.length)
                                root.configVm.requestFilamentPresetForSlot(
                                    filamentPixelRow.index,
                                    root.configVm.plainPresetName(names[i]))
                        }
                    }

                    Rectangle {
                        visible: !!root.configVm
                                 && root.slotIncompatible(filamentPixelRow.index)
                        Layout.preferredWidth: 8
                        Layout.preferredHeight: 8
                        radius: Theme.radiusSM
                        color: Theme.statusError
                        ToolTip.text: root.configVm ? root.configVm.currentPresetCompatibilityMessage : ""
                        ToolTip.visible: filamentCompatMA.containsMouse
                        MouseArea { id: filamentCompatMA; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    }

                    // Ref/upstream row tail keeps a single edit button
                    // (Plater.cpp:3358,3382 "menu_filament" edit button).
                    // The upstream menu_filament bitmap is absent from the
                    // project set; the dots glyph (same affordance as the
                    // printer row's preset menu) stands in.
                    PixelIconButton {
                        iconSource: "qrc:/qml/assets/icons/dots.svg"
                        toolTipText: qsTr("点击编辑预设")
                        onClicked: root.editPreset(1, root.configVm
                            ? root.configVm.filamentPresetForSlot(filamentPixelRow.index) : "")
                    }
                }
            }
        }

        PixelHeader {
            Layout.fillWidth: true
            title: qsTr("工艺")
            iconSource: "qrc:/qml/assets/icons/list-details.svg"
            // Ref/upstream: Global|Objects scope pills sit inline in the
            // process title row (ParamsPanel.cpp:265-267,417).
            middleComponent: processScopePills
            actionIcon: "qrc:/qml/assets/icons/settings.svg"
            actionToolTip: qsTr("工艺设置")
            onActionTriggered: backend.forwardSettingsRequest("process")
        }

        Rectangle {
            id: processPresetRow
            Layout.fillWidth: true
            Layout.preferredHeight: 38
            radius: Theme.radiusSM
            color: root.sectionSurface
            border.width: 1
            border.color: root.dividerColor

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                anchors.topMargin: 4
                anchors.bottomMargin: 4
                spacing: 6

                CxComboBox {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 30
                    font.pixelSize: Theme.fontSizeSM
                    // v5.16 (PSET2-05): decorated list — section
                    // separators + incompatibility gray-out.
                    model: root.configVm ? root.configVm.decoratedPrintPresetNames : []
                    currentIndex: {
                        if (!root.configVm) return -1
                        return root.configVm.decoratedPrintPresetNames.indexOf(root.configVm.currentPrintPreset)
                    }
                    onActivated: (i) => {
                        if (root.configVm && i >= 0)
                            root.configVm.requestCurrentPrintPreset(
                                root.configVm.plainPresetName(model[i]))
                    }
                }

                Rectangle {
                    visible: !!root.configVm && root.configVm.isPresetDirty
                    Layout.preferredWidth: 8
                    Layout.preferredHeight: 8
                    radius: Theme.radiusSM
                    color: Theme.accent
                    ToolTip.text: qsTr("预设已修改（未保存）")
                    ToolTip.visible: processDirtyMA.containsMouse
                    MouseArea { id: processDirtyMA; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                }

                // Ref/upstream process preset row tail: copy + search icon
                // pair; search opens the find dialog instead of a permanent
                // search field. The upstream copy_menu bitmap is absent from
                // the project set; the copy glyph stands in.
                PixelIconButton {
                    iconSource: "qrc:/qml/assets/icons/copy.svg"
                    iconSize: 16
                    toolTipText: qsTr("预设操作")
                    onClicked: root.editPreset(0, root.configVm ? root.configVm.currentPrintPreset : "")
                }

                PixelIconButton {
                    iconSource: "qrc:/qml/assets/icons/search.svg"
                    iconSize: 16
                    toolTipText: qsTr("搜索设置")
                    onClicked: root.openParamsSearch()
                }
            }
        }

        // ── U14 (Prepare 页信息密度)：常驻"切片信息"卡 ─────────────────────
        // 上游 Plater 侧栏 SlicedInfo 对齐（third_party/OrcaSlicer
        // Plater.cpp:358-414）：侧栏常驻盒子，切片完成后回填；每行初值 "N/A"，
        // SetTextAndShow(:403-412) 对仍无值的行整行隐藏。本 fork 的这批数值
        // 一直存在于 EditorViewModel（sliceEstimatedTime / sliceResultCost /
        // sliceResultFilament / sliceResultWeight，全部按当前盘有效结果门控），
        // 但此前只有预览页 StatsPanel 可见——现在侧栏常驻，Prepare 页无需切页
        // 即可读数。对象数是场景态（非切片派生），首片前也显示；切片派生各行
        // 跟随各自 getter（当前盘无有效结果/被编辑置 stale 时返回空串 → 整行
        // 隐藏），与上游"切片成功才显示、无值行隐藏"语义一致。数值沿用 U12 的
        // 只读 TextEdit 可选中样式，便于抄录。
        Rectangle {
            id: slicedInfoCard
            Layout.fillWidth: true
            visible: root.editorVm
                     && (root.editorVm.modelCount > 0 || root.editorVm.hasSliceResult)
            radius: Theme.radiusSM
            color: root.sectionSurface
            border.width: 1
            border.color: root.dividerColor
            implicitHeight: slicedInfoLayout.implicitHeight + 12

            ColumnLayout {
                id: slicedInfoLayout
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 6
                spacing: 3

                Text {
                    text: qsTr("切片信息")
                    color: root.mutedText
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                }

                SlicedInfoRow {
                    label: qsTr("对象")
                    value: root.editorVm ? String(root.editorVm.modelCount) : "0"
                }
                SlicedInfoRow {
                    label: qsTr("预计耗时")
                    value: root.editorVm ? root.editorVm.sliceEstimatedTime : ""
                }
                SlicedInfoRow {
                    label: qsTr("总成本")
                    value: root.editorVm ? root.editorVm.sliceResultCost : ""
                }
                SlicedInfoRow {
                    label: qsTr("耗材")
                    value: root.editorVm ? root.editorVm.sliceResultFilament : ""
                }
                SlicedInfoRow {
                    label: qsTr("重量")
                    value: root.editorVm ? root.editorVm.sliceResultWeight : ""
                }
            }
        }

        // Params area: full-height container outlined by a 1px accent border
        // (ref layout), filling the sidebar remainder; only the option list
        // scrolls (params-1/layout-6).
        Rectangle {
            id: paramsInlinePanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 0
            color: "transparent"
            border.width: 1
            border.color: Theme.accent

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingXS
                spacing: 4

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.spacingXL

                    Repeater {
                        model: root.paramsTabs
                        delegate: Item {
                            id: paramsTabDelegate
                            required property var modelData
                            readonly property bool selected: paramsTabDelegate.modelData.key === root.paramsCurrentTab
                            implicitWidth: paramsTabLabel.implicitWidth + 8
                            implicitHeight: 24

                            Text {
                                id: paramsTabLabel
                                anchors.centerIn: parent
                                text: paramsTabDelegate.modelData.label
                                color: paramsTabDelegate.selected ? Theme.accent : Theme.textTertiary
                                font.pixelSize: Theme.fontSizeLG
                                font.bold: paramsTabDelegate.selected
                            }

                            // 3px selected underline (TabCtrl.cpp:346-351).
                            Rectangle {
                                visible: paramsTabDelegate.selected
                                anchors.bottom: parent.bottom
                                width: parent.width
                                height: 3
                                color: Theme.accent
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                // Tab flips only change visibility — the
                                // per-page proxy inputs (page/searchText) are
                                // untouched, so no re-filtering happens.
                                onClicked: root.paramsCurrentTab = paramsTabDelegate.modelData.key
                            }
                        }
                    }
                }

                // 1px full-width baseline under the text tab strip
                // (TabCtrl.cpp:346-351).
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 1
                    color: Theme.bgPanel
                }

                // One persistent page per process tab (upstream Tab.cpp keeps
                // a widget tree per page and only shows/hides it — pages are
                // built once, never rebuilt on a switch). Each page loads on
                // its first visit; a flip to an already-built page is a
                // visibility change only, so repeated switching stays free.
                Item {
                    id: paramsPagesHost
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    // Prewarm: once the panel has painted, build the six
                    // pages one per tick so the first click on any tab is a
                    // flip instead of a ~350ms delegate build. A click racing
                    // the prewarm still builds its own page on visit.
                    property int prewarmIdx: 0
                    Timer {
                        id: prewarmDelay
                        interval: 1200
                        onTriggered: prewarmStep.restart()
                    }
                    Timer {
                        id: prewarmStep
                        // Step wider than one page build (~550ms with the
                        // cacheBuffer rows) so a click landing mid-prewarm
                        // queues behind at most one build.
                        interval: 900
                        repeat: true
                        triggeredOnStart: true
                        onTriggered: {
                            if (paramsPagesHost.prewarmIdx >= paramsPagesRepeater.count) {
                                stop()
                                return
                            }
                            var page = paramsPagesRepeater.itemAt(paramsPagesHost.prewarmIdx)
                            if (page) page.visited = true
                            paramsPagesHost.prewarmIdx = paramsPagesHost.prewarmIdx + 1
                        }
                    }
                    Component.onCompleted: prewarmDelay.restart()

                    Repeater {
                        id: paramsPagesRepeater
                        model: root.paramsTabs
                        delegate: Item {
                            id: paramsPageHost
                            required property var modelData
                            readonly property string pageKey: paramsPageHost.modelData.key
                            readonly property bool isCurrent: root.paramsCurrentTab === paramsPageHost.pageKey
                            property bool visited: paramsPageHost.isCurrent
                            onIsCurrentChanged: if (paramsPageHost.isCurrent) paramsPageHost.visited = true
                            width: parent.width
                            height: parent.height
                            // QQuickItemView releases every delegate when the
                            // view's effective visibility drops, so a
                            // visible=false flip silently destroyed and
                            // rebuilt the incoming page's rows on every
                            // switch. Keep pages effectively visible and
                            // shift the non-current ones out of the window
                            // instead: the ListView's own clip keeps them
                            // culled and unhittable, so a flip is a pure
                            // reposition and revisits reuse the live rows.
                            x: paramsPageHost.isCurrent ? 0 : -100000

                            Loader {
                                anchors.fill: parent
                                active: paramsPageHost.visited
                                sourceComponent: ListView {
                                    id: paramsPage
                                    anchors.fill: parent
                                    clip: true
                                    spacing: 0
                                    // cacheBuffer 0 was a stopgap from when a
                                    // tab flip rebuilt every delegate; pages
                                    // are persistent now, so pre-building rows
                                    // past the viewport only costs prewarm
                                    // time and keeps fast scrolling from
                                    // building rows mid-flick.
                                    cacheBuffer: 600
                                    // Per-page C++ filter proxy (replaces the
                                    // QML-side index-array filter
                                    // orchestration). LeftSidebar is always
                                    // advanced mode. Tab flips only toggle
                                    // this page's visibility — no proxy input
                                    // changes, so no re-filtering, and value
                                    // dataChanged now refreshes visible rows
                                    // in place.
                                    model: ConfigOptionFilterProxy {
                                        sourceModel: root.paramsOptionModel
                                        page: paramsPageHost.pageKey
                                        searchText: root.paramsSearchText
                                        advancedMode: true
                                    }

                                    ScrollBar.vertical: ScrollBar {
                                        visible: paramsPage.contentHeight > paramsPage.height
                                    }

                                    delegate: Item {
                                        id: paramsDelegate
                                        required property int index
                                        required property int optIdx
                                        required property string optGroup
                                        required property string optPrevGroup
                                        required property string optType
                                        required property string optKey
                                        required property string displayLabel
                                        required property var optValue
                                        required property double optMin
                                        required property double optMax
                                        required property double optStep
                                        required property bool optReadonly
                                        required property bool optDirty
                                        required property string optTooltip
                                        required property string optUnit
                                        required property string optSidetext
                                        required property bool optNullable
                                        required property bool optIsVector
                                        required property var optEnumLabels
                                        required property string valueSource

                                        // optPrevGroup is the proxy-fed
                                        // GroupRole of the previous *accepted*
                                        // row; indexing neighbors by source
                                        // row is wrong under a filtered proxy.
                                        readonly property bool showGroupHeader:
                                            optGroup !== ""
                                            && (index === 0 || optGroup !== optPrevGroup)

                                        width: paramsPage.width
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
                                            compactLabelWidth: 150
                                            compactFieldWidth: 86
                                            compactEnumWidth: 132
                                            valueSource: paramsDelegate.valueSource
                                            oType: paramsDelegate.optType
                                            oKey: paramsDelegate.optKey
                                            oLabel: paramsDelegate.displayLabel
                                            oVal: paramsDelegate.optValue
                                            oMin: paramsDelegate.optMin
                                            oMax: paramsDelegate.optMax
                                            oStep: paramsDelegate.optStep
                                            oRO: paramsDelegate.optReadonly
                                            oDirty: paramsDelegate.optDirty
                                            oTip: paramsDelegate.optTooltip
                                            oUnit: paramsDelegate.optUnit
                                            oSidetext: paramsDelegate.optSidetext
                                            oNullable: paramsDelegate.optNullable
                                            oIsVector: paramsDelegate.optIsVector
                                            oEnumLabels: paramsDelegate.optEnumLabels
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

    // ── preset search dialog (sidebar-9/layout-8) ──────────────────────────
    // Upstream keeps the sidebar search bar hidden (Plater.cpp:3246-3247);
    // searching happens through a dialog instead of a permanent field.
    function openParamsSearch() {
        paramsSearchDialog.open()
    }

    CxDialog {
        id: paramsSearchDialog
        modal: true
        dialogTitle: qsTr("搜索设置")
        width: 380
        height: 160
        padding: 0

        onAboutToShow: {
            searchDialogField.text = root.paramsSearchText
            searchDialogField.selectAll()
            searchDialogField.forceActiveFocus()
        }

        contentItem: Rectangle {
            color: Theme.bgPanel
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingXL
                spacing: Theme.spacingMD

                CxTextField {
                    id: searchDialogField
                    Layout.fillWidth: true
                    implicitHeight: 28
                    font.pixelSize: Theme.fontSizeSM
                    placeholderText: qsTr("搜索设置...")
                    onTextChanged: {
                        if (root.paramsSearchText !== text.trim())
                            root.paramsSearchText = text.trim()
                    }
                    onAccepted: paramsSearchDialog.accept()
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("筛选结果实时生效，关闭对话框后仍保持筛选。")
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeXS
                    wrapMode: Text.WordWrap
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: Theme.spacingMD
                    CxButton {
                        text: qsTr("清除")
                        onClicked: {
                            searchDialogField.text = ""
                            root.paramsSearchText = ""
                        }
                    }
                    CxButton {
                        text: qsTr("关闭")
                        cxStyle: CxButton.Style.Primary
                        onClicked: paramsSearchDialog.accept()
                    }
                }
            }
        }
    }

    // ── v5.16 (PSET2-07): preset rename/delete affordance ────────────────
    // Upstream exposes rename/delete on each preset combo's context menu;
    // here each preset row's edit button opens the same actions.
    property int presetEditCategory: -1
    property string presetEditName: ""

    function editPreset(category, currentName) {
        if (!root.configVm || !currentName || currentName.length === 0)
            return
        root.presetEditCategory = category
        root.presetEditName = currentName
        presetEditMenu.popup()
    }

    CxMenu {
        id: presetEditMenu

        CxMenuItem {
            text: qsTr("重命名…")
            onTriggered: {
                renamePresetField.text = root.presetEditName
                renamePresetError.visible = false
                renamePresetDialog.open()
            }
        }
        CxMenuItem {
            text: qsTr("删除…")
            onTriggered: {
                // canDeletePreset guards read-only/built-ins (service-side
                // delete would reject them too); in-use presets warn first.
                if (!root.configVm)
                    return
                if (!root.configVm.canDeletePreset(root.presetEditName)) {
                    backend.postError(root.configVm.presetActionBlocker(
                        root.presetEditCategory, root.presetEditName, "delete"), 1)
                    return
                }
                deleteConfirmInUse.visible = root.configVm.isPresetInUse(root.presetEditName)
                deletePresetDialog.open()
            }
        }
    }

    // Inline rename dialog (upstream SavePresetDialog rename path).
    CxDialog {
        id: renamePresetDialog
        modal: true
        dialogTitle: qsTr("重命名预设")
        width: 380
        height: 160
        padding: 0

        onAccepted: {
            if (!root.configVm)
                return
            const newName = renamePresetField.text.trim()
            if (newName.length === 0 || newName === root.presetEditName)
                return
            if (!root.configVm.renamePreset(root.presetEditCategory, root.presetEditName, newName)) {
                renamePresetError.visible = true
                renamePresetDialog.open()
            } else {
                root.presetEditName = newName
            }
        }

        contentItem: Rectangle {
            color: Theme.bgPanel
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingXL
                spacing: Theme.spacingMD

                Text {
                    text: root.presetEditName
                    color: Theme.textSecondary
                    font.pixelSize: Theme.fontSizeSM
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                CxTextField {
                    id: renamePresetField
                    Layout.fillWidth: true
                    implicitHeight: 28
                    font.pixelSize: Theme.fontSizeSM
                    placeholderText: qsTr("输入新的预设名称")
                    onAccepted: renamePresetDialog.accept()
                }

                Text {
                    id: renamePresetError
                    text: qsTr("重命名失败（名称为空、重复或内置预设）")
                    color: Theme.statusError
                    font.pixelSize: Theme.fontSizeXS
                    visible: false
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: Theme.spacingMD
                    CxButton {
                        text: qsTr("取消")
                        onClicked: renamePresetDialog.reject()
                    }
                    CxButton {
                        text: qsTr("确定")
                        cxStyle: CxButton.Style.Primary
                        onClicked: renamePresetDialog.accept()
                    }
                }
            }
        }
    }

    // Delete confirmation (upstream deletes the selected user preset and
    // falls the selection back to the category default).
    CxDialog {
        id: deletePresetDialog
        modal: true
        dialogTitle: qsTr("删除预设")
        width: 380
        height: 170
        padding: 0

        onAccepted: {
            if (root.configVm)
                root.configVm.deletePreset(root.presetEditCategory, root.presetEditName)
        }

        contentItem: Rectangle {
            color: Theme.bgPanel
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.spacingXL
                spacing: Theme.spacingMD

                Text {
                    Layout.fillWidth: true
                    text: qsTr("确定删除预设 “%1”？").arg(root.presetEditName)
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeMD
                    wrapMode: Text.WordWrap
                }

                Text {
                    id: deleteConfirmInUse
                    Layout.fillWidth: true
                    text: qsTr("该预设正在使用，删除后将切换回默认预设。")
                    color: Theme.statusWarning
                    font.pixelSize: Theme.fontSizeXS
                    wrapMode: Text.WordWrap
                    visible: false
                }

                Item { Layout.fillHeight: true }

                RowLayout {
                    Layout.alignment: Qt.AlignRight
                    spacing: Theme.spacingMD
                    CxButton {
                        text: qsTr("取消")
                        onClicked: deletePresetDialog.reject()
                    }
                    CxButton {
                        text: qsTr("删除")
                        cxStyle: CxButton.Style.Primary
                        onClicked: deletePresetDialog.accept()
                    }
                }
            }
        }
    }

    // Scope pills live inside the process PixelHeader row (sidebar-5).
    // BUILDGATE restore (2026-09-24): the plate pill is back — PREPSB-04
    // (leftSidebarPresetControlsAreWiredAndHonest) locks the Global/Object/
    // Plate scope set, and requestPlateScope stays on ConfigViewModel.
    Component {
        id: processScopePills
        RowLayout {
            id: processScopeBar
            spacing: 6

            PixelSegment {
                text: qsTr("全局")
                selected: root.configVm && root.configVm.settingsScope === "global"
                onClicked: if (root.configVm) root.configVm.requestGlobalScope()
            }

            PixelSegment {
                text: qsTr("对象")
                selected: root.configVm && root.configVm.settingsScope !== "global" && root.configVm.settingsScope !== "plate"
                onClicked: {
                    if (root.editorVm && root.editorVm.selectedObjectIndex >= 0 && root.configVm) {
                        root.configVm.requestObjectScope("object", "",
                            root.editorVm.selectedObjectIndex, -1)
                    }
                }
            }

            PixelSegment {
                text: qsTr("盘")
                selected: root.configVm && root.configVm.settingsScope === "plate"
                enabled: root.editorVm && root.editorVm.currentPlateIndex >= 0
                onClicked: {
                    if (root.editorVm && root.editorVm.currentPlateIndex >= 0 && root.configVm)
                        root.configVm.requestPlateScope(root.editorVm.currentPlateIndex)
                }
            }
        }
    }

    // Filament slot compatibility (G-07): index into configVm
    // .filamentSlotCompatibility; rows beyond the list fall back to list[0],
    // mirroring filamentPresetForSlot's out-of-range fallback to
    // currentFilamentPreset_ (an out-of-range slot judges identically to
    // slot 0).
    // Property reads inside the function are still tracked by the calling
    // bindings, so filamentSlotCompatibilityChanged re-evaluates them.
    function slotIncompatible(i) {
        var list = root.configVm ? root.configVm.filamentSlotCompatibility : []
        return list.length > i ? list[i] === 0 : (list.length > 0 ? list[0] === 0 : false)
    }

    function filamentColor(index) {
        // v5.16 (CIRC-04): configured filament colours (same source as the
        // MMU paint palette) instead of a hardcoded theme palette.
        if (root.editorVm && root.editorVm.extrudersColors
            && index < root.editorVm.extrudersColors.length)
            return root.editorVm.extrudersColors[index]
        var colors = [Theme.statusWarning, Theme.textSecondary, Theme.textSecondary, Theme.statusInfo, Theme.chromeDangerHover]
        return index < colors.length ? colors[index] : Theme.textSecondary
    }

    // Section header: 30px title bar (upstream filament title is
    // SetMinSize(-1, FromDIP(30)), Plater.cpp:2915, printer title 3*em) with
    // leading icon + title, optional inline middle content, then a
    // right-aligned icon group (sidebar-3). Collapsible headers carry the
    // upstream title-bar band (SetBackgroundColor(title_bg) #F8F8F8,
    // Plater.cpp:2463,2891, dark-adjusted) and a 2px bottom separator shown
    // while expanded (Plater.cpp:2527-2529).
    component PixelHeader: Item {
        id: headerRoot
        property string title: ""
        property url iconSource: ""
        property var extraActions: []
        property Component middleComponent: null
        property url actionIcon: ""
        property string actionToolTip: ""
        property bool collapsible: false
        property bool expanded: true
        // Extra title detail shown while folded ("title | detail", upstream
        // printer title swap, Plater.cpp:2518-2521).
        property string collapsedDetail: ""
        signal extraActionTriggered(int index)
        signal actionTriggered()

        implicitHeight: 30

        // Title-bar band.
        Rectangle {
            anchors.fill: parent
            color: root.sectionSurface
        }

        // 2px bottom separator; hidden together with the content while the
        // section is folded (upstream separator->Show(isShown),
        // Plater.cpp:2527-2529).
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 2
            color: root.dividerColor
            visible: headerRoot.collapsible && headerRoot.expanded
        }

        // Whole-bar fold toggle. Declared before the row so the icon buttons
        // above it keep their own clicks (upstream excludes the button zone
        // from the toggle, Plater.cpp:2897-2905).
        MouseArea {
            anchors.fill: parent
            enabled: headerRoot.collapsible
            cursorShape: Qt.PointingHandCursor
            onClicked: headerRoot.expanded = !headerRoot.expanded
        }

        RowLayout {
            anchors.fill: parent
            spacing: 6

            Image {
                visible: headerRoot.iconSource !== ""
                source: headerRoot.iconSource
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                fillMode: Image.PreserveAspectFit
                opacity: 0.85
            }

            Text {
                text: {
                    if (!headerRoot.collapsible || headerRoot.expanded
                        || headerRoot.collapsedDetail.length === 0)
                        return headerRoot.title
                    return headerRoot.title + "  |  " + headerRoot.collapsedDetail
                }
                color: Theme.textPrimary
                font.pixelSize: Theme.fontSizeLG
                font.bold: true
                elide: Text.ElideRight
            }

            Item { Layout.fillWidth: true }

            Loader {
                sourceComponent: headerRoot.middleComponent
                Layout.alignment: Qt.AlignVCenter
            }

            Repeater {
                model: headerRoot.extraActions
                delegate: PixelIconButton {
                    required property int index
                    required property var modelData
                    visible: modelData.visible !== false
                    iconSource: modelData.icon
                    toolTipText: modelData.tooltip
                    onClicked: headerRoot.extraActionTriggered(index)
                }
            }

            PixelIconButton {
                visible: headerRoot.actionIcon !== ""
                iconSource: headerRoot.actionIcon
                toolTipText: headerRoot.actionToolTip
                enabled: headerRoot.enabled
                onClicked: headerRoot.actionTriggered()
            }
        }
    }

    component PixelIconButton: Rectangle {
        id: iconButtonRoot
        property url iconSource: ""
        property string toolTipText: ""
        property bool hoverShow: false
        property int buttonSize: 24
        property int iconSize: 13
        signal clicked()

        Layout.preferredWidth: iconButtonRoot.buttonSize
        Layout.preferredHeight: iconButtonRoot.buttonSize
        implicitWidth: iconButtonRoot.buttonSize
        implicitHeight: iconButtonRoot.buttonSize
        radius: Theme.radiusSM
        color: !enabled ? root.fieldSurface
              : iconMA.containsMouse ? Theme.bgPressed
              : root.controlSurface
        border.width: 1
        border.color: root.dividerColor
        // hoverShow mirrors upstream printer edit buttons that appear only
        // while the row is hovered (Plater.cpp:2636-2646); space stays
        // reserved so the row layout does not jump.
        opacity: !enabled ? 0.45
              : hoverShow ? (hoverHandler.hovered ? 1.0 : 0.0)
              : 1.0
        Behavior on opacity { NumberAnimation { duration: Theme.motionFast } }

        HoverHandler {
            id: hoverHandler
            enabled: iconButtonRoot.hoverShow
        }

        Image {
            anchors.centerIn: parent
            width: iconButtonRoot.iconSize
            height: iconButtonRoot.iconSize
            source: iconButtonRoot.iconSource
            fillMode: Image.PreserveAspectFit
            opacity: 0.8
        }

        MouseArea {
            id: iconMA
            anchors.fill: parent
            hoverEnabled: true
            enabled: iconButtonRoot.enabled && (!iconButtonRoot.hoverShow || hoverHandler.hovered)
            cursorShape: Qt.PointingHandCursor
            onClicked: iconButtonRoot.clicked()
        }

        ToolTip.visible: iconMA.containsMouse && iconButtonRoot.toolTipText.length > 0
        ToolTip.text: iconButtonRoot.toolTipText
        ToolTip.delay: 400
    }

    // Inline scope pill (ref: small Global/Objects pills inside the title
    // row, selected = accent fill + white text).
    component PixelSegment: Rectangle {
        id: segmentRoot
        property string text: ""
        property bool selected: false
        signal clicked()

        implicitWidth: segmentLabel.implicitWidth + 20
        implicitHeight: 22
        radius: Theme.radiusXL
        color: segmentRoot.selected ? Theme.accent : root.sectionSurface
        border.width: 1
        border.color: segmentRoot.selected ? Theme.accent : root.dividerColor
        opacity: enabled ? 1.0 : 0.45

        Text {
            id: segmentLabel
            anchors.centerIn: parent
            text: segmentRoot.text
            color: segmentRoot.selected ? Theme.textOnAccent : root.mutedText
            font.pixelSize: Theme.fontSizeSM
            font.bold: segmentRoot.selected
        }

        MouseArea {
            anchors.fill: parent
            enabled: segmentRoot.enabled
            cursorShape: Qt.PointingHandCursor
            onClicked: segmentRoot.clicked()
        }
    }

    // U14: one label/value pair of the persistent Sliced Info card. The row
    // hides itself when the value is empty -- the EditorViewModel slice-stat
    // getters return QString{} when the current plate has no valid (non-stale)
    // slice result, which reproduces upstream SlicedInfo::SetTextAndShow's
    // "N/A rows stay hidden" (Plater.cpp:403-412). Value cell is a read-only
    // selectable TextEdit (U12 pattern, Theme selection tokens).
    component SlicedInfoRow: RowLayout {
        id: slicedInfoRowRoot
        property string label: ""
        property string value: ""

        visible: slicedInfoRowRoot.value.length > 0
        Layout.fillWidth: true
        spacing: 8

        Text {
            text: slicedInfoRowRoot.label
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeXS
        }
        Item { Layout.fillWidth: true }
        TextEdit {
            text: slicedInfoRowRoot.value
            readOnly: true
            selectByMouse: true
            color: Theme.textPrimary
            selectionColor: Theme.selectionColor
            selectedTextColor: Theme.selectionText
            font.pixelSize: Theme.fontSizeSM
            font.family: Theme.fontMono
            wrapMode: TextEdit.NoWrap
        }
    }
}
