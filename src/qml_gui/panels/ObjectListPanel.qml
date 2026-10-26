pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import OWzx

// ─────────────────────────────────────────────────────────────────────────────
// ObjectListPanel.qml — X6 object-list panel (spec: 开发计划 §6c X6; upstream
// truth: docs/ui-reference/object-list-parity.md).
//
// Upstream position parity (Plater.cpp:3234, 3243-3245): the ObjectList is a
// section INSIDE the settings sidebar (above the params area, below the
// search bar), not a standalone right rail. This panel mounts into
// LeftSidebar between the sliced-info card and the params panel and rides
// DockableSidebar's left/right docking -- same-position parity with the
// upstream sidebar section.
//
// Data: EditorViewModel.sceneTreeModel (SceneTreeModel, a mirror of upstream
// ObjectDataViewModel: plates -> objects -> volumes + Instances -> Instance N).
// All mutations route back through the VM's undo-wired invokables
// (renameObject / moveObject / toggleObjectPrintableAt / toggleInstancePrintable
// -- RenameCommand / MoveObjectCommand / PrintableCommand; upstream
// "Rename Object" / "Object order changed" / anonymous printable snapshots).
//
// Cx* discipline (X2): the only hand-drawn visuals here are the row cells
// themselves (text + highlight rectangles + the expander glyph); every
// interactive control is a system control (CxIconButton, CxTextField).
//
// Known deviations (registered in object-list-parity.md §7, not re-derived
// here): layer/settings/info rows live in dialogs + canvas gizmos; volume
// rows are read-only (no renameVolume/moveVolume service surface); plate
// rename stays on the canvas pencil; double-click zoom needs a
// selection-scoped fit channel (panel double-click only re-selects).
// ─────────────────────────────────────────────────────────────────────────────
Rectangle {
    id: root

    required property var editorVm

    // Resolved by the panel itself through the shared VM surface (the same
    // synchronizeViewportContext the canvas right-click uses); bubbled up so
    // PreparePage can popup the SAME PrepareContextMenus instance the canvas
    // uses (= upstream "object list and canvas share one menu family",
    // parity doc §6). Coordinates are scene coordinates.
    signal contextMenuResolved(int family, real menuX, real menuY)

    // Upstream node-type ids (SceneTreeModel::NodeType mirrors
    // ObjectDataViewModel.hpp:25-36; kept as literals because the model type
    // is not QML-registered).
    readonly property int nodePlate: 1
    readonly property int nodeObject: 2
    readonly property int nodeVolume: 4
    readonly property int nodeInstanceRoot: 8
    readonly property int nodeInstance: 16

    // ViewportContextTarget ids (ViewportContextHit.h) for the context-menu
    // routing (same values the canvas contextMenuRequested forwards).
    readonly property int targetObject: 0
    readonly property int targetPart: 1
    readonly property int targetPlate: 2

    readonly property int rowHeight: 26

    color: Theme.bgElevated
    radius: Theme.radiusSM
    border.width: 1
    border.color: Theme.bgPressed

    // ── inline rename state ──────────────────────────────────────────────────
    // Upstream rename entry = the shared context menu's "Rename" item
    // (GUI_Factories.cpp:1422 append_menu_item_rename -> rename_item,
    // GUI_ObjectList.cpp:6080). Set from outside (canvas menu OR panel row
    // menu) via beginRenameSelection(); -1 = no edit in flight.
    property int editingSourceIndex: -1
    function beginRenameSelection() {
        if (!root.editorVm)
            return
        const idx = root.editorVm.selectedSourceObjectIndex
        if (idx >= 0)
            root.editingSourceIndex = idx
    }

    // ── expansion persistence across model resets ────────────────────────────
    // The VM topology cluster (projectChanged relay) rebuilds the model on
    // every mutation, and a reset collapses the whole view. Upstream wx list
    // cells never collapse on rename, so expansion is remembered per node
    // identity (plate/object/instance-root key) and re-applied after each
    // reset. Defaults mirror the upstream fresh-load look: plates expanded
    // (their objects visible), object children collapsed.
    property var _userExpansion: ({})   // key -> true(expanded)/false(collapsed)

    function _expansionKey(type, plateIdx, srcIdx) {
        if (type === root.nodePlate)
            return "p" + plateIdx
        if (type === root.nodeObject)
            return "o" + srcIdx
        if (type === root.nodeInstanceRoot)
            return "ir" + srcIdx
        return ""
    }

    function _reapplyExpansion() {
        const m = treeView.model
        if (!m || !root.editorVm)
            return
        // Pass 1: plate rows are root children and sit at rows 0..plateCount-1
        // while everything below them is still collapsed.
        const plateCount = root.editorVm.plateCount
        for (let row = 0; row < plateCount && row < treeView.rows; ++row) {
            const idx = treeView.index(row, 0)
            if (m.nodeTypeAt(idx) !== root.nodePlate)
                continue
            const key = root._expansionKey(root.nodePlate, m.plateIndexAt(idx), -1)
            const want = key in root._userExpansion ? root._userExpansion[key] : true
            if (want !== treeView.isExpanded(row)) {
                if (want) treeView.expand(row)
                else treeView.collapse(row)
            }
        }
        // Pass 2: object / instance-root rows (only materialized where their
        // parents are expanded; expanding a found row can reveal more rows,
        // so loop until a full sweep adds nothing).
        var expandedSomething = true
        while (expandedSomething) {
            expandedSomething = false
            for (let row = 0; row < treeView.rows; ++row) {
                const idx = treeView.index(row, 0)
                const type = m.nodeTypeAt(idx)
                if (type !== root.nodeObject && type !== root.nodeInstanceRoot)
                    continue
                const key = root._expansionKey(type, -1, m.sourceObjectIndexAt(idx))
                const want = root._userExpansion[key] === true
                if (want && !treeView.isExpanded(row)) {
                    treeView.expand(row)
                    expandedSomething = true
                }
            }
        }
    }

    Connections {
        target: treeView.model
        function onModelReset() {
            // Defer until the view has laid the reset rows out.
            Qt.callLater(root._reapplyExpansion)
        }
    }

    // ── drag-reorder state (upstream OnDrop, parity doc §2) ──────────────────
    // Object rows only; single-selection drags; same-plate targets only
    // (GUI_ObjectList.cpp:1847-1851, :1920-1939). Drop semantics = the dragged
    // object takes the target row's slot (moveObject swap-walk).
    property int dragSourceIndex: -1
    property int dragPlateIndex: -1
    property int dropTargetRow: -1

    function _updateDropTarget(fromItem, localX, localY) {
        root.dropTargetRow = -1
        if (root.dragSourceIndex < 0 || !treeView.model)
            return
        const pos = fromItem.mapToItem(treeView.contentItem, localX, localY)
        const visualRow = Math.floor(pos.y / root.rowHeight)
        if (visualRow < 0 || visualRow >= treeView.rows)
            return
        const idx = treeView.index(visualRow, 0)
        const m = treeView.model
        if (m.nodeTypeAt(idx) !== root.nodeObject)
            return
        const targetSource = m.sourceObjectIndexAt(idx)
        if (targetSource === root.dragSourceIndex)
            return
        if (m.plateIndexAt(idx) !== root.dragPlateIndex)
            return // upstream can_drop: cross-plate drags are rejected
        root.dropTargetRow = visualRow
    }

    function _finishDrag() {
        const targetRow = root.dropTargetRow
        const from = root.dragSourceIndex
        root.dragSourceIndex = -1
        root.dragPlateIndex = -1
        root.dropTargetRow = -1
        if (targetRow < 0 || from < 0 || !treeView.model || !root.editorVm)
            return
        const idx = treeView.index(targetRow, 0)
        root.editorVm.moveObject(from, treeView.model.sourceObjectIndexAt(idx))
    }

    // ── context-menu routing (shared family with the canvas, parity §6) ──────
    function _openContextMenu(type, srcObj, volIdx, instIdx, plateIdx, fromItem, localX, localY) {
        if (!root.editorVm)
            return
        var kind = root.targetObject
        if (type === root.nodePlate)
            kind = root.targetPlate
        else if (type === root.nodeVolume)
            kind = root.targetPart
        const family = root.editorVm.synchronizeViewportContext(
                    kind, srcObj, volIdx, instIdx, plateIdx)
        if (family < 0)
            return
        const scene = fromItem.mapToItem(null, localX, localY)
        root.contextMenuResolved(family, scene.x, scene.y)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TreeView {
            id: treeView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.editorVm ? root.editorVm.sceneTreeModel : null
            // The object list column is one composed cell (name + state
            // icons), matching upstream's single tree column
            // (BitmapTextRenderer composition, GUI_ObjectList.cpp:551-576).
            // TreeView's column count derives from the model (read-only).

            onExpanded: function(row, depth) {
                const m = treeView.model
                if (!m)
                    return
                const idx = treeView.index(row, 0)
                const key = root._expansionKey(m.nodeTypeAt(idx), m.plateIndexAt(idx),
                                               m.sourceObjectIndexAt(idx))
                if (key !== "")
                    root._userExpansion[key] = true
            }
            onCollapsed: function(row, recursively) {
                const m = treeView.model
                if (!m)
                    return
                const idx = treeView.index(row, 0)
                const key = root._expansionKey(m.nodeTypeAt(idx), m.plateIndexAt(idx),
                                               m.sourceObjectIndexAt(idx))
                if (key !== "")
                    root._userExpansion[key] = false
            }

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Item {
                id: treeRow

                required property bool expanded
                required property bool hasChildren
                required property int depth
                required property int index

                required property int nodeType
                required property string display
                required property bool printable
                // "objectSelected" model role -- NOT `selected`, which
                // TreeView injects as a view-driven selection state.
                required property bool objectSelected
                required property bool isCurrentPlate
                required property int sourceObjectIndex
                required property int volumeIndex
                required property int instanceIndex
                required property int plateIndex
                required property int extruderId
                required property string volumeTypeLabel

                implicitHeight: root.rowHeight
                // F4: TableView warns on zero delegate implicitWidth ("needs
                // to be greater than zero") -- width alone is not enough.
                implicitWidth: treeView.width
                width: treeView.width

                readonly property bool isDragSource:
                    root.dragSourceIndex === treeRow.sourceObjectIndex
                        && treeRow.nodeType === root.nodeObject
                readonly property bool isDropTarget: root.dropTargetRow === treeRow.index

                // Row highlight: object selection (VM selection set) and the
                // drop-target underline during drags. Plate rows carry the
                // current-plate band instead.
                Rectangle {
                    anchors.fill: parent
                    visible: treeRow.nodeType === root.nodeObject && treeRow.objectSelected
                    color: Theme.bgPressed
                }
                Rectangle {
                    anchors.fill: parent
                    visible: treeRow.nodeType === root.nodePlate && treeRow.isCurrentPlate
                    color: Theme.bgHover
                }
                // Drop indicator: 2px accent band on the target row's top
                // edge (insertion before the hovered row).
                Rectangle {
                    visible: treeRow.isDropTarget
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: 2
                    color: Theme.accent
                }
                Rectangle {
                    visible: treeRow.isDropTarget
                    anchors.fill: parent
                    color: "transparent"
                    border.width: 1
                    border.color: Theme.accent
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6 + treeRow.depth * 14
                    anchors.rightMargin: 6
                    spacing: 4
                    opacity: treeRow.isDragSource ? 0.45 : 1.0

                    // Expander (upstream branch arrows). Instance/leaf rows
                    // keep the slot for column alignment.
                    Item {
                        Layout.preferredWidth: 12
                        Layout.preferredHeight: 12
                        Layout.alignment: Qt.AlignVCenter

                        Text {
                            anchors.centerIn: parent
                            visible: treeRow.hasChildren
                            text: "▸"
                            color: Theme.textTertiary
                            font.pixelSize: Theme.fontSizeXS
                            rotation: treeRow.expanded ? 90 : 0
                            Behavior on rotation { NumberAnimation { duration: Theme.motionFast } }
                        }
                        MouseArea {
                            anchors.fill: parent
                            enabled: treeRow.hasChildren
                            cursorShape: Qt.PointingHandCursor
                            onClicked: treeView.toggleExpanded(treeRow.index)
                        }
                    }

                    // Node glyph: plate color chip / object box / volume type
                    // tag / instance dot (upstream colName icon column).
                    Rectangle {
                        visible: treeRow.nodeType === root.nodePlate
                        Layout.preferredWidth: 10
                        Layout.preferredHeight: 10
                        Layout.alignment: Qt.AlignVCenter
                        radius: 2
                        color: root.editorVm
                               ? root.editorVm.plateThumbnailColor(treeRow.plateIndex)
                               : Theme.textTertiary
                    }
                    Image {
                        visible: treeRow.nodeType === root.nodeObject
                        Layout.preferredWidth: 14
                        Layout.preferredHeight: 14
                        Layout.alignment: Qt.AlignVCenter
                        source: "qrc:/qml/assets/icons/box.svg"
                        fillMode: Image.PreserveAspectFit
                        opacity: 0.8
                    }
                    Rectangle {
                        visible: treeRow.nodeType === root.nodeInstance
                        Layout.preferredWidth: 6
                        Layout.preferredHeight: 6
                        Layout.alignment: Qt.AlignVCenter
                        radius: Theme.radiusSM
                        color: Theme.textTertiary
                    }
                    Text {
                        visible: treeRow.nodeType === root.nodeVolume
                        Layout.preferredWidth: 14
                        Layout.preferredHeight: 14
                        Layout.alignment: Qt.AlignVCenter
                        text: "◆"
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                    }

                    // Name cell: inline editor while renaming this object row
                    // (upstream in-list rename, parity §3); read text with
                    // type suffix otherwise.
                    CxTextField {
                        visible: treeRow.nodeType === root.nodeObject
                                 && root.editingSourceIndex === treeRow.sourceObjectIndex
                        Layout.fillWidth: true
                        implicitHeight: 20
                        font.pixelSize: Theme.fontSizeSM
                        padding: 2
                        text: treeRow.display
                        onVisibleChanged: {
                            // Focus when the edit starts (the delegate may
                            // already exist when beginRenameSelection fires).
                            if (visible) {
                                selectAll()
                                forceActiveFocus()
                            }
                        }
                        onAccepted: {
                            if (root.editorVm)
                                root.editorVm.renameObject(treeRow.sourceObjectIndex, text.trim())
                            root.editingSourceIndex = -1
                        }
                        onEditingFinished: {
                            // Upstream OnEditingDone commits unless cancelled
                            // (GUI_ObjectList.cpp:6564-6597); Esc-only cancels
                            // (see Keys handler).
                            if (root.editingSourceIndex === treeRow.sourceObjectIndex) {
                                if (root.editorVm && text.trim().length > 0
                                        && text !== treeRow.display)
                                    root.editorVm.renameObject(treeRow.sourceObjectIndex, text.trim())
                                root.editingSourceIndex = -1
                            }
                        }
                        Keys.onEscapePressed: root.editingSourceIndex = -1
                    }
                    Text {
                        visible: !(treeRow.nodeType === root.nodeObject
                                   && root.editingSourceIndex === treeRow.sourceObjectIndex)
                        Layout.fillWidth: true
                        text: treeRow.nodeType === root.nodeVolume
                              ? treeRow.display + " · " + treeRow.volumeTypeLabel
                              : treeRow.display
                        color: treeRow.nodeType === root.nodePlate
                               ? Theme.textPrimary
                               : (treeRow.nodeType === root.nodeObject
                                  ? Theme.textPrimary : Theme.textSecondary)
                        font.pixelSize: Theme.fontSizeSM
                        font.bold: treeRow.nodeType === root.nodePlate
                        elide: Text.ElideRight
                    }

                    // Extruder readout (upstream colFilament as a read-only
                    // number; edits go through the Change Filament menu,
                    // parity §7).
                    Text {
                        visible: treeRow.nodeType === root.nodeObject && treeRow.extruderId >= 0
                        text: String(treeRow.extruderId + 1)
                        color: Theme.textTertiary
                        font.pixelSize: Theme.fontSizeXS
                        Layout.alignment: Qt.AlignVCenter
                    }

                    // Printable eye/lock (upstream colPrint). ONE engine flag
                    // covers show/hide AND lock (R-P0.5 single-flag engine;
                    // parity §4) -- open padlock = printable/visible, closed
                    // = excluded from slicing. Object rows flip the whole
                    // object (toggleObjectPrintableAt), instance rows flip
                    // that instance only (upstream :6846-6849).
                    CxIconButton {
                        visible: treeRow.nodeType === root.nodeObject
                                 || treeRow.nodeType === root.nodeInstance
                        buttonSize: 18
                        iconSize: 12
                        cxStyle: CxIconButton.Style.Surface
                        iconSource: treeRow.printable
                                    ? "qrc:/qml/assets/icons/lock_open.svg"
                                    : "qrc:/qml/assets/icons/lock_closed.svg"
                        toolTipText: treeRow.printable ? qsTr("可打印（点击从切片中排除）")
                                                       : qsTr("已排除（点击恢复打印）")
                        onClicked: {
                            if (!root.editorVm)
                                return
                            if (treeRow.nodeType === root.nodeObject)
                                root.editorVm.toggleObjectPrintableAt(treeRow.sourceObjectIndex)
                            else
                                root.editorVm.toggleInstancePrintable(treeRow.sourceObjectIndex,
                                                                      treeRow.instanceIndex)
                        }
                    }
                }

                // Row interaction surface. Object rows additionally carry the
                // drag-reorder gesture (press selects, >6px move starts the
                // drag, release on a valid same-plate row commits the move).
                MouseArea {
                    id: rowMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    enabled: !(treeRow.nodeType === root.nodeObject
                               && root.editingSourceIndex === treeRow.sourceObjectIndex)

                    property real _pressX: 0
                    property real _pressY: 0
                    property bool _dragArmed: false

                    onPressed: function(mouse) {
                        if (mouse.button === Qt.RightButton) {
                            root._openContextMenu(treeRow.nodeType, treeRow.sourceObjectIndex,
                                                   treeRow.volumeIndex, treeRow.instanceIndex,
                                                   treeRow.plateIndex, rowMouse, mouse.x, mouse.y)
                            return
                        }
                        if (!root.editorVm)
                            return
                        rowMouse._pressX = mouse.x
                        rowMouse._pressY = mouse.y
                        rowMouse._dragArmed = treeRow.nodeType === root.nodeObject
                        // wx list: selection lands on left-down; drags start
                        // only after the threshold.
                        root._selectRow(treeRow, mouse.modifiers)
                    }
                    onPositionChanged: function(mouse) {
                        if (!pressed || !rowMouse._dragArmed)
                            return
                        const dx = mouse.x - rowMouse._pressX
                        const dy = mouse.y - rowMouse._pressY
                        if (root.dragSourceIndex < 0
                                && (dx * dx + dy * dy) > 36) {
                            root.dragSourceIndex = treeRow.sourceObjectIndex
                            root.dragPlateIndex = treeRow.plateIndex
                        }
                        if (root.dragSourceIndex >= 0)
                            root._updateDropTarget(rowMouse, mouse.x, mouse.y)
                    }
                    onReleased: {
                        if (root.dragSourceIndex >= 0)
                            root._finishDrag()
                        rowMouse._dragArmed = false
                    }
                    onDoubleClicked: function(mouse) {
                        if (mouse.button !== Qt.LeftButton)
                            return
                        // Upstream double-click = zoom_to_selection
                        // (:495-503); the Qt side has no selection-scoped
                        // fit channel yet (parity §7) -- re-affirm the
                        // selection so the gesture is not a dead click.
                        if (root.editorVm)
                            root._selectRow(treeRow, mouse.modifiers)
                    }
                    onCanceled: {
                        root.dragSourceIndex = -1
                        root.dropTargetRow = -1
                        rowMouse._dragArmed = false
                    }
                }
            }
        }
    }

    // Row selection routing (upstream selection_changed :1229-1266 /
    // update_selections :4814): object rows select by SOURCE index (plain
    // click replaces, Ctrl toggles); volume rows select the volume on the
    // canvas via selectVolumeBySource -- the SOURCE-index twin of
    // selectVolume() built for callers that carry (sourceObjectIndex,
    // volumeIndex) directly (EditorViewModel.cpp:7804, the viewport-pick
    // pattern; selectVolume maps a FILTERED index through
    // mapFilteredToSourceIndex and would mis-address multi-plate scenes
    // where visibleObjectIndices = current-plate objects); instance rows
    // select the owning object (no instance-scoped selection surface);
    // plate rows switch the current plate (Plater plate selection surface).
    function _selectRow(row, modifiers) {
        if (!root.editorVm)
            return
        if (row.nodeType === root.nodeObject) {
            if (modifiers & Qt.ControlModifier)
                root.editorVm.toggleSourceObjectSelection(row.sourceObjectIndex)
            else
                root.editorVm.selectSourceObject(row.sourceObjectIndex)
        } else if (row.nodeType === root.nodeVolume) {
            root.editorVm.selectVolumeBySource(row.sourceObjectIndex, row.volumeIndex)
        } else if (row.nodeType === root.nodeInstance) {
            root.editorVm.selectSourceObject(row.sourceObjectIndex)
        } else if (row.nodeType === root.nodePlate) {
            // MP6 (dev plan 多盘 2026-10-20): the synthetic Outside group
            // (plateIndex -1) is not a switchable plate.
            if (row.plateIndex >= 0)
                root.editorVm.setCurrentPlateIndex(row.plateIndex)
        }
    }
}
