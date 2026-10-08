import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// ─────────────────────────────────────────────────────────────────────────────
// PresetDiffDialog.qml — preset side-by-side diff (upstream DiffPresetDialog)
//
// Upstream truth: third_party/OrcaSlicer/src/slic3r/GUI/UnsavedChangesDialog.cpp
//   - Preset rows per type (1732-1776): left combo (35em) + equal state
//     button + right combo; combo changes recompute the tree directly
//     (1747-1753, no manual compare button); the equal button copies the
//     left selection to the right (1768-1774); states equal/not_equal/
//     question (2091/2097/2064).
//   - "Show all presets (including incompatible)" checkbox (1778-1792):
//     toggles compatibility filtering of the non-printer combos.
//   - Bold top info line "Select presets to compare" and bold bottom info
//     line (1794-1803); the bottom line shows the failure reason and only
//     while the tree is hidden (update_bottom_info, 2036-2042).
//   - DiffViewCtrl tree (1805-1814): hidden 6em toggle column + 35em
//     name/icon column + "Left Preset Value" + "Right Preset Value"; rows
//     are preset -> category -> group -> option with icons (cog/spool/
//     printer roots, per-category icons, node_dot groups), zebra rows and
//     expanders (2103-2138, 579); option rows color the left value with the
//     default text color and the right value orange (172-173); color values
//     ("#...") render as solid swatches with "Undefined" on the other side
//     (142-169); long values truncate at 30 chars with "..." (660-675) and
//     double-click opens FullCompareDialog (592-593, 677-703, 1634-1710).
//   - Transfer block (1882-1908, 1840-1871): "Transfer values from left to
//     right" checkbox reveals the toggle column and Transfer/Cancel buttons;
//     unchecked rows gray out (UpdateEnabling, 189-193).
//
// Qt6 adaptations (data-model / test-locked, see summary):
//   - The scope combo is the Qt6 counterpart of upstream show(Preset::Type)
//     row filtering (TYPE_INVALID shows every row).
//   - The Qt6 preset store has no printer-technology field and no
//     per-extruder extruder_colour vectors, so the upstream "different
//     printer technology" state and the "Extruder count" capability row
//     have no data to trigger on and are not reproduced.
//   - Leaves keep the Qt6 added/removed/changed status badges that upstream
//     does not have (test-locked, QmlUiAuditTests.cpp:8269-8276).
//   - No differences: an empty diff keeps the tree hidden and shows the bold
//     bottom info line instead (upstream update_bottom_info).
// ─────────────────────────────────────────────────────────────────────────────

CxDialog {
    id: root
    modal: false
    dialogTitle: qsTr("预设对比")
    // Upstream SetMinSize(80em x 30em) with a 65em x 40em tree at em = 10
    // (UnsavedChangesDialog.cpp:1807, 1922); CxDialog is fixed-size, so the
    // initial upstream fit is used as the constant geometry.
    width: 800
    height: 700
    padding: 0

    property var configVm: null
    // Upstream DiffPresetDialog::show(Preset::Type) filter: 0 = TYPE_INVALID
    // (every row), 1 = printer, 2 = filament, 3 = print.
    property int viewType: 0

    // Diff tree state ---------------------------------------------------------
    property var treeRoots: []
    property var visibleTreeRows: []
    // Expansion deviations from the defaults (preset/category collapsed,
    // group open - upstream expands group nodes only, 2103-2138).
    property var expandedState: ({})
    property bool treeShown: false
    property bool canTransferOptions: false
    property string baseBottomInfo: ""
    property string hoverBottomInfo: ""
    // Per-row selector state, aligned with presetTypeRows.
    property var rowState: [
        { left: "", right: "", equalState: "equal" },
        { left: "", right: "", equalState: "equal" },
        { left: "", right: "", equalState: "equal" }
    ]

    // Column widths at em = 10: hidden 6em toggle + 35em name + two 15em
    // value columns (UnsavedChangesDialog.cpp:1808-1811). The badge column is
    // the test-locked Qt6 addition.
    readonly property int toggleColWidth: 60
    readonly property int nameColWidth: 350
    readonly property int badgeColWidth: 60

    readonly property bool transferMode: transferCheckBox.checked
    readonly property bool treeHasSelection: anyLeafChecked()
    readonly property bool transferEnabled: treeHasSelection && transferTargetsWritable()

    // The three FFF preset rows upstream builds (TYPE_PRINTER / TYPE_FILAMENT
    // / TYPE_PRINT, UnsavedChangesDialog.cpp:1735); the SLA rows are omitted
    // because the app has no SLA preset categories.
    readonly property var presetTypeRows: [
        { view: 1, listProp: "printerPresetNames", compatProp: "",
          currentProp: "currentPrinterPreset", icon: "preset_printer" },
        { view: 2, listProp: "filamentPresetNames", compatProp: "compatibleFilamentPresetNames",
          currentProp: "currentFilamentPreset", icon: "preset_spool" },
        { view: 3, listProp: "printPresetNames", compatProp: "compatiblePrintPresetNames",
          currentProp: "currentPrintPreset", icon: "preset_cog" }
    ]

    onOpened: {
        hoverBottomInfo = ""
        scopeCombo.currentIndex = root.viewType
        root.seedRows()
        root.syncRowCombos()
        root.updateTree()
    }

    // ── Preset lists / visibility ────────────────────────────────────────────
    // show_all only affects the non-printer combos (UnsavedChangesDialog.cpp
    // :1782-1790); the printer combo always lists every preset.
    function presetListFor(typeDef) {
        if (!configVm)
            return []
        if (typeDef.compatProp.length > 0 && !showAllCheckBox.checked)
            return configVm[typeDef.compatProp] || []
        return configVm[typeDef.listProp] || []
    }

    function rowIsVisible(typeDef) {
        return viewType === 0 || viewType === typeDef.view
    }

    // Seed both combos of every row with the currently selected preset of the
    // category (upstream uses collection->get_selected_preset().name, 1754).
    function seedRows() {
        for (var i = 0; i < presetTypeRows.length; i++) {
            var def = presetTypeRows[i]
            var names = configVm ? (configVm[def.listProp] || []) : []
            var current = configVm ? (configVm[def.currentProp] || "") : ""
            if (current.length === 0 || names.indexOf(current) === -1)
                current = names.length > 0 ? names[0] : ""
            rowState[i].left = current
            rowState[i].right = current
            rowState[i].equalState = "equal"
        }
        rowState = rowState.slice()
    }

    function syncRowCombos() {
        for (var i = 0; i < presetTypeRows.length; i++) {
            var rowItem = rowRepeater.itemAt(i)
            if (!rowItem)
                continue
            rowItem.leftCombo.currentIndex = rowItem.leftCombo.model.indexOf(rowState[i].left)
            rowItem.rightCombo.currentIndex = rowItem.rightCombo.model.indexOf(rowState[i].right)
        }
    }

    // ── Tree building (upstream DiffPresetDialog::update_tree, 2036-2147) ───
    function categoryIcon(categoryName) {
        var map = {
            "质量": "param_wall",
            "填充": "param_infill",
            "速度": "param_speed",
            "加速度": "param_acceleration",
            "温度": "param_temperature",
            "支撑": "param_support",
            "底座": "param_adhension",
            "冷却": "param_cooling",
            "回退": "param_retraction",
            "其他": "param_information",
            "Basic information": "param_information",
            "Machine G-code": "param_gcode"
        }
        return "qrc:/qml/assets/icons/" + (map[categoryName] || "param_settings") + ".svg"
    }

    function buildPresetRoot(typeDef, left, right, diffRows) {
        var categories = []
        var categoryByName = {}
        for (var i = 0; i < diffRows.length; i++) {
            var r = diffRows[i]
            var catName = r.category && r.category.length > 0 ? r.category : qsTr("其他")
            var grpName = r.group && r.group.length > 0 ? r.group : qsTr("参数")
            var category = categoryByName[catName]
            if (!category) {
                category = { kind: "category", name: catName, icon: root.categoryIcon(catName),
                             checked: true, children: [], groupByName: {} }
                categoryByName[catName] = category
                categories.push(category)
            }
            var group = category.groupByName[grpName]
            if (!group) {
                group = { kind: "group", name: grpName,
                          icon: "qrc:/qml/assets/icons/preset_node_dot.svg",
                          checked: true, children: [] }
                category.groupByName[grpName] = group
                category.children.push(group)
            }
            group.children.push({
                kind: "leaf",
                name: r.label && r.label.length > 0 ? r.label : r.key,
                key: r.key,
                valueA: r.valueA || "",
                valueB: r.valueB || "",
                fullValueA: r.fullValueA || "",
                fullValueB: r.fullValueB || "",
                isLong: r.isLong === true,
                status: r.status || "",
                checked: true
            })
        }
        for (var c = 0; c < categories.length; c++)
            delete categories[c].groupByName
        // Root label mirrors upstream: "\"A\" vs \"B\"" (2103).
        return { kind: "preset", name: "\"" + left + "\" vs \"" + right + "\"",
                 icon: "qrc:/qml/assets/icons/" + typeDef.icon + ".svg",
                 left: left, right: right, checked: true, children: categories }
    }

    function updateTree() {
        var roots = []
        var bottom = ""
        var showTree = false
        var leftRightDiffer = false
        if (configVm) {
            for (var i = 0; i < presetTypeRows.length; i++) {
                var def = presetTypeRows[i]
                if (!rowIsVisible(def))
                    continue
                var sel = rowState[i]
                var names = configVm[def.listProp] || []
                if (names.indexOf(sel.left) === -1 || names.indexOf(sel.right) === -1) {
                    sel.equalState = "question"
                    bottom = qsTr("其中一个预设不存在")
                    continue
                }
                if (sel.left !== sel.right)
                    leftRightDiffer = true
                var rows = configVm.comparePresetsDetailed(sel.left, sel.right) || []
                if (rows.length === 0) {
                    // Same preset or no differing keys: equal state, no info.
                    sel.equalState = "equal"
                    bottom = ""
                    continue
                }
                showTree = true
                sel.equalState = "not_equal"
                roots.push(buildPresetRoot(def, sel.left, sel.right, rows))
            }
        }
        // Upstream clears the tree on every update, so expansions reset to
        // the defaults as well.
        expandedState = {}
        treeRoots = roots
        treeShown = showTree
        canTransferOptions = viewType !== 0 || leftRightDiffer
        baseBottomInfo = bottom
        rebuildVisibleRows()
        rowState = rowState.slice()
    }

    // ── Visible-row flattening ───────────────────────────────────────────────
    function nodeOpen(path, node) {
        if (expandedState[path] !== undefined)
            return expandedState[path]
        return node.kind === "group"
    }

    function rebuildVisibleRows() {
        var rows = []
        function walk(node, path, depth) {
            var isBranch = node.children && node.children.length > 0
            var open = isBranch ? nodeOpen(path, node) : false
            // Leaf fields are spread onto the row so delegates read them
            // directly (key / valueA / valueB / status / isLong).
            var row = { node: node, path: path, depth: depth, open: open }
            if (node.kind === "leaf") {
                row.key = node.key
                row.valueA = node.valueA
                row.valueB = node.valueB
                row.status = node.status
                row.isLong = node.isLong
            }
            rows.push(row)
            if (open) {
                for (var i = 0; i < node.children.length; i++)
                    walk(node.children[i], path + "/" + i, depth + 1)
            }
        }
        for (var i = 0; i < treeRoots.length; i++)
            walk(treeRoots[i], "" + i, 0)
        visibleTreeRows = rows
    }

    function toggleExpanded(path, currentOpen) {
        // Copy-on-write so the var property change signal always fires.
        var state = {}
        for (var key in expandedState)
            state[key] = expandedState[key]
        state[path] = !currentOpen
        expandedState = state
        rebuildVisibleRows()
    }

    // ── Toggle (transfer selection) handling ─────────────────────────────────
    function nodeAtPath(path) {
        var segments = path.split("/")
        var node = { children: treeRoots }
        for (var i = 0; i < segments.length; i++)
            node = node.children[parseInt(segments[i], 10)]
        return node
    }

    function setChildrenChecked(node, value) {
        node.checked = value
        if (!node.children)
            return
        for (var i = 0; i < node.children.length; i++)
            setChildrenChecked(node.children[i], value)
    }

    function setCheckedPath(path, value) {
        var node = nodeAtPath(path)
        if (!node)
            return
        // Toggling a node applies to all children (update_children, 395-404)
        // and parents follow "any child checked" (update_parents, 406-420).
        setChildrenChecked(node, value)
        var segments = path.split("/")
        for (var level = segments.length - 1; level > 0; level--) {
            var ancestor = nodeAtPath(segments.slice(0, level).join("/"))
            if (!ancestor || !ancestor.children)
                continue
            var any = false
            for (var c = 0; c < ancestor.children.length; c++) {
                if (ancestor.children[c].checked === true) {
                    any = true
                    break
                }
            }
            ancestor.checked = any
        }
        treeRoots = treeRoots.slice()
        rebuildVisibleRows()
    }

    function anyLeafChecked() {
        function walk(node) {
            if (node.kind === "leaf")
                return node.checked === true
            if (!node.children)
                return false
            for (var i = 0; i < node.children.length; i++)
                if (walk(node.children[i]))
                    return true
            return false
        }
        for (var i = 0; i < treeRoots.length; i++)
            if (walk(treeRoots[i]))
                return true
        return false
    }

    function collectCheckedKeys(node, out) {
        if (node.kind === "leaf") {
            if (node.checked === true)
                out.push(node.key)
            return
        }
        if (!node.children)
            return
        for (var i = 0; i < node.children.length; i++)
            collectCheckedKeys(node.children[i], out)
    }

    function transferTargetsWritable() {
        if (!configVm)
            return false
        for (var i = 0; i < treeRoots.length; i++) {
            if (treeRoots[i].right.length > 0 && configVm.presetIsReadOnly(treeRoots[i].right))
                return false
        }
        return true
    }

    // Transfer the selected option values of every compared pair from left to
    // right (upstream Action::Transfer + Tab::transfer_options), then close.
    function doTransfer() {
        if (!configVm || !transferEnabled)
            return
        for (var i = 0; i < treeRoots.length; i++) {
            var presetNode = treeRoots[i]
            var keys = []
            collectCheckedKeys(presetNode, keys)
            if (keys.length === 0 || presetNode.left.length === 0 || presetNode.right.length === 0)
                continue
            configVm.transferPresetValues(presetNode.left, presetNode.right, keys)
        }
        root.close()
    }

    function openFullCompare(row) {
        // Leaf payload (option name / untruncated values) rides on the node.
        var leaf = row.node
        fullCompareDialog.fullOptionName = leaf.name || ""
        fullCompareDialog.leftValue = leaf.fullValueA || ""
        fullCompareDialog.rightValue = leaf.fullValueB || ""
        fullCompareDialog.computeExclusive()
        fullCompareDialog.open()
    }

    // ── FullCompareDialog (upstream UnsavedChangesDialog.cpp:1634-1710) ──────
    component FullCompareSideDialog: CxDialog {
        id: fullCompare
        modal: true
        dialogTitle: fullOptionName
        // Two 400x400 read-only value panes plus chrome (1681).
        width: 860
        height: 560

        property string fullOptionName: ""
        property string leftValue: ""
        property string rightValue: ""
        property var leftExclusive: []
        property var rightExclusive: []

        function escapeHtml(value) {
            var out = value.replace(/&/g, "&amp;")
            out = out.replace(/</g, "&lt;")
            out = out.replace(/>/g, "&gt;")
            return out
        }

        // get_set_from_val (1660-1672): single-line values split on spaces.
        function tokenSet(raw) {
            var normalized = raw.indexOf("\n") === -1 ? raw.replace(/ /g, "\n") : raw
            var parts = normalized.split("\n")
            var out = []
            for (var i = 0; i < parts.length; i++) {
                if (parts[i].length > 0 && out.indexOf(parts[i]) === -1)
                    out.push(parts[i])
            }
            return out
        }

        // std::set_difference of both token sets (1674-1681).
        function computeExclusive() {
            var a = tokenSet(leftValue)
            var b = tokenSet(rightValue)
            var onlyLeft = []
            var onlyRight = []
            for (var i = 0; i < a.length; i++) {
                if (b.indexOf(a[i]) === -1)
                    onlyLeft.push(a[i])
            }
            for (var j = 0; j < b.length; j++) {
                if (a.indexOf(b[j]) === -1)
                    onlyRight.push(b[j])
            }
            leftExclusive = onlyLeft
            rightExclusive = onlyRight
        }

        // Bold the first occurrence of each exclusive token (1672-1690).
        function applyExclusive(html, tokens) {
            var out = html
            for (var i = 0; i < tokens.length; i++) {
                var token = escapeHtml(tokens[i])
                var pos = out.indexOf(token)
                if (pos === -1)
                    continue
                out = out.substring(0, pos) + "<b>" + token + "</b>" + out.substring(pos + token.length)
            }
            return out
        }

        // Left column keeps the default text color (1683); explicit font
        // wrapper so rich text does not fall back to the palette black.
        function leftHtml() {
            return "<font color=\"" + Theme.textPrimary.name + "\">"
                   + applyExclusive(escapeHtml(leftValue), leftExclusive) + "</font>"
        }

        // The right column is orange as a whole (1683, 1694-1695).
        function rightHtml() {
            return "<font color=\"" + Theme.statusWarning.name + "\">"
                   + applyExclusive(escapeHtml(rightValue), rightExclusive) + "</font>"
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingLG
            spacing: Theme.spacingMD

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                Text {
                    Layout.fillWidth: true
                    text: qsTr("左侧预设值")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("右侧预设值")
                    color: Theme.textPrimary
                    font.pixelSize: Theme.fontSizeSM
                    font.bold: true
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: Theme.spacingMD
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Theme.bgInset
                    border.color: Theme.borderInput
                    border.width: 1
                    radius: Theme.radiusSM
                    clip: true
                    TextArea {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingSM
                        readOnly: true
                        textFormat: TextEdit.RichText
                        wrapMode: TextEdit.Wrap
                        color: Theme.textPrimary
                        font.pixelSize: Theme.fontSizeSM
                        text: fullCompare.leftHtml()
                    }
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: Theme.bgInset
                    border.color: Theme.borderInput
                    border.width: 1
                    radius: Theme.radiusSM
                    clip: true
                    TextArea {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingSM
                        readOnly: true
                        textFormat: TextEdit.RichText
                        wrapMode: TextEdit.Wrap
                        color: Theme.statusWarning
                        font.pixelSize: Theme.fontSizeSM
                        text: fullCompare.rightHtml()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                CxButton {
                    text: qsTr("确定")
                    onClicked: fullCompare.close()
                }
            }
        }
    }

    FullCompareSideDialog { id: fullCompareDialog }

    // Diff value cell: color values ("#...") render as solid swatches, the
    // opposite side shows "Undefined" (upstream 142-169); text is grayed out
    // when the row is untoggled in transfer mode (UpdateEnabling, 189-193).
    component DiffValueCell: Item {
        id: valueCell
        property string cellValue: ""
        property bool cellIsColor: cellValue.startsWith("#")
        property bool otherIsColor: false
        property bool greyed: false
        property color cellColor: Theme.textPrimary
        clip: true

        Rectangle {
            visible: valueCell.cellIsColor
            anchors.left: parent.left
            anchors.leftMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            width: 48
            height: 12
            radius: 2
            color: valueCell.cellValue
            border.color: Theme.borderSubtle
            border.width: 1
            opacity: valueCell.greyed ? 0.4 : 1.0
        }
        Text {
            visible: !valueCell.cellIsColor
            anchors.left: parent.left
            anchors.leftMargin: 4
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: valueCell.otherIsColor ? qsTr("未定义") : valueCell.cellValue
            color: valueCell.greyed ? Theme.textDisabled : valueCell.cellColor
            font.pixelSize: Theme.fontSizeSM
            elide: Text.ElideRight
        }
    }

    // ── Layout: top info / presets / checkbox / tree / bottom info / edit ────
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Bold top info line (create_info_lines, 1794-1803).
        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.topMargin: 20
            text: qsTr("请选择要对比的预设")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
            elide: Text.ElideRight
        }

        // Per-type preset rows: [left combo][equal state button][right combo]
        // (create_presets_sizer, 1732-1776). The scope row is the Qt6
        // counterpart of upstream show(Preset::Type) row filtering.
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.topMargin: 10
            spacing: 5

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingMD
                Text {
                    text: qsTr("范围：")
                    color: Theme.textMuted
                    font.pixelSize: Theme.fontSizeSM
                }
                CxComboBox {
                    id: scopeCombo
                    Layout.preferredWidth: 120
                    model: [qsTr("全部"), qsTr("打印机"), qsTr("材料"), qsTr("工艺")]
                    onActivated: {
                        root.viewType = currentIndex
                        root.syncRowCombos()
                        root.updateTree()
                    }
                }
                Item { Layout.fillWidth: true }
            }

            Repeater {
                id: rowRepeater
                model: root.presetTypeRows.length

                delegate: RowLayout {
                    required property int index
                    readonly property var typeDef: root.presetTypeRows[index]
                    property alias leftCombo: leftCombo
                    property alias rightCombo: rightCombo

                    spacing: 5
                    Layout.fillWidth: true

                    CxComboBox {
                        id: leftCombo
                        Layout.fillWidth: true
                        Layout.preferredWidth: 350
                        enabled: model.length > 0
                        model: root.presetListFor(typeDef)
                        onActivated: {
                            if (currentIndex >= 0 && currentIndex < model.length) {
                                root.rowState[index].left = model[currentIndex]
                                root.rowState = root.rowState.slice()
                                root.updateTree()
                            }
                        }
                    }

                    // Equal state button (ScalableButton "equal", 1742):
                    // click copies the left selection to the right combo
                    // (1768-1774); question state carries the failure info.
                    Item {
                        Layout.preferredWidth: 16
                        Layout.preferredHeight: 16
                        Layout.alignment: Qt.AlignVCenter
                        Image {
                            anchors.fill: parent
                            source: "qrc:/qml/assets/icons/preset_"
                                    + root.rowState[index].equalState + ".svg"
                        }
                        MouseArea {
                            id: equalMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.rowState[index].right = root.rowState[index].left
                                root.rowState = root.rowState.slice()
                                root.syncRowCombos()
                                root.updateTree()
                            }
                        }
                        ToolTip.visible: equalMouse.containsMouse
                                         && root.rowState[index].equalState === "question"
                        ToolTip.text: root.baseBottomInfo
                    }

                    CxComboBox {
                        id: rightCombo
                        Layout.fillWidth: true
                        Layout.preferredWidth: 350
                        enabled: model.length > 0
                        model: root.presetListFor(typeDef)
                        onActivated: {
                            if (currentIndex >= 0 && currentIndex < model.length) {
                                root.rowState[index].right = model[currentIndex]
                                root.rowState = root.rowState.slice()
                                root.updateTree()
                            }
                        }
                    }
                }
            }
        }

        // "Show all presets (including incompatible)" checkbox; hidden in the
        // printer-only view (UnsavedChangesDialog.cpp:1981).
        CxCheckBox {
            id: showAllCheckBox
            visible: root.viewType !== 1
            Layout.fillWidth: true
            Layout.margins: 10
            text: qsTr("显示全部预设（含不兼容）")
            onToggled: {
                // Combo models re-bind after this handler; sync the current
                // indexes once the new lists are in place.
                Qt.callLater(root.syncRowCombos)
                root.updateTree()
            }
        }

        // DiffViewCtrl tree section: header + 4-level flat rows.
        ColumnLayout {
            visible: root.treeShown
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 10
            spacing: 0

            // Column header: hidden toggle "✔" + "" + Left/Right Preset Value
            // (1808-1811); the badge column stays unlabeled.
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                color: Theme.bgInset
                radius: 3

                RowLayout {
                    anchors.fill: parent
                    spacing: 0
                    Item {
                        visible: root.transferMode
                        Layout.preferredWidth: root.transferMode ? root.toggleColWidth : 0
                        Text {
                            anchors.centerIn: parent
                            text: "✔"
                            color: Theme.textMuted
                            font.pixelSize: Theme.fontSizeSM
                        }
                    }
                    Item { Layout.preferredWidth: root.nameColWidth }
                    Text {
                        Layout.fillWidth: true
                        Layout.leftMargin: 4
                        Layout.rightMargin: 2
                        text: qsTr("左侧预设值")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSizeXS
                        font.bold: true
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.fillWidth: true
                        Layout.leftMargin: 4
                        Layout.rightMargin: 2
                        text: qsTr("右侧预设值")
                        color: Theme.textMuted
                        font.pixelSize: Theme.fontSizeXS
                        font.bold: true
                        elide: Text.ElideRight
                    }
                    Item { Layout.preferredWidth: root.badgeColWidth }
                }
            }

            ListView {
                id: diffTree
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: root.visibleTreeRows
                reuseItems: false

                ScrollBar.vertical: ScrollBar { }

                delegate: Rectangle {
                    id: treeRow
                    required property var modelData
                    required property int index

                    width: diffTree.width
                    height: 28
                    // wxDV_ROW_LINES zebra striping.
                    color: index % 2 === 0 ? Theme.bgElevated : Theme.bgBase

                    // Long (truncated) leaf rows open the full-compare dialog
                    // on double-click (upstream item activation, 592-593).
                    MouseArea {
                        anchors.fill: parent
                        enabled: treeRow.modelData.node.kind === "leaf"
                                 && treeRow.modelData.isLong === true
                        onDoubleClicked: root.openFullCompare(treeRow.modelData)
                    }

                    // Option key tooltip for leaf rows (the canonical config
                    // id behind the displayed label).
                    HoverHandler { id: rowHover }
                    ToolTip.visible: rowHover.hovered
                                     && treeRow.modelData.node.kind === "leaf"
                    ToolTip.text: treeRow.modelData.key || ""
                    ToolTip.delay: 350

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0

                        // Toggle column, only visible in transfer mode
                        // (m_use_for_transfer reveals colToggle, 1887-1897).
                        Item {
                            visible: root.transferMode
                            Layout.preferredWidth: root.transferMode ? root.toggleColWidth : 0
                            Layout.fillHeight: true
                            CxCheckBox {
                                anchors.centerIn: parent
                                checked: treeRow.modelData.node.checked === true
                                onToggled: root.setCheckedPath(treeRow.modelData.path, checked)
                            }
                        }

                        // Name column: indent + expander + icon + bold label.
                        Item {
                            Layout.preferredWidth: root.nameColWidth
                            Layout.fillHeight: true
                            clip: true

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                x: treeRow.modelData.depth * 16
                                spacing: 4

                                Item {
                                    width: 12
                                    height: 16
                                    visible: treeRow.modelData.node.children
                                             && treeRow.modelData.node.children.length > 0
                                    Text {
                                        anchors.centerIn: parent
                                        text: treeRow.modelData.open ? "▾" : "▸"
                                        color: Theme.textMuted
                                        font.pixelSize: Theme.fontSizeXS
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.toggleExpanded(treeRow.modelData.path,
                                                                       treeRow.modelData.open)
                                    }
                                }

                                // Preset/category/group icon; leaves use the
                                // upstream "empty" icon (blank slot).
                                Image {
                                    width: 16
                                    height: 16
                                    anchors.verticalCenter: parent.verticalCenter
                                    source: treeRow.modelData.node.icon || ""
                                    opacity: treeRow.modelData.node.checked === false ? 0.4 : 1.0
                                }

                                Text {
                                    width: root.nameColWidth - treeRow.modelData.depth * 16 - 44
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: treeRow.modelData.node.name || ""
                                    color: treeRow.modelData.node.checked === false
                                           ? Theme.textDisabled : Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeSM
                                    font.bold: treeRow.modelData.node.kind !== "leaf"
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        // Left value: default text color or a color swatch.
                        DiffValueCell {
                            visible: treeRow.modelData.node.kind === "leaf"
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            cellValue: treeRow.modelData.valueA || ""
                            otherIsColor: (treeRow.modelData.valueB || "").startsWith("#")
                            greyed: treeRow.modelData.node.checked === false
                            cellColor: Theme.textPrimary
                        }

                        // Right value: orange (upstream color_string orange
                        // #ed6b21, mapped to the statusWarning token).
                        DiffValueCell {
                            visible: treeRow.modelData.node.kind === "leaf"
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            cellValue: treeRow.modelData.valueB || ""
                            otherIsColor: (treeRow.modelData.valueA || "").startsWith("#")
                            greyed: treeRow.modelData.node.checked === false
                            cellColor: Theme.statusWarning
                        }

                        // Qt6-only status badge, test-locked
                        // (QmlUiAuditTests.cpp:8269-8276); upstream has no
                        // badge column.
                        Rectangle {
                            visible: treeRow.modelData.node.kind === "leaf"
                            Layout.preferredWidth: root.badgeColWidth
                            Layout.preferredHeight: 18
                            Layout.alignment: Qt.AlignVCenter
                            Layout.rightMargin: 4
                            radius: 9
                            color: treeRow.modelData.status === "added"   ? Theme.accentDark
                                 : treeRow.modelData.status === "removed" ? Theme.statusErrorDark
                                 : treeRow.modelData.status === "changed" ? Theme.statusWarning
                                                                          : Theme.bgPanel
                            Text {
                                anchors.centerIn: parent
                                text: treeRow.modelData.status || ""
                                color: "white"
                                font.pixelSize: Theme.fontSizeXS
                                font.bold: true
                            }
                        }
                    }
                }
            }
        }

        // Bold bottom info line: failure reason, transfer hover hints; shown
        // only while the tree is hidden (update_bottom_info, 2036-2042,
        // plus the button hover hooks at 1865-1867).
        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            Layout.topMargin: 10
            Layout.bottomMargin: 10
            visible: !root.treeShown
            text: root.hoverBottomInfo.length > 0 ? root.hoverBottomInfo : root.baseBottomInfo
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            font.bold: true
            elide: Text.ElideRight
        }

        // Edit sizer: transfer checkbox + Transfer/Cancel buttons; visible
        // once the tree is shown (m_edit_sizer->Show, 2145-2147); the buttons
        // appear only while the checkbox is checked (1887-1897).
        RowLayout {
            Layout.fillWidth: true
            visible: root.treeShown && root.canTransferOptions
            spacing: 0

            CxCheckBox {
                id: transferCheckBox
                Layout.leftMargin: 10
                Layout.topMargin: 10
                Layout.bottomMargin: 10
                text: qsTr("将值从左侧传输到右侧")
                ToolTip.visible: hovered
                ToolTip.text: qsTr("启用后，此对话框可用于将所选值从左侧预设传输到右侧预设。")
            }
            Item { Layout.preferredWidth: 100 }
            Item { Layout.fillWidth: true }

            CxButton {
                id: transferButton
                Layout.rightMargin: 6
                text: qsTr("传输")
                enabled: root.transferEnabled
                onClicked: root.doTransfer()

                // Hover hint lands in the bottom info line (upstream
                // ENTER/LEAVE window bindings, 1865-1867); NoButton so the
                // button keeps its own clicks.
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.NoButton
                    hoverEnabled: true
                    onContainsMouseChanged: {
                        root.hoverBottomInfo = containsMouse
                            ? (root.transferEnabled
                               ? qsTr("将所选选项从左侧预设传输到右侧。\n注意：关闭此对话框后，新修改的预设将在设置页签中选中。")
                               : qsTr("只能传输到可写的用户预设，右侧存在系统只读预设。"))
                            : ""
                    }
                }
            }

            CxButton {
                Layout.rightMargin: 10
                text: qsTr("取消")
                onClicked: root.reject()
            }
        }
    }
}
