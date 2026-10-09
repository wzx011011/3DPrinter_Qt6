import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../controls"

// P8.3 -- EditGCodeDialog: G-code editor with placeholder insertion
// Aligns 1:1 with upstream EditGCodeDialog (wxDataViewCtrl tree + wxTextCtrl
// + wxSearchCtrl + DialogButtons):
//   - 9 top-level placeholder groups built from the real ConfigDefs through
//     ConfigViewModel::editGcodeParamGroups (upstream init_params_list,
//     EditGCodeDialog.cpp:153-243);
//   - bare-key placeholder insertion (no braces), vector keys as "key[]"
//     (upstream ParamsNode :485-491 / add_selected_value_to_gcode :315-332);
//   - full-width selection label "(type)" + real tooltip description below
//     the grid (:96-108, selection_changed :334-400);
//   - resizable frame (min 45em x 35em, cap 100em x 70em, :37/:116/:415-423)
//     with a 1:2 parameter-column / editor split (:92-94);
//   - icon-only add button (:75-76), search field with embedded search and
//     cancel buttons (:54-59), node bitmap icons (:430-435, :161-309);
//   - search hit highlighting + expansion-state restore (:444-545, :638-664).
// Usage: EditGCodeDialog { id: dlg } -> dlg.open()

CxDialog {
    id: root

    closePolicy: Popup.NoAutoClose

    // Public API -- set before opening
    property string initialGCode: ""
    // Phase 236 (DLG-02): config option key this editor instance edits
    // (e.g. "machine_start_gcode"). Empty for key-less uses (template
    // browsing); onGcodeAccepted callers skip the write-back then.
    property string optionKey: ""
    // Note: dialogTitle is used by CxDialog for the header.
    // This dialog also uses dialogTitle as its title text.
    dialogTitle: qsTr("编辑自定义 G-code")
    signal gcodeAccepted(string gcode)

    // Upstream is a wxRESIZE_BORDER dialog (EditGCodeDialog.cpp:37): clear the
    // base centering anchor so a resize drag keeps the top-left corner fixed;
    // centering is done manually in onAboutToShow.
    anchors.centerIn: undefined

    // Upstream on_dpi_changed SetMinSize(45em, 35em) (EditGCodeDialog.cpp:
    // 415-423) and fit_in_display {100em, 70em} (:116); em = 10px at the
    // reference DPI.
    readonly property int minDialogWidth: 450
    readonly property int minDialogHeight: 350
    readonly property int maxDialogWidth: 1000
    readonly property int maxDialogHeight: 700

    // Grid geometry -- wxFlexGridSizer(1, 3, 5, 15) with border 10
    // (EditGCodeDialog.cpp:44-49) and AddGrowableCol(0,1)/(2,2) weights
    // (:92-94), i.e. the parameter column takes 1/3 and the editor 2/3 of the
    // leftover width.
    readonly property int dlgBorder: 10
    readonly property int gridHGap: 15

    // -- Tree data (loaded from ConfigViewModel on open) --
    property var groupData: []
    property var paramTree: []
    // Flat rows currently displayed by the tree ListView.
    property var displayRows: []

    // -- Search state (upstream ParamsModel::RefreshSearch / FinishSearch) --
    property string searchText: ""
    property bool searching: false
    property var expandedSnapshot: ({})

    // -- Selection state --
    property string selectedNodeId: ""
    property string selectedParamCode: ""
    property string selectedParamLabel: ""
    property string selectedParamDesc: ""

    // Node icon tables -- upstream EditGCodeDialog.cpp:161-238 (groups and
    // lock subgroups), :301-309 (presets subgroups) and :430-435 ParamsInfo
    // (param type bitmaps; FilamentVector is part of ParamsInfo upstream but
    // is never constructed in that dialog).
    readonly property var kNodeIcons: ({
        "global_slicing_state": "qrc:/qml/assets/icons/custom-gcode_slicing-state_global.svg",
        "read_only": "qrc:/qml/assets/icons/lock_closed.svg",
        "read_write": "qrc:/qml/assets/icons/lock_open.svg",
        "slicing_state": "qrc:/qml/assets/icons/custom-gcode_slicing-state.svg",
        "print_statistics": "qrc:/qml/assets/icons/custom-gcode_stats.svg",
        "objects_info": "qrc:/qml/assets/icons/custom-gcode_object-info.svg",
        "dimensions": "qrc:/qml/assets/icons/custom-gcode_measure.svg",
        "temperatures": "qrc:/qml/assets/icons/custom-gcode_temperature.svg",
        "timestamps": "qrc:/qml/assets/icons/custom-gcode_time.svg",
        "specific": "qrc:/qml/assets/icons/custom-gcode_gcode.svg",
        "presets": "qrc:/qml/assets/icons/cog.svg",
        "print_settings": "qrc:/qml/assets/icons/process.svg",
        "filament_settings": "qrc:/qml/assets/icons/filament.svg",
        "printer_settings": "qrc:/qml/assets/icons/editgcode_printer.svg"
    })
    readonly property var kParamIcons: ({
        0: "qrc:/qml/assets/icons/custom-gcode_single.svg",
        1: "qrc:/qml/assets/icons/custom-gcode_vector.svg",
        2: "qrc:/qml/assets/icons/custom-gcode_vector-index.svg"
    })

    width: 620
    height: 400

    function centerToOverlay() {
        var ov = Overlay.overlay
        if (!ov)
            return
        x = Math.max(0, Math.round((ov.width - width) / 2))
        y = Math.max(0, Math.round((ov.height - height) / 2))
    }

    onAboutToShow: centerToOverlay()

    // Localized display name for a group/subgroup id coming from
    // ConfigViewModel (upstream group titles, EditGCodeDialog.cpp:160-238 and
    // :303-309).
    function groupDisplayName(id) {
        switch (id) {
        case "global_slicing_state": return qsTr("[Global] 切片状态")
        case "read_only": return qsTr("只读")
        case "read_write": return qsTr("读写")
        case "slicing_state": return qsTr("切片状态")
        case "print_statistics": return qsTr("打印统计")
        case "objects_info": return qsTr("对象信息")
        case "dimensions": return qsTr("尺寸")
        case "temperatures": return qsTr("温度")
        case "timestamps": return qsTr("时间戳")
        case "specific": return qsTr("特定于 %1").arg(root.optionKey)
        case "presets": return qsTr("预设参数")
        case "print_settings": return qsTr("打印设置")
        case "filament_settings": return qsTr("耗材设置")
        case "printer_settings": return qsTr("打印机设置")
        }
        return id
    }

    function escapeHtml(s) {
        return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
    }

    // Rich text for a tree row -- upstream ParamsNode::GetFormattedText
    // (:494-508): group/subgroup nodes render bold (m_bold, :456/:467) and a
    // matching param's hit substring gets a colored background (highlight()
    // :444-449, upstream teal replaced by the OWzx brand accent).
    function rowRichText(node) {
        var body = escapeHtml(node.text)
        if (root.searching && node.kind === "param" && root.searchText !== "") {
            var idx = node.text.toLowerCase().indexOf(root.searchText)
            if (idx >= 0) {
                body = escapeHtml(node.text.slice(0, idx))
                    + '<span style="background-color: ' + Theme.accent + '">'
                    + escapeHtml(node.text.slice(idx, idx + root.searchText.length))
                    + '</span>'
                    + escapeHtml(node.text.slice(idx + root.searchText.length))
            }
        }
        return node.kind === "param" ? body : "<b>" + body + "</b>"
    }

    function makeParamNode(p, depth, parentPath, pi) {
        // Upstream ParamsNode display text: bare key, vector keys get a "[]"
        // suffix (EditGCodeDialog.cpp:485-491). No braces anywhere.
        var code = p.key + (p.kind === 1 ? "[]" : "")
        return {
            nodeId: parentPath + "/p" + pi,
            kind: "param",
            depth: depth,
            icon: root.kParamIcons[p.kind] || root.kParamIcons[0],
            text: code,
            code: code,
            key: p.key,
            typeStr: p.typeStr || "",
            label: p.label || "",
            fullLabel: p.fullLabel || "",
            tooltip: p.tooltip || "",
            children: []
        }
    }

    // Build the JS tree from the ConfigViewModel payload. Top-level groups
    // start expanded (upstream AppendGroup runs m_ctrl->Expand on the root
    // parent for every group, EditGCodeDialog.cpp:573); subgroups start
    // collapsed except the Specific group, which upstream expands explicitly
    // (:233) and which carries its params directly (no subgroups).
    function rebuildTree() {
        var tree = []
        for (var gi = 0; gi < root.groupData.length; ++gi) {
            var g = root.groupData[gi]
            var gNode = {
                nodeId: "g" + gi,
                kind: "group",
                depth: 0,
                icon: root.kNodeIcons[g.id] || "",
                text: groupDisplayName(g.id),
                expanded: true,
                children: []
            }
            var params = g.params || []
            for (var pi = 0; pi < params.length; ++pi)
                gNode.children.push(makeParamNode(params[pi], 1, gNode.nodeId, pi))
            var subs = g.subgroups || []
            for (var si = 0; si < subs.length; ++si) {
                var s = subs[si]
                var sNode = {
                    nodeId: gNode.nodeId + "/s" + si,
                    kind: "subgroup",
                    depth: 1,
                    icon: root.kNodeIcons[s.id] || "",
                    text: groupDisplayName(s.id),
                    expanded: false,
                    children: []
                }
                var sParams = s.params || []
                for (var spi = 0; spi < sParams.length; ++spi)
                    sNode.children.push(makeParamNode(sParams[spi], 2, sNode.nodeId, spi))
                gNode.children.push(sNode)
            }
            tree.push(gNode)
        }
        root.paramTree = tree
        root.selectedNodeId = ""
        root.selectedParamCode = ""
        root.selectedParamLabel = ""
        root.selectedParamDesc = ""
    }

    function captureExpanded() {
        var snap = {}
        for (var gi = 0; gi < root.paramTree.length; ++gi) {
            var g = root.paramTree[gi]
            snap[g.nodeId] = g.expanded
            for (var ci = 0; ci < g.children.length; ++ci)
                snap[g.children[ci].nodeId] = g.children[ci].expanded
        }
        return snap
    }

    function applyExpandedSnapshot() {
        // Upstream ParamsNode::FinishSearch (:536-545) restores the expansion
        // each node had before the search started.
        for (var gi = 0; gi < root.paramTree.length; ++gi) {
            var g = root.paramTree[gi]
            if (g.nodeId in root.expandedSnapshot)
                g.expanded = root.expandedSnapshot[g.nodeId]
            for (var ci = 0; ci < g.children.length; ++ci) {
                var s = g.children[ci]
                if (s.nodeId in root.expandedSnapshot)
                    s.expanded = root.expandedSnapshot[s.nodeId]
            }
        }
    }

    function nodeMatches(node) {
        return node.text.toLowerCase().indexOf(root.searchText) !== -1
    }

    // Rebuild the flat rows shown by the ListView.
    //  - No search: honor each group/subgroup expanded flag.
    //  - Searching (upstream ParamsModel::RefreshSearch :638-664): only nodes
    //    with a matching param in their subtree stay enabled/visible, the
    //    tree renders fully expanded, and matching params get their hit
    //    substring highlighted.
    function rebuildDisplayRows() {
        var rows = []
        for (var gi = 0; gi < root.paramTree.length; ++gi) {
            var g = root.paramTree[gi]
            var gRows = []
            var groupHasMatch = false
            for (var ci = 0; ci < g.children.length; ++ci) {
                var child = g.children[ci]
                if (child.kind === "subgroup") {
                    var sRows = []
                    var subHasMatch = false
                    for (var pi = 0; pi < child.children.length; ++pi) {
                        var p = child.children[pi]
                        if (root.searching && !nodeMatches(p))
                            continue
                        if (root.searching)
                            subHasMatch = true
                        sRows.push(p)
                    }
                    if (root.searching && !subHasMatch)
                        continue
                    if (root.searching)
                        groupHasMatch = true
                    if (!root.searching && !child.expanded)
                        continue
                    gRows.push(child)
                    for (var si = 0; si < sRows.length; ++si)
                        gRows.push(sRows[si])
                } else {
                    if (root.searching && !nodeMatches(child))
                        continue
                    if (root.searching)
                        groupHasMatch = true
                    gRows.push(child)
                }
            }
            if (root.searching && !groupHasMatch)
                continue
            rows.push(g)
            for (var ri = 0; ri < gRows.length; ++ri)
                rows.push(gRows[ri])
        }

        for (var fi = 0; fi < rows.length; ++fi)
            rows[fi].richText = rowRichText(rows[fi])
        root.displayRows = rows

        // Keep the visual selection on the same node across rebuilds.
        paramListView.currentIndex = -1
        if (root.selectedNodeId !== "") {
            for (var i = 0; i < rows.length; ++i) {
                if (rows[i].nodeId === root.selectedNodeId) {
                    paramListView.currentIndex = i
                    break
                }
            }
        }
    }

    // Upstream on_search_update (:141-146) -> RefreshSearch / FinishSearch.
    function updateFilter(text) {
        var q = text.toLowerCase()
        if (q === "") {
            if (root.searching) {
                applyExpandedSnapshot()
                root.searching = false
                root.expandedSnapshot = {}
            }
        } else if (!root.searching) {
            // Upstream ParamsModel::RefreshSearch saves the expansion state of
            // every node the first time a search runs (:510-517, :643-646).
            root.expandedSnapshot = captureExpanded()
            root.searching = true
        }
        root.searchText = q
        rebuildDisplayRows()
    }

    // Selection label panel -- upstream selection_changed (:384-391):
    //   "key\n(type)" / "Full label > label\n(type)" / "label\n(type)".
    function selectionLabelText(p) {
        var fl = p.fullLabel
        var l = p.label
        if (fl === "" && l === "")
            return p.key + "\n(" + p.typeStr + ")"
        if (fl !== "" && l !== "")
            return fl + " > " + l + "\n(" + p.typeStr + ")"
        return (l === "" ? fl : l) + "\n(" + p.typeStr + ")"
    }

    function selectParam(node, index) {
        root.selectedNodeId = node.nodeId
        root.selectedParamCode = node.code
        root.selectedParamLabel = selectionLabelText(node)
        root.selectedParamDesc = node.tooltip
        paramListView.currentIndex = index
    }

    // -- Insert the selected placeholder at the cursor; mirrors upstream
    //    add_selected_value_to_gcode (EditGCodeDialog.cpp:315-332): a "\n"
    //    prefix when writing at the very end, then for values ending in "]"
    //    either the cursor moves into the (empty) brackets or -- the
    //    upstream "[current_extruder]" branch, unreachable in that dialog
    //    since FilamentVector is never constructed -- the 16-char default
    //    suffix gets selected. Kept for parity. --
    function insertSelectedParam() {
        var val = root.selectedParamCode
        if (val === "" || !gcodeEditor)
            return

        var cursorPos = gcodeEditor.cursorPosition
        var atEnd = (cursorPos >= gcodeEditor.text.length)
        var insertText = atEnd ? "\n" + val : val

        gcodeEditor.insert(cursorPos, insertText)

        if (val.charAt(val.length - 1) === "]") {
            var newPos = gcodeEditor.cursorPosition
            if (val.charAt(val.length - 2) === "[")
                gcodeEditor.cursorPosition = newPos - 1          // into the brackets
            else
                gcodeEditor.select(newPos - 17, newPos - 1)      // "current_extruder"
        }

        gcodeEditor.forceActiveFocus()
    }

    function loadPlaceholderGroups() {
        // Real ConfigDef-driven placeholder tree (upstream init_params_list).
        var vm = backend && backend.configViewModel ? backend.configViewModel : null
        root.groupData = vm ? vm.editGcodeParamGroups(root.optionKey) : []
    }

    contentItem: ColumnLayout {
        spacing: root.dlgBorder
        anchors.fill: parent
        anchors.leftMargin: root.dlgBorder
        anchors.rightMargin: root.dlgBorder
        anchors.topMargin: root.dlgBorder

        // Top label (aligns with upstream "Built-in placeholders (Double click item to add to G-code)",
        // EditGCodeDialog.cpp:40 + :108)
        Text {
            Layout.fillWidth: true
            text: qsTr("内置占位符（双击项添加到 G-code）：")
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSM
            wrapMode: Text.Wrap
        }

        // Grid row: parameter column | add button | editor (1:2 weight split)
        Item {
            id: gridRow
            Layout.fillWidth: true
            Layout.fillHeight: true

            // -- Parameter column (grid col 0, growable weight 1) --
            ColumnLayout {
                id: paramColumn
                x: 0
                y: 0
                width: Math.floor((gridRow.width - addBtn.width - 2 * root.gridHGap) / 3)
                height: parent.height
                spacing: 0

                // Search field -- upstream wxSearchCtrl with the search
                // button, cancel button and descriptive text
                // (EditGCodeDialog.cpp:54-59), framed by wxALL border 10
                // (:68).
                Item {
                    id: searchBox
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.controlHeightSM
                    Layout.leftMargin: root.dlgBorder
                    Layout.rightMargin: root.dlgBorder
                    Layout.topMargin: root.dlgBorder
                    Layout.bottomMargin: root.dlgBorder

                    CxTextField {
                        id: searchField
                        anchors.fill: parent
                        leftPadding: Theme.spacingXXL
                        rightPadding: searchCancelBtn.visible ? 26 : Theme.spacingMD
                        placeholderText: qsTr("搜索 G-code 占位符...")
                        onTextChanged: root.updateFilter(text)
                    }

                    Image {
                        anchors.left: parent.left
                        anchors.leftMargin: Theme.spacingSM
                        anchors.verticalCenter: parent.verticalCenter
                        width: 14
                        height: 14
                        source: "qrc:/qml/assets/icons/editgcode_search.svg"
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                    }

                    // Cancel (clear) button -- upstream ShowCancelButton(true)
                    Item {
                        id: searchCancelBtn
                        anchors.right: parent.right
                        anchors.rightMargin: Theme.spacingXS
                        anchors.verticalCenter: parent.verticalCenter
                        width: 16
                        height: 16
                        visible: searchField.text.length > 0

                        Image {
                            anchors.fill: parent
                            anchors.margins: Theme.spacingXS
                            source: "qrc:/qml/assets/icons/x.svg"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: searchField.text = ""
                        }
                    }
                }

                // Parameter tree (upstream ParamsViewCtrl single-column
                // BitmapTextRenderer tree, EditGCodeDialog.cpp:816-841)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: root.dlgBorder
                    Layout.rightMargin: root.dlgBorder
                    Layout.bottomMargin: root.dlgBorder
                    color: Theme.bgInset
                    radius: Theme.radiusSM
                    border.color: Theme.borderSubtle
                    border.width: 1
                    clip: true

                    ListView {
                        id: paramListView
                        anchors.fill: parent
                        anchors.margins: Theme.spacingXS
                        model: root.displayRows
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: Rectangle {
                            id: delegateRoot
                            required property int index
                            required property var modelData
                            width: paramListView.width
                            height: delegateRoot.modelData.kind === "param" ? 22 : 26
                            color: {
                                if (delegateRoot.modelData.kind !== "param") return "transparent"
                                if (paramListView.currentIndex === delegateRoot.index) return Theme.accentSubtle
                                if (delegateMa.containsMouse) return Theme.bgHover
                                return "transparent"
                            }

                            Row {
                                anchors.fill: parent
                                anchors.leftMargin: delegateRoot.modelData.depth * 16 + Theme.spacingSM
                                anchors.rightMargin: Theme.spacingSM
                                spacing: Theme.spacingXS

                                // Expander arrow (container rows only)
                                Text {
                                    visible: delegateRoot.modelData.kind !== "param"
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: delegateRoot.modelData.kind !== "param" && delegateRoot.modelData.expanded ? "▼" : "▶"
                                    color: Theme.textTertiary
                                    font.pixelSize: Theme.fontSizeXS
                                }

                                // Node bitmap (upstream icon_name per group /
                                // lock subgroup / ParamsInfo param type)
                                Image {
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 16
                                    height: 16
                                    source: delegateRoot.modelData.icon
                                    fillMode: Image.PreserveAspectFit
                                    smooth: true
                                }

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: parent.width - x - Theme.spacingXS
                                    text: delegateRoot.modelData.richText
                                    textFormat: Text.RichText
                                    color: delegateRoot.modelData.kind === "param"
                                           ? (paramListView.currentIndex === delegateRoot.index ? Theme.textPrimary : Theme.textSecondary)
                                           : Theme.textPrimary
                                    font.pixelSize: Theme.fontSizeSM
                                    font.family: Theme.fontMono
                                    elide: Text.ElideRight
                                }
                            }

                            MouseArea {
                                id: delegateMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor

                                onClicked: {
                                    if (delegateRoot.modelData.kind !== "param") {
                                        // During a search the tree is fully
                                        // expanded by the search itself
                                        // (upstream RefreshSearch ExpandChildren).
                                        if (!root.searching) {
                                            delegateRoot.modelData.expanded = !delegateRoot.modelData.expanded
                                            root.rebuildDisplayRows()
                                        }
                                    } else {
                                        root.selectParam(delegateRoot.modelData, delegateRoot.index)
                                    }
                                }

                                onDoubleClicked: {
                                    if (delegateRoot.modelData.kind === "param") {
                                        root.selectParam(delegateRoot.modelData, delegateRoot.index)
                                        root.insertSelectedParam()
                                    }
                                }
                            }
                        }

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }
                    }
                }
            }

            // -- Add button (grid col 1): upstream ScalableButton "add_copies"
            //    bitmap-only + tooltip (EditGCodeDialog.cpp:75-76). Always
            //    enabled -- add_selected_value_to_gcode() returns early on an
            //    empty selection (:317-319). --
            CxIconButton {
                id: addBtn
                x: paramColumn.width + root.gridHGap
                anchors.verticalCenter: parent.verticalCenter
                iconSource: "qrc:/qml/assets/icons/add_copies.svg"
                iconSize: 16
                toolTipText: qsTr("将选中的占位符添加到 G-code")
                onClicked: root.insertSelectedParam()
            }

            // -- G-code editor (grid col 2, growable weight 2) --
            Rectangle {
                id: editorFrame
                x: paramColumn.width + addBtn.width + 2 * root.gridHGap
                y: 0
                width: gridRow.width - x
                height: parent.height
                color: Theme.bgBase
                radius: Theme.radiusSM
                border.color: gcodeEditor.activeFocus ? Theme.borderFocus : Theme.borderSubtle
                border.width: 1

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingXXS
                    clip: true

                    TextArea {
                        id: gcodeEditor
                        width: parent.width
                        height: parent.height
                        placeholderText: qsTr("在此编辑自定义 G-code...")
                        text: root.initialGCode
                        wrapMode: TextArea.Wrap
                        font.family: Theme.fontMono
                        font.pixelSize: Theme.fontSizeMD
                        color: Theme.textPrimary
                        selectionColor: Theme.accentSubtle
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

        // -- Selection label (full width below the grid; upstream
        //    m_param_label, EditGCodeDialog.cpp:96-97/:107, bold font) --
        Text {
            Layout.fillWidth: true
            text: root.selectedParamLabel !== "" ? root.selectedParamLabel : qsTr("选择占位符")
            color: Theme.textPrimary
            font.pixelSize: Theme.fontSizeSM
            font.bold: root.selectedParamLabel !== ""
            wrapMode: Text.Wrap
        }

        // -- Selection description (full width; upstream m_param_description,
        //    :98/:108, real ConfigDef tooltip) --
        Text {
            Layout.fillWidth: true
            text: root.selectedParamDesc
            color: Theme.textTertiary
            font.pixelSize: Theme.fontSizeXS
            wrapMode: Text.Wrap
            visible: root.selectedParamDesc !== ""
        }
    }

    footer: Rectangle {
        width: parent.width
        height: 48
        color: Theme.bgSurface
        radius: Theme.radiusLG
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 12
            color: parent.color
        }

        // Upstream DialogButtons(this, {"OK", "Cancel"}) (EditGCodeDialog.cpp:
        // :101/:109): right-aligned row in list order -- OK (Confirm primary
        // style, :104-105) then Cancel, separated by ChoiceButtonGap() = 10
        // (DialogButtons.cpp:141-163, Button.hpp:12).
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacingXL
            anchors.rightMargin: root.dlgBorder
            spacing: root.dlgBorder
            Item { Layout.fillWidth: true }

            CxButton {
                text: qsTr("确定")
                cxStyle: CxButton.Style.Primary
                onClicked: {
                    root.gcodeAccepted(gcodeEditor.text)
                    root.accept()
                }
            }

            CxButton {
                text: qsTr("取消")
                cxStyle: CxButton.Style.Secondary
                onClicked: root.reject()
            }
        }

        // Height resize strip along the footer bottom edge (the upstream
        // wxRESIZE_BORDER frame has no Qt Quick Popup equivalent).
        MouseArea {
            id: resizeHeightArea
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 6
            cursorShape: Qt.SizeVerCursor
            property real pressGlobalY
            property real pressHeight
            onPressed: (mouse) => {
                pressGlobalY = mapToItem(Overlay.overlay, mouse.x, mouse.y).y
                pressHeight = root.height
            }
            onPositionChanged: (mouse) => {
                if (!pressed)
                    return
                const gy = mapToItem(Overlay.overlay, mouse.x, mouse.y).y
                root.height = Math.min(root.maxDialogHeight,
                                       Math.max(root.minDialogHeight,
                                                Math.round(pressHeight + gy - pressGlobalY)))
            }
        }

        // Bottom-right diagonal resize grip. 7px tall: flush with the button
        // row's bottom gap (34px buttons in a 48px bar) so it never overlaps
        // the Cancel button's clickable area.
        MouseArea {
            id: resizeCornerArea
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: 24
            height: 7
            cursorShape: Qt.SizeFDiagCursor
            property real pressGlobalX
            property real pressGlobalY
            property real pressWidth
            property real pressHeight
            onPressed: (mouse) => {
                const gp = mapToItem(Overlay.overlay, mouse.x, mouse.y)
                pressGlobalX = gp.x
                pressGlobalY = gp.y
                pressWidth = root.width
                pressHeight = root.height
            }
            onPositionChanged: (mouse) => {
                if (!pressed)
                    return
                const gp = mapToItem(Overlay.overlay, mouse.x, mouse.y)
                root.width = Math.min(root.maxDialogWidth,
                                      Math.max(root.minDialogWidth,
                                               Math.round(pressWidth + gp.x - pressGlobalX)))
                root.height = Math.min(root.maxDialogHeight,
                                       Math.max(root.minDialogHeight,
                                                Math.round(pressHeight + gp.y - pressGlobalY)))
            }
        }
    }

    // Right-edge width resize strip. The base background styling is
    // reproduced here because the resize handle must live in the popup's
    // background layer to receive clicks along the frame edge.
    background: Rectangle {
        color: Theme.bgElevated
        border.color: Theme.borderInput
        border.width: 1
        radius: Theme.radiusLG

        MouseArea {
            id: resizeWidthArea
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 6
            cursorShape: Qt.SizeHorCursor
            property real pressGlobalX
            property real pressWidth
            onPressed: (mouse) => {
                pressGlobalX = mapToItem(Overlay.overlay, mouse.x, mouse.y).x
                pressWidth = root.width
            }
            onPositionChanged: (mouse) => {
                if (!pressed)
                    return
                const gx = mapToItem(Overlay.overlay, mouse.x, mouse.y).x
                root.width = Math.min(root.maxDialogWidth,
                                      Math.max(root.minDialogWidth,
                                               Math.round(pressWidth + gx - pressGlobalX)))
            }
        }
    }

    // Initialize on opened
    onOpened: {
        gcodeEditor.text = root.initialGCode
        gcodeEditor.cursorPosition = gcodeEditor.text.length
        root.selectedNodeId = ""
        root.selectedParamCode = ""
        root.selectedParamLabel = ""
        root.selectedParamDesc = ""
        searchField.text = ""
        loadPlaceholderGroups()
        rebuildTree()
        rebuildDisplayRows()
    }
}
