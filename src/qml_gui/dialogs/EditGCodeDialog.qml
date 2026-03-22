import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.3 -- EditGCodeDialog: G-code editor with placeholder insertion
// Aligns with upstream EditGCodeDialog (wxDataViewCtrl + wxTextCtrl + search)
// Layout: Left panel (parameter list) | Center panel (monospace TextArea)
// Usage: EditGCodeDialog { id: dlg } -> dlg.open()

Dialog {
    id: root

    modal: true
    closePolicy: Popup.NoAutoClose
    anchors.centerIn: parent
    width: 620
    height: 400

    // Public API -- set before opening
    property string initialGCode: ""
    property string dialogTitle: qsTr("编辑自定义 G-code")
    signal gcodeAccepted(string gcode)

    // Internal state
    property string selectedParamCode: ""
    property string selectedParamLabel: ""
    property string selectedParamDesc: ""

    background: Rectangle {
        color: "#1a1f28"
        radius: 8
        border.color: "#2e3848"
        border.width: 1
    }

    header: Rectangle {
        width: parent.width
        height: 40
        color: "#141920"
        radius: 8
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 12
            color: parent.color
        }

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            text: root.dialogTitle
            color: "#e2e8f5"
            font.pixelSize: 14
            font.bold: true
        }
    }

    contentItem: ColumnLayout {
        spacing: 0
        anchors.fill: parent
        anchors.margins: 0

        // Top label (对齐上游 "Built-in placeholders (Double click item to add to G-code)")
        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.topMargin: 8
            Layout.bottomMargin: 4
            text: qsTr("内置占位符（双击项添加到 G-code）：")
            color: Theme.textSecondary
            font.pixelSize: 11
            wrapMode: Text.Wrap
        }

        // Main 3-column area: param list | add btn | editor
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: 8
            spacing: 0

            // -- Left panel: parameter list --
            ColumnLayout {
                Layout.preferredWidth: 180
                Layout.fillHeight: true
                spacing: 6

                // Search bar
                CxTextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("搜索 G-code 占位符...")
                    onTextChanged: paramProxyModel.updateFilter(text)
                }

                // Parameter tree view (Listview with grouped expandable categories)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Theme.bgInset
                    radius: 4
                    border.color: Theme.borderSubtle
                    border.width: 1
                    clip: true

                    ListView {
                        id: paramListView
                        anchors.fill: parent
                        anchors.margins: 2
                        model: paramProxyModel
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: Rectangle {
                            id: delegateRoot
                            required property int index
                            required property var modelData
                            width: paramListView.width
                            height: modelData.isCategory ? 26 : 22
                            color: {
                                if (modelData.isCategory) return "transparent"
                                if (paramListView.currentIndex === index) return Theme.accentSubtle
                                if (delegateMa.containsMouse) return Theme.bgHover
                                return "transparent"
                            }
                            visible: modelData.visible !== false

                            // Category header row
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 6
                                anchors.rightMargin: 6
                                spacing: 4
                                visible: modelData.isCategory

                                // Expand/collapse arrow
                                Text {
                                    text: modelData.expanded ? "\u25BC" : "\u25B6"
                                    color: Theme.textTertiary
                                    font.pixelSize: 8
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.label
                                    color: Theme.textPrimary
                                    font.pixelSize: 11
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                            }

                            // Parameter item row
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: modelData.isCategory ? 0 : 18
                                anchors.rightMargin: 6
                                spacing: 4
                                visible: !modelData.isCategory

                                // Type indicator icon (scalar vs vector)
                                Rectangle {
                                    width: 14
                                    height: 14
                                    radius: 2
                                    color: modelData.isVector ? "#1a3a5c" : "#1a3a2c"
                                    border.color: modelData.isVector ? "#3b9eff" : Theme.accent
                                    border.width: 1

                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData.isVector ? "[]" : "S"
                                        color: modelData.isVector ? "#3b9eff" : Theme.accent
                                        font.pixelSize: 7
                                        font.bold: true
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.label
                                    color: paramListView.currentIndex === index ? Theme.textPrimary : Theme.textSecondary
                                    font.pixelSize: 11
                                    font.family: "Consolas, Monaco, monospace"
                                    elide: Text.ElideRight
                                }
                            }

                            MouseArea {
                                id: delegateMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor

                                onClicked: {
                                    if (modelData.isCategory) {
                                        paramModel.toggleCategory(modelData.categoryId)
                                    } else {
                                        paramListView.currentIndex = index
                                        root.selectedParamCode = modelData.code
                                        root.selectedParamLabel = modelData.label
                                        root.selectedParamDesc = modelData.desc || ""
                                    }
                                }

                                onDoubleClicked: {
                                    if (!modelData.isCategory) {
                                        insertSelectedParam()
                                    }
                                }
                            }
                        }

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }
                    }
                }

                // Parameter description area (对齐上游 m_param_label + m_param_description)
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        Layout.fillWidth: true
                        id: paramLabelDisplay
                        text: root.selectedParamLabel
                            ? root.selectedParamLabel + qsTr("（双击添加）")
                            : qsTr("选择占位符")
                        color: Theme.textPrimary
                        font.pixelSize: 11
                        font.bold: !!root.selectedParamLabel
                        elide: Text.ElideRight
                        wrapMode: Text.Wrap
                        Layout.maximumHeight: 30
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.selectedParamDesc
                        color: Theme.textTertiary
                        font.pixelSize: 10
                        wrapMode: Text.Wrap
                        Layout.maximumHeight: 36
                        visible: !!root.selectedParamDesc
                    }
                }
            }

            // -- Add button (vertical, centered between list and editor) --
            CxButton {
                Layout.alignment: Qt.AlignVCenter
                Layout.leftMargin: 8
                Layout.rightMargin: 8
                text: qsTr("添加")
                compact: true
                enabled: !!root.selectedParamCode
                cxStyle: CxButton.Style.Primary
                onClicked: insertSelectedParam()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("将选中的占位符添加到 G-code")
            }

            // -- Center panel: G-code text editor --
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#0d1017"
                radius: 4
                border.color: gcodeEditor.activeFocus ? Theme.borderFocus : Theme.borderSubtle
                border.width: 1

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true

                    TextArea {
                        id: gcodeEditor
                        width: parent.width
                        height: parent.height
                        placeholderText: qsTr("在此编辑自定义 G-code...")
                        text: root.initialGCode
                        wrapMode: TextArea.Wrap
                        font.family: "Consolas, Monaco, 'Courier New', monospace"
                        font.pixelSize: 12
                        color: Theme.textPrimary
                        selectionColor: "#1a5c3a"
                        selectedTextColor: Theme.textPrimary
                        placeholderTextColor: Theme.textDisabled
                        background: null

                        // Persistent selection tracking
                        property int _selStart: 0
                        property int _selEnd: 0

                        onCursorRectangleChanged: {
                            _selStart = selectionStart
                            _selEnd = selectionEnd
                        }
                    }
                }
            }
        }
    }

    footer: Rectangle {
        width: parent.width
        height: 48
        color: "#141920"
        radius: 8
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 12
            color: parent.color
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 10

            Item { Layout.fillWidth: true }

            CxButton {
                text: qsTr("清除全部")
                cxStyle: CxButton.Style.Ghost
                onClicked: {
                    gcodeEditor.text = ""
                    gcodeEditor.forceActiveFocus()
                }
            }

            Item { Layout.fillWidth: true }

            CxButton {
                text: qsTr("取消")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }

            CxButton {
                text: qsTr("确定")
                cxStyle: CxButton.Style.Primary
                onClicked: {
                    root.gcodeAccepted(gcodeEditor.text)
                    root.accept()
                }
            }
        }
    }

    // -- Flat parameter data model (static, no backend needed) --
    // Aligned with upstream categories:
    //   [Global] Slicing State, Slicing State, Print Statistics,
    //   Objects Info, Dimensions, Temperatures, Timestamps, Presets
    ListModel {
        id: paramModel

        // categories map: categoryId -> { expanded, startIndex, endIndex }
        property var categoryStates: ({})
        property var allItems: []

        function rebuildFlatList() {
            clear()
            var flat = []
            categoryStates = {}

            // -- Category definitions (对齐 upstream init_params_list) --
            var categories = [
                {
                    id: "slicing_state", label: qsTr("切片状态"),
                    items: [
                        { label: "layer_num", code: "{layer_num}", desc: qsTr("当前层号 (从 1 开始)"), isVector: false },
                        { label: "layer_z", code: "{layer_z}", desc: qsTr("当前层 Z 高度 (mm)"), isVector: false },
                        { label: "max_layer_z", code: "{max_layer_z}", desc: qsTr("模型最大 Z 高度 (mm)"), isVector: false },
                        { label: "print_z", code: "{print_z}", desc: qsTr("当前打印 Z 高度 (mm)"), isVector: false },
                        { label: "extruder_id", code: "[extruder_id]", desc: qsTr("当前挤出机 ID"), isVector: false },
                        { label: "extruder_temperature", code: "{temperature[extruder_id]}", desc: qsTr("当前挤出机温度"), isVector: true }
                    ]
                },
                {
                    id: "print_stats", label: qsTr("打印统计"),
                    items: [
                        { label: "filament_type", code: "{filament_type[extruder_id]}", desc: qsTr("耗材类型"), isVector: true },
                        { label: "filament_used", code: "{filament_used[extruder_id]}", desc: qsTr("已用耗材长度 (mm)"), isVector: true },
                        { label: "filament_weight", code: "{filament_weight[extruder_id]}", desc: qsTr("已用耗材重量 (g)"), isVector: true },
                        { label: "retract_len", code: "{retract_length[extruder_id]}", desc: qsTr("回抽长度 (mm)"), isVector: true },
                        { label: "print_time", code: "{print_time}", desc: qsTr("预估打印时间"), isVector: false },
                        { label: "total_layer_count", code: "{total_layer_count}", desc: qsTr("总层数"), isVector: false }
                    ]
                },
                {
                    id: "objects_info", label: qsTr("对象信息"),
                    items: [
                        { label: "object_name", code: "{object_name}", desc: qsTr("当前对象名称"), isVector: false },
                        { label: "object_id", code: "{object_id}", desc: qsTr("当前对象 ID"), isVector: false },
                        { label: "num_objects", code: "{num_objects}", desc: qsTr("总对象数"), isVector: false },
                        { label: "num_instances", code: "{num_instances}", desc: qsTr("总实例数"), isVector: false },
                        { label: "has_wipe_tower", code: "{has_wipe_tower}", desc: qsTr("是否启用擦料塔"), isVector: false }
                    ]
                },
                {
                    id: "dimensions", label: qsTr("尺寸"),
                    items: [
                        { label: "first_layer_height", code: "{first_layer_height}", desc: qsTr("首层层高 (mm)"), isVector: false },
                        { label: "layer_height", code: "{layer_height}", desc: qsTr("层高 (mm)"), isVector: false },
                        { label: "max_print_height", code: "{max_print_height}", desc: qsTr("最大打印高度 (mm)"), isVector: false },
                        { label: "object_height", code: "{current_object_height}", desc: qsTr("当前对象高度 (mm)"), isVector: false },
                        { label: "first_layer_print_height", code: "{first_layer_print_height}", desc: qsTr("首层打印高度 (mm)"), isVector: false }
                    ]
                },
                {
                    id: "temperatures", label: qsTr("温度"),
                    items: [
                        { label: "nozzle_temp", code: "{temperature[extruder_id]}", desc: qsTr("喷嘴温度"), isVector: true },
                        { label: "bed_temp", code: "{bed_temperature[extruder_id]}", desc: qsTr("热床温度"), isVector: true },
                        { label: "first_layer_nozzle_temp", code: "{first_layer_temperature[extruder_id]}", desc: qsTr("首层喷嘴温度"), isVector: true },
                        { label: "first_layer_bed_temp", code: "{first_layer_bed_temperature[extruder_id]}", desc: qsTr("首层热床温度"), isVector: true },
                        { label: "chamber_temp", code: "{chamber_temperature}", desc: qsTr("封闭室温度"), isVector: false }
                    ]
                },
                {
                    id: "timestamps", label: qsTr("时间戳"),
                    items: [
                        { label: "timestamp", code: "{timestamp}", desc: qsTr("当前时间戳 (YYYYMMDD-HHmmss)"), isVector: false },
                        { label: "date", code: "{date}", desc: qsTr("当前日期 (YYYYMMDD)"), isVector: false },
                        { label: "time", code: "{time}", desc: qsTr("当前时间 (HHmmss)"), isVector: false },
                        { label: "year", code: "{year}", desc: qsTr("年份 (YYYY)"), isVector: false },
                        { label: "month", code: "{month}", desc: qsTr("月份 (MM)"), isVector: false },
                        { label: "day", code: "{day}", desc: qsTr("日期 (DD)"), isVector: false },
                        { label: "hour", code: "{hour}", desc: qsTr("小时 (HH)"), isVector: false },
                        { label: "minute", code: "{minute}", desc: qsTr("分钟 (mm)"), isVector: false },
                        { label: "second", code: "{second}", desc: qsTr("秒 (ss)"), isVector: false }
                    ]
                },
                {
                    id: "presets", label: qsTr("预设参数"),
                    items: [
                        { label: "infill_type", code: "{infill_type}", desc: qsTr("填充模式"), isVector: false },
                        { label: "infill_density", code: "{infill_density}", desc: qsTr("填充密度 (%)"), isVector: false },
                        { label: "wall_loops", code: "{wall_loops}", desc: qsTr("墙体层数"), isVector: false },
                        { label: "top_shell_layers", code: "{top_shell_layers}", desc: qsTr("顶部层数"), isVector: false },
                        { label: "bottom_shell_layers", code: "{bottom_shell_layers}", desc: qsTr("底部层数"), isVector: false },
                        { label: "support_material", code: "{support_material}", desc: qsTr("是否生成支撑"), isVector: false },
                        { label: "print_speed", code: "{print_speed}", desc: qsTr("打印速度 (mm/s)"), isVector: false },
                        { label: "travel_speed", code: "{travel_speed}", desc: qsTr("空走速度 (mm/s)"), isVector: false },
                        { label: "brim_width", code: "{brim_width}", desc: qsTr("裙边宽度 (mm)"), isVector: false },
                        { label: "skirt_distance", code: "{skirt_distance}", desc: qsTr("裙边距离 (mm)"), isVector: false },
                        { label: "nozzle_diameter", code: "{nozzle_diameter[extruder_id]}", desc: qsTr("喷嘴直径 (mm)"), isVector: true }
                    ]
                }
            ]

            // Build flat list with category headers interleaved
            for (var c = 0; c < categories.length; ++c) {
                var cat = categories[c]
                var catStartIdx = flat.length

                // Category header
                flat.push({
                    categoryId: cat.id,
                    label: cat.label,
                    isCategory: true,
                    code: "",
                    desc: "",
                    isVector: false,
                    expanded: categoryStates[cat.id] ? categoryStates[cat.id].expanded : (c < 2),
                    visible: true
                })

                // Category items
                for (var i = 0; i < cat.items.length; ++i) {
                    var item = cat.items[i]
                    flat.push({
                        categoryId: cat.id,
                        label: item.label,
                        code: item.code,
                        desc: item.desc,
                        isCategory: false,
                        isVector: item.isVector,
                        expanded: false,
                        visible: categoryStates[cat.id] ? categoryStates[cat.id].expanded : (c < 2)
                    })
                }

                categoryStates[cat.id] = {
                    expanded: categoryStates[cat.id] ? categoryStates[cat.id].expanded : (c < 2),
                    headerIndex: catStartIdx
                }
            }

            allItems = flat

            // Populate the ListModel
            for (var j = 0; j < flat.length; ++j) {
                append(flat[j])
            }
        }

        function toggleCategory(categoryId) {
            var state = categoryStates[categoryId]
            if (!state) return

            var newExpanded = !state.expanded
            state.expanded = newExpanded

            // Update visibility and rebuild
            rebuildFlatList()
        }
    }

    // Proxy model for search filtering (对齐 upstream on_search_update / RefreshSearch)
    ListModel {
        id: paramProxyModel

        property string searchText: ""

        function updateFilter(text) {
            searchText = text.toLowerCase()
            rebuild()
        }

        function rebuild() {
            clear()
            var filterText = searchText
            var src = paramModel

            if (!src.allItems || src.allItems.length === 0) {
                src.rebuildFlatList()
            }

            var items = src.allItems
            var catVisibleCount = {}

            // First pass: count visible items per category
            for (var i = 0; i < items.length; ++i) {
                var item = items[i]
                if (item.isCategory) continue

                if (filterText === "" || item.label.toLowerCase().indexOf(filterText) !== -1
                    || (item.desc && item.desc.toLowerCase().indexOf(filterText) !== -1)
                    || item.code.toLowerCase().indexOf(filterText) !== -1) {
                    if (!catVisibleCount[item.categoryId])
                        catVisibleCount[item.categoryId] = 0
                    catVisibleCount[item.categoryId]++
                }
            }

            // Second pass: build filtered list
            for (var j = 0; j < items.length; ++j) {
                var it = items[j]
                if (it.isCategory) {
                    // Show category header if it has matching children
                    if (catVisibleCount[it.categoryId] > 0) {
                        append({
                            categoryId: it.categoryId,
                            label: it.label,
                            isCategory: true,
                            code: "",
                            desc: "",
                            isVector: false,
                            expanded: true,
                            visible: true
                        })
                    }
                } else {
                    // Show item if matches filter (or no filter)
                    var match = filterText === ""
                        || it.label.toLowerCase().indexOf(filterText) !== -1
                        || (it.desc && it.desc.toLowerCase().indexOf(filterText) !== -1)
                        || it.code.toLowerCase().indexOf(filterText) !== -1
                    if (match) {
                        append({
                            categoryId: it.categoryId,
                            label: it.label,
                            isCategory: false,
                            code: it.code,
                            desc: it.desc,
                            isVector: it.isVector,
                            expanded: false,
                            visible: true
                        })
                    }
                }
            }
        }
    }

    // -- Insert selected param into editor at cursor --
    function insertSelectedParam() {
        if (!root.selectedParamCode || !gcodeEditor)
            return

        var cursorPos = gcodeEditor.cursorPosition
        var atEnd = (cursorPos >= gcodeEditor.text.length)
        var insertText = atEnd ? "\n" + root.selectedParamCode : root.selectedParamCode

        gcodeEditor.insert(cursorPos, insertText)

        // If code has brackets, place cursor inside them (对齐 upstream add_selected_value_to_gcode)
        if (root.selectedParamCode.indexOf("[") >= 0 && root.selectedParamCode.indexOf("]") >= 0) {
            var openIdx = root.selectedParamCode.indexOf("[")
            var closeIdx = root.selectedParamCode.indexOf("]")
            if (closeIdx - openIdx === 1) {
                // Empty brackets: place cursor inside
                gcodeEditor.cursorPosition = cursorPos + openIdx + 1
            } else {
                // Named parameter: select the default name inside brackets
                var selStart = cursorPos + openIdx + 1
                var selEnd = cursorPos + closeIdx
                gcodeEditor.select(selStart, selEnd)
            }
        } else {
            gcodeEditor.cursorPosition = cursorPos + insertText.length
        }

        gcodeEditor.forceActiveFocus()
    }

    // Initialize on opened
    onOpened: {
        gcodeEditor.text = root.initialGCode
        gcodeEditor.cursorPosition = gcodeEditor.text.length
        root.selectedParamCode = ""
        root.selectedParamLabel = ""
        root.selectedParamDesc = ""
        searchField.text = ""
        paramModel.rebuildFlatList()
        paramProxyModel.rebuild()
    }
}
